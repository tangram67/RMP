/*
 * semophore.h
 *
 *  Created on: 20.09.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include <mutex>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "classes.h"
#include "memory.h"
#include "templates.h"
#include "../config.h"

namespace app {

class TMutex;
class TSpinLock;
class TCondition;
class TSemaphore;
class TReadWriteLock;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PMutex = TMutex*;
using PSpinLock = TSpinLock*;
using PCondition = TCondition*;
using PSemaphore = TSemaphore*;
using PReadWriteLock = TReadWriteLock*;

#else

typedef TMutex* PMutex;
typedef TSpinLock* PSpinLock;
typedef TCondition* PCondition;
typedef TSemaphore* PSemaphore;
typedef TReadWriteLock* PReadWriteLock;

#endif


enum ELockScope {
	LCK_PROC_PRIVATE = PTHREAD_PROCESS_PRIVATE,
	LCK_PROC_SHARED = PTHREAD_PROCESS_SHARED
};

enum TShareType {
	EST_NONE,
	EST_LOCAL,
	EST_SHARED,
	EST_GLOBAL
};

enum EReadWriteLock {
	RWL_READ,
	RWL_WRITE
};

enum TMutexType {
	EMT_MUTEX_NOINIT = PTHREAD_MUTEX_DEFAULT,
	EMT_MUTEX_RECURSIVE = PTHREAD_MUTEX_RECURSIVE,
	EMT_MUTEX_ERRORCHECK = PTHREAD_MUTEX_ERRORCHECK,
#ifdef USE_DEFAULT_ERRORCHECK_MUTEX
	EMT_MUTEX_DEFAULT = EMT_MUTEX_ERRORCHECK
#else
	EMT_MUTEX_DEFAULT = EMT_MUTEX_NOINIT
#endif
};


class TBaseLock {
private:
	int errval;
	std::string errmsg;

	virtual void open() = 0;
	virtual void close() = 0;
	virtual bool valid() const = 0;
	std::string validateName(const std::string& name);

protected:
	ELockScope scope;
	std::string name;

public:
	int error() const { return errval; };
	const std::string& message() const { return errmsg; };
	int getTime(const time_t delay, struct timespec* ts);
	void setName(const std::string& name);
	const std::string& getName() const { return name; };
	void setScope(const ELockScope scope);
	ELockScope getScope() const { return scope; };

	explicit TBaseLock();
	virtual ~TBaseLock() {}
};


class TSemaphore : public TBaseLock {
private:
	TShareType type;
#ifdef USE_STATIC_SEMAPHORES
	sem_t mem;
#endif
	sem_t *sema;
	int value;
	util::TSharedMemory<sem_t> shm;

	void global();
	void shared();
	void local();

protected:	
	void init(const int sval);
	
public:
	void open();
	void close();

	bool wait();
	bool tryWait();
	bool tryTimedWait(time_t ms);
	void post();
	
	bool valid() const { return util::assigned(sema); };
	
	TSemaphore(const TSemaphore&) = delete;
	TSemaphore& operator=(const TSemaphore&) = delete;

	TSemaphore();
	TSemaphore(int value, ELockScope scope = LCK_PROC_PRIVATE, const std::string& name = "");
	virtual ~TSemaphore();
};


class TSharedSemaphore : public TSemaphore {
public:
	void open() {
		if (!valid()) {
			TSemaphore::open();
		}
	};
	void open(int value) {
		if (!valid()) {
			init(value);
			TSemaphore::open();
		}
	};

	TSharedSemaphore(const TSharedSemaphore&) = delete;
	TSharedSemaphore& operator=(const TSharedSemaphore&) = delete;

	TSharedSemaphore() : TSemaphore(1, LCK_PROC_SHARED, "") {};
	virtual ~TSharedSemaphore() = default;
};


class TNamedSemaphore : public TSemaphore {
public:
	void open(const std::string& name) {
		if (!valid()) {
			setName(name);
			TSemaphore::open();
		}
	};
	void open(const std::string& name, int value) {
		if (!valid()) {
			setName(name);
			init(value);
			TSemaphore::open();
		}
	};

	TNamedSemaphore(const TNamedSemaphore&) = delete;
	TNamedSemaphore& operator=(const TNamedSemaphore&) = delete;

	TNamedSemaphore() : TSemaphore(1, LCK_PROC_SHARED, "") {};
	virtual ~TNamedSemaphore() = default;
};


class TMutex : public TBaseLock {
friend class TCondition;
private:
	TMutexType type;
	pthread_mutex_t * mutex;
#ifdef USE_STATIC_SEMAPHORES
	pthread_mutex_t mem;
#endif
	pthread_mutexattr_t * attr;
	util::TSharedMemory<pthread_mutex_t> shm;

	void init();
	void prime();
	void shared();
	void local();
	void attribute();
	pthread_mutex_t* value() { return mutex; };

public:
	void open();
	void close();

	void lock();
	bool tryLock();
	bool tryTimedLock(time_t ms);
	void unlock();

	bool valid() const { return util::assigned(mutex); };

	TMutex(const TMutex&) = delete;
	TMutex& operator=(const TMutex&) = delete;

	TMutex();
	TMutex(const TMutexType type);
	TMutex(const TMutexType type, const ELockScope scope);
	virtual ~TMutex();
};


class TErrorCheckMutex : public TMutex {
public:
	TErrorCheckMutex(const TErrorCheckMutex&) = delete;
	TErrorCheckMutex& operator=(const TErrorCheckMutex&) = delete;

	TErrorCheckMutex() : TMutex(EMT_MUTEX_ERRORCHECK) {};
	virtual ~TErrorCheckMutex() = default;
};


class TRecursiveMutex : public TMutex {
public:
	TRecursiveMutex(const TRecursiveMutex&) = delete;
	TRecursiveMutex& operator=(const TRecursiveMutex&) = delete;

	TRecursiveMutex() : TMutex(EMT_MUTEX_RECURSIVE) {};
	virtual ~TRecursiveMutex() = default;
};


class TSharedMutex : public TMutex {
public:
	TSharedMutex(const TSharedMutex&) = delete;
	TSharedMutex& operator=(const TSharedMutex&) = delete;

	TSharedMutex() : TMutex(EMT_MUTEX_DEFAULT, LCK_PROC_SHARED) {};
	virtual ~TSharedMutex() = default;
};


class TNamedMutex : public TMutex {
public:
	void open(const std::string& name) {
		if (!valid()) {
			setName(name);
			TMutex::open();
		}
	};

	TNamedMutex(const TNamedMutex&) = delete;
	TNamedMutex& operator=(const TNamedMutex&) = delete;

	TNamedMutex() : TMutex(EMT_MUTEX_DEFAULT, LCK_PROC_SHARED) {};
	virtual ~TNamedMutex() = default;
};


class TCondition : public TBaseLock {
private:
	pthread_cond_t *cond;
#ifdef USE_STATIC_SEMAPHORES
	pthread_cond_t mem;
#endif
	TMutex *mutex;

	void open();
	void close();

public:
	void lock();
	bool tryLock();
	bool tryTimedLock(time_t ms);
	void unlock();
	void wait();
	bool timedWait(time_t ms);
	void signal();
	void broadcast();

	bool valid() const { return util::assigned(cond); };
	
	TCondition(const TCondition&) = delete;
	TCondition& operator=(const TCondition&) = delete;

	TCondition();
	virtual ~TCondition();
};


class TSpinLock : public TBaseLock {
private:
	pthread_spinlock_t *spinlock;
#ifdef USE_STATIC_SEMAPHORES
	pthread_spinlock_t mem;
#endif

	void open();
	void close();

public:
	void lock();
	bool tryLock();
	void unlock();

	bool valid() const { return util::assigned(spinlock); };

	TSpinLock(const TSpinLock&) = delete;
	TSpinLock& operator=(const TSpinLock&) = delete;

	TSpinLock();
	virtual ~TSpinLock();
};


class TReadWriteLock : public TBaseLock {
private:
	pthread_rwlock_t *rwlock;
#ifdef USE_STATIC_SEMAPHORES
	pthread_rwlock_t mem;
#endif
#ifdef USE_MUTEX_AS_RWLOCK
	TErrorCheckMutex mutex;
#endif

	void open();
	void close();

public:
	void rdLock();
	bool rdTryLock();
	bool rdTryTimedLock(time_t ms);
	void wrLock();
	bool wrTryLock();
	bool wrTryTimedLock(time_t ms);
	void unlock();

	bool valid() const { return util::assigned(rwlock); };

	TReadWriteLock(const TReadWriteLock&) = delete;
	TReadWriteLock& operator=(const TReadWriteLock&) = delete;

	TReadWriteLock();
	virtual ~TReadWriteLock();
};


/*
 * Locking guards (RAII "Resource Acquisition Is Initialization" pattern)
 */
