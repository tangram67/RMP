/*
 * websockets.cpp
 *
 *  Created on: 19.06.2020
 *      Author: dirk
 *
 *  See: https://tools.ietf.org/html/rfc6455
 *
 */

#include "websockets.h"
#include "compare.h"
#include "typeid.h"
#include "ASCII.h"

namespace app {

/*
 * Local signal save wrapper for system calls
 */
int __s_close(app::THandle socket) {
	int r = EINVAL;
	if (socket != INVALID_HANDLE_VALUE) {
		do {
			errno = EXIT_SUCCESS;
			r = ::close(socket);
		} while (r == EXIT_ERROR && errno == EINTR);
	}
	return r;
}

int __s_shutdown(app::THandle socket, int how) {
	// SHUT_RD   = No more receptions;
	// SHUT_WR   = No more transmissions;
	// SHUT_RDWR = No more receptions or transmissions.
	int r = EINVAL;
	if (socket != INVALID_HANDLE_VALUE) {
		do {
			errno = EXIT_SUCCESS;
			r = ::shutdown(socket, how);
		} while (r == EXIT_ERROR && errno == EINTR);
	}
	return r;
}


TWebSockets::TWebSockets(const std::string name, app::PThreadController threads, app::PLogFile logger, app::PIniFile config) {
	prime();
	this->threads = threads;
	this->logger = logger;
	this->config = config;
	this->instance = name;
	if (util::assigned(config)) {
		reWriteConfig();
	}
}

TWebSockets::~TWebSockets() {
	closeEpollHandle();
}

void TWebSockets::prime() {
	debug = false;
	secure = false;
	running = false;
	shutdown = false;
	invalidated = false;
	mtu = WEBSOCKET_MAX_SEND_SIZE;
	mask = inet::SOCKET_EPOLL_MASK;
	epollfd = INVALID_HANDLE_VALUE;
	onWebSocketConnect = nil;
	onWebSocketVariant = nil;
	onWebSocketData = nil;
	timeout = 720;
	revents = nil;
	logger = nil;
	thread = nil;
	ecount = 0;
	scount = 0;
	rsize = 0;
	removed = 0;
}

void TWebSockets::readConfig() {
	config->setSection("WebSockets");
	debug = config->readBool("Debug", debug);
	timeout = config->readInteger("Timeout", timeout);
	mtu = config->readInteger("MTU", mtu);
}

void TWebSockets::writeConfig() {
	config->setSection("WebSockets");
	config->writeBool("Debug", debug, app::INI_BLYES);
	config->writeInteger("Timeout", timeout);
	config->writeInteger("MTU", mtu);
}

void TWebSockets::reWriteConfig() {
	readConfig();
	writeConfig();
}


void TWebSockets::writeLog(const std::string& s) const {
	if (getDebug()) std::cout << s << std::endl;
	if (util::assigned(logger)) {
		logger->write("[" + instance + "] [Websockets] " + s);
	}
}

void TWebSockets::errorLog(const std::string& s) const {
	if (getDebug()) std::cout << s << std::endl;
	if (util::assigned(logger)) {
		logger->write("[" + instance + "] [Websockets] [Error] " + s);
	}
}


void TWebSockets::incEpollCount() {
	++ecount;
}

void TWebSockets::decEpollCount() {
	if (ecount)
		--ecount;
}


app::THandle TWebSockets::createEpollHandle(int flags) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		if (flags > 0) {
			r = ::epoll_create1(flags);
		} else {
			r = ::epoll_create(99);
		}
	} while (r == EXIT_ERROR && errno == EINTR);

	if (r >= 0)
		epollfd = r;

	return epollfd;
}

void TWebSockets::closeEpollHandle() {
	if (epollfd >= 0)
		__s_close(epollfd);
	epollfd = INVALID_HANDLE_VALUE;
}


void TWebSockets::upgrade(MHD_socket socket, struct MHD_UpgradeResponseHandle *urh, const char *data, size_t size) {
	// Create web socket structure
	CWebSocket* ws = new CWebSocket;
	ws->handle = socket;
	ws->valid = true;
	ws->urh = urh;

	// Create new polling event
	epoll_event * p = new epoll_event;
	p->data.ptr = (void *)ws;
	p->events = mask;
	ws->event = p;

	// Add new websocket to polling list
	addSocketHandle(ws);
	onSockectConnected(socket);

	// Process given data
	if (util::assigned(data) && size > 0) {
		bool closed = false;
		std::string decoded;
		util::TVariantValues variants;
		decodeSocketData(socket, data, size, decoded, variants, closed);
		if (!decoded.empty()) {
			processSocketData(socket, decoded, variants);
		}
	}
}


