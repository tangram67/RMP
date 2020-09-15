/*
 * semophore.cpp
 *
 *  Created on: 20.09.2014
 *      Author: Dirk Brinkmeier
 */

#include <iostream>
#include <fcntl.h>		/* For O_* constants */
#include <sys/stat.h>	/* For mode constants */
#include <sys/mman.h>
#include <semaphore.h>
#include "timeconsts.h"
#include "semaphores.h"
#include "templates.h"
#include "datetime.h"
#include "nullptr.h"
#include "functors.h"
#include "exception.h"
#include "memory.h"

#define CHECK_SEMA(location) if (sema == nil) throw util::app_error(location" Semaphore not initialized.");
#define CHECK_MUTEX(location) if (mutex == nil) throw util::app_error(location" Mutex not initialized.");

/* Read-write lock initializers.  */
#ifdef __PTHREAD_RWLOCK_ELISION_EXTRA

#ifndef PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP
# define PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP \
   { { 0, 0, 0, 0, 0, 0, 0, 0, __PTHREAD_RWLOCK_ELISION_EXTRA, 0, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP } }
#endif   

#define PTHREAD_RWLOCK_PREFER_READER_INITIALIZER_NP \
  { { 0, 0, 0, 0, 0, 0, 0, 0, __PTHREAD_RWLOCK_ELISION_EXTRA, 0, PTHREAD_RWLOCK_PREFER_READER_NP } }

#define PTHREAD_RWLOCK_PREFER_WRITER_INITIALIZER_NP \
  { { 0, 0, 0, 0, 0, 0, 0, 0, __PTHREAD_RWLOCK_ELISION_EXTRA, 0, PTHREAD_RWLOCK_PREFER_WRITER_NP } }

#else

#ifndef PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP
# define PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP PTHREAD_RWLOCK_INITIALIZER
#endif

#define PTHREAD_RWLOCK_PREFER_READER_INITIALIZER_NP PTHREAD_RWLOCK_INITIALIZER
#define PTHREAD_RWLOCK_PREFER_WRITER_INITIALIZER_NP PTHREAD_RWLOCK_INITIALIZER

#endif