template<typename T>
class TLockGuard
{
private:
	typedef T lock_t;
	lock_t&  instance;
	bool owns;

public:
	void lock() {
		instance.lock();
		owns = true;
	}
	bool tryLock() {
		owns = instance.tryLock();
		return owns;
	}
	void unlock() {
		if (owns) {
			owns = false;
			instance.unlock();
		}
	}

	TLockGuard& operator=(const TLockGuard&) = delete;
	TLockGuard(const TLockGuard&) = delete;

	explicit TLockGuard(lock_t& F, bool aquire = true) : instance(F) {
		owns = false;
		if (aquire) {
			lock();
		}
	}
	~TLockGuard() { unlock(); }
};


template<typename T>
class TSemaphoreGuard
{
private:
	typedef T sema_t;
	sema_t&  instance;
	bool owns;

public:
	void wait() {
		instance.wait();
		owns = true;
	}
	void post() {
		if (owns) {
			owns = false;
			instance.post();
		}
	}

	TSemaphoreGuard& operator=(const TSemaphoreGuard&) = delete;
	TSemaphoreGuard(const TSemaphoreGuard&) = delete;

	explicit TSemaphoreGuard(sema_t& F, bool wait = true) : instance(F) {
		owns = false;
		if (wait) {
			instance.wait();
			owns = true;
		}
	}
	~TSemaphoreGuard() {
		if (owns)
			instance.post();
	}
};


