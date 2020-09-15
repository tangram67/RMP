/*
 * threadtypes.h
 *
 *  Created on: 19.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef THREADTYPES_H_
#define THREADTYPES_H_

#include <mutex>
#include <vector>
#include <functional>
#include "variant.h"
#include "gcc.h"

namespace app {

enum EThreadType {
	THD_CREATE_DETACHED = PTHREAD_CREATE_DETACHED,
	THD_CREATE_JOINABLE = PTHREAD_CREATE_JOINABLE
};

enum EThreadStartType {
	THD_START_ON_CREATE,
	THD_START_ON_DEMAND
};

enum EMessageTransferType {
	THD_MTT_SEND,
	THD_MTT_POST
};

enum EThreadMessageType {
	THD_MSG_INIT,
	THD_MSG_SYN,
	THD_MSG_HUP,
	THD_MSG_ERR,
	THD_MSG_USR0,
	THD_MSG_USR1,
	THD_MSG_USR2,
	THD_MSG_USR3,
	THD_MSG_USR4,
	THD_MSG_USR5,
	THD_MSG_USR6,
	THD_MSG_USR7,
	THD_MSG_USR8,
	THD_MSG_USR9,
	THD_MSG_QUIT
};


// Forward declaration of template thread class...
template<typename T>
class TWorkerThread;
class TBaseThread;
class TManagedThread;
class TDetachedThread;
class TThreadController;
class TThreadDataItem;
class TThreadData;


#ifdef STL_HAS_TEMPLATE_ALIAS

using PBaseThread = TBaseThread*;
using PManagedThread = TManagedThread*;
using PDetachedThread = TDetachedThread*;

using TThreadList = std::vector<app::PBaseThread>;

using PThreadController = TThreadController*;
using PThreadDataItem = TThreadDataItem*;
using PThreadData = TThreadData*;

using TThreadExecMethod = std::function<int(TManagedThread&)>;
using TThreadMessageMethod = std::function<void(TManagedThread&, EThreadMessageType)>;

using TThreadDataList = std::vector<app::PThreadDataItem>;
using TThreadDataRequest = std::function<void(TThreadData&, const void*&, size_t&, const util::TVariantValues&, const util::TVariantValues&, util::TVariantValues&, bool&, bool&, int&)>;
using TThreadDataReceived = std::function<void(TThreadData&, void const * const, const size_t, const util::TVariantValues&, const util::TVariantValues&, bool, int&)>;

#else

typedef TBaseThread* PBaseThread;
typedef TManagedThread* PManagedThread;
typedef TDetachedThread* PDetachedThread;

typedef std::vector<app::PBaseThread> TThreadList;

typedef TThreadController* PThreadController;
typedef TThreadDataItem* PThreadDataItem;
typedef TThreadData* PThreadData;

typedef std::function<int(TManagedThread&)> TThreadExecMethod;
typedef std::function<void(TManagedThread&, EThreadMessageType)> TThreadMessageMethod;

typedef std::vector<app::PThreadDataItem> TThreadDataList;
typedef std::function<void(TThreadData&, const void*&, size_t&, const util::TVariantValues&, const util::TVariantValues&, util::TVariantValues&, bool&, bool&, int&)> TThreadDataRequest;
typedef std::function<void(TThreadData&, void const * const, const size_t, const util::TVariantValues&, const util::TVariantValues&, bool, int&)> TThreadDataReceived;

#endif


template<typename T>
class TThreadSyncGuard
{
private:
	typedef T thread_t;
	thread_t&  instance;
	bool owns;

public:
	TThreadSyncGuard& operator=(const TThreadSyncGuard&) = delete;
	bool waitForSync() {
		bool retVal;
		retVal = instance.waitForSync();
		owns = true;
		return retVal;
	}
	void releaseSync() {
		if (owns)
			instance.releaseSync();
	}
	explicit TThreadSyncGuard(thread_t& F) : instance(F), owns(false) {}
	TThreadSyncGuard(const TThreadSyncGuard&) = delete;
	~TThreadSyncGuard() { releaseSync(); }
};


template<typename T>
class TThreadDataActionGuard
{
private:
	typedef T action_t;
	action_t&  instance;
	bool running;

public:
	TThreadDataActionGuard& operator=(const TThreadDataActionGuard&) = delete;

	void setRunning(const bool running) {
		instance.setRunning(running);
		this->running = running;
	}

	explicit TThreadDataActionGuard(action_t& F) : instance(F) {
		running = instance.getRunning();
	}
	TThreadDataActionGuard(const TThreadDataActionGuard&) = delete;
	~TThreadDataActionGuard() {
		if (running) {
			running = false;
			instance.setRunning(running);
		}
	}
};


template<typename T>
class TThreadSaveBooleanGuard {
private:
	typedef T bool_t;
	std::mutex& mtx;
	bool_t& value;
	bool leave;

	void setState() {
		if (leave) set();
		else reset();
	}

public:
	bool get() const {
		std::lock_guard<std::mutex> lock(mtx);
		return value;
	}
	void set() {
		std::lock_guard<std::mutex> lock(mtx);
		value = true;
	}
	void reset() {
		std::lock_guard<std::mutex> lock(mtx);
		value = false;
	}
	void toggle() {
		std::lock_guard<std::mutex> lock(mtx);
		value = !value;
	}

	TThreadSaveBooleanGuard& operator=(const TThreadSaveBooleanGuard&) = delete;
	TThreadSaveBooleanGuard(const TThreadSaveBooleanGuard&) = delete;

	explicit TThreadSaveBooleanGuard(bool_t& F, std::mutex& lock, bool state) : mtx(lock), value(F), leave(state) {}
	explicit TThreadSaveBooleanGuard(bool_t& F, std::mutex& lock) : mtx(lock), value(F), leave(false) {}
	~TThreadSaveBooleanGuard() { setState(); }
};


} // namespace app

#endif /* THREADTYPES_H_ */
