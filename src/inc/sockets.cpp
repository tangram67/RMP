/*
 * socket.cpp
 *
 *  Created on: 23.04.2016
 *      Author: Dirk Brinkmeier
 */

#include <unistd.h>
#include <sys/un.h>
#include <openssl/ssl.h>
#include "exception.h"
#include "stringutils.h"
#include "sslconsts.h"
#include "ssltypes.h"
#include "templates.h"
#include "compare.h"
#include "sockets.h"
#include "ansi.h"
#include "ssl.h"


namespace inet {

static bool execIPv6 = false;
static bool compIPv6 = false;


static int sslVerifyCallback(int verified, X509_STORE_CTX *ctx) {
	// Ignore root CA error for TLS connection
	if (verified <= 0)
		return 1;

	SSL * ssl = nil;
	X509 * cert = nil;
	PSocket socket = nil;
	int error, depth;
	std::string issuer;
	bool ok = false;

	error = X509_STORE_CTX_get_error(ctx);
	depth = X509_STORE_CTX_get_error_depth(ctx);

	// Retrieve certificate information for current context
	cert = X509_STORE_CTX_get_current_cert(ctx);
	if (util::assigned(cert)) {

		// Retrieve issuer string
		char * p = X509_NAME_oneline(X509_get_issuer_name(cert), nil, 0);
		issuer = util::charToStr(p, "<none>");
		if (util::assigned(p))
			OPENSSL_free(p);

		// Retrieve the pointer to the SSL of the connection currently treated
		// and the application specific data stored into the SSL object.
		if (util::assigned(cert))
			ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
		if (util::assigned(ssl))
			socket = static_cast<PSocket>(SSL_get_ex_data(ssl, util::SSLInit.getIndex()));
		if(util::assigned(socket)) {
			verified = socket->verify(verified, ctx, ssl, cert, issuer, depth, error);
			ok = true;
		}

	}

	if (!ok)
		verified = 0;

	return verified;
}


/*
 * Local signal save wrapper for system calls
 */
app::THandle __s_socket (int domain, int type, int protocol) {
	app::THandle r;
	do {
		errno = EXIT_SUCCESS;
		r = ::socket(domain, type, protocol);
	} while (r == EXIT_ERROR && errno == EINTR);
	return r;
}

int __s_close(app::THandle hnd) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::close(hnd);
	} while (r == EXIT_ERROR && errno == EINTR);
	return r;
}

int __s_fcntl(app::THandle fd, int flags) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::fcntl(fd, F_SETFL, flags);
	} while (r == EXIT_ERROR && errno == EINTR);
	return r;
}


bool hasIPv6() {
	if (!execIPv6) {
		int hnd = __s_socket(AF_INET6, SOCK_STREAM, 0);
		if (hnd >= 0) {
			__s_close(hnd);
			compIPv6 = true;
		}
		execIPv6 = true;
	}
	return compIPv6;
}


bool isIPv6Address(const std::string& addr) {
	if (!addr.empty()) {
		char c;
		size_t seperators = 0;
		for (size_t i=0; i<addr.size(); ++i) {
			c = addr[i];
			if (c == ':') {
				++seperators;
			} else {
				if (!util::isHexaDecimal(c)) {
					return false;
				}
			}
		}
		return (seperators > 1);
	}
	return false;
}


bool isPrivateIPv4AddressRange(const std::string& addr) {
	if (!addr.empty()) {
		in_addr_t ip_addr = 0;
	    if (inet4StrToAddr(addr, ip_addr)) {
	    	// Optimization: Use precalculated network mask...
			//	if (isMemberOfIPv4AddressRange(ip_addr, INET4_PRIVATE1_ADDR, INET4_PRIVATE1_MASK_SIZE))
			//		return true;
			//	if (isMemberOfIPv4AddressRange(ip_addr, INET4_PRIVATE2_ADDR, INET4_PRIVATE2_MASK_SIZE))
			//		return true;
			//	if (isMemberOfIPv4AddressRange(ip_addr, INET4_PRIVATE3_ADDR, INET4_PRIVATE3_MASK_SIZE))
			//		return true;
			//	if (isMemberOfIPv4AddressRange(ip_addr, INET4_PRIVATE4_ADDR, INET4_PRIVATE4_MASK_SIZE))
			//	return true;

			in_addr_t net_addr = ntohl(ip_addr);

			// 127.0.0.0/8
			// Lower limit = 2130706432 Upper limit = 2147483647
	    	if (net_addr >= 2130706432 &&
	    		net_addr <= 2147483647)
	            return true;

			// 10.0.0.0/8
	    	// Lower limit = 167772160 Upper limit = 184549375
	    	if (net_addr >= 167772160 &&
	    		net_addr <= 184549375)
	            return true;

			// 172.16.0.0/12
	    	// Lower limit = 2886729728 Upper limit = 2887778303
	    	if (net_addr >= 2886729728 &&
	    		net_addr <= 2887778303)
	            return true;

			// 192.168.0.0/16
	    	// Lower limit = 3232235520 Upper limit = 3232301055
	    	if (net_addr >= 3232235520 &&
	    		net_addr <= 3232301055)
	            return true;
	    }
	}
	return false;
}


bool isMemberOfIPv4HostRange(const in_addr_t addr, const in_addr_t network, const in_addr_t mask) {
	// Addresses must be host byte ordered
    if (0 == addr)
    	return false;
    if (0 == network)
    	return false;
    if (0 == mask)
    	return false;

    in_addr_t net_lower = (network & mask);
    in_addr_t net_upper = (net_lower | (~mask));

	if (addr >= net_lower &&
        addr <= net_upper)
        return true;

    return false;
}

bool isMemberOfIPv4AddressMask(const in_addr_t addr, const in_addr_t network, const in_addr_t mask) {
	in_addr_t ip_addr = ntohl(addr);
	in_addr_t network_addr = ntohl(network);
	in_addr_t mask_addr = ntohl(mask);

	return isMemberOfIPv4HostRange(ip_addr, network_addr, mask_addr);
}

bool isMemberOfIPv4AddressRange(const in_addr_t addr, const in_addr_t network, const in_addr_t bits) {
	in_addr_t ip_addr = ntohl(addr);
	in_addr_t network_addr = ntohl(network);
	in_addr_t mask_addr = inet4BitsToHostOrderMask(bits);

	return isMemberOfIPv4HostRange(ip_addr, network_addr, mask_addr);
}

bool isMemberOfIPv4AddressRange(const std::string addr, const std::string network, const std::string mask) {
	in_addr_t ip_addr = 0;
	in_addr_t network_addr = 0;
	in_addr_t mask_addr = 0;

    if (!inet4StrToAddr(addr, ip_addr))
    	return false;
    if (!inet4StrToAddr(network, network_addr))
    	return false;
    if (!inet4StrToAddr(mask, mask_addr))
    	return false;

    return isMemberOfIPv4AddressMask(ip_addr, network_addr, mask_addr);
}


in_addr_t inet4BitsToHostOrderMask(const in_addr_t bits) {
	if (bits < (in_addr_t)32)
		return ((in_addr_t)(-1)) << ((in_addr_t)32 - bits);
	if (bits == (in_addr_t)32)
		return ((in_addr_t)(-1));
	return (in_addr_t)0;
}

in_addr_t inet4BitsToNetworkOrderMask(const in_addr_t bits) {
	return htonl(inet4BitsToHostOrderMask(bits));
}


bool inet4CompareAddress(const in_addr_t addr1, const in_addr_t addr2, const in_addr_t bits) {
	in_addr_t mask_addr = inet4BitsToHostOrderMask(bits);
	in_addr_t ip_addr1 = ntohl(addr1);
	in_addr_t ip_addr2 = ntohl(addr2);
	if (mask_addr > 0)
		return (ip_addr1 && mask_addr) == (ip_addr2 && mask_addr);
	return false;
}

bool inet4CompareAddress(struct sockaddr const * const addr1, struct sockaddr const * const addr2, const in_addr_t bits) {
	if (util::assigned(addr1) && util::assigned(addr2) && bits > 0) {
		return inet4CompareAddress(((struct sockaddr_in *)addr1)->sin_addr.s_addr, ((struct sockaddr_in *)addr2)->sin_addr.s_addr, bits);
	}
	return false;
}

bool inet4StrToAddr(const char *str, in_addr_t& addr) {
	in_addr_t r = inet_addr(str);
	if (INADDR_NONE != r) {
		addr = r;
		return true;
	}
	return false;
}

bool inet4StrToAddr(const std::string& str, in_addr_t& addr) {
	if (!str.empty()) {
		return inet4StrToAddr(str.c_str(), addr);
	}
	return false;
}


std::string inetAddrToStr(const TAddrInfo& addr) {
	return inetAddrToStr(addr.ai_addr);
}

std::string inetAddrToStr(const TAddressInfo& addr) {
	return inetAddrToStr(&addr.ai_addr);
}

std::string inetAddrToStr(struct sockaddr const * const addr) {
	char retVal[INET6_ADDRSTRLEN] = { '\0' };
	const char* p;
	if(util::assigned(addr)) {
		switch(addr->sa_family) {
			case AF_INET:
				p = inet_ntop(	AF_INET,
								&(((struct sockaddr_in *)addr)->sin_addr),
								retVal,
								INET_ADDRSTRLEN );
				if (!util::assigned(p))
					strncpy(retVal, "[INVALID IPv4 ADDRESS]\0", 23);
				break;

			case AF_INET6:
				p = inet_ntop(	AF_INET6,
								&(((struct sockaddr_in6 *)addr)->sin6_addr),
								retVal,
								INET6_ADDRSTRLEN );
				if (!util::assigned(p))
					strncpy(retVal, "[INVALID IPv6 ADDRESS]\0", 23);
				break;

			default:
				strncpy(retVal, "[INVALID IPv6 ADDRESS]\0", 23);
				break;
		}
	} else
		strncpy(retVal, "[UNASSIGNED ADDRESS]\0", 21);
	return retVal;
}

int inetPortToInt(const TAddrInfo& addr) {
	return inetPortToInt(addr.ai_addr);
}

int inetPortToInt(const TAddressInfo& addr) {
	return inetPortToInt(&addr.ai_addr);
}

int inetPortToInt(struct sockaddr const * const addr) {
	if (util::assigned(addr)) {
		struct sockaddr_in* s = (struct sockaddr_in*)addr;
		return ntohs(s->sin_port);
	}
	return -1;
}


std::string getInetAddress(const std::string& host, const inet::EAddressFamily family) {
	inet::TInetAddress addr;
	addr.setFamily(family);
	std::string ip;
	if (addr.getAddress(host)) {
		ip = addr.getAddress();
	}
	return ip;
}

std::string getInet4Address(const std::string& host) {
	return getInetAddress(host, inet::EAddressFamily::AT_INET4);
}

std::string getInet6Address(const std::string& host) {
	return getInetAddress(host, inet::EAddressFamily::AT_INET6);
}


bool TInetAddress::getAddress(const std::string& host, const std::string& service) {
	const char *h = nil;
	const char *s = nil;
	if (!host.empty())
		h = host.c_str();
	if (!service.empty())
		s = service.c_str();
	return getAddress(h, s);
}


bool TInetAddress::getAddress(const char *host, const char *service) {
	// Clear everything
	addrret = EXIT_SUCCESS;
	clear();

	// Return UNIX local socket address
	if (inet::EAddressFamily::AT_UNIX == family) {

		// Device (named as host) is needed
		if (!util::assigned(host)) {
			addrret = EAI_SYSTEM;
			addrsys = ENOENT;
			return false;
		}

		// Set UNIX "socket address" as device file name
		// e.g.: "/run/lirc/lircd"
		create();
		size_t size = sizeof(struct sockaddr_un);
		struct sockaddr_un* addr = (struct sockaddr_un*)calloc(1, size);
		addr->sun_family = AF_LOCAL;
		strcpy(addr->sun_path, host);
		address()->ai_addr = (sockaddr*)addr;
		address()->ai_addrlen = SUN_LEN(addr);
		address()->ai_family = AF_UNIX;
		address()->ai_protocol = IPPROTO_IP;
		address()->ai_socktype = socketTypeToInet(type);
		address()->ai_next = nil;
		setFreeAddress(true);

		// Remember device name as host
		this->host = util::charToStr(host);
		return true;
	}

	// Initialize address info
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = addressFamilyToInet(family);
	hints.ai_socktype = socketTypeToInet(type);
	hints.ai_protocol = socketTypeToProto(type);
	bool any = !util::assigned(host);

	// Get port from service (digit, no name, expected)
	int port = 0;
	if (util::assigned(service)) {
		char* q;
		errno = EXIT_SUCCESS;
		port = strtol(service, &q, 10);
		if (EXIT_SUCCESS != errno || service == q) {
			port = -1;
		}
	}

	// Remember parameters
	this->host = util::charToStr(host);
	this->service = util::charToStr(service);

	// Is port valid?
	// Allow no service --> Port = 0
	if (port < 0) {
		addrret = EAI_SYSTEM;
		addrsys = EINVAL;
		return false;
	}

	// Get linked list for given host and service
	if (any) {
		create();
		size_t size;
		sockaddr_in* ipv4;
		sockaddr_in6* ipv6;
		switch (family) {
			default:
			case inet::EAddressFamily::AT_INET4:
				size = sizeof(sockaddr_in);
				ipv4 = (sockaddr_in*)calloc(1, size);
				ipv4->sin_addr.s_addr = INADDR_ANY;
				ipv4->sin_family = AF_INET;
				ipv4->sin_port = htons(port);
				address()->ai_addr = (sockaddr*)ipv4;
				address()->ai_addrlen = size;
				address()->ai_family = AF_INET;
				setFreeAddress(true);
				break;
			case inet::EAddressFamily::AT_INET6:
				size = sizeof(sockaddr_in6);
				ipv6 = (sockaddr_in6*)calloc(1, size);
				ipv6->sin6_addr = in6addr_any;
				ipv6->sin6_family = AF_INET6;
				ipv6->sin6_port = htons(port);
				address()->ai_addr = (sockaddr*)ipv6;
				address()->ai_addrlen = size;
				address()->ai_family = AF_INET6;
				setFreeAddress(true);
				break;
		}
		hints.ai_flags = AI_PASSIVE;
		address()->ai_flags = hints.ai_flags;
		address()->ai_socktype = hints.ai_socktype;
		address()->ai_protocol = hints.ai_protocol;
		address()->ai_next = nil;
	} else {
		do {
			errno = EXIT_SUCCESS;
			addrret = getaddrinfo(host, service, &hints, addresses());
		} while (addrret != EXIT_SUCCESS && errno == EINTR);
		addrsys = errno;
	}

	//debugOutput(hints.ai_family);
	if (EXIT_SUCCESS != addrret)
		return false;

	return valid();
}