namespace app {

TBaseLock::TBaseLock() {
	errval = EXIT_SUCCESS;
	scope = LCK_PROC_PRIVATE;
}

int TBaseLock::getTime(const time_t delay, struct timespec* ts) {
	time_t tsec = delay / util::MILLI_JIFFIES;
	time_t nsec = (delay % util::MILLI_JIFFIES) * util::MICRO_JIFFIES; // MILLI_JIFFIES * MICRO_JIFFIES = NANO_JIFFIES

	int r = clock_gettime(CLOCK_REALTIME, ts);
	if (r != EXIT_SUCCESS) {
		errval = errno;
		errmsg = "TBaseLock::getTime()::clock_gettime() failed.";
		return errval;
	}

	ts->tv_sec += tsec;
	ts->tv_nsec += nsec;

	if (ts->tv_nsec >= util::NANO_JIFFIES) {
		ts->tv_nsec -= util::NANO_JIFFIES;
		ts->tv_sec += 1;
	}

	return EXIT_SUCCESS;
}

std::string TBaseLock::validateName(const std::string& name) {
	std::string s;
	if (!name.empty()) {
		s = name;
		if (s[0] != '/')
			s.insert(0, 1, '/');
	}
	return s;
}

void TBaseLock::setName(const std::string& name) {
	this->name = validateName(name);
}

void TBaseLock::setScope(const ELockScope scope) {
	this->scope = scope;
}


/*
 * Create a global semaphore in /dev/shm --> name = "/<name>"
 *
 * Create semaphore locked   --> value = 0
 * Create semaphore unlocked --> value > 0
 *
 * Create shared semaphore between processes
 * e.g. via fork() --> shared = SEM_PROCESS_SHARED
 *
 * Create private semaphore for threads and
 * main process --> shared = SEM_PROCESS_PRIVATE (standard)
 *
 */
TSemaphore::TSemaphore() : TBaseLock() {
	sema = nil;
	init(1);
	open();
}


TSemaphore::TSemaphore(int value, ELockScope scope, const std::string& name) : TBaseLock(), value(value) {
	sema = nil;
	setName(name);
	setScope(scope);
	init(value);
	if (type == EST_LOCAL) {
		// Open local semaphores only
		open();
	}
}

TSemaphore::~TSemaphore() {
	close();
}


void TSemaphore::open() {
	if (sema == nil) {
		// Open semaphore for given type
		switch (type) {
			case EST_GLOBAL:
				global();
				break;
			case EST_SHARED:
				shared();
				break;
			case EST_LOCAL:
			default:
				local();
				break;
		}
	}
}


void TSemaphore::init(const int sval) {
	type = EST_NONE;
	value = sval;

	// Global named semaphore
	if (type == EST_NONE && !name.empty()) {
		type = EST_GLOBAL;
	}

	// Shared memory semaphore
	if (type == EST_NONE && scope == LCK_PROC_SHARED) {
		type = EST_SHARED;
	}

	// Local semaphore
	if (type == EST_NONE) {
		type = EST_LOCAL;
	}
}


void TSemaphore::global() {
	if (!name.empty()) {
		do {
			const char* p = name.c_str();
			sema = sem_open(p, O_CREAT, S_IRUSR | S_IWUSR, value);
		} while (sema == SEM_FAILED && errno == EINTR);
		if (SEM_FAILED == sema)
			throw util::sys_error("TSemaphore::global()::sem_open() failed for \"" + name + "\"", errno);
	} else
		throw util::app_error("TSemaphore::global()::sem_open() failed on empty name.");
}

void TSemaphore::shared() {
	int retVal;

	// Get shared memory for semaphore
	retVal = shm.create(&sema, sizeof(sem_t));
	if (retVal != EXIT_SUCCESS || !util::assigned(sema)) {
		throw util::sys_error("TSemaphore::shared() : " + shm.message(), shm.error());
	}

	// Lock shared memory for semaphore
	if (!shm.lock())
		throw util::sys_error("TSemaphore::shared()::lockAddress() failed.");

	// Initialize semaphore with memory mapped region
	memset(sema, 0, sizeof(sem_t));
	retVal = sem_init(sema, scope, value);
	if (EXIT_SUCCESS != retVal) {
		shm.destroy();
		sema = nil;
		throw util::sys_error("TSemaphore::shared()::sem_init() failed.", retVal);
	}
}


void TSemaphore::local() {
	int retVal;

	// Lock local semaphore memory
#ifdef USE_STATIC_SEMAPHORES
	sema = &mem;
#else
	sema = util::createLockedObject<sem_t>();
	if (!util::assigned(sema)) {
		throw util::sys_error("TSemaphore::local()::createLockedObject() failed.");
	}
#endif

	// Initialize local semaphore
	memset(sema, 0, sizeof(sem_t));
	retVal = sem_init(sema, scope, value);
	if (EXIT_SUCCESS != retVal) {
		delete sema;
		sema = nil;
		throw util::sys_error("TSemaphore::local()::sem_init() failed.", retVal);
	}
}


void TSemaphore::close() {
	if (util::assigned(sema)) {
		int error1 = EXIT_SUCCESS;
		int error2 = EXIT_SUCCESS;
		int error3 = EXIT_SUCCESS;
		int error4 = EXIT_SUCCESS;
		int error5 = EXIT_SUCCESS;
		errno = EXIT_SUCCESS;

		// Close semaphore depending on type
		switch (type) {
			case EST_SHARED:
				error1 = sem_destroy(sema);
				error2 = shm.destroy();
				break;

			case EST_GLOBAL:
				error3 = sem_unlink(name.c_str());
				error4 = sem_close(sema);
				break;

			case EST_LOCAL:
			default:
				error5 = sem_destroy(sema);
#ifndef USE_STATIC_SEMAPHORES
				delete sema;
#endif
				break;
		}

		// Error handling...
		sema = nil;
		if (EXIT_SUCCESS != error1)
			throw util::sys_error("TSemaphore::close()::sem_destroy() for shared semaphore failed.");
		if (EXIT_SUCCESS != error2)
			throw util::sys_error("TSemaphore::close() : " + shm.message(), shm.error());
		if (EXIT_SUCCESS != error3)
			throw util::sys_error("TSemaphore::close()::sem_unlink() for global semaphore \"" + name + "\" failed.");
		if (EXIT_SUCCESS != error4)
			throw util::sys_error("TSemaphore::close()::sem_close() for global semaphore \"" + name + "\" failed.");
		if (EXIT_SUCCESS != error5)
			throw util::sys_error("TSemaphore::close()::sem_destroy() for local semaphore failed.");

	}
}


bool TSemaphore::wait() {
	CHECK_SEMA("TSemaphore::wait()");
	int retVal;
	do {
		errno = EXIT_SUCCESS;
		retVal = sem_wait(sema);
	} while (retVal == EXIT_ERROR && errno == EINTR);
	if (retVal == EXIT_ERROR)
		throw util::sys_error("TSemaphore::wait()::sem_wait() failed.", retVal);
	return retVal == EXIT_SUCCESS;
}


bool TSemaphore::tryWait() {
	CHECK_SEMA("TSemaphore::tryWait()");
	int retVal;
	do {
		errno = EXIT_SUCCESS;
		retVal = sem_trywait(sema);
	} while (retVal == EXIT_ERROR && errno == EINTR);
	if (retVal == EXIT_ERROR && errno != EAGAIN)
		throw util::sys_error("TSemaphore::tryWait()::sem_trywait() failed.", retVal);
	return (retVal == EXIT_SUCCESS);
}


bool TSemaphore::tryTimedWait(time_t ms) {
	CHECK_SEMA("TSemaphore::tryTimedWait()");
	int retVal;
	struct timespec ts;
	int r = getTime(ms, &ts);
	if (r != EXIT_SUCCESS)
		throw util::sys_error("TSemaphore::tryTimeWait() : " + message(), error());
	do {
		errno = EXIT_SUCCESS;
		retVal = sem_timedwait(sema, &ts);
	} while (retVal == EXIT_ERROR && errno == EINTR);
	if (retVal == EXIT_ERROR && errno != ETIMEDOUT)
		throw util::sys_error("TSemaphore::tryTimeWait()::sem_timedwait() failed.", retVal);

	return (retVal == EXIT_SUCCESS);
}


void TSemaphore::post() {
	CHECK_SEMA("TSemaphore::post()");
	int retVal;
	retVal = sem_post(sema);
	if (EXIT_SUCCESS != retVal)
		throw util::sys_error("TSemaphore::close()::sem_post() failed.", retVal);
}



TMutex::TMutex() : TBaseLock() {
	type = EMT_MUTEX_DEFAULT;
	prime();
	open();
}

TMutex::TMutex(const TMutexType type) : TBaseLock() {
	this->type = type;
	prime();
	open();
}

TMutex::TMutex(const TMutexType type, const ELockScope scope) : TBaseLock() {
	this->scope = scope;
	this->type = type;
	prime();
	open();
}

TMutex::~TMutex() {
	close();
}

void TMutex::prime() {
	mutex = nil;
	attr = nil;
}

void TMutex::open() {
	if (mutex == nil) {
		// Initialize mutex
		switch (scope) {
			case LCK_PROC_SHARED:
				shared();
				break;
			case LCK_PROC_PRIVATE:
			default:
				local();
				break;
		}
		attribute();
	}
}


void TMutex::shared() {
	// Get shared memory for mutex
	int retVal = shm.create(&mutex, sizeof(pthread_mutex_t));
	if (retVal != EXIT_SUCCESS || !util::assigned(mutex)) {
		throw util::sys_error("TMutex::shared() : " + shm.message(), shm.error());
	}

	// Lock shared memory region for mutex
	if (!shm.lock())
		throw util::sys_error("TMutex::shared()::lockAddress() failed.");
}


void TMutex::local() {
#ifdef USE_STATIC_SEMAPHORES
	mutex = &mem;
#else
	// Lock local mutex memory
	mutex = util::createLockedObject<pthread_mutex_t>();
	if (!util::assigned(mutex)) {
		throw util::sys_error("TMutex::local()::createLockedObject() failed.");
	}
#endif
}


void TMutex::attribute() {
    int retVal;
    bool init = false;
	
	// Free attributes...
	if (util::assigned(attr)) {
		pthread_mutexattr_destroy(attr);
		util::freeAndNil(attr);
	}

	// Default mutex initalizer
	*mutex = PTHREAD_MUTEX_INITIALIZER;

	// Set attributes
	if (type != EMT_MUTEX_NOINIT) {
    	attr = new pthread_mutexattr_t;
    	memset(attr, 0, sizeof(pthread_mutexattr_t));

		retVal = pthread_mutexattr_init(attr);
		if (util::checkFailed(retVal))
			throw util::sys_error("TMutex::attribute()::pthread_mutexattr_init() failed.", retVal);

		if (EMT_MUTEX_NOINIT != type) {
			retVal = pthread_mutexattr_settype(attr, (int)type);
			if (util::checkFailed(retVal))
				throw util::sys_error("TMutex::attribute()::pthread_mutexattr_settype() failed.", retVal);
			init = true;
		}

		if (scope != LCK_PROC_PRIVATE) {
			retVal = pthread_mutexattr_setpshared(attr, (int)scope);
			if (util::checkFailed(retVal))
				throw util::sys_error("TMutex::attribute()::pthread_mutexattr_setpshared() failed.", retVal);
			init = true;
		}

		if (init) {
			retVal = pthread_mutex_init(mutex, attr);
			if (util::checkFailed(retVal))
				throw util::sys_error("TMutex::attribute()::pthread_mutex_init() failed.", retVal);
		}
    }
}


void TMutex::close() {
	int c = 0;
	int error1 = EXIT_SUCCESS;
	int error2 = EXIT_SUCCESS;
	int error3 = EXIT_SUCCESS;

	if (util::assigned(mutex)) {

		// Destroy mutex properties
		if (type != EMT_MUTEX_NOINIT) {
			do {
				error1 = pthread_mutex_destroy(mutex);
				if (error1 == EBUSY)
					unlock();
				c++;
			} while (error1 == EBUSY && c <= 3);
		}

		// Unmap shared memory
		if (!shm.empty()) {
			error2 = shm.destroy();
			mutex = nil;
		}

		// Free local mutex
#ifndef USE_STATIC_SEMAPHORES
		util::freeAndNil(mutex);
#endif
	}

	// Release attribute properties
	if (util::assigned(attr)) {
		error3 = pthread_mutexattr_destroy(attr);
		util::freeAndNil(attr);
	}

	// Error handling...
	mutex = nil;
	if (util::checkFailed(error1))
		throw util::sys_error("TMutex::close()::pthread_mutex_destroy() failed.", error1);
	if (util::checkFailed(error2))
		throw util::sys_error("TMutex::close() : " + shm.message(), shm.error());
	if (util::checkFailed(error3))
		throw util::sys_error("TMutex::close()::pthread_mutexattr_destroy() failed.", error3);
}


void TMutex::lock() {
	CHECK_MUTEX("TMutex::lock()");
	int retVal = pthread_mutex_lock(mutex);
	if (util::checkFailed(retVal))
		throw util::sys_error("TMutex::lock()::pthread_mutex_lock() failed", retVal);
}


bool TMutex::tryLock() {
	CHECK_MUTEX("TMutex::tryLock()");
	int retVal = pthread_mutex_trylock(mutex);
	if (util::checkFailed(retVal) && retVal != EBUSY)
		throw util::sys_error("TMutex::tryLock()::pthread_mutex_trylock() failed.", retVal);
	return (retVal == EXIT_SUCCESS);
}


bool TMutex::tryTimedLock(time_t ms) {
	CHECK_MUTEX("TMutex::tryTimedLock()");
	struct timespec ts;
	int r = getTime(ms, &ts);
	if (r != EXIT_SUCCESS)
		throw util::sys_error("TMutex::tryTimedLock() : " + message(), error());
	int retVal = pthread_mutex_timedlock(mutex, &ts);
	if (util::checkFailed(retVal) && retVal != ETIMEDOUT)
		throw util::sys_error("TMutex::tryTimedLock()::pthread_mutex_timedlock() failed.", retVal);
	return (retVal == EXIT_SUCCESS);
}


void TMutex::unlock() {
	CHECK_MUTEX("TMutex::unlock()");
	int retVal = pthread_mutex_unlock(mutex);
	if (util::checkFailed(retVal))
		throw util::sys_error("TMutex::unlock()::pthread_mutex_unlock() failed.", retVal);
}




TCondition::TCondition() : TBaseLock() {
	open();
}

TCondition::~TCondition() {
	close();
}

void TCondition::open() {
	// TODO Check if mutex->lock() is needed
	mutex = new TMutex(EMT_MUTEX_NOINIT);

#ifdef USE_STATIC_SEMAPHORES
	cond = &mem;
#else
	// Lock local mutex memory
	cond = util::createLockedObject<pthread_cond_t>();
	if (!util::assigned(cond)) {
		util::freeAndNil(mutex);
		throw util::sys_error("TCondition::open()::createLockedObject() failed.");
	}
#endif
	*cond = PTHREAD_COND_INITIALIZER;

	// Initialize condition variable
	if (scope != LCK_PROC_PRIVATE) {
		int retVal = pthread_cond_init(cond, NULL);
		if (util::checkFailed(retVal)) {
			util::freeAndNil(cond);
			util::freeAndNil(mutex);
			throw util::sys_error("TCondition::open()::pthread_cond_init() failed.", retVal);
		}
	}
}

void TCondition::close() {
	int retVal = pthread_cond_destroy(cond);
#ifndef USE_STATIC_SEMAPHORES
	delete cond;
#endif
	cond = nil;
	util::freeAndNil(mutex);
	if (util::checkFailed(retVal))
		throw util::sys_error("TCondition::close()::pthread_cond_destroy() failed.", retVal);
}

void TCondition::lock() {
	mutex->lock();
}

bool TCondition::tryLock() {
	return mutex->tryLock();
}

bool TCondition::tryTimedLock(time_t ms) {
	return mutex->tryTimedLock(ms);
}

void TCondition::unlock() {
	mutex->unlock();
}

void TCondition::wait() {
	// pthread_cond_wait functions will not return an error code of EINTR
	int retVal = pthread_cond_wait(cond, mutex->value());
	if (util::checkFailed(retVal))
		throw util::sys_error("TCondition::wait()::pthread_cond_wait() failed.", retVal);
}

bool TCondition::timedWait(time_t ms) {
	int retVal;
	struct timespec ts;
	int r = getTime(ms, &ts);
	if (r != EXIT_SUCCESS)
		throw util::sys_error("TCondition::timedWait() : " + message(), error());

	retVal = pthread_cond_timedwait(cond, mutex->value(), &ts);
	if (retVal == EXIT_ERROR && retVal != ETIMEDOUT)
		throw util::sys_error("TCondition::timeWait()::pthread_cond_timedwait() failed.", retVal);

	return (retVal == EXIT_SUCCESS);
}

void TCondition::signal() {
	int retVal = pthread_cond_signal(cond);
	if (util::checkFailed(retVal))
		throw util::sys_error("TCondition::signal()::pthread_cond_signal() failed.", retVal);
}

void TCondition::broadcast() {
	int retVal = pthread_cond_broadcast(cond);
	if (util::checkFailed(retVal))
		throw util::sys_error("TCondition::signal()::pthread_cond_signal() failed.", retVal);
}



TSpinLock::TSpinLock() : TBaseLock() {
	open();
}

TSpinLock::~TSpinLock() {
	close();
}

void TSpinLock::open() {
#ifdef USE_STATIC_SEMAPHORES
	spinlock = &mem;
#else
	// Lock spinlock memory
	spinlock = util::createLockedObject<pthread_spinlock_t>();
	if (!util::assigned(spinlock)) {
		throw util::sys_error("TSpinLock::open()::createLockedObject() failed.");
	}
#endif

	// Set default spin lock value
	*spinlock = 0; // --> see typedef int pthread_spinlock_t

	// Initialize shared spin lock
	if (scope != LCK_PROC_PRIVATE) {
		int retVal = pthread_spin_init(spinlock, (int)scope);
		if (util::checkFailed(retVal)) {
			util::freeAndNil(spinlock);
			throw util::sys_error("TSpinLock::open()::pthread_spin_init() failed.", retVal);
		}
	}
}

void TSpinLock::close() {
	int retVal = EXIT_SUCCESS;
	if (scope != LCK_PROC_PRIVATE) {
		retVal = pthread_spin_destroy(spinlock);
	}
#ifndef USE_STATIC_SEMAPHORES
	delete spinlock;
#endif
	spinlock = nil;
	if (util::checkFailed(retVal))
		throw util::sys_error("TSpinLock::close()::pthread_spin_destroy() failed.", retVal);
}

void TSpinLock::lock() {
	int retVal;
	retVal = pthread_spin_lock(spinlock);
	if (util::checkFailed(retVal))
		throw util::sys_error("TSpinLock::lock()::pthread_spin_lock() failed.", retVal);
}

bool TSpinLock::tryLock() {
	int retVal = pthread_spin_trylock(spinlock);
	if (util::checkFailed(retVal) && retVal != EBUSY)
		throw util::sys_error("TSpinLock::tryLock()::pthread_spin_trylock() failed.", retVal);
	return (retVal == EXIT_SUCCESS);
}

void TSpinLock::unlock() {
	int retVal = pthread_spin_unlock(spinlock);
	if (util::checkFailed(retVal))
		throw util::sys_error("TSpinLock::unlock()::pthread_spin_unlock() failed.", retVal);
}



TReadWriteLock::TReadWriteLock() : TBaseLock() {
	open();
}

TReadWriteLock::~TReadWriteLock() {
	close();
}

void TReadWriteLock::open() {
	int error = EXIT_SUCCESS;

#ifdef USE_STATIC_SEMAPHORES
	rwlock = &mem;
#else
	// Lock rwlock memory
	rwlock = util::createLockedObject<pthread_rwlock_t>();
	if (!util::assigned(rwlock)) {
		throw util::sys_error("TReadWriteLock::open()::createLockedObject() failed.");
	}
#endif

	// Static initialization of R/W lock
	*rwlock = PTHREAD_RWLOCK_PREFER_READER_INITIALIZER_NP;

	// Set shared lock attributes
	if (scope != LCK_PROC_PRIVATE) {
		pthread_rwlockattr_t attr;
		int retVal;

		if (EXIT_SUCCESS == error) {
			retVal = pthread_rwlockattr_init(&attr);
			if (util::checkFailed(retVal)) {
				error = 1;
			}
		}

		if (EXIT_SUCCESS == error) {
			retVal = pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_READER_NP);
			if (util::checkFailed(retVal)) {
				error = 2;
			}
		}

		if (EXIT_SUCCESS == error) {
			retVal = pthread_rwlockattr_setpshared(&attr, (int)scope);
			if (util::checkFailed(retVal)) {
				error = 3;
			}
		}

		if (EXIT_SUCCESS == error) {
			retVal = pthread_rwlock_init(rwlock, &attr);
			if (util::checkFailed(retVal)) {
				error = 4;
			}
		}

		if (EXIT_SUCCESS == error) {
			retVal = pthread_rwlockattr_destroy(&attr);
			if (util::checkFailed(retVal)) {
				error = 5;
			}
		}
	}

