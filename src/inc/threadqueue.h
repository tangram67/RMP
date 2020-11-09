/*
 * threadqueue.h
 *
 *  Created on: 11.06.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef THREADQUEUE_H_
#define THREADQUEUE_H_

#include "semaphore.h"
#include "templates.h"

namespace app {


template<typename T>
class TThreadQueue {
protected:
	typedef T object_t;
	typedef T* object_p;
	typedef struct queue_c {
		object_p object;
		int refC;
	} queue_t;
	typedef queue_t* queue_p;
	typedef std::vector<queue_p> vector_t;

private:
	app::TMutex mtx;
	vector_t queue;

	queue_p top() {
		if (!queue.empty()) {
			return queue[0];
		}
		return nil;
	}

	void push_back(const object_t& item) {
		queue_p o = new queue_t;
		o->object = new object_t;
		*o->object = item;
		o->refC = 1;
		queue.push_back(o);
	}

	queue_p pull() {
		queue_p item = top();
		if (util::assigned(item)) {
			if (util::assigned(item->object)) {
				--item->refC;
				return item;
			}
		}
		return nil;
	}

	struct CQueueEraser {
	    bool operator()(queue_p o) const {
	    	bool r = false;
	    	if (util::assigned(o)) {
				if (o->refC <= 0) {
					util::freeAndNil(o);
					r = true;
				}
	    	}
	    	return r;
	    }
	};

	size_t cleanup() {
		size_t size = queue.size();
		queue.erase(std::remove_if(queue.begin(), queue.end(), CQueueEraser()), queue.end());
		return size - queue.size();
	}

public:
	void clear() {
		app::TLockGuard<app::TMutex> lock(mtx);
		util::clearObjectList(queue);
	}

	void add(const object_t& item) {
		app::TLockGuard<app::TMutex> lock(mtx);
		push_back(item);
	}

	void poke(const object_t& item) {
		app::TLockGuard<app::TMutex> lock(mtx);
		util::clearObjectList(queue);
		push_back(item);
	}

	bool peek(object_t& item) {
		app::TLockGuard<app::TMutex> lock(mtx);
		queue_p o = top();
		if (util::assigned(o)) {
			if (util::assigned(o->object)) {
				item = *o->object;
				return true;
			}
		}
		return false;
	}

	bool next(object_t& item) {
		app::TLockGuard<app::TMutex> lock(mtx);
		queue_p o = pull();
		if (util::assigned(o)) {
			if (util::assigned(o->object)) {
				item = *o->object;
				cleanup();
				return true;
			}
		}
		return false;
	}

	virtual ~TThreadQueue() { util::clearObjectList(queue); };
};


} /* namespace app */

#endif /* THREADQUEUE_H_ */
