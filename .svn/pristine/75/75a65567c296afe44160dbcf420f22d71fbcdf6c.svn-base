/*
 * sockettypes.h
 *
 *  Created on: 25.04.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef SOCKETTYPES_H_
#define SOCKETTYPES_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <map>
#include "../config.h"
#include "ssltypes.h"
#include "exception.h"
#include "templates.h"

#ifdef HAS_EPOLL
#  include <sys/epoll.h>
#endif


namespace inet {

class TSocket;
class TBaseSocket;
class TUDPSocket;
class TServerSocket;
class TClientSocket;
class TWebSocket;
class TTLSServerSocket;
class TTLSClientSocket;
class TMulticastSocket;
class TUnixServerSocket;
class TUnixClientSocket;
class TSocketController;
struct CSocketConnection;

#ifdef STL_HAS_TEMPLATE_ALIAS

using TAddrInfo = struct addrinfo;

using PSocket = TSocket*;
using PUDPSocket = TUDPSocket*;
using PBaseSocket = TBaseSocket*;
using PServerSocket = TServerSocket*;
using PClientSocket = TClientSocket*;
using PTLSServerSocket = TTLSServerSocket*;
using PTLSClientSocket = TTLSClientSocket*;
using PUnixServerSocket = TUnixServerSocket*;
using PUnixClientSocket = TUnixClientSocket*;
using PMulticastSocket = TMulticastSocket*;

using PSocketConnection = CSocketConnection*;
using TSocketConnectionList = std::vector<inet::PSocketConnection>;

using PSocketController = TSocketController*;

using TSocketList = std::vector<inet::PSocket>;
using TConnectionMap = std::map<app::THandle, inet::PSocketConnection>;
using TConnectionMapItem = std::pair<app::THandle, inet::PSocketConnection>;

using TUDPDataMethod = std::function<ssize_t(const TUDPSocket&)>;
using TServerDataMethod = std::function<ssize_t(const TServerSocket&, const app::THandle, bool& drop)>;
using TClientDataMethod = std::function<ssize_t(const TClientSocket&)>;
using TWebDataMethod = std::function<ssize_t(const TWebSocket&, const std::string&)>;
using TTLSServerDataMethod = std::function<ssize_t(const TTLSServerSocket&, const util::PSSLConnection)>;
using TTLSClientDataMethod = std::function<ssize_t(const TTLSClientSocket&)>;
using TUnixServerDataMethod = std::function<ssize_t(const TUnixServerSocket&, const app::THandle)>;
using TUnixClientDataMethod = std::function<ssize_t(const TUnixClientSocket&)>;
using TSocketEventMethod = std::function<void(TSocket&, const app::THandle)>;
using TSocketAcceptMethod = std::function<void(const std::string&, bool&)>;

#else

typedef struct addrinfo TAddrInfo;

typedef TSocket* PSocket;
typedef TUDPSocket* PUDPSocket;
typedef TBaseSocket* PBaseSocket;
typedef TServerSocket* PServerSocket;
typedef TClientSocket* PClientSocket;
typedef TTLSServerSocket* PTLSServerSocket;
typedef TTLSClientSocket* PTLSClientSocket;
typedef TUnixServerSocket* PUnixServerSocket;
typedef TUnixClientSocket* PUnixClientSocket;
typedef TMulticastSocket* PMulticastSocket;

typedef CSocketConnection* PSocketConnection;
typedef std::vector<inet::PSocketConnection> TSocketConnectionList;

typedef TSocketController* PSocketController;

typedef std::vector<inet::PSocket> TSocketList;
typedef std::map<app::THandle, inet::PSocketConnection> TConnectionMap;
typedef std::pair<app::THandle, inet::PSocketConnection> TConnectionMapItem;

typedef std::function<ssize_t(const TUDPSocket&)> TUDPDataMethod;
typedef std::function<ssize_t(const TServerSocket&, const app::THandle, bool& drop)> TServerDataMethod;
typedef std::function<ssize_t(const TClientSocket&)> TClientDataMethod;
typedef std::function<ssize_t(const TWebSocket&, const std::string&)> TWebDataMethod;
typedef std::function<ssize_t(const TTLSServerSocket&, const util::PSSLConnection)> TTLSServerDataMethod;
typedef std::function<ssize_t(const TTLSClientSocket&)> TTLSClientDataMethod;
typedef std::function<ssize_t(const TUnixServerSocket&, const app::THandle)> TUnixServerDataMethod;
typedef std::function<ssize_t(const TUnixClientSocket&)> TUnixClientDataMethod;
typedef std::function<void(const TSocket&, app::THandle)> TSocketEventMethod;
typedef std::function<void(const std::string&, bool&)> TSocketAcceptMethod;

#endif


STATIC_CONST int MAX_SOCK_WAIT = 20;

STATIC_CONST char SSL_CURVED_DH_PLACEHOLDER[] = "curved";
STATIC_CONST char SSL_BUILTIN_PLACEHOLDER[]   = "builtin";
STATIC_CONST char IPV4ANY[] = "0.0.0.0";
STATIC_CONST char IPV6ANY[] = "::";

// IPv4 private address ranges as string representation
STATIC_CONST char IPV4_PRIVATE1_ADDR[] = "127.0.0.0";
STATIC_CONST char IPV4_PRIVATE2_ADDR[] = "10.0.0.0";
STATIC_CONST char IPV4_PRIVATE3_ADDR[] = "172.16.0.0";
STATIC_CONST char IPV4_PRIVATE4_ADDR[] = "192.168.0.0";

// IPv4 private address ranges as binary representation
STATIC_CONST in_addr_t INET4_PRIVATE1_MASK_SIZE = 8;
STATIC_CONST in_addr_t INET4_PRIVATE2_MASK_SIZE = 8;
STATIC_CONST in_addr_t INET4_PRIVATE3_MASK_SIZE = 12;
STATIC_CONST in_addr_t INET4_PRIVATE4_MASK_SIZE = 16;

static const UNUSED in_addr_t INET4_PRIVATE1_ADDR = inet_addr(IPV4_PRIVATE1_ADDR); // Mask 8 Bit
static const UNUSED in_addr_t INET4_PRIVATE2_ADDR = inet_addr(IPV4_PRIVATE2_ADDR); // Mask 8 Bit
static const UNUSED in_addr_t INET4_PRIVATE3_ADDR = inet_addr(IPV4_PRIVATE3_ADDR); // Mask 12 Bit
static const UNUSED in_addr_t INET4_PRIVATE4_ADDR = inet_addr(IPV4_PRIVATE4_ADDR); // Mask 16 Bit

STATIC_CONST size_t INET_DEFAULT_MTU_SIZE = 1492;

# ifdef HAS_EPOLL
STATIC_CONST uint32_t SOCKET_EPOLL_MASK = EPOLLIN | EPOLLHUP | EPOLLERR;
# endif


enum class ESocketType {
	ST_DGRAM = SOCK_DGRAM,
	ST_STREAM = SOCK_STREAM,
	ST_RAW = SOCK_RAW,
	ST_DEFAULT = ST_STREAM
};

enum class EAddressFamily {
	AT_UNSPEC = AF_UNSPEC,
	AT_UNIX = AF_UNIX,
	AT_INET4 = AF_INET,
	AT_INET6 = AF_INET6,
	AT_DEFAULT = AT_INET4
};


union CSocketAddress {
	struct sockaddr addr;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
};


typedef struct CAddressInfo {
	bool assigned;
	size_t ai_size;
	socklen_t ai_addrlen;		/* Length of socket address.  */
	struct sockaddr ai_addr;	/* Socket address for socket. */

	void clear() {
		assigned = false;
		ai_addrlen = ai_size;
		memset(&ai_addr, 0, ai_size);
	}

	CAddressInfo& operator = (const CAddressInfo& value) {
		assigned = true;
		ai_addrlen = value.ai_addrlen;
		memcpy(&ai_addr, &value.ai_addr, ai_size);
		return *this;
	}

	CAddressInfo() {
		ai_size = sizeof ai_addr;
		clear();
	}
} TAddressInfo;