bool TInetAddress::compare(const TAddrInfo& addr) const {
	return compare(addr.ai_addr);
}

bool TInetAddress::compare(const TAddressInfo& addr) const {
	return compare(&addr.ai_addr);
}

bool TInetAddress::compare(struct sockaddr const * const addr) const {
	struct sockaddr_in *self = (struct sockaddr_in *)address();
	struct sockaddr_in *value = (struct sockaddr_in *)addr;
	if (util::assigned(self) && util::assigned(value)) {
		if (sizeof(self) == sizeof(value) &&
			(self->sin_family == value->sin_family) &&
			(self->sin_addr.s_addr == value->sin_addr.s_addr)) {
			return true;
		}
	}
	return false;
}


std::string TInetAddress::getLastAddressInfoError() {
	if (addrret != EXIT_SUCCESS) {
		std::string text = (EAI_SYSTEM == addrret) ? sysutil::getSysErrorMessage(addrsys) : sysutil::getInetErrorMessage(addrret);
		return "inet::getAddressInfo(\"" + getHost() + "\",\"" + getService() + "\") failed. [" +
			text + " (" + std::to_string((size_s)addrret) + ")]";
	}
	return sysutil::getSysErrorMessage(EXIT_SUCCESS);
}

void TInetAddress::debugOutput(const int family) {
	// Walk through result set
	if (valid()) {
		for(struct addrinfo *p = address(); util::assigned(p); p = p->ai_next) {
			if (p->ai_family & family) {
				std::string ip = inetAddrToStr(p->ai_addr);
				if (host.empty())
					std::cout << app::yellow << "Address for <any> on port <" << service << "> : " << ip << app::reset << std::endl;
				else
					std::cout << app::yellow << "Address for \"" << host << "\" on port <" << service << "> : " << ip << app::reset << std::endl;
			}
		}
	} else {
		if (host.empty())
			std::cout << app::red << "No valid IP address for <any> on port <" << service << ">" << app::reset << std::endl;
		else
			std::cout << app::red << "No valid IP address for \"" << host << "\" on port <" << service << ">" << app::reset << std::endl;
	}

}


std::string TInetAddress::getAddress() {
	// Walk through result set
	if (valid()) {
		for(struct addrinfo *p = address(); util::assigned(p); p = p->ai_next) {
			if (p->ai_family & (int)family) {
				std::string ip = inetAddrToStr(p->ai_addr);
				if (!ip.empty())
					return ip;
			}
		}
	}
	return "";
}




TBaseSocket::TBaseSocket(PSocketController owner, const bool debug) : TInetAddress(), util::TFileHandle() {
	prime();
	this->debug = debug;
	this->owner = owner;
}

TBaseSocket::TBaseSocket(const bool debug) : TInetAddress(), util::TFileHandle() {
	prime();
	this->debug = debug;
}

TBaseSocket::TBaseSocket() : TInetAddress(), util::TFileHandle() {
	prime();
	this->debug = false;
}

TBaseSocket::~TBaseSocket() {
	close();
}

void TBaseSocket::prime() {
	reset();
	port = -1;
	conncnt = 0;
	self.socket = nil;
	debug = false;
	owner = nil;
	config = nil;
	logger = nil;
	recval = EXIT_SUCCESS;
    sndval = EXIT_SUCCESS;
    id = util::randomize(1, util::TLimits::LIMIT_INT32_MAX);
}

void TBaseSocket::reset() {
	edge = false;
	dualstack = false;
	keepalive = true;
	blocking = false;
	listening = false;
	connected = false;
	listener = false;
	connector = false;
	secure = false;
	descOK = false;
	local = false;
	udp = false;
	desc.clear();
}

const std::string& TBaseSocket::getDescription() const {
	if (!descOK) {
		const std::string& host = getHost();
		const std::string& service = getService();
		if (isUnix()) {
			if (!host.empty()) {
				desc = util::quote(host);
				descOK = true;
			}
		} else {
			if (!host.empty() || !service.empty()) {
				desc = (host.empty() ? "\"any\"" : util::quote(host)) + "," + (service.empty() ? "\"none\"" : util::quote(service));
				descOK = true;
			}
		}
		if (!descOK) {
			desc = util::quote("invalid");
		}
	}
	return desc;
}


int TBaseSocket::open(const std::string& host, const int port, const ESocketType type, const EAddressFamily family) {
	std::string s = util::csnprintf("%", port);
	this->port = port;
	return open(host, s, type, family);
}

int TBaseSocket::open(const std::string& host, const std::string& service, const ESocketType type, const EAddressFamily family) {
	app::THandle fd = INVALID_HANDLE_VALUE;
	app::THandle hnd = handle();

	if (isOpen()) {
		throw util::app_error_fmt("TBaseSocket::open($,$) is already open.", host, service);
	}

	// Service == port number
	// this->service = service;

	// Get address information
	setType(type);
	setFamily(family);

	// Use stored address information if possible
	// TInetAddress::clear();
	if (!util::assigned(address())) {
		if (!getAddress(host, service)) {
			errval = errno;
			throw util::app_error_fmt("TBaseSocket::open($,$) failed: %", host, service, getLastAddressInfoError());
		}
	}

	// Check for UNIX server socket file
	if (inet::EAddressFamily::AT_UNIX == family && isServer() && !host.empty()) {
		if (util::fileExists(host))
			throw util::app_error_fmt("TBaseSocket::open($) Socket file already in use.", host);
	}

	// Open the socket file descriptor
	fd = __s_socket(address()->ai_family, address()->ai_socktype, address()->ai_protocol);
	if (fd < 0) {
		errval = errno;
		throw util::sys_error_fmt("TBaseSocket::open($,$) failed.", host, service);
	}

	assign(fd);
	setSelf(fd);
	if (hnd != handle())
		change();

	writeLogFmt("TBaseSocket::open(%) Socket <%> opened successfully.", getDescription(), fd);
	return fd;
}


bool TBaseSocket::upgrade(const app::THandle socket) {

	// Upgraded socket should be already opened by upgrade handling of webserver
	if (isUpgraded()) {
		assign(socket);
		if (isOpen()) {
			setSelf(socket);
			assign(socket);
			writeLogFmt("TBaseSocket::upgrade() Client socket <%> upgraded.", handle());
			return true;
		} else {
			assign(INVALID_HANDLE_VALUE);
			writeLogFmt("TBaseSocket::upgrade() failed: Client socket <%> is closed.", handle());
		}
	}

	return false;
}


void TBaseSocket::close() {
	if (isOpen()) {
		app::THandle hnd = handle();
		disconnect();
		if (debug && 0 < handle())
			writeLogFmt("TBaseSocket::close(%), closed socket <%>", getDescription(), handle());

		// Close socket file descriptor
		TFileHandle::close();

		// Unlink server socket file for local UNIX sockets
		bool failed = false;
		if (inet::EAddressFamily::AT_UNIX == getFamily() && isServer()) {
			std::cout << "TBaseSocket::close() Unlink UNIX socket file." << std::endl;
			const std::string& host = getHost();
			if (!host.empty()) {
				int r = unlink(host.c_str());
				if (r != EXIT_SUCCESS && errno != ENOENT) {
					errval = errno;
					failed = true;
				}
			}
		}

		// TInetAddress::clear();
		setSelf(handle());
		reset();
		if (hnd != handle())
			change();

		// Check for UNIX server socket unlink failure
		if (failed)
			throw util::sys_error_fmt("TBaseSocket::close($) failed on unlink local socket file.", getHost());

	}
}

void TBaseSocket::disconnect() {
	app::TLockGuard<app::TMutex> lock(clientMtx);
	if (!clients.empty()) {
		TConnectionMap::const_iterator it = clients.begin();
		while (it != clients.end()) {
			PSocketConnection o = it->second;
			if (util::assigned(o))
				__s_close(o->client);
			// Do debug output with o->... before destroying object!
			writeLogFmt("TBaseSocket::disconnect() Client <%> closed.", o->client);
			util::freeAndNil(o);
			it++;
		}
		clients.clear();
	}
}

void TBaseSocket::drop(const app::THandle client) {
	app::TLockGuard<app::TMutex> lock(clientMtx);
	TConnectionMap::const_iterator it = clients.find(client);
	if (it != clients.end()) {
		__s_close(client);
		clients.erase(it);
		writeLogFmt("TBaseSocket::drop() Client <%> dropped.", client);
	}
}

void TBaseSocket::shutdown(int how) {
	// SHUT_RD   = No more receptions;
	// SHUT_WR   = No more transmissions;
	// SHUT_RDWR = No more receptions or transmissions.
	if (isOpen()) {
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = ::shutdown(handle(), how);
		} while (r == EXIT_ERROR && errno == EINTR);
	    errval = errno;
	}
}


void TBaseSocket::setSelf(const app::THandle socket) {
	self.server = socket;
	self.client = socket;
}


bool TBaseSocket::setOption(const int option, const bool value) {
	errval = errno = ENOENT;
	if (isOpen()) {
		int optval = value ? 1 : 0;
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = setsockopt(handle(), SOL_SOCKET, option, &optval, sizeof optval);
		} while (r == EXIT_ERROR && errno == EINTR);
	    errval = errno;
		if (r == EXIT_SUCCESS)
			return true;
	}
	return false;
}

bool TBaseSocket::getOption(const int option, bool& value) {
	// Example options: TCP_KEEPCNT, TCP_KEEPIDLE TCP_KEEPINTVL
	errval = errno = ENOENT;
	if (isOpen()) {
		int optval = 0;
		unsigned int optlen = sizeof optval;
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = getsockopt(handle(), SOL_SOCKET, option, &optval, &optlen);
		} while (r == EXIT_ERROR && errno == EINTR);
	    errval = errno;
		if (r == EXIT_SUCCESS) {
			value = (optval > 0);
			return true;
		}
	}
	return false;
}


bool TBaseSocket::setIPv6OnlyOption() {
	errval = errno = ENOENT;
	if (isOpen()) {
		int optval = 1;
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = setsockopt(handle(), IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof optval);
		} while (r == EXIT_ERROR && errno == EINTR);
	    errval = errno;
		if (r == EXIT_SUCCESS)
			return true;
	}
	return false;
}


bool TBaseSocket::bind() {
	if (!isOpen()) {
		throw util::app_error_fmt("TBaseSocket::bind(%) failed, socket not open.", getDescription());
	}

	// Allow socket descriptor to be reuseable
	// This avoid "Address already in use" error message for standard sockets
	if (!isUnix()) {
		if (!setOption(SO_REUSEADDR)) {
			close();
			throw util::sys_error_fmt("TBaseSocket::bind::setOption(%) failed: Invalid socket options.", getDescription());
		}
	}

	// Allow socket to bind to IPv6 only
	// This allows/forces use of dual socket support for IPv4 mixed with IPv6
	if (!dualstack && isFamily6()) {
		if (!setIPv6OnlyOption()) {
			close();
			throw util::sys_error_fmt("TBaseSocket::bind::setIPv6OnlyOption(%) failed: Invalid socket options.", getDescription());
		}
	}

	// Set socket to be nonblocking. All of the sockets for
	// the incoming connections will also be nonblocking since
	// they will inherit that state from the listening socket
	if (!blocking) {
		if (!control(O_NONBLOCK)) {
			close();
			throw util::sys_error_fmt("TBaseSocket::bind::setNonblocking(%) failed.", getDescription());
		}
	}

	// Bind socket file descriptor to given port
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::bind(handle(), address()->ai_addr, address()->ai_addrlen);
	} while (r == EXIT_ERROR && errno == EINTR);
    errval = errno;

    if (EXIT_SUCCESS != r) {
    	r = errval;
    	std::string msg = sysutil::getSysErrorMessage(errval);
		close();
		throw util::app_error_fmt("TBaseSocket::bind(%) failed (%) $", getDescription(), r, msg);
	}

    writeLogFmt("TBaseSocket::bind(%) Socket <%> successfully bound.", getDescription(), handle());
	return true;
}


bool TBaseSocket::listen() {
	if (!isOpen()) {
		throw util::app_error(util::csnprintf("TBaseSocket::listen(%) failed, socket not open.", getDescription()));
	}

	// Bind socket file descriptor to given port
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::listen(handle(), MAX_SOCK_WAIT);
	} while (r == EXIT_ERROR && errno == EINTR);
    errval = errno;

    if (EXIT_SUCCESS != r) {
    	r = errval;
    	std::string msg = sysutil::getSysErrorMessage(errval);
		close();
		throw util::app_error_fmt("TBaseSocket::listen(%) failed (%) $", getDescription(), r, msg);
	}

    writeLogFmt("TBaseSocket::listen(%) Socket <%> listening on port <%>", getDescription(), handle(), isUnix() ? getHost() : getService());
	listening = true;
	return true;
}


bool TBaseSocket::connect() {
	if (!isOpen()) {
		throw util::app_error_fmt("TBaseSocket::connect(%) failed, socket not open.", getDescription());
	}
	if (getHost().empty()) {
		throw util::app_error_fmt("TBaseSocket::connect(%) failed, undefined host.", getDescription());
	}

	// Bind socket file descriptor to given host/port
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::connect(handle(), address()->ai_addr, address()->ai_addrlen);
	} while (r == EXIT_ERROR && errno == EINTR);
    errval = errno;

	if (EXIT_SUCCESS == r) {
		writeLogFmt("TBaseSocket::connect(%) Connection established.", getDescription());
		connected = true;
		++conncnt;
		change();
		return true;
	}

	std::string msg = sysutil::getSysErrorMessage(errval);
	errorLogFmt("TBaseSocket::connect(%) Connection failed (%) $", getDescription(), r, msg);
	return false;
}


