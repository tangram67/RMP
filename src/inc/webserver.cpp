/*
 * webserver.cpp
 *
 *  Created on: 03.02.2015
 *      Author: Dirk Brinkmeier
 */

#include "webserver.h"
#include "stringutils.h"
#include "localizations.h"
#include "webrequest.h"
#include "mimetypes.h"
#include "compare.h"
#include "convert.h"
#include "sockets.h"
#include "globals.h"
#include "typeid.h"
#include "tables.h"
#include "ansi.h"
#include "credentials.h"
#include "microhttpd/internal.h"
#include "../config.h"
#include <cstring>


// Static dispatcher functions forwards to method of TWebServer instance
static MHD_Result acceptHandlerDispatcher( void *cls, const struct sockaddr *addr, socklen_t addrlen ) {
	if (util::assigned(cls)) {
		return (static_cast<app::PWebServer>(cls))->acceptHandler(addr, addrlen);
	}
	return MHD_NO;
}


static MHD_Result requestHandlerDispatcher( void *cls,
									 struct MHD_Connection *connection,
									 const char *url,
									 const char *method,
									 const char *version,
									 const char *upload_data,
									 size_t *upload_data_size,
									 void **con_cls ) {
	if (util::assigned(cls)) {
		return (static_cast<app::PWebServer>(cls))->requestHandler(	connection,
																	url,
																	method,
																	version,
																	upload_data,
																	upload_data_size,
																	con_cls );
	}
	return MHD_NO;
}


static void* logHandlerDispatcher( void *cls, const char *uri, struct MHD_Connection *connection ) {
	if (util::assigned(cls)) {
		(static_cast<app::PWebServer>(cls))->logHandler(uri, connection);
	}
	return nil;
}


static void customErrorLogDispatcher( void *cls, const char *fmt, va_list va ) {
	if (util::assigned(cls)) {
		(static_cast<app::PWebServer>(cls))->customErrorLog(fmt, va);
	}
}


static void requestCompletedCallbackDispatcher ( void *cls,
												 struct MHD_Connection *connection,
												 void **con_cls,
												 enum MHD_RequestTerminationCode toe ) {
	if (util::assigned(cls)) {
		(static_cast<app::PWebServer>(cls))->requestCompletedCallback( connection,
																	   con_cls,
																	   toe );
	}
}


static void panicCallbackDispatcher ( void *cls,
               	  	  	  	  	   	  const char *file,
               	  	  	  	  	   	  unsigned int line,
               	  	  	  	  	   	  const char *reason ) {
	if (util::assigned(cls)) {
		(static_cast<app::PWebServer>(cls))->panicCallback(	file,
															line,
															reason );
	}
}


static void webSocketConnectionDispatcher(void *cls, struct MHD_Connection *connection, void *con_cls, const char *extra_in, size_t extra_in_size, MHD_socket sock, struct MHD_UpgradeResponseHandle *urh) {
	if (util::assigned(cls)) {
		(static_cast<app::PWebServer>(cls))->webSocketHandler(cls, connection, con_cls, extra_in, extra_in_size, sock, urh);
	}
}


namespace app {


TWebServer::TWebServer(const std::string& name, const std::string& documentRoot, app::PIniFile config, app::PThreadController threads, app::PTimerController timers, PLogFile infoLog, PLogFile exceptionLog) {
	this->name = name;
	this->config = config;
	this->infoLog = infoLog;
	this->exceptionLog = exceptionLog;
	this->threads = threads;
	this->timers = timers;
	web.documentRoot = documentRoot;
	credentialCallbackMethod = nil;
	httpsParam = HTTPS_PARAMS;
	httpsCert = HTTPS_CERT;
	httpsKey = HTTPS_KEY;
	certFile = nil;
	keyFile = nil;
	dhFile = nil;
	sockets = nil;
	socketTimer = nil;
	httpServer4 = nil;
	httpServer6 = nil;
	reWriteConfig();
	rest.setRoot(web.restRoot);
	maxUrlSize = util::maxPathSize();
	numa = sysutil::getProcessorCount();
	sessionTimer = timers->addTimer(name, "SessionDeleteTimer", web.sessionDeleteDelay, &app::TWebServer::onSessionTimer, this);
	requestTimer = timers->addTimer(name, "RequestDeleteTimer", web.requestDeleteDelay, &app::TWebServer::onRequestTimer, this);
	bufferTimer = timers->addTimer(name, "BufferDeleteTimer", BUFFER_TIMER_DELAY, &app::TWebServer::onBufferTimer, this);
	statsTimer = timers->addTimer(name, "StatisticsTimer", STATISTICS_TIMER_DELAY, &app::TWebServer::onStatsTimer, this);
	web.rejectedDeleteAge = sessionTimer->getDelay() * 90 / 100;
	executer.setExecHandler(&app::TWebServer::actionAsyncExecuter, this);
	executer.setName("Executer-" + getName());
	defaultToken = new app::TWebToken("default");
	tokenList.setFiles(content);
	mode = ESM_CREATED;
	port = 0;

	// Set "always authenticated" web pages
	if (web.allowManifestFiles) {
		addUrlAuthExclusion("/manifest.json");
		addUrlAuthExclusion("/favicon.ico");
		addUrlAuthExclusion("/images/");
	}
}


TWebServer::~TWebServer() {
	terminate();
	clear();
}


const std::string& TWebServer::getFullURL() const {
	if (rootURL.empty()) {
		std::string address, proto = isSecure() ? "https" : "http";
		util::TStringList addresses;
		sysutil::getLocalIpAddress(addresses);
		address = addresses.empty() ? "127.0.0.1" : addresses[0];
		if (inet::isIPv6Address(address))
			address = "[" + address + "]";
		rootURL = util::csnprintf("%://%:%", proto, address, getPort());
	}
	return rootURL;
};


void TWebServer::update() {
	app::TLockGuard<app::TMutex> mtx(updateMtx, false);

	// Prevent updater called twice!
	if (!mtx.tryLock()) {
		writeInfoLog("[Update web server] Update already running...");
		return;
	}

	// Check if server is ready for update
	if (setAndCompareMode(ESM_SCANNING, ESM_RUNNING)) {
		writeInfoLog("[Update web server] Server changed state to paused.");

		// Wait for all request processed...
		while (!isRequestQueueIdle())
			util::wait(30);

		// Read current token values
		writeInfoLog("[Update web server] [Step 1] Store web token values.");
		TWebTokenValueMap values;
		readTokenList(values);

		// Rescan web root
		writeInfoLog("[Update web server] [Step 2] Rescan web root.");
		scanWebRoot(web.debug);

		// Update file token list and restore values
		writeInfoLog("[Update web server] [Step 3] Update web token values.");
		updateTokenList(values);

		// Cleanup object lists
		writeInfoLog("[Update web server] [Step 4] Cleanup request and session lists.");
		sessionGarbageCollector(true);
		requestGarbageCollector(true);

		// Save current web sessions
		writeInfoLog("[Update web server] [Step 5] Rebuild session store.");
		saveWebSessionsToFile(web.sessionStore);

		// Update status display with current token values
		updateStatusToken();

		// Update finished
		if (setAndCompareMode(ESM_RUNNING, ESM_SCANNING)) {
			writeInfoLog("[Update web server] Server state resumed.");
		} else {
			writeInfoLog("[Update web server] Server state changed during update.");
		}

	} else {
		writeInfoLog("[Update web server] Update skipped due to invalid state.");
	}
}

bool TWebServer::isRequestQueueIdle() {
	std::lock_guard<std::mutex> lock(requestMtx);
	if (!requestList.empty()) {
		PWebRequest request;
		TWebRequestList::const_iterator it = requestList.begin();
		while (it != requestList.end()) {
			request = *it;
			if (util::assigned(request)) {
				if (request->getRefCount() > 0) {
					return false;
				}
			}
			it++;
		}
	}
	return true;
}


void TWebServer::getRunningConfiguration(TWebSettings& config) {
	app::TLockGuard<app::TMutex> mtx(configMtx);
	config = web;
}

void TWebServer::setRunningConfiguration(const TWebSettings& config) {
	{
		app::TLockGuard<app::TMutex> mtx(configMtx);
		web.verbosity = config.verbosity;
		web.refreshInterval = config.refreshInterval;
		web.refreshTimer = config.refreshTimer;
		web.allowWebSockets = config.allowWebSockets;
		writeConfig();
	}
	updateWebToken();
}


bool fileDecider(util::TFile& file) {
	return file.isHTML() || file.isJSON();
}

bool zipDecider(util::TFile& file) {
	if (std::string::npos != file.getMime().find("image"))
		return false;
	if (file.hasToken())
		return false;
	return true;
}

void TWebServer::scanWebRoot(const bool debug) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(requestLck, RWL_WRITE);
	util::TDateTime time;
	time.start();

	// Cache files in web root folder
	content.setDebug(debug);
	util::ELoadType minimize = (web.minimize) ? util::LT_MINIMIZED : util::LT_BINARY;
	size_t n = content.scan(web.documentRoot, util::FLK_URL, true, util::SD_RECURSIVE, minimize);
	if (!content.empty()) {
		writeInfoLog("[File scanner] Scanning <" + web.documentRoot + "> took " + std::to_string((size_u)time.stop(util::ETP_MILLISEC)) + " milliseconds.");
		writeInfoLog("[File scanner] Scanned " + std::to_string((size_u)n) + " files (" + util::sizeToStr(content.getFileSize()) + ") in web root <" + web.documentRoot + ">");

		// Parse HTML content
		time.start();
		int t = content.parse(web.tokenHeader, web.tokenFooter, fileDecider, zipDecider);
		writeInfoLog("[File scanner] Parsing HTML files took " + std::to_string((size_u)time.stop(util::ETP_MILLISEC)) + " milliseconds.");
		writeInfoLog("[File scanner] Parser found " + std::to_string((size_u)t) + " token.");

	} else {
		writeInfoLog("[File scanner] No files found in web root <" + web.documentRoot + ">");
		tokenList.clear();
	}
}

PWebToken TWebServer::addWebToken(const std::string& key, const std::string& value) {
	PWebToken retVal = tokenList.addToken(key);
	if (!util::assigned(retVal)) {
		// Add default token if no file for key found!
		writeErrorLog("[WARNING] No file found containing web token key " + web.tokenHeader + key + web.tokenFooter);
		retVal = defaultToken;
	} else {
		*retVal = value;
	}
	return retVal;
}

inline int isValidURL(char c) {
	unsigned char u = (unsigned char)c;
	if ((u <= USPC) || (u == (unsigned char)'/') || (u == (unsigned char)'\\'))
		return true;
	return false;
}

void TWebServer::addUrlAuthExclusion(const std::string& url) {
	if (!url.empty()) {
		allowedPagesList.add(url);
	}
}

void TWebServer::addRestAuthExclusion(const std::string& api) {
	if (!api.empty()) {
		std::string s = api;
		util::trimLeft(s, isValidURL);
		addUrlAuthExclusion(web.restRoot + s);
	}
}

void TWebServer::setMode(const EServerMode value) {
	std::lock_guard<std::mutex> lock(modeMtx);
	mode = value;
}

EServerMode TWebServer::getMode() const {
	std::lock_guard<std::mutex> lock(modeMtx);
	return mode;
}

bool TWebServer::setAndCompareMode(const EServerMode value, const EServerMode compare) {
	std::lock_guard<std::mutex> lock(modeMtx);
	if (mode == compare) {
		mode = value;
		return true;
	}
	return false;
}

bool TWebServer::isRunning() const {
	std::lock_guard<std::mutex> lock(modeMtx);
	return util::isMemberOf(mode, ESM_RUNNING,ESM_PAUSED);
}

bool TWebServer::isResponding() const {
	std::lock_guard<std::mutex> lock(modeMtx);
	return mode == ESM_RUNNING;
}

bool TWebServer::setTerminateMode() {
	std::lock_guard<std::mutex> lock(modeMtx);
	if (util::isMemberOf(mode, ESM_RUNNING,ESM_PAUSED,ESM_SCANNING)) {
		mode = ESM_STOPPED;
		return true;
	}
	return false;
}


struct MHD_Daemon* TWebServer::startServer(unsigned int flags) {
	// Derive max. connection count from computing core count
	int connections = (numa > 1) ? web.maxConnectionsPerCPU * numa : web.maxConnectionsPerCPU;
	data.threadLimit = connections;

	// Add standard options
	TWebOption options;
	options.add(MHD_OPTION_CONNECTION_LIMIT, connections, NULL);
	options.add(MHD_OPTION_CONNECTION_TIMEOUT, CONNECTION_TIMEOUT, NULL);
	options.add(MHD_OPTION_NOTIFY_COMPLETED, (intptr_t)requestCompletedCallbackDispatcher, this);
	options.add(MHD_OPTION_EXTERNAL_LOGGER, (intptr_t)customErrorLogDispatcher, this);
	options.add(MHD_OPTION_URI_LOG_CALLBACK, (intptr_t)logHandlerDispatcher, this);

	// Add authentication options
	if (web.auth != HAT_DIGEST_NONE) {
		// Calculate new seed for digest authentication:
		// e.g. digest = "dcd98b7102dd2f0e8b11d0f600bfb0c093"
		//                123456789-123456789-123456789-1234
		digest = util::fastCreateHexStr(HTTP_DIGEST_SIZE, false);
		options.add(MHD_OPTION_NONCE_NC_SIZE, HTTP_NONCE_SIZE, NULL);
		options.add(MHD_OPTION_DIGEST_AUTH_RANDOM, (intptr_t)digest.size(), (void*)digest.c_str());
	}

	// Add HTTPS certificates
	if (flags & MHD_USE_SSL) {
		options.add(MHD_OPTION_HTTPS_MEM_KEY, (intptr_t)strlen(httpsKey), (void*)httpsKey);
		options.add(MHD_OPTION_HTTPS_MEM_CERT, (intptr_t)strlen(httpsCert), (void*)httpsCert);
		options.add(MHD_OPTION_HTTPS_MEM_DHPARAMS, (intptr_t)strlen(httpsParam), (void*)httpsParam);
		options.add(MHD_OPTION_HTTPS_PRIORITIES, (intptr_t)web.tlsCipherPriority.size(), (void*)web.tlsCipherPriority.c_str());
	}

	// Terminate options array
	options.terminate();
	if (web.verbosity > 1) {
		std::cout << std::endl << "MHD start options:" << std::endl;
		options.debugOutput();
	}

	// Use threaded connection handling
	flags |= MHD_USE_INTERNAL_POLLING_THREAD;

	// Use one thread per connection?
	if (web.threaded) {
		flags |= MHD_USE_THREAD_PER_CONNECTION;
	}

	// Enable websockets
	if (web.allowWebSockets) {
		flags |= MHD_ALLOW_UPGRADE;
	}

	// Enable error callback
	if (web.verbosity > 0) {
		flags |= MHD_USE_ERROR_LOG;
	}

	// Set external logger
	if (web.debug) {
		flags |= MHD_USE_DEBUG;
	}

#ifdef HAS_EPOLL
	if (!web.threaded) {
		flags |= MHD_USE_EPOLL;
	}
#endif

	// Get port number from protocol
	port = web.port > 0 ? web.port : (web.useHttps ? 443 : 80);

	// Start webserver with dynamic option array
	struct MHD_Daemon* daemon = MHD_start_daemon(flags, port, // @suppress("Invalid arguments")
												 acceptHandlerDispatcher, this,
												 requestHandlerDispatcher, this,
												 MHD_OPTION_ARRAY, options(),
												 MHD_OPTION_END);

	// Set global panic callback handler
	if (util::assigned(daemon)) {
		MHD_set_panic_func(panicCallbackDispatcher, this);
	}

	return daemon;
}