	if (EXIT_SUCCESS != error) {
#ifndef USE_STATIC_SEMAPHORES
			delete rwlock;
#endif
		rwlock = nil;
		switch (error) {
			case 1: throw util::sys_error("TReadWriteLock::open()::pthread_rwlockattr_init() failed.");
			case 2: throw util::sys_error("TReadWriteLock::open()::pthread_rwlockattr_setkind_np() failed.");
			case 3: throw util::sys_error("TReadWriteLock::open()::pthread_rwlockattr_setpshared() failed.");
			case 4: throw util::sys_error("TReadWriteLock::open()::pthread_rwlock_init() failed.");
			case 5: throw util::sys_error("TReadWriteLock::open()::pthread_rwlockattr_destroy() failed.");
			default:
				break;
		}
	}
}

void TReadWriteLock::close() {
	int retVal = EXIT_SUCCESS;
	if (scope != LCK_PROC_PRIVATE) {
		retVal = pthread_rwlock_destroy(rwlock);
	}
#ifndef USE_STATIC_SEMAPHORES
	if (util::assigned(rwlock)) {
		delete rwlock;
	}
#endif
	rwlock = nil;
	if (util::checkFailed(retVal))
		throw util::sys_error("TReadWriteLock::close()::pthread_rwlock_destroy() failed.", retVal);
}