template<typename T>
class TConditionGuard
{
private:
	typedef T lock_t;
	lock_t&  instance;
	bool owns;

public:
	void lock() {
		instance.lock();
		owns = true;
	}
	bool tryLock() {
		owns = instance.tryLock();
		return owns;
	}
	void unlock() {
		if (owns) {
			owns = false;
			instance.unlock();
		}	
	}
	void signal() {
		instance.signal();
	}

	TConditionGuard& operator=(const TConditionGuard&) = delete;
	TConditionGuard(const TConditionGuard&) = delete;

	explicit TConditionGuard(lock_t& F, bool lock = true) : instance(F) {
		owns = false;
		if (lock) {
			instance.lock();
			owns = true;
		}
	}
	~TConditionGuard() {
		if (owns)
			instance.unlock();
	}
};


template<typename T>
class TReadWriteGuard
{
private:
	typedef T lock_t;
	lock_t&  instance;
	bool owns;

public:
	void rdLock() {
		instance.rdLock();
		owns = true;
	}
	bool rdTryLock() {
		owns = instance.rdTryLock();
		return owns;
	}
	void wrLock() {
		instance.wrLock();
		owns = true;
	}
	bool wrTryLock() {
		owns = instance.wrTryLock();
		return owns;
	}
	void unlock() {
		if (owns) {
			owns = false;
			instance.unlock();
		}
	}

	TReadWriteGuard& operator=(const TReadWriteGuard&) = delete;
	TReadWriteGuard(const TReadWriteGuard&) = delete;

	explicit TReadWriteGuard(lock_t& F, const EReadWriteLock type, bool aquire = true) : instance(F) {
		owns = false;
		if (aquire) {
			switch (type) {
				case RWL_READ:
					rdLock();
					break;
				case RWL_WRITE:
					wrLock();
					break;
			}
		}
	}
	explicit TReadWriteGuard(lock_t& F) : instance(F) {
		owns = false;
	}
	~TReadWriteGuard() {
		if (owns)
			instance.unlock();
	}
};


} /* namespace app */

#endif /* SEMAPHORE_H_ */