bool TWebServer::start(const bool autostart) {
	bool retVal = false;

	if (web.enabled) {
		std::lock_guard<std::mutex> lock(modeMtx);
		if (ESM_CREATED == mode) {
			memory = util::sizeToStr(sysutil::getCurrentMemoryUsage());
			unsigned int flags = 0;
			bool ip6start = false;

			// Cache files in web root folder
			if (!util::folderExists(web.documentRoot))
				throw util::app_error("TWebServer::start() : Web root missing <" + web.documentRoot + ">");
			scanWebRoot(web.debug);

			// Read settings from config file if objects initialized
			if (web.init)
				readConfig();

			// Check authentication algorithm
			writeInfoLog("[Start web server] Authentication algorithm <" + getWebAuthType(web.auth) + "> in use.");

			// Check if upload folder exists or create it
			if (!util::folderExists(web.uploadFolder))
				if (!util::createDirektory(web.uploadFolder))
					throw util::app_error("TWebServer::start() : Creating upload folder failed <" + web.uploadFolder + ">");

			// Add standard web variables
			addWebToken();

			// Create session store
			if (!createWebSessionStore(web.sessionStore))
				throw util::app_error("TWebServer::start() : Can't create session store <" + web.sessionStore + ">");

			// Load stored sessions
			int r = loadWebSessionsFromFile(web.sessionStore);
			if (r > 0) {
				writeInfoLog(util::csnprintf("[Start web server] Loaded % sessions from store $", r, web.sessionStore));
			} else {
				if (r < 0) {
					writeInfoLog("[Start web server] Loading stored sessions failed.");
				}
			}

			// Set TLS properties
			if (web.useHttps) {

				// Load files into buffer
				if (web.certFile != "builtin" && web.keyFile != "builtin") {
					if (!util::fileExists(web.certFile))
						throw util::app_error("TWebServer::start : Certificate file missing <" + web.certFile + ">");
					if (!util::fileExists(web.keyFile))
						throw util::app_error("TWebServer::start : Key file missing <" + web.keyFile + ">");

					certFile = new util::TFile(web.certFile);
					keyFile = new util::TFile(web.keyFile);

					if (keyFile->load()) {
						httpsKey = keyFile->getData();
						writeInfoLog("[Start web server] HTTPS key file <" + keyFile->getFile() + "> loaded.");
					}
					if (certFile->load()) {
						httpsCert = certFile->getData();
						writeInfoLog("[Start web server] HTTPS certificate file <" + certFile->getFile() + "> loaded.");
					}
				} else
					writeInfoLog("[Start web server] Using internal HTTPS certificate.");

				// Check for external Diffie-Hellman parameters
				// ALWAYS recommended to use unique and 2048 bit sized prime to prevent easy man-in-the-middle attacks!
				if (web.dhFile != "builtin") {
					if (!util::fileExists(web.dhFile))
						throw util::app_error("TWebServer::start : Diffie-Hellman parameter file missing <" + web.dhFile + ">");

					// Load file into buffer
					dhFile = new util::TFile(web.dhFile);
					if (dhFile->load()) {
						httpsParam = dhFile->getData();
						writeInfoLog("[Start web server] HTTPS Diffie-Hellman parameter file <" + dhFile->getFile() + "> loaded.");
					}
				} else
					writeInfoLog("[Start web server] Use internal Diffie-Hellman prime.");

				// Set HTTPS flag
				flags |= MHD_USE_SSL;
			}

			// Check for IPv6 supported by target system
			if (inet::hasIPv6()) {
				httpServer6 = startServer(flags | MHD_USE_IPv6);
				ip6start = true;
			}

			httpServer4 = startServer(flags);

			bool running = util::assigned(httpServer4);
			if (ip6start && running)
				running = util::assigned(httpServer6);

			if (running) {
				std::string http = web.useHttps ? "https" : "http";
				writeInfoLog("[Start web server] Started service on " + http + "://" + sysutil::getHostName() + ":" + std::to_string((size_u)web.port));
				if (ip6start)
					writeInfoLog("[Start web server] Server is IPv6 enabled.");
			} else {
				writeInfoLog("[Start web server] Failed to start web server.");
				throw util::app_error("TWebServer::start() : Failed to start web server <" + name + ">");
			}

			// Start websocket handling
			if (running && web.allowWebSockets) {
				sockets = new TWebSockets(name, threads, infoLog, config);
				sockets->setSecure(isSecure());
				sockets->start();
				sockets->bindSocketDataEvent(&app::TWebServer::onSocketData, this);
				sockets->bindSocketVariantEvent(&app::TWebServer::onSocketVariant, this);
				sockets->bindSocketConnectEvent(&app::TWebServer::onSockeConnect, this);
				socketTimer = timers->addTimer(name, "WebSocketPingTimer", web.refreshTimer, &app::TWebServer::onHeartbeatTimer, this);
			}

			sessionTimer->setEnabled(running);
			requestTimer->setEnabled(running);
			bufferTimer->setEnabled(running);
			statsTimer->setEnabled(running);

			// Check if server response mode is paused
			if (running) {
				mode = autostart ? ESM_RUNNING : ESM_PAUSED;
				if (mode == ESM_PAUSED) {
					writeInfoLog("[Start web server] Server started in paused mode.");
				}
				if (mode == ESM_RUNNING) {
					writeInfoLog("[Start web server] Server started nomally.");
				} else {
					writeErrorLog(util::csnprintf("[Start web server] Invalid server state (%)", mode));
				}
			} else {
				mode = ESM_STOPPED;
				writeErrorLog("[Start web server] Starting server failed.");
			}
			retVal = running;
		}

	} else {
		writeInfoLog("[Start web server] Start disabled via configuration file <" + config->getFileName() + ">");
		mode = ESM_DISABLED;
		retVal = true;
	}

	return retVal;
}


bool TWebServer::terminate() {
	if (setTerminateMode()) {

		// Stop timer events
		sessionTimer->stop();
		requestTimer->stop();
		bufferTimer->stop();
		statsTimer->stop();
		if (util::assigned(socketTimer)) {
			socketTimer->stop();
		}

		// Stop web socket handler
		if (util::assigned(sockets)) {
			sockets->terminate();
			sockets->waitFor();
		}

		// Stop webserver instances
		if (util::assigned(httpServer4))
			MHD_stop_daemon(httpServer4);

		if (util::assigned(httpServer6))
			MHD_stop_daemon(httpServer6);

		httpServer4 = nil;
		httpServer6 = nil;

		writeInfoLog("[Stop web server] Server handled " + std::to_string((size_u)data.requestCount) + " requests, served " + util::sizeToStr(data.bytesServed));
		writeInfoLog("[Stop web server] Server changed state to stopped.");

		// Save web sessions
		int r = saveWebSessionsToFile(web.sessionStore);
		if (r > 0) {
			writeInfoLog(util::csnprintf("[Stop web server] Writing % sessions to $", r, web.sessionStore));
		} else {
			if (r < 0) {
				writeInfoLog("[Stop web server] Writing session store failed.");
			}
		}

		return true;
	}
	return false;
}

bool TWebServer::pause() {
	std::lock_guard<std::mutex> lock(modeMtx);
	if (mode == ESM_RUNNING) {
		mode = ESM_PAUSED;
		writeInfoLog("[Pause web server] Server changed state to paused.");
	}
	return (mode == ESM_PAUSED);
}

bool TWebServer::resume() {
	std::lock_guard<std::mutex> lock(modeMtx);
	if (mode == ESM_PAUSED) {
		mode = ESM_RUNNING;
		writeInfoLog("[Resume web server] Server state resumed.");
	}
	return (mode == ESM_RUNNING);
}