size_t TWebSockets::getSocketHandles(TWebHandleList& handles) const {
	app::TLockGuard<app::TMutex> mtx(listMtx);
	handles.clear();
	if (!sockets.empty()) {
		for (size_t i=0; i<sockets.size(); ++i) {
			PWebSocket socket = sockets[i];
			if (util::assigned(socket)) {
				if (socket->handle != INVALID_HANDLE_VALUE)
					handles.push_back(socket->handle);
			}
		}
	}
	return handles.size();
}


struct CSocketRemover {
	CSocketRemover(app::TWebSockets* owner): owner(owner) {}
	app::TWebSockets* owner;
    bool operator() (PWebSocket socket) const {
    	bool retVal = true;
    	if (util::assigned(socket)) {
			retVal = false;
    		if (!socket->valid) {
				if (socket->handle != INVALID_HANDLE_VALUE) {
					if (EXIT_SUCCESS == owner->removeEpollHandle(socket->event, socket->handle)) {
						owner->writeLog("TWebSockets::removeInvalidatedSockets() Removed handle <" + std::to_string((size_s)socket->handle) + ">");
						owner->decEpollCount();
					}
					owner->close(socket);
					socket->handle = INVALID_HANDLE_VALUE;
				}
				util::freeAndNil(socket);
    			retVal = true;
    		}
    	}
    	return retVal;
    }
};

void TWebSockets::invalidateSocket(const app::THandle handle) {
	app::TLockGuard<app::TMutex> mtx(listMtx);
	if (!sockets.empty()) {
		for (size_t i=0; i<sockets.size(); ++i) {
			PWebSocket socket = sockets[i];
			if (util::assigned(socket)) {
				if (socket->handle == handle) {
					invalidateSocketWithNolock(socket);
					break;
				}
			}
		}
	}
}

void TWebSockets::invalidateSocket(PWebSocket socket) {
	app::TLockGuard<app::TMutex> mtx(listMtx);
	invalidateSocketWithNolock(socket);
}

void TWebSockets::invalidateSocketWithNolock(PWebSocket socket) {
	if (util::assigned(socket)) {
		socket->valid = false;
		++removed;
	}
}

void TWebSockets::removeInvalidatedSockets() {
	app::TLockGuard<app::TMutex> mtx(listMtx);
	if (removed > 0) {
		removed = 0;
		sockets.erase(std::remove_if(sockets.begin(), sockets.end(), CSocketRemover(this)), sockets.end());
	}
}


struct CSocketEraser {
    bool operator() (PWebSocket o) const {
    	bool retVal = true;
    	if (util::assigned(o)) {
			retVal = false;
    		if (!o->valid) {
				util::freeAndNil(o);
    			retVal = true;
    		}
    	}
    	return retVal;
    }
	CSocketEraser() {}
};

void TWebSockets::addSocket(PWebSocket socket) {
	if (util::assigned(socket)) {
		app::TLockGuard<app::TMutex> mtx(listMtx);
		sockets.push_back(socket);
		invalidated = true;
	}
}

void TWebSockets::removeSocket(PWebSocket socket) {
	if (util::assigned(socket)) {
		app::TLockGuard<app::TMutex> mtx(listMtx);
		invalidateSocketWithNolock(socket);
		sockets.erase(std::remove_if(sockets.begin(), sockets.end(), CSocketEraser()), sockets.end());
	}
	close(socket);
}

void TWebSockets::addSocketHandle(PWebSocket socket) {
	if (util::assigned(socket)) {
		if (socket->handle > 0) {
			if (EXIT_SUCCESS == addEpollHandle(socket->event, socket->handle)) {
				writeLog("TWebSockets::addEpollHandle() Added handle <" + std::to_string((size_s)socket->handle) + ">");
				addSocket(socket);
				++ecount;
			}
		}
	}
}

