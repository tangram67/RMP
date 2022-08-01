/*
 * upnp.cpp
 *
 *  Created on: 21.01.2019
 *      Author: dirk
 */

#include <stdlib.h>
#include "../inc/globals.h"
#include "../inc/variant.h"
#include "../inc/compare.h"
#include "../inc/sysutils.h"
#include "../inc/stringutils.h"
#include "../inc/ansi.h"
#include "upnpconsts.h"
#include "upnp.h"


namespace upnp {


std::string typeToStr(const EUPnPMessageType type) {
	switch (type) {
		case UPNP_TYPE_ALIVE:
			return "Livesign";
			break;
		case UPNP_TYPE_BYEBYE:
			return "Byebye";
			break;
		case UPNP_TYPE_REQUEST:
			return "Request";
			break;
		case UPNP_TYPE_RESPONSE:
			return "Response";
			break;
		case UPNP_TYPE_DISCOVERY:
			return "Discovery";
			break;
		default:
			return "Unknown";
			break;
	}
	return "Unknown";
}

std::string reqToStr(const EUPnPRequestType type) {
	switch (type) {
		case UPNP_AD_DEVICE:
			return "Device";
			break;
		case UPNP_AD_SERVICE:
			return "Service";
			break;
		case UPNP_AD_ROOT:
			return "Root";
			break;
		case UPNP_AD_UUID:
			return "UUID";
			break;
		case UPNP_AD_ALL:
			return "All";
			break;
		default:
			return "Unknown";
			break;
	}
	return "Unknown";
}


TMessageQueue::TMessageQueue() {
}

TMessageQueue::~TMessageQueue() {
	util::clearObjectList(messages);
}

void TMessageQueue::add(PUPnPMessage message) {
	if (util::assigned(message)) {
		app::TLockGuard<app::TMutex> lock(listMtx);
		messages.push_back(message);
	}
}

void TMessageQueue::insert(PUPnPMessage message) {
	if (util::assigned(message)) {
		app::TLockGuard<app::TMutex> lock(listMtx);
		messages.insert(messages.begin(), message);
	}
}

PUPnPMessage TMessageQueue::first() {
	app::TLockGuard<app::TMutex> lock(listMtx);
	if (!messages.empty()) {
		for (size_t i=0; i<messages.size(); ++i) {
			PUPnPMessage o = messages[i];
			if (util::assigned(o)) {
				if (o->valid)
					return o;
			}
		}
	}
	return nil;
}

size_t TMessageQueue::size() {
	app::TLockGuard<app::TMutex> lock(listMtx);
	return messages.size();
}

bool TMessageQueue::empty() {
	app::TLockGuard<app::TMutex> lock(listMtx);
	return messages.empty();
}

void TMessageQueue::remove(PUPnPMessage message) {
	if (util::assigned(message)) {
		app::TLockGuard<app::TMutex> lock(listMtx);
		message->valid = false;
		cleanup();
	}
}

struct CMessageEraser
{
	CMessageEraser() {}
    bool operator()(PUPnPMessage o) const {
    	if (util::assigned(o)) {
			if (!o->valid) {
				util::freeAndNil(o);
				return true;
			}
    	}
    	return false;
    }
};

void TMessageQueue::cleanup() {
	messages.erase(std::remove_if(messages.begin(), messages.end(), CMessageEraser()), messages.end());
}



TUPnP::TUPnP() {
	deviceURL = "DeviceDescription.xml";
	device = "MediaServer";
	mcast4 = nil;
	toBroadcast = nil;
	thread = nil;
	avahi = nil;
	timeout = 0;
	bonjour = false;
	discovery = false;
	enabled = true;
	running = false;
	active = false;
	debug = false;
	verbose = false;
}

TUPnP::~TUPnP() {
}


int TUPnP::prepare() {
	// Read configuration file
	openConfig(application.getConfigFolder());
	reWriteConfig();

	// Store current webserver root for further use
	serverid = getServerVersion();
	webroot = getWebRoot();

	// Check if verbose messages to stdout possible
	if (debug) {
		if (!application.isDaemonized()) {
			verbose = true;
		}
	}

	return EXIT_SUCCESS;
}

int TUPnP::execute() {
	// Initialize advertising timer for message broadcast
	toBroadcast = application.addTimeout("UPnPBroadcastTimeout", BROADCAST_DELAY, false);
	toBroadcast->bindEventHandler(&upnp::TUPnP::onBroadcast, this);
	timeout = toBroadcast->getTimeout();

	// Add UPnP discovery response
	if (application.hasWebServer()) {
		application.addWebLink(deviceURL, &upnp::TUPnP::getDeviceDescription, this, false);

		// Catch all UPnP requests
		application.addWebLink("upnp/", &upnp::TUPnP::getUPnPResponse, this, false);

		// Catch event and control requests
		application.addWebLink("upnp/event/", &upnp::TUPnP::getUPnPResponse, this, false);
		application.addWebLink("upnp/control/", &upnp::TUPnP::getUPnPResponse, this, false);

		// Catch content and connection requests
		application.addWebLink("upnp/event/ContentDirectory", &upnp::TUPnP::getUPnPResponse, this, false);
		application.addWebLink("upnp/event/ConnectionManager", &upnp::TUPnP::getUPnPResponse, this, false);
		application.addWebLink("upnp/control/ContentDirectory", &upnp::TUPnP::getUPnPResponse, this, false);
		application.addWebLink("upnp/control/ConnectionManager", &upnp::TUPnP::getUPnPResponse, this, false);
		application.addWebLink("upnp/control/X_MS_MediaReceiverRegistrar", &upnp::TUPnP::getUPnPResponse, this, false);

		// Catch posted XML data
		application.addWebData("upnp/event/ContentDirectory", &upnp::TUPnP::setUPnPResponse, this, app::WAM_SYNC);
		application.addWebData("upnp/event/ConnectionManager", &upnp::TUPnP::setUPnPResponse, this, app::WAM_SYNC);
		application.addWebData("upnp/control/ContentDirectory", &upnp::TUPnP::setUPnPResponse, this, app::WAM_SYNC);
		application.addWebData("upnp/control/ConnectionManager", &upnp::TUPnP::setUPnPResponse, this, app::WAM_SYNC);
		application.addWebData("upnp/control/X_MS_MediaReceiverRegistrar", &upnp::TUPnP::setUPnPResponse, this, app::WAM_SYNC);
		
		// Allow access to UPnP API for everyone
		application.addRestAuthExclusion(deviceURL);
		application.addRestAuthExclusion("upnp/");
		application.addUrlAuthExclusion("/upnp/");
	}

	// Open multicats sockets for UPnP traffic
	if (application.useSockets() && enabled) {
		bool goon = true;

		// Create brodcast worker thread
		thread = application.addThread<TMessageQueue>("UPnP-Brodcast", messages,
													   &upnp::TUPnP::broadcastThreadHandler,
													   this, app::THD_START_ON_DEMAND, app::nsizet);

		// Open Multicast socket for SSDP (UPnP) test
		mcast4 = application.addSocket<inet::TMulticastSocket>("UPnP socket IPv4", &upnp::TUPnP::onMulticastData, this);
		mcast4->open("239.255.255.250", 1900);
		if (mcast4->isOpen()) {
			logger("Device discovery started on \"" + mcast4->getHost() + ":" + mcast4->getService() + "\"");
		} else {
			logger("Device discovery failed for IPv4 multicast.");
			goon = false;
		}
		if (goon) {
			// UPnP handling is possible...
			running = true;

			// Start broadcast thread and timer
			if (mcast4->isOpen()) {
				if (util::assigned(thread)) {
					if (!thread->isStarted()) {
						thread->execute();
					}
					toBroadcast->restart(5500);
				}
			}
		}

	}

	// Enable Avahi Zerconf/Bonjour support
	if (bonjour && application.hasWebServer()) {
		avahi = new avahi::TAvahiClient(application.getThreads(), application.getApplicationLogger());
		if (util::assigned(avahi)) {
			std::string service = application.getWebServer().isSecure() ? "_https._tcp" : "_http._tcp";
			avahi->start();
			avahi->addGroupEntry(friendlyName, service, "index.html", application.getWebServer().getPort());
		}
	}

	// Leave after initialization
	return EXIT_SUCCESS;
}

void TUPnP::unprepare() {
	running = false;
	toBroadcast->stop();
	broadcastExitMessage();
	if (util::assigned(avahi)) {
		avahi->terminate();
	}
}


void TUPnP::cleanup() {
	// UPnP brodcast terminated, say "Bye bye"...
	while (active) util::wait(SEND_DELAY / 2);
	if (util::assigned(avahi)) {
		avahi->waitFor();
	}
}


void TUPnP::openConfig(const std::string& configPath) {
	std::string path = util::validPath(configPath);
	std::string file = path + "upnp.conf";
	config.open(file);
	reWriteConfig();
}

void TUPnP::readConfig() {
	config.setSection("UPnP");
	uuid = config.readString("GUID", util::fastCreateUUID(true, true));
	// description = config.readString("Description", application.getDescription());
	// friendlyName = config.readString("FriendlyName", description + " on " + util::capitalize(application.getHostName()) + " [" + application.getSerialKey() + "]");
	description = application.getDescription();
	friendlyName = description + " @ " + util::capitalize(application.getHostName());
	enabled = config.readBool("Enabled", enabled);
	discovery = config.readBool("Discovery", discovery);
	debug = config.readBool("Debug", debug);

	config.setSection("Zeroconf");
	bonjour = config.readBool("Enabled", bonjour || enabled);
}

void TUPnP::writeConfig() {
	config.setSection("UPnP");
	config.writeString("GUID", uuid);
	config.writeString("Description", description);
	config.writeString("FriendlyName", friendlyName);
	config.writeBool("Enabled", enabled, app::INI_BLYES);
	config.writeBool("Discovery", discovery, app::INI_BLYES);
	config.writeBool("Debug", debug, app::INI_BLYES);

	config.setSection("Zeroconf");
	config.writeBool("Enabled", bonjour, app::INI_BLYES);

	// Save changes to disk
	config.flush();
}

void TUPnP::reWriteConfig() {
	readConfig();
	writeConfig();
}


void TUPnP::addServiceEntry(const std::string& type, const std::string& page, int port) {
	if (util::assigned(avahi)) {
		avahi->addGroupEntry(friendlyName, type, page, port);
	}
}


bool TUPnP::isOnline() {
	if (util::assigned(mcast4)) {
		if (mcast4->isOpen()) {
			return true;
		}
	}
	return false;
}

void TUPnP::onBroadcast() {
	broadcastUPnPDevice();
	if (discovery)
		broadcastSSDPDiscovery();
	toBroadcast->restart(timeout);
}

int TUPnP::broadcastThreadHandler(TBroadcastThread& sender, TMessageQueue& messages) {
	sender.writeLog("Thread started.");
	active = true;
	util::TBooleanGuard<bool> bg(active);
	util::TStringList body;
	bool exit = false;
	do {
		event.wait();
		if (debug) logger(util::csnprintf("Broadcast % UPnP messages.", messages.size()));
		bool r = true;
		bool broadcast = true;
		PUPnPMessage o = messages.first();
		while (util::assigned(o) && r) {
			if (o->sender.assigned) {
				r = sendUPnPResponse(o->sender, o->query, o->service, o->type, o->response, body);
				broadcast = false;
			} else {
				r = sendUPnPMessage(o->query, o->service, o->type, o->response, body);
			}
			if (r) {
				if (debug) {
					if (broadcast) {
						logger("Broadcast UPnP message \"" + typeToStr(o->type) + "/" + reqToStr(o->response) + "\"");
					} else {
						const std::string& addr = inet::inetAddrToStr(o->sender);
						logger("Send UPnP message \"" + typeToStr(o->type) + "/" + reqToStr(o->response) + "\" to [" + addr + "]");
					}
				}
				messages.remove(o);
				util::wait(SEND_DELAY);
			} else {
				if (broadcast) {
					logger("Broadcast UPnP message failed for \"" + typeToStr(o->type) + "/" + reqToStr(o->response) + "\"");
				} else {
					const std::string& addr = inet::inetAddrToStr(o->sender);
					logger("Send UPnP message failed for \"" + typeToStr(o->type) + "/" + reqToStr(o->response) + "\" to [" + addr + "]");
				}
				if (!running) {
					exit = true;
				}
			}
			o = messages.first();
		}
		if (!exit) {
			if (messages.empty()) {
				exit = !running || sender.isTerminating();
			}
		}
	} while (!exit);

	sender.writeLog("Thread terminated.");
	return EXIT_SUCCESS;
}

void TUPnP::notifyEvent(const std::string message) {
	app::TLockGuard<app::TMutex> lock(eventMtx, false);
	if (!lock.tryLock())
		return;
	TEventResult r = event.notify();
	if (r != EV_SUCCESS)
		logger(message + " Notify failed.");
}

void TUPnP::logger(const std::string& text) const {
	application.writeLog("[UPnP] " + text);
}

ssize_t TUPnP::onMulticastData(const inet::TUDPSocket& socket) {
	ssize_t len = 0;
	size_t size = 1500;
	util::TBuffer buffer(size);

	if (verbose) {
		util::TDateTime date;
		std::cout << std::endl;
		std::cout << date << " : TUPnP::onMulticastData() [" << util::quote(socket.getName()) << "/" + socket.getService() << "] Data received from ";
	}
	do {
		std::string sender;
		util::TVariantValues message;
		inet::TAddressInfo from;
		len = socket.receiveFrom(buffer(), buffer.size(), from);
		if (len > 0) {
			sender = inet::inetAddrToStr(from);
			int port = inetPortToInt(from);
			message.parseCSV(buffer(), len, ':');
			message.add("Sender", sender);
			if (verbose) {
				std::cout << "TUPnP::onMulticastData() Received [" << sender << ":" << port << "] (" << len << " bytes) :" << std::endl;
				std::cout << app::yellow;
				debugUPnPMessage(buffer(), len);
				std::cout << app::reset;
			}
			processUPnPMessage(message, from);
		}
	} while ((len > 0) && (len >= (ssize_t)buffer.size()));

	return (ssize_t)0;
}


std::string TUPnP::getWebRoot() {
	// Find current webroot, IPV4 only for now...
	std::string webroot = "http://localhost:8099/";
	if (application.hasWebServer()) {
		bool ok = false;
		int port = 80;
		std::string address;

		// Use local IPv4 address for UPnP root URL
		util::TStringList addresses;
		if (sysutil::getLocalIpAddresses(addresses, sysutil::EAT_IPV4)) {
			address = addresses[0];
			port = application.getWebServer().getPort();
			ok = true;
		}
		if (ok) {
			std::string proto = application.getWebServer().isSecure() ? "https" : "http";
			int wellknown = application.getWebServer().isSecure() ? 443 : 80;
			if (inet::isIPv6Address(address))
				address = "[" + address + "]";
			if (port != wellknown)
				webroot = util::csnprintf("%://%:%/", proto, address, port);
			else
				webroot = util::csnprintf("%://%/", proto, address);
		}
	}
	return webroot;
}

std::string TUPnP::getSystemInfo() {
	std::string system = "Unknown system";
	if (sysutil::isLinux()) {
		sysutil::TSysInfo info;
		if (sysutil::uname(info)) {
			// Output: e.g. Linux 3.13.0-85-generic x86_64>
			system = info.sysname + "/" + info.release;
		}
	}
	return system;
}

std::string TUPnP::getUPnPVersion() {
	return "UPnP/1.0";
}

std::string TUPnP::getApplicationVersion() {
	return "TWebServer/" + application.getVersion();
}

std::string TUPnP::getWebServerBuild() {
	return "MHD/" + app::TWebServer::mhdVersion();
}

std::string TUPnP::getServerVersion() {
	return getSystemInfo() + " " + getUPnPVersion() + " " + getWebServerBuild() + " " + getApplicationVersion();
}



EUPnPMessageType TUPnP::getMessageType(const std::string& request) {
	EUPnPMessageType type = UPNP_TYPE_UNKNOWN;
	if (!request.empty()) {
		// Check for SSDP discovery
		bool found = false;
		if (!found && 0 == util::strncasecmp(request, "M-SEARCH", 8)) {
			type = UPNP_TYPE_DISCOVERY;
			found = true;
		}
		if (!found && 0 == util::strncasecmp(request, "NOTIFY", 6)) {
			type = UPNP_TYPE_ALIVE;
			found = true;
		}
		if (!found && 0 == util::strncasecmp(request, "HTTP", 4)) {
			if (request.size() > 5) {
				if (util::strcasestr(request.c_str() + 4, "200")) {
					type = UPNP_TYPE_RESPONSE;
					found = true;
				}
			}
		}
	}
	return type;
}

bool TUPnP::getRequestType(const std::string& query, EUPnPServiceType& service, EUPnPRequestType& type) {
	type = UPNP_AD_UNKNOWN;
	service = UPNP_SRV_UNKNOWN;
	bool found = false;

	// Get response type
	if (!found && 0 == util::strcasecmp(query, "ssdp:all")) {
		type = UPNP_AD_ALL;
		found = true;
	}
	if (!found && 0 == util::strcasecmp(query, "upnp:rootdevice")) {
		type = UPNP_AD_ROOT;
		found = true;
	}
	if (!found && 0 == util::strncasecmp(query, "uuid:", 5)) {
		type = UPNP_AD_UUID;
		found = true;
	}
	if (!found && util::strcasestr(query, device)) {
		type = UPNP_AD_DEVICE;
		found = true;
	}
	if (!found) {
		const TUPnPService * it = services.getServices();
		while (it->valid) {
			const TUPnPService& srv = *it;
			if (!found && util::strcasestr(query, srv.name)) {
				type = UPNP_AD_SERVICE;
				service = srv.type;
				found = true;
				break;
			}
			++it;
		}
	}
	return found;
}


bool TUPnP::isSelf(const std::string serial) {
	if (serial.size() > (36 + 4)) {
		if (0 == util::strncasecmp(serial, "uuid:", 5)) {
			std::string guid = serial.substr(5, 36);
			if (util::isValidUUID(guid)) {
				if (0 == util::strncasecmp(guid, uuid, uuid.size())) {
					return true;
				}
			}
		}
	}
	return false;
}

bool TUPnP::isExtension(const std::string service) {
	return 0 == util::strncasecmp(service, "X_MS_", 5);
}

std::string TUPnP::getDefaultSchema(const std::string service) {
	return isExtension(service) ? "urn:microsoft.com" : "urn:schemas-upnp-org";
}


void TUPnP::processUPnPMessage(const util::TVariantValues& message, const inet::TAddressInfo sender) {

	// Check for SSDP discovery
	const std::string& request = message.variant(0).name();
	EUPnPMessageType type = getMessageType(request);

	// Check for loopback
	const std::string& serial = message["USN"].asString();
	if (isSelf(serial)) {
		return;
	}

	// Check if response needed
	if (type != UPNP_TYPE_UNKNOWN) {
		switch (type) {
			case UPNP_TYPE_DISCOVERY:
				if (debug) {
					const std::string& addr = inet::inetAddrToStr(sender);
					const int port = inet::inetPortToInt(sender);
					logger("TUPnP::processUPnPMessage() Discovery received from [" + addr + ":" + std::to_string(port) + "]");
				}
				sendUPnPResponse(message, sender);
				break;

			case UPNP_TYPE_RESPONSE:
			case UPNP_TYPE_ALIVE:
			default:
				if (debug) {
					const std::string& addr = inet::inetAddrToStr(sender);
					const int port = inet::inetPortToInt(sender);
					logger("TUPnP::processUPnPMessage() Message \"" + typeToStr(type) + "\" received from [" + addr + ":" + std::to_string(port) + "]");
				}
				break;
		}
	} else {
		const std::string& addr = inet::inetAddrToStr(sender);
		const int port = inet::inetPortToInt(sender);
		logger("TUPnP::processUPnPMessage() Unknown message \"" + request + "\" received from [" + addr + ":" + std::to_string(port) + "]");
	}
}


void TUPnP::sendUPnPResponse(const util::TVariantValues& message, const inet::TAddressInfo sender) {
	if (isOnline() && !message.empty()) {
		EUPnPRequestType type;
		EUPnPServiceType service;

		// Check for SSDP discovery
		const std::string& ssdp = message["MAN"].asString();
		if (0 != util::strncasecmp(ssdp, "\"ssdp:discover\"", 15)) {
			if (verbose)
				std::cout << "TUPnP::sendUPnPResponse() Request \"" << ssdp << "\" is no SSDP discovery." << std::endl;
			return;
		}

		// Get SSDP query string from message
		const std::string& query = message["ST"].asString();
		if (query.empty()) {
			if (verbose)
				std::cout << "TUPnP::sendUPnPResponse() No query specified." << std::endl;
			return;
		}

		// Get response type
		if (!getRequestType(query, service, type)) {
			if (verbose)
				std::cout << "TUPnP::sendUPnPResponse() No query type found for \"" << query << "\"" << std::endl;
			return;
		}
		if (type == UPNP_AD_UNKNOWN) {
			if (verbose)
				std::cout << "TUPnP::sendUPnPResponse() Invalid type found for \"" << query << "\"" << std::endl;
			return;
		}

		// Queue response messages
		app::TLockGuard<app::TMutex> lock(sendMtx);
		util::TStringList body;
		if (type == UPNP_AD_ALL) {
			// Send all information
			const TUPnPService * it = services.getServices();
			while (it->valid) {
				const TUPnPService& srv = *it;
				queueUPnPResponse(sender, query, srv.type, UPNP_TYPE_RESPONSE, UPNP_AD_SERVICE, body);
				++it;
			}
			queueUPnPResponse(sender, query, UPNP_SRV_UNKNOWN, UPNP_TYPE_RESPONSE, UPNP_AD_DEVICE, body);
			queueUPnPResponse(sender, query, UPNP_SRV_UNKNOWN, UPNP_TYPE_RESPONSE, UPNP_AD_UUID, body);
			queueUPnPResponse(sender, query, UPNP_SRV_UNKNOWN, UPNP_TYPE_RESPONSE, UPNP_AD_ROOT, body);
		} else {
			if (type == UPNP_AD_SERVICE) {
				if (service == UPNP_SRV_UNKNOWN) {
					const TUPnPService * it = services.getServices();
					while (it->valid) {
						const TUPnPService& srv = *it;
						queueUPnPResponse(sender, query, srv.type, UPNP_TYPE_RESPONSE, type, body);
						++it;
					}
				} else {
					queueUPnPResponse(sender, query, service, UPNP_TYPE_RESPONSE, type, body);
				}
			} else {
				queueUPnPResponse(sender, query, service, UPNP_TYPE_RESPONSE, type, body);
			}
		}
		if (verbose) {
			const std::string& request = message.variant(0).name();
			std::cout << app::green << "TUPnP::sendUPnPResponse() Queued response [" << reqToStr(type) << "] for request \"" << request << "\" and query \"" << query << "\"" << app::reset << std::endl;
		}
		notifyEvent("TUPnP::sendUPnPResponse()");
	}
}


void TUPnP::queueUPnPDevice(const EUPnPMessageType type, const util::TStringList& body) {
	queueUPnPMessage("", UPNP_SRV_UNKNOWN, type, UPNP_AD_UUID, body);
	queueUPnPMessage("", UPNP_SRV_UNKNOWN, type, UPNP_AD_ROOT, body);
	queueUPnPMessage("", UPNP_SRV_UNKNOWN, type, UPNP_AD_DEVICE, body);
}

void TUPnP::queueUPnPService(const EUPnPMessageType type, const util::TStringList& body) {
	const TUPnPService * it = services.getServices();
	while (it->valid) {
		const TUPnPService& srv = *it;
		queueUPnPMessage("", srv.type, type, UPNP_AD_SERVICE, body);
		++it;
	}
}

void TUPnP::broadcastUPnPDevice() {
	if (isOnline()) {
		app::TLockGuard<app::TMutex> lock(sendMtx);
		util::TStringList body;
		queueUPnPDevice(UPNP_TYPE_ALIVE, body);
		queueUPnPService(UPNP_TYPE_ALIVE, body);
		notifyEvent("TUPnP::broadcastUPnPDevice()");
	}
}

void TUPnP::broadcastUPnPServices() {
	if (isOnline()) {
		app::TLockGuard<app::TMutex> lock(sendMtx);
		util::TStringList body;
		queueUPnPService(UPNP_TYPE_ALIVE, body);
		notifyEvent("TUPnP::broadcastUPnPServices()");
	}
}


void TUPnP::broadcastSSDPDiscovery() {
	if (isOnline()) {
		app::TLockGuard<app::TMutex> lock(sendMtx);
		util::TStringList body;
		queueUPnPMessage("", UPNP_SRV_UNKNOWN, UPNP_TYPE_DISCOVERY, UPNP_AD_ROOT, body);
		notifyEvent("TUPnP::broadcastSSDPDiscovery()");
	}
}

void TUPnP::broadcastExitMessage() {
	if (isOnline()) {
		app::TLockGuard<app::TMutex> lock(sendMtx);
		active = true;
		util::TStringList body;
		queueUPnPDevice(UPNP_TYPE_BYEBYE, body);
		queueUPnPService(UPNP_TYPE_BYEBYE, body);
		notifyEvent("TUPnP::broadcastExitMessage()");
		logger("Device discovery shutdown.");
	}
}


void TUPnP::buildUPnPMessage(const std::string& query, const EUPnPServiceType service,
		const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body, util::TStringList& message) {
	message.clear();

	// Add resopnse header
	switch (type) {
		case UPNP_TYPE_DISCOVERY:
			message.add("M-SEARCH * HTTP/1.1");
			break;
		case UPNP_TYPE_RESPONSE:
			message.add("HTTP/1.1 200 OK");
			break;
		case UPNP_TYPE_BYEBYE:
		case UPNP_TYPE_ALIVE:
		default:
			message.add("NOTIFY * HTTP/1.1");
			break;
	}

	// Add host address
	if (type != UPNP_TYPE_RESPONSE) {
		message.add("HOST: 239.255.255.250:1900");
	}

	// Set cache control
	if (type != UPNP_TYPE_BYEBYE) {
		message.add("CACHE-CONTROL: max-age=" + std::to_string(MESSAGE_EXPIRE_TIME));
	}

	// Set extended response information
	if (type == UPNP_TYPE_RESPONSE) {
		const std::string& time = util::RFC1123DateTimeToStr(util::now());
		message.add("DATE: " + time);
		message.add("EXT:");
	}

	// Add device description URL
	if (type != UPNP_TYPE_DISCOVERY && type != UPNP_TYPE_BYEBYE) {
		message.add("LOCATION: " + webroot + "rest/" + deviceURL);
	}

	// Get service properties
	const TUPnPService& srv = services[service];

	// Set defaults
	std::string st = query.empty() ? "ssdp:alive" : query;
	std::string nt = srv.valid ? srv.name : "ConnectionManager";
	std::string rt = srv.valid ? srv.schema : getDefaultSchema(nt);

	// Set NT header
	if (type != UPNP_TYPE_DISCOVERY && type != UPNP_TYPE_RESPONSE) {

		// Set response headers
		switch (response) {
			case UPNP_AD_UUID:
				message.add("NT: uuid:" + uuid);
				break;
			case UPNP_AD_DEVICE:
				message.add("NT: urn:schemas-upnp-org:device:" + device + ":1");
				break;
			case UPNP_AD_SERVICE:
				message.add("NT: " + rt + ":service:" + nt + ":1");
				break;
			case UPNP_AD_ROOT:
				message.add("NT: upnp:rootdevice");
				break;
			default:
				break;
		}
	}

    // Set message specific headers
	switch (type) {
		case UPNP_TYPE_DISCOVERY:
			message.add("MAN: \"ssdp:discover\"");
			message.add("ST: upnp:rootdevice");
			message.add("MX: 2");
			message.add("User-Agent: " + serverid);
			break;
		case UPNP_TYPE_RESPONSE:
			// Respond with the original ST from request
			message.add("ST: " + st);
			break;
		case UPNP_TYPE_BYEBYE:
			message.add("NTS: ssdp:byebye");
			break;
		case UPNP_TYPE_ALIVE:
			message.add("NTS: ssdp:alive");
			break;
		default:
			break;
	}

	// Add server description
	if (type != UPNP_TYPE_DISCOVERY && type != UPNP_TYPE_BYEBYE) {
		message.add("SERVER: " + serverid);
	}

	// Set USN header information
	if (type != UPNP_TYPE_DISCOVERY) {

		// Set response headers
		switch (response) {
			case UPNP_AD_UUID:
				message.add("USN: uuid:" + uuid);
				break;
			case UPNP_AD_DEVICE:
				message.add("USN: uuid:" + uuid + "::urn:schemas-upnp-org:device:" + device + ":1");
				break;
			case UPNP_AD_SERVICE:
				message.add("USN: uuid:" + uuid + "::" + rt + ":service:" + nt + ":1");
				break;
			case UPNP_AD_ROOT:
				message.add("USN: uuid:" + uuid + "::upnp:rootdevice");
				break;
			default:
				break;
		}
	}

	// Add body content data
	if (!body.empty() && type != UPNP_TYPE_RESPONSE) {
    	message.add("Content-Length: " + std::to_string(body.size()));
		message.add(body);
    } else {
    	if (type != UPNP_TYPE_ALIVE && type != UPNP_TYPE_BYEBYE) {
    		message.add("Content-Length: 0");
    	}
    }

	// Add blank line
	message.add("\r\n");
}


void TUPnP::queueUPnPResponse(const inet::TAddressInfo& sender, const std::string& query, const EUPnPServiceType service,
		const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body) {
	PUPnPMessage o = new TUPnPMessage;
	o->type = type;
	o->response = response;
	o->query = query;
	o->service = service;
	o->sender = sender;
	o->valid = true;
	insertUPnPMessage(o);
}

void TUPnP::queueUPnPMessage(const std::string& query, const EUPnPServiceType service,
		const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body) {
	PUPnPMessage o = new TUPnPMessage;
	o->type = type;
	o->response = response;
	o->query = query;
	o->service = service;
	o->sender.clear();
	o->valid = true;
	addUPnPMessage(o);
}

void TUPnP::insertUPnPMessage(const std::string& query, const EUPnPServiceType service,
		const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body) {
	PUPnPMessage o = new TUPnPMessage;
	o->type = type;
	o->response = response;
	o->query = query;
	o->service = service;
	o->sender.clear();
	o->valid = true;
	insertUPnPMessage(o);
}

bool TUPnP::sendUPnPResponse(const inet::TAddressInfo& sender, const std::string& query, const EUPnPServiceType service,
		const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body) {
	util::TStringList message;
	buildUPnPMessage(query, service, type, response, body, message);
	if (!message.empty()) {
		std::string data = message.raw('\r');
		ssize_t r = mcast4->sendTo(data.c_str(), data.size(), sender);
		if (r == (ssize_t)data.size()) {
			if (verbose) {
				std::cout << "TUPnP::sendUPnPResponse() Size = " << data.size() << " bytes : " << std::endl;
				std::cout << app::cyan; debugUPnPMessage(data.c_str(), data.size()); std::cout << app::reset;
			}
			return true;
		}
	}
	return false;
}

bool TUPnP::sendUPnPMessage(const std::string& query, const EUPnPServiceType service,
		const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body) {
	util::TStringList message;
	buildUPnPMessage(query, service, type, response, body, message);
	if (!message.empty()) {
		std::string data = message.raw('\r');
		ssize_t r = mcast4->send(data.c_str(), data.size());
		if (r == (ssize_t)data.size()) {
			if (verbose) {
				std::cout << "TUPnP::sendUPnPMessage() Size = " << data.size() << " bytes : " << std::endl;
				std::cout << app::cyan;	debugUPnPMessage(data.c_str(), data.size()); std::cout << app::reset;
			}
			return true;
		}
	}
	return false;
}


void TUPnP::addUPnPMessage(PUPnPMessage message) {
	if (util::assigned(message)) {
		if (active || message->type == UPNP_TYPE_BYEBYE) {
			messages.add(message);
		}
	}
}

void TUPnP::insertUPnPMessage(PUPnPMessage message) {
	if (util::assigned(message)) {
		if (active || message->type == UPNP_TYPE_BYEBYE) {
			messages.insert(message);
		}
	}
}


void TUPnP::debugUPnPMessage(void const * const data, size_t const size) {
	util::TStringList lines;
	if (lines.readBinary(data, size) > 0) {
		lines.debugOutput();
		std::cout << std::endl;
	}
}


void TUPnP::getDeviceDescription(app::TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	bool writeFile = params["write"].asBoolean();

	// Send result data
	if (!xmlResponseData.empty()) {
		data = xmlResponseData.c_str();
		size = xmlResponseData.size();
		return;
	}

	util::TStringList xml;
	xml.add("<?xml version=\"1.0\"?>");
	xml.add("<root xmlns=\"urn:schemas-upnp-org:device-1-0\">");
	xml.add("  <specVersion>");
	xml.add("    <major>1</major>");
	xml.add("    <minor>0</minor>");
	xml.add("  </specVersion>");
	xml.add("  <URLBase>" + webroot + "rest/" + deviceURL + "</URLBase>");
	xml.add("  <device>");
	xml.add("    <deviceType>urn:schemas-upnp-org:device:" + device + ":1</deviceType>");
	xml.add("    <friendlyName>" + friendlyName + "</friendlyName>");
	xml.add("    <manufacturer>db Applications</manufacturer>");
	xml.add("    <manufacturerURL>http://www.dbrinkmeier.com/</manufacturerURL>");
	xml.add("    <modelDescription>" + description + " (" + application.getVersion() + ")</modelDescription>");
	xml.add("    <modelName>" + description + "</modelName>");
	xml.add("    <modelNumber>" + application.getVersion() + "</modelNumber>");
	xml.add("    <modelURL>" + webroot + "</modelURL>");
	xml.add("    <serialNumber>" + application.getSerialKey() + "-" + util::cprintf("%05d", application.getLicenseNumber()) + "</serialNumber>");
	xml.add("    <UDN>uuid:" + uuid + "</UDN>");
	xml.add("    <UPC>" + application.getSerialKey() + "</UPC>");

	// Add service URLs
	xml.add("    <serviceList>");
	const TUPnPService * service = services.getServices();
	while (service->valid) {
		const TUPnPService& srv = *service;
		xml.add("      <service>");
		xml.add("        <serviceType>" + srv.schema + ":service:" + srv.name + ":" + std::to_string(srv.version) + "</serviceType>");
		xml.add("        <serviceId>" + srv.schema + ":serviceId:" + srv.name + "</serviceId>");
		xml.add("        <controlURL>" + srv.controlURL + "</controlURL>");
		xml.add("        <eventSubURL>" + srv.eventSubURL + "</eventSubURL>");
		xml.add("        <SCPDURL>" + srv.scpdURL + "</SCPDURL>");
		xml.add("      </service>");
		++service;
	}
	xml.add("    </serviceList>");

	// Add JPEG and PNG icons
	xml.add("    <iconList>");
	const TUPnPLogo * logo = services.getLogos();
	while (logo->valid) {
		xml.add("      <icon>");
		xml.add("        <mimetype>image/png</mimetype>");
		xml.add("        <width>" + std::to_string(logo->width)  + "</width>");
		xml.add("        <height>" + std::to_string(logo->height)  + "</height>");
		xml.add("        <depth>" + std::to_string(logo->depth)  + "</depth>");
		xml.add("        <url>/images/" + logo->name + ".png</url>");
		xml.add("      </icon>");
		++logo;
	}
	logo = services.getLogos();
	while (logo->valid) {
		xml.add("      <icon>");
		xml.add("        <mimetype>image/jpeg</mimetype>");
		xml.add("        <width>" + std::to_string(logo->width)  + "</width>");
		xml.add("        <height>" + std::to_string(logo->height)  + "</height>");
		xml.add("        <depth>" + std::to_string(logo->depth)  + "</depth>");
		xml.add("        <url>/images/" + logo->name + ".jpg</url>");
		xml.add("      </icon>");
		++logo;
	}
	xml.add("    </iconList>");

	xml.add("    <presentationURL>" + webroot + "</presentationURL>");
	xml.add("  </device>");
	xml.add("</root>");
	xml.add("\r\n");

	xmlResponseData = xml.raw('\r');

	// Set result data
	if (!xmlResponseData.empty()) {
		data = xmlResponseData.c_str();
		size = xmlResponseData.size();
		if (writeFile) {
			util::writeFile(deviceURL, xmlResponseData);
		}
	}
}

void TUPnP::getUPnPResponse(app::TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	const std::string& url = params[app::URI_REQUEST_URL].asString();
	const std::string& addr = params[app::URI_CLIENT_ADDRESS].asString();
	if (debug) {
		logger("URL \"" + url + "\" requested from [" + addr + "]");
	}
	// error = app::WSC_NotImplemented;
	response = "HTTP/1.1 501 NOT IMPLEMENTED\r\n";
	if (!response.empty()) {
		data = response.c_str();
		size = response.size();
	}
}


void TUPnP::setUPnPResponse(app::TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error) {
	bool ok = false;
	if (util::assigned(data) && size > 0) {
		std::string xml((char*)data, size);
		if (!xml.empty()) {
			const std::string& url = params[app::URI_REQUEST_URL].asString();
			const std::string& addr = params[app::URI_CLIENT_ADDRESS].asString();
			if (debug) {
				logger("SOAP data \"" + xml + "\" received from URL \"" + url + "\" requested from [" + addr + "]");
			}
			ok = true;
		}
	}
	if (!ok) {
		error = app::WSC_BadRequest;
	}
}



} /* namespace app */
