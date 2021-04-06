/*
 * threads.cpp
 *
 *  Created on: 19.09.2014
 *      Author: Dirk Brinkmeier
 */

#define _MULTI_THREADED


#include <sys/syscall.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include "ansi.h"
#include "atomic.h"
#include "threads.h"
#include "nullptr.h"
#include "convert.h"
#include "sysutils.h"
#include "exception.h"
#include "../config.h"


static void* threadMethodDispatcher(void *creator) {
	if (util::assigned(creator)) {
		return (void *)(long)((app::PBaseThread)(((app::PThreadCreator)creator)->sender))->dispatcher(creator);
	}
	return (void *)(long)(EXIT_FAILURE);
}

static void* threadSignalDispatcher(void *message) {
	if (util::assigned(message)) {
		return (void *)(long)((app::PBaseThread)(((app::PThreadMessage)message)->sender))->receiver(message);
	}
	return (void *)(long)(EXIT_FAILURE);
}


namespace app {


/*
 * TMessageSlot
 */
TMessageSlot::TMessageSlot() {
}

TMessageSlot::~TMessageSlot() {
	clear();
}

PThreadMessage TMessageSlot::find(int id) {
	std::lock_guard<std::mutex> lock(mtx);
	PThreadMessage o = nil;
	for (size_t i=0; i<msgList.size(); i++) {
		o = msgList[i];
		if (util::assigned(o)) {
			if (o->id == id)
				break;
		}
	}
	return o;
}

void TMessageSlot::remove(int id) {
	std::lock_guard<std::mutex> lock(mtx);
	PThreadMessage o;
	size_t i = 0;
	while (i < msgList.size()) {
		o = msgList[i];
		if (util::assigned(o)) {
			if (o->id == id) {
				msgList.erase(msgList.begin() + i);
				util::freeAndNil(o);
				break;
			} else i++;
		} else i++;
	}
}

void TMessageSlot::clear() {
	std::lock_guard<std::mutex> lock(mtx);
	PThreadMessage o = nil;
	for (size_t i=0; i<msgList.size(); i++) {
		o = msgList[i];
		util::freeAndNil(o);
	}
	msgList.clear();
}

void TMessageSlot::release(PThreadMessage msg) {
	std::lock_guard<std::mutex> lock(mtx);
	msg->executed = true;
}

bool TMessageSlot::empty() const {
	std::lock_guard<std::mutex> lock(mtx);
	PThreadMessage o = nil;
	for (size_t i=0; i<msgList.size(); i++) {
		o = msgList[i];
		if (util::assigned(o)) {
			if (!o->executed)
				return false;
		}
	}
	return true;
}

PThreadMessage TMessageSlot::enqueue() {
	std::lock_guard<std::mutex> lock(mtx);
	PThreadMessage o = nil;
	size_t slot = nsizet;
	int id = 0;
	for (size_t i=0; i<msgList.size(); i++) {
		o = msgList[i];
		if (util::assigned(o)) {
			if (o->executed) {
				slot = i;
				id = o->id;
				break;
			}
			if (o->id > id)
				id = o->id + 1;
		}
	}
	if (slot != nsizet) {
		o = msgList[slot];
		o->id = id;
		o->executed = false;
	} else {
		o = new TThreadMessage;
		o->id = id;
		o->executed = false;
		msgList.push_back(o);
	}
	return o;
}



/*
 * TThreadAffinity
 */
TThreadAffinity::TThreadAffinity() {
	cpuset = nil;
	initCpuMask();
}

TThreadAffinity::~TThreadAffinity() {
	releaseCpuMask();
}

void TThreadAffinity::initCpuMask() {
	if (!util::assigned(cpuset)) {
		numa = sysutil::getProcessorCount();
		if (numa > 0) {
			cpusize = CPU_ALLOC_SIZE(numa);
			cpuset = CPU_ALLOC(numa);
			CPU_ZERO_S(cpusize, cpuset);
		}
	}
}

void TThreadAffinity::releaseCpuMask() {
	if (util::assigned(cpuset)) {
		CPU_FREE(cpuset);
		cpuset = nil;
	}
}

ssize_t TThreadAffinity::setAffinity(size_t cpu) {
	pid_t tid = TThreadUtil::gettid();
	return setAffinity(cpu, tid);
}

ssize_t TThreadAffinity::setAffinity(size_t cpu, pid_t tid) {
	// Set affinity to specific processor (-1 = disabled, 0 = choose processor, 1..n fixed processor)
	ssize_t retVal = -EINVAL;

	if (numa == 1)
		return retVal;

	if (tid > 0) {
		if (util::assigned(cpuset)) {
			// CPU == 0 --> Use a fixed core depending on core count
			if (cpu == 0 || cpu > numa) {
				if (numa > 2) {
					cpu = numa / 2; // e.g. second CPU of 4 CPUs == 2!
				} else {
					if (numa > 1) cpu = 2;
					else cpu = 1;
				}
			}
			// Use core from 1 to max.core count:
			//   - First logical core for setAffinity() is 1
			//   - First physical core is 0
			--cpu;
			if (cpu < numa) {
				CPU_SET_S(cpu, cpusize, cpuset);
				size_t c = CPU_COUNT_S(cpusize, cpuset);
				if (c > 0) {
					if (sched_setaffinity(tid, cpusize, cpuset) < 0)
						throw util::sys_error("TThreadAffinity::setAffinity()::sched_setaffinity() failed." );
					retVal = cpu + 1;
				} else
					throw util::app_error("TThreadAffinity::setAffinity() failed: No CPU in mask." );
			} else
				throw util::app_error("TThreadAffinity::setAffinity() failed: Invalid affinity CPU [" + std::to_string((size_u)(cpu+1)) + " of " + std::to_string((size_u)numa) + "]");
		} else
			throw util::app_error("TThreadAffinity::setAffinity() failed: No CPU set allocated." );
	} else
		throw util::app_error("TThreadAffinity::setAffinity() failed: Invalid thread id." );

	return retVal;
};



/*
 * TThreadUtil
 */
TThreadUtil::TThreadUtil() {
}

TThreadUtil::~TThreadUtil() {
}

pid_t TThreadUtil::gettid() {
	return (pid_t)syscall(SYS_gettid);
}

bool TThreadUtil::createObjectThread(pthread_t& thread, TThreadHandler handler, EThreadType type, void *object, const char* name, int priority, size_t stack) {
	if (thread < 1) {
		pthread_attr_t attr;

		int retVal = pthread_attr_init(&attr);
		if (util::checkFailed(retVal))
			throw util::sys_error("TThreadUtil::createDetachedThread::pthread_attr_init() failed.", errno);

		/* Set scheduler priority */
		if (priority != THREAD_DEFAULT_PRIO) {
			sched_param parm;
			parm.sched_priority = priority;
			retVal = pthread_attr_setschedparam(&attr, &parm);
			if (util::checkFailed(retVal))
				throw util::sys_error("TThreadUtil::createDetachedThread::pthread_attr_setschedparam failed", errno);
		}

		/* Increase stack size in regard of working with class members in thread routine... */
		if (stack > (size_t)THREAD_DEFAULT_STACK) {
			retVal = pthread_attr_setstacksize(&attr, stack);
			if (util::checkFailed(retVal))
				throw util::sys_error("TThreadUtil::createDetachedThread::pthread_attr_setstacksize() failed.", errno);
		}

		retVal = pthread_attr_setdetachstate(&attr, (int)type);
		if (util::checkFailed(retVal))
			throw util::sys_error("TThreadUtil::createDetachedThread::pthread_attr_setdetachstate() failed.", errno);

		retVal = pthread_create(&thread, &attr, handler, object);
		if (util::checkFailed(retVal))
			throw util::sys_error("TThreadUtil::createDetachedThread::pthread_create() failed.", errno);

		retVal = pthread_attr_destroy(&attr);
		if (util::checkFailed(retVal))
			throw util::sys_error("TThreadUtil::createDetachedThread::pthread_attr_destroy() failed.", errno);

		if (thread < 1)
			throw util::app_error("TThreadUtil::createDetachedThread::pthread_create() failed with invalid thread id : " + std::to_string((size_u)thread));

		if (util::assigned(name)) {
			if (strnlen(name, 20) > 15) {
				// Limit name size to 15 characters + NULL
				char desc[18];
				strncpy(desc, name, 15);
				desc[14] = '~';
				desc[15] = '\0';
				retVal = pthread_setname_np(thread, desc);
			} else {
				retVal = pthread_setname_np(thread, name);
			}
			if (util::checkFailed(retVal))
				throw util::sys_error("TThreadUtil::createDetachedThread::pthread_setname_np() failed.", errno);
		} else {
			retVal = pthread_setname_np(thread, "Anonimous");
			if (util::checkFailed(retVal))
				throw util::sys_error("TThreadUtil::createDetachedThread::pthread_setname_np() failed.", errno);
		}

		return true;
	}
	thread = 0;
	return false;
}

int TThreadUtil::terminateThread(pthread_t& thread) {
	if (thread > 1) {
		void *result;
		errno = EXIT_SUCCESS;
		int retVal = EXIT_SUCCESS;
		int r = pthread_join(thread, &result);
		if (util::checkFailed(r))
			retVal = r;
		thread = 0;
		return retVal;
	}
	errno = EINVAL;
	return EXIT_FAILURE;
}



/*
 * TBaseThread
 */
TBaseThread::TBaseThread() {
	thread = 0;
	process = 0;
	debug = false;
	started = false;
	terminate = false;
	terminated = false;
	globalMtx = nil;
	exceptionLog  = nil;
	infoLog = nil;
	owner = nil;
}

TBaseThread::~TBaseThread() {
}

bool TBaseThread::createPersistentObjectThread(void* object, const char* name) {
	bool r = createObjectThread(thread, threadMethodDispatcher, THD_CREATE_JOINABLE, object, name);
	setStarted(r);
	return r;
}

bool TBaseThread::createIntermittentObjectThread(void* object, const char* name) {
	pthread_t thd = 0;
	bool r = createObjectThread(thd, threadSignalDispatcher, THD_CREATE_DETACHED, object, name);
	setStarted(r);
	return r;
}

int TBaseThread::terminatePersistentThread() {
	int r = terminateThread(thread);
	setTerminated(true);
	return r;
}


void TBaseThread::setProperties(const TThreadProperties thread) {
	this->owner = thread.owner;
	this->globalMtx = thread.mtx;
	this->infoLog = thread.infoLog;
	this->exceptionLog = thread.exceptionLog;
	this->process = thread.process;
}


int TBaseThread::checkProperties() {
	if ( !util::assigned(owner) ||
		 !util::assigned(infoLog) ||
		 !util::assigned(exceptionLog) ||
		 !util::assigned(globalMtx) ) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


bool TBaseThread::isStarted() const {
	std::lock_guard<std::mutex> lock(statusMtx);
	return started;
}

void TBaseThread::setStarted(const bool value) {
	std::lock_guard<std::mutex> lock(statusMtx);
	started = value;
}

bool TBaseThread::isTerminating() const {
	std::lock_guard<std::mutex> lock(statusMtx);
	return terminate;
}

void TBaseThread::setTerminate(bool terminate) {
	std::lock_guard<std::mutex> lock(statusMtx);
	if (this->terminate != terminate) {
		this->terminate = terminate;
		if (terminate)
			unlock();
	}
}

bool TBaseThread::isTerminated() const {
	std::lock_guard<std::mutex> lock(statusMtx);
	return terminated;
}

void TBaseThread::setTerminated(bool terminated) {
	std::lock_guard<std::mutex> lock(statusMtx);
	this->terminated = terminated;
	if (terminated)
		started = false;
}


void TBaseThread::writeLog(const std::string& s) {
	if (util::assigned(infoLog))
		infoLog->write("[Threads] [" + name + "] " + s);
}

void TBaseThread::debugLog(const std::string& s) {
	if (util::assigned(infoLog) && debug)
		infoLog->write("[Threads] [" + name + "] " + s);
}

void TBaseThread::errorLog(const std::string& s) {
	if (util::assigned(exceptionLog)) {
		exceptionLog->write("[" + name + "] " + s);
		infoLog->write("[Threads] [" + name + "] " + s);
	}
}


pid_t TBaseThread::gettid() {
	return TThreadUtil::gettid();
}


int TBaseThread::receiver(void* message) {
	if (util::assigned(message)) {
		// Restore sliced virtual methods by calling derived methods from within derived class object
		return receive(message);
	}
	return EXIT_FAILURE;
}

int TBaseThread::dispatcher(void* creator) {
	// Restore sliced virtual methods by calling derived methods from within derived class object
	return run(creator);
}

void TBaseThread::wait() {
	// Restore sliced virtual methods by calling derived methods from within derived class object
	waitFor();
}

void TBaseThread::unlock() {
	this->unlockThreadSync();
}


/*
 * TManagedThread
 */
TManagedThread::TManagedThread(const std::string& name) : TBaseThread(), TThreadAffinity() {
	initialize();
	this->name = name;
}

// Delegating ctors since gcc 4.7
// TBaseThread::TBaseThread(const string& name, const TThreadProperties thread) : TBaseThread(name){
// --> calls ctor of the same class!
#ifdef STL_HAS_DELEGATING_CTOR
TManagedThread::TManagedThread(const std::string& name, const TThreadProperties thread) : TManagedThread(name) {
#else
TManagedThread::TManagedThread(const std::string& name, const TThreadProperties thread) : TBaseThread(), TThreadAffinity() {
	initialize();
	this->name = name;
#endif
	this->owner = thread.owner;
	this->globalMtx = thread.mtx;
	this->infoLog = thread.infoLog;
	this->exceptionLog = thread.exceptionLog;
	this->process = thread.process;
}

TManagedThread::TManagedThread(const std::string& name, std::mutex& mtx, pid_t process, TThreadController& owner,
			TLogFile& infoLog, TLogFile& exceptionLog) : TBaseThread(), TThreadAffinity() {
	initialize();
	this->name = name;
	this->owner = &owner;
	this->globalMtx = &mtx;
	this->infoLog = &infoLog;
	this->exceptionLog = &exceptionLog;
	this->process = process;
}

TManagedThread::~TManagedThread() {
	util::freeAndNil(sema);
}

void TManagedThread::initialize() {
	affinity = app::nsizet;
	tid = 0;
	owner = nil;
	executed = false;
	threadExecMethod = nil;
	threadMessageMethod = nil;
	sema = new TSemaphore(0, LCK_PROC_PRIVATE, "");
}

bool TManagedThread::hasMessageHandler() const {
	return threadMessageMethod != nil;
}

bool TManagedThread::hasExecHandler() const {
	return threadExecMethod != nil;
}

int TManagedThread::receive(void* message) {
	if (util::assigned(message)) {
		app::PThreadMessage msg = (app::PThreadMessage)message;
		receiveSignal(msg);
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

void TManagedThread::receiveSignal(PThreadMessage message) {
	try {
		if (hasMessageHandler()) {
			threadMessageMethod(*this, message->msg);
		}
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sText = "TBaseThread::receiveSignal : Exception in threadMessageMethod() for [" + name + "] " + sExcept;
		errorLog(sText);
	}

	if (message->type == THD_MTT_SEND)
		sema->post();

	msgSlot.release(message);
}

void TManagedThread::sendSignal(const TThreadMessage& message)
{
	if (hasMessageHandler()) {
		// Internal message parameter
		PThreadMessage msg = msgSlot.enqueue();
		msg->sender = this;
		msg->type = message.type;

		// User message parameter
		msg->msg = message.msg;
		msg->data = message.data;

		try {
			createSignalThread(msg);
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::string sText = "TBaseThread::sendSignal() Exception in createSignalThread() for [" + name + "] " + sExcept;
			errorLog(sText);
		}
	}
}


void TManagedThread::sendThreadMessage(EThreadMessageType message, void *data) {
	if (hasMessageHandler()) {
		{	// Open locking scope --> lock_guard lock.lock()
			std::lock_guard<std::mutex> lock(*globalMtx);
			TThreadMessage msg;
			msg.msg = message;
			msg.data = data;
			msg.type = THD_MTT_SEND;
			sendSignal(msg);
		}	// Leaving scope --> lock.unlock();

		// Wait for asynchronous handling in detached signal thread finished
		sema->wait();
	}
}


void TManagedThread::sendMessage(EThreadMessageType message) {
	sendThreadMessage(message, nil);
}

void TManagedThread::postThreadMessage(EThreadMessageType message, void *data) {
	if (hasMessageHandler()) {
		std::lock_guard<std::mutex> lock(*globalMtx);
		TThreadMessage msg;
		msg.msg = message;
		msg.data = data;
		msg.type = THD_MTT_POST;
		sendSignal(msg);
		// Return to sender, while detached signal thread is still working...
	}
}

void TManagedThread::postMessage(EThreadMessageType message) {
	postThreadMessage(message, nil);
}


void TManagedThread::createBaseThread(PThreadCreator creator) {
	if (util::checkFailed(checkProperties()))
		throw util::app_error("TBaseThread::createBaseThread() Thread properties not set.");
	const char* desc = !name.empty() ? name.c_str() : nil;
	if (!createPersistentThread(creator, desc))
		throw util::app_error("TBaseThread::createBaseThread() Create thread failed.");
}

void TManagedThread::createSignalThread(PThreadMessage message) {
	const char* desc = !name.empty() ? name.c_str() : nil;
	if (!createIntermittentThread(message, desc))
		throw util::app_error("TBaseThread::createSignalThread() Create thread failed.");
}


void TManagedThread::prepare(void* data, size_t cpu) {
	app::TLockGuard<app::TMutex> lock(execMtx);
	if (cpu != app::nsizet)
		affinity = cpu;
	creator.data = data;
}

void TManagedThread::start(void* data, size_t cpu) {
	app::TLockGuard<app::TMutex> lock(execMtx);
	if (!executed && hasExecHandler()) {
		executed = true;
		lock.unlock();
		terminate = false;
		terminated = false;
		if (cpu != app::nsizet)
			affinity = cpu;
		creator.data = data;
		creator.sender = this;
		createBaseThread(&creator);
	}
}

void TManagedThread::execute() {
	start(creator.data, affinity);
}

bool TManagedThread::isExecuted() const {
	app::TLockGuard<app::TMutex> lock(execMtx);
	return executed;
}

int TManagedThread::run(void* creator) {
	int retVal = EXIT_FAILURE;
	{
		TThreadSaveBooleanGuard<bool> btm(terminate, statusMtx, true);
		TThreadSaveBooleanGuard<bool> btd(terminated, statusMtx, true);
		try {
			// Set thread's CPU affinity
			tid = TBaseThread::gettid();
			if (affinity != app::nsizet)
				setAffinity(affinity, tid);

			// Signal start of thread to calling process
			sendMessage(THD_MSG_INIT);

			// Call thread's execution method
			if (hasExecHandler()) {
				retVal = threadExecMethod(*this);
			}
			if (util::checkFailed(retVal)) {
				std::string sText = util::csnprintf("TBaseThread::run() : threadExecMethod() for [" + name + "] failed on error code = %", retVal);
				writeLog(sText);
			}
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::string sText = "TBaseThread::run() : Exception in threadExecMethod() of [" + name + "] " + sExcept;
			errorLog(sText);
		} catch (...)	{
			std::string sText = "TBaseThread::run() : Unknown exception in threadExecMethod() of [" + name + "]";
			errorLog(sText);
		}
	}
	// Thread is done...
	sendMessage(THD_MSG_QUIT);
	return retVal;
}

void TManagedThread::unlockThreadSync() {
	// Not needed for managed threads
}


void TManagedThread::waitFor() {
	if (thread > 0) {
		int retVal, c = 0;
		while (!msgSlot.empty() && c < 60) {
			c++;
			util::saveWait(30);
		}
		retVal = terminatePersistentThread();
		if (util::checkFailed(retVal))
			throw util::sys_error("app::TBaseThread::waitFor::pthread_join() failed.", errno);
		msgSlot.clear();
	}
}


/*
 * TThreadList
 */
TThreadController::TThreadController(TLogFile& infoLog, TLogFile& exeptionLog) {
	this->infoLog = &infoLog;
	this->exceptionLog = &exeptionLog;
	initialize();
}


TThreadController::~TThreadController() {
	clear();
}

void TThreadController::initialize() {
	// Removed signal usage, left method as an example for installing a customized signal handler
	// addSignalHandler(signalDispatcher, SIGRTBTHREAD);
}

void TThreadController::addSignalHandler(TSignalHandler handler, int signal)
{
	struct sigaction action;
	sigset_t systemMask, signalMask;
	int errnum;

	infoLog->write("[Threads] Install signal handler for SIGRTBTHREAD (" + std::to_string((size_s)signal) + ")");

	/* Create alternate stack for RT signal handler */
	stack_t stack;
	size_t size = 8 * SIGSTKSZ;
	util::TBuffer* buffer = new util::TBuffer(size);
	stack.ss_sp = buffer->data();
	if (!util::assigned(stack.ss_sp))
		throw util::sys_error("TThreadList::installSignalHandler::buffer->data() failed", errno);

	stack.ss_size = size;
	stack.ss_flags = 0;
	errnum = sigaltstack(&stack, NULL);
	if (util::checkFailed(errnum))
		throw util::sys_error("TThreadList::installSignalHandler::sigaltstack() failed", errno);

	errnum = sigfillset(&signalMask);
	if (util::checkFailed(errnum))
		throw util::sys_error("TThreadList::installSignalHandler::sigfillset() failed", errno);

	errnum = pthread_sigmask(SIG_SETMASK, &signalMask, &systemMask);
	if (util::checkFailed(errnum))
		throw util::sys_error("TThreadList::installSignalHandler::pthread_sigmask() failed", errno);

	errnum = sigemptyset(&signalMask);
	if (util::checkFailed(errnum))
		throw util::sys_error("TThreadList::installSignalHandler::sigemptyset() failed", errno);

	memset(&action, 0, sizeof(action));

	/* Block all signals in signal handler --> use sigfillset! */
	errnum = sigfillset(&action.sa_mask);
	if (util::checkFailed(errnum))
		throw util::sys_error("TThreadList::installSignalHandler::sigfillset() failed", errno);

	/* Block own RT signal within signal handler --> leave option SA_NODEFER unset*/
	action.sa_sigaction = handler;
	action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;

	if (sigaction(signal, &action, NULL) < 0)
		throw util::app_error("TThreadList::installSignalHandler() Could not register SIGRTBTHREAD handler");

	errnum = sigaddset(&signalMask, signal);
	if (util::checkFailed(errnum)) {
		throw util::app_error("TThreadList::installSignalHandler::sigaddset() failed.");
	}

	errnum = pthread_sigmask(SIG_UNBLOCK, &signalMask, NULL);
	if (util::checkFailed(errnum))
		throw util::sys_error("TThreadList::installSignalHandler::pthread_sigmask() failed", errno);

	errnum = pthread_sigmask(SIG_SETMASK, &systemMask, NULL);
	if (util::checkFailed(errnum))
		throw util::sys_error("TThreadList::installSignalHandler::pthread_sigmask() failed", errno);
}

void TThreadController::terminate() {
	infoLog->write("[Threads] Shutdown threads.");
	
#ifndef STL_HAS_RANGE_FOR
	PBaseThread o;
	for (size_t i=0; i<threadList.size(); i++) {
		o = threadList[i];
		if (util::assigned(o))
			o->setTerminate(true);
	}
#else
	for (PBaseThread o : threadList) {
		if (util::assigned(o))
			o->setTerminate(true);
	}
#endif
}

void TThreadController::waitFor() {
	// Wait for all threads to be done
	infoLog->write("[Threads] Waiting for all threads terminated...");

#ifndef STL_HAS_RANGE_FOR
	PBaseThread o;
	for (size_t i=0; i<threadList.size(); i++) {
		o = threadList[i];
		if (util::assigned(o)) {
			while (!o->isTerminated())
				util::saveWait(30);
			o->wait();
		}
	}
#else
	for (PBaseThread o : threadList) {
		if (util::assigned(o)) {
			if (o->isStarted()) {
				while (!o->isTerminated()) {
					util::saveWait(30);
				}
				o->wait();
			}
		}
	}
#endif

	infoLog->write("[Threads] All threads terminated.");
}

void TThreadController::clear()
{
#ifndef STL_HAS_RANGE_FOR
	PBaseThread o;
	for (size_t i=0; i<threadList.size(); i++) {
		o = threadList[i];
		if (util::assigned(o))
			delete o;
	}
#else
	for (PBaseThread o : threadList) {
		if (util::assigned(o))
			delete o;
	}
#endif

	threadList.clear();
}




TThreadDataItem::TThreadDataItem() {
	prime();
}

TThreadDataItem::TThreadDataItem(util::TBuffer& data) {
	prime();
	initialize(data);
}

TThreadDataItem::TThreadDataItem(const void *const data, size_t size) {
	prime();
	initialize(data, size);
}

TThreadDataItem::~TThreadDataItem() {
	data.clear();
}

void TThreadDataItem::prime() {
	refc = 0;
	bIsZipped = false;
	timestamp = util::now();
}

void TThreadDataItem::initialize(const void *const data, size_t size) {
	this->data.move((char*)data, size);
}

void TThreadDataItem::initialize(util::TBuffer& data) {
	this->data.move(data);
}

void TThreadDataItem::finalize(bool free) {
	decRefCount();
	timestamp = util::now();
	if (refc <= 0) {
		// Cleanup by caller?
		if (free) {
			data.clear();
		}
		refc = 0;
	}
}

void TThreadDataItem::getData(const void*& data, size_t& size) {
	data = this->data.data();
	size = this->data.size();
	timestamp = util::now();
	refc++;
}

void TThreadDataItem::getData(util::TBuffer& data) {
	// Data is duplicated by assignement operator
	// --> No reference counting!
	data = this->data;
	timestamp = util::now();
}

util::TBuffer& TThreadDataItem::getData() {
	refc++;
	timestamp = util::now();
	return data;
}




TThreadData::TThreadData(util::TTimePart delay) : delay {delay} {
	// Delay in milliseconds = 0 --> delete invalidated buffer immediately
	prime();
}

TThreadData::~TThreadData() {
	clear();
	waitFor();
	clearReceiverList();
}

void TThreadData::prime() {
	hash = 0;
	bUseZip = false;
	onDataNeeded = nil;
	onDataReceived = nil;
	thread.setName("TThreadData::receiver()");
	thread.setExecHandler(&app::TThreadData::threadExecuter, this);
}

void TThreadData::setDelay(util::TTimePart delay) {
	std::lock_guard<std::mutex> lock(dataMtx);
	this->delay = delay;
}

PWebDataReceiver TThreadData::findReceiverSlot() {
	std::lock_guard<std::mutex> lock(threadMtx);
	PWebDataReceiver o = nil;
	bool found = false;

	// Find empty action slot
	for (size_t i=0; i<threadList.size(); i++) {
		o = threadList[i];
		if (util::assigned(o)) {
			if (!o->running) {
				found = true;
				break;
			}
		}
	}

	// Create new action object
	if (!found) {
		o = new TWebDataReceiver;
		o->mtx = &threadMtx;
		o->sender = this;
		o->thread = new TWebDataThread(&app::TThreadData::threadExecuter, this);
		o->thread->setName("WebDataThread");
		threadList.push_back(o);
	}

	// Set running flag as early as possible
	// to mark item as used
	o->running = true;

	return o;
}

void TThreadData::executeAction(const TThreadDataReceived& handler, util::TBuffer& data, const std::string& url,
		const util::TVariantValues& params, const util::TVariantValues& session, const bool deferred, const bool zipped, int& error) {
	// Create action object for thread execution
	PWebDataReceiver receiver = findReceiverSlot();

	// Set current thread data
	receiver->data.move(data);
	receiver->handler = handler;
	receiver->params = params;
	receiver->session = session;
	receiver->zipped = zipped;
	receiver->error = error;
	receiver->url = url;
	TWebDataReceiver& r = *receiver;

	if (deferred) {
		// Execute action via thread
		thread.run(r);
	} else {
		// Execute action immediately via thread method
		threadExecuter(*receiver);
	}

	// Return result value (can take effect on synchronous execution only)
	error = receiver->error;
}

void TThreadData::threadExecuter(TWebDataReceiver& receiver) {
	TThreadDataActionGuard<TWebDataReceiver> guard(receiver);
	try {
		guard.setRunning(true);
		receiver.handler(*receiver.sender, receiver.data.data(), receiver.data.size(), receiver.params, receiver.session, receiver.zipped, receiver.error);
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		std::string sName = "TThreadData::threadExecuter(\"" + receiver.url + "\")";
		std::string sText = "[EXCEPTION] Exception in " + sName + " :\n" + sExcept + "\n";
		// receiver.logger.write(sText);
	} catch (...)	{
		std::string sName = "TThreadData::threadExecuter(\"" + receiver.url + "\")";
		std::string sText = "[EXCEPTION] Exception in " + sName;
		// receiver.logger.(sText);
	}
	// Data is no longer needed by thread!
	receiver.reset();
}

void TThreadData::clearReceiverList() {
	if (!threadList.empty()) {
		std::lock_guard<std::mutex> lock(threadMtx);
		PWebDataReceiver o;
		for (size_t i=0; i<threadList.size(); i++) {
			o = threadList[i];
			util::freeAndNil(o);
		}
		threadList.clear();
	}
}

void TThreadData::waitFor() {
	// Wait for all actions to be done
	bool running;
	if (!threadList.empty()) {
		PWebDataReceiver o;
		do {
			{ // Scope mutex threadMtx
				std::lock_guard<std::mutex> lock(threadMtx);
				running = false;
				for (size_t i=0; i<threadList.size(); i++) {
					o = threadList[i];
					if (util::assigned(o)) {
						if (o->running) {
							running = true;
							break;
						}
					}
				}
			} // End of scope mutex threadMtx
			if (running)
				util::saveWait(30);
		} while (running);
	}
}

PThreadDataItem TThreadData::setData(util::TBuffer& data, const std::string& url, const util::TVariantValues& params, const util::TVariantValues& session, bool deferred, bool zipped, int& error) {
	PThreadDataItem o = nil;
	if (!data.empty()) {

		if (nil != onDataReceived) {
			std::lock_guard<std::mutex> lock(dataMtx);
			char* dest = data.data();
			size_t size = data.size();

			// Unzip data if requested
			if (zipped) {
				// Do unzip action
				const char* src = (const char*)data.data();
				size = zip.gunzip(src, dest, size);
				o = new TThreadDataItem(dest, size);
			} else {
				// Take ownership (move) of given data
				// --> Original data is cleared by move in thread object
				o = new TThreadDataItem(data);
			}

			// Execute data action
			if (util::assigned(o)) {
				// Add object to data list
				list.push_back(o);

				// Execute callback with given data...
				executeAction(onDataReceived, o->getData(), url, params, session, deferred, zipped, error);
			}

		}

		// Original data is no longer needed!
		data.clear();

	}
	return o;
}

PThreadDataItem TThreadData::getData(const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers,
		bool& zipped, bool& cached, int& error, bool forceUpdate) {
	PThreadDataItem o = nil;
	if (nil != onDataNeeded) {
		std::lock_guard<std::mutex> lock(dataMtx);
		zipped = bUseZip && zipped;
		o = dataNeeded(params, session, headers, zipped, cached, error, forceUpdate);
		if (util::assigned(o)) {
			o->getData(data, size);
		} else {
			data = nil;
			size = 0;
		}
	}
	return o;
}

PThreadDataItem TThreadData::dataNeeded(const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers,
		bool& zipped, bool& cached, int& error, bool forceUpdate) {
	PThreadDataItem o = nil;
	if (!compareParams(params) || forceUpdate) {
		const void* p = nil;
		size_t n = 0;
		onDataNeeded(*this, p, n, params, session, headers, zipped, cached, error);
		if (util::assigned(p) && n > 0) {
			char* q = nil;
			if (zipped) {
				// GZip data to new buffer
		    	n = zip.gzip((char*)p, q, n);
			} else {
				// Just a plain copy...
				q = new char[n];
				if (util::assigned(q))
					::memcpy(q, p, n);
			}
			if (util::assigned(q) && n > 0) {
				o = new TThreadDataItem(q, n);
				o->setZipped(zipped);
				list.push_back(o);
			}
		}
		hash = params.hash();
	}
	return o;
}

bool TThreadData::compareParams(const util::TVariantValues& params) {
	return hash == params.hash();
}

void TThreadData::finalize(PThreadDataItem& data) {
	std::lock_guard<std::mutex> lock(dataMtx);
	bool free = delay <= 0;
	if (util::assigned(data)) {
		data->finalize(free);
	}
	release();
	data = nil;
}

void TThreadData::clear()
{
#ifndef STL_HAS_RANGE_FOR
	PThreadDataItem o;
	for (size_t i=0; i<threadList.size(); i++) {
		o = list[i];
		util::freeAndNil(o);
	}
#else
	for (PThreadDataItem o : list) {
		util::freeAndNil(o);
	}
#endif
	hash = 0;
	list.clear();
	release(true);
}


struct CItemEraser
{
	CItemEraser(util::TTimePart delay, util::TTimePart now) : _delay(delay), _now(now) {}
	util::TTimePart _delay;
	util::TTimePart _now;
    bool operator()(PThreadDataItem o) const {
    	bool retVal = true;
    	if (util::assigned(o)) {
			retVal = (((_now - o->getTimeStamp()) > (_delay / 1000)) && (o->getRefCount() <= 0) /* && o->hasData() */);
			if (retVal)
				util::freeAndNil(o);
    	}
    	return retVal;
    }
};

size_t TThreadData::release(bool force) {
	// Private method, MUST (!) be locked by caller!
	// std::lock_guard<std::mutex> lock(dataMtx);
	util::TTimePart _delay = force ? util::epoch() : delay;
	int retVal = 0;
	if (list.size() > 0) {
		size_t size = list.size();
		util::TTimePart now = util::now();

		// Delete invalidated buffers by age
		list.erase(std::remove_if(list.begin(), list.end(), CItemEraser(_delay, now)), list.end());
		retVal += size - list.size();

	}
	return retVal;
}

size_t TThreadData::garbageCollector() {
	std::lock_guard<std::mutex> lock(dataMtx);
	return release();
}

} /* namespace app */
