/*
 * socketlists.h
 *
 *  Created on: 27.05.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef SOCKETLISTS_H_
#define SOCKETLISTS_H_

#include <string>
#include <sys/poll.h>
#include "../config.h"
#include "convert.h"
#include "array.h"

#ifdef HAS_EPOLL
#  include <sys/epoll.h>
#endif


namespace inet {


class TPollEvent : public util::TArray<pollfd> {
private:
	typedef util::TArray<pollfd> poll_t;

public:
	void add(pollfd * item) {
		poll_t::add(item);
	}

	bool add(const pollfd& item) {
		size_t idx = find(item.fd);
		if (std::string::npos == idx) {
			poll_t::object_p o = new pollfd;
			*o = item;
			poll_t::add(o);
			return true;
		}
		return false;
	}

	bool add(app::THandle handle, short int events = 0) {
		size_t idx = find(handle);
		if (std::string::npos == idx) {
			poll_t::object_p o = new pollfd;
			memset(o, 0, sizeof(pollfd));
			o->fd = handle;
			o->events = events;
			poll_t::add(o);
			return true;
		}
		return false;
	}

	size_t find(const app::THandle handle) const {
		poll_t::object_p o;
		for (size_t i=0; i<size(); ++i) {
			o = poll_t::vector_t::at(i);
			if (handle == o->fd)
				return i;
		}
		return std::string::npos;
	}

	bool eraseByHandle(app::THandle handle) {
		size_t index = find(handle);
		if (std::string::npos != index) {
			poll_t::object_p p = poll_t::vector_t::at(index);
			util::freeAndNil(p);
			poll_t::vector_t::erase(poll_t::vector_t::begin() + index);
			poll_t::invalidate();
		}
		return false;
	}

	bool eraseByIndex(size_t index) {
		if (validIndex(index)) {
			poll_t::object_p p = poll_t::vector_t::at(index);
			util::freeAndNil(p);
			poll_t::vector_t::erase(poll_t::vector_t::begin() + index);
			poll_t::invalidate();
		}
		return false;
	}

	void assign(const poll_t& list) {
		// Reuse objects instead of clearing local list
		for (size_t i=0; i<list.size(); ++i) {
			poll_t::object_p q, p = list.at(i);
			if (validIndex(i)) {
				q = at(i);
				*q = *p;
			} else {
				q = new pollfd;
				*q = *p;
				poll_t::add(q);
			}
		}
		while (size() > list.size()) {
			eraseByIndex(util::pred(size()));
		}
	}

	void assign(const poll_t& list, short int events) {
		// Reuse objects instead of clearing local list
		for (size_t i=0; i<list.size(); ++i) {
			poll_t::object_p q, p = list.at(i);
			if (validIndex(i)) {
				q = at(i);
				q->fd = p->fd;
				q->events = events;
				q->revents = 0;
			} else {
				q = new pollfd;
				q->fd = p->fd;
				q->events = events;
				q->revents = 0;
				poll_t::add(q);
			}
		}
		while (size() > list.size()) {
			eraseByIndex(util::pred(size()));
		}
	}

	void debugOutput() {
		pollfd* p = array();
		for (size_t i=0; i<size (); ++i, ++p) {
			size_t k = util::succ(i);
			std::cout << k << ". Handle = " << p->fd << std::endl;
			std::cout << k << ". Mask   = " << util::valueToBinary(p->events) << std::endl;
			std::cout << k << ". Events = " << util::valueToBinary(p->revents) << std::endl;
		}
		std::cout << std::endl;
	}

};

#ifdef HAS_EPOLL

class TEpollEvent : public util::TArray<epoll_event> {
private:
	typedef util::TArray<epoll_event> poll_t;

public:
	void add(epoll_event * item) {
		poll_t::add(item);
	}

	size_t find(const app::THandle handle) const {
		poll_t::object_p o;
		for (size_t i=0; i<size(); ++i) {
			o = poll_t::vector_t::at(i);
			if (util::assigned(o)) {
				PSocketConnection p = (PSocketConnection)o->data.ptr;
				if (handle == p->client)
					return i;
			}
		}
		return std::string::npos;
	}

	bool eraseByHandle(app::THandle handle) {
		size_t index = find(handle);
		if (std::string::npos != index) {
			poll_t::object_p p = poll_t::vector_t::at(index);
			util::freeAndNil(p);
			poll_t::vector_t::erase(poll_t::vector_t::begin() + index);
			poll_t::invalidate();
		}
		return false;
	}

	bool eraseByIndex(size_t index) {
		if (validIndex(index)) {
			poll_t::object_p p = poll_t::vector_t::at(index);
			util::freeAndNil(p);
			poll_t::vector_t::erase(poll_t::vector_t::begin() + index);
			poll_t::invalidate();
		}
		return false;
	}

	void debugOutput() {
		PSocketConnection connection;
		epoll_event* p = array();
		for (size_t i=0; i<size (); ++i, ++p) {
			connection = (PSocketConnection)p->data.ptr;
			size_t k = util::succ(i);
			if (util::assigned(connection)) {
				std::cout << k << ". Handle = " << connection->client << std::endl;
				std::cout << k << ". Socket = " << connection->server << std::endl;
			} else {
				std::cout << k << ". <Undefined connection>" << std::endl;
			}
			std::cout << k << ". Events = " << util::valueToBinary(p->events) << std::endl;
		}
		std::cout << std::endl;
	}
};

#endif


} /* namespace inet */

#endif /* SOCKETLISTS_H_ */