app::THandle TBaseSocket::accept(struct sockaddr * from, socklen_t * fromlen, int const flags) const {
	app::THandle fd = EXIT_ERROR;

	// Accept incoming connections on stream type (!) socket
	do {
		errno = EXIT_SUCCESS;
		if (flags > 0)
			// Implied setting of SOCK_NONBLOCK and/or SOCK_CLOEXEC
			// a.k.a. setting O_NONBLOCK and/or O_CLOEXEC via fcntl()
			fd = ::accept4(handle(), from, fromlen, flags);
		else
			fd = ::accept(handle(), from, fromlen);
	} while (fd == EXIT_ERROR && errno == EINTR);
    errval = errno;

	if (fd < 0 && errno != EWOULDBLOCK) {
		throw util::sys_error_fmt("TBaseSocket::accept(%) failed, handle = %", getDescription(), fd);
	}

	if (fd >= 0) {
		if (util::assigned(from)) {
			writeLogFmt("TBaseSocket::accept(%) Connection from client [%] accepted, handle = %", getDescription(), inetAddrToStr(from), fd);
		} else {
			writeLogFmt("TBaseSocket::accept(%) Connection from client accepted, handle = %", getDescription(), fd);
		}
	} else {
		writeLogFmt("TBaseSocket::accept(%) No more pending connections for socket <%>", getDescription(), handle());
	}

	return fd;
}


ssize_t TBaseSocket::receive(const app::THandle hnd, void * const data, size_t const size, int const flags) const
{
	ssize_t r;

	// Receive data from stream type TCP socket:
	//	MSG_OOB		Receive Out of Band data. This is how to get data that has been sent to
	//				you with the MSG_OOB flag in send(). As the receiving side, you will
	//				have had signal SIGURG raised telling you there is urgent data. In your
	//				handler for that signal, you could call recv() with this MSG_OOB flag.
	//	MSG_PEEK 	If you want to call recv() “just for pretend”, you can call it with this
	//				flag. This will tell you what's waiting in the buffer for when you call
	//				recv() “for real” (i.e. without the MSG_PEEK flag. It's like a sneak
	//				preview into the next recv() call.
	//	MSG_WAITALL	Tell recv() to not return until all the data you specified in the len
	//				parameter. It will ignore your wishes in extreme circumstances, however,
	//				like if a signal interrupts the call or if some error occurs or if the remote
	//				side closes the connection, etc. Don't be mad with it.
    do {
		errno = EXIT_SUCCESS;
        r = recv(hnd, data, size, flags);
    } while (r == (ssize_t)EXIT_ERROR && errno == EINTR);
    errval = errno;
    recval = errno;

    // Read some bytes...
    if (r >= (ssize_t)0) {
    	return r;
    }

    // Ignore error result of read() if errno not set
    if (r == (ssize_t)EXIT_ERROR) {
   		// Return 0 on EWOULDBLOCK a.k.a. EAGAIN
    	if (errno == EWOULDBLOCK)
   			return (ssize_t)0;
		return (ssize_t)EXIT_ERROR;
    }

    // Invalid result of ::read()
    // Should not happen by design!
	if (EXIT_SUCCESS == errno)
		errno = EIO;

	return (ssize_t)EXIT_ERROR;
}

ssize_t TBaseSocket::receive(void * const data, size_t const size, int const flags) const
{
	return receive(handle(), data, size, flags);
}

ssize_t TBaseSocket::receiveFrom(void * const data, size_t const size, struct sockaddr * const from, socklen_t * const fromlen, int const flags) const
{
	ssize_t r;

	// Receive data from datagram type UDP socket:
	//	MSG_PEEK 	If you want to call recv() “just for pretend”, you can call it with this
	//				flag. This will tell you what's waiting in the buffer for when you call
	//				recv() “for real” (i.e. without the MSG_PEEK flag. It's like a sneak
	//				preview into the next recv() call.
	//	MSG_WAITALL	Tell recv() to not return until all the data you specified in the len
	//				parameter. It will ignore your wishes in extreme circumstances, however,
	//				like if a signal interrupts the call or if some error occurs or if the remote
	//				side closes the connection, etc. Don't be mad with it.
    do {
		errno = EXIT_SUCCESS;
        r = recvfrom(handle(), data, size, flags, from, fromlen);
    } while (r == (ssize_t)EXIT_ERROR && errno == EINTR);
    errval = errno;
    recval = errno;

    // Read some bytes...
    if (r >= (ssize_t)0) {
    	return r;
    }

    // Ignore error result of read() if errno not set
    if (r == (ssize_t)EXIT_ERROR) {
    	if (errno != EXIT_SUCCESS)
    		return (ssize_t)EXIT_ERROR;
    	else
    		return (ssize_t)0;
    }

    // Invalid result of ::read()
    // Should not happen by design!
	if (EXIT_SUCCESS == errno)
		errno = EIO;

	return (ssize_t)EXIT_ERROR;
}


ssize_t TBaseSocket::send(const app::THandle hnd, void const * const data, size_t const size, int const flags) const
{
    char const *p = (char const *)data;
    char const *const q = (char const *)data + size;
    ssize_t r;

	// Send data to stream type TCP socket:
	//	MSG_OOB 		Send as “out of band” data. TCP supports this, and it's a way to tell the
	//					receiving system that this data has a higher priority than the normal data.
	//					The receiver will receive the signal SIGURG and it can then receive this
	//					data without first receiving all the rest of the normal data in the queue.
	//	MSG_DONTROUTE	Don't send this data over a router, just keep it local.
	//	MSG_DONTWAIT	If send() would block because outbound traffic is clogged, have it
	//					return EAGAIN. This is like a “enable non-blocking just for this send.”
	//					See the section on blocking for more details.
	//	MSG_NOSIGNAL	If you send() to a remote host which is no longer recv()ing, you'll
	//					typically get the signal SIGPIPE. Adding this flag prevents that signal
	//					from being raised.
    while (p < q) {
    	do {
    		errno = EXIT_SUCCESS;
    		r = ::send(hnd, p, (size_t)(q - p), flags);
    	} while (r == (ssize_t)EXIT_ERROR && errno == EINTR);
        errval = errno;
        sndval = errno;

    	// Write failed
    	if (r == (ssize_t)EXIT_ERROR)
        	return (ssize_t)EXIT_ERROR;

		// Something else went wrong
    	if (r < (ssize_t)1) {
    		if (EXIT_SUCCESS == errno)
    			errno = EIO;
        	return (ssize_t)EXIT_ERROR;
    	}

    	p += (size_t)r;
    }

    // Unexpected pointer after data transfer
    if (p != q) {
		if (EXIT_SUCCESS == errno)
			errno = EFAULT;
    	return (ssize_t)EXIT_ERROR;
    }

    // Buffer has been fully written
    // Possible overflow on result value (ssize_t)size !!!
    return (ssize_t)size;
}

ssize_t TBaseSocket::send(void const * const data, size_t const size, int const flags) const
{
	return send(handle(), data, size, flags);
}


ssize_t TBaseSocket::sendTo(void const * const data, size_t const size, struct sockaddr const * const to, socklen_t const tolen, int const flags) const
{
    char const *p = (char const *)data;
    char const *const q = (char const *)data + size;
    ssize_t r;

	// Send data to datagram type UDP socket:
	//	MSG_DONTROUTE	Don't send this data over a router, just keep it local.
	//	MSG_DONTWAIT	If send() would block because outbound traffic is clogged, have it
	//					return EAGAIN. This is like a “enable non-blocking just for this send.”
	//					See the section on blocking for more details.
	//	MSG_NOSIGNAL	If you send() to a remote host which is no longer recv()ing, you'll
	//					typically get the signal SIGPIPE. Adding this flag prevents that signal
	//					from being raised.
    while (p < q) {
    	do {
    		errno = EXIT_SUCCESS;
    		r = ::sendto(handle(), p, (size_t)(q - p), flags, to, tolen);
    	} while (r == (ssize_t)EXIT_ERROR && errno == EINTR);
        errval = errno;
        sndval = errno;

    	// Write failed
    	if (r == (ssize_t)EXIT_ERROR)
        	return (ssize_t)EXIT_ERROR;

		// Something else went wrong
    	if (r < (ssize_t)1) {
    		if (EXIT_SUCCESS == errno)
    			errno = EIO;
        	return (ssize_t)EXIT_ERROR;
    	}

    	p += (size_t)r;
    }

    // Unexpected pointer after data transfer
    if (p != q) {
		if (EXIT_SUCCESS == errno)
			errno = EFAULT;
    	return (ssize_t)EXIT_ERROR;
    }

    // Buffer has been fully written
    // Possible overflow on result value (ssize_t)size !!!
    return (ssize_t)size;
}


PSocketConnection TBaseSocket::addClient(const app::THandle client) {
	PSocketConnection c = new CSocketConnection;
	c->client = client;
	return addClient(c);
}

PSocketConnection TBaseSocket::addClient(const app::THandle client, const std::string& remote) {
	PSocketConnection c = new CSocketConnection;
	c->client = client;
	c->remote = remote;
	return addClient(c);
}

PSocketConnection TBaseSocket::addClient(PSocketConnection connection) {
	app::TLockGuard<app::TMutex> lock(clientMtx);
	connection->server = handle();
	clients[connection->client] = connection;
	return connection;
}

void TBaseSocket::removeClient(const app::THandle client) {
	app::TLockGuard<app::TMutex> lock(clientMtx);
	TConnectionMap::iterator it = clients.find(client);
	if (it != clients.end()) {
		util::freeAndNil(it->second);
		clients.erase(it);
		if (debug) std::cout << "TBaseSocket::removeClient() Client <" << client << "> removed from client list." << std::endl;
	}
	__s_close(client);
	if (debug) std::cout << "TBaseSocket::removeClient() Client <" << client << "> closed." << std::endl;
}

bool TBaseSocket::hasClients() const {
	app::TLockGuard<app::TMutex> lock(clientMtx);
	return !clients.empty();
}

bool TBaseSocket::hasClient(const app::THandle client) const {
	app::TLockGuard<app::TMutex> lock(clientMtx);
	if (!clients.empty()) {
		TConnectionMap::const_iterator it = clients.find(client);
		if (it != clients.end())
			return true;
	}
	return false;
}

template<typename value_t, typename... variadic_t>
void TBaseSocket::writeLogFmt(const std::string& str, const value_t value, variadic_t... args) const {
	writeLog(util::csnprintf(str, value, std::forward<variadic_t>(args)...));
}

template<typename value_t, typename... variadic_t>
void TBaseSocket::errorLogFmt(const std::string& str, const value_t value, variadic_t... args) const {
	errorLog(util::csnprintf(str, value, std::forward<variadic_t>(args)...));
}

void TBaseSocket::writeLog(const std::string& s) const {
	if (getDebug()) std::cout << s << std::endl;
	if (util::assigned(logger)) {
		logger->write("[" + getName() + "] " + s);
	}
}

void TBaseSocket::errorLog(const std::string& s) const {
	if (getDebug()) std::cout << s << std::endl;
	if (util::assigned(logger)) {
		logger->write("[Error] [" + getName() + "] " + s);
	}
}


void TBaseSocket::readConfig(std::string& host, int& port, const inet::EAddressFamily family)
{
	if (util::assigned(config) && !name.empty()) {
		config->setSection(name);
		std::string _host;
		if (EAddressFamily::AT_UNIX != family) {
			_host = config->readString("Host", host);
			port = config->readInteger("Port", port);
			keepalive = config->readBool("KeepAlive", keepalive);
		} else {
			_host = config->readString("Device", host);
			keepalive = false;
			port = 0;
		}
		debug = config->readBool("Debug", debug);

		// Is IPv6 dual stack socket?
		if (inet::EAddressFamily::AT_INET6 == family) {
			dualstack = config->readBool("DualStack", dualstack);
		}

		// Bind to any local address?
		// --> empty host forces AI_PASSIVE to be used...
		if (0 == util::strncasecmp(_host, IPV4ANY, 7) || 0 == util::strncasecmp(_host, IPV6ANY, 2) || 0 == util::strncasecmp(_host, "any", 3)) {
			host.clear();
		} else {
			host = _host;
		}
	}
}

void TBaseSocket::writeConfig(const std::string& host, const int port, const inet::EAddressFamily family)
{
	if (util::assigned(config) && !name.empty()) {

		// Add acronym for any host address
		std::string _host = host;
		if (host.empty()) {
			switch (family) {
				case inet::EAddressFamily::AT_INET4:
					_host = IPV4ANY;
					break;
				case inet::EAddressFamily::AT_INET6:
					_host = IPV6ANY;
					break;
				default:
					break;
			}
		}

		config->setSection(name);
		if (EAddressFamily::AT_UNIX != family) {
			config->writeString("Host", (_host == IPV4ANY || _host == IPV6ANY) ? "any" : _host);
			config->writeInteger("Port", port);
			config->writeBool("KeepAlive", keepalive, app::INI_BLYES);
		} else {
			config->writeString("Device", _host);

		}
		config->writeBool("Debug", debug, app::INI_BLYES);

		// Is IPv6 dual stack socket?
		if (inet::EAddressFamily::AT_INET6 == family) {
			config->writeBool("DualStack", dualstack, app::INI_BLYES);
		}
	}
}

void TBaseSocket::reWriteConfig(std::string& host, int& port, const inet::EAddressFamily family)
{
	readConfig(host, port, family);
	writeConfig(host, port, family);
}




TSocket::TSocket(PSocketController owner, const bool debug) {
	prime();
	socket.setOwner(owner);
	socket.setDebug(debug);
}

TSocket::TSocket(const bool debug) {
	prime();
	socket.setDebug(debug);
}

TSocket::TSocket() {
	prime();
}

TSocket::~TSocket() {
	close();
}

void TSocket::prime() {
	onSocketConnect = nil;
	onSocketClose = nil;
	socket.setSocket(this);
	setTime();
}

inline app::THandle TSocket::accept(struct sockaddr * from, socklen_t * fromlen, int const flags) const {
	return socket.accept(from, fromlen, flags);
}

void TSocket::reconnect() {
	if (util::assigned(owner) && !isConnected()) {
		if (isOpen() && wasConnected())
			close();
		owner->addReconnectSocket(this);
	}
}

PSocketConnection TSocket::addClient(const app::THandle client) {
	PSocketConnection conn = socket.addClient(client);
	conn->socket = this;
	executeConnectMethod(client);
	return conn;
}

PSocketConnection TSocket::addClient(const app::THandle client, const std::string& remote) {
	PSocketConnection conn = socket.addClient(client, remote);
	conn->socket = this;
	executeConnectMethod(client);
	return conn;
}

PSocketConnection TSocket::addClient(PSocketConnection connection) {
	PSocketConnection conn = socket.addClient(connection);
	conn->socket = this;
	executeConnectMethod(conn->client);
	return conn;
}