struct CSocketConnection {
	PSocket socket;
	app::THandle server;
	app::THandle client;
	std::string remote;
	CSocketAddress addr;
	uint32_t state;
#ifdef HAS_EPOLL
	epoll_event* event;
#endif
	util::PSSLConnection ssl;

	CSocketConnection() {
		socket = nil;
		ssl = nil;
		server = INVALID_HANDLE_VALUE;
		client = INVALID_HANDLE_VALUE;
		state = 0;
#ifdef HAS_EPOLL
		event = nil;
#endif
	}
	~CSocketConnection() {
#ifdef HAS_EPOLL
		util::freeAndNil(event);
#endif
	}
};


inline int socketTypeToInet(const ESocketType type) {
	return static_cast<int>(type);
//	int retVal = SOCK_RAW;
//	switch (type) {
//		case ESocketType::ST_DGRAM:
//			retVal = SOCK_DGRAM;
//			break;
//		case ESocketType::ST_STREAM:
//			retVal = SOCK_STREAM;
//			break;
//		default:
//			break;
//	}
//	return retVal;
}

inline int socketTypeToProto(const ESocketType type) {
	if (type == inet::ESocketType::ST_DGRAM)
		return IPPROTO_UDP;
	return IPPROTO_TCP;
//	switch (type) {
//		default:
//		case inet::ESocketType::ST_STREAM:
//			return IPPROTO_TCP;
//			break;
//		case inet::ESocketType::ST_DGRAM:
//			return IPPROTO_UDP;
//			break;
//	}
}

inline ESocketType inetToSocketType(const int type) {
	if (util::isMemberOf(static_cast<__socket_type>(type), SOCK_RAW, SOCK_DGRAM, SOCK_STREAM))
		return static_cast<ESocketType>(type);
	throw util::app_error("Unsupported socket type (" + std::to_string((size_s)type) + ")");
}


inline int addressFamilyToInet(const EAddressFamily family) {
	return static_cast<int>(family);
//	int retVal = AF_UNSPEC;
//	switch (family) {
//		case EAddressFamily::AT_UNIX:
//			retVal = AF_UNIX;
//			break;
//		case EAddressFamily::AT_INET:
//			retVal = AF_INET;
//			break;
//		case EAddressFamily::AT_INET6:
//			retVal = AF_INET6;
//			break;
//		default:
//			break;
//	}
//	return retVal;
}

inline EAddressFamily inetToAddressFamily(const int family) {
	if (util::isMemberOf(family, AF_UNSPEC, AF_UNIX, AF_INET, AF_INET6))
		return static_cast<EAddressFamily>(family);
	throw util::app_error("Unsupported address family (" + std::to_string((size_s)family) + ")");
}


} /* namespace inet */

#endif /* SOCKETTYPES_H_ */