void TWebSockets::removeSocketHandle(PWebSocket socket) {
	if (util::assigned(socket)) {
		if (socket->handle != INVALID_HANDLE_VALUE) {
			if (EXIT_SUCCESS == removeEpollHandle(socket->event, socket->handle)) {
				writeLog("TWebSockets::addEpollHandle() Removed handle <" + std::to_string((size_s)socket->handle) + ">");
				removeSocket(socket);
				if (ecount)
					--ecount;
			}
			socket->handle = INVALID_HANDLE_VALUE;
		}
	}
}

int TWebSockets::addEpollHandle(epoll_event* event, const app::THandle socket) {
	if (debug) writeLog("TWebSockets::addEpollHandle() Add handle <" + std::to_string((size_s)socket) + ">");
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::epoll_ctl(epollfd, EPOLL_CTL_ADD, socket, event);
	} while (r != EXIT_SUCCESS && errno == EINTR);
	return r;
}

int TWebSockets::removeEpollHandle(epoll_event* event, const app::THandle socket) {
	if (debug) writeLog("TWebSockets::removeEpollHandle() Remove handle <" + std::to_string((size_s)socket) + ">");
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::epoll_ctl(epollfd, EPOLL_CTL_DEL, socket, event);
	} while (r != EXIT_SUCCESS && errno == EINTR);
	return r;
}


int TWebSockets::epoll() {
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


void TWebSockets::start() {
	if (epollfd < 0) {
		if (createEpollHandle() < 0)
			throw util::sys_error("TWebSockets::start()::createEpollHandle() failed.");
		writeLog("TWebSockets::start()::createEpollHandle() Epoll file descriptor <" + std::to_string((size_s)epollfd) + ">");
	}
	if (!util::assigned(thread)) {
		running = true;
		app::EThreadStartType execute = running ? app::THD_START_ON_CREATE : app::THD_START_ON_DEMAND;
		thread = threads->addThread("WebSocket-Poll",
									&app::TWebSockets::eloop,
									this, execute);
	}
}


size_t TWebSockets::createEpollEvents() {
	app::TLockGuard<app::TMutex> mtx(listMtx);

	// Check if sockets added to list prior to polling
	if (scount != sockets.size() || invalidated) {
		invalidated = false;
		if (debug) std::cout << "TWebSockets::createEpollEvents() Sockets added, changed or removed from list (" << scount << "/" << sockets.size() << ")" << std::endl;
		scount = sockets.size();
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
			if (debug) std::cout << "TWebSockets::createEpollEvents() Create event list with " << rsize << " entries." << std::endl;
		} else {
			if (rsize < n) {
				rsize = n;
				delete[] revents;
				revents = new epoll_event[rsize];
				resized = true;
				if (debug) std::cout << "TWebSockets::createEpollEvents() Resized event list to " << rsize << " entries." << std::endl;
			}
		}

		// Initialize new array
		if (resized) {
			memset(revents, 0, rsize * sizeof(epoll_event));
		}

	}

	return ecount;
}