void TSocket::removeClient(const app::THandle client) {
	executeCloseMethod(client);
	socket.removeClient(client);
}

void TSocket::executeConnectMethod(app::THandle client) {
	try {
		if (onSocketConnect != nil)
			return onSocketConnect(*this, client);
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		socket.errorLogFmt("TSocket::executeConnectMethod() Failed for client socket <%> $", client, sExcept);
	} catch (...)	{
		socket.errorLogFmt("TSocket::executeConnectMethod() Failed for client socket <%> on unknown exception.", client);
	}
}

void TSocket::executeCloseMethod(app::THandle client) {
	try {
		if (onSocketClose != nil)
			return onSocketClose(*this, client);
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		socket.errorLogFmt("TSocket::executeCloseMethod() Failed for client socket <%> $", client, sExcept);
	} catch (...)	{
		socket.errorLogFmt("TSocket::executeCloseMethod() Failed for client socket <%> on unknown exception.", client);
	}
}

template<typename value_t, typename... variadic_t>
void TSocket::writeLogFmt(const std::string& str, const value_t value, variadic_t... args) const {
	std::string s = util::csnprintf(str, value, std::forward<variadic_t>(args)...);
	socket.writeLog(s);
}

template<typename value_t, typename... variadic_t>
void TSocket::errorLogFmt(const std::string& str, const value_t value, variadic_t... args) const {
	std::string s = util::csnprintf(str, value, std::forward<variadic_t>(args)...);
	socket.errorLog(s);
}



TSocketController::TSocketController(app::PThreadController threads, app::PTimerController timers, app::PLogFile logger, const std::string& configFolder) {
	prime();
	this->threads = threads;
	this->timers  = timers;
	this->logger  = logger;
	this->configFolder = configFolder;
	this->configFile = this->configFolder + "sockets.conf";
	config = new app::TIniFile(configFile);
	reWriteConfig();
}

TSocketController::~TSocketController() {
#ifdef HAS_EPOLL
	closeEpollHandle();
#endif
	clear();
	config->flush();
	util::freeAndNil(config);
}

void TSocketController::prime() {
	debug = false;
	enabled = false;
	thread = nil;
	timer = nil;
	running = false;
	shutdown = false;
	invalidated = false;
	reconnected = false;
	onSocketAccept = nil;
	allowFromAll = false;
	useEpoll = true;
	timeout = 650;
	logger = nil;

#ifdef HAS_EPOLL
	epollfd = INVALID_HANDLE_VALUE;
	ecount = 0;
	scount = 0;
	mask = SOCKET_EPOLL_MASK;
	rsize = 10;
	revents = new epoll_event[rsize];
#endif
}

void TSocketController::waitFor() {
	while (running)
		util::wait(250);
	if (util::assigned(thread))
		while (!thread->isTerminated())
			util::wait(250);
}


void TSocketController::readConfig() {
	config->setSection("Global");
	useEpoll = config->readBool("UseEpoll", useEpoll);
	debug = config->readBool("Debug", debug);
	timeout = config->readInteger("Timeout", timeout);
	allowedList = config->readString("AllowedFromIP", "0.0.0.0;127.0.0.1;::1;fd66:1967:0:1;192.168.200;192.168.201");

	// Read allowed networks/IPs
	if (std::string::npos != allowedList.find(IPV4ANY, util::EC_COMPARE_VALUE_IN_LIST) ||
		std::string::npos != allowedList.find(IPV6ANY, util::EC_COMPARE_VALUE_IN_LIST)) {
		allowFromAll = true;
		writeLog("[Configuration] Allow access to sockets from all source IP's.");
	} else
		writeLog("[Configuration] Allowed IP list <" + allowedList.asString(';') + "> for socket access.");

}

void TSocketController::writeConfig() {
	config->setSection("Global");
	config->writeBool("UseEpoll", useEpoll, app::INI_BLYES);
	config->writeBool("Debug", debug, app::INI_BLYES);
	config->writeInteger("Timeout", timeout);
	config->writeString("AllowedFromIP", allowedList.csv());
}

void TSocketController::reWriteConfig() {
	readConfig();
	writeConfig();
}


int TSocketController::poll() {
	// Poll file descriptors in event list
	int r = EXIT_SUCCESS;
	if (!events.empty()) {
		do {
			errno = EXIT_SUCCESS;
			r = ::poll(events(), events.size(), timeout);
		} while (r != EXIT_SUCCESS && errno == EINTR && running);
	} else {
		// Nothing to do, sleep instead!
		util::saveWait(timeout);
	}
	return r;
}

#ifdef HAS_EPOLL

int TSocketController::epoll() {
	// Poll file descriptors in event list
	int r = EXIT_SUCCESS;
	if (rsize > 0) {
		do {
			errno = EXIT_SUCCESS;
			r = ::epoll_wait(epollfd, revents, rsize, timeout);
		} while (r != EXIT_SUCCESS && errno == EINTR && running);
	} else {
		// Nothing to do, sleep instead!
		util::saveWait(timeout);
	}
	return r;
}

app::THandle TSocketController::createEpollHandle(int flags) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		if (flags > 0)
			r = ::epoll_create1(flags);
		else
			r = ::epoll_create(99);
	} while (r == EXIT_ERROR && errno == EINTR);

	if (r >= 0)
		epollfd = r;

	return epollfd;
}

void TSocketController::closeEpollHandle() {
	if (epollfd >= 0)
		__s_close(epollfd);
	epollfd = INVALID_HANDLE_VALUE;
}

int TSocketController::addEpollHandle(epoll_event* event, const app::THandle hnd) {
	writeLog("TSocketList::start()::addEpollHandle() Add handle <" + std::to_string((size_s)hnd) + ">");
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::epoll_ctl(epollfd, EPOLL_CTL_ADD, hnd, event);
	} while (r != EXIT_SUCCESS && errno == EINTR);
	return r;
}

int TSocketController::removeEpollHandle(epoll_event* event, const app::THandle hnd) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::epoll_ctl(epollfd, EPOLL_CTL_DEL, hnd, event);
	} while (r != EXIT_SUCCESS && errno == EINTR);
	return r;
}

#endif

// See: https://www.ibm.com/support/knowledgecenter/ssw_i5_54/rzab6/poll.htm
// "Using poll() instead of select()"
void TSocketController::start() {
#ifdef HAS_EPOLL
	if (useEpoll && epollfd < 0) {
		int r = createEpollHandle();
		if (r < 0)
			throw util::sys_error("TSocketList::start()::createEpollHandle() failed.");
		writeLog("TSocketList::start()::createEpollHandle() Epoll file descriptor <" + std::to_string((size_s)epollfd) + ">");
	}
#endif
	if (!util::assigned(thread)) {
		running = true;
		app::EThreadStartType execute = running ? app::THD_START_ON_CREATE : app::THD_START_ON_DEMAND;
#ifdef HAS_EPOLL
		if (useEpoll) {
			thread = threads->addThread("Socket Poll Thread",
										&inet::TSocketController::eloop,
										this, execute);
		} else {
#endif
			thread = threads->addThread("Socket Poll Thread",
										&inet::TSocketController::loop,
										this, execute);
#ifdef HAS_EPOLL
		}
#endif
	}
	if (!util::assigned(timer))
		timer = timers->addTimer("Sockets", "ReconnectDelay", 2000, &inet::TSocketController::onReconnectTimer, this);
}

void TSocketController::terminate() {
	shutdown = true;
	if (util::assigned(timer))
		timer->stop();
	if (util::assigned(thread))
		thread->setTerminate(true);
}


#ifdef HAS_EPOLL

PSocketConnection TSocketController::addHandle(PSocket socket, util::PSSLConnection ssl, const app::THandle client, const std::string& remote, const uint32_t mask) {
	if (socket->isUnix()) {
		writeLogFmt("TSocketList::addHandle() Add new client for UNIX Socket $/%, handle = %", socket->getName(), socket->handle(), client);
	} else {
		writeLogFmt("TSocketList::addHandle() Add new client [%] for Socket $/%, handle = %", remote, socket->getName(), socket->handle(), client);
	}
	PSocketConnection connection = addHandle(socket, client, remote);
	connection->ssl = ssl;
	if (useEpoll && util::assigned(connection))
		addEpollEvent(connection, client, mask);
	return connection;
}

#endif

PSocketConnection TSocketController::addHandle(PSocket socket, const app::THandle client, const std::string& remote) {
	if (util::assigned(socket)) {
		invalidated = true;
		return socket->addClient(client, remote);
	}
	return nil;
}

#ifdef HAS_EPOLL

void TSocketController::removeHandle(PSocketConnection connection, PSocket socket, const app::THandle client) {
	// TODO Simplify: Use epoll() as standard!!!
	if (useEpoll && util::assigned(connection))
		removeEpollEvent(connection, client);
	removeHandle(socket, client);
	if (util::assigned(connection)) {
		if (util::assigned(connection->ssl)) {
			connection->ssl->close();
			util::freeAndNil(connection->ssl);
			writeLogFmt("TSocketList::removeHandle() TLS connection destroyed for client socket <%>", client);
		}
	}
}

#endif

void TSocketController::removeHandle(PSocket socket, const app::THandle client) {
	if (util::assigned(socket)) {
		socket->removeClient(client);
		invalidated = true;
	}
}

void TSocketController::removeHandle(const app::THandle client) {
	PSocket o = findClient(client);
	removeHandle(o, client);
}

void TSocketController::addSocket(PSocket socket) {
	app::TLockGuard<app::TMutex> mtx(evntMtx);
	socket->setDebug(getDebug());
	socket->setOwner(this);
	socket->setConfig(config);
	socket->setLogger(logger);
	sockets.push_back(socket);
	invalidated = true;
}

#ifdef HAS_EPOLL

void TSocketController::addSocketHandle(PSocket socket) {
	if (useEpoll && socket->handle() > 0)
		addEpollEvent(socket->getSocket(), socket->handle(), mask);
}

#endif

void TSocketController::removeSocket(PSocket socket) {
	app::TLockGuard<app::TMutex> mtx(evntMtx);
	if (!sockets.empty()) {
		TSocketList::iterator it = sockets.begin();
		while (it != sockets.end()) {
			PSocket o = *it;
			if (util::assigned(o))
				if (o->handle() == socket->handle()) {
					sockets.erase(it);
					break;
				}
			it++;
		}
	}
#ifdef HAS_EPOLL
	if (useEpoll)
		removeEpollEvent(socket->getSocket(), socket->handle());
#endif
	invalidated = true;
}

#ifdef HAS_EPOLL

void TSocketController::addEpollEvent(PSocketConnection connection, const app::THandle hnd, const uint32_t mask) {
	// Prevent reassigning socket to polling list
	if (util::assigned(connection->event))
		return;

	epoll_event * p = new epoll_event;
	connection->event = p;
	connection->client = hnd;
	p->data.ptr = (void *)connection;
	p->events = mask;

	int r = addEpollHandle(p, hnd);
	if (r != EXIT_SUCCESS)
		throw util::sys_error("TSocketList::addEpollEvent()::addEpollHandle() failed.");

	// Increment event count
	++ecount;
	writeLogFmt("TSocketList::addEpollEvent() Socket <%> added to epolling list.", hnd);
}

void TSocketController::removeEpollEvent(PSocketConnection connection, const app::THandle hnd) {
	// First remove epoll entry
	if (util::assigned(connection)) {
		epoll_event* p = connection->event;
		if (util::assigned(p)) {
			removeEpollHandle(p, hnd);
			util::freeAndNil(connection->event);
		}
	}
	if (ecount)
		--ecount;
	writeLogFmt("TSocketList::removeEpollEvent() Socket <%> removed from epolling list.", hnd);
}

#endif

PSocket TSocketController::findSocket(const app::THandle socket) {
	if (!sockets.empty()) {
		for (size_t i=0; i<sockets.size(); i++) {
			PSocket o = sockets[i];
			if (util::assigned(o))
				if (o->handle() == socket)
					return o;
		}
	}
	return nil;
}

PSocket TSocketController::findClient(const app::THandle client) {
	if (!sockets.empty()) {
		for (size_t i=0; i<sockets.size(); i++) {
			PSocket o = sockets[i];
			if (util::assigned(o))
				if (o->hasClient(client))
					return o;
		}
	}
	return nil;
}

PSocket TSocketController::find(const std::string& name) {
	if (!sockets.empty()) {
		for (size_t i=0; i<sockets.size(); i++) {
			PSocket o = sockets[i];
			if (util::assigned(o))
				if (o->getName() == name)
					return o;
		}
	}
	return nil;
}

size_t TSocketController::createPollEvents(short int mask) {
	app::TLockGuard<app::TMutex> mtx(evntMtx);
	pollfd* p;
	PSocket socket;
	size_t n = 0;

	// Check if building new list needed
	if (!events.isValid() || invalidated) {
		invalidated = false;
		events.invalidate();

		if (!sockets.empty()) {

			// Iterate through sockets
			for (size_t i=0; i<sockets.size(); ++i) {
				socket = sockets[i];

				// Check if socket is listening...
				if (socket->isOpen() && socket->isListening()) {

					// Add socket to polling list
					if (events.validIndex(n)) {
						p = events.at(n);
						p->fd = socket->handle();
						p->events = mask;
						p->revents = 0;
					} else {
						p = new pollfd;
						p->fd = socket->handle();
						p->events = mask;
						p->revents = 0;
						events.add(p);
					}

					// Increment size of polling list
					++n;

					if (socket->hasClients()) {

						// Iterate through clients of socket
						TConnectionMap::const_iterator it = socket->begin();
						while (it != socket->end()) {
							PSocketConnection client = it->second;

							// Add client socket entries to event polling list
							if (events.validIndex(n)) {
								p = events.at(n);
								p->fd = client->client;
								p->events = mask;
								p->revents = 0;
							} else {
								p = new pollfd;
								p->fd = client->client;
								p->events = mask;
								p->revents = 0;
								events.add(p);
							}

							// Increment size of polling list
							++n;

							// Next client connection
							++it;

						}

					} // if (socket->hasClients())

				} // if (socket->isOpen() && socket->isListening())

			} // for (size_t i=0; i<sockets.size(); i++)

		} // if (!sockets.empty())

		// Remove unused entries from event polling list
		while (events.size() > n) {
			events.eraseByIndex(util::pred(events.size()));
		}

	} else { // if (!events.isValid() || invalidated)
		resetPollEvents(mask);
	}

	return events.size();
}

