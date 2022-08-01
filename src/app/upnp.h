/*
 * upnp.h
 *
 *  Created on: 21.01.2019
 *      Author: dirk
 */

#ifndef APP_UPNP_H_
#define APP_UPNP_H_

#include "../inc/application.h"
#include "../inc/semaphores.h"
#include "../inc/inifile.h"
#include "../inc/timeout.h"
#include "../inc/sockets.h"
#include "../inc/gcc.h"
#include "upnptypes.h"
#include "upnpservices.h"
#include "avahi.h"

namespace upnp {


class TMessageQueue;
typedef app::TWorkerThread<TMessageQueue> TBroadcastThread;


class TMessageQueue {
private:
	app::TMutex listMtx;
	TMessageList messages;

	void cleanup();

public:
	PUPnPMessage first();
	void add(PUPnPMessage message);
	void insert(PUPnPMessage message);
	void remove(PUPnPMessage message);

	size_t size();
	bool empty();

	TMessageQueue();
	virtual ~TMessageQueue();
};


class TUPnP : public app::TModule {
private:
	std::string uuid;
	std::string device;
	std::string webroot;
	std::string serverid;
	std::string deviceURL;
	std::string xmlResponseData;
	std::string friendlyName;
	std::string description;
	app::TTimerDelay timeout;
	inet::PMulticastSocket mcast4;
	TBroadcastThread* thread;
	app::TNotifyEvent event;
	TMessageQueue messages;
	TUPnPServices services;
	avahi::TAvahiClient* avahi;
	app::PTimeout toBroadcast;
	app::TMutex sendMtx;
	app::TMutex eventMtx;
	app::TIniFile config;
	std::string response;
	bool bonjour;
	bool discovery;
	bool enabled;
	bool running;
	bool active;
	bool debug;
	bool verbose;

	void logger(const std::string& text) const;
	ssize_t onMulticastData(const inet::TUDPSocket& socket);
	int broadcastThreadHandler(TBroadcastThread& sender, TMessageQueue& messages);
	void processUPnPMessage(const util::TVariantValues& message, const inet::TAddressInfo sender);
	void sendUPnPResponse(const util::TVariantValues& message, const inet::TAddressInfo sender);
	void notifyEvent(const std::string message);
	void onBroadcast();

	std::string getWebRoot();
	std::string getSystemInfo();
	std::string getUPnPVersion();
	std::string getWebServerBuild();
	std::string getApplicationVersion();
	std::string getServerVersion();
	void getDeviceDescription(app::TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getServiceDescription(app::TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getUPnPResponse(app::TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void setUPnPResponse(app::TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error);

	void queueUPnPDevice(const EUPnPMessageType type, const util::TStringList& body);
	void queueUPnPService(const EUPnPMessageType type, const util::TStringList& body);

	EUPnPMessageType getMessageType(const std::string& request);
	std::string getDefaultSchema(const std::string service);
	bool getRequestType(const std::string& query, EUPnPServiceType& service, EUPnPRequestType& type);
	bool isExtension(const std::string service);
	bool isSelf(const std::string serial);

	void addUPnPMessage(PUPnPMessage message);
	void insertUPnPMessage(PUPnPMessage message);
	void debugUPnPMessage(void const * const data, size_t const size);

	void openConfig(const std::string& configPath);
	void readConfig();
	void writeConfig();
	void reWriteConfig();

public:
	void buildUPnPMessage(const std::string& query, const EUPnPServiceType service, const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body, util::TStringList& message);
	void queueUPnPMessage(const std::string& query, const EUPnPServiceType service, const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body);
	void insertUPnPMessage(const std::string& query, const EUPnPServiceType service, const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body);
	bool sendUPnPMessage(const std::string& query, const EUPnPServiceType service, const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body);
	void queueUPnPResponse(const inet::TAddressInfo& sender, const std::string& query, const EUPnPServiceType service, const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body);
	bool sendUPnPResponse(const inet::TAddressInfo& sender, const std::string& query, const EUPnPServiceType service, const EUPnPMessageType type, const EUPnPRequestType response, const util::TStringList& body);

	void broadcastSSDPDiscovery();
	void broadcastUPnPDevice();
	void broadcastUPnPServices();
	void broadcastExitMessage();

	void addServiceEntry(const std::string& type, const std::string& page, int port);

	bool isOnline();

	int prepare();
	int execute();
	void unprepare();
	void cleanup();

	TUPnP();
	virtual ~TUPnP();
};

} /* namespace app */

#endif /* APP_UPNP_H_ */