void TWebServer::getWebServerInfo() {
	const union MHD_DaemonInfo* info;

	if (util::assigned(httpServer4)) {
		info = MHD_get_daemon_info(httpServer4, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
		if (util::assigned(info)) {
			data.connectionsV4 = info->num_connections;
		}
	}

	if (util::assigned(httpServer6)) {
		info = MHD_get_daemon_info(httpServer6, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
		if (util::assigned(info)) {
			data.connectionsV6 = info->num_connections;
		}
	}
}


void TWebServer::onSessionTimer() {
	sessionGarbageCollector();
}

void TWebServer::onRequestTimer() {
	requestGarbageCollector();
	linkGarbageCollector();
}

void TWebServer::onBufferTimer() {
	bufferGarbageCollector();
}

void TWebServer::onStatsTimer() {
	getWebServerInfo();
	updateStatusDisplay();
}

void TWebServer::onHeartbeatTimer() {
	util::TDateTime ts;
	util::TVariantValues msg;
	msg.add("websocket", "ping");
	msg.add("date", ts.asISO8601());
	msg.add("host", application.getHostName());
	broadcast(msg);
}


void TWebServer::addWebToken() {
	// Add default web variables
	addWebToken("APP_VER",  application.getVersion());
	addWebToken("APP_DESC", application.getDescription());
	addWebToken("APP_NAME", application.getJumbotron());
	addWebToken("APP_HINT", application.getBanner());
	addWebToken("APP_LOGO", application.getLogo());

	// Add system tokens
	addWebToken("SYS_LANGUAGE_NAME", syslocale.asISO639());
	addWebToken("SYS_HOST_NAME", application.getHostName());
	addWebToken("SYS_SCREEN_ORIENTATION", web.usePortraitMode ? "portrait" : "any");

	// Get isolated CPU and nice level
	const std::string& iso = application.getIsolatedCPUs();
	std::string cpu = (application.getAssignedCPU() > 0) ? std::to_string((size_s)application.getAssignedCPU()) : "-";
	cpu += (iso.empty() ? "/-" : ("/" + iso));
	cpu += ("/" + std::to_string(application.getNiceLevel()));

	// Add statistics, timer, refresh, etc.
	data.wtUpTime = addWebToken("APP_UP_TIME", util::timeToHuman(util::now() - data.startTime, 2));
	data.wtWebRefreshTimer = addWebToken("WEB_REFRESH_TIMER", std::to_string((size_u)web.refreshTimer));
	data.wtDisplayRefreshTimer = addWebToken("REFRESH_TIMER", std::to_string((size_u)web.refreshTimer));
	data.wtWebRefreshInterval = addWebToken("WEB_REFRESH_INTERVAL", std::to_string((size_u)web.refreshInterval));
	data.wtDisplayRefreshInterval = addWebToken("REFRESH_INTERVAL", std::to_string((size_u)web.refreshInterval));
	data.wtCurrentMemory = addWebToken("APP_CURRENT_MEMORY", "0");
	data.wtRequestCount = addWebToken("REQUEST_COUNT", "0/0");
	data.wtVirtualCount = addWebToken("VIRTUAL_COUNT", "0/0");
	data.wtSessionCount = addWebToken("SESSION_COUNT", "0/0");
	data.wtBytesServed  = addWebToken("SEND_BYTES", "0");
	data.wtRequestQueue = addWebToken("REQUESTS_IN_QUEUE", "0/0");
	data.wtActionQueue  = addWebToken("ACTIONS_IN_QUEUE", "0/0");
	data.wtSocketCount  = addWebToken("WEB_SOCKET_COUNT", "-/-");
	data.wtConnections  = addWebToken("WEB_CONNECTIONS", "0/0/-");
	data.wtStartMemory = addWebToken("APP_START_MEMORY", "0");
	data.wtPeakMemory  = addWebToken("APP_PEAK_MEMORY", "0");
	data.wtCpuAffinity = addWebToken("APP_CPU_AFFINITY", cpu);
	data.wtCpuCores    = addWebToken("APP_CPU_CORES", std::to_string((size_u)numa) + "/" + std::to_string((size_u)sysutil::getThreadCount()));
	data.wtUserCount   = addWebToken("APP_USER_COUNT", std::to_string((size_u)application.getCredentials().size()));

	// Init done...
	data.init = util::assigned(data.wtUserCount);
}

void TWebServer::updateWebToken() {
	if (data.init) {

		// Set static web token values
		*data.wtWebRefreshTimer = web.refreshTimer;
		*data.wtDisplayRefreshTimer = web.refreshTimer;
		*data.wtWebRefreshInterval = web.refreshInterval;
		*data.wtDisplayRefreshInterval = web.refreshInterval;

		// Set dynamic web token values
		updateStatusToken();
	}
}

void TWebServer::updateStatusDisplay() {
	if (isResponding()) {
		updateStatusToken();
		raiseStatusEvent();
	}
}

void TWebServer::updateStatusToken() {
	data.virtualCount = getWebRequestCount();
	int delta;

	// Set new values to web tokens, but do not invalidate page buffers for every token
	*data.wtUpTime = util::timeToHuman(util::now() - data.startTime, 2);
	*data.wtBytesServed = util::sizeToStr(data.bytesServed);

	if (data.requestQueue > data.maxRequestQueue) data.maxRequestQueue = data.requestQueue;
	*data.wtRequestQueue = std::to_string((size_u)data.requestQueue) + "/" + std::to_string((size_u)data.maxRequestQueue);

	if (data.actionQueue > data.maxActionQueue) data.maxActionQueue = data.actionQueue;
	*data.wtActionQueue  = std::to_string((size_u)data.actionQueue) + "/" + std::to_string((size_u)data.maxActionQueue);

	if (data.sessionCount > data.maxSessionCount) data.maxSessionCount = data.sessionCount;
	*data.wtSessionCount = std::to_string((size_u)data.sessionCount) + "/" + std::to_string((size_u)data.maxSessionCount);

	// Get websocket statistics
	if (util::assigned(sockets)) {
		data.socketCount = sockets->getEpollCounT();
		if (data.socketCount > data.maxSocketCount) data.maxSocketCount = data.socketCount;
		*data.wtSocketCount = std::to_string((size_u)data.socketCount) + "/" + std::to_string((size_u)data.maxSocketCount);
	}

	delta = (data.requestCount - data.lastRequestCount) / (statsTimer->getDelay() / 1000);
	if (delta > data.maxRequestCount) data.maxRequestCount = delta;
	if (delta > 0) {
		data.deltaRequestCount = delta;
		*data.wtRequestCount = (std::to_string((size_u)data.requestCount) + " (" + std::to_string((size_u)delta) + "/" + std::to_string((size_u)data.maxRequestCount) + " per second)");
	} else {
		data.deltaRequestCount = 0;
		*data.wtRequestCount = data.requestCount;
	}
	data.lastRequestCount = data.requestCount;

	delta = (data.virtualCount - data.lastVirtualCount) / (statsTimer->getDelay() / 1000);
	if (delta > data.maxVirtualCount) data.maxVirtualCount = delta;
	if (delta > 0) {
		data.deltaVirtualCount = delta;
		*data.wtVirtualCount = (std::to_string((size_u)data.virtualCount) + " (" + std::to_string((size_u)delta) + "/" + std::to_string((size_u)data.maxVirtualCount) + " per second)");
	} else {
		data.deltaVirtualCount = 0;
		*data.wtVirtualCount = data.virtualCount;
	}
	data.lastVirtualCount = data.virtualCount;

	int c = data.connectionsV4 + data.connectionsV6;
	std::string s = std::to_string((size_u)c) + "/" + std::to_string((size_u)data.connectionsV4) + "/";
	if (util::assigned(httpServer6))
		s += std::to_string((size_u)data.connectionsV6);
	else
		s += "-";
	*data.wtConnections = s;

	*data.wtCurrentMemory = util::sizeToStr(sysutil::getCurrentMemoryUsage());
	*data.wtPeakMemory    = util::sizeToStr(sysutil::getPeakMemoryUsage());
	*data.wtStartMemory   = memory;

	data.threadCount = sysutil::getThreadCount();
	*data.wtCpuCores  = std::to_string((size_u)numa) + "/" + std::to_string((size_u)data.threadCount);
	*data.wtUserCount = std::to_string((size_u)application.getCredentials().size());

	// Invalidate the object that is most likely to be changed:
	// All related web page buffers will be rebuild with new token values on next access!
	data.wtSessionCount->invalidate();
	data.wtRequestCount->invalidate();
	data.wtUpTime->invalidate();
}

void TWebServer::raiseStatusEvent() {
	// Forward statistics event
	for (const auto& handler : statisticsEventList) {
		try {
			handler(*this, data);
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::string sName = util::nameOf(handler);
			std::string sText = "Exception in TWebServer::raiseStatusEvent() event handler [" + sName + "] : " + sExcept;
			writeErrorLog(sText);
		} catch (...)	{
			std::string sName = util::nameOf(handler);
			std::string sText = "Unknown exception in TWebServer::raiseStatusEvent() event handler [" + sName + "]";
			writeErrorLog(sText);
		}
	}
}


void TWebServer::setMemoryStatus(const size_t current) {
	memory = util::sizeToStr(current);
}


void TWebServer::readTokenList(TWebTokenValueMap& values) {
	if (!tokenList.empty()) {
		tokenList.getTokenValues(values);
	}
	tokenList.invalidate();
}

void TWebServer::updateTokenList(const TWebTokenValueMap& values) {
	if (!tokenList.empty()) {
		writeInfoLog("[Update web server] Update token list with " + std::to_string((size_u)tokenList.size()) + " token.");
		tokenList.update();
		tokenList.setTokenValues(values);
	}
}


void TWebServer::getWebSessionValues(const std::string& sid, util::TVariantValues& values) {
	std::lock_guard<std::mutex> lock(sessionMtx);
	std::string uid = util::tolower(sid);
	TWebSessionMap::const_iterator session = sessionMap.find(uid);
	if (session != sessionMap.end()) {
		PWebSession o = session->second;
		if (util::assigned(o)) {
			o->getSessionValues(values);
		}
	}
}

bool TWebServer::createWebSessionStore(const std::string path) {
	std::lock_guard<std::mutex> lock(sessionMtx);
	std::string root = util::validPath(path);
	if (!util::folderExists(root)) {
		return util::createDirektory(root);
	}
	return true;
}

int TWebServer::saveWebSessionsToFile(const std::string path) {
	std::lock_guard<std::mutex> lock(sessionMtx);
	std::string root = util::validPath(path);
	int retVal = 0;
	int r = util::deleteFolders(root);
	if (r >= 0) {
		if (sessionMap.size() > 0) {
			PWebSession session;
			TWebSessionMap::const_iterator it = sessionMap.begin();
			while (it != sessionMap.end()) {
				session = it->second;
				if (util::assigned(session)) {
					const std::string& sid = session->sid;
					if (!sid.empty()) {

						// Do not save anonimous and default web sessions
						if (WUA_UNKNOWN != session->userAgent && session->useC > 1 && session->matrix.size() > 3) {

							// Create session folder
							std::string dir = util::validPath(root + sid);
							util::createDirektory(dir);

							// Save session values to JSON file
							util::TVariantValues values;
							values.add("SID", sid);
							values.add("AGENT", session->userAgent);
							values.add("USERNAME", session->username);
							values.add("USERLEVEL", session->userlevel);
							values.add("AUTHENTICATED", session->authenticated);
							values.add("TIMESTAMP", (int64_t)session->timestamp);
							values.add("TIMEOUT", (int64_t)session->timeout);
							values.add("COUNT", session->refC);
							values.add("REQUESTED", session->useC);
							values.asJSON().saveToFile(dir + "session.json");

							// Save matrix values as JSON file
							session->getSessionValues(values);
							if (!values.empty()) {
								values.asJSON().saveToFile(dir + "matrix.json");
							}

							// Save matrix values as JSON file
							session->getCookieValues(values);
							if (!values.empty()) {
								values.asJSON().saveToFile(dir + "cookies.json");
							}

							++retVal;
						}
					}
				}
				++it;
			}
		}
	} else {
		retVal = EXIT_ERROR;
	}
	return retVal;
}

int TWebServer::loadWebSessionsFromFile(const std::string path) {
	std::lock_guard<std::mutex> lock(sessionMtx);
	int retVal = 0;
	app::TStringVector store;
	util::readDirectoryTree(web.sessionStore, store, util::SD_ROOT, false);
	if (!store.empty()) {
		for (size_t i=0; i<store.size(); ++i) {
			std::string path = store.at(i);
			util::validPath(path);
			if (!path.empty()) {

				// Read session values from file
				data::TTable table;
				table.loadAsJSON(path + "session.json");
				if (!table.empty()) {

					// Get session SID
					std::string sid = table[0]["SID"].asString();
					if (util::isValidUUID(sid)) {
						const data::TRecord& row = table[0];

						// Get default values
						EWebUserAgent value = (EWebUserAgent)table[0]["AGENT"].asInteger();
						EWebUserAgent agent = WUA_UNKNOWN;
						if (util::isMemberOf(value, WUA_MSIE,WUA_MSEDGE,WUA_MSCHROME,WUA_FIREFOX,WUA_CHROMIUM,WUA_SAFARI,WUA_NETSCAPE,WUA_OPERA,WUA_CURL,WUA_APPLICATION))
							agent = value;

						// Set default value for session
						PWebSession session = new TWebSession(sid);
						session->sid = sid;
						session->userAgent = agent;
						session->username = row["USERNAME"].asString();
						session->userlevel = row["USERLEVEL"].asInteger();
						session->authenticated = row["AUTHENTICATED"].asBoolean();
						session->timestamp = (util::TTimePart)row["TIMESTAMP"].asInteger64(0);
						session->timeout = (util::TTimePart)row["TIMEOUT"].asInteger64(0);
						session->useC = row["REQUESTED"].asInteger(1);
						session->refC = 0;

						// Read matrix values from file
						table.loadAsJSON(path + "matrix.json");
						if (!table.empty()) {
							const data::TRecord& record = table[0];
							if (!record.empty()) {
								for (size_t j=0; j<record.size(); ++j) {
									const std::string& key = record.field(j).getName();
									const std::string& value = record.value(j).asString();
									session->setSessionValue(key, value);
								}
								++retVal;
							}
						}

						// Read cookies from file
						table.loadAsJSON(path + "cookies.json");
						if (!table.empty()) {
							const data::TRecord& record = table[0];
							if (!record.empty()) {
								for (size_t j=0; j<record.size(); ++j) {
									const std::string& key = record.field(j).getName();
									const std::string& value = record.value(j).asString();
									session->setCookieValue(key, value);
								}
								++retVal;
							}
						}

						// Add session to mapped list
						if (retVal > 0) {
							sessionMap.insert(TWebSessionItem(sid, session));
						} else {
							delete session;
						}
					}

				}

			}
		}
	}
	return retVal;
}


void TWebServer::sessionGarbageCollector(const bool cleanup) {
	if (isResponding()) {
		std::lock_guard<std::mutex> lock(sessionMtx);
		if (sessionMap.size() > 0) {
			int verbosity = 4;
			PWebSession o;
			util::TTimePart now = util::now();
			util::TTimePart sessionDeleteAge = cleanup ? web.sessionDeleteAge / 10000 : web.sessionDeleteAge / 1000;
			util::TTimePart unusedDeleteAge = cleanup ? web.rejectedDeleteAge / 50000 : web.rejectedDeleteAge / 5000;
			util::TTimePart rejectedDeleteAge = cleanup ? web.rejectedDeleteAge / 10000 : web.rejectedDeleteAge / 1000;
			util::TTimePart zombieDeleteAge = cleanup ? web.sessionDeleteAge / 250 : web.sessionDeleteAge / 25;
			if (sessionDeleteAge < 3) sessionDeleteAge = 3;
			if (unusedDeleteAge < 3) unusedDeleteAge = 3;
			if (rejectedDeleteAge < 3) rejectedDeleteAge = 3;
			if (zombieDeleteAge < 30) zombieDeleteAge = 30;
			TWebSessionMap::iterator it = sessionMap.begin();
			size_t size = sessionMap.size();
			while (it != sessionMap.end()) {
				o = it->second;
				util::TTimePart age = now - o->timestamp;
				if (web.verbosity >= verbosity)
					writeInfoLog(util::csnprintf("[Session garbage collector] Session <" + o->sid + "> Age = % seconds, Refcnt = %, Authenticated = %", age, o->refC, o->authenticated));

				// Ignore sessions with active post data transfer
				if (o->busy()) {
					if (web.verbosity >= verbosity)
						writeInfoLog("[Session garbage collector] Session <" + o->sid + "> Age = " + std::to_string((size_u)age) + " seconds, post process active.");
				} else {	
					// Check for unused sessions out of date
					if ((age > sessionDeleteAge) && (o->refC <= 0)) {
						if (web.verbosity >= verbosity)
							writeInfoLog("[Session garbage collector] Session <" + o->sid + "> deleted.");
						util::freeAndNil(o);
						it = sessionMap.erase(it);
						continue;
					} else {
						// Delete zombies... (40 times older!)
						if (age > zombieDeleteAge) {
							if (web.verbosity >= verbosity)
								writeInfoLog("[Session garbage collector] Zombie session <" + o->sid + "> deleted.");
							util::freeAndNil(o);
							it = sessionMap.erase(it);
							continue;
						} else {
							// Delete rejected sessions
							bool rejected = !o->authenticated /*&& (o->refC <= 0)*/;
							bool unused = WUA_UNKNOWN == o->userAgent || o->matrix.size() < 4;
							if (unused && (age > unusedDeleteAge)) {
								if (web.verbosity >= verbosity)
									writeInfoLog("[Session garbage collector] Unused session <" + o->sid + "> deleted.");
								util::freeAndNil(o);
								it = sessionMap.erase(it);
								continue;
							} else {
								if (rejected && (age > rejectedDeleteAge)) {
									if (web.verbosity >= verbosity)
										writeInfoLog("[Session garbage collector] Rejected session <" + o->sid + "> deleted.");
									util::freeAndNil(o);
									it = sessionMap.erase(it);
									continue;
								}
							}
						}
					}
				}
				it++;
			}
			size_t deleted = size - sessionMap.size();
			if (deleted > 0) {
				writeInfoLog(util::csnprintf("[Session garbage collector] % sessions of % sessions deleted, % sessions remaining.", deleted, size, sessionMap.size()));
				if (deleted > 100)
					deallocateHeapMemory(deleted);
			} else {
				if (web.verbosity >= verbosity)
					writeInfoLog(util::csnprintf("[Session garbage collector] % session(s) in queue.", sessionMap.size()));
			}
		}
	}
}


bool sessionSorterAsc(const TWebSession* o, const TWebSession* p) {
	if (o->useC == p->useC)
		return o->timestamp < p->timestamp; // Older is less!
	return o->useC < p->useC; // Low usage count first
}

void TWebServer::sessionCountLimiter() {
	if (isResponding()) {
		std::lock_guard<std::mutex> lock(sessionMtx);
		size_t max = web.maxSessionCount;
		if (max > 0) {
			if (sessionMap.size() > max) {
				int verbosity = 4;
				PWebSession o;
				TWebSessionList list;
				size_t limit = max * 3 / 4;

				// Store websessions in list
				TWebSessionMap::iterator it = sessionMap.begin();
				size_t size = sessionMap.size();
				while (it != sessionMap.end()) {
					o = it->second;
					if (util::assigned(o)) {
						list.push_back(o);
					}
					it++;
				}

				// Calculate count of oldest entries to delete
				size_t count = list.size() > limit ? list.size() - limit : 0;
				size_t marked = 0;

				// Delete oldes entries if:
				//  - not referenced
				//  - no active post process
				//  - used by one request only (retain recurring browswer based requests)
				if (count > 0) {

					// Order list by timestamp, oldest on top...
					std::sort(list.begin(), list.end(), sessionSorterAsc);

					// Search for reusable entries...
					for (size_t i=0; i<list.size(); i++) {
						if (count > 0) {
							o = list[i];
							if (util::assigned(o)) {
								if (!o->busy() && o->refC <= 0 && o->useC < 2) {
									o->deleted = true;
									marked++;
								}
							}
							count--;
						} else {
							break;
						}
					}
				}

				// Delete marked entries in session map
				if (marked > 0) {
					it = sessionMap.begin();
					while (it != sessionMap.end()) {
						o = it->second;

						// Check for deleted sessions
						if (o->deleted) {
							if (web.verbosity >= verbosity)
								writeInfoLog("[Session Storm Limiter] Session <" + o->sid + "> deleted.");
							util::freeAndNil(o);
							it = sessionMap.erase(it);
							continue;
						}

						it++;
					}
				}

				size_t deleted = size - sessionMap.size();
				if (deleted > 0) {
					writeInfoLog(util::csnprintf("[Session Storm Limiter] % sessions of % sessions deleted, % sessions remaining.", deleted, size, sessionMap.size()));
					if (deleted > (limit / 2))
						deallocateHeapMemory(deleted);
				} else {
					if (web.verbosity >= verbosity)
						writeInfoLog(util::csnprintf("[Session Storm Limiter] % session(s) in queue.", sessionMap.size()));
				}
			}
		}
	}
}


struct CRequestEraser
{
	CRequestEraser(int age, util::TTimePart now, bool zombies) : _age(age), _now(now), _zombies(zombies) {}
	int _age;
	util::TTimePart _now;
    bool _zombies;
    bool operator()(PWebRequest o) const {
    	bool retVal = false;
    	if (util::assigned(o)) {
			if (_zombies) {
				bool permit = true;
				PWebSession session = o->getSession();
				if (util::assigned(session))
					permit = session->idle();
				if (permit)
					retVal = ((_now - o->getTimeStamp()) > _age);
			} else {
				retVal = (((_now - o->getTimeStamp()) > (_age / 1000)) && (o->getRefCount() <= 0));
			}	
			if (retVal)
				util::freeAndNil(o);
    	}
    	return retVal;
    }
};

void TWebServer::requestGarbageCollector(const bool cleanup) {
	if (isResponding()) {
		std::lock_guard<std::mutex> lock(requestMtx);
		if (requestList.size() > 0) {
			size_t deleted = 0;
			size_t count = requestList.size();
			writeInfoLog("[Request garbage collector] " + std::to_string((size_s)count) + " requests in queue.", 3);
			util::TTimePart now = util::now();

			// Delete closed request by age
			count = requestList.size();
			util::TTimePart requestDeleteAge = cleanup ? web.requestDeleteAge / 10 : web.requestDeleteAge;
			if (requestDeleteAge < 3) requestDeleteAge = 3;
			requestList.erase(std::remove_if(requestList.begin(), requestList.end(), CRequestEraser(requestDeleteAge, now, false)), requestList.end());
			count -= requestList.size();
			if (count > 0) {
				deleted += count;
				writeInfoLog("[Request garbage collector] " + std::to_string((size_s)count) + " requests deleted.", 3);
			}

			// Delete zombie request (> 3 hours = 10800 seconds)
			count = requestList.size();
			requestList.erase(std::remove_if(requestList.begin(), requestList.end(), CRequestEraser(10800, now, true)), requestList.end());
			count -= requestList.size();
			if (count > 0) {
				deleted += count;
				writeInfoLog("[Request garbage collector] " + std::to_string((size_s)count) + " zombies deleted.");
			}

			if (deleted > 0 && application.getHeapDelay() > 0) {
				deallocateHeapMemory(deleted);
			}

		}
		rest.garbageCollector();
	}
}

void TWebServer::linkGarbageCollector() {
	if (ESM_RUNNING == getMode()) {
		size_t size = rest.garbageCollector();
		if (size)
			writeInfoLog("[VFS link garbage collector] " + std::to_string((size_s)size) + " link buffer deleted.");
	}
}


void TWebServer::bufferGarbageCollector() {
	if (isResponding()) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(requestLck);
		if (!lock.rdTryLock()) {
			writeInfoLog("[Parser garbage collector] List locked, skipping now.", 3);
			return;
		}
		if (content.size()) {
			size_t count = 0;
			util::PFile o;
			util::TFileMap::const_iterator it = content.getFiles()->begin();
			while (it != content.getFiles()->end()) {
				o = it->second;
				if (util::assigned(o))
					count += o->deleteParserData();
				it++;
			}
			if (count) {
				writeInfoLog("[Parser garbage collector] " + std::to_string((size_s)count) + " buffers deleted.", 3);
				if (application.getHeapDelay() > 0)
					deallocateHeapMemory(count);
			}
		}
	}
}


void TWebServer::deallocateHeapMemory(const size_t deleted) {
	if (deleted > 64) {
		application.deallocateHeapMemory();
	}
}


bool TWebServer::logoffSessionUser(const std::string& sid) {
	std::lock_guard<std::mutex> lock(sessionMtx);
	if (!sessionMap.empty()) {
		TWebSessionMap::iterator it = sessionMap.find(sid);
		if (it != sessionMap.end()) {
			PWebSession o = it->second;
			if (util::assigned(o)) {
				{
					// Force user to log off...
					app::TLockGuard<app::TMutex> lock(o->userMtx);
					o->logoff = true;
					o->authenticated = false;
					o->username.clear();
					o->password.clear();
					o->userlevel = 0;
					o->valuesRead = false;
				}
				o->clearUserValues();
				return true;
			}
		}
	}
	return false;
}

void TWebServer::addUserCredential(TCredential& user) {
	if (app::TCredentials::hash(user, web.auth, web.realm)) {
		app::TLockGuard<app::TMutex> mtx(userMtx);
		web.users[user.username] = user;
	}
}

void TWebServer::assignUserCredentials(const TCredentialMap& users) {
	app::TLockGuard<app::TMutex> mtx(userMtx);
	web.users.clear();
	if (!users.empty()) {
		TCredentialMap::const_iterator it = users.begin();
		while (it != users.end()) {
			TCredential user = it->second;
			if (user.valid()) {
				web.users[user.username] = user;
			}
			++it;
		}
	}
}

void TWebServer::showUserCredentials(const std::string& preamble) {
	app::TLockGuard<app::TMutex> mtx(userMtx);
	TCredentialMap::const_iterator it = web.users.begin();
	while (it != web.users.end()) {
		std::cout << preamble << "Username      : " << it->second.username << std::endl;
		std::cout << preamble << "Lastname      : " << it->second.lastname << std::endl;
		std::cout << preamble << "Givenname     : " << it->second.givenname << std::endl;
		std::cout << preamble << "Password      : " << it->second.password << std::endl;
		std::cout << preamble << "Description   : " << it->second.description << std::endl;
		std::cout << preamble << "Gecos         : " << it->second.gecos << std::endl;
		std::cout << preamble << "Realm         : " << it->second.realm << std::endl;
		std::cout << preamble << "Hash MD5      : " << it->second.hashMD5 << std::endl;
		std::cout << preamble << "Hash SHA256   : " << it->second.hashSHA256 << std::endl;
		std::cout << preamble << "Hash SHA512   : " << it->second.hashSHA512 << std::endl;
		std::cout << preamble << "Digest MD5    : " << it->second.digestMD5 << std::endl;
		std::cout << preamble << "Digest SHA256 : " << it->second.digestSHA256 << std::endl;
		std::cout << preamble << "Digest SHA512 : " << it->second.digestSHA512 << std::endl;
		std::cout << preamble << "Level         : " << it->second.level << std::endl << std::endl;
		++it;
	}
}

void TWebServer::readConfig() {
	// Read values from ini file
	config->setSection(name);
	web.enabled = config->readBool("Enabled", web.enabled);
	web.useHttps = config->readBool("UseHttps", web.useHttps);
	web.port = config->readInteger("Port", web.port);
	web.allowedList = config->readString("AllowedFromIP", ALLOWED_LIST);
	web.allowManifestFiles = config->readBool("AllowManifestFiles", web.allowManifestFiles);
	web.documentRoot = config->readPath("DocumentRoot", web.documentRoot);
	web.uploadFolder = config->readPath("UploadFolder", util::validPath(web.documentRoot + "upload"));
	web.sessionStore = config->readPath("SessionStore", util::validPath(application.getDataRootFolder() + "sessions"));
	web.restRoot = config->readPath("RestfulAPIRoot", web.restRoot);
	web.disableVfsGZip = config->readBool("DisableVfsGZip", web.disableVfsGZip);
	web.vfsDataDeleteDelay = config->readInteger("VfsDataDeleteDelay", web.vfsDataDeleteDelay);
	web.indexPages = config->readString("IndexPage", INDEX_PAGE);
	web.certFile = config->readString("CertFile", web.certFile); // Default value is "builtin"
	web.keyFile = config->readString("KeyFile", web.keyFile); // Default value is "builtin"
	web.dhFile = config->readString("DiffieHellmanFile", web.dhFile); // Default value is "builtin"
	web.tlsCipherPriority = config->readString("TLSCipherPriority", web.tlsCipherPriority);
	web.tokenDelimiter = config->readString("TokenDelimiter", web.tokenDelimiter);
	web.maxSessionCount = config->readInteger("MaxSessionCount", web.maxSessionCount);
	web.sessionDeleteAge = config->readInteger("SessionDeleteAge", web.sessionDeleteAge);
	web.requestDeleteAge = config->readInteger("RequestDeleteAge", web.requestDeleteAge);
	web.refreshInterval = config->readInteger("ClientRefreshInterval", web.refreshInterval);
	web.refreshTimer = config->readInteger("JavaScriptRefreshTimer", web.refreshTimer);
	web.verbosity = config->readInteger("VerbosityLevel", web.verbosity);
	web.credentials = config->readString("Credentials", web.credentials);
	web.realm = config->readString("AuthRealm", web.realm);
	web.sessionExpired = config->readInteger("SessionLogonExpired", web.sessionExpired);
	web.greeterURL = config->readString("GreeterURL", web.greeterURL);
	web.caching = config->readBool("EnableCaching", web.caching);
	web.minimize = config->readBool("MinimizeHTML", web.minimize);
	web.threaded = config->readBool("Multithreading", web.threaded);
	web.maxConnectionsPerCPU = config->readInteger("MaxConnectionsPerCPU", web.maxConnectionsPerCPU);
	web.allowWebSockets = config->readBool("AllwoWebSockets", web.allowWebSockets);
	web.usePortraitMode = config->readBool("UsePortraitMode", web.usePortraitMode);
	web.debug = config->readBool("Debug", web.debug);

	// Read deprecated boolean authentication setting
	web.auth = HAT_DIGEST_NONE;
	bool useAuth = config->readBool("Authentication", false);
	if (useAuth) {
		web.auth = HAT_DIGEST_MD5;
	}

	// Read digest algorithm from new string parameter
	std::string auth = getWebAuthType(web.auth);
	auth = config->readString("DigestAlgorithm", auth);
	web.auth = getWebAuthType(auth);

	// Read user:password
	if (web.users.empty()) {
		util::TStringList s(web.credentials, ':');
		if (s.size() > 1) {
			TCredential user;
			user.username = s[0];
			user.password = s[1];
			user.realm = web.realm;
			user.level = s.size() > 2 ? util::strToInt(s[2], DEFAULT_USER_LEVEL) : DEFAULT_USER_LEVEL;
			addUserCredential(user);
		}
	}

	// Read web token begin/end delimiter, e.g. [[X]]
	util::TStringList s(web.tokenDelimiter, 'X');
	if (s.size() > 0)
		web.tokenHeader = s[0];
	if (s.size() > 1)
		web.tokenFooter = s[1];

	// Read allowed networks/IPs
	if (std::string::npos != web.allowedList.find("0.0.0.0", util::EC_COMPARE_VALUE_IN_LIST)) {
		web.allowFromAll = true;
		writeInfoLog("[Configuration] Allow access from all source IP's.");
	} else {
		writeInfoLog("[Configuration] Allowed IP list <" + web.allowedList.asString(';') + ">");
		
	}
}


void TWebServer::writeConfig() {
	// Write all entries
	config->setSection(name);
	config->writeBool("Enabled", web.enabled, INI_BLYES);
	config->writeBool("UseHttps", web.useHttps, INI_BLYES);
	config->writeInteger("Port", web.port);
	config->writeString("AllowedFromIP", web.allowedList.csv());
	config->writeBool("AllowManifestFiles", web.allowManifestFiles);
	config->writePath("DocumentRoot", web.documentRoot);
	config->writePath("UploadFolder", web.uploadFolder);
	config->writePath("SessionStore", web.sessionStore);
	config->writePath("RestfulAPIRoot", web.restRoot);
	config->writeBool("DisableVfsGZip", web.disableVfsGZip, INI_BLYES);
	config->writeInteger("VfsDataDeleteDelay", web.vfsDataDeleteDelay);
	config->writeString("IndexPage", web.indexPages.csv());
	config->writeString("CertFile", web.certFile);
	config->writeString("KeyFile", web.keyFile);
	config->writeString("DiffieHellmanFile", web.dhFile);
	config->writeString("TLSCipherPriority", web.tlsCipherPriority);
	config->writeString("TokenDelimiter", web.tokenDelimiter);
	config->writeInteger("MaxSessionCount", web.maxSessionCount);
	config->writeInteger("SessionDeleteAge", web.sessionDeleteAge);
	config->writeInteger("RequestDeleteAge", web.requestDeleteAge);
	config->writeInteger("ClientRefreshInterval", web.refreshInterval);
	config->writeInteger("JavaScriptRefreshTimer", web.refreshTimer);
	config->writeInteger("VerbosityLevel", web.verbosity);
	config->writeString("DigestAlgorithm", getWebAuthType(web.auth));
	config->writeString("Credentials", web.credentials);
	config->writeString("AuthRealm", web.realm);
	config->writeInteger("SessionLogonExpired", web.sessionExpired);
	config->writeString("GreeterURL", web.greeterURL);
	config->writeBool("EnableCaching", web.caching, INI_BLYES);
	config->writeBool("MinimizeHTML", web.minimize, INI_BLYES);
	config->writeBool("Multithreading", web.threaded, INI_BLYES);
	config->writeInteger("MaxConnectionsPerCPU", web.maxConnectionsPerCPU);
	config->writeBool("AllwoWebSockets", web.allowWebSockets, INI_BLYES);
	config->writeBool("UsePortraitMode", web.usePortraitMode, INI_BLYES);
	config->writeBool("Debug", web.debug, INI_BLYES);

	config->deleteKey("Authentication");

	// Write changes to file!
	config->flush();
}


void TWebServer::reWriteConfig() {
	readConfig();
	readVirtualDirectoryConfig();
	writeConfig();
	web.init = true;
}


void TWebServer::readVirtualDirectoryConfig() {
	if (config->size() > 0) {
		bool rootfs = false;
		bool imagefs = false;
		std::string section;

		for (TIniFile::const_iterator it = config->begin(); it != config->end(); ++it) {

			// Skip general configuration section for this->name
			if (it->first != name) {
				// Set current section
				config->setSection(it->first);

				// Check if section references current webserver instance
				if (name == config->readString("Reference", "none")) {

					// Check for root filesystem
					section = "Filesystem";
					if (section.size() == it->first.size() && 0 == util::strncasecmp(it->first, section, section.size())) {
						rootfs = true;
					}

					// Check for root filesystem
					section = "Explorerpreview";
					if (section.size() == it->first.size() && 0 == util::strncasecmp(it->first, section, section.size())) {
						imagefs = true;
					}

					// Read configuration from file
					CWebDirectory instance;
					instance.directory = config->readPath("Directory", "");
					instance.alias     = config->readPath("Alias", "");
					instance.redirect  = config->readString("Redirect", "");
					instance.enabled   = config->readBool("Enabled", true);
					instance.execCGI   = config->readBool("ExecCGI", false);
					instance.scaleJPG  = config->readInteger("ScaleJPG", 0);
					instance.useExif   = config->readBool("UseExif", false);

					// Add new virtual directory to list
					if (instance.enabled && !instance.alias.empty() && !instance.directory.empty()) {
						// Add virtual directory to list
						PWebDirectory o = vdl.add(instance);
						if (util::assigned(o))
							writeInfoLog("[Configuration] Virtual directory added: " + util::quote(o->alias()) + " --> " + o->directory());
						else
							writeErrorLog("[Configuration] Malformed or doubled virtual directory skipped: " + util::quote(instance.alias) + " --> " + instance.directory);
					}

					// Rewrite complete section
					config->writePath("Directory", instance.directory);
					config->writePath("Alias", instance.alias);
					config->writeString("Redirect", instance.redirect.csv());
					config->writeBool("Enabled", instance.enabled, INI_BLYES);
					config->writeBool("ExecCGI", instance.execCGI, INI_BLYES);
					config->writeInteger("ScaleJPG", instance.scaleJPG);
					config->writeBool("UseExif", instance.useExif, INI_BLYES);
				}
			}
		}

		// Add virtual root filesystem
		if (!rootfs) {

			// Build standard virtual instance for root filesystem
			CWebDirectory instance;
			instance.directory = "/";
			instance.alias     = "/fs0/";
			instance.redirect  = "";
			instance.enabled   = true;
			instance.execCGI   = false;
			instance.scaleJPG  = 600;
			instance.useExif   = true;
			createVirtualDirectoryConfig(instance, "Filesystem");
		}

		// Add virtual root filesystem
		if (!imagefs) {

			// Build standard virtual instance for root filesystem
			CWebDirectory instance;
			instance.directory = "/";
			instance.alias     = "/fs1/";
			instance.redirect  = "folder;cover;front;album";
			instance.enabled   = true;
			instance.execCGI   = false;
			instance.scaleJPG  = 200;
			instance.useExif   = false;
			createVirtualDirectoryConfig(instance, "Explorerpreview");
		}

	}
}

void TWebServer::createVirtualDirectoryConfig(CWebDirectory& instance, const std::string& section) {
	// Add virtual directory to this webserver instance
	bool ok = true;
	PWebDirectory o = nil;
	if (instance.enabled && !instance.alias.empty() && !instance.directory.empty()) {
		o = vdl.add(instance);
		ok = util::assigned(o);
	}
	if (ok) {

		// Save new section
		config->setSection(section);
		config->writeString("Reference", name);
		config->writePath("Directory", instance.directory);
		config->writePath("Alias", instance.alias);
		config->writeString("Redirect", instance.redirect.csv());
		config->writeBool("Enabled", instance.enabled, INI_BLYES);
		config->writeBool("ExecCGI", instance.execCGI, INI_BLYES);
		config->writeInteger("ScaleJPG", instance.scaleJPG);
		config->writeBool("UseExif", instance.useExif, INI_BLYES);

		if (util::assigned(o))
			writeInfoLog("[Configuration] Virtual directory object \"" + section + "\" created: " + util::quote(o->alias()) + " --> " + o->directory());
		else
			writeInfoLog("[Configuration] Virtual directory entry \"" + section + "\" created: " + util::quote(instance.alias) + " --> " + instance.directory);

	} else {
		writeErrorLog("[Configuration] Duplicated virtual directory \"" + section + "\" for " + util::quote(instance.alias) + " --> " + instance.directory);
	}
}



void TWebServer::writeLog(const std::string& s) const {
	infoLog->write("[" + name + "] " + s);
}

void TWebServer::writeInfoLog(const std::string& s, int verbosity) const {
	if (util::assigned(infoLog) && web.verbosity >= verbosity)
		writeLog(s);
}

void TWebServer::writeSessionLog(const TWebSession * session, const std::string& s, int verbosity) const {
	if (util::assigned(infoLog) && web.verbosity >= verbosity) {
		std::string header = "[" + name + "] ";
		if (util::assigned(session)) {
			std::string sid, username, host;
			session->getDefaultValues(sid, username, host);
			if (web.auth != HAT_DIGEST_NONE && session->authenticated) {
				header = header + "[" + sid + ":" + username + "@" + host + "] ";
			} else {
				header = header + "[" + sid + "@" + host + "] ";
			}
		}
		infoLog->write(header + s);
	}
}


void TWebServer::writeErrorLog(const std::string& s) const {
	if (util::assigned(exceptionLog))
		exceptionLog->write("[" + name + "] " + s);
	if (util::assigned(infoLog))
		infoLog->write("[" + name + "] " + s);
}


void TWebServer::logHandler( const char *uri,
							 struct MHD_Connection *connection ) {
	if (web.verbosity >= 3)
		writeInfoLog(util::cprintf("[Internal logger] Received request for [%s] from client IP [%s]", uri, inet::inetAddrToStr(connection->addr).c_str()));
}


void TWebServer::customErrorLog(const char *fmt, va_list va) {
	std::string s;
	if (util::assigned(fmt)) {
		std::string text = util::cprintf(fmt, va);
		s = "[INTERNAL ERROR] " + util::TBinaryConvert::binToText(text.c_str(), text.size(), util::TBinaryConvert::EBT_TEXT);
	} else {
		s = "[INTERNAL ERROR] Undefined internal error raised.";
	}
	writeInfoLog(s);
}


void TWebServer::panicCallback ( const char *file, unsigned int line, const char *reason ) {
	std::string unit = util::strToStr(file, "undefined");
	std::string error = util::trim(util::strToStr(reason, "unknown"));
	std::string text = util::TBinaryConvert::binToText(error.c_str(), error.size(), util::TBinaryConvert::EBT_TEXT);
	std::string s = "[DAEMON PANIC] Trapped \"" + text + "\" in file \"" + unit + "\" at line (" + std::to_string((size_s)line) + ")";
	writeErrorLog(s);
	//throw util::app_error("[" + name + "] " + s);
}

void TWebServer::requestCompletedCallback (	struct MHD_Connection *connection,
											void **con_cls,
											enum MHD_RequestTerminationCode toe ) {
	PWebRequest o = (static_cast<app::PWebRequest>(*con_cls));
	int httpStatusCode = MHD_HTTP_OK;
	bool verbose = web.verbosity > 2;
	*con_cls = nil;
	if (util::assigned(o)) {
		if (verbose)
			writeInfoLog("[Request completion callback] Finalize request for session <" + o->getSession()->sid + ">");
		// Finalize request:
		// --> Decrement reference count, set timestamp, etc.
		httpStatusCode = o->getStatusCode();
		o->finalize();
		if (verbose) {
			writeInfoLog("[Request completion callback] Request reference count is now " + std::to_string((size_s)o->getRefCount()));
		}
	}
	if (verbose) {
		std::string wsc = getWebStatusMessage((EWebStatusCode)httpStatusCode);
		writeInfoLog(util::cprintf("[Request completion callback] Request completed for URL [%s] with HTML response code [%d] \"%s\" and termination code [%d]",
				connection->url, httpStatusCode, wsc.c_str(), toe));
	}
}


MHD_Result TWebServer::acceptHandler( const struct sockaddr *addr, socklen_t addrlen ) {
	std::string ip = inet::inetAddrToStr(addr);

	// Accept connection on running server only!
	// Attention: Existing connection are not effected, requests will still be accepted!
	if (isResponding()) {

		// Check for trusted IPs/networks
		if (web.verbosity >= 3)
			writeInfoLog("[Accept Handler] Connect from client IP [" + ip + "]", 3);

		if (web.allowFromAll)
			return MHD_YES;

		if (web.allowedList.empty())
			return MHD_YES;

		app::TStringVector::const_iterator it = web.allowedList.begin();
		while (it != web.allowedList.end()) {
			const std::string& s = *it;
			if (!s.empty()) {
				if (0 == util::strncasecmp(ip, s, s.size())) {
					return MHD_YES;
				}
			}
			it++;
		}

		writeInfoLog("[Accept Handler] Client IP [" + ip + "] is blocked.");
		return MHD_NO;
	}

	writeInfoLog("[Accept Handler] Server paused, client IP [" + ip + "] is rejected.");
	return MHD_NO;
}



MHD_Result TWebServer::requestHandler(	struct MHD_Connection *connection,
										const char *url,
										const char *method,
										const char *version,
										const char *upload_data,
										size_t *upload_data_size,
										void **con_cls ) {
	// Check prerequisites...
	if (!util::assigned(connection)) {
		writeErrorLog("TWebServer::requestHandler() Prerequisite failed: \"Connection object missing\"");
		return MHD_NO;
	}
	if (!util::assigned(url)) {
		writeErrorLog("TWebServer::requestHandler() Prerequisite failed: \"No URL given\"");
		return MHD_NO;
	}
	if (!util::assigned(method)) {
		writeErrorLog("TWebServer::requestHandler() Prerequisite failed: \"No HTTP method given\"");
		return MHD_NO;
	}
	if (!util::assigned(version)) {
		writeErrorLog("TWebServer::requestHandler() Prerequisite failed: \"No HTTP version given\"");
		return MHD_NO;
	}

	// Execute request handler
	try {
		return requestDispatcher(connection, url, method, version, upload_data, upload_data_size, con_cls);
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		writeErrorLog(util::csnprintf("TWebServer::requestHandler() Exception $", sExcept));
	} catch (...)	{
		writeErrorLog("TWebServer::requestHandler() Unknown exception.");
	}

	// Execution failed on exception
	return MHD_NO;
}

MHD_Result TWebServer::requestDispatcher( struct MHD_Connection *connection,
								 	 	 const char *url,
										 const char *method,
										 const char *version,
										 const char *upload_data,
										 size_t *upload_data_size,
										 void **con_cls ) {
	MHD_Result retVal = MHD_NO;
	int error = MHD_HTTP_NOT_FOUND;
	int result = MHD_HTTP_OK;
	std::string URL(url);
	EHttpMethod httpMethod = getHttpMethod(method);
	bool post = util::isMemberOf(httpMethod, HTTP_POST,HTTP_PUT);
	writeInfoLog(util::cprintf("[Request handler] Prepare requested URL [%s], method [%s], version [%s]", url, method, version), 3);

	// Check for first call to requestHandler
	// --> Prepare response objects
	if (!util::assigned(*con_cls)) {

		// Log request callback
		if (web.verbosity >= 3) {
			if (post || *upload_data_size > 0)
				writeInfoLog(util::cprintf("[Request handler] Prepare requested URL [%s], method [%s], version [%s], size [%d]", url, method, version, *upload_data_size), 3);
			else
				writeInfoLog(util::cprintf("[Request handler] Prepare requested URL [%s], method [%s], version [%s]", url, method, version), 3);
		}

		// Create response handler
		bool created = false;
		PWebRequest request = findRequestSlot(connection, created);

		// Limit web session count
		// --> Limit the memory consumtion in case of brute force attack
		// --> Session created but never used afterwards....
		if (created) {
			sessionCountLimiter();
		}

		// Add POST iterator if needed
		retVal = MHD_YES;
		if (post && isResponding()) {
			retVal = request->addPostIterator(connection, URL, httpMethod, *upload_data_size, &app::TWebServer::postIteratorHandler, this);
			if (MHD_YES == retVal) {
				if (web.verbosity >= 3) {
					EWebPostMode mode = request->getPostMode();
					if (WPM_HTML == mode)
						writeInfoLog(util::cprintf("[Request handler] Prepare requested URL [%s] as default HTML POST request. ", url));
					else
						writeInfoLog(util::cprintf("[Request handler] Prepare requested URL [%s] as unspecific data upload POST request.", url));
				}
			} else {
				writeInfoLog(util::cprintf("[Request handler] Prepare requested URL [%s], method [%s] failed: Can't create any post processor handler.", url, method));
			}
		}

		// Log browser connection values
		if (web.verbosity >= 2 && !request->getSession()->valuesRead)
			logConnectionValues(request, connection);

		// Return with object set to current request handler
		*con_cls = request;

	} else { // if (!util::assigned(*con_cls))

		// Log request callback
		if (web.verbosity >= 4) {
			if (post || *upload_data_size > 0)
				writeInfoLog(util::cprintf("[Request handler] Execute prepared URL [%s], method [%s], version [%s], size [%d]", url, method, version, *upload_data_size));
			else
				writeInfoLog(util::cprintf("[Request handler] Execute prepared URL [%s], method [%s], version [%s]", url, method, version));
		}

		// Serve requested file
		PWebRequest request = static_cast<app::PWebRequest>(*con_cls);
		if (util::assigned(request)) {
			bool reply = true;
			bool found = false;
			bool failed = false;

			// Digest authentication
			bool authenticated = true;
			if (!request->isAuthenticated()) {
				if (web.auth != HAT_DIGEST_NONE) {
					// Execute digest authentication...
					app::TLockGuard<app::TMutex> mtx(userMtx);
					authenticated = request->authenticate(connection, web.users, web.sessionExpired, error);
				} else {
					// Authenticate request to default level 3 to allow administrative access
					authenticated = request->authenticate(DEFAULT_USER_LEVEL);
				}
			}

			// Allow access to web application files via GET/HEAD/POST without any authentication level
			if (web.auth != HAT_DIGEST_NONE && !authenticated && !allowedPagesList.empty()) {
				size_t len = strnlen(url, maxUrlSize);
				for (size_t i=0; i<allowedPagesList.size(); ++i) {
					const std::string& page = allowedPagesList[i];
					if (!authenticated && len >= page.size()) {
						authenticated = 0 == util::strncasecmp(url, page, page.size());
					}
					if (authenticated)
						break;
				}
				if (!authenticated && error == MHD_HTTP_NOT_FOUND) {
					error = MHD_HTTP_FORBIDDEN;
				}
				if (authenticated && web.verbosity > 0) {
					std::string ip(inet::inetAddrToStr(connection->addr));
					writeInfoLog(util::cprintf("[Request handler] Allow unrestricted access to URL [%s] from client [%s]", url, ip.c_str()));
				}
			}

			// Process authenticated request
			if (authenticated) {

				// Check if response allowed (server is in running mode)
				if (isResponding()) {
					bool goon = true;

					// Check for Websocket upgrade request
					if (util::assigned(sockets) && request->isUpgradeRequest()) {
						if (MHD_YES == request->sendUpgradeResponse(connection, webSocketConnectionDispatcher, this)) {
							goon = false;
							found = true;
							retVal = MHD_YES;
							std::string ip(inet::inetAddrToStr(connection->addr));
							writeLog(util::cprintf("[Request handler] Spawned web socket [%s] for client [%s]", url, ip.c_str()));
						}
					}

					// Process HTML request
					if (goon) {

						// Call action for given method
						switch (httpMethod) {
							case HTTP_PUT:
							case HTTP_POST:
							case HTTP_PATCH:
							case HTTP_DELETE:
								if (*upload_data_size > 0) {

									// Do some logging on post/upload data
									if (web.verbosity >= 2) {
										if (!request->isMultipartMessage()) {
											if (request->isXMLHttpRequest())
												writeInfoLog(util::cprintf("[Request handler] Execute POST for XML HTTP request [%s]," \
														" posted data size [%u]", url, *upload_data_size), 2);
											else
												writeInfoLog(util::cprintf("[Request handler] Execute POST for URL [%s]," \
														" posted data size [%u]", url, *upload_data_size), 2);

										} else {
											// Log browser header values
											if (web.verbosity >= 4)
												logConnectionValues(request, connection, true);
										}
									}

									// Parse POSTed data
									request->executePostProcess(upload_data, upload_data_size);

									// All data processed
									*upload_data_size = 0;
									return MHD_YES;

								} else {

									// No more data delivered by callback
									// --> Terminate current POST process
									PWebSession session = request->getSession();
									if (util::assigned(session)) {
										EWebPostMode mode = request->getPostMode();

										// File upload or post process finished?
										if (WPM_HTML == mode) {
											terminateFileUpload(request, session);
											terminatePostProcess(request, session, result);
											if (web.verbosity >= 2)
												writeInfoLog(util::cprintf("[Request handler] Execute GET after POST for URL [%s]", url), 2);

											// Execute GET after POST!
											reply = true;
										}

										// Posted data upload finished
										if (WPM_DATA == mode) {
											util::TBuffer& data = request->getPostData();
											error = execDataUpload(request, session, URL, data);
											if (web.verbosity >= 2)
												writeInfoLog(util::cprintf("[Request handler] Finished POST data upload for URL [%s]", url), 2);

											// Do NOT execute GET after POST!
											reply = false;
										}
									}

								}

							/* Intended to serve URL after POST processing */
							/* no break : Text disables warning in Eclipse */
							case HTTP_GET:
							case HTTP_HEAD:
							case HTTP_OPTIONS:
							case HTTP_SUBSCRIBE:
								if (reply) {
									util::TFile inode;
									util::PFile file = nil;
									app::PWebLink link = nil;
									app::PWebDirectory pwd = nil;

									// Find file for requested URL
									// For performance reasons check for buffered files first!
									if (!found && !content.empty()) {
										file = findFileForURL(URL);
										if (util::assigned(file)) {
											if (web.verbosity > 0) {
												const char* pFile = file->getFile().empty() ? "none" : file->getFile().c_str();
												writeInfoLog(util::cprintf("[Request handler] Prepare buffered file for URL [%s], file [%s], method [%s]", url, pFile, method));
											}
											found = true;
										}
									}

									// Check for Restful API data URL
									if (!found && !rest.empty()) {
										link = rest.find(URL);
										if (util::assigned(link)) {
											if (web.verbosity > 0) {
												writeInfoLog(util::cprintf("[Request handler] Prepare virtual file for URL [%s], method [%s]", url, method));
											}
											found = true;
										}
									}

									// Check for virtual directory
									if (!found && !vdl.empty()) {
										// Example:
										//  - Web request for http://music/Tangerine Dream/Tangram/folder.jpg
										//  - Requested URL is "/music/Tangerine Dream/Tangram/folder.jpg"
										//  - Folder on file system is "/mnt/data/music/files/"
										//  - Folder "/mnt/data/music/files/" is mapped to virtual root directory "/music/"
										//  - The real file name is "/mnt/data/music/files/Tangerine Dream/Tangram/folder.jpg"
										pwd = findDirectoryForURL(inode, URL);
										if (util::assigned(pwd)) {
											if (web.verbosity > 0) {
												const char* pFile = inode.getFile().empty() ? "none" : inode.getFile().c_str();
												writeInfoLog(util::cprintf("[Request handler] Prepare virtual directory for URL [%s], file [%s], method [%s]", url, pFile, method));
											}
											found = true;
										}
									}

									// Ressource was found
									if (found) {

										// Get mutex from requested ressource
										// --> Lock requested ressource only if requested by prepared method!
										// --> Fallback to global mutex
										app::TMutex& lock = util::assigned(file) ? file->getMutex() :
																(util::assigned(link) ? link->getMutex() :
																		(util::assigned(pwd) ? pwd->getMutex() : prepareMtx));

										// Process given request
										app::TLockGuard<app::TMutex> mtx(lock, false);
										bool prepared = false;
										bool processed = false;
										found = false;

										// Grab result from POST processing
										error = result;

										// Prepare web request
										PWebSession session = request->getSession();
										request->readUriArguments(connection);
										if (!post && request->hasURIArguments() && util::assigned(session)) {
											mtx.lock();
											prepareRequest(request, session, URL, prepared);
											if (!prepared) {
												// Nothing prepared --> unlock mutex
												mtx.unlock();
											}
										}

										// Update application cookies
										updateApplicationValues(session);

										// Authenticate user via HTTP form or serve requested URL
										// Allow all other files like css, js, ...
										// if (web.useAuth && !o->getSession()->authenticated)
										//     authenticate(file, URL, url);

										// Find file for requested URL
										// For performance reasons check for buffered files first!
										if (!processed && !found && !failed && util::assigned(file)) {
											if (web.verbosity > 0) {
												const char* pFile = file->getFile().empty() ? "none" : file->getFile().c_str();
												writeSessionLog(session, util::cprintf("[Request handler] Serve buffered file for URL [%s], file [%s], method [%s]", url, pFile, method));
											}
											retVal = request->sendResponseFromFile(connection, httpMethod, WTM_ASYNC, web.caching, file, data.bytesServed, error);
											retVal == MHD_YES ? found = true : failed = true;
											processed = true;
										}

										// Check for Restful API data URL
										if (!processed && !found && !failed && util::assigned(link)) {
											// Send data for link
											if (web.verbosity > 0) {
												writeSessionLog(session, util::cprintf("[Request handler] Serve virtual file for URL [%s], method [%s]", url, method));
											}
											retVal = request->sendResponseFromVirtualFile(connection, httpMethod, WTM_ASYNC, false, link, url, data.bytesServed, error);
											retVal == MHD_YES ? found = true : failed = true;
											processed = true;
										}

										// Check for virtual directory
										if (!processed && !found && !failed && util::assigned(pwd) && inode.valid()) {
											// Example:
											//  - Web request for http://music/Tangerine Dream/Tangram/folder.jpg
											//  - Requested URL is "/music/Tangerine Dream/Tangram/folder.jpg"
											//  - Folder on file system is "/mnt/data/music/files/"
											//  - Folder "/mnt/data/music/files/" is mapped to virtual root directory "/music/"
											//  - The real file name is "/mnt/data/music/files/Tangerine Dream/Tangram/folder.jpg"
											if (web.verbosity > 0) {
												const char* pFile = inode.getFile().empty() ? "none" : inode.getFile().c_str();
												writeSessionLog(session, util::cprintf("[Request handler] Serve virtual directory for URL [%s], file [%s], method [%s]", url, pFile, method));
											}
											// Send native file response...
											retVal = request->sendResponseFromDirectory(connection, httpMethod, web.caching, *pwd, inode, data.bytesServed, error);
											retVal == MHD_YES ? found = true : failed = true;
											processed = true;
										}

									} else { // if (found)
										if (web.verbosity > 0) {
											writeInfoLog(util::cprintf("[Request handler] File not found for URL [%s], method [%s]", url, method));
										}
										error = MHD_HTTP_NOT_FOUND;
										failed = true;
									}

								} // if (reply)
								break;

							default:
								error = MHD_HTTP_NOT_IMPLEMENTED;
								break;

						} // switch (httpMethod)

					}

				} else { // if (isResponding())
					error = MHD_HTTP_SERVICE_UNAVAILABLE; // MHD_HTTP_NOT_ACCEPTABLE
					failed = true;
				}

			} else { // if (authenticated)
				// Authentication failed...
				failed = true;
			}

			// Error handling
			if (!found /* TODO: Check needed? && MHD_HTTP_OK != error*/) {
				if (reply) {
					if (!failed)
						error = MHD_HTTP_NOT_FOUND;
				}
				retVal = sendErrorResponse(connection, request, url, httpMethod, method, error);
			}

		} else { // if (util::assigned(request))
			writeErrorLog("[INTERNAL ERROR] No valid instance found for processing request.");
		}

		// Store statistic data
		data.requestCount++;
		data.sessionCount = sessionMap.size();
		data.requestQueue = requestList.size();
		data.actionQueue  = actionList.size();

	} // else if (!util::assigned(*con_cls))

	return retVal;
}


void TWebServer::prepareRequest(PWebRequest request, PWebSession session, const std::string& URL, bool& prepared) const {

	// Walk through event handler list
	if (!prepareHandlerList.empty()) {

		// Prepare requests with URI
		if (util::assigned(request) && util::assigned(session)) {
			if (web.verbosity >= 2)
				writeInfoLog(util::cprintf("[Request handler] Prepare URL [%s] with %d arguments.", URL.c_str(), request->getURIArguments().size()), 2);
			bool prepare;

			// Walk through event handler list
#ifdef STL_HAS_RANGE_FOR
			for (TWebPrepareHandler handler : prepareHandlerList) {
				prepare = false;
				try {
					// Lock session matrix value access
					app::TReadWriteGuard<app::TReadWriteLock> lock(session->matrixLck, RWL_READ);
					handler(URL, request->getURIArguments(), session->getSessionValuesWithNolock(), prepare);
					if (!prepared) {
						if (prepare) {
							prepared = true;
						}
					}
				} catch (const std::exception& e) {
					std::string sExcept = e.what();
					std::string sName = util::nameOf(handler);
					writeErrorLog(util::csnprintf("[Request handler] Exception $ in $", sExcept, sName));
					prepared = false;
				} catch (...) {
					std::string sName = util::nameOf(handler);
					writeErrorLog("[Request handler] Unknown exception in $");
					writeErrorLog(util::csnprintf("[Request handler] Unknown exception in $", sName));
					prepared = false;
				}
			}
#else
			size_t i,n;
			TWebPrepareHandler handler;
			n = prepareHandlerList.size();
			for (i=0; i<n; i++) {
				handler = prepareHandlerList[i];
				prepare = false;
				try {
					// Lock session matrix value access
					app::TReadWriteGuard<app::TReadWriteLock> lock(session->matrixLck, RWL_READ);
					handler(URL, request->getURIArguments(), session->getSessionValues(), prepare);
					if (!prepared) {
						if (prepare) {
							prepared = true;
						}
					}
				} catch (const std::exception& e)	{
					std::string sExcept = e.what();
					std::string sName = util::nameOf(handler);
					writeErrorLog(util::csnprintf("[Request handler] Exception $ in $", sExcept, sName));
					prepared = false;
				} catch (...)	{
					std::string sName = util::nameOf(handler);
					writeErrorLog("[Request handler] Unknown exception in $");
					writeErrorLog(util::csnprintf("[Request handler] Unknown exception in $", sName));
					prepared = false;
				}
			}
#endif
		}
	}
}


void TWebServer::webSocketHandler(void *cls, struct MHD_Connection *connection, void *con_cls, const char *extra_in, size_t extra_in_size, MHD_socket sock, struct MHD_UpgradeResponseHandle *urh) {
	if (util::assigned(sockets)) {
		sockets->upgrade(sock, urh, extra_in, extra_in_size);
	}
}


void TWebServer::onSockeConnect(const app::THandle handle) {
	util::TVariantValues msg;
	msg.add("websocket", "connect");
	msg.add("handle", handle);
	msg.add("delay", web.refreshTimer);
	msg.add("host", application.getHostName());
	write(handle, msg);
}

void TWebServer::onSocketData(const app::THandle handle, const std::string& message) {
	if (web.debug) std::cout << "TWebServer::onSocketData() Data \"" << util::TBinaryConvert::binToText(message.c_str(), message.size(), util::TBinaryConvert::EBT_HEX) << "\" size = " << message.size() << std::endl;
	if (!webSocketCientDataList.empty()) {
#ifdef STL_HAS_RANGE_FOR
			for (TWebSocketDataHandler handler : webSocketCientDataList) {
				try {
					handler(handle, message);
				} catch (const std::exception& e) {
					std::string sExcept = e.what();
					std::string sName = util::nameOf(handler);
					writeErrorLog(util::csnprintf("[Web socket handler] Exception $ in $", sExcept, sName));
				} catch (...) {
					std::string sName = util::nameOf(handler);
					writeErrorLog("[Web socket handler] Unknown exception in $");
					writeErrorLog(util::csnprintf("[Web socket handler] Unknown exception in $", sName));
				}
			}
#else
			size_t i,n;
			TWebSocketDataHandler handler;
			n = webSocketCientDataList.size();
			for (i=0; i<n; i++) {
				handler = webSocketCientDataList[i];
				try {
					handler(handle, message);
				} catch (const std::exception& e)	{
					std::string sExcept = e.what();
					std::string sName = util::nameOf(handler);
					writeErrorLog(util::csnprintf("[Web socket handler] Exception $ in $", sExcept, sName));
				} catch (...)	{
					std::string sName = util::nameOf(handler);
					writeErrorLog("[Web socket handler] Unknown exception in $");
					writeErrorLog(util::csnprintf("[Web socket handler] Unknown exception in $", sName));
				}
			}
#endif
	}
}

void TWebServer::onSocketVariant(const app::THandle handle, const util::TVariantValues& variants) {
	if (web.debug) variants.debugOutput("TWebServer::onSocketVariant() ");
	if (!webSocketCientVariantList.empty()) {
#ifdef STL_HAS_RANGE_FOR
			for (TWebSocketVariantHandler handler : webSocketCientVariantList) {
				try {
					handler(handle, variants);
				} catch (const std::exception& e) {
					std::string sExcept = e.what();
					std::string sName = util::nameOf(handler);
					writeErrorLog(util::csnprintf("[Web socket handler] Exception $ in $", sExcept, sName));
				} catch (...) {
					std::string sName = util::nameOf(handler);
					writeErrorLog("[Web socket handler] Unknown exception in $");
					writeErrorLog(util::csnprintf("[Web socket handler] Unknown exception in $", sName));
				}
			}
#else
			size_t i,n;
			TWebSocketVariantHandler handler;
			n = webSocketCientVariantList.size();
			for (i=0; i<n; i++) {
				handler = webSocketCientVariantList[i];
				try {
					handler(handle, variants);
				} catch (const std::exception& e)	{
					std::string sExcept = e.what();
					std::string sName = util::nameOf(handler);
					writeErrorLog(util::csnprintf("[Web socket handler] Exception $ in $", sExcept, sName));
				} catch (...)	{
					std::string sName = util::nameOf(handler);
					writeErrorLog("[Web socket handler] Unknown exception in $");
					writeErrorLog(util::csnprintf("[Web socket handler] Unknown exception in $", sName));
				}
			}
#endif
	}
}


void TWebServer::logConnectionValues(PWebRequest request, struct MHD_Connection *connection, bool reload) const {
	std::lock_guard<std::mutex> lock(sessionMtx);
	PWebSession session = request->getSession();
	if (util::assigned(session)) {

		// Reload values if needed or forced
		if (!session->valuesRead || reload) {
			request->readConnectionValues(connection);
		}

		// Log browser type
		writeInfoLog("[User Agent] Browser is \"" + app::userAgentToStr(session->userAgent) + "\"", 2);

		// Log connection values
		TValueMap::const_iterator it = session->values.begin();
		int cnt = 1;
		while (it != session->values.end()) {
			writeInfoLog("[Connection value] " + std::to_string((size_s)cnt) + ". " + it->first + " = " + it->second, 2);
			it++;
			cnt++;
		}
	}
}


void TWebServer::setApplicationValue(const std::string& key, const std::string& value) {
	std::lock_guard<std::mutex> lock(cookieMtx);
	cookies.add(key, value);
}

void TWebServer::setApplicationValue(const char * key, const std::string& value) {
	std::lock_guard<std::mutex> lock(cookieMtx);
	cookies.add(key, value);
}

std::string TWebServer::getApplicationValue(const char * key) const {
	std::lock_guard<std::mutex> lock(cookieMtx);
	return cookies[key].asString();
}

std::string TWebServer::getApplicationValue(const std::string& key) const {
	std::lock_guard<std::mutex> lock(cookieMtx);
	return cookies[key].asString();
}

void TWebServer::updateApplicationValues(PWebSession session) const {
	if (util::assigned(session)) {
		std::lock_guard<std::mutex> lock(cookieMtx);
		session->updateCookieValues(cookies);
	}
}


void TWebServer::setSessionValue(PWebRequest request, const std::string& key, const std::string& value) const {
	PWebSession session = request->getSession();
	if (util::assigned(session)) {
		session->setSessionValue(key, value);
	}
}

std::string TWebServer::getSessionValue(PWebRequest request, const std::string& key) const {
	PWebSession session = request->getSession();
	if (util::assigned(session)) {
		return session->getSessionValue(key);
	}
	return "";
}


void TWebServer::setCookieValue(PWebRequest request, const std::string& key, const std::string& value) const {
	PWebSession session = request->getSession();
	if (util::assigned(session)) {
		session->setCookieValue(key, value);
	}
}

void TWebServer::setCookieValues(PWebRequest request, const util::TVariantValues& cookies) const {
	PWebSession session = request->getSession();
	if (util::assigned(session)) {
		session->setCookieValues(cookies);
	}
}

std::string TWebServer::getCookieValue(PWebRequest request, const std::string& key) const {
	PWebSession session = request->getSession();
	if (util::assigned(session)) {
		return session->getCookieValue(key);
	}
	return "";
}


std::string TWebServer::getConnectionValue(PWebRequest request, const std::string& key) const {
	PWebSession session = request->getSession();
	if (util::assigned(session)) {
		TValueMap::const_iterator it = session->values.find(key);
		if (it != session->values.end())
			return it->second;
	}
	return "";
}


void TWebServer::authenticate(util::PFile& file, std::string& URL, const char *url) {
	bool find = true;
	if (util::assigned(file)) {
		// If HTML show greeter page instead
		if (file->isHTML()) {
			URL = web.greeterURL;
			writeInfoLog(util::cprintf("[Authentication] Requested URL [%s] is HTML.", url), 2);
		} else {
			writeInfoLog(util::cprintf("[Authentication] URL [%s] is not HTML, let pass through.", url), 2);
			find = false;
		}
	} else {
		writeInfoLog(util::cprintf("[Authentication] URL [%s] not found.", url), 2);
	}
	if (find) {
		file = findFileForURL(URL);
		writeInfoLog(util::cprintf("[Authentication] URL [%s] is redirected to greeter page.", url), 2);
	}
}


util::PFile TWebServer::findFileInContent(const std::string& URL) {
	if (!URL.empty()) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(requestLck, RWL_READ);

		// Avoid string copy, so check first to do trimming or not
		// --> Speedup locking duration ;-)
		if (URL[util::pred(URL.size())] == '/')
			return content.find(content.trim(URL));
		else
			return content.find(URL);
	}

	return nil;
}