void TReadWriteLock::rdLock() {
#ifdef USE_MUTEX_AS_RWLOCK
	mutex.lock();
#else
	int retVal = pthread_rwlock_rdlock(rwlock);
	if (util::checkFailed(retVal))
		throw util::sys_error("TReadWriteLock::rdLock()::pthread_rwlock_rdlock() failed.", retVal);
#endif
}

bool TReadWriteLock::rdTryLock() {
#ifdef USE_MUTEX_AS_RWLOCK
	return mutex.tryLock();
#else
	int retVal = pthread_rwlock_tryrdlock(rwlock);
	if (util::checkFailed(retVal) && retVal != EBUSY)
		throw util::sys_error("TReadWriteLock::rdTryLock()::pthread_rwlock_tryrdlock() failed.", retVal);
	return (retVal == EXIT_SUCCESS);
#endif
}

bool TReadWriteLock::rdTryTimedLock(time_t ms) {
#ifdef USE_MUTEX_AS_RWLOCK
	return mutex.tryTimedLock(ms);
#else
	int retVal;
	struct timespec ts;
	int r = getTime(ms, &ts);
	if (r != EXIT_SUCCESS)
		throw util::sys_error("TReadWriteLock::rdTryTimedLock() : " + message(), error());
	retVal = pthread_rwlock_timedrdlock(rwlock, &ts);
	if (util::checkFailed(retVal) && retVal != ETIMEDOUT)
		throw util::sys_error("TReadWriteLock::rdtryTimedLock()::pthread_rwlock_timedrdlock() failed.", retVal);

	return (retVal == EXIT_SUCCESS);
#endif
}

