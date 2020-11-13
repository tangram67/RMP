/*
 * webserver.h
 *
 *  Created on: 03.02.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <vector>
#include <functional>
#include "stringconsts.h"
#include "webrequest.h"
#include "classes.h"
#include "inifile.h"
#include "fileutils.h"
#include "webtoken.h"
#include "weblink.h"
#include "webdirectory.h"
#include "websockets.h"
#include "webtypes.h"
#include "semaphore.h"
#include "exception.h"
#include "logger.h"
#include "detach.h"
#include "timer.h"
#include "credentialtypes.h"
#include "microhttpd/microhttpd.h"

namespace app {

class TWebServer;

#ifdef STL_HAS_TEMPLATE_ALIAS

using MHD_UriLogger = void* (*) (void *cls, const char *uri, struct MHD_Connection *con);

#else

typedef void* (*MHD_UriLogger) (void *cls, const char *uri, struct MHD_Connection *con);

#endif

enum EServerMode {
	ESM_CREATED,
	ESM_DISABLED,
	ESM_SCANNING,
	ESM_IDLE,
	ESM_RUNNING,
	ESM_PAUSED,
	ESM_STOPPED,
	ESM_TERMINATED
};


class TWebServer : public TObject {
private:
	int port;
	EServerMode mode;
	TWebConfig web;
	CWebData data;
	std::string digest;
	const char* httpsCert;
	const char* httpsKey;
	const char* httpsParam;
	util::PFile certFile;
	util::PFile keyFile;
	util::PFile dhFile;
	util::TFileList content;
	app::TWebTokenList tokenList;
	app::TWebLinkList rest;
	app::TWebDirectoryList vdl;
	app::PIniFile config;
	app::PLogFile infoLog;
	app::PLogFile exceptionLog;
	app::PThreadController threads;
	app::PTimerController timers;
	app::TWebSessionMap sessionMap;
	util::TVariantValues cookies;
	std::mutex actionMtx;
	std::mutex requestMtx;
	app::TMutex prepareMtx;
	app::TMutex updateMtx;
	app::TMutex configMtx;
	app::TMutex userMtx;
	mutable std::mutex cookieMtx;
	mutable std::mutex sessionMtx;
	mutable std::mutex modeMtx;
	app::TReadWriteLock actionLck;
	app::TReadWriteLock requestLck;
	app::TActionThread executer;
	app::TWebSockets* sockets;
	TWebActionMap actionMap;
	TWebActionList actionList;
	TWebRequestList requestList;
	PTimer socketTimer;
	PTimer sessionTimer;
	PTimer requestTimer;
	PTimer bufferTimer;
	PTimer statsTimer;
	PWebToken defaultToken;
	TCredentialCallback credentialCallbackMethod;
	TWebSocketDataHandlerList webSocketCientDataList;
	TWebSocketVariantHandlerList webSocketCientVariantList;
	TPrepareHandlerList prepareHandlerList;
	TFileUploadEventList uploadEventList;
	TStatisticsEventList statisticsEventList;
	util::TStringList allowedPagesList;
	std::string memory;
	mutable std::string rootURL;
	size_t maxUrlSize;
	size_t numa;

	struct MHD_Daemon* httpServer4;
	struct MHD_Daemon* httpServer6;

	struct MHD_Daemon* startServer(unsigned int flags);

	void readConfig();
	void writeConfig();
	void reWriteConfig();
	void readVirtualDirectoryConfig();
	void createVirtualDirectoryConfig(CWebDirectory& instance, const std::string& section);
	void clear();

	void onSessionTimer();
	void onRequestTimer();
	void onStatsTimer();
	void onBufferTimer();
	void onHeartbeatTimer();
	void updateStatusDisplay();
	void updateStatusToken();
	void raiseStatusEvent();
	void addWebToken();
	void updateWebToken();
	void updateTokenList(const TWebTokenValueMap& values);
	void readTokenList(TWebTokenValueMap& values);
	bool isRequestQueueIdle();
	void sessionCountLimiter();
	void sessionGarbageCollector(const bool cleanup = false);
	void requestGarbageCollector(const bool cleanup = false);
	void linkGarbageCollector();
	void bufferGarbageCollector();
	void deallocateHeapMemory(const size_t deleted);
	void clearWebActionMap();
	void clearWebActionList();
	void clearWebSessionMap();
	void clearWebRequestList();
	bool createWebSessionStore(const std::string path);
	int saveWebSessionsToFile(const std::string path);
	int loadWebSessionsFromFile(const std::string path);
	void getWebServerInfo();
	MHD_Result sendErrorResponse(struct MHD_Connection *connection, PWebRequest request, const char *url, const EHttpMethod methodType, const char *methodStr, int error);
	void executeAction(const TWebActionHandler& handler, const std::string& key, const std::string& value,
			const util::TVariantValues& params, const util::TVariantValues& session, const EWebActionMode mode, int& error);
	void actionAsyncExecuter(TWebAction& action);
	void actionSyncExecuter(const TWebActionHandler& handler, const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	int getRequestDelta() const;
	PWebAction findActionSlot();
	PWebRequest findRequestSlot(struct MHD_Connection *connection, bool& created);
	util::PFile findFileForURL(std::string& URL);
	util::PFile findFileInContent(const std::string& URL);
	PWebDirectory findDirectoryForURL(util::TFile& file, std::string& URL);
	void setSessionValue(PWebRequest request, const std::string& key, const std::string& value) const;
	std::string getSessionValue(PWebRequest request, const std::string& key) const;
	void setCookieValue(PWebRequest request, const std::string& key, const std::string& value) const;
	void setCookieValues(PWebRequest request, const util::TVariantValues& cookies) const;
	std::string getCookieValue(PWebRequest request, const std::string& key) const;
	std::string getConnectionValue(PWebRequest request, const std::string& key) const;
	void logConnectionValues(PWebRequest request, struct MHD_Connection *connection, bool reload = false) const;
	void scanWebRoot(const bool debug);
	void authenticate(util::PFile& file, std::string& URL, const char *url);
	void prepareRequest(PWebRequest request, PWebSession session, const std::string& URL, bool& prepared) const;
	bool prepareFileUpload(PWebRequest request, PWebSession session, const char *filename, const char *content_type, size_t size);
	MHD_Result execFileUpload(PWebSession session, const char *data, uint64_t off, size_t size);
	void terminateFileUpload(PWebRequest request, PWebSession session);
	int execDataUpload(PWebRequest request, PWebSession session, const std::string& URL, util::TBuffer data);
	MHD_Result execPostData(PWebRequest request, PWebSession session, const char *key, size_t len, const char *data, uint64_t off, size_t size);
	void startPostProcess(PWebSession session, const char *key, size_t len, const char *data, size_t size);
	void continuePostProcess(PWebSession session, const char *data, size_t size);
	void nextPostProcess(PWebSession session, const char *key, size_t len, const char *data, size_t size);
	void terminatePostProcess(PWebRequest request, PWebSession session, int& error);
	void dataPostProcess(PWebSession session);
	void execPostAction(PWebRequest request, PWebSession session, int& error);
	void onFileUploadEvent(PWebSession session, const std::string& fileName, const size_t& size);
	std::string rewriteFileName(const std::string& baseName, const std::string& fileName);
	std::string rewriteFileName(const util::TStringList& rewrite, const std::string& fileName);
	bool setTerminateMode();
	void updateApplicationValues(PWebSession session) const;
	std::string stripRestRoot(const std::string& url) const;
	void onSocketData(const app::THandle handle, const std::string& message);
	void onSocketVariant(const app::THandle handle, const util::TVariantValues& variants);
	void onSockeConnect(const app::THandle handle);
	MHD_Result requestDispatcher( struct MHD_Connection *connection,
						   const char *url,
						   const char *method,
						   const char *version,
						   const char *upload_data,
						   size_t *upload_data_size,
						   void **con_cls );

public:
	static std::string mhdVersion() {
		int major = (int)MHD_MAJOR;
		int minor = (int)MHD_MINOR;
		int patch = (int)MHD_PATCH;
		int release = (int)MHD_RELEASE;
		return util::cprintf("%02d%02d%02d%02d", major, minor, patch, release);
	}

	MHD_Result acceptHandler( const struct sockaddr *addr,
							  socklen_t addrlen );

	MHD_Result requestHandler(	struct MHD_Connection *connection,
						const char *url,
						const char *method,
						const char *version,
						const char *upload_data,
						size_t *upload_data_size,
						void **con_cls );

	void webSocketHandler(	void *cls,
							struct MHD_Connection *connection,
							void *con_cls,
							const char *extra_in,
							size_t extra_in_size,
							MHD_socket sock,
							struct MHD_UpgradeResponseHandle *urh);

	void logHandler( const char *uri,
					 struct MHD_Connection *connection );

	void customErrorLog ( const char *fmt, va_list va );

	void requestCompletedCallback (	struct MHD_Connection *connection,
									void **con_cls,
									enum MHD_RequestTerminationCode toe );

	void panicCallback ( const char *file,
						 unsigned int line,
						 const char *reason );

	MHD_Result postIteratorHandler (PWebRequest request,
							 enum MHD_ValueKind kind,
							 const char *key, size_t len,
							 const char *filename,
							 const char *content_type,
							 const char *transfer_encoding,
							 const char *data, uint64_t off, size_t size );

	template<typename cred_t, typename class_t>
		inline void setCredentialCallbackHandler(cred_t &&credentialCallbackMethod, class_t &&owner) {
			this->credentialCallbackMethod = std::bind(credentialCallbackMethod, owner,
						std::placeholders::_1, std::placeholders::_2);
		}

	template<typename member_t, typename class_t>
		inline void addWebPrepareHandler(member_t &&webPrepare, class_t &&owner) {
			TWebPrepareHandler handler = std::bind(webPrepare, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
			prepareHandlerList.push_back(handler);
		}

	template<typename member_t, typename class_t>
		inline void addFileUploadEventHandler(member_t &&webUpload, class_t &&owner) {
			TFileUploadEvent handler = std::bind(webUpload, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
			uploadEventList.push_back(handler);
		}

	template<typename member_t, typename class_t>
		inline void addStatisticsEventHandler(member_t &&webStatistics, class_t &&owner) {
			TStatisticsEvent handler = std::bind(webStatistics, owner, std::placeholders::_1, std::placeholders::_2);
			statisticsEventList.push_back(handler);
		}

	template<typename member_t, typename class_t>
		inline TWebActionHandler bindWebAction(member_t &&webAction, class_t &&owner) {
			TWebActionHandler method = std::bind(webAction, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
	    	return method;
		}

	template<typename member_t, typename class_t>
		inline void addWebAction(const std::string& url, member_t &&webAction, class_t &&owner, const EWebActionMode mode = WAM_ASYNC) {
			if (url.empty())
				throw util::app_error("TWebServer::addWebAction : Empty URL not allowed.");
			static_assert(std::is_reference<decltype(webAction)>::value, "TWebServer::addWebActionHandler : Argument <webAction> is not a reference.");
			static_assert(std::is_reference<decltype(owner)>::value, "TWebServer::addWebActionHandler : Argument <owner> is not a reference.");
			PWebActionItem o = new TWebActionItem;
			o->URL = url;
			o->mode = mode;
			o->tag = util::DEFAULT_STRING_TAG;
			o->action = bindWebAction(webAction, owner);
			app::TReadWriteGuard<app::TReadWriteLock> lock(actionLck, RWL_WRITE);
			actionMap.insert(TWebActionMapItem(url, o));
		}

	template<typename member_t, typename class_t>
		inline void addDefaultWebAction(member_t &&webAction, class_t &&owner, const EWebActionMode mode = WAM_ASYNC) {
			addWebAction(DEFAULT_WEB_ACTION, webAction, owner, mode);
		}

	template<typename request_t, typename class_t>
		inline void addWebLink(const std::string& url, request_t &&onDataRequest, class_t &&owner, bool zipped = false, bool cached = false) {
			std::string path = web.restRoot + url;
			PWebLink o = rest.verify(path);
			if (zipped && web.disableVfsGZip)
				zipped = false;
			if (util::assigned(o)) {
				// Update properties
				o->setZipped(zipped);
				o->setCached(cached);
				o->getDataHandler().bindDataRequestHandler(onDataRequest, owner);
			} else {
				// Add new entry
				rest.addLink(path, onDataRequest, owner, zipped, cached, web.vfsDataDeleteDelay);
			}
		}

	template<typename request_t, typename class_t>
		inline void addWebData(const std::string& url, request_t &&onDataReceive, class_t &&owner, const EWebActionMode mode = WAM_ASYNC) {
			std::string path = web.restRoot + url;
			PWebLink o = rest.verify(path);
			if (util::assigned(o)) {
				// Update properties
				o->setDeferred(mode == WAM_ASYNC);
				o->getDataHandler().bindDataReceivedHandler(onDataReceive, owner);
			} else {
				// Add new entry
				rest.addData(path, onDataReceive, owner, mode == WAM_ASYNC, web.vfsDataDeleteDelay);
			}
		}

	template<typename data_t, typename class_t>
		inline void addWebSocketDataHandler(data_t &&onSocketData, class_t &&owner) {
			TWebSocketDataHandler handler = std::bind(onSocketData, owner, std::placeholders::_1, std::placeholders::_2);
			webSocketCientDataList.push_back(handler);
		}

	template<typename data_t, typename class_t>
		inline void addWebSocketVariantHandler(data_t &&onSocketData, class_t &&owner) {
			TWebSocketVariantHandler handler = std::bind(onSocketData, owner, std::placeholders::_1, std::placeholders::_2);
			webSocketCientVariantList.push_back(handler);
		}

	PWebToken addWebToken(const std::string& key, const std::string& value);

	void setApplicationValue(const std::string& key, const std::string& value);
	void setApplicationValue(const char * key, const std::string& value);
	std::string getApplicationValue(const std::string& key) const;
	std::string getApplicationValue(const char * key) const;

	void addUrlAuthExclusion(const std::string& url);
	void addRestAuthExclusion(const std::string& api);

	void setMode(const EServerMode value);
	bool setAndCompareMode(const EServerMode value, const EServerMode compare);
	EServerMode getMode() const;
	bool isResponding() const;
	bool isRunning() const;

	bool start(const bool autostart = true);
	bool pause();
	bool resume();
	bool terminate();
	void waitFor();
	void update();

	bool isSecure() const { return web.useHttps; };
	unsigned int getPort() const { return port; };
	const std::string& getRestRoot() const { return web.restRoot; };
	const std::string& getWebRoot() const { return web.documentRoot; };
	const std::string& getUploadFolder() const { return web.uploadFolder; };
	const std::string& getFullURL() const;

	bool getDebug() const { return web.debug; };
	const std::string& getRealm() const { return web.realm; };
	const std::string& getCredentials() const { return web.credentials; };
	EHttpAuthType getDigestType() const { return web.auth; };

	bool hasWebSockets() const { return util::assigned(sockets); };
	ssize_t write(const app::THandle handle, const std::string& text) const;
	ssize_t write(const app::THandle handle, const util::TVariantValues& variants) const;
	ssize_t write(const app::THandle handle, void const * const data, size_t const size) const;
	ssize_t broadcast(const std::string& text) const;
	ssize_t broadcast(const util::TVariantValues& variants) const;
	ssize_t broadcast(void const * const data, size_t const size) const;

	void showUserCredentials(const std::string& preamble);
	void assignUserCredentials(const TCredentialMap& users);
	void addUserCredential(TCredential& user);
	bool logoffSessionUser(const std::string& sid);

	void getWebSessionInfoList(TWebSessionInfoList& list);
	void getWebRequestInfoList(TWebRequestInfoList& list, size_t& requests, size_t threshhold = 0);
	void getWebSessionValues(const std::string& sid, util::TVariantValues& values);

	size_t getWebRequestCount() const;
	size_t getWebSessionCount() const;
	size_t getWebApiCount() const;

	void getRunningConfiguration(TWebSettings& config);
	void setRunningConfiguration(const TWebSettings& config);

	void setMemoryStatus(const size_t current);
	void createSubresourceIntegrityFile(const std::string& folder) const;

	void writeLog(const std::string& s) const;
	void writeSessionLog(const TWebSession * session, const std::string& s, int verbosity = 0) const;
	void writeInfoLog(const std::string& s, int verbosity = 0) const;
	void writeErrorLog(const std::string& s) const;

	TWebServer(const std::string& name, const std::string& documentRoot, app::PIniFile config, app::PThreadController threads, app::PTimerController timers, PLogFile infoLog, PLogFile exceptionLog);
	virtual ~TWebServer();
};


} /* namespace app */

#endif /* WEBSERVER_H_ */