util::PFile TWebServer::findFileForURL(std::string& URL) {
	util::PFile file = findFileInContent(URL);

	// Look for index file if given URL only
	if (util::assigned(file) && URL.size() >= 1) {

		// URL for folder only valid with trailing slash
		if (file->isFolder() && URL[util::pred(URL.size())] == '/') {

			// Check for index file in folder
			std::string s;
			app::TStringVector::const_iterator it = web.indexPages.begin();
			while (it != web.indexPages.end()) {
				s = URL + *it;
				file = findFileInContent(s);
				if (util::assigned(file)) {
					URL = s;
					break;
				}
				it++;
			}
		}
	}

	// Check if valid file found
	if (util::assigned(file)) {
		// Check for valid file and if file loaded
		if (file->isFile() && file->isLoaded()) {
			return file;
		}
	}

	return nil;
}


PWebDirectory TWebServer::findDirectoryForURL(util::TFile& file, std::string& URL) {
	std::string fileName;

	PWebDirectory o = vdl.getDirectory(URL);
	if (util::assigned(o)) {

		if (o->enabled()) {
			fileName = o->getFileName(URL);

			// Rewrite filename if requested by configuration
			if (!o->redirect().empty())
				fileName = rewriteFileName(o->redirect(), fileName);

			// Look for index page if URL is folder
			if (fileName.empty()) {

				// Check for index file in folder
				std::string s;
				app::TStringVector::const_iterator it = web.indexPages.begin();
				while (it != web.indexPages.end()) {
					s = o->directory() + *it;
					if (util::fileExists(s)) {
						URL += *it;
						fileName = s;
						break;
					}
					it++;
				}
			}
		}
	}

	// Set file instance if valid filename found
	if (!fileName.empty()) {
		file.assign(fileName);
	}

	// Check for valid file (size > 0, no folder, ...)
	if (file.valid()) {
		return o;
	}

	return nil;
}