void TSocketController::resetPollEvents(short int mask) {
	pollfd* p = events();
	for (size_t i=0; i<events.size (); ++i, ++p) {
		p->events = mask;
		p->revents = 0;
	}
}

//
// See: http://www.ulduzsoft.com/2014/01/select-poll-epoll-practical-difference-for-system-architects/
// for comparison of select(), poll() and epoll()
//
int TSocketController::loop(app::TManagedThread& sender) {
	int r = 0;
	size_t i;
	struct pollfd * p;
	size_t n;

	// Be sure to reset running...
	util::TBooleanGuard<bool> bg(running);

	// Thread execution loop
	do {
		// Create new polling list from master events
		n = createPollEvents(POLLIN);

		// Check for data on sockets
		r = poll();
		if (r > 0 && n > 0) {
			if (debug) {
				std::cout << "TSocketList::loop() Fired events (" << r << ")" << std::endl;
				events.debugOutput();
			}

			// Find socket with data signaled
			p = events();
			for (i=0; i<n; ++i, ++p) {

					// No event on this socket
				if (p->revents == 0)
					continue;

				// Get client socket from current handle
				app::THandle hnd = p->fd;
				PSocket server = findSocket(hnd);
				PSocket client = findClient(hnd);

				// Received data from socket
				if (p->revents & POLLIN) {

					// Listening socket file descriptor == polled file descriptor?
					// Or, in other words, polled file descriptor belongs to a listening socket
					if (util::assigned(server)) {

						// Current socket handle belongs to a listener
						// --> New client(s) has connected
						// Add new handle(s) to current polling list
						app::THandle fd = EXIT_ERROR;
						do {
							socklen_t fromlen = INET6_ADDRSTRLEN;
							util::TBuffer from(fromlen);
							fd = server->accept((sockaddr *)from(), &fromlen, SOCK_NONBLOCK);
							if (fd > 0) {
								bool accept = true;
								std::string addr = inet::inetAddrToStr((sockaddr *)from());
								if (acceptSocket(server, addr, accept)) {
									addHandle(server, fd, addr);
								} else {
									__s_close(fd);
								}
							}
						} while (fd > 0);

					} else { // if (util::assigned(socket))

						// (1) Synchronous handling via callback
						//     Pro: Avoid reading the same socket twice by design
						//     Cons: Read one socket after the other
						// (2) Repeat read until socket empty
						//
						// TODO TSocketConnection for poll()
						ssize_t r;
						do {
							if (debug) std::cout << "TSocketList::eloop() Data for socket <" << hnd << ">" << std::endl;
							r = onSocketData(client, nil, hnd);
							if (r == (ssize_t)0) {
								// Data signaled, but no data received
								// --> Connection closed by foreign host
								if (client->recerr() == EXIT_SUCCESS) {
									if (debug) std::cout << "TSocketList::eloop() Client Socket <" << hnd << "> closed (no data received on signal POLLIN)" << std::endl;
									// Current handle belongs to a client connection
									// --> Connection closed by remote (client) side
									removeHandle(client, hnd);
								}
								// Data signaled, but socket blocked
								// --> No more data available
								if (client->recerr() == EWOULDBLOCK) {
									if (debug) std::cout << "TSocketList::eloop() No more data for client Socket <" << hnd << "> available." << std::endl;
									break;
								}
							}
						} while (r > (ssize_t)0);

					} // else if (util::assigned(socket))

				} // if (p->revents & POLLIN)

				// Client has disconnected
				if (p->revents & POLLHUP) {
					// Remove from current socket polling list
					// if socket is not listening socket!
					if (debug) std::cout << "TSocketList::loop() Signal POLLHUP for socket <" << hnd << ">" << std::endl;
					if (!util::assigned(server)) {
						// Current handle belongs to a client connection
						// --> Connection closed by remote (client) side
						removeHandle(hnd);
						if (debug) std::cout << "TSocketList::loop() Client socket <" << hnd << "> closed." << std::endl;
					} else {
						// We're in a client socket:
						// Server socket closed on remote server side
						server->setConnected(false);
						if (debug) std::cout << "TSocketList::loop() Remote server closed on socket <" << hnd << ">" << std::endl;
					}
				}

				// Error on socket
				if (p->revents & POLLERR) {
					if (debug) std::cout << "TSocketList::loop() Signal POLLERR for socket <" << hnd << ">" << std::endl;
					if (!util::assigned(server)) {
						if (debug) std::cout << "TSocketList::loop() Client socket <" << hnd << "> closed." << std::endl;
						removeHandle(hnd);
					}
				}

				// Invalid socket descriptor
				if (p->revents & POLLNVAL) {
					if (debug) std::cout << "TSocketList::loop() Signal POLLNVAL for socket <" << hnd << ">" << std::endl;
					if (!util::assigned(server)) {
						// Server socket disconnected from client
						// Client connection disconnected from server socket
						if (debug) std::cout << "TSocketList::loop() Client socket <" << hnd << "> closed." << std::endl;
						removeHandle(hnd);
					}
				}

				p->revents = 0;

			} // for (i=0; i<events.size(); ++i, ++p)

			if (debug) std::cout << "TSocketList::loop() End of polling events." << std::endl;

		} // if (r > 0)

	} while (!shutdown && !sender.isTerminating());

	// Close all client connections
	if (debug) std::cout << "TSocketList::loop() Close client connections." << std::endl;
	close();

	// Close all sockets
	if (debug) std::cout << "TSocketList::loop() Close sockets in list." << std::endl;
	clear();

	if (debug) std::cout << "TSocketList::loop() Reached end of thread." << std::endl;
	return EXIT_SUCCESS;
}


#ifdef HAS_EPOLL

size_t TSocketController::createEpollEvents() {
	app::TLockGuard<app::TMutex> mtx(evntMtx);

	// Check if sockets added to list prior to polling
	if (scount != sockets.size() || reconnected || invalidated) {
		bool retry = false;
		reconnected = false;
		invalidated = false;
		TSocketList::iterator it = sockets.begin();
		while (it != sockets.end()) {
			PSocket socket = *it;
			if (util::assigned(socket)) {

				// Check if TLS socket is initialized:
				// Workaround for race condition changed() is set before TLS socket connection object initialized
				if (socket->isSecure() && socket->isClient()) {
					if (!socket->hasSSLConnection()) {
						retry = true;
						if (debug && socket->wasConnected()) std::cout << "TSocketList::createEpollEvents() SSL context for TLS Socket \"" << socket->getName() << "\" not initialized." << std::endl;
					}
				}

				// Check if TLS socket has been negotiated
				bool negotiated = true;
				if (socket->hasSSLConnection()) {
					negotiated = socket->getSSLConnection()->isNegotiated();
					if (!negotiated) {
						retry = true;
						if (debug) std::cout << "TSocketList::createEpollEvents() SSL connection for TLS Socket \"" << socket->getName() << "\" not negotiated." << std::endl;
					}
				}

				// Check if socket state has been changed
				//  - Add listening server socket
				//  - Add connected client socket
				//  - Add datagram socket
				if (negotiated && socket->changed()) {
					if (socket->isOpen() && (socket->isListening() || socket->isConnected() || socket->isDatagram())) {
						writeLogFmt("TSocketList::createEpollEvents() Socket state changed for $", socket->getName());
						addSocketHandle(socket);
						socket->unchange();
					}
				}
			}
			it++;
		}

		// Retry polling sockets?
		scount = retry ? (size_t)-1 : sockets.size();
	}

	// Create polling event list
	if (ecount && !sockets.empty()) {

		// Resize result event array if needed
		bool resized = false;
		size_t n = 5 * sockets.size() + 2 * ecount;
		if (!util::assigned(revents)) {
			rsize = n;
			revents = new epoll_event[rsize];
			resized = true;
			if (debug) std::cout << "TSocketList::createEpollEvents() Create event list with " << rsize << " entries." << std::endl;
		} else {
			if (rsize < n) {
				rsize = n;
				delete[] revents;
				revents = new epoll_event[rsize];
				resized = true;
				if (debug) std::cout << "TSocketList::createEpollEvents() Resized event list to " << rsize << " entries." << std::endl;
			}
		}

		// Initialize new array
		if (resized) {
			memset(revents, 0, rsize * sizeof(epoll_event));
		}

	}

	return ecount;
}

void TSocketController::debugOutputEpoll(size_t size) {
	if (util::assigned(revents) && rsize) {
		if (size == 0)
			size = rsize;
		PSocketConnection connection;
		epoll_event* p = revents;
		for (size_t i=0; i<size; ++i, ++p) {
			size_t k = util::succ(i);
			if (util::assigned(p)) {
				connection = (PSocketConnection)p->data.ptr;
				if (util::assigned(connection)) {
					std::cout << k << ". Handle = " << connection->client << std::endl;
					std::cout << k << ". Socket = " << connection->server << std::endl;
					if (connection->socket->isUnix()) {
						std::cout << k << ". Name   = \"" << connection->socket->getName() << "\" on device \"" << connection->socket->getHost() << "\"" << std::endl;
					} else {
						std::cout << k << ". Name   = \"" << connection->socket->getName() << "\" on port " << connection->socket->getService() << std::endl;
					}
				} else {
					std::cout << k << ". <Undefined connection>" << std::endl;
				}
				std::cout << k << ". Events = " << util::valueToBinary(p->events) << std::endl;
			} else
				std::cout << k << ". <Undefined entry> (" << i << ")" << std::endl;
		}
		std::cout << std::endl;
	} else
		std::cout << "<Event list is empty>" << std::endl << std::endl;
}


int TSocketController::eloop(app::TManagedThread& sender) {
	int rcount = 0;
	size_t n;
	epoll_event* p;

	// Be sure to reset running...
	util::TBooleanGuard<bool> bg(running);

	do {
		if (enabled) {
			// Create new polling list from master events
			n = createEpollEvents();
			//std::cout << "TSocketList::eloop() Watched events (" << n << ")" << std::endl;

			// Check for data on sockets
			rcount = epoll();
			if (rcount < 0)
				throw util::sys_error("TSocketList::eloop()::epoll() failed.");

			if (rcount > 0 && n > 0 && running && !sender.isTerminating()) {
				if (debug) {
					std::cout << "TSocketList::eloop() Watched events           = " << n << std::endl;
					std::cout << "TSocketList::eloop() Fired events (rcount)    = " << rcount << std::endl;
					std::cout << "TSocketList::eloop() Poll array size (rsize)  = " << rsize << std::endl;
					std::cout << "TSocketList::eloop() Event list size (ecount) = " << ecount << std::endl;
					std::cout << "TSocketList::eloop() Socket count (scount)    = " << scount << std::endl;
					std::cout << "TSocketList::eloop() Socket list size         = " << sockets.size() << std::endl << std::endl;
					debugOutputEpoll(rcount);
				}

				// Find socket with data signaled
				if (!reader.empty()) reader.clear();
				p = revents;
				for (int i=0; i<rcount; ++i, ++p) {

					// No event on this socket
					if (p->events == 0)
						continue;

					// Get listening socket from data pointer
					PSocketConnection connection = (PSocketConnection)p->data.ptr;
					PSocket socket = connection->socket;
					app::THandle server = connection->server;
					app::THandle client = connection->client;

					// Received data from socket
					if (p->events & POLLIN) {

						// Event received from server socket
						// Descriptor on server side == client side (set by design!)
						if (server == client && socket->isServer()) {

							// Current socket handle belongs to a listener
							// --> New client(s) has connected
							// Add new handle(s) to current polling list
							app::THandle fd = EXIT_ERROR;
							do {

								// Accept incoming client socket connection
								CSocketAddress from;
								socklen_t fromlen = sizeof (from.addr6);
								fd = socket->accept(&from.addr, &fromlen, SOCK_NONBLOCK);
								if (fd > 0) {

									// Accept connection from client address?
									bool accept = true;
									util::PSSLConnection ssl = nil;
									std::string addr = inet::inetAddrToStr(&from.addr);

									// Check if socket accepted by rules
									if (acceptSocket(socket, addr, accept)) {
										// Check if TLS socket accepted by OpenSSL library
										acceptTLS(socket, fd, ssl, accept);
									}

									// Add client socket if connection was accepted
									if (accept) {
										addHandle(socket, ssl, fd, addr, mask);
									} else {
										__s_close(fd);
										errorLogFmt("TSocketList::eloop() Server socket $/% rejected client <%>",
												socket->getName(), socket->handle(), client);
									}

								}
							} while (fd > 0);

						} else { // if (server == client)

							// Store connection in reader list
							connection->state |= POLLIN;
							reader.push_back(connection);

						} // else if (server == client)

					} // if (p->revents & POLLIN)

					// Client has disconnected
					if (p->events & POLLHUP) {
						// Remove from current socket polling list
						// if socket is not listening socket!
						writeLogFmt("TSocketList::eloop() Received signal POLLHUP for socket <%>", client);
						doDisconnectAction(connection, socket, server, client);
					}

					// Error on socket
					if (p->events & POLLERR) {
						if (debug) std::cout << "TSocketList::eloop() Received signal POLLERR for socket <" << client << ">" << std::endl;
						if (server != client && util::assigned(socket)) {
							writeLogFmt("TSocketList::eloop() Client socket <%> closed on POLLERR.", client);
							removeHandle(connection, socket, client);
						}
					}

					// Invalid socket descriptor
					if (p->events & POLLNVAL) {
						if (debug) std::cout << "TSocketList::eloop() Received signal POLLNVAL for socket <" << client << ">" << std::endl;
						if (server != client) {
							writeLogFmt("TSocketList::eloop() Client socket <%> closed on POLLNVAL.", client);
							removeHandle(connection, socket, client);
						}
					}

					p->events = 0;

				} // for (i=0; i<events.size(); ++i, ++p)

				// Executer reader
				if (!shutdown)
					roundRobinReader();

				if (debug) std::cout << "TSocketList::eloop() End of polling events." << std::endl;

			} // if (r > 0)

			// Reconnect sockets
			if (!shutdown)
				reconnector();

		} else {
			// Polling disabled, just wait given time...
			util::wait(timeout);
		}

	} while (!shutdown && !sender.isTerminating());

	// Close all client connections
	writeLog("TSocketList::eloop() Close client connections.");
	close();

	// Close all sockets
	writeLog("TSocketList::eloop() Close sockets in list.");
	clear();

	writeLog("TSocketList::eloop() Reached end of thread.");
	return EXIT_SUCCESS;
}