void TWebSockets::debugOutputEpoll(size_t size) {
	if (util::assigned(revents) && rsize) {
		if (size == 0)
			size = rsize;
		PWebSocket socket;
		epoll_event* p = revents;
		for (size_t i=0; i<size; ++i, ++p) {
			size_t k = util::succ(i);
			if (util::assigned(p)) {
				socket = (PWebSocket)p->data.ptr;
				if (util::assigned(socket)) {
					std::cout << k << ". Handle = " << socket->handle << std::endl;
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

int TWebSockets::eloop(app::TManagedThread& sender) {
	int rcount = 0;
	epoll_event* p;
	size_t n;

	// Be sure to reset running flag...
	util::TBooleanGuard<bool> bg(running);
	do {
		// Create new polling list from master events
		n = createEpollEvents();

		// Check for data on sockets
		rcount = epoll();
		if (rcount < 0)
			throw util::sys_error("TWebSockets::eloop()::epoll() failed.");

		if (rcount > 0 && n > 0 && !shutdown && !sender.isTerminating()) {
			if (debug) {
				std::cout << "TWebSockets::eloop() Watched events           = " << n << std::endl;
				std::cout << "TWebSockets::eloop() Fired events (rcount)    = " << rcount << std::endl;
				std::cout << "TWebSockets::eloop() Poll array size (rsize)  = " << rsize << std::endl;
				std::cout << "TWebSockets::eloop() Event list size (ecount) = " << ecount << std::endl;
				std::cout << "TWebSockets::eloop() Socket count (scount)    = " << scount << std::endl;
				std::cout << "TWebSockets::eloop() Socket list size         = " << sockets.size() << std::endl << std::endl;
				debugOutputEpoll(rcount);
			}

			// Find socket with data signaled
			reader.clear();
			p = revents;
			for (int i=0; i<rcount; ++i, ++p) {
				bool invalidated = false;

				// No event on this socket
				if (p->events == 0)
					continue;

				// Get listening socket from data pointer
				PWebSocket socket = (PWebSocket)p->data.ptr;
				app::THandle handle = socket->handle;

				// Received data from socket
				if (p->events & POLLIN) {

					// Store connection in reader list
					socket->state |= POLLIN;
					reader.push_back(socket);
					if (debug) writeLogFmt("TWebSockets::eloop() Data for socket <%> available.", handle);

				} // if (p->revents & POLLIN)

				// Client has disconnected
				if (!invalidated && (p->events & POLLHUP)) {
					// Remove from current socket polling list
					// if socket is not listening socket!
					writeLogFmt("TWebSockets::eloop() Client socket <%> closed on POLLHUP.", handle);
					invalidateSocket(socket);
					invalidated = true;
				}

				// Error on socket
				if (!invalidated && (p->events & POLLERR)) {
					writeLogFmt("TWebSockets::eloop() Client socket <%> closed on POLLERR.", handle);
					invalidateSocket(socket);
					invalidated = true;
				}

				// Invalid socket descriptor
				if (!invalidated && (p->events & POLLNVAL)) {
					writeLogFmt("TWebSockets::eloop() Client socket <%> closed on POLLNVAL.", handle);
					invalidateSocket(socket);
					invalidated = true;
				}

				p->events = 0;

			} // for (i=0; i<events.size(); ++i, ++p)

			if (!shutdown) {
				// Executer reader
				roundRobinReader();

				// Removed invalid web sockets
				removeInvalidatedSockets();
			}

			if (debug) std::cout << "TWebSockets::eloop() End of polling events." << std::endl;

		} // if (r > 0)

		//if (debug) util::wait(500);

	} while (!shutdown && !sender.isTerminating());


	// Close/Shutdown all client connections
	writeLog("TWebSockets::eloop() Shutdown web socket connections.");
	close();

	// Clear EPOLL events
	writeLog("TWebSockets::eloop() Clear web socket list.");
	clear();

	writeLog("TWebSockets::eloop() Reached end of thread.");
	return EXIT_SUCCESS;
}

void TWebSockets::roundRobinReader() {
	if (!reader.empty()) {
		ssize_t r;
		bool data;
		PWebSocket socket;
		app::THandle handle;

		do {
			data = false;
			TWebSocketList::const_iterator it = reader.begin();
			while (it != reader.end()) {
				socket = *it;
				handle = socket->handle;

				if ((socket->state & POLLIN) && socket->valid) {

					if (debug) std::cout << "TWebSockets::roundRobinReader() Read data from socket <" << handle << ">" << std::endl;
					bool closed = false;
					r = readSocketData(socket, closed);

					if (r == (ssize_t)0) {

						// Data signaled for TCP socket, but no data received
						// --> Connection closed by foreign host
						if (socket->error == EXIT_SUCCESS) {
							writeLogFmt("TWebSockets::roundRobinReader() Client Socket <%> closed (EOF on POLLIN)", handle);
							invalidateSocket(socket);
						}

						// Data signaled, but socket blocked
						// --> No more data available
						if (socket->error == EWOULDBLOCK) {
							if (debug) std::cout << "TWebSockets::roundRobinReader() No more data from client Socket <" << handle << "> available." << std::endl;
						}

						// Reset read request
						socket->state &= ~POLLIN;

					} else {
						if (r > (ssize_t)0) {
							if (!isSecure() && !closed) {
								if (debug) std::cout << "TWebSockets::roundRobinReader() Data from client Socket <" << handle << "> read, size = " << r << " bytes." << std::endl;
								data = true;
							}
						} else {
							// Result < 0 --> Error on socket read
							socket->state &= ~POLLIN;
						}
					}

					// Socket was signaled to be closed soon...
					if (closed) {
						writeLogFmt("TWebSockets::roundRobinReader() Client Socket <%> closed (Close Message)", handle);
						invalidateSocket(socket);
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

ssize_t TWebSockets::readSocketData(PWebSocket socket, bool& closed) {
	if (util::assigned(socket)) {
		util::TBuffer buffer(WEBSOCKET_BUFFER_SIZE);
		util::TBuffer data;
		ssize_t received = 0;
		ssize_t read = receive(socket, buffer.data(), buffer.size());
		if (debug) std::cout << "TWebSockets::readSocketData() Received " << read << " bytes from client Socket <" << socket->handle << ">" << std::endl;

		// Check for more data in received buffers
		if (read == (ssize_t)buffer.size()) {
			// Read data needed full buffer size
			// --> Try to read at least one more data chunk
			data.reserve(2 * buffer.size(), false);
			data.append(buffer.data(), read);
			received = data.size();
			do {
				read = receive(socket, buffer.data(), buffer.size());
				if (read > 0) {
					if (read == (ssize_t)buffer.size()) {
						// One more buffer expected
						// --> Resere space for current and next buffer
						data.reserve(data.size() + 2 * buffer.size(), true);
					}
					data.append(buffer.data(), read);
					received = data.size();
					if (debug) std::cout << "TWebSockets::readSocketData() Received next " << read << "/" << received << " bytes from client Socket <" << socket->handle << ">" << std::endl;
				}
			} while (read > 0);
		} else {
			// Smaller chunk than buffer size read
			// -->  Packet is complete
			received = read;
		}

		// Parse received data
		if (received > 0) {
			if (debug) std::cout << "TWebSockets::readSocketData() Parse " << received << " bytes from client Socket <" << socket->handle << ">" << std::endl;
			std::string decoded;
			util::TVariantValues variants;
			if (received > 0) {
				decodeSocketData(socket->handle, data.empty() ? buffer.data() : data.data(), received, decoded, variants, closed);
			}
			if (!decoded.empty()) {
				processSocketData(socket->handle, decoded, variants);
			}
			if (debug) std::cout << "TWebSockets::readSocketData() Data processed for client Socket <" << socket->handle << ">" << std::endl;
		}

		return received;
	}
	return (ssize_t)EXIT_ERROR;
}

void TWebSockets::decodeSocketData(const app::THandle handle, const void *const data, const size_t size, std::string& output, util::TVariantValues& variants, bool& closed) {
	if (util::assigned(data) && size > 0) {
		// Unmask received data
		bool ping;
		util::TBuffer unmasked;
		unmaskSocketData(data, size, unmasked, ping, closed);
		if (!closed && !ping) {
			if (debug) std::cout << "TWebSockets::decodeSocketData() Unmasked " << unmasked.size() << " bytes received." << std::endl;
			if (!unmasked.empty()) {
				// Websocket data should be UTF-8 by design!
				if (util::TASCII::isValidUTF8MultiByteStr(unmasked.data(), unmasked.size())) {
					parseSocketData(unmasked.data(), unmasked.size(), output, variants);
				}
			}
		} else {
			// Check for valid ping request
			if (ping) {
				if (debug) std::cout << "TWebSockets::decodeSocketData() Ping of " << unmasked.size() << " bytes received." << std::endl;
				if (!unmasked.empty()) {
					sendPingResponse(handle, unmasked);
				}
			}
		}
	}
}

void TWebSockets::parseSocketData(const void *const data, const size_t size, std::string& output, util::TVariantValues& variants) const {
	if (util::assigned(data) && size > 2) {
		output.assign((char*)data, size);
		if (debug) std::cout << "TWebSockets::parseSocketData() Text size = " << output.size() << std::endl;
		if (!output.empty()) {
			variants.parseJSON(output);
			if (debug) {
				variants.debugOutput("TWebSockets::parseSocketData() ");
			}
		}
	}
}

bool TWebSockets::unmaskSocketData(const void* data, size_t size, util::TBuffer& output, bool& ping, bool& closed) const {
	const uint8_t* p = (uint8_t*)data;
	ping = closed = false;
	if (util::assigned(data) && size > 2) {

		// Check for masked date
		bool masked = *(p+1) & 0x80;
		size_t len = *(p+1) & ~0x80;
		if (debug) std::cout << "TWebRequest::unmaskSocketData() Length = " << len << ", masked = " << masked << std::endl;

		// Is valid header byte
		if (*p == 0x81) {
			bool ok = false;
			uint8_t key[4];

			if (masked) {
				// Check for payload length byte >= 126
				//  126 --> 2 length bytes (max. 65636 bytes block length)
				//  127 --> 4 length bytes (64 bit buffers not implemented here!!!)
				if (len >= 126) {
					size_t hi = *(p+2);
					size_t lo = *(p+3);
					len = (hi << 8) + lo;

					// Length should be start byte + length byte + 2 bytes real length + 4 byte XOR mask = 8 bytes
					if (size >= ((size_t)len + 8)) {

						// XOR decode mask
						key[0] = *(p+4);
						key[1] = *(p+5);
						key[2] = *(p+6);
						key[3] = *(p+7);

						// First payload byte
						p += 8;
						ok = true;
					}

				} else {
					// Length should be start byte, length byte + 4 byte XOR mask = 6 bytes
					if (size >= ((size_t)len + 6)) {

						// XOR decode mask
						key[0] = *(p+2);
						key[1] = *(p+3);
						key[2] = *(p+4);
						key[3] = *(p+5);

						// First payload byte
						p += 6;
						ok = true;
					}

				}

				if (ok) {
					// Decode bytes
					output.resize(len);
					for (size_t i=0; i<len; ++i) {
						output[i] = *p++ ^ key[i % 4];
					}
					return true;
				}

			} else { // if (masked)

				// Check for payload length byte >= 126
				//  126 --> 2 length bytes (max. 65636 bytes block length)
				//  127 --> 4 length bytes (64 bit buffers not implemented here!!!)
				if (len >= 126) {
					size_t hi = *(p+2);
					size_t lo = *(p+3);
					len = (hi << 8) + lo;

					// Length should be start byte, length byte + 2 bytes real length + = 4 bytes
					if (size >= ((size_t)len + 4)) {

						// First payload byte
						p += 4;
						ok = true;
					}

				} else {
					// Length should be start byte, length byte + = 2 bytes
					if (size >= ((size_t)len + 2)) {

						// First payload byte
						p += 2;
						ok = true;
					}

				}

				if (ok) {
					// Copy payload bytes
					output.resize(len);
					uint8_t* q = (uint8_t*)output.data();
					memcpy(q, p, len);
					return true;
				}

			} // else if (masked)
		} else {
			if ((*p == 0x89) || (*p == 0x09)) {
				// Copy whole ping request
				output.assign(data, size);
				ping = true;
				if (debug) std::cout << app::yellow << "TWebSockets::unmaskSocketData() Ping frame detected." << app::reset << std::endl;
			} else {
				// Close frame received
				if ((*p == 0x88) || (*p == 0x08)) {
					closed = true;
					if (debug) std::cout << app::red << "TWebSockets::unmaskSocketData() Close frame detected." << app::reset << std::endl;
				}
			}
		}

		if (!closed && debug) {
			std::cout << app::yellow << "TWebSockets::unmaskSocketData() Non data frame [" << util::cprintf("0x%X", *p)  << "] \"" << util::TBinaryConvert::binToText(data, size, util::TBinaryConvert::EBT_HEX) << "\" size = " << size << app::reset << std::endl;
		}
	}
	return false;
}

void TWebSockets::maskSocketData(const void* data, size_t size, util::TBuffer& output, const bool masked) const {
	if (util::assigned(data) && size > 0) {
		const uint8_t* q = (uint8_t*)data;
		uint8_t* p = nil;
		bool ok = false;

		// XOR encode mask
		uint8_t key[4];
		if (masked) {
			key[0] = 1 + (rand() % 255);
			key[1] = 1 + (rand() % 255);
			key[2] = 1 + (rand() % 255);
			key[3] = 1 + (rand() % 255);
		}

		// Encode huge block up from 0x7E == 126 bytes == ASCII "~" ?
		if (size >= 126) {

			// Set output buffer size
			size_t len = masked ? size + 8 : size + 4;
			output.resize(len);
			p = (uint8_t*)output.data();

			// Set header
			*(p+0) = 0x81;

			// Store payload length bytes
			size_t lo = size & 0xFF;
			size_t hi = (size >> 8) & 0xFF;
			*(p+2) = hi;
			*(p+3) = lo;

			// Set masked header
			if (masked) {
				*(p+1) = 0xFE; // 126 + 0x80 = 0xFE --> Mask Bit 7 set!

				// Store keys in result
				*(p+4) = key[0];
				*(p+5) = key[1];
				*(p+6) = key[2];
				*(p+7) = key[3];

				// First payload byte
				p = (uint8_t*)(output.data() + 8);

			} else {
				// Unmasked 2 byte size block
				*(p+1) = 0x7E; // 126 = 0x7E

				// First payload byte
				p = (uint8_t*)(output.data() + 4);
			}

			ok = true;

		} else {
			// Set output buffer size for block smaller that 126 bytes
			size_t len = masked ? size + 6 : size + 2;
			output.resize(len);
			p = (uint8_t*)output.data();

			// Set header
			*(p+0) = 0x81;

			// Set masked header
			if (masked) {
				*(p+1) = size | 0x80; // Mask Bit 7 set!

				// Store keys in result
				*(p+2) = key[0];
				*(p+3) = key[1];
				*(p+4) = key[2];
				*(p+5) = key[3];

				// First payload byte
				p = (uint8_t*)(output.data() + 6);

			} else {
				// Store native size
				*(p+1) = size;
				p = (uint8_t*)(output.data() + 2);
			}

			ok = true;

		}

		if (ok && util::assigned(p)) {
			if (masked) {
				// Encode bytes
				for (size_t i=0; i<size; ++i) {
					*p++ = *q++ ^ key[i % 4];
				}
			} else {
				// Plain buffer copy...
				memcpy(p, q, size);
			}
		}
	}
}


void TWebSockets::processSocketData(const app::THandle handle, const std::string& output, const util::TVariantValues& variants) {
	if (!shutdown) {
		if (!output.empty() && nil != onWebSocketData) {
			if (debug) std::cout << "TWebSockets::processSocketData() Execute OnWebSocketData for client Socket <" << handle << ">" << ", size = " << output.size() << std::endl;
			try {
				// Execute data event method
				onWebSocketData(handle, output);
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				errorLogFmt("TWebSockets::onSocketData() Process message from socket <%> failed: $", handle, sExcept);
			} catch (...)	{
				errorLogFmt("TWebSockets::onSocketData() Process message from socket <%> failed on unknown exception.", handle);
			}
		}
		if (!variants.empty() && nil != onWebSocketVariant) {
			if (debug) std::cout << "TWebSockets::processSocketData() Execute onWebSocketVariant for client Socket <" << handle << ">" << ", size = " << variants.size() << std::endl;
			try {
				// Execute data event method
				onWebSocketVariant(handle, variants);
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				errorLogFmt("TWebSockets::onSocketData() Process variants from socket <%> failed: $", handle, sExcept);
			} catch (...)	{
				errorLogFmt("TWebSockets::onSocketData() Process variants from socket <%> failed on unknown exception.", handle);
			}
		}
	}
}


void TWebSockets::onSockectConnected(const app::THandle handle) {
	if (onWebSocketConnect != nil) {
		if (debug) std::cout << "TWebSockets::onSockectConnected() Add new client socket <" << handle << ">" << std::endl;
		try {
			onWebSocketConnect(handle);
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::string sName = util::nameOf(onWebSocketConnect);
			std::string sText = "Exception in TWebSockets connect handler [" + sName + "] : " + sExcept;
			errorLog(sText);
		} catch (...)	{
			std::string sName = util::nameOf(onWebSocketConnect);
			std::string sText = "Unknown exception in TWebSockets connect handler [" + sName + "]";
			errorLog(sText);
		}
	}
}


ssize_t TWebSockets::write(const app::THandle handle, void const * const data, size_t const size) {
	if (util::assigned(data) && size > 0) {

		if (size > mtu)
			throw util::app_error_fmt("TWebSockets::write() Write size % exceeds fragmentation limit %", size, mtu);

		// Mask JSON text data, but do not encode it (!)
		if (debug) std::cout << "TWebSockets::write() Data \"" << util::TBinaryConvert::binToText(data, size, util::TBinaryConvert::EBT_HEX) << "\" size = " << size << std::endl;
		util::TBuffer encoded;
		maskSocketData(data, size, encoded, false);
		if (!encoded.empty()) {
			app::TLockGuard<app::TMutex> lock(sendMtx);
			return send(handle, encoded.data(), encoded.size());
		}
	}
	return (ssize_t)0;
}


ssize_t TWebSockets::sendPingResponse(const app::THandle handle, util::TBuffer& message) {
	ssize_t retVal = 0;
	if (!message.empty()) {
		// Set "Pong" header and send unmodified data back to sender
		uint8_t* p = (uint8_t*)message.data();
		bool fin = *p & 0x80;
		*p = fin ? 0x8A : 0x0A;
		retVal = send(handle, p, message.size());
		writeLogFmt("TWebSockets::sendPingResponse() Send pong to client <%>", handle);
	}
	return retVal;;
}


ssize_t TWebSockets::broadcast(void const * const data, size_t const size) {
	ssize_t retVal = 0;
	if (util::assigned(data) && size > 0) {
		timestamp = util::now();

		if (size > mtu)
			throw util::app_error_fmt("TWebSockets::write() Write size % exceeds fragmentation limit %", size, mtu);

		TWebHandleList handles;
		size_t count = getSocketHandles(handles);
		if (count > 0) {

			// Mask JSON text data
			if (debug) std::cout << "TWebSockets::broadcast() Data \"" << util::TBinaryConvert::binToText(data, size, util::TBinaryConvert::EBT_HEX) << "\" size = " << size << " to " << handles.size() << " clients." << std::endl;
			util::TBuffer encoded;
			maskSocketData(data, size, encoded, false);
			if (!encoded.empty()) {
				app::TLockGuard<app::TMutex> lock(sendMtx);
				for (size_t i=0; i<handles.size(); ++i) {
					THandle handle = handles[i];
					ssize_t r = send(handle, encoded.data(), encoded.size());
					if (r > 0)
						retVal += r;
				}

			}
		}
	}
	return retVal;;
}


void TWebSockets::terminate() {
	shutdown = true;
	if (util::assigned(thread))
		thread->setTerminate(true);
}


ssize_t TWebSockets::receive(PWebSocket socket, void * const data, size_t const size, int const flags) const {
	ssize_t r;
	THandle hnd = socket->handle;

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
    socket->error = errno;
	socket->timestamp = util::now();

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
	if (EXIT_SUCCESS == errno) {
		socket->error = errno = EIO;
	}

	return (ssize_t)EXIT_ERROR;
}

ssize_t TWebSockets::send(const app::THandle handle, void const * const data, size_t const size, int const flags) {
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
    		r = ::send(handle, p, (size_t)(q - p), flags);
    	} while (r == (ssize_t)EXIT_ERROR && errno == EINTR);

    	// Write failed
    	if (r == (ssize_t)EXIT_ERROR) {
    		if (errno == EBADF || errno == EPIPE) {
    			// Socket not exisiting or closed...
				writeLogFmt("TWebSockets::send() Client socket <%> dead.", handle);
    			invalidateSocket(handle);
    		}
        	return (ssize_t)EXIT_ERROR;
    	}

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


void TWebSockets::waitFor() {
	while (running)
		util::wait(50);
	if (util::assigned(thread))
		while (!thread->isTerminated())
			util::wait(75);
}

bool TWebSockets::isTerminated() const {
	return !running && thread->isTerminated();
}

void TWebSockets::downgrade(PWebSocket socket) {
	if (util::assigned(socket)) {
		if (util::assigned(socket->urh)) {
			// Close handle via libmicrohttpd
			MHD_upgrade_action(socket->urh, MHD_UPGRADE_ACTION_CLOSE);
			socket->urh = nil;
			if (debug) writeLog("TWebSockets::downgrade() Client Socket downgraded.");
		}
	}
}

void TWebSockets::close(PWebSocket socket) {
	if (util::assigned(socket)) {
		// Shutdown socket
		downgrade(socket);
		if (socket->handle != INVALID_HANDLE_VALUE) {
			if (EXIT_SUCCESS == __s_shutdown(socket->handle, SHUT_RDWR)) {
				if (debug) writeLogFmt("TWebSockets::close() Client Socket <%> shutdown.", socket->handle);
				socket->handle = INVALID_HANDLE_VALUE;
			}
		}
	}
}

void TWebSockets::close() {
	app::TLockGuard<app::TMutex> mtx(listMtx);
	for (auto o : sockets) {
		close(o);
	}
}

void TWebSockets::clear() {
	app::TLockGuard<app::TMutex> mtx(listMtx);
	for (auto o : sockets) {
		close(o);
		util::freeAndNil(o);
	}
	sockets.clear();
	if (util::assigned(revents)) {
		delete[] revents;
		revents = nil;
	}
}

} /* namespace app */
