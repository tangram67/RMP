/*
 * tasks.cpp
 *
 *  Created on: 07.09.2014
 *      Author: Dirk Brinkmeier
 */

#include <unistd.h>
#include "tasks.h"
#include "timer.h"
#include "typeid.h"
#include "nullptr.h"
#include "templates.h"
#include "exception.h"

namespace app {


/*
 * TTaskTimer
 */
void TTaskTimer::onTimer() {
	if (util::assigned(owner)) {
		try {
			owner->onTimer();
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::string sText = "Exception in TTaskTimer::timerCallback() : " + sExcept;
			owner->getExceptionLog()->write(sText);
		} catch (...)	{
			std::string sText = "Unknown exception in TTaskTimer::timerCallback()";
			owner->getExceptionLog()->write(sText);
		}
	}
}



/*
 * TBaseTask
 */
TBaseTask::TBaseTask() {
	this->name = "";
	this->cycletime = TTimer::ndelay;
	this->owner = nil;
	this->config = nil;
	this->mtx = nil;
	bindTaskHandler(&app::TBaseTask::execute, this);
	running = false;
	terminate = false;
	duration = 0;
	threshold = 0;
	stop = 0;
	start = 0;
	state = 0;
	phase = 0;
}


void TBaseTask::initialize(const std::string& name, TTimerDelay cycletime, app::TIniFile& config,
		std::mutex& mtx, TTaskController& owner) {
	this->name = name;
	this->cycletime = cycletime;
	this->owner = &owner;
	this->config = &config;
	this->mtx = &mtx;
	reWriteConfig();
	this->threshold = this->cycletime * 9 / 10;
	phase = this->owner->time() + cycletime;
}


void TBaseTask::execute(TBaseTask& sender, TTaskState& state) {
	throw util::app_error("TBaseTask::execute() called, no custom method defined.");
}


TTaskState TBaseTask::getState() const {
	std::lock_guard<std::mutex> lock(*mtx);
	return state;
}

void TBaseTask::setState(TTaskState state) {
	std::lock_guard<std::mutex> lock(*mtx);
	this->state = state;
}

bool TBaseTask::isRunning() const {
	std::lock_guard<std::mutex> lock(*mtx);
	return running;
}

void TBaseTask::setRunning(bool running) {
	std::lock_guard<std::mutex> lock(*mtx);
	this->running = running;
}


void TBaseTask::readConfig() {
	config->setSection("CycleTime");
	cycletime = config->readInteger(name, cycletime);
}


void TBaseTask::writeConfig() {
	config->setSection("CycleTime");
	config->writeInteger(name, cycletime);
}


void TBaseTask::reWriteConfig() {
	readConfig();
	writeConfig();
}




/*
 * TTaskController
 */
TTaskController::TTaskController(TTimerDelay resolution, const std::string& configFolder, TLogFile& infoLog, TLogFile& exceptionLog) {
	now = 0;
	terminated = false;
	this->resolution = resolution;
	this->infoLog = &infoLog;
	this->exceptionLog = &exceptionLog;
	this->configFolder = configFolder;
	this->configFile = this->configFolder + "tasks.conf";
	config = new app::TIniFile(configFile);
	reWriteConfig();
	timer = new TTaskTimer(timerMtx, this->resolution, *this);
}


TTaskController::~TTaskController() {
	config->flush();
	disable();
	clear();
	delete timer;
	delete config;
}


void TTaskController::onTimer() {
	std::lock_guard<std::mutex> listlock(listMtx);
	now += resolution;

	TTaskState state;
	PBaseTask o;
	size_t i,n;
	n = taskList.size();

	for (i=0; i<n; i++) {
		o = taskList[i];
		if (util::assigned(o)) {
			std::unique_lock<std::mutex> lock(*o->mtx);
			if (!o->running && !o->terminate) {
				if (o->phase <= now) {
					util::TBooleanGuard<bool> b(o->running);
					o->phase = now + o->cycletime;
					o->start = now;
					b.set();
					try {
						lock.unlock();
						state = o->state;
						o->taskMethod(*o, o->state);
					} catch (const std::exception& e)	{
						std::string sExcept = e.what();
						std::string sText = "Exception in task [" + o->name + "] State=" + std::to_string((size_u)state) + " : " + sExcept;
						exceptionLog->write(sText);
						infoLog->write(sText);
					} catch (...)	{
						std::string sText = "Exception in task [" + o->name + "] State=" + std::to_string((size_u)state);
						exceptionLog->write(sText);
						infoLog->write(sText);
					}
					// onTimer() is called by timer thread again during execution of taskMethod()
					// --> if duration exceeds timer cycle time "now" is set to current "timestamp" prior to compare
					lock.lock();
					b.reset();
					o->stop = now;
					o->duration = o->stop - o->start;
					if (o->duration > o->threshold) {
						std::string sText = "Cycle time exceeded in task [" + o->name + "] State=" + std::to_string((size_u)state) + " Cycle time=" +
								std::to_string((size_u)o->cycletime) + ", Duration=" + std::to_string((size_u)o->duration);
						infoLog->write(sText);
					}
				}
			}
		}
	}
}


void TTaskController::readConfig() {
	config->setSection("Global");
	resolution = config->readInteger("TimerResolution", resolution);
}


void TTaskController::writeConfig() {
	config->setSection("Global");
	config->writeInteger("TimerResolution", resolution);
}


void TTaskController::reWriteConfig() {
	readConfig();
	writeConfig();
}


size_t TTaskController::find(const std::string& name) {
	PBaseTask o;
	size_t i,n;
	n = taskList.size();
	for (i=0; i<n; i++) {
		o = taskList[i];
		if (util::assigned(o)) {
			if (o->name == name)
				return i;
		}
	}
	return app::nsizet;
}


void TTaskController::addTask(const std::string& name, TTimerDelay cycletime, PTask task) {
	// Mutex lock is needed, because timer callback works with this list!
	std::lock_guard<std::mutex> lock(listMtx);

	size_t idx;
	idx = find(name);
	if (idx != nsizet) {
		throw util::app_error("TTAskList::addTask failed: Task <" + name + "> exists.");
	}

	// Set task parameters and add to list
	task->initialize(name, cycletime, *config, taskMtx, *this);
	taskList.push_back(task);

}


void TTaskController::terminate() {
	if (terminated)
		return;

	infoLog->write("TTaskList::terminate : Shutdown tasks.");
	std::lock_guard<std::mutex> lock(listMtx);
	disable();

	PBaseTask o;
	size_t i,n;
	n = taskList.size();

	// Set termination flag
	for (i=0; i<n; i++) {
		o = taskList[i];
		if (util::assigned(o)) {
			o->terminate = true;
		}
	}

}


void TTaskController::waitFor() {
	if (terminated)
		return;

	// Wait for all timers to be done
	infoLog->write("TTaskList::waitFor : Waiting for all tasks terminated...");

	bool finished;
	PBaseTask o;
	size_t i,n;

	n = taskList.size();
	do {
		finished = true;
		for (i=0; i<n; i++) {
			o = taskList[i];
			if (util::assigned(o)) {
				if (o->isRunning())
					finished = false;
			}
		}

		// Do some of nothing;
		if (!finished)
			util::saveWait(50);

	} while (!finished);

	terminated = true;
	infoLog->write("TTaskList::waitFor : All tasks terminated.");

}


void TTaskController::disable() {
	if (terminated)
		return;
	timer->destroy();
}


void TTaskController::clear() {
#ifndef STL_HAS_RANGE_FOR
	PBaseTask o;
	size_t i,n;
	n = taskList.size();
	for (i=0; i<n; i++) {
		o = taskList[i];
		if (util::assigned(o))
			delete o;
	}
#else
	for (PBaseTask o : taskList) {
		if (util::assigned(o))
			delete o;
	}
#endif

	taskList.clear();
}


} /* namespace app */