void TSocketController::roundRobinReader() {
	if (!reader.empty()) {
		ssize_t r;
		bool data;
		PSocket socket;
		app::THandle server, client;
		PSocketConnection connection;
		TSocketConnectionList::const_iterator it;

		do {
			data = false;
			it = reader.begin();
			while (it != reader.end()) {
				connection = *it;
				socket = connection->socket;
				server = connection->server;
				client = connection->client;

				if (connection->state & POLLIN) {

					if (debug) std::cout << "TSocketList::roundRobinReader() Read data from socket <" << client << ">" << std::endl;
					r = onSocketData(socket, connection, client);

					if (r == (ssize_t)0) {

						// No data returned, stream type socket may have been closed
						// --> No close action for datagram type UDP and native UNIX file sockets!
						if (!socket->isDatagram() && !socket->isUnix()) {

							// Data signaled for TCP socket, but no data received
							// --> Connection closed by foreign host
							if (socket->recerr() == EXIT_SUCCESS) {
								writeLogFmt("TSocketList::roundRobinReader() Client Socket <%> closed (EOF on POLLIN)", client);
								doDisconnectAction(connection, socket, server, client);
							}

							// Data signaled, but socket blocked
							// --> No more data available
							if (socket->recerr() == EWOULDBLOCK) {
								if (debug) std::cout << "TSocketList::roundRobinReader() No more data from client Socket <" << client << "> available." << std::endl;
							}
						}

						// Reset read request
						connection->state &= ~POLLIN;

					} else {
						if (r > (ssize_t)0) {
							// Try to receive more data on native (unencrypted) sockets only
							if (!socket->isSecure())
								data = true;
						} else {
							// Result < 0 --> Error on socket read
							connection->state &= ~POLLIN;
						}
					}

				} // if (connection->state & POLLIN)

				// Next connection entry
				++it;

			} // while (it != reader.end())

		} while (data);

		// All requests finished
		reader.clear();
	}
}


void TSocketController::doDisconnectAction(PSocketConnection connection, PSocket socket, const app::THandle server, const app::THandle client) {
	if (util::assigned(socket)) {
		if (server == client) {
			// We're in a client socket that tries to get a server connection
			// Server socket connection was closed on remote side
			socket->setConnected(false);
			if (socket->isOpen() && socket->wasConnected())
				removeEpollEvent(connection, socket->handle());
			if (socket->hasSSLConnection()) {
				writeLogFmt("TSocketList::doDisconnectAction() TLS connection cleared on socket <%>", server);
				if (socket->getSSLConnection()->isNegotiated())
					socket->getSSLConnection()->close();
				socket->getSSLConnection()->reset();
			}
			socket->reconnect();
			writeLogFmt("TSocketList::doDisconnectAction() Remote server closed on socket <%>", server);
		} else {
			// We're in a server socket:
			// Handle belongs to a client connection
			// Connection closed by remote (client) side
			removeHandle(connection, socket, client);
			writeLogFmt("TSocketList::doDisconnectAction() Client socket <%> closed.", client);
		}
	}
}

#endif

void TSocketController::addReconnectSocket(PSocket socket) {
	if (util::assigned(socket)) {
		app::TLockGuard<app::TMutex> mtx(recnMtx);
		reconnects.push_back(socket);
		socket->setTime();
		if (socket->getService().empty()) {
			writeLogFmt("TSocketList::socketDisconnect() Add socket $ [%] to reconnect list.",
					socket->getName(), socket->getHost());
		} else {
			writeLogFmt("TSocketList::socketDisconnect() Add socket $ [%:%] to reconnect list.",
					socket->getName(), socket->getHost(), socket->getService());
		}
	}
}

void TSocketController::onReconnectTimer() {
	if (!useEpoll && running && !shutdown)
		reconnector();
}


struct CReconnectEraser
{
    bool operator()(PSocket o) const {
    	if (util::assigned(o))
    		if (o->isConnected())
    			return true;
    	return false;
    }
};

void TSocketController::reconnector() {
	app::TLockGuard<app::TMutex> mtx(recnMtx);
	if (!reconnects.empty()) {
		TSocketList::iterator it = reconnects.begin();
		while (it != reconnects.end()) {

			// Try to reconnect plain, UNIX or TLS client sockets
			if (!reconnect(util::asClass<TTLSClientSocket>(*it)))
				if (!reconnect(util::asClass<TClientSocket>(*it)))
					reconnect(util::asClass<TUnixClientSocket>(*it));

			it++;
		}
		// Delete all connected items from list
		reconnects.erase(std::remove_if(reconnects.begin(), reconnects.end(), CReconnectEraser()), reconnects.end());
	}
}

template<typename socket_t>
bool TSocketController::reconnect(socket_t&& socket) {
	bool retVal = false;
	if (util::assigned(socket)) {
		// Wait delay before try to reconnect socket
		// --> prevent race condition when timer thread interrupts polling thread
		if ((util::now() - socket->getTime()) > 3) {
			if (socket->getService().empty()) {
				writeLogFmt("TSocketList::reconnect() Reconnect socket $ [%]",
						socket->getName(), socket->getHost());
			} else {
				writeLogFmt("TSocketList::reconnect() Reconnect socket $ [%:%]",
						socket->getName(), socket->getHost(), socket->getService());
			}
			socket->setTime();
			socket->open();
			if (socket->isConnected()) {
				// Force createEpollEvents() to recognize a change in sockets!
				reconnected = true;
				if (socket->getService().empty()) {
					writeLogFmt("TSocketList::reconnect() Socket $ is now connected to [%] (%)",
							socket->getName(), socket->getHost(), socket->changed() ? "triggered" : "static");
				} else {
					writeLogFmt("TSocketList::reconnect() Socket $ is now connected to [%:%] (%)",
							socket->getName(), socket->getHost(), socket->getService(), socket->changed() ? "triggered" : "static");
				}
			} else {
				if (socket->getService().empty()) {
					socket->writeLogFmt("TSocketList::reconnect() Connecting socket $ to [%] failed.",
							socket->getName(), socket->getHost());
				} else {
					socket->writeLogFmt("TSocketList::reconnect() Connecting socket $ to [%:%] failed.",
							socket->getName(), socket->getHost(), socket->getService());
				}
			}
			retVal = true;
		}	
	}
	return retVal;
}


void TSocketController::close() {
	app::TLockGuard<app::TMutex> mtx(evntMtx);
	if (!sockets.empty()) {
		for (size_t i=0; i<sockets.size(); i++) {
			PSocket o = sockets[i];
			if (util::assigned(o))
				o->close();
		}
	}
}

void TSocketController::clear() {
	app::TLockGuard<app::TMutex> mtx(evntMtx);
	if (!sockets.empty()) {
		for (size_t i=0; i<sockets.size(); i++) {
			PSocket o = sockets[i];
			if (util::assigned(o))
				o->close();
			util::freeAndNil(o);
		}
	}
	sockets.clear();
#ifdef HAS_EPOLL
	if (util::assigned(revents)) {
		delete[] revents;
		revents = nil;
	}
#endif
}


ssize_t TSocketController::onSocketData(const app::THandle client) {
	PSocket socket = findClient(client);
	// TODO TSocketConnection for poll()
	return onSocketData(socket, nil, client);
}

ssize_t TSocketController::onSocketData(PSocket socket, PSocketConnection connection, const app::THandle client) {
	if (util::assigned(socket)) {
		try {
			return socket->executeDataWrapper(connection, client);
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			errorLogFmt("TSocketList::onSocketData() Receive [%] from client socket <%> failed: $",
					socket->getDescription(), client, sExcept);
		} catch (...)	{
			errorLogFmt("TSocketList::onSocketData() Receive [%] from client socket <%> failed on unknown exception.",
					socket->getDescription(), client);
		}
	}
	return (ssize_t)EXIT_ERROR;
}

bool TSocketController::acceptSocket(PSocket socket, const std::string& addr, bool& accept) {

	// Accept all UNIX Sun RPC file socket connections
	if (util::assigned(socket)) {
		if (socket->isUnix()) {
			accept = true;
			return accept;
		}
	}

	// Execute callback
	try {
		if (onSocketAccept != nil)
			onSocketAccept(addr, accept);
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		errorLogFmt("TSocketList::onAcceptSocket() Accept [%] failed: $", addr, sExcept);
	} catch (...)	{
		errorLogFmt("TSocketList::onAcceptSocket() Accept [%] failed on unknown exception.", addr);
	}

	// Check if accept still allowed after executing callback
	if (accept) {

		// Check for accepted IPs/networks
		if (allowFromAll)
			return accept;

		if (allowedList.empty())
			return accept;

		app::TStringVector::const_iterator it = allowedList.begin();
		while (it != allowedList.end()) {
			const std::string& s = *it;
			if (!s.empty()) {
				if (0 == util::strncasecmp(addr, s, s.size()))
					return accept;
			}
			it++;
		}

		// Address not permitted
		accept = false;
	}

	return accept;
}

bool TSocketController::acceptTLS(PSocket socket, const app::THandle client, util::PSSLConnection& ssl, bool& accept) {
	if (util::assigned(socket)) {
		if (socket->isServer() && socket->isSecure() && accept) {

			// We're on server side and accepting a new TLS client connection...
			if (util::isClass<TTLSServerSocket>(socket)) {

				// Create a new TLS connection for client
				PTLSServerSocket server = util::asClass<TTLSServerSocket>(socket);
				ssl = new util::TSSLConnection;
				ssl->setDebug(server->getDebug());
				ssl->initialize(server->context());
				writeLogFmt("TSocketList::acceptTLS() New TLS connection created on $/% for client <%>", socket->getName(), socket->handle(), client);

				// Negotiate incoming TLS connection
				accept = ssl->assign(client);
				if (accept)
					accept = ssl->setCaller(this, util::SSLInit.getIndex());
				if (accept)
					accept = ssl->accept();

				if (accept)
					writeLogFmt("TSocketList::acceptTLS() TLS negotiation for socket $/% succeeded.", socket->getName(), socket->handle());
				else
					writeLogFmt("TSocketList::acceptTLS() TLS negotiation for socket $/% failed: $:%",
							socket->getName(), socket->handle(), ssl->errmsg(), ssl->error());
			}
		}
	} else
		accept = false;
	return accept;
}


template<typename value_t, typename... variadic_t>
void TSocketController::writeLogFmt(const std::string& str, const value_t value, variadic_t... args) {
	writeLog(util::csnprintf(str, value, std::forward<variadic_t>(args)...));
}

template<typename value_t, typename... variadic_t>
void TSocketController::errorLogFmt(const std::string& str, const value_t value, variadic_t... args) {
	errorLog(util::csnprintf(str, value, std::forward<variadic_t>(args)...));
}

void TSocketController::writeLog(const std::string& s) const {
	if (getDebug()) std::cout << s << std::endl;
	if (util::assigned(logger)) {
		logger->write("[Socket list] " + s);
	}
}

void TSocketController::errorLog(const std::string& s) const {
	if (getDebug()) std::cout << s << std::endl;
	if (util::assigned(logger)) {
		logger->write("[Error] [Socket list] " + s);
	}
}



TServerSocket::TServerSocket(const bool debug) : TSocket(debug) {
	prime();
	setServer();
}

TServerSocket::~TServerSocket() {
}

void TServerSocket::prime() {
	onServerData = nil;
}

bool TServerSocket::open(const std::string& bindTo, const int port, const inet::EAddressFamily family) {
	std::string _host = bindTo;
	int _port = port;
	socket.reWriteConfig(_host, _port, family);
	std::string _service = std::to_string((size_s)_port);
	if (socket.open(_host, _service, inet::ESocketType::ST_STREAM, family)) {
		if (socket.bind()) {
			writeLogFmt("TServerSocket::open() Server socket <%> for Port <%> opened successfully.", handle(), getService());

			// Activate keep alive on server socket
			if (socket.useKeepAlive())
				if (!socket.setOption(SO_KEEPALIVE))
					throw util::sys_error_fmt("TServerSocket::open::setOption(%) failed on reopen for SO_KEEPALIVE", getDescription());

			if (socket.listen()) {
				if (isListening()) {
					if (getDebug()) {
						writeLogFmt("TServerSocket::open() Server socket <%> listening on port <%>", handle(), getService());
						socket.debugOutput();
					}

					// Server socket is listening
					return true;
				}
			}
			if (getDebug()) std::cout << std::endl;
		}
	}
	return false;
}

ssize_t TServerSocket::executeDataWrapper(PSocketConnection connection, app::THandle client) {
	if (client > 0)
		return executeDataMethod(client);
	throw util::app_error_fmt("TServerSocket::executeDataWrapper(%) failed: Invalid parameter.", getName());
}

ssize_t TServerSocket::executeDataMethod(app::THandle client) {
	try {
		bool exit = false;
		if (onServerData != nil) {
			ssize_t r = onServerData(*this, client, exit);
			if (exit) {
				socket.errorLogFmt("TServerSocket::executeDataMethod() Drop for client socket <%> requested.", client);
				drop(client);
			}
			return r;
		}
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		socket.errorLogFmt("TServerSocket::executeDataMethod() for client socket <%> failed: $", client, sExcept);
	} catch (...)	{
		socket.errorLogFmt("TServerSocket::executeDataMethod() for client socket <%> failed on unknown exception.", client);
	}
	return (ssize_t)EXIT_ERROR;
}

ssize_t TServerSocket::send(const app::THandle hnd, void const * const data, size_t const size, int const flags) const {
	return socket.send(hnd, data, size, flags);
}

ssize_t TServerSocket::receive(const app::THandle hnd, void * const data, size_t const size, int const flags) const {
	return socket.receive(hnd, data, size, flags);
}




TClientSocket::TClientSocket(const bool debug) : TSocket(debug) {
	prime();
	setClient();
}

TClientSocket::~TClientSocket() {
}

void TClientSocket::prime() {
	onClientData = nil;
}

