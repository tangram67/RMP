/*
 * socket.h
 *
 *  Created on: 23.04.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <string>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../config.h"
#include "sockettypes.h"
#include "socketlists.h"
#include "fileutils.h"
#include "ssltypes.h"
#include "inifile.h"
#include "convert.h"
#include "threads.h"
#include "logger.h"
#include "timer.h"
#include "array.h"
#include "ssl.h"

#ifdef HAS_EPOLL
#  include <sys/epoll.h>
#endif

namespace inet {

bool hasIPv6();

bool inet4StrToAddr(const char *str, in_addr_t& addr);
bool inet4StrToAddr(const std::string& str, in_addr_t& addr);

std::string inetAddrToStr(const TAddrInfo& addr);
std::string inetAddrToStr(const TAddressInfo& addr);
std::string inetAddrToStr(struct sockaddr const * const addr);

std::string getInet4Address(const std::string& host);
std::string getInet6Address(const std::string& host);

int inetPortToInt(const TAddrInfo& addr);
int inetPortToInt(const TAddressInfo& addr);
int inetPortToInt(struct sockaddr const * const addr);

bool isIPv6Address(const std::string& addr);
bool isPrivateIPv4AddressRange(const std::string& addr);
bool isMemberOfIPv4AddressRange(const std::string addr, const std::string network, const std::string mask);
bool isMemberOfIPv4AddressRange(const in_addr_t addr, const in_addr_t network, const in_addr_t bits);
bool isMemberOfIPv4AddressMask(const in_addr_t addr, const in_addr_t network, const in_addr_t mask);

in_addr_t inet4BitsToHostOrderMask(const in_addr_t bits);
in_addr_t inet4BitsToNetworkOrderMask(const in_addr_t bits);

bool inet4CompareAddress(const in_addr_t addr1, const in_addr_t addr2, const in_addr_t bits);
bool inet4CompareAddress(struct sockaddr const * const addr, struct sockaddr const * const range, const in_addr_t bits);

template<typename T>
class TSocketGuard {
private:
	typedef T socket_t;
	socket_t&  instance;

public:
	TSocketGuard& operator=(const TSocketGuard&) = delete;
	explicit TSocketGuard(socket_t& F) : instance(F) {}
	TSocketGuard(const TSocketGuard&) = delete;
	~TSocketGuard() { instance.close(); }
};


class TByteOrder {
public:
	static uint16_t networkToHost(const uint16_t value) { return ntohs(value); }
	static uint32_t networkToHost(const uint32_t value) { return ntohl(value); }
	static int16_t  networkToHost(const int16_t value)  { return (int16_t)ntohs(value); }
	static int32_t  networkToHost(const int32_t value)  { return (int32_t)ntohl(value); }

	static uint16_t hostToNetwork(const uint16_t value) { return htons(value); }
	static uint32_t hostToNetwork(const uint32_t value) { return htonl(value); }
	static int16_t  hostToNetwork(const int16_t value)  { return (int16_t)htons(value); }
	static int32_t  hostToNetwork(const int32_t value)  { return (int32_t)htonl(value); }
};


class TInetAddressInfo {
private:
	typedef struct addrinfo * paddrinfo;
	paddrinfo info;
	bool freeAddress;

	void prime() {
		info = nil;
		freeAddress = false;
	}

public:
	struct addrinfo ** addresses() {
		return &info;
	}
	struct addrinfo * address() const {
		return info;
	}

	struct addrinfo * operator() () const {
		return address();
	}

	void setFreeAddress(const bool value) { freeAddress = value; };

	void clear() {
		if (util::assigned(info)) {
			if (freeAddress &&  util::assigned(info->ai_addr) ) {
				free(info->ai_addr);
				info->ai_addr = nil;
			}
			freeaddrinfo(info);
		}
		prime();
	}

	void create() {
		if (util::assigned(info))
			freeaddrinfo(info);
		info = (paddrinfo)calloc(1, sizeof(addrinfo));
	}

	TInetAddressInfo() {
		prime();
	}
	virtual ~TInetAddressInfo() {
		clear();
	}
};


class TInetAddress : public TInetAddressInfo {
private:
	std::string host;
	std::string service;
	ESocketType type;
	EAddressFamily family;
	int addrsys;
	int addrret;

	void prime() {
		addrsys = EXIT_SUCCESS;
		addrret = EXIT_SUCCESS;
	}

public:
	bool valid() const { return util::assigned(address()); };

	ESocketType getType() const { return type; };
	EAddressFamily getFamily() const { return family; };
	const std::string& getHost() const { return host; };
	const std::string& getService() const { return service; };

	void setType(const ESocketType type) { this->type = type; }
	void setFamily(const EAddressFamily family) { this->family = family; }

	bool getAddress(const std::string& host, const std::string& service = "");
	bool getAddress(const char *host, const char *service = NULL);
	std::string getAddress();

	bool compare(const TAddrInfo& addr) const;
	bool compare(const TAddressInfo& addr) const;
	bool compare(struct sockaddr const * const addr) const;

	std::string getLastAddressInfoError();
	void debugOutput(const int family = (AF_INET | AF_INET6));

	void reset() {
		host.clear();
		service.clear();
	}

	void clear() {
		reset();
		TInetAddressInfo::clear();
	}

	TInetAddress() : TInetAddressInfo(),
			type(ESocketType::ST_DEFAULT), family(EAddressFamily::AT_UNSPEC) { prime(); }
	TInetAddress(const ESocketType type, const EAddressFamily family) : TInetAddressInfo(),
			type(type), family(family) { prime(); }
	virtual ~TInetAddress() {
		clear();
	}
};


class TBaseSocket : private TInetAddress, public util::TFileHandle {
private:
	int id;
	int port;
	bool debug;
	bool blocking;
	bool keepalive;
	bool dualstack;
	bool listening;
	bool connected;
	bool listener;
	bool connector;
	bool upgraded;
	bool secure;
	bool local;
	bool udp;
	bool edge;
	int conncnt;
	mutable int recval;
	mutable int sndval;
	mutable std::string desc;
	mutable bool descOK;
	mutable app::TMutex clientMtx;
	TConnectionMap clients;
	app::PIniFile config;
	app::PLogFile logger;
	mutable CSocketConnection self;

	void prime();
	void reset();
	void disconnect();
	bool setIPv6OnlyOption();

protected:
	void setSelf(const app::THandle socket);
	void setPort(const int port);
	void setService(const std::string& service);

public:
	typedef typename TConnectionMap::const_iterator const_iterator;

	inline const_iterator begin() { return clients.begin(); };
	inline const_iterator end() { return clients.end(); };

	bool upgrade(const app::THandle socket);
	int open(const std::string& host, const std::string& service,
			const ESocketType type = ESocketType::ST_DEFAULT,
			const EAddressFamily family = EAddressFamily::AT_DEFAULT);
	int open(const std::string& host, const int port,
			const ESocketType type = ESocketType::ST_DEFAULT,
			const EAddressFamily family = EAddressFamily::AT_DEFAULT);
	void close();

	bool bind();
	bool listen();
	bool connect();
	app::THandle accept(struct sockaddr * from, socklen_t * fromlen, int const flags = 0) const;
	bool setOption(const int option, const bool value = true);
	bool getOption(const int option, bool& value);
	void drop(const app::THandle client);
	void shutdown(int how);

	PSocketConnection addClient(const app::THandle client);
	PSocketConnection addClient(const app::THandle client, const std::string& remote);
	PSocketConnection addClient(PSocketConnection connection);
	void removeClient(const app::THandle client);

	ssize_t receive(const app::THandle hnd, void * const data, size_t const size, int const flags = 0) const;
	ssize_t receive(void * const data, size_t const size, 	int const flags = 0) const;
	ssize_t receiveFrom(void * const data, size_t const size, struct sockaddr * const from, socklen_t * const fromlen, int const flags = 0) const;

	ssize_t send(const app::THandle hnd, void const * const data, size_t const size, int const flags = 0) const;
	ssize_t send(void const * const data, size_t const size, const int flags = 0) const;
	ssize_t sendTo(void const * const data, size_t const size, struct sockaddr const * const to, socklen_t const tolen, int const flags = 0) const;

	void setSSLConnection(const util::PSSLConnection connection) { self.ssl = connection; };
	void setSocket(PSocket socket) { self.socket = socket; };
	inline PSocketConnection getSocket() const { return &self; };
	inline const std::string& getHost() const { return TInetAddress::getHost(); };
	inline const std::string& getService() const { return TInetAddress::getService(); };
	inline EAddressFamily getFamily() const { return TInetAddress::getFamily(); };
	const std::string& getDescription() const;
	int getPort() const { return port; };
	int getID() const { return id; };

	void setServer() { listener = true; }
	void setClient() { connector = true; };
	void setUpgraded() { upgraded = true; };
	void setSecure() { secure = true; };
	void setDatagram() { udp = true; };
	void setUnix() { local = true; };
	bool isUnix() const { return local; };
	bool isServer() const { return listener; };
	bool isClient() const { return connector; };
	bool isUpgraded() const { return upgraded; };
	bool isSecure() const { return secure; };
	bool isDatagram() const { return udp; };
	bool isBlocking() const { return blocking; };
	bool isListening() const { return listening; };
	bool isConnected() const { return connected; };
	void setConnected(const bool connected) { this->connected = connected; };
	bool wasConnected() const { return conncnt > 0; };
	bool hasClient(const app::THandle client) const;
	bool hasClients() const;

	bool isFamily4() const { return inet::EAddressFamily::AT_INET4 == getFamily(); };
	bool isFamily6() const { return inet::EAddressFamily::AT_INET6 == getFamily(); };

	bool changed() const { return edge; };
	void change() { edge = true; };
	void unchange() { edge = false; };
	int recerr() const { return recval; };
	int snderr() const { return sndval; };

	void setConfig(app::PIniFile config) { this->config = config; };
	app::PIniFile getConfig() const { return config; };
	void setLogger(app::PLogFile logger) { this->logger = logger; };
	app::PLogFile getLogger() const { return logger; };

	bool useKeepAlive() const { return keepalive; };
	bool getDebug() const { return debug; };
	void setDebug(bool debug) { this->debug = debug; };

	void debugOutput() { TInetAddress::debugOutput(); };
	void writeLog(const std::string& s) const;
	void errorLog(const std::string& s) const;

	template<typename value_t, typename... variadic_t>
	void writeLogFmt(const std::string& str, const value_t value, variadic_t... args) const;

	template<typename value_t, typename... variadic_t>
	void errorLogFmt(const std::string& str, const value_t value, variadic_t... args) const;

	void readConfig(std::string& host, int& port, const inet::EAddressFamily family);
	void writeConfig(const std::string& host, const int port, const inet::EAddressFamily family);
	void reWriteConfig(std::string& host, int& port, const inet::EAddressFamily family);

	explicit TBaseSocket(PSocketController owner, const bool debug);
	TBaseSocket(const bool debug);
	TBaseSocket();
	virtual ~TBaseSocket();
};


class TSocket {
friend class TSocketController;

private:
	PSocketController owner;
	util::TTimePart timestamp;

	TSocketEventMethod onSocketConnect;
	TSocketEventMethod onSocketClose;

	virtual ssize_t executeDataWrapper(PSocketConnection connection, app::THandle client) = 0;
	void executeConnectMethod(app::THandle client);
	void executeCloseMethod(app::THandle client);

	void prime();

protected:
	TBaseSocket socket;

	inline void setServer() { socket.setServer(); }
	inline void setClient() { socket.setClient(); };
	inline void setUpgraded() { socket.setUpgraded(); };
	inline void setSecure() { socket.setSecure(); };
	inline void setDatagram() { socket.setDatagram(); };
	inline void setUnix() { socket.setUnix(); };
	inline void setTime() { timestamp = util::now(); };
	inline util::TTimePart getTime() const { return timestamp; };

	inline bool changed() const { return socket.changed(); };
	inline void change() { socket.change(); };
	inline void unchange() { socket.unchange(); };

	inline bool hasClient(const app::THandle client) const { return socket.hasClient(client); };
	inline PSocketConnection getSocket() const { return socket.getSocket(); };

	inline void setSSLConnection(const util::PSSLConnection connection) { socket.setSSLConnection(connection); };
	inline bool hasSSLConnection() const { return util::assigned(socket.getSocket()->ssl); };
	inline util::PSSLConnection getSSLConnection() const { return socket.getSocket()->ssl; };
	inline void clearSSLConnection() const { util::freeAndNil(socket.getSocket()->ssl); };

	inline app::THandle accept(struct sockaddr * from, socklen_t * fromlen, int const flags = 0) const;
	PSocketConnection addClient(const app::THandle client);
	PSocketConnection addClient(const app::THandle client, const std::string& remote);
	PSocketConnection addClient(PSocketConnection connection);
	void removeClient(const app::THandle client);
	void reconnect();

public:
	typedef typename TConnectionMap::const_iterator const_iterator;

	inline const_iterator begin() { return socket.begin(); };
	inline const_iterator end() { return socket.end(); };

	inline void close() { socket.close(); };
	inline void drop(const app::THandle client) { socket.drop(client); };
	inline app::THandle handle() const { return socket.handle(); };
	inline bool isOpen() const { return socket.isOpen(); };
	inline bool isServer() const { return socket.isServer(); };
	inline bool isClient() const { return socket.isClient(); };
	inline bool isUpgraded() const { return socket.isUpgraded(); };
	inline bool isUnix() const { return socket.isUnix(); };
	inline bool isSecure() const { return socket.isSecure(); };
	inline bool isDatagram() const { return socket.isDatagram(); };
	inline bool isListening() const { return socket.isListening(); };
	inline bool isConnected() const { return socket.isConnected(); };
	inline void setConnected(const bool connected) { socket.setConnected(connected); };
	inline bool wasConnected() const { return socket.wasConnected(); };
	inline bool hasClients() const { return socket.hasClients(); };
	inline int getPort() const { return socket.getPort(); };
	inline int getID() const { return socket.getID(); };

	inline bool isFamily4() const { return socket.isFamily4(); };
	inline bool isFamily6() const { return socket.isFamily6(); };

	inline void setName(const std::string& name) { socket.setName(name); };
	inline const std::string& getName() const { return socket.getName(); };
	inline const std::string& getHost() const { return socket.getHost(); };
	inline const std::string& getService() const { return socket.getService(); };
	inline EAddressFamily getFamily() const { return socket.getFamily(); };
	inline const std::string& getDescription() const { return socket.getDescription(); };

	inline int recerr() const { return socket.recerr(); };
	inline int snderr() const { return socket.snderr(); };

	inline void setOwner(PSocketController owner) { this->owner = owner; };
	inline PSocketController getOwner() const { return owner; };
	inline void setConfig(app::PIniFile config) { socket.setConfig(config); };
	inline app::PIniFile getConfig() const { return socket.getConfig(); };
	inline void setLogger(app::PLogFile logger) { socket.setLogger(logger); };
	inline void setDebug(bool debug) { socket.setDebug(debug); };
	inline bool getDebug() const { return socket.getDebug(); };

	inline void writeLog(const std::string& s) const { socket.writeLog(s); };
	inline void errorLog(const std::string& s) const { socket.errorLog(s); };

	template<typename value_t, typename... variadic_t>
	void writeLogFmt(const std::string& str, const value_t value, variadic_t... args) const;

	template<typename value_t, typename... variadic_t>
	void errorLogFmt(const std::string& str, const value_t value, variadic_t... args) const;

	virtual int verify(int verified, X509_STORE_CTX *ctx, SSL * ssl, X509 * crt, const std::string& issuer, int depth, int error) = 0;

	template<typename connect_t, typename class_t>
		inline void bindConnectHandler(connect_t &&onSocketConnect, class_t &&owner) {
			this->onSocketConnect = std::bind(onSocketConnect, owner, std::placeholders::_1, std::placeholders::_2);
		}

	template<typename close_t, typename class_t>
		inline void bindCloseHandler(close_t &&onSocketClose, class_t &&owner) {
			this->onSocketClose = std::bind(onSocketClose, owner, std::placeholders::_1, std::placeholders::_2);
		}

	explicit TSocket(PSocketController owner, const bool debug);
	TSocket(const bool debug);
	TSocket();
	virtual ~TSocket();
};




class TSocketController : public app::TObject {
private:
	bool debug;
	int timeout;
	bool running;
	bool shutdown;
	bool enabled;
	app::PThreadController threads;
	app::PTimerController timers;
	app::PManagedThread thread;
	app::PTimer timer;
	TPollEvent events;
	app::TMutex evntMtx;
	app::TMutex recnMtx;
	TSocketList sockets;
	TSocketList reconnects;
	std::string configFolder;
	std::string configFile;
	app::PIniFile config;
	app::PLogFile logger;
	util::TStringList allowedList;
	bool allowFromAll;
	bool invalidated;
	bool reconnected;
	bool useEpoll;

#ifdef HAS_EPOLL
	app::THandle epollfd;
	struct epoll_event eevent;
	struct epoll_event* revents;
	size_t rsize;
	size_t ecount;
	size_t scount;
	uint32_t mask;
	TSocketConnectionList reader;
#endif

	TSocketAcceptMethod onSocketAccept;

	ssize_t onSocketData(PSocket socket, PSocketConnection connection, const app::THandle client);
	ssize_t onSocketData(const app::THandle client);

	bool acceptSocket(PSocket socket, const std::string& addr, bool& accept);
	bool acceptTLS(PSocket socket, const app::THandle client, util::PSSLConnection& ssl, bool& accept);

	void addSocket(PSocket socket);
	void removeSocket(PSocket socket);

	PSocketConnection addHandle(PSocket socket, const app::THandle client, const std::string& remote);
	void removeHandle(PSocket socket, const app::THandle client);
	void removeHandle(const app::THandle client);

	PSocket findSocket(const app::THandle socket);
	PSocket findClient(const app::THandle client);

	void onReconnectTimer();

#ifdef HAS_EPOLL
	PSocketConnection addHandle(PSocket socket, util::PSSLConnection ssl, const app::THandle client,
			const std::string& remote, const uint32_t mask);
	void removeHandle(PSocketConnection connection, PSocket socket, const app::THandle client);

	app::THandle createEpollHandle(int flags = 0);
	void closeEpollHandle();

	void addSocketHandle(PSocket socket);
	int addEpollHandle(struct epoll_event* event, const app::THandle hnd);
	int removeEpollHandle(struct epoll_event* event, const app::THandle hnd);

	void addEpollEvent(PSocketConnection connection, const app::THandle hnd, const uint32_t mask);
	void removeEpollEvent(PSocketConnection connection, const app::THandle hnd);

	int epoll();
	size_t createEpollEvents();
	void roundRobinReader();
	void invalidateReader(const app::THandle client);
	void doDisconnectAction(PSocketConnection connection, PSocket socket, const app::THandle server, const app::THandle client);

	void debugOutputEpoll(size_t size);
#endif

	size_t createPollEvents(short int events);
	void resetPollEvents(short int events);
	int poll();

	int loop(app::TManagedThread& sender);
	int eloop(app::TManagedThread& sender);

	void clear();
	void close();
	void prime();

	void readConfig();
	void writeConfig();
	void reWriteConfig();

    template<typename socket_t>
    bool reconnect(socket_t&& socket);
    void reconnector();

public:
	template<class class_t, typename data_t, typename owner_t>
		inline class_t* addSocket(const std::string& name, data_t &&onSocketData, owner_t &&owner) {
			static_assert(std::is_reference<decltype(onSocketData)>::value, "TSocketList::addSocket() : Argument <onSocketData> is not a reference.");
			static_assert(std::is_reference<decltype(owner)>::value,        "TSocketList::addSocket() : Argument <owner> is not a reference.");
			if (getDebug()) std::cout << "TSocketList::addSocket(" << name << ")" << std::endl;
			class_t* o = new class_t;
			o->bindDataHandler(onSocketData, owner);
			o->setName(name);
			// Use "type erasure" to store socket in socket list of generic type TSocket
			PSocket p = dynamic_cast<PSocket>(o);
			addSocket(p);
			return o;
		};

	template<class class_t, typename data_t, typename connect_t, typename close_t, typename owner_t>
		inline class_t* addSocket(const std::string& name, data_t &&onSocketData, connect_t &&onSocketConnect, close_t &&onSocketClose, owner_t &&owner) {
			static_assert(std::is_reference<decltype(onSocketData)>::value,    "TSocketList::addSocket() : Argument <onSocketData> is not a reference.");
			static_assert(std::is_reference<decltype(onSocketConnect)>::value, "TSocketList::addSocket() : Argument <onSocketConnect> is not a reference.");
			static_assert(std::is_reference<decltype(onSocketClose)>::value,   "TSocketList::addSocket() : Argument <onSocketClose> is not a reference.");
			static_assert(std::is_reference<decltype(owner)>::value,           "TSocketList::addSocket() : Argument <owner> is not a reference.");
			class_t* o = new class_t;
			o->bindDataHandler(onSocketData, owner);
			o->bindConnectHandler(onSocketConnect, owner);
			o->bindCloseHandler(onSocketClose, owner);
			o->setName(name);
			// Use "type erasure" to store socket in socket list of generic type TSocket
			PSocket p = dynamic_cast<PSocket>(o);
			addSocket(p);
			return o;
		};

	template<typename accept_t, typename owner_t>
		inline void setAcceptHandler(accept_t &&onSocketAccept, owner_t &&owner) {
			static_assert(std::is_reference<decltype(onSocketAccept)>::value, "TSocketList::setAcceptHandler() : Argument <onSocketAccept> is not a reference.");
			static_assert(std::is_reference<decltype(owner)>::value,          "TSocketList::setAcceptHandler() : Argument <owner> is not a reference.");
			this->onSocketAccept = std::bind(onSocketAccept, owner, std::placeholders::_1, std::placeholders::_2);
		}

	void start();
	void enable() { enabled = true; };
	void disable() { enabled = false; };
	void terminate();
	void waitFor();

	void addReconnectSocket(PSocket socket);
	PSocket find(const std::string& name);

	bool getEnabled() const { return enabled; };
	bool getDebug() const { return debug; };
	void setDebug(bool debug) { this->debug = debug; };
	void debugOutput() { events.debugOutput(); };
	void writeLog(const std::string& s) const;
	void errorLog(const std::string& s) const;

	template<typename value_t, typename... variadic_t>
	void writeLogFmt(const std::string& str, const value_t value, variadic_t... args);

	template<typename value_t, typename... variadic_t>
	void errorLogFmt(const std::string& str, const value_t value, variadic_t... args);

	TSocketController(app::PThreadController threads, app::PTimerController timers, app::PLogFile logger, const std::string& configFolder);
	virtual ~TSocketController();
};



class TServerSocket : public inet::TSocket {
friend class TSocketController;

private:
	TServerDataMethod onServerData;

	void prime();
	ssize_t executeDataWrapper(PSocketConnection connection, app::THandle client);
	ssize_t executeDataMethod(app::THandle client);
	int verify(int verified, X509_STORE_CTX *ctx, SSL * ssl, X509 * crt, const std::string& issuer, int depth, int error) { return verified; };

public:
	template<typename data_t, typename class_t>
		inline void bindDataHandler(data_t &&onSocketData, class_t &&owner) {
			this->onServerData = std::bind(onSocketData, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		}

	bool open(const std::string& bindTo, const int port, const inet::EAddressFamily family);
	ssize_t receive(const app::THandle hnd, void * const data, size_t const size, int const flags = 0) const;
	ssize_t send(const app::THandle hnd, void const * const data, size_t const size, int const flags = 0) const;

	TServerSocket(const bool debug = false);
	~TServerSocket();
};


class TClientSocket : public inet::TSocket {
friend class TSocketController;

private:
	TClientDataMethod onClientData;

	void prime();
	ssize_t executeDataWrapper(PSocketConnection connection, app::THandle client);
	ssize_t executeDataMethod();
	int verify(int verified, X509_STORE_CTX *ctx, SSL * ssl, X509 * crt, const std::string& issuer, int depth, int error) { return verified; };

protected:
	bool open();

public:
	template<typename data_t, typename class_t>
		inline void bindDataHandler(data_t &&onSocketData, class_t &&owner) {
			this->onClientData = std::bind(onSocketData, owner, std::placeholders::_1);
		}

	bool open(const std::string& serverIP, const int port, const inet::EAddressFamily family);
	ssize_t receive(void * const data, size_t const size, int const flags = 0) const;
	ssize_t send(void const * const data, size_t const size, int const flags = 0) const;

	TClientSocket(const bool debug = false);
	~TClientSocket();
};


class TTLSBaseSocket {
protected:
	util::TSSLContext ssl;
	TTLSBaseSocket() {};

public:
	inline util::PSSLContext context() { return &ssl; };
	~TTLSBaseSocket() {};
};


class TTLSServerSocket final : public inet::TServerSocket, public inet::TTLSBaseSocket {
friend class TSocketController;

private:
	TTLSServerDataMethod onServerData;

	std::string certFile;
	std::string keyFile;
	std::string dhFile;
	bool certsLoaded;

	void prime();
	void init();
	void loadCertificates();
	void readConfig();
	void writeConfig();
	void reWriteConfig();

	int verify(int verified, X509_STORE_CTX *ctx, SSL * ssl, X509 * crt, const std::string& issuer, int depth, int error) { return verified; };

	ssize_t executeDataWrapper(PSocketConnection connection, app::THandle client);
	ssize_t executeDataMethod(const util::PSSLConnection connection);

public:
	template<typename data_t, typename class_t>
		inline void bindDataHandler(data_t &&onSocketData, class_t &&owner) {
			this->onServerData = std::bind(onSocketData, owner, std::placeholders::_1, std::placeholders::_2);
		}

	bool open(const std::string& bindTo, const int port, const inet::EAddressFamily family);
	ssize_t send(const util::PSSLConnection connection, void const * const data, size_t const size) const;
	ssize_t receive(const util::PSSLConnection connection, void * const data, size_t const size) const;

	TTLSServerSocket(const bool debug = false);
	~TTLSServerSocket();
};


class TTLSClientSocket final : public inet::TClientSocket, public inet::TTLSBaseSocket {
friend class TSocketController;

private:
	std::string issuer;
	std::string ciphers;
	TTLSClientDataMethod onClientData;
	void prime();
	void init();
	bool open();
	void startTLS();
	void reconnect();

	int verify(int verified, X509_STORE_CTX *ctx, SSL * ssl, X509 * crt, const std::string& issuer, int depth, int error);

	ssize_t executeDataWrapper(PSocketConnection connection, app::THandle client);
	ssize_t executeDataMethod();

	void readConfig();
	void writeConfig();
	void reWriteConfig();

public:
	template<typename data_t, typename class_t>
		inline void bindDataHandler(data_t &&onSocketData, class_t &&owner) {
			this->onClientData = std::bind(onSocketData, owner, std::placeholders::_1);
		}

	bool open(const std::string& serverIP, const int port, const inet::EAddressFamily family);
	void close();

	ssize_t send(void const * const data, size_t const size) const;
	ssize_t receive(void * const data, size_t const size) const;

	TTLSClientSocket(const bool debug = false);
	~TTLSClientSocket();
};



class TUDPSocket : public inet::TSocket {
friend class TSocketController;

private:
	TUDPDataMethod onUDPData;
	std::string host;
	bool unicast;

	void prime();
	ssize_t executeDataWrapper(PSocketConnection connection, app::THandle client);
	ssize_t executeDataMethod(app::THandle client);
	int verify(int verified, X509_STORE_CTX *ctx, SSL * ssl, X509 * crt, const std::string& issuer, int depth, int error) { return verified; };

protected:
	TInetAddress receiver;
	void setHost(const std::string host) { this->host = host; };

public:
	template<typename data_t, typename class_t>
		inline void bindDataHandler(data_t &&onSocketData, class_t &&owner) {
			this->onUDPData = std::bind(onSocketData, owner, std::placeholders::_1);
		}

	const std::string& getHost() const { return host; };
	bool isUnicast() const { return unicast; };
	void setUnicast(const bool value) { unicast = value; };

	bool open(const std::string& sendTo, const int port, const inet::EAddressFamily family);
	ssize_t receive(void * const data, size_t const size, int const flags = 0) const;
	ssize_t receiveFrom(void * const data, size_t const size, TAddressInfo& from, int const flags = 0) const;
	ssize_t send(void const * const data, size_t const size, int const flags = 0) const;
	ssize_t sendTo(void const * const data, size_t const size, const TAddressInfo& to, int const flags = 0) const;

	TUDPSocket(const bool debug = false);
	~TUDPSocket();
};


class TMulticastSocket final : public inet::TUDPSocket {
friend class TSocketController;

private:
	bool isUnicast() const;
	void setUnicast(const bool value);
	//bool open(const std::string& sendTo, const int port, const inet::EAddressFamily family);

	bool setOption(const int level, const int option, void const * const value, const size_t size);

public:
	bool open(const std::string& mcast, const int port, const inet::EAddressFamily family = EAddressFamily::AT_INET4);

	TMulticastSocket(const bool debug = false);
	~TMulticastSocket();
};


class TUnixServerSocket final : public inet::TSocket {
friend class TSocketController;

private:
	TUnixServerDataMethod onServerData;

	void prime();
	ssize_t executeDataWrapper(PSocketConnection connection, app::THandle client);
	ssize_t executeDataMethod(app::THandle client);
	int verify(int verified, X509_STORE_CTX *ctx, SSL * ssl, X509 * crt, const std::string& issuer, int depth, int error) { return verified; };

public:
	template<typename data_t, typename class_t>
		inline void bindDataHandler(data_t &&onSocketData, class_t &&owner) {
			this->onServerData = std::bind(onSocketData, owner, std::placeholders::_1, std::placeholders::_2);
		}

	bool open(const std::string& device);
	ssize_t receive(const app::THandle hnd, void * const data, size_t const size, int const flags = 0) const;
	ssize_t send(const app::THandle hnd, void const * const data, size_t const size, int const flags = 0) const;

	TUnixServerSocket(const bool debug = false);
	~TUnixServerSocket();
};


class TUnixClientSocket final : public inet::TSocket {
friend class TSocketController;

private:
	TUnixClientDataMethod onClientData;

	void prime();
	ssize_t executeDataWrapper(PSocketConnection connection, app::THandle client);
	ssize_t executeDataMethod();
	int verify(int verified, X509_STORE_CTX *ctx, SSL * ssl, X509 * crt, const std::string& issuer, int depth, int error) { return verified; };

protected:
	bool open();

public:
	template<typename data_t, typename class_t>
		inline void bindDataHandler(data_t &&onSocketData, class_t &&owner) {
			this->onClientData = std::bind(onSocketData, owner, std::placeholders::_1);
		}

	bool open(const std::string& device);
	ssize_t receive(void * const data, size_t const size, int const flags = 0) const;
	ssize_t send(void const * const data, size_t const size, int const flags = 0) const;

	TUnixClientSocket(const bool debug = false);
	~TUnixClientSocket();
};


} /* namespace inet */

#endif /* SOCKETS_H_ */
