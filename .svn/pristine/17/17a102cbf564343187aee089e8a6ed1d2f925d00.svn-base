/*
 * timer.cpp
 *
 *  Created on: 30.08.2014
 *      Author: Dirk Brinkmeier
 */

#include <unistd.h>
#include "timer.h"
#include "typeid.h"
#include "threads.h"
#include "functors.h"
#include "templates.h"
#include "exception.h"
#include "random.h"
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>


static void timerCallbackDispatcher(sigval_t signal) {
	if (util::assigned(signal.sival_ptr))
		((app::TBaseTimer*)signal.sival_ptr)->onTimer();
}


namespace app {

/*
 * TBaseTimer
 */
TBaseTimer::TBaseTimer(std::mutex& mtx) {
	this->owner = nil;
	this->attr = nil;
	this->id = nil;
	this->mtx = &mtx;
	this->delay = ndelay;
	createTimer();
}

TBaseTimer::~TBaseTimer() {
	stopTimer();
	destroyTimer();
}

void TBaseTimer::addEvent(const app::TEventHandler& handler) {
	std::lock_guard<std::mutex> lock(*mtx);
	evList.push_back(handler);
}

void TBaseTimer::createTimer() {
    if (!util::assigned(id)) {
    	sigevent_t sev;
		sched_param parm;
		size_t size;
		int errnum;

		if (!util::assigned(attr))
			attr = new pthread_attr_t;

		errnum = pthread_attr_init(attr);
		if (util::checkFailed(errnum))
			throw util::sys_error("TBaseTimer::createTimer(): pthread_attr_init failed", errno);

		parm.sched_priority = THREAD_DEFAULT_PRIO;
		errnum = pthread_attr_setschedparam(attr, &parm);
		if (util::checkFailed(errnum))
			throw util::sys_error("TBaseTimer::createTimer(): pthread_attr_setschedparam failed", errno);

		/* Increase stack size in regard of working with class members in thread routine... */
		size = THREAD_STACK_SIZE;
		errnum = pthread_attr_setstacksize(attr, size);
		if (EXIT_SUCCESS != errnum)
			throw util::sys_error("TBaseTimer::createTimer(): pthread_attr_setstacksize() failed.", errno);

		sev.sigev_signo = 0;
		sev.sigev_notify = SIGEV_THREAD;
		sev.sigev_notify_attributes = attr;
		sev.sigev_value.sival_ptr = (void*)this;
		sev.sigev_notify_function = &timerCallbackDispatcher;

		errnum = timer_create(CLOCKIDR, &sev, &id);
		if (util::checkFailed(errnum))
			throw util::sys_error("TBaseTimer::createTimer(): timer_create failed", errno);
    }
}


void TBaseTimer::destroyTimer() {
	int errnum = EXIT_SUCCESS;
	if (util::assigned(id)) {
		errnum = timer_delete(id);
		id = nil;
    }
    util::freeAndNil(attr);
	if (util::checkFailed(errnum))
		throw util::sys_error("TBaseTimer::destroyTimer(): timer_delete failed", errno);
}


void TBaseTimer::startTimer() {
    if ((delay != ndelay) && (delay > 0) && util::assigned(id)) {
        itimerspec its;

        /* Cyclic timer, delay in ms */
		its.it_value.tv_sec = delay / 1000;
		its.it_value.tv_nsec = (delay % 1000) * 1000000;
		its.it_interval.tv_sec = its.it_value.tv_sec;
		its.it_interval.tv_nsec = its.it_value.tv_nsec;

		int errnum = timer_settime(id, 0, &its, nil);
	    if (util::checkFailed(errnum))
			throw util::sys_error("TBaseTimer::startTimer(): timer_settime failed", errno);
    }
}


void TBaseTimer::stopTimer() {
    if (util::assigned(id)) {
		itimerspec its;
		its.it_value.tv_sec = 0;
		its.it_value.tv_nsec = 0;
		its.it_interval.tv_sec = 0;
		its.it_interval.tv_nsec = 0;

		int errnum = timer_settime(id, 0, &its, nil);
		if (util::checkFailed(errnum))
			throw util::sys_error("TBaseTimer::stopTimer(): timer_settime failed", errno);
    }
}

int TBaseTimer::getOverrun() const {
	return timer_getoverrun(id);
}

void TBaseTimer::onTimer() {
#ifdef STL_HAS_RANGE_FOR
	for (auto method : evList) {
		try {
			method();
		} catch (...)	{
			// TODO
		}
	}
#else
	TEventHandler method;
	size_t i,n;
	n = evList.size();
	for (i=0; i<n; i++) {
		method = evList[i];
		try {
			method();
		} catch (const std::exception& e)	{
			// TODO
		}
	}
#endif
}



/*
 * TSystemTimer
 */
TSystemTimer::TSystemTimer(std::mutex& mtx, int resolution) : TBaseTimer(mtx) {
	delay = resolution;
	started = false;
	start();
}

TSystemTimer::~TSystemTimer() {
	stopTimer();
	destroyTimer();
}

void TSystemTimer::destroy() {
	std::lock_guard<std::mutex> lock(*mtx);
	stopTimer();
	destroyTimer();
}

void TSystemTimer::start() {
	std::lock_guard<std::mutex> lock(*mtx);
	if (!started) {
		startTimer();
		started = true;
	}
}

void TSystemTimer::stop() {
	std::lock_guard<std::mutex> lock(*mtx);
	if (started) {
		stopTimer();
		started = false;
	}
}


/*
 * TTimer
 */
TTimer::TTimer() : TBaseTimer(*(localMtx = new std::mutex)) {
	name = util::fastCreateUUID();
	section = "";
	delay = ndelay;
	config = nil;
	infoLog = nil;
	exceptionLog = nil;
	enabled = false;
	running = false;
	terminate = false;
	cyclicViolation = false;
}


TTimer::TTimer(const std::string& section, const std::string& name, TTimerDelay delay, const TEventHandler& onTimer,
		 	 	  std::mutex& mtx, app::TIniFile& config, TLogFile& infoLog, TLogFile& exceptionLog ) : TBaseTimer(mtx) {
	localMtx = nil;
	this->name = name;
	this->section = section;
	this->delay = delay;
	this->config = &config;
	this->infoLog = &infoLog;
	this->exceptionLog = &exceptionLog;
	enabled = true;
	running = false;
	terminate = false;
	cyclicViolation = false;
	addEvent(onTimer);
	reWriteConfig();
	startTimer();
}

TTimer::~TTimer() {
	stopTimer();
	destroyTimer();
	util::freeAndNil(localMtx);
}

TTimerDelay TTimer::getDelay() const {
	std::lock_guard<std::mutex> lock(*mtx);
	return delay;
}

void TTimer::setDelay(TTimerDelay delay) {
	std::lock_guard<std::mutex> lock(*mtx);
	if (this->delay != delay) {
		if (enabled) {
			stopTimer();
			startTimer();
		}
	}
	this->delay = delay;
}

bool TTimer::isEnabled() const {
	std::lock_guard<std::mutex> lock(*mtx);
	return enabled;
}

void TTimer::setEnabled(bool value) {
	std::lock_guard<std::mutex> lock(*mtx);
	if (value) {
		if (!enabled)
			startTimer();
	} else {		
		if (enabled)
			stopTimer();
	}
	enabled = value;
}


bool TTimer::isRunning() const {
	std::lock_guard<std::mutex> lock(*mtx);
	return running;
}


void TTimer::readConfig() {
	if (util::assigned(config)) {
		config->setSection(section);
		delay = config->readInteger(name, delay);
	}
}


void TTimer::writeConfig() {
	if (util::assigned(config)) {
		config->setSection(section);
		config->writeInteger(name, delay);
	}
}


void TTimer::reWriteConfig() {
	readConfig();
	writeConfig();
}


void TTimer::writeInfo(const std::string text) {
	if (util::assigned(infoLog))
		infoLog->write(text);
}

void TTimer::writeError(const std::string text) {
	if (util::assigned(exceptionLog))
		exceptionLog->write(text);
}


void TTimer::onTimer() {
	std::unique_lock<std::mutex> lock(*mtx);
	if (evList.size() > 0) {
		if (enabled && !terminate) {
			if (!running) {
				cyclicViolation = false;
				running = true;
				lock.unlock();
#ifdef STL_HAS_RANGE_FOR
				for (auto method : evList) {
					try {
						method();
					} catch (const std::exception& e)	{
						std::string sExcept = e.what();
						std::string sName = util::nameOf(method);
						std::string sText = "Exception in timer [" + name + "] : " + sExcept;
						exceptionLog->write(sText);
						infoLog->write(sText);
					} catch (...)	{
						std::string sName = util::nameOf(method);
						std::string sText = "Unknown exception in timer [" + name + "]";
						exceptionLog->write(sText);
						infoLog->write(sText);
					}
				}
#else
				TEventHandler method;
				for (size_t i=0; i<evList.size(); i++) {
					method = evList[i];
					try {
						method();
					} catch (const std::exception& e)	{
						std::string sExcept = e.what();
						std::string sName = util::className(method);
						std::string sText = "Exception in timer [" + name + "] : " + sExcept;
						exceptionLog->write(sText);
						infoLog->write(sText);
					} catch (...)	{
						std::string sName = util::className(method);
						std::string sText = "Unknown exception in timer [" + name + "]";
						exceptionLog->write(sText);
						infoLog->write(sText);
					}
				}
#endif
				lock.lock();
				running = false;
			} else {
				if (!cyclicViolation) {
					cyclicViolation = true;
					infoLog->write("Cycle time violation in timer [%] : Duration > % ms", name, delay);
				}
			}
		}
	}
}




/*
 * TTimerList
 */
TTimerController::TTimerController(const std::string& configFolder, TLogFile& infoLog, TLogFile& exceptionLog) {
	terminated = false;
	this->infoLog = &infoLog;
	this->exceptionLog = &exceptionLog;
	this->configFolder = configFolder;
	this->configFile = this->configFolder + "timer.conf";
	config = new app::TIniFile(configFile);
}

TTimerController::~TTimerController() {
	config->flush();
	destroy();
	clear();
	util::freeAndNil(config);
}


PTimer TTimerController::addTimer(const TTimerProperties timer) {
	return addTimer(timer.section, timer.name, timer.delay, timer.onTimer);
}


PTimer TTimerController::addTimer(const std::string& section, const std::string& name, TTimerDelay delay, const TEventHandler& onTimer) {
	std::lock_guard<std::mutex> lock(listMtx);
	app::PTimer o = nil;
	size_t idx = find(name);
	if (idx == nsizet) {
		o = new app::TTimer(section, name, delay, onTimer, timerMtx, *config, *infoLog, *exceptionLog);
	} else {
		throw util::app_error_fmt("TTimerList::addTimer() failed: Timer <%> duplicated.", name);
	}
	if (util::assigned(o)) {
		timerList.push_back(o);
	}
	return o;
}



size_t TTimerController::find(const std::string& name) {
	PTimer o;
	size_t i,n;
	n = timerList.size();
	for (i=0; i<n; i++) {
		o = timerList[i];
		if (util::assigned(o)) {
			if (o->name.compare(name) == 0) {
				return i;
			}
		}
	}
	return app::nsizet;
}


void TTimerController::clear() {
#ifndef STL_HAS_RANGE_FOR
	PTimer o;
	size_t i,n;
	n = timerList.size();
	for (i=0; i<n; i++) {
		o = timList[i];
		if (util::assigned(o)) {
			delete o;
		}
	}
#else
	for (PTimer o : timerList) {
		if (util::assigned(o)) {
			delete o;
		}
	}
#endif
	timerList.clear();
}


void TTimerController::destroy() {
	if (terminated)
		return;
		
	// Disables all timers and
	// frees system timers (irreversible!)
	std::lock_guard<std::mutex> lock(listMtx);
#ifndef STL_HAS_RANGE_FOR
	PTimer o;
	size_t i,n;
	n = timList.size();
	for (i=0; i<n; i++) {
		o = timerList[i];
		if (util::assigned(o)) {
			std::lock_guard<std::mutex> lock(*o->mtx);
			if (o->enabled)
				o->enabled = false;
			o->destroyTimer();
		}
	}
#else
	for (PTimer o : timerList) {
		if (util::assigned(o)) {
			std::lock_guard<std::mutex> lock(*o->mtx);
			if (o->enabled)
				o->enabled = false;
			o->destroyTimer();
		}
	}
#endif
}


void TTimerController::terminate() {
	if (terminated)
		return;

	infoLog->write("TTimerController::terminate() Shutdown timer.");
	{
		// Set termination flags
		std::lock_guard<std::mutex> lock(listMtx);
		PTimer o;
		size_t i,n;
		n = timerList.size();
		for (i=0; i<n; i++) {
			o = timerList[i];
			if (util::assigned(o)) {
				std::lock_guard<std::mutex> lock(*o->mtx);
				o->terminate = true;
			}
		}
	}

	// Destroy timer objects
	destroy();
}



void TTimerController::waitFor() {
	if (terminated)
		return;

	infoLog->write("TTimerController::waitFor() Waiting for all timers terminated...");
	std::lock_guard<std::mutex> lock(listMtx);

	bool finished;
	PTimer o;
	size_t i,n;

	n = timerList.size();
	do {
		finished = true;
		for (i=0; i<n; i++) {
			o = timerList[i];
			if (util::assigned(o)) {
				if (o->isRunning())
					finished = false;
			}
		}
		// Do some sort of nothing
		if (!finished)
			util::wait(50);
	} while (!finished);

	terminated = true;
	infoLog->write("TTimerController::waitFor() All timers terminated.");
}




} /* namespace app */