std::string TWebServer::rewriteFileName(const std::string& baseName, const std::string& fileName) {
	if (!fileName.empty() && !util::fileExists(fileName)) {

		std::string name = util::fileBaseName(fileName);
		if (name == baseName) {

			std::string ext = util::fileExt(fileName);
			if (!ext.empty()) {

				std::string folder = util::filePath(fileName);
				util::TFolderList files;
				files.scan(folder, "*." + ext, util::SD_ROOT);
				if (!files.empty()) {
					util::PFile o = files[0];
					if (util::assigned(o)) {
						writeInfoLog("[File redirect] File <" + fileName + "> not found, redirecting to <" + o->getFile() + ">");
						return o->getFile();
					}
				}
			}
		}
	}
	return fileName;
}


std::string TWebServer::rewriteFileName(const util::TStringList& rewrite, const std::string& fileName) {
	if (!fileName.empty() && !util::fileExists(fileName) && !rewrite.empty()) {

		std::string name = util::fileBaseName(fileName);
		for (size_t i=0; i<rewrite.size(); ++i) {

			std::string baseName = rewrite[i];
			if (name == baseName) {

				std::string ext = util::fileExt(fileName);
				if (!ext.empty()) {

					std::string folder = util::filePath(fileName);
					util::TFolderList files;
					files.scan(folder, "*." + ext, util::SD_ROOT);
					if (!files.empty()) {
						util::PFile o = files[0];
						if (util::assigned(o)) {
							writeInfoLog("[File redirect] File <" + fileName + "> not found, redirecting to <" + o->getFile() + ">");
							return o->getFile();
						}
					}
				}
			}
		}
	}
	return fileName;
}


