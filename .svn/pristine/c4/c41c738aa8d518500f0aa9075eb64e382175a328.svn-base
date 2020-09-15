/*
 * control.cpp
 *
 *  Created on: 01.05.2017
 *      Author: Dirk Brinkmeier
 */

#include <functional>
#include <algorithm>
#include "control.h"
#include "../inc/templates.h"

namespace app {

TCommandQueue::TCommandQueue() {
}

TCommandQueue::~TCommandQueue() {
	destroy();
}

void TCommandQueue::clear() {
	app::TLockGuard<app::TMutex> lock(mtx);
	destroy();
}

void TCommandQueue::destroy() {
	util::clearObjectList(queue);
}

struct CCommandEraser {
    bool operator()(PCommand o) const {
    	bool r = false;
    	if (util::assigned(o)) {
			if (o->state == ECS_FINISHED) {
				util::freeAndNil(o);
				r = true;
			}
    	}
    	return r;
    }
};

size_t TCommandQueue::cleanup() {
	size_t size = queue.size();
	queue.erase(std::remove_if(queue.begin(), queue.end(), CCommandEraser()), queue.end());
	return size - queue.size();
}

size_t TCommandQueue::pending() {
	app::TLockGuard<app::TMutex> lock(mtx);
	size_t r = 0;
	PCommand o;
	iterator it = queue.begin();
	while (it != queue.end()) {
		o = *it;
		if (util::assigned(o)) {
			if (o->state == ECS_IDLE) {
				++r;
			}
		}
		++it;
	}
	return r;
}

bool TCommandQueue::next(TCommand& command) {
	PCommand o = next();
	if (util::assigned(o)) {
		command = *o;
		return true;
	}
	return false;
}

PCommand TCommandQueue::next() {
	app::TLockGuard<app::TMutex> lock(mtx);
	PCommand o;
	iterator it = queue.begin();
	while (it != queue.end()) {
		o = *it;
		if (util::assigned(o)) {
			if (o->state == ECS_IDLE) {
				if (o->hasDelay()) {
					if (o->isDelayed()) {
						o->state = ECS_WAIT;
						return o;
					}
				} else {
					o->state = ECS_WAIT;
					return o;
				}
			}
		}
		++it;
	}
	return nil;
}

bool TCommandQueue::add(const TCommand& command) {
	app::TLockGuard<app::TMutex> lock(mtx);
	bool ok = true;
	PCommand o = nil;

	// Don't add duplicates...
	if (!empty()) {
		const_iterator it = last();
		if (it != end()) {
			o = *it;
			if (util::assigned(o)) {
				if ((o->state == ECS_WAIT || o->state == ECS_IDLE) && o->action == command.action) {
					// std::cout << "TCommandQueue::add() Command (" << command.action << ") for file \"" << command.file << "\" ignored." << std::endl;
					ok = false;
				}
			}
		}
	}

	// Add command
	if (ok) {
		// std::cout << "TCommandQueue::add() Add command (" << command.action << ") for file \"" << command.file << "\"" << std::endl;
		o = new TCommand;
		*o = command;
		queue.push_back(o);
	}

	return ok;
}

void TCommandQueue::terminate(PCommand command) {
	app::TLockGuard<app::TMutex> lock(mtx);
	if (util::assigned(command)) {
		// std::cout << "TCommandQueue::terminate() Command (" << command->action << ") for file \"" << command->file << "\" terminated." << std::endl;
		command->state = ECS_FINISHED;
		cleanup();
	}
}

void TCommandQueue::busy(PCommand command) {
	app::TLockGuard<app::TMutex> lock(mtx);
	if (util::assigned(command)) {
		// std::cout << "TCommandQueue::busy() Command (" << command->action << ") for file \"" << command->file << "\" is busy." << std::endl;
		command->state = ECS_BUSY;
	}
}



} /* namespace app */