void TReadWriteLock::wrLock() {
#ifdef USE_MUTEX_AS_RWLOCK
	mutex.lock();
#else
	int retVal = pthread_rwlock_wrlock(rwlock);
	if (util::checkFailed(retVal))
		throw util::sys_error("TReadWriteLock::wrLock()::pthread_rwlock_wrlock() failed.", retVal);
#endif
}

bool TReadWriteLock::wrTryLock() {
#ifdef USE_MUTEX_AS_RWLOCK
	return mutex.tryLock();
#else
	int retVal = pthread_rwlock_trywrlock(rwlock);
	if (util::checkFailed(retVal) && retVal != EBUSY)
		throw util::sys_error("TReadWriteLock::wrTryLock()::pthread_rwlock_trywrlock() failed.", retVal);
	return (retVal == EXIT_SUCCESS);
#endif
}

bool TReadWriteLock::wrTryTimedLock(time_t ms) {
#ifdef USE_MUTEX_AS_RWLOCK
	return mutex.tryTimedLock(ms);
#else
	int retVal;
	struct timespec ts;
	int r = getTime(ms, &ts);
	if (r != EXIT_SUCCESS)
		throw util::sys_error("TReadWriteLock::wrTryTimedLock() : " + message(), error());
	retVal = pthread_rwlock_timedwrlock(rwlock, &ts);
	if (util::checkFailed(retVal) && retVal != ETIMEDOUT)
		throw util::sys_error("TReadWriteLock::wrtryTimedLock()::pthread_rwlock_timedwrlock() failed.", retVal);

	return (retVal == EXIT_SUCCESS);
#endif
}

void TReadWriteLock::unlock() {
#ifdef USE_MUTEX_AS_RWLOCK
	mutex.unlock();
#else
	int retVal = pthread_rwlock_unlock(rwlock);
	if (util::checkFailed(retVal))
		throw util::sys_error("TReadWriteLock::unlock()::pthread_rwlock_unlock() failed.", retVal);
#endif
}


} /* namespace app */