int TWebServer::getRequestDelta() const {
	return web.maxSessionCount / (2 * std::max(data.deltaRequestCount, web.maxSessionCount / 10));
}

PWebRequest TWebServer::findRequestSlot(struct MHD_Connection *connection, bool& created) {
	std::lock_guard<std::mutex> lock(requestMtx);
	PWebRequest request = nil;
	bool verbose = web.verbosity > 3;
	int delta = getRequestDelta();
	time_t now = util::now();
	created = false;

	// Reuse request if reference count == 0
	// and request older than 2 seconds
	size_t i,n;
	PWebRequest o;
	n = requestList.size();
	for (i=0; i<n; i++) {
		o = requestList[i];
		if (util::assigned(o)) {
			if ((o->getRefCount() <= 0) && ((now - o->getTimeStamp()) > 2)) {
				if (verbose)
					writeInfoLog("[Request slot finder] Found usable request.");
				request = o;
				break;
			}
		}
	}

	if (util::assigned(request)) {
		// Initialize reused request object
		request->initialize(connection, delta);
	} else {
		// Create new request object
		request = new TWebRequest(connection, sessionMap, sessionMtx, requestMtx, delta, web);
		if (util::assigned(request)) {
			requestList.push_back(request);
			created = true;
			if (verbose)
				writeInfoLog("[Request slot finder] Create new request.");
		}
	}

	if (util::assigned(request)) {
		request->setOwner(this);
	}

	return request;
}


PWebAction TWebServer::findActionSlot() {
	std::lock_guard<std::mutex> lock(actionMtx);
	PWebAction o = nil;
	bool found = false;

	// Find empty action slot
	size_t i,n;
	n = actionList.size();
	for (i=0; i<n; i++) {
		o = actionList[i];
		if (util::assigned(o)) {
			if (!o->running) {
				found = true;
				break;
			}
		}
	}

	// Create new action object
	if (!found) {
		o = new TWebAction;
		actionList.push_back(o);
	}

	// Set running flag as early as possible
	o->running = true;

	return o;
}


void TWebServer::executeAction(const TWebActionHandler& handler, const std::string& key, const std::string& value,
		const util::TVariantValues& params, const util::TVariantValues& session, const EWebActionMode mode, int& error) {
	if (web.verbosity >= 2) {
		switch (mode) {
			case WAM_SYNC:
				writeInfoLog("[Action handler] Execute synchronous action for [" + key + "] = [" + util::ellipsis(value, 50) + "]", 2);
				break;
			case WAM_ASYNC:
				writeInfoLog("[Action handler] Execute threaded action for [" + key + "] = [" + util::ellipsis(value, 50) + "]", 2);
				break;
		}
	}

	// Create action object for thread execution
	bool executed = false;
	if (mode == WAM_ASYNC) {
		PWebAction slot = findActionSlot();
		if (util::assigned(slot)) {
			TWebAction& action = *slot;
			action.key = key;
			action.value = value;
			action.handler = handler;
			action.params = params;
			action.session = session;

			// Execute action immediately via thread function
			// --> same handling via actionExecuter() as asynchronous execution
			executer.run(action);
			executed = true;
		}
	}
	
	// Execute action "in place"
	if (!executed) {
		actionSyncExecuter(handler, key, value, params, session, error);
	}
}


