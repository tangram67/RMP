/*
 * timeout.cpp
 *
 *  Created on: 07.09.2014
 *      Author: Dirk Brinkmeier
 */

#include "timeout.h"
#include "exception.h"

namespace app {

/*
 * TTimeoutTimer
 */
TTimeoutTimer::TTimeoutTimer(std::mutex& mtx, int resolution, TTimeoutController& owner) : TSystemTimer(mtx, resolution) {
	this->owner = &owner;
	delay = resolution;
}

void TTimeoutTimer::onTimer() {
	if (util::assigned(owner)) {
		try {
			owner->onTimer();
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::string sText = "Exception in TTimeoutTimer::onTimer() : " + sExcept;
			std::cout << sText << std::endl;
		} catch (...)	{
			std::string sText = "Unknown exception in TTimeoutTimer::onTimer()";
			std::cout << sText << std::endl;
		}
	}
}


/*
 * TTimeout
 */
TTimeout::TTimeout(const std::string& name, TTimerDelay timeout, std::mutex& mtx, app::TIniFile& config, TTimeoutController& owner, const bool signaled) {
	this->name = name;
	this->timeout = timeout;
	this->signaled = signaled;
	this->started = false;
	this->changed = false;
	this->phase = TTimer::ndelay;
	this->config = &config;
	this->owner = &owner;
	this->valueMtx = (std::mutex*)&mtx;
	this->onTimeoutHandler = nil;
	reWriteConfig();
}


TTimeout::~TTimeout() {
	// Nothing to do...
}


void TTimeout::start() {
	std::lock_guard<std::mutex> lock(*valueMtx);
	if (!started) {
		phase = owner->getTime() + timeout;
		signaled = false;
		started = true;
	}
}

TTimerDelay TTimeout::restart(const TTimerDelay timeout) {
	std::lock_guard<std::mutex> lock(*valueMtx);
	if (timeout > 0) {
		this->timeout = timeout;
	}
	phase = owner->getTime() + this->timeout;
	signaled = false;
	started = true;
	return this->timeout;
}

void TTimeout::stop() {
	std::lock_guard<std::mutex> lock(*valueMtx);
	if (started) {
		phase = TTimer::ndelay;
		signaled = false;
		started = false;
	}
}

void TTimeout::adjust(const TTimerDelay timeout) {
	std::lock_guard<std::mutex> lock(*valueMtx);
	if (this->timeout < timeout) {
		this->timeout = timeout;
		writeConfig();
		changed = true;
	}
}

void TTimeout::onTimeout() {
	if (nil != onTimeoutHandler) {
		std::lock_guard<std::mutex> lock(execMtx);
		try {
			onTimeoutHandler();
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::string sText = "Exception in timeout [" + name + "] : " + sExcept;
			std::cout << sText << std::endl;
		} catch (...)	{
			std::string sText = "Unknown exception in timeout [" + name + "]";
			std::cout << sText << std::endl;
		}
	}
}

bool TTimeout::isSignaled() const {
	std::lock_guard<std::mutex> lock(*valueMtx);
	return signaled;
}


bool TTimeout::isTriggered() {
	std::lock_guard<std::mutex> lock(*valueMtx);
	if (signaled) {
		signaled = false;
		return true;
	}
	return false;
}


void TTimeout::setSignaled(bool signaled) {
	bool exec = false;
	{
		std::lock_guard<std::mutex> lock(*valueMtx);
		this->signaled = signaled;
		if (signaled && started) {
			started = false;
			exec = true;
		}
	}
	if (exec) {
		onTimeout();
	}
}


TTimerDelay TTimeout::getPhase() const {
	std::lock_guard<std::mutex> lock(*valueMtx);
	return phase;
}

TTimerDelay TTimeout::getTimeout() const {
	std::lock_guard<std::mutex> lock(*valueMtx);
	return timeout;
};

void TTimeout::readConfig() {
	config->setSection("Timeouts");
	timeout = config->readInteger(name, timeout);
}


void TTimeout::writeConfig() {
	config->setSection("Timeouts");
	config->writeInteger(name, timeout);
}


void TTimeout::reWriteConfig() {
	readConfig();
	writeConfig();
}



/*
 * TTimeoutList
 */
TTimeoutController::TTimeoutController(TTimerDelay resolution, const std::string& configFolder) {
	now = 0;
	busy = false;
	terminated = false;
	this->resolution = resolution;
	this->configFolder = configFolder;
	this->configFile = this->configFolder + "timeouts.conf";
	config = new app::TIniFile(configFile);
	reWriteConfig();
	timer = new TTimeoutTimer(timerMtx, this->resolution, *this);
}


TTimeoutController::~TTimeoutController() {
	disable();
	
#ifndef STL_HAS_RANGE_FOR
	PTimeout o;
	size_t i,n;
	n = toList.size();
	for (i=0; i<n; i++) {
		o = toList[i];
		if (util::assigned(o))
			delete o;
	}
#else
	for (PTimeout o : toList) {
		if (util::assigned(o))
			delete o;
	}
#endif

	config->flush();
	delete config;
	delete timer;
}

void TTimeoutController::readConfig() {
	config->setSection("Global");
	resolution = config->readInteger("TimerResolution", resolution);
}


void TTimeoutController::writeConfig() {
	config->setSection("Global");
	config->writeInteger("TimerResolution", resolution);
}


void TTimeoutController::reWriteConfig() {
	readConfig();
	writeConfig();
}


size_t TTimeoutController::find(const std::string& name) {
	PTimeout o;
	size_t i,n;
	n = toList.size();
	for (i=0; i<n; i++) {
		o = toList[i];
		if (util::assigned(o)) {
			if (o->name == name)
				return i;
		}
	}
	return app::nsizet;
}


void TTimeoutController::disable() {
	if (!terminated) {
		timer->destroy();
	}	
}

void TTimeoutController::terminate() {
	if (!terminated) {
		std::lock_guard<std::mutex> lock(listMtx);
		disable();
	}	
}

void TTimeoutController::waitFor() {
	if (!terminated) {
		while (busy) {
			util::wait(50);
		};
		std::lock_guard<std::mutex> lock(listMtx);
		terminated = true;
	}	
}


void TTimeoutController::onTimer() {
	std::lock_guard<std::mutex> lock(listMtx);
	if (!terminated) {
		util::TBooleanGuard<bool> running(busy);
		now += resolution;
		running.set();

#ifndef STL_HAS_RANGE_FOR
		PTimeout o;
		size_t i,n;
		n = toList.size();
		for (i=0; i<n; i++) {
			o = toList[i];
			// Visible here via "friend"
			if (util::assigned(o)) {
				std::lock_guard<std::mutex> lock(*o->mtx);
				if ((o->phase != TTimer::ndelay) &&
					 !o->signaled && o->started) {
					if (o->phase <= now) {
						o->onTimeout();
					}
				}
			}
		}
#else
		for (PTimeout o : toList) {
			// Visible here via "friend"
			if (util::assigned(o)) {
				bool exec = false;
				{
					std::lock_guard<std::mutex> lock(*o->valueMtx);
					if ((o->phase != TTimer::ndelay) &&
						 !o->signaled && o->started) {
						if (o->phase <= now) {
							if (o->started) {
								o->started = false;
								o->signaled = true;
								exec = true;
							}
						}
					}
				}
				if (exec) {
					o->onTimeout();
				}
			}
		}
#endif
	}
}

PTimeout TTimeoutController::addTimeout(const std::string& name, const TTimerDelay timeout, const bool signaled) {
	// Mutex lock is needed, because timerCallback() works with this list!
	std::lock_guard<std::mutex> lock(listMtx);
	PTimeout o = nil;
	size_t idx = find(name);
	if (idx == nsizet) {
		std::lock_guard<std::mutex> lock(timeoutMtx);
		o = new TTimeout(name, timeout, timeoutMtx, *config, *this, signaled);
	} else {
		throw util::app_error_fmt("TTimeoutList::addTimeout() failed: Timeout <%> duplicated.", name);
	}

	if (util::assigned(o))
		toList.push_back(o);
	return o;
}


} /* namespace app */