bool TClientSocket::open(const std::string& serverIP, const int port, const inet::EAddressFamily family) {
	bool retVal = false;
	std::string _host = serverIP;
	int _port = port;
	socket.reWriteConfig(_host, _port, family);
	std::string _service = std::to_string((size_s)_port);
	if (_host.empty()) {
		throw util::app_error("TClientSocket::open() Open failed, no server address given.");
	}
	if (socket.open(_host, _service, inet::ESocketType::ST_STREAM, family)) {
		writeLogFmt("TClientSocket::open() Client socket <%> for [%] opened successfully.", handle(), getDescription());
		if (getDebug())
			socket.debugOutput();

		// Activate keep alive on client socket
		if (socket.useKeepAlive())
			if (!socket.setOption(SO_KEEPALIVE))
				throw util::sys_error(util::csnprintf("TClientSocket::open::setOption(%) failed for SO_KEEPALIVE", getDescription()));

		socket.connect();
		if (isConnected()) {
			retVal = true;
			writeLogFmt("TClientSocket::open() Client socket <%> successfully connected to [%]", handle(), getDescription());
		} else {
			// Connect failed, try to reconnect...
			reconnect();
		}

	}
	return retVal;
}

bool TClientSocket::open() {
	bool retVal = false;

	// Try to open socket again
	if (!isOpen()) {
		if (socket.open(getHost(), getService(), inet::ESocketType::ST_STREAM, getFamily())) {
			writeLogFmt("TClientSocket::open() Client socket <%> reopened for [%]", handle(), getDescription());
			if (getDebug())
				socket.debugOutput();

			// Activate keep alive on client socket
			if (socket.useKeepAlive())
				if (!socket.setOption(SO_KEEPALIVE))
					throw util::sys_error(util::csnprintf("TClientSocket::open::setOption(%) failed on reopen for SO_KEEPALIVE", getDescription()));

		}
	}
	if (isOpen()) {

		// Try to connect client socket to server
		socket.connect();

		if (isConnected()) {
			retVal = true;
			writeLogFmt("TClientSocket::open() Client socket <%> successfully connected to [%]", handle(), getDescription());
		} else {
			// Connect failed, close client socket!
			if (wasConnected())
				close();
		}
	}
	return retVal;
}

ssize_t TClientSocket::executeDataWrapper(PSocketConnection connection, app::THandle client) {
	return executeDataMethod();
}

ssize_t TClientSocket::executeDataMethod() {
	try {
		if (onClientData != nil)
			return onClientData(*this);
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		socket.errorLogFmt("TClientSocket::executeDataMethod() failed: $", sExcept);
	} catch (...)	{
		socket.errorLog("TClientSocket::executeDataMethod() failed on unknown exception.");
	}
	return (ssize_t)EXIT_ERROR;
}

ssize_t TClientSocket::send(void const * const data, size_t const size, int const flags) const {
	return socket.send(data, size, flags);
}

ssize_t TClientSocket::receive(void * const data, size_t const size, int const flags) const {
	return socket.receive(data, size, flags);
}



TTLSServerSocket::TTLSServerSocket(const bool debug) : TServerSocket(debug), TTLSBaseSocket() {
	prime();
	init();
}

TTLSServerSocket::~TTLSServerSocket() {
}

void TTLSServerSocket::prime() {
	onServerData = nil;
	certsLoaded = false;
}

void TTLSServerSocket::init() {
	setSecure();

	// Initialize TLS context for server socket
	ssl.setDebug(getDebug());
	ssl.initialize(util::EContextType::ECT_SERVER);
	ssl.mode(SSL_MODE_ENABLE_PARTIAL_WRITE);
#ifdef SSL_HAS_PROTO_VERSION
	ssl.version(TLS1_VERSION);
#else
	ssl.option(SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif
}

void TTLSServerSocket::loadCertificates() {
	if (!certsLoaded) {

		// Load configuration for certificate files by socket name
		reWriteConfig();

		// Load certificate, key and parameter
		if (!certFile.empty()) {
			if (util::fileExists(certFile)) {
				if (ssl.loadCertificate(certFile))
					writeLogFmt("TTLSServerSocket::loadCertificates() Certificate file <%> loaded.", certFile);
				else
					throw util::app_error_fmt("TTLSServerSocket::loadCertificates() failed: Certificate file <%> not loaded.", certFile);
			} else
				throw util::app_error_fmt("TTLSServerSocket::loadCertificates() failed: Certificate file <%> not found.", certFile);
		} else {
			if (ssl.useCertificate(util::TLS_CERT, util::TLS_CERT_SIZE))
				writeLog("TTLSServerSocket::loadCertificates() Internal certificate loaded.");
			else
				throw util::app_error("TTLSServerSocket::loadCertificates()::useCertificate() failed.");
		}

		if (!keyFile.empty()) {
			if (util::fileExists(keyFile)) {
				if (ssl.loadPrivateKey(keyFile))
					writeLogFmt("TTLSServerSocket::loadCertificates() Private key file <%> loaded.", keyFile);
				else
					throw util::app_error_fmt("TTLSServerSocket::loadCertificates() failed: Private key file <%> not loaded.", keyFile);

			} else
				throw util::app_error_fmt("TTLSServerSocket::loadCertificates() failed: Private key file <%> not found.", keyFile);
		} else {
			if (ssl.usePrivateKey(util::TLS_KEY, util::TLS_KEY_SIZE))
				writeLog("TTLSServerSocket::loadCertificates() Internal private key loaded.");
			else
				throw util::app_error("TTLSServerSocket::loadCertificates()::usePrivateKey() failed.");

		}

		if (dhFile != SSL_CURVED_DH_PLACEHOLDER) {
			if (!dhFile.empty()) {
				if (util::fileExists(dhFile)) {
					if (ssl.loadDiffieHellmannParameter(dhFile))
						writeLogFmt("TTLSServerSocket::loadCertificates() Diffie-Hellmann parameter file <%> loaded.", dhFile);
					else
						throw util::app_error_fmt("TTLSServerSocket::loadCertificates() failed: Diffie-Hellmann parameter file <%> not loaded.", dhFile);
				} else
					throw util::app_error_fmt("TTLSServerSocket::loadCertificates() failed: Diffie-Hellmann parameter file <%> not found.", dhFile);
			} else {
				if (ssl.useDiffieHellmannParameter(util::TLS_DH_PARAMS, util::TLS_DH_PARAMS_SIZE))
					writeLog("TTLSServerSocket::loadCertificates() Internal Diffie-Hellmann parameter loaded.");
				else
					throw util::app_error("TTLSServerSocket::loadCertificates()::useDiffieHellmannParameter() failed.");
			}

			// Check Diffie-Hellmann parameter
			std::string message;
			int r = ssl.checkDiffieHellmanParams(message);
			if (EXIT_SUCCESS != r)
				throw util::app_error_fmt("TTLSServerSocket::checkDiffieHellmanParams() failed: $ (%)", message, r);

		} else {
			// Load curved DH parameter
			if (ssl.useEllipticCurveDiffieHellmannParameter())
				writeLog("TTLSServerSocket::loadCertificates() Using elliptic curved Diffie-Hellmann key.");
			else
				throw util::app_error_fmt("TTLSServerSocket::loadCertificates()::useEllipticCurveDiffieHellmannParameter() failed: $", util::getOpenSSLErrorMessage());
		}

		// Check certificate against key
		if (!ssl.checkPrivateKey())
			throw util::app_error("TTLSServerSocket::loadCertificates() failed: Private key does not match certificate.");

		certsLoaded = true;
		writeLog("TTLSServerSocket::loadCertificates() All certificates loaded.");
	}
}

void TTLSServerSocket::readConfig()
{
	if (getDebug()) std::cout << "TTLSServerSocket::readConfig(\"" + getName() + "\")" << std::endl;
	app::PIniFile config = getConfig();
	if (util::assigned(config) && !getName().empty()) {
		config->setSection(getName());

		std::string _certFile = config->readString("CertFile", SSL_BUILTIN_PLACEHOLDER);
		std::string _keyFile = config->readString("KeyFile", SSL_BUILTIN_PLACEHOLDER);
		std::string _dhFile = config->readString("DiffieHellmanFile", SSL_BUILTIN_PLACEHOLDER);

		certFile = (_certFile != SSL_BUILTIN_PLACEHOLDER) ? _certFile : "";
		keyFile = (_keyFile != SSL_BUILTIN_PLACEHOLDER) ? _keyFile : "";

		// 1. Empty filename represented by "builtin" means to use internal DH params
		// 2. Curved DH params are represented by "curved"
		// 3. Otherwise filename is used to load DH parameter file
		if (_dhFile == SSL_CURVED_DH_PLACEHOLDER)
			dhFile = SSL_CURVED_DH_PLACEHOLDER;
		else
			dhFile = (_dhFile != SSL_BUILTIN_PLACEHOLDER) ? _dhFile : "";
	}
}

void TTLSServerSocket::writeConfig()
{
	app::PIniFile config = getConfig();
	if (util::assigned(config) && !getName().empty()) {
		config->setSection(getName());
		config->writeString("CertFile", (certFile.empty()) ? SSL_BUILTIN_PLACEHOLDER : certFile);
		config->writeString("KeyFile", (keyFile.empty()) ? SSL_BUILTIN_PLACEHOLDER : keyFile);
		config->writeString("DiffieHellmanFile", (dhFile.empty()) ? SSL_BUILTIN_PLACEHOLDER : dhFile);
	}
}

void TTLSServerSocket::reWriteConfig()
{
	readConfig();
	writeConfig();
}

bool TTLSServerSocket::open(const std::string& bindTo, const int port, const inet::EAddressFamily family) {
	ssl.setDebug(getDebug());
	loadCertificates();
	return TServerSocket::open(bindTo, port, family);
}


ssize_t TTLSServerSocket::send(const util::PSSLConnection connection, void const * const data, size_t const size) const {
	if (util::assigned(connection))
		return connection->send(data, size);
	return (ssize_t)0;
}

ssize_t TTLSServerSocket::receive(const util::PSSLConnection connection, void * const data, size_t const size) const {
	if (util::assigned(connection)) {
		ssize_t retVal = connection->receive(data, size);
		if (retVal < 0) {
			if (EWOULDBLOCK == connection->syserr()) {
				// TODO Set socket sysval and recval to EWOULDBLOCK
				// --> Mark socket read as finished!
			}
		}
		return retVal;
	}
	return (ssize_t)0;
}

ssize_t TTLSServerSocket::executeDataWrapper(PSocketConnection connection, app::THandle client) {
	if (getDebug()) std::cout << "TTLSServerSocket::executeDataWrapper(" << util::quote(getName()) << ") called." << std::endl;
	if (util::assigned(connection))
		if (util::assigned(connection->ssl))
			return executeDataMethod(connection->ssl);
	throw util::app_error_fmt("TTLSServerSocket::executeDataWrapper($) failed: Invalid parameter.", getName());
}

ssize_t TTLSServerSocket::executeDataMethod(const util::PSSLConnection connection) {
	if (getDebug()) std::cout << "TTLSServerSocket::executeDataMethod(" << util::quote(getName()) << ") called." << std::endl;
	if (util::assigned(connection)) {
		try {
			if (onServerData != nil) {
				ssize_t r = onServerData(*this, connection);
				if (r <= 0) {
					errorLogFmt("TTLSServerSocket::executeDataMethod() Result = %", r);
					if (util::assigned(connection)) {
						errorLogFmt("TTLSServerSocket::executeDataMethod() Read error = %", connection->recerr());
						errorLogFmt("TTLSServerSocket::executeDataMethod() Error Message $", connection->errmsg());
					}
				}
				return r;
			}
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			socket.errorLogFmt("TTLSServerSocket::executeDataMethod() Client socket <%> $", connection->handle(), sExcept);
		} catch (...)	{
			socket.errorLogFmt("TTLSServerSocket::executeDataMethod() Unknown exception on client socket <%>", connection->handle());
		}
	} else
		throw util::app_error_fmt("TTLSServerSocket::executeDataMethod($) failed: Invalid connection.", getName());
	return (ssize_t)EXIT_ERROR;
}




TTLSClientSocket::TTLSClientSocket(const bool debug) : TClientSocket(debug), TTLSBaseSocket() {
	prime();
	init();
}

TTLSClientSocket::~TTLSClientSocket() {
	close();
}

void TTLSClientSocket::prime() {
	onClientData = nil;
}

void TTLSClientSocket::init() {
	setSecure();
	setSSLConnection(nil);
	ssl.setDebug(getDebug());
	ssl.initialize(util::EContextType::ECT_CLIENT);
	ssl.verify(util::EContextType::ECT_CLIENT, sslVerifyCallback);
	ssl.mode(SSL_MODE_ENABLE_PARTIAL_WRITE);
}

void TTLSClientSocket::readConfig()
{
	if (getDebug()) std::cout << "TTLSClientSocket::readConfig(\"" + getName() + "\")" << std::endl;
	app::PIniFile config = getConfig();
	if (util::assigned(config) && !getName().empty()) {
		config->setSection(getName());
		std::string _ciphers = config->readString("Ciphers", util::TLS_PREFERRED_CIPHERS);
		ciphers = (_ciphers != "default") ? _ciphers : "";
		std::string _issuer = config->readString("Issuer", "any");
		issuer = (_issuer != "any") ? _issuer : "";
	}
}

void TTLSClientSocket::writeConfig()
{
	app::PIniFile config = getConfig();
	if (util::assigned(config) && !getName().empty()) {
		config->setSection(getName());
		config->writeString("Ciphers", (ciphers.empty()) ? "default" : ciphers);
		config->writeString("Issuer", (issuer.empty()) ? "any" : issuer);
	}
}

void TTLSClientSocket::reWriteConfig()
{
	readConfig();
	writeConfig();
}


bool TTLSClientSocket::open(const std::string& serverIP, const int port, const inet::EAddressFamily family) {
	reWriteConfig();
	ssl.setDebug(getDebug());
	bool retVal = TClientSocket::open(serverIP, port, family);
	writeLogFmt("TTLSClientSocket::open()::TClientSocket::open() called, result = %", retVal);
	if (retVal)
		startTLS();
	return retVal;
}

bool TTLSClientSocket::open() {
	bool retVal = TClientSocket::open();
	writeLogFmt("TTLSClientSocket::open()::TClientSocket::open() called, result = %", retVal);
	if (retVal)
		startTLS();
	return retVal;
}

void TTLSClientSocket::startTLS() {
	writeLog("TTLSClientSocket::startTLS() Starting TLS negotiation.");
	if (isOpen() && isConnected()) {
		util::PSSLConnection connection = getSSLConnection();
		if (!util::assigned(connection)) {
			writeLog("TTLSClientSocket::startTLS() Create new TLS client context.");
			connection = new util::TSSLConnection;
			connection->setDebug(getDebug());
			connection->initialize(&ssl);
			connection->setCaller(this, util::SSLInit.getIndex());
			if (!ciphers.empty()) {
				if (!connection->ciphers(ciphers))
					throw util::app_error_fmt("TTLSClientSocket::startTLS($) Setting ciphers $ failed: $", getName(), ciphers, connection->errmsg());
			}
			setSSLConnection(connection);
		}

		bool retVal = connection->assign(handle());
		if (retVal)
			writeLogFmt("TTLSClientSocket::startTLS() Set socket descriptor <%>", handle());

		if (retVal) {
			retVal = connection->connect();
			if (retVal)
				writeLogFmt("TTLSClientSocket::startTLS() Connect TLS socket <%>", handle());
		}

		if (!retVal) {
			close();
			throw util::app_error_fmt("TTLSClientSocket::startTLS($) failed: $", getName(), connection->errmsg());
		}

		// TLS negotiation successful
		writeLog("TTLSClientSocket::startTLS() TLS socket successfully negotiated with the following credentials:");
		writeLogFmt("TTLSClientSocket::startTLS()   Subject $", connection->getSubject());
		writeLogFmt("TTLSClientSocket::startTLS()   Issuer  $", connection->getIssuer());
	}
}

void TTLSClientSocket::close() {
	if (hasSSLConnection()) {
		getSSLConnection()->close();
		clearSSLConnection();
	}
	TClientSocket::close();
}

ssize_t TTLSClientSocket::send(void const * const data, size_t const size) const {
	if (hasSSLConnection())
		return getSSLConnection()->send(data, size);
	return (ssize_t)0;
}

ssize_t TTLSClientSocket::receive(void * const data, size_t const size) const {
	if (hasSSLConnection())
		return getSSLConnection()->receive(data, size);
	return (ssize_t)0;
}

void TTLSClientSocket::reconnect() {
	TClientSocket::reconnect();
}

ssize_t TTLSClientSocket::executeDataWrapper(PSocketConnection connection, app::THandle client) {
	if (getDebug()) std::cout << "TTLSClientSocket::executeDataWrapper(" << util::quote(getName()) << ") called." << std::endl;
	return executeDataMethod();
}

ssize_t TTLSClientSocket::executeDataMethod() {
	if (getDebug()) std::cout << "TTLSClientSocket::executeDataMethod(" << util::quote(getName()) << ") called." << std::endl;
	try {
		if (onClientData != nil) {
			ssize_t r = onClientData(*this);
			if (r <= 0) {
				errorLogFmt("TTLSClientSocket::executeDataMethod() Result = %", r);
				if (hasSSLConnection()) {
					errorLogFmt("TTLSClientSocket::executeDataMethod() Read error = %", getSSLConnection()->recerr());
					errorLogFmt("TTLSClientSocket::executeDataMethod() Error Message $", getSSLConnection()->errmsg());
				}
			}
			return r;
		}
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		errorLogFmt("TTLSClientSocket::executeDataMethod() failed: $", sExcept);
	} catch (...)	{
		errorLog("TTLSClientSocket::executeDataMethod() Unknown exception.");
	}
	return (ssize_t)EXIT_ERROR;
}

int TTLSClientSocket::verify(int verified, X509_STORE_CTX *ctx, SSL * ssl, X509 * crt, const std::string& issuer, int depth, int error) {
	if (!this->issuer.empty()) {
		if (this->issuer != issuer) {
			verified = 0;
			errorLogFmt("TTLSClientSocket::verify() failed: Issuer $ denied.", issuer);
		} else {
			writeLogFmt("TTLSClientSocket::verify() Issuer $ accepted.", issuer);
		}
	}
	return verified;
}




TUDPSocket::TUDPSocket(const bool debug) : TSocket(debug) {
	prime();
	setDatagram();
}

TUDPSocket::~TUDPSocket() {
}

void TUDPSocket::prime() {
	onUDPData = nil;
	unicast = false;
}

bool TUDPSocket::open(const std::string& sendTo, const int port, const inet::EAddressFamily family) {
	inet::ESocketType type = inet::ESocketType::ST_DGRAM;
	std::string _host = sendTo;
	int _port = port;
	socket.reWriteConfig(_host, _port, family);
	std::string _service = std::to_string((size_s)_port);

	// Calculate send address
	receiver.setType(type);
	receiver.setFamily(family);
	if (!_host.empty()) {
		if (!receiver.getAddress(_host, _service)) {
			throw util::app_error_fmt("TUDPSocket::open::getAddress(%) failed to get receive address for host <%>", getDescription(), _host);
		}
	}

	// Open UDP socket for all local addresses
	if (socket.open("", _service, type, family)) {
		if (socket.bind()) {
			writeLogFmt("TUDPSocket::open() Datagram socket <%> for Port <%> opened successfully.", handle(), getService());
			if (getDebug())
				socket.debugOutput();

			// UDP "Server" socket is open
			// --> No listening here, it's a datagram socket...
			return true;
		}
	}
	return false;
}

ssize_t TUDPSocket::executeDataWrapper(PSocketConnection connection, app::THandle client) {
	if (client > 0)
		return executeDataMethod(client);
	throw util::app_error_fmt("TUDPSocket::executeDataWrapper(%) failed: Invalid parameter.", getName());
}

ssize_t TUDPSocket::executeDataMethod(app::THandle client) {
	try {
		if (onUDPData != nil)
			return onUDPData(*this);
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		socket.errorLogFmt("TUDPSocket::executeDataMethod() for client socket <%> failed: $", client, sExcept);
	} catch (...)	{
		socket.errorLog("TUDPSocket::executeDataMethod() for client socket <%> failed on unknown exception.");
	}
	return (ssize_t)EXIT_ERROR;
}

ssize_t TUDPSocket::send(void const * const data, size_t const size, int const flags) const {
	if (!receiver.valid())
		return (ssize_t)EXIT_ERROR;
	return socket.sendTo(data, size, receiver()->ai_addr, receiver()->ai_addrlen, flags);
}

ssize_t TUDPSocket::sendTo(void const * const data, size_t const size, const TAddressInfo& to, int const flags) const {
	return socket.sendTo(data, size, &to.ai_addr, to.ai_addrlen, flags);
}

ssize_t TUDPSocket::receive(void * const data, size_t const size, int const flags) const {
	if (getDebug()) std::cout << " TUDPSocket::receive() called." << std::endl;
	TAddressInfo client;
	return receiveFrom(data, size, client, flags);
}


ssize_t TUDPSocket::receiveFrom(void * const data, size_t const size, TAddressInfo& from, int const flags) const {
	if (getDebug()) std::cout << " TUDPSocket::receiveFrom() called." << std::endl;

	ssize_t r = socket.receiveFrom(data, size, &from.ai_addr, &from.ai_addrlen, flags);
	if (getDebug()) {
		std::cout << " TUDPSocket::receiveFrom() Size = " << size << ", result = " << r << std::endl;
		if (r < 0)
			std::cout << " TUDPSocket::receiveFrom() Error = " << socket.recerr() << " \"" << sysutil::getSysErrorMessage(socket.recerr()) << "\""<< std::endl;
	}

	// Address is valid?
	from.assigned = r >= 0;

	// Check if data received from known host...
	if (unicast) {
		if (!receiver.compare(from)) {
			if (getDebug()) std::cout << " TUDPSocket::receiveFrom()::compare() failed." << std::endl;
			return (ssize_t)0;
		}
	}

	return r;
}



TMulticastSocket::TMulticastSocket(const bool debug) : TUDPSocket(debug) {
}

TMulticastSocket::~TMulticastSocket() {
}


bool TMulticastSocket::setOption(const int level, const int option, void const * const value, const size_t size) {
	if (isOpen()) {
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = setsockopt(handle(), level, option, value, size);
		} while (r == EXIT_ERROR && errno == EINTR);
		return true;
	}
	return false;
}