void TWebServer::actionAsyncExecuter(TWebAction& action) {
	int error;
	actionSyncExecuter(action.handler, action.key, action.value, action.params, action.session, error);
	std::lock_guard<std::mutex> lock(actionMtx);
	action.reset();
}

void TWebServer::actionSyncExecuter(const TWebActionHandler& handler, const std::string& key,
		const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	try {
		if (web.verbosity >= 2) {
			writeInfoLog("[Action handler] Executed action for [" + key + "] = [" + util::ellipsis(value, 50) + "]", 2);
		}
		handler(key, value, params, session, error);
	} catch (const std::exception& e) {
		std::string sExcept = e.what();
		std::string sName = "TWebServer::actionExecuter(\"" + key + "\", \"" + value + "\")";
		std::string sText = "[EXCEPTION] Exception in " + sName + " :\n" + sExcept + "\n";
		writeErrorLog(sText);
	} catch (...) {
		std::string sName = "TWebServer::actionExecuter(\"" + key + "\", \"" + value + "\")";
		std::string sText = "[EXCEPTION] Exception in " + sName;
		writeErrorLog(sText);
	}
}


/*
 * There are two (or three with file upload) cases for post iteration:
 *
 * A. No data offset (just one callback)
 *
 *  1. Call: [Post iterator] POST iterator called with parameters len = 15, offset = 0, size = 396
 *  2. Call: [Post iterator] POST iterator called with parameters len = 15, offset = 396, size = 0
 *
 * B. Data is split in several calls using offset handling
 *
 *  1. Call: [Post iterator] POST iterator called with parameters len = 15, offset = 0, size = 512
 *  2. Call: [Post iterator] POST iterator called with parameters len = 15, offset = 512, size = 296
 *  3. Call: [Post iterator] POST iterator called with parameters len = 15, offset = 808, size = 0
 *
 * Parameter size = 0 always signs, that all data transferred (with off = full size of data)
 *
 */
MHD_Result TWebServer::postIteratorHandler (	PWebRequest request,
												enum MHD_ValueKind kind,
												const char *key, size_t len,
												const char *filename,
												const char *content_type,
												const char *transfer_encoding,
												const char *data, uint64_t off, size_t size ) {
	// Check for valid caller
	if (!util::assigned(request)) {
		writeErrorLog("[INTERNAL ERROR] POST iterator has no valid caller.");
		return MHD_NO;
	}

	// Read current session
	PWebSession session = request->getSession();
	if (!util::assigned(session)) {
		writeErrorLog("[INTERNAL ERROR] No valid session found for POST iterator.");
		return MHD_NO;
	}

	// Execute file upload or process post data
	return prepareFileUpload(request, session, filename, content_type, size) ?
			execFileUpload(session, data, off, size) :
			execPostData(request, session, key, len, data, off, size);
}


bool TWebServer::prepareFileUpload(PWebRequest request, PWebSession session, const char *filename, const char *content_type, size_t size) {
	bool retVal = false;

	// Check for multipart file content
	if (request->isMultipartMessage() && util::assigned(filename)) {

		// Start new upload
		// std::lock_guard<std::mutex> lock(session->upload.fileMtx);
		if (session->upload.state == WTS_IDLE) {

			std::string fileName = util::fileExtName(filename);
			if (fileName.size()) {
				session->upload.fileName = util::uniqueFileName(web.uploadFolder + fileName);

				// Set estimated file size
				// Content size should always greater than actual file size
				// --> resize file afterwards
				session->upload.fileSize = session->upload.contentSize > 0 ? request->getContentLength() : 5 * size;

				// Set file properties for new upload
				session->upload.file.assign(session->upload.fileName);
				session->upload.file.create(session->upload.fileSize);
				session->upload.state = WTS_PROGRESS;
				session->upload.setTimeStamp();

				if (web.verbosity >= 2)
					writeInfoLog("[File Upload] Start upload of file [" + session->upload.fileName + \
							"], estimated file size: " + std::to_string((size_u)session->upload.fileSize) + " bytes.");
			}

		}

		retVal = session->upload.state == WTS_PROGRESS;
	}
	return retVal;
}


MHD_Result TWebServer::execFileUpload(PWebSession session, const char *data, uint64_t off, size_t size) {
	MHD_Result retVal = MHD_YES;

	// Upload in progress?
	// std::lock_guard<std::mutex> lock(session->upload.fileMtx);
	if (session->upload.state == WTS_PROGRESS) {

		// Open file if not yet happened
		if (!session->upload.file.isOpen())
			session->upload.file.open(O_WRONLY);

		// Write buffer to file
		size_t n = session->upload.file.write(data, size);
		if (n > 0) {
			// n bytes written to file
			session->upload.dataSize += n;
			if (web.verbosity >= 4)
				writeInfoLog("[File Upload] Write " + std::to_string((size_s)n) + " bytes to file [" + session->upload.fileName + "]");
		} else {
			// Write failed, inhibit further write operations for current download
			session->upload.state = WTS_INTERRUPTED;
			session->upload.errnum = errno;
			session->upload.file.close();
			retVal = MHD_NO;
			writeErrorLog("[UPLOAD ERROR] Writing to file [" + session->upload.fileName + "] failed: " + sysutil::getSysErrorMessage(session->upload.errnum));
		}

	}
	return retVal;
}


void TWebServer::terminateFileUpload(PWebRequest request, PWebSession session) {
	// Upload in progress?
	// std::lock_guard<std::mutex> lock(session->upload.fileMtx);
	if (util::isMemberOf(session->upload.state, WTS_PROGRESS,WTS_INTERRUPTED)) {
		
		// Set new usage timestamps
		session->setTimeStamp();
		request->setTimeStamp();

		// Close file and set final size
		if (session->upload.file.isOpen()) {
			session->upload.file.resize(session->upload.dataSize);
			session->upload.file.close();
		}

		// Delete file on partial download
		if (session->upload.state == WTS_INTERRUPTED)
			if (util::fileExists(session->upload.fileName))
				util::deleteFile(session->upload.fileName);

		// Call event method on file upload finished
		onFileUploadEvent(session, session->upload.fileName, session->upload.dataSize);

		// Do some logging on finished upload...
		if (web.verbosity >= 2) {
			if (session->upload.state == WTS_PROGRESS) {

				// Calculate bandwidth/speed in bits per second
				time_t duration = util::now() - session->upload.timestamp;
				size_t speed = 0;
				if (duration > 0 && session->upload.dataSize > 0)
					speed = session->upload.dataSize * 8 / duration;

				writeInfoLog("[File Upload] Finished file upload for [" + session->upload.fileName + "], " + \
						std::to_string((size_u)session->upload.dataSize) + " bytes written (" + \
						util::sizeToStr(session->upload.dataSize) + " / " + util::sizeToStr(speed, 1, util::VD_BIT) + "/sec)");
			} else {
				writeErrorLog("[UPLOAD ERROR] Failed to upload file [" + session->upload.fileName + "]: " + sysutil::getSysErrorMessage(session->upload.errnum));
			}
		}

		// File upload finished
		session->upload.finalize();
	}
}


MHD_Result TWebServer::execPostData(PWebRequest request, PWebSession session, const char *key, size_t len, const char *data, uint64_t off, size_t size) {
	MHD_Result retVal = MHD_YES;

	// Parameter logging...
	if (web.verbosity >= 2) {
		writeInfoLog("[Post iterator] POST iterator called with parameters " \
				 "len = " + std::to_string((size_u)len) +
				 ", offset = " + std::to_string((size_u)off) +
				 ", size = " + std::to_string((size_u)size));
	}

	// Check for valid key data when processing post data
	if (len <= 0 && session->post.state != WTS_IDLE) {
		writeErrorLog("[INTERNAL ERROR] No valid key during post processing.");
		session->post.clear();
		retVal = MHD_NO;
	}

	// Valid data, but missing key!
	if (len <= 0 && size > 0) {
		std::string sData(data, size);
		writeErrorLog("[INTERNAL ERROR] Missing key for POST iterator data [" + util::ellipsis(sData, 50)  + "], size = " + std::to_string((size_u)size));
		retVal = MHD_NO;
	}

	// Process post data?
	if (retVal == MHD_YES && len > 0) {

		// POST data processing
		switch (session->post.state) {
			case WTS_IDLE:
				// Post data starts with off == 0 and size > 0
				if (off == 0 && size > 0) {
					startPostProcess(session, key, len, data, size);
				}
				break;

			case WTS_PROGRESS:
				// Append offset data
				if (off > 0) {
					// Check for end of data
					if (size > 0) {
						// Append valid data
						continuePostProcess(session, data, size);
					} else {
						// Current offset should be transfer size...
						if (session->post.data.size() == off) {
							// Take over posted data:
							// --> Should never happen here since TWebServer::requestHandler()
							//     detects end of posted data on upload_data_size == 0!
							if (web.verbosity >= 2)
								writeInfoLog("[Post iterator] POST process terminated by post processor callback.");
							
						} else {
							// Refuse posted data:
							// Allow further post processing by leaving retVal as it is
							writeErrorLog("[INTERNAL ERROR] Posted data size incorrect (" + std::to_string((size_u)session->post.data.size()) + "/" + std::to_string((size_u)off) + ")");
							session->post.clear();
						}	
					}
				} else {
					// Offset == 0 --> New post data started
					// First process current key/data pair...
					if (web.verbosity >= 2)
						writeInfoLog("[Post iterator] POST process received new upload data.");
					nextPostProcess(session, key, len, data, size);
				}
				break;

			default:
				session->post.clear();
				retVal = MHD_NO;
				break;

		} // switch (session->post.state)

	} // if (retVal == MHD_YES && len > 0)

	// All data processed, clear buffer!
	if (!session->post.empty() && (session->post.state == WTS_IDLE || retVal == MHD_NO)) {
		session->post.clear();
	}

	return retVal;
}


void TWebServer::startPostProcess(PWebSession session, const char *key, size_t len, const char *data, size_t size) {
	// Start of new post data 
	session->post.clear();
	if (size > 0 && len > 0) {
		session->post.ident(key, len);
		session->post.assign(data, size);
		session->post.state = WTS_PROGRESS;
		if (web.verbosity >= 3) {
			writeInfoLog("[Post iterator] Assigned data [" + session->post.key +  "] = [" + util::ellipsis(session->post.data, 50) + "]");
		}
	}	
}


void TWebServer::continuePostProcess(PWebSession session, const char *data, size_t size) {
	// Append valid data
	if (size > 0) {
		session->post.append(data, size);
		if (web.verbosity >= 3) {
			writeInfoLog("[Post iterator] Appended data [" + session->post.key +  "] = [" + util::ellipsis(session->post.data, 50) + "]");
		}
	}
}


void TWebServer::nextPostProcess(PWebSession session, const char *key, size_t len, const char *data, size_t size) {
	// Store current post process data
	dataPostProcess(session);

	// Start of new post data
	if (size > 0 && len > 0) {
		session->post.ident(key, len);
		session->post.assign(data, size);
		session->post.state = WTS_PROGRESS;
		if (web.verbosity >= 3) {
			writeInfoLog("[Post iterator] Next key data [" + session->post.key +  "] = [" + util::ellipsis(session->post.data, 50) + "]");
		}
	}
}


void TWebServer::terminatePostProcess(PWebRequest request, PWebSession session, int& error) {
	if (session->post.state == WTS_PROGRESS) {

		// Store current post process data
		dataPostProcess(session);
		if (web.verbosity >= 2)
			writeInfoLog("[Post iterator] POST process terminated by empty upload data.");

		// Execute post process
		execPostAction(request, session, error);
	}
}


void TWebServer::dataPostProcess(PWebSession session) {
	if (session->post.state == WTS_PROGRESS) {
		if (!session->post.key.empty()) {
			// Store primary key/value pair
			if (session->post.query.empty()) {
				session->post.query = session->post.key;
				session->post.value = session->post.data;
			}

			// Add key/value pair to parameter list
			session->post.params.add(session->post.key, session->post.data);

			// Clear current values
			session->post.key.clear();
			session->post.data.clear();
		}
	}
}


void TWebServer::execPostAction(PWebRequest request, PWebSession session, int& error) {
	if (session->post.valid()) {
		TWebActionMap::const_iterator it = actionMap.end();

		// Post data key handling
		if (web.verbosity >= 2)
			writeInfoLog("[Post iterator] POST iterator handler called for [" + session->post.query + "] = [" + util::ellipsis(session->post.value, 50)  + "], size = " + std::to_string((size_u)session->post.value.size()), 2);

		// Check for valid posted value
		if (0 != util::strncasecmp("null", session->post.value, session->post.value.size())) {

			// Check if URL is member of action list
			app::TReadWriteGuard<app::TReadWriteLock> lock(actionLck, RWL_READ);
			it = actionMap.find(session->post.query);
			if (it != actionMap.end()) {
				// Execute given web action
				if (web.verbosity >= 4) {
					std::string value = session->post.value;
					util::writeFile(util::uniqueFileName(util::sanitizeFileName(session->post.query) + ".post"), value);
				}
				if (web.verbosity >= 2)
					writeInfoLog("[Post iterator] Execute action for key [" + session->post.query + "] = [" + util::ellipsis(session->post.value, 50)  + "]", 2);
				executeAction(it->second->action, session->post.query, session->post.value, session->post.params, session->matrix, it->second->mode, error);
			} else {
				// No special action defined, call default action instead
				it = actionMap.find(DEFAULT_WEB_ACTION);
				if (it != actionMap.end()) {
					if (web.verbosity >= 2)
						writeInfoLog("[Post iterator] Execute default action for key [" + session->post.query + "] = [" + util::ellipsis(session->post.value, 50)  + "]", 2);
					executeAction(it->second->action, session->post.query, session->post.value, session->post.params, session->matrix, it->second->mode, error);
				}
			}

		}

		// Set request counter
		if (it != actionMap.end()) {
			PWebActionItem wa = it->second;
			if (util::assigned(wa)) {
				wa->incRequested();
				
			}
		}
		
		// Post process finished
		session->post.clear();
		request->setTimeStamp();
	}
}


int TWebServer::execDataUpload(PWebRequest request, PWebSession session, const std::string& URL, util::TBuffer data) {
	int retVal = MHD_HTTP_OK;

	// Set new usage timestamps
	session->setTimeStamp();
	request->setTimeStamp();

	// Get POSTed data properties
	const size_t size = data.size();
	const bool zipped = request->isZippedContent();
	const util::TVariantValues& params = request->getURIArguments();
	if (size > 0) {
		PWebLink link = rest.find(URL);
		if (util::assigned(link)) {
			PThreadDataItem o = nil;
			link->garbageCollector();
			{
				// Lock session matrix value access
				app::TReadWriteGuard<app::TReadWriteLock> lock(session->matrixLck, RWL_READ);
				const util::TVariantValues& values = session->getSessionValuesWithNolock();
				o = link->setData(data, URL, params, values, zipped, retVal);
			}
			if (util::assigned(o)) {
				if (web.verbosity >= 2) {
					writeInfoLog(util::csnprintf("[Terminate post data] Received % bytes for restful API $", size, URL));
				}
			} else {
				writeInfoLog(util::csnprintf("[Terminate post data] Receiving % bytes for undefined restful API $ discarded.", size, URL));
				retVal = MHD_HTTP_NOT_IMPLEMENTED;
			}
		} else {
			if (web.verbosity >= 2) {
				std::string s = util::TBinaryConvert::binToAsciiA(data(), size, true);
				writeInfoLog(util::csnprintf("[Terminate post data] Received % bytes for unknown URL $ : $", size, URL, s));
				retVal = MHD_HTTP_NOT_FOUND;
			}
		}
	}

	// Clear POST request data
	request->finalizePostData();
	return retVal;

}


