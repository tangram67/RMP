/*
 * websockets.h
 *
 *  Created on: 19.06.2020
 *      Author: dirk
 */

#ifndef INC_WEBSOCKETS_H_
#define INC_WEBSOCKETS_H_

#include "sockets.h"
#include "variant.h"
#include "fileutils.h"
#include "socketlists.h"
#include "sockettypes.h"
#include "websockettypes.h"

namespace app {

STATIC_CONST size_t WEBSOCKET_BUFFER_SIZE = inet::INET_DEFAULT_MTU_SIZE + 8;
STATIC_CONST size_t WEBSOCKET_MAX_SEND_SIZE = inet::INET_DEFAULT_MTU_SIZE - 32; // Take JSON header and mask bits in account

class TWebSockets : public TObject {
private:
	bool debug;
	bool running;
	bool shutdown;
	bool invalidated;
	bool secure;
	int timeout;
	size_t mtu;
	size_t removed;
	util::TTimePart timestamp;

	TWebSocketDataHandler onWebSocketData;
	TWebSocketVariantHandler onWebSocketVariant;
	TWebSocketConnectHandler onWebSocketConnect;

	app::PThreadController threads;
	app::PManagedThread thread;
	app::PIniFile config;
	app::PLogFile logger;

	std::string instance;
	app::TWebSocketList sockets;
	app::TWebSocketList reader;

	mutable app::TMutex listMtx;
	mutable app::TMutex sendMtx;
	inet::TPollEvent events;
	app::THandle epollfd;
	epoll_event* revents;
	size_t rsize;
	size_t ecount;
	size_t scount;
	uint32_t mask;

	void prime();
	void clear();
	void close();
	void downgrade(PWebSocket socket);

	void readConfig();
	void writeConfig();
	void reWriteConfig();

	int epoll();
	app::THandle createEpollHandle(int flags = 0);
	void closeEpollHandle();

	int eloop(app::TManagedThread& sender);
	size_t createEpollEvents();
	void debugOutputEpoll(size_t size);
	void roundRobinReader();

	bool unmaskSocketData(const void* data, size_t size, util::TBuffer& output, bool& ping, bool& closed) const;
	void maskSocketData(const void* data, size_t size, util::TBuffer& output, const bool masked) const;

	void parseSocketData(const void *const data, const size_t size, std::string& output, util::TVariantValues& variants) const;
	void decodeSocketData(const app::THandle handle, const void *const data, const size_t size, std::string& output, util::TVariantValues& variants, bool& closed);
	void processSocketData(const app::THandle handle, const std::string& output, const util::TVariantValues& variants);
	void onSockectConnected(const app::THandle handle);

	ssize_t readSocketData(PWebSocket socket, bool& closed);
	ssize_t receive(PWebSocket socket, void * const data, size_t const size, int const flags = 0) const;
	ssize_t send(const app::THandle handle, void const * const data, size_t const size, int const flags = 0);
	ssize_t sendPingResponse(const app::THandle handle, util::TBuffer& message);

	void invalidateSocket(PWebSocket socket);
	void invalidateSocket(const app::THandle handle);
	void invalidateSocketWithNolock(PWebSocket socket);
	void removeInvalidatedSockets();

	void addSocket(PWebSocket socket);
	void removeSocket(PWebSocket socket);
	void addSocketHandle(PWebSocket socket);
	void removeSocketHandle(PWebSocket socket);

	size_t getSocketHandles(TWebHandleList& handles) const;

public:
	void start();
	void terminate();
	void waitFor();

	bool isTerminated() const;

	void incEpollCount();
	void decEpollCount();
	size_t getEpollCounT() const { return ecount; };

	bool getDebug() const { return debug; };
	void setDebug(bool debug) { this->debug = debug; };

	bool isSecure() const { return secure; };
	void setSecure(bool secure) { this->secure = secure; };

	void debugOutput() { events.debugOutput(); };
	void writeLog(const std::string& s) const;
	void errorLog(const std::string& s) const;

	int addEpollHandle(epoll_event* event, const app::THandle socket);
	int removeEpollHandle(epoll_event* event, const app::THandle socket);
	void close(PWebSocket socket);

	void upgrade(MHD_socket socket, struct MHD_UpgradeResponseHandle* urh, const char* data, size_t size);
	ssize_t write(const app::THandle handle, void const * const data, size_t const size);
	ssize_t broadcast(void const * const data, size_t const size);

	template<typename value_t, typename... variadic_t>
		void writeLogFmt(const std::string& str, const value_t value, variadic_t... args) {
			writeLog(util::csnprintf(str, value, std::forward<variadic_t>(args)...));
		}

	template<typename value_t, typename... variadic_t>
		void errorLogFmt(const std::string& str, const value_t value, variadic_t... args) {
			errorLog(util::csnprintf(str, value, std::forward<variadic_t>(args)...));
		}

	template<typename reader_t, typename class_t>
		inline void bindSocketConnectEvent(reader_t &&onConnected, class_t &&owner) {
			onWebSocketConnect = std::bind(onConnected, owner, std::placeholders::_1);
		}

	template<typename reader_t, typename class_t>
		inline void bindSocketDataEvent(reader_t &&onReceived, class_t &&owner) {
			onWebSocketData = std::bind(onReceived, owner, std::placeholders::_1, std::placeholders::_2);
		}

	template<typename reader_t, typename class_t>
		inline void bindSocketVariantEvent(reader_t &&onReceived, class_t &&owner) {
			onWebSocketVariant = std::bind(onReceived, owner, std::placeholders::_1, std::placeholders::_2);
		}

	TWebSockets(const std::string name, app::PThreadController threads, app::PLogFile logger, app::PIniFile config);
	virtual ~TWebSockets();
};

} /* namespace app */

#endif /* INC_WEBSOCKETS_H_ */