bool TMulticastSocket::open(const std::string& mcast, const int port, const inet::EAddressFamily family) {
	if (mcast.empty())
		return false;

	ESocketType type = ESocketType::ST_DGRAM;
	std::string _host = mcast;
	int _port = port;
	socket.reWriteConfig(_host, _port, family);
	std::string _service = std::to_string((size_s)_port);

	// Calculate multicast receiver address
	setHost(mcast);
	receiver.setType(type);
	receiver.setFamily(family);
	if (!_host.empty()) {
		if (!receiver.getAddress(_host, _service)) {
			throw util::app_error_fmt("TMulticastSocket::open::getAddress(%) failed to get receive address for multicast group <%>", getDescription(), _host);
		}
	}

	// Open UDP socket for all local addresses
	if (socket.open("", _service, type, family)) {

		// Set TTL value
		int ttl = 3;
		if (!setOption(IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl))) {
			socket.close();
			throw util::sys_error_fmt("TMulticastSocket::open(%) Setting TTL failed for multicast group <%>", getDescription(), _host);
		}

		if (!socket.bind()) {
			socket.close();
			throw util::sys_error_fmt("TMulticastSocket::open(%) Binding failed for multicast group <%>", getDescription(), _host);
		}

		// Join multicast group
		struct ip_mreq mreq;
	    mreq.imr_multiaddr.s_addr = inet_addr(_host.c_str());
	    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if (!setOption(IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) {
			socket.close();
			throw util::sys_error_fmt("TMulticastSocket::open(%) Joining multicast group failed for multicast group <%>", getDescription(), _host);
		}

		writeLogFmt("TMulticastSocket::open() Socket <%> for multicast group <%> on port <%> opened successfully.", handle(), _host, getService());
		if (getDebug())
			socket.debugOutput();
		return true;

	}
	return false;
}



TUnixServerSocket::TUnixServerSocket(const bool debug) : TSocket(debug) {
	prime();
	setUnix();
	setServer();
}

TUnixServerSocket::~TUnixServerSocket() {
}

void TUnixServerSocket::prime() {
	onServerData = nil;
}

bool TUnixServerSocket::open(const std::string& device) {
	int _port = 0;
	std::string _device = device;
	inet::EAddressFamily family = EAddressFamily::AT_UNIX;
	socket.reWriteConfig(_device, _port, family);
	if (socket.open(_device, "", inet::ESocketType::ST_STREAM, family)) {
		if (socket.bind()) {
			writeLogFmt("TUnixServerSocket::open() Server socket <%> for Port <%> opened successfully.", handle(), getHost());

			if (socket.listen()) {
				if (isListening()) {
					if (getDebug()) {
						writeLogFmt("TUnixServerSocket::open() Server socket <%> listening on port <%>", handle(), getHost());
						socket.debugOutput();
					}

					// Server socket is listening
					return true;
				}
			}
			if (getDebug()) std::cout << std::endl;
		}
	}
	return false;
}

ssize_t TUnixServerSocket::executeDataWrapper(PSocketConnection connection, app::THandle client) {
	if (client > 0)
		return executeDataMethod(client);
	throw util::app_error_fmt("TUnixServerSocket::executeDataWrapper(%) failed: Invalid parameter.", getName());
}

ssize_t TUnixServerSocket::executeDataMethod(app::THandle client) {
	try {
		if (onServerData != nil) {
			return onServerData(*this, client);
		}
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		socket.errorLogFmt("TUnixServerSocket::executeDataMethod() for client socket <%> failed: $", client, sExcept);
	} catch (...)	{
		socket.errorLogFmt("TUnixServerSocket::executeDataMethod() for client socket <%> failed on unknown exception.", client);
	}
	return (ssize_t)EXIT_ERROR;
}

ssize_t TUnixServerSocket::send(const app::THandle hnd, void const * const data, size_t const size, int const flags) const {
	return socket.send(hnd, data, size, flags);
}

ssize_t TUnixServerSocket::receive(const app::THandle hnd, void * const data, size_t const size, int const flags) const {
	return socket.receive(hnd, data, size, flags);
}



TUnixClientSocket::TUnixClientSocket(const bool debug) : TSocket(debug) {
	prime();
	setUnix();
	setClient();
}

TUnixClientSocket::~TUnixClientSocket() {
}

void TUnixClientSocket::prime() {
	onClientData = nil;
}

bool TUnixClientSocket::open(const std::string& device) {
	bool retVal = false;
	int _port = 0;
	std::string _device = device;
	inet::EAddressFamily family = EAddressFamily::AT_UNIX;
	socket.reWriteConfig(_device, _port, family);
	if (_device.empty()) {
		throw util::app_error("TUnixClientSocket::open() Open failed, no server address given.");
	}
	if (socket.open(_device, "", inet::ESocketType::ST_STREAM, family)) {
		writeLogFmt("TUnixClientSocket::open() Client socket <%> for [%] opened successfully.", handle(), getHost());
		if (getDebug())
			socket.debugOutput();

		socket.connect();
		if (isConnected()) {
			retVal = true;
			writeLogFmt("TUnixClientSocket::open() Client socket <%> successfully connected to [%]", handle(), getHost());
		} else {
			// Connect failed, try to reconnect...
			reconnect();
		}

	}
	return retVal;
}

bool TUnixClientSocket::open() {
	bool retVal = false;

	// Try to open socket again
	if (!isOpen()) {
		if (socket.open(getHost(), getService(), inet::ESocketType::ST_STREAM, getFamily())) {
			writeLogFmt("TUnixClientSocket::open() Client socket <%> reopened for [%]", handle(), getHost());
			if (getDebug())
				socket.debugOutput();
		}
	}
	if (isOpen()) {

		// Try to connect client socket to server
		socket.connect();

		if (isConnected()) {
			retVal = true;
			writeLogFmt("TUnixClientSocket::open() Client socket <%> successfully connected to [%]", handle(), getHost());
		} else {
			// Connect failed, close client socket!
			if (wasConnected())
				close();
		}
	}
	return retVal;
}

ssize_t TUnixClientSocket::executeDataWrapper(PSocketConnection connection, app::THandle client) {
	return executeDataMethod();
}

ssize_t TUnixClientSocket::executeDataMethod() {
	try {
		if (onClientData != nil)
			return onClientData(*this);
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		socket.errorLogFmt("TUnixClientSocket::executeDataMethod() failed: $", sExcept);
	} catch (...)	{
		socket.errorLog("TUnixClientSocket::executeDataMethod() failed on unknown exception.");
	}
	return (ssize_t)EXIT_ERROR;
}

ssize_t TUnixClientSocket::send(void const * const data, size_t const size, int const flags) const {
	return socket.send(data, size, flags);
}

ssize_t TUnixClientSocket::receive(void * const data, size_t const size, int const flags) const {
	return socket.receive(data, size, flags);
}



} /* namespace inet */