void TWebServer::onFileUploadEvent(PWebSession session, const std::string& fileName, const size_t& size) {
		// Walk through event handler list
#ifdef STL_HAS_RANGE_FOR
		for (TFileUploadEvent handler : uploadEventList) {
			try {
				util::TVariantValues values;
				session->getSessionValues(values);
				handler(*this, values, fileName, size);
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				writeErrorLog(util::csnprintf("TWebServer::onFileUploadEvent() Exception $", sExcept));
			} catch (...)	{
				writeErrorLog("TWebServer::onFileUploadEvent() Unknown exception.");
			}
		}
#else
		size_t i,n;
		TFileUploadEvent handler;
		n = uploadEventList.size();
		for (i=0; i<n; i++) {
			handler = uploadEventList[i];
			try {
				handler(*this, fileName, size);
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				writeErrorLog(util::csnprintf("TWebServer::onFileUploadEvent() Exception $", sExcept));
			} catch (...)	{
				writeErrorLog("TWebServer::onFileUploadEvent() Unknown exception.");
			}
		}
#endif

}

MHD_Result TWebServer::sendErrorResponse(struct MHD_Connection *connection, PWebRequest request, const char *url, const EHttpMethod methodType, const char *methodStr, int error) {
    const char *payload = nil;
    size_t size = 0;
    std::string page;
    std::string mhd(mhdVersion());
	std::string ip(inet::inetAddrToStr(connection->addr));
	bool responding = isResponding();
    const char *v = application.getVersion().c_str();
    const char *m = mhd.c_str();
    const char *i = ip.c_str();
	bool persist = false; // Source buffer is NOT persistent!
	bool caching = false;
	bool zipped = false;
	std::string message;

	// Get response mime type from requested URL
	std::string mime(HTML_MIME_TYPE);
	std::string file(url);
	if (!file.empty()) {
		std::string ext  = util::fileExt(file);
		mime = util::getMimeType(ext);
		if (DEFAULT_MIME_TYPE == mime)
			mime = HTML_MIME_TYPE;
	}

	// Create appropriate error page content
	switch (error) {
		case MHD_HTTP_OK:
			// Send 200 OK as JSON if requested by mime type of URL
			page = (JSON_MIME_TYPE == mime) ? util::cprintf(PAGE_JSON_OK, methodStr, url, i) : util::cprintf(PAGE_HTTP_OK, url, v, m, i);
			break;

		case MHD_HTTP_NOT_FOUND:
			message = util::cprintf("[ERROR %d] File not found for URL [%s], method [%s] from client [%s]", error, url, methodStr, i);
			page = util::cprintf(PAGE_HTTP_NOT_FOUND, url, v, m, i);
			break;

		case MHD_HTTP_NOT_IMPLEMENTED:
			message = util::cprintf("[ERROR %d] Requested method [%s] from client [%s] for URL [%s] not supported.", error, methodStr, i, url);
			page = util::cprintf(PAGE_HTTP_NOT_IMPLEMENTED, error, error, methodStr, v, m, i);
			break;

		case MHD_HTTP_SERVICE_UNAVAILABLE:
			message = util::cprintf("[ERROR %d] Requested service for URL [%s] not available for method [%s] from client [%s].", error, url, methodStr, i);
			page = util::cprintf(PAGE_HTTP_NOT_AVAILABLE, error, error, url, v, m, i);
			break;

		case MHD_HTTP_FORBIDDEN:
		case MHD_HTTP_UNAUTHORIZED:
			message = util::cprintf("[ERROR %d] Access denied for URL [%s] from client [%s]", error, url, i);
			page = util::cprintf(PAGE_HTTP_FORBIDDEN, error, error, url, v, m, i);
			break;

		case MHD_INVALID_NONCE:
			message = util::cprintf("[ERROR %d] Invalid nonce during authentication process for URL [%s] from client [%s]", MHD_HTTP_NOT_ACCEPTABLE, url, i);
			page = util::cprintf(PAGE_HTTP_FORBIDDEN, MHD_HTTP_NOT_ACCEPTABLE, MHD_HTTP_NOT_ACCEPTABLE, url, v, m, i);
			break;

		case MHD_HTTP_INTERNAL_SERVER_ERROR:
		default:
			std::string msg = app::getWebStatusMessage(static_cast<EWebStatusCode>(error));
			message = util::cprintf("[ERROR %d] Error \"%s\" for URL [%s], method [%s] from client [%s]", error, msg.c_str(), url, methodStr, i);
			page = util::cprintf(PAGE_HTTP_INTERNAL_SERVER_ERROR, error, error, error, msg.c_str(), v, m, i);
			break;
	}

	// Ignore error on paused webserver
	if (!message.empty()) {
		if (!responding && MHD_HTTP_SERVICE_UNAVAILABLE == error) {
			writeInfoLog(message);
		} else {
			writeErrorLog(message);
		}
	}

    // Set page response data
	if (!page.empty()) {
		size = page.size();
		payload = page.c_str();
    }

    // Send error page synchronous and force MHD to copy the content buffer (persist = false, mode = WTM_SYNC)
	// --> Avoid overwriting error buffers on concurrent requests for error pages
	if (web.verbosity > 0) {
		writeInfoLog(util::csnprintf("[HTTP Response] Send response $ (%)", app::getWebStatusMessage(static_cast<EWebStatusCode>(error)), error));
	}
	util::TVariantValues headers;
	return request->sendResponseFromBuffer(connection, methodType, WTM_SYNC, payload, size, headers, persist, caching, zipped, mime, error);
}


ssize_t TWebServer::write(const app::THandle handle, const std::string& text) const {
	if (!text.empty()) {
		return write(handle, text.c_str(), text.size());
	}
	return (ssize_t)0;
}

ssize_t TWebServer::write(const app::THandle handle, const util::TVariantValues& variants) const {
	if (!variants.empty()) {
		const std::string& s = variants.asJSON().text();
		if (!s.empty())
			return write(handle, s.c_str(), s.size());
	}
	return (ssize_t)0;
}

ssize_t TWebServer::broadcast(const std::string& text) const {
	if (!text.empty()) {
		return broadcast(text.c_str(), text.size());
	}
	return (ssize_t)0;
}

ssize_t TWebServer::broadcast(const util::TVariantValues& variants) const {
	if (!variants.empty()) {
		const std::string& s = variants.asJSON().text();
		if (!s.empty())
			return broadcast(s.c_str(), s.size());
	}
	return (ssize_t)0;
}

ssize_t TWebServer::write(const app::THandle handle, void const * const data, size_t const size) const {
	if (util::assigned(sockets)) {
		return sockets->write(handle, data, size);
	}
	return (ssize_t)0;
}

ssize_t TWebServer::broadcast(void const * const data, size_t const size) const {
	if (util::assigned(sockets)) {
		return sockets->broadcast(data, size);
	}
	return (ssize_t)0;
}


void TWebServer::createSubresourceIntegrityFile(const std::string& folder) const {
	if (util::folderExists(folder)) {
		// Create HTLM resource integrity check file for Javascript files in webroot
		std::string fileName = util::validPath(folder) + "java-integrity-checks.csv";
		if (!util::fileExists(fileName)) {
			std::string root = getWebRoot();
			util::TFolderList files;
			util::TStringList links, csv;
			TStringVector pattern = {"*.js", "*.css"};
			files.scan(root, pattern, util::SD_RECURSIVE, false);
			if (!files.empty()) {
				util::PFile o;
				std::string digest, link, entry;
				util::TDigest sha(util::EDT_SHA384);
				sha.setFormat(util::ERT_BASE64);
				for (size_t i=0; i<files.size(); ++i) {
					o = files[i];
					o->load();
					if (o->getSize() > 0) {
						digest = sha.getDigest(o->getData(), o->getSize());
						if (!digest.empty()) {
							if (o->isCSS()) {
								link = util::csnprintf("<link rel=\"stylesheet\" href=$ integrity=\"sha384-%\" crossorigin=\"anonymous\">",
									o->getFile(), digest);
							} else {
								link = util::csnprintf("<script src=$ integrity=\"sha384-%\" crossorigin=\"anonymous\"></script>",
									o->getFile(), digest);
							}
							entry = util::csnprintf("$;sha384-%;%", o->getFile(), digest, link);
							csv.add(entry);
							links.add(link);
						}
						o->deleteFileBuffer();
					}
				}
				csv.saveToFile(fileName);
				links.saveToFile(util::validPath(folder) + util::fileBaseName(fileName) + ".txt");
			}
		}
	}
}


bool webSessionSorter(PWebSessionInfo o, PWebSessionInfo p) {
	if (o->remote == p->remote)
		return o->timestamp > p->timestamp;
	return o->remote < p->remote;
}

bool webRequestSorter(PWebRequestInfo o, PWebRequestInfo p) {
	if (o->requested == p->requested)
		return util::strnatsort(o->url, p->url);
	return o->requested > p->requested;
}


void TWebServer::getWebSessionInfoList(TWebSessionInfoList& list) {
	util::clearObjectList(list);
	std::lock_guard<std::mutex> lock(sessionMtx);
	if (sessionMap.size() > 0) {
		PWebSession o;
		PWebSessionInfo p;
		TWebSessionMap::const_iterator it = sessionMap.begin();
		while (it != sessionMap.end()) {
			o = it->second;
			if (util::assigned(o)) {
				p = new TWebSessionInfo;
				o->getDefaultValues(p->sid, p->username, p->remote);
				p->authenticated = o->authenticated;
				p->password = o->password;
				p->timestamp = o->timestamp;
				p->userAgent = o->userAgent;
				p->refC = o->refC;
				p->useC = o->useC;
				list.push_back(p);
			}
			it++;
		}
		std::sort(list.begin(), list.end(), webSessionSorter);
	}
}

size_t TWebServer::getWebSessionCount() const {
	return sessionMap.size();
}


std::string TWebServer::stripRestRoot(const std::string& url) const {
	std::string s = url;
	const std::string& root = getRestRoot();
	if (!root.empty()) {
		size_t pos = url.find(root);
		if (pos == 0) {
			s = url.substr(pos + root.size());
		}
	}
	return s;
}

void TWebServer::getWebRequestInfoList(TWebRequestInfoList& list, size_t& requests, size_t threshhold) {
	requests = 0;
	util::clearObjectList(list);
	PWebRequestInfo p;

	// Collect web links
	if (rest.size() > 0) {
		PWebLink wl;
		TWebLinkList::const_iterator it = rest.begin();
		while (it != rest.end()) {
			wl = it->second;
			if (util::assigned(wl)) {
				if (wl->getRequested() >= threshhold) {
					p = new TWebRequestInfo;
					p->requested = wl->getRequested();
					p->timestamp = wl->getTimeStamp();
					p->url = stripRestRoot(wl->getURL());
					bool folder = false;
					if (!p->url.empty()) {
						folder = p->url[p->url.size() - 1] == '/';
					}
					p->type = folder ? "VFS" : "Link";
					requests += p->requested;
					list.push_back(p);
				}
			}
			it++;
		}
	}

	// Collect virtual directories
	if (vdl.size() > 0) {
		PWebDirectory wd;
		TWebDirectoryList::const_iterator it = vdl.begin();
		while (it != vdl.end()) {
			wd = it->second;
			if (util::assigned(wd)) {
				if (wd->getRequested() >= threshhold) {
					p = new TWebRequestInfo;
					p->requested = wd->getRequested();
					p->timestamp = wd->getTimeStamp();
					p->url = stripRestRoot(wd->directory());
					bool folder = false;
					if (!p->url.empty()) {
						folder = p->url[p->url.size() - 1] == '/';
					}
					p->type = folder ? "Directory" : "File";
					requests += p->requested;
					list.push_back(p);
				}
			}
			it++;
		}
	}

	// Collect web (post) actions
	if (actionMap.size() > 0) {
		PWebActionItem wa;
		TWebActionMap::const_iterator it = actionMap.begin();
		while (it != actionMap.end()) {
			wa = it->second;
			if (util::assigned(wa)) {
				if (wa->requested >= threshhold) {
					p = new TWebRequestInfo;
					p->requested = wa->requested;
					p->timestamp = wa->timestamp;
					p->url = stripRestRoot(wa->URL);
					p->type = wa->mode == app::WAM_SYNC ? "Post" : "Deferred";
					requests += p->requested;
					list.push_back(p);
				}
			}
			it++;
		}
	}	

	// Calulate relative usage by request count
	if (!list.empty()) {
		if (requests > 0) {
			TWebRequestInfoList::const_iterator it = list.begin();
			while (it != list.end()) {
				p = *it;
				if (p->requested > 0) {
					p->percent = p->requested * 100 / requests;
				}
				it++;
			}
		}

		// Sort list by request count
		std::sort(list.begin(), list.end(), webRequestSorter);
	}
}

size_t TWebServer::getWebApiCount() const {
	return rest.size() + vdl.size() + actionMap.size();
}

size_t TWebServer::getWebRequestCount() const {
	size_t requests = 0;

	// Collect web links
	if (rest.size() > 0) {
		PWebLink wl;
		TWebLinkList::const_iterator it = rest.begin();
		while (it != rest.end()) {
			wl = it->second;
			if (util::assigned(wl)) {
				requests += wl->getRequested();
			}
			it++;
		}
	}

	// Collect virtual directories
	if (vdl.size() > 0) {
		PWebDirectory wd;
		TWebDirectoryList::const_iterator it = vdl.begin();
		while (it != vdl.end()) {
			wd = it->second;
			if (util::assigned(wd)) {
				requests += wd->getRequested();
			}
			it++;
		}
	}

	// Collect web (post) actions
	if (actionMap.size() > 0) {
		PWebActionItem wa;
		TWebActionMap::const_iterator it = actionMap.begin();
		while (it != actionMap.end()) {
			wa = it->second;
			if (util::assigned(wa)) {
				requests += wa->requested;
			}
			it++;
		}
	}

	return requests;
}


void TWebServer::clearWebSessionMap() {
	std::lock_guard<std::mutex> lock(sessionMtx);
	if (sessionMap.size() > 0) {
		PWebSession o;
		TWebSessionMap::const_iterator it = sessionMap.begin();
		while (it != sessionMap.end()) {
			o = it->second;
			util::freeAndNil(o);
			it++;
		}
		sessionMap.clear();
	}
}


void TWebServer::clearWebActionMap() {
	if (!actionMap.empty()) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(actionLck, RWL_WRITE);
		PWebActionItem o;
		TWebActionMap::const_iterator it = actionMap.begin();
		while (it != actionMap.end()) {
			o = it->second;
			util::freeAndNil(o);
			it++;
		}
		actionMap.clear();
	}
}


void TWebServer::clearWebActionList() {
	if (!actionList.empty()) {
		std::lock_guard<std::mutex> lock(actionMtx);
		PWebAction o;
		size_t i,n;
		n = actionList.size();
		for (i=0; i<n; i++) {
			o = actionList[i];
			util::freeAndNil(o);
		}
		actionList.clear();
	}
}


void TWebServer::clearWebRequestList() {
	if (!requestList.empty()) {
		std::lock_guard<std::mutex> lock(requestMtx);
		PWebRequest o;
		size_t i,n;
		n = requestList.size();
		for (i=0; i<n; i++) {
			o = requestList[i];
			util::freeAndNil(o);
		}
		requestList.clear();
	}
}


void TWebServer::clear() {
	util::freeAndNil(sockets);
	util::freeAndNil(defaultToken);
	util::freeAndNil(certFile);
	util::freeAndNil(keyFile);
	util::freeAndNil(dhFile);
	clearWebSessionMap();
	clearWebActionMap();
	clearWebActionList();
	clearWebRequestList();
}


void TWebServer::waitFor() {
	// Wait for rescan to be done
	EServerMode sm = getMode();
	while (sm == ESM_SCANNING) {
		util::saveWait(30);
		sm = getMode();
	}

	// Wait for all actions to be done
	writeInfoLog("[Wait for idle] Waiting for all web action threads terminated...");
	bool running;
	if (!actionList.empty()) {
		PWebAction o;
		size_t i,n;
		do {
			{ // Scope mutex threadMtx
				std::lock_guard<std::mutex> lock(actionMtx);
				running = false;
				n = actionList.size();
				for (i=0; i<n; i++) {

					o = actionList[i];
					if (util::assigned(o)) {
						if (o->running) {
							running = true;
							break;
						}
					}
				}
			} // End of scope mutex threadMtx
			if (running)
				util::saveWait(30);
		} while (running);
	}
	if (util::assigned(sockets)) {
		if (!sockets->isTerminated()) {
			sockets->waitFor();
		}
	}
	setMode(ESM_TERMINATED);
	writeInfoLog("[Wait for idle] All web action threads terminated.");
}

} /* namespace app */
