/*
 * detach.cpp
 *
 *  Created on: 18.07.2015
 *      Author: Dirk Brinkmeier
 */

#include "detach.h"
#include "logger.h"
#include "functors.h"
#include "threads.h"

namespace app {

static std::mutex detachedTreadMtx;
static size_t detachedTreadCount = 0;

/*
 * TIntermittentThread
 */
TIntermittentThread::TIntermittentThread() {
	init();
}

TIntermittentThread::TIntermittentThread(const std::string& name) {
	init();
	this->name = name;
}

TIntermittentThread::~TIntermittentThread() {
}

void TIntermittentThread::init() {
	logger = nil;
}

void TIntermittentThread::createDetachedThread(TThreadHandler method) {
	pthread_t thd = 0;
	const char* desc = !name.empty() ? name.c_str() : nil;
	TThreadUtil::createThread(thd, method, THD_CREATE_DETACHED, this, desc);
}

void TIntermittentThread::write(const std::string& text) {
	if (util::assigned(logger)) {
		if (name.empty())
			logger->write(text);
		else
			logger->write(name + " " + text);
	}
}

void TIntermittentThread::writeException(const std::exception& e, const std::string& info) {
	std::string sExcept = e.what();
	std::string sName = info;
	if (!getName().empty())
		sName = sName + " of [" + getName() + "]";
	std::string sText = "[EXCEPTION] Exception in " + sName + " :\n" + sExcept + "\n";
	write(sText);
}

void TIntermittentThread::writeException(const std::string& info) {
	std::string sName = info;
	if (!getName().empty())
		sName = sName + " of [" + getName() + "]";
	std::string sText = "[EXCEPTION] Unknown exception in " + sName;
	write(sText);
}

size_t TIntermittentThread::getThreadCount() {
	std::lock_guard<std::mutex> lock(detachedTreadMtx);
	return detachedTreadCount;
}

void TIntermittentThread::decThreadCount() {
	std::lock_guard<std::mutex> lock(detachedTreadMtx);
	if (detachedTreadCount > 0)
		--detachedTreadCount;
}

void TIntermittentThread::incThreadCount() {
	std::lock_guard<std::mutex> lock(detachedTreadMtx);
	++detachedTreadCount;
}

void TIntermittentThread::waitFor() {
	while (TIntermittentThread::hasThreads())
		util::wait(250);
}



/*
 * TDetachedThread
 */
TDetachedThread::TDetachedThread() {
	init();
}

TDetachedThread::TDetachedThread(const std::string& name) {
	init();
	this->name = name;
}

TDetachedThread::~TDetachedThread() {
}

void TDetachedThread::init() {
	threadExecMethod = nil;
}

void TDetachedThread::execute() {
	if (threadExecMethod != nil)
		threadExecMethod(*this);
}

void TDetachedThread::run() {
	if (threadExecMethod != nil) {
		createDetachedThread(detachedThreadExecuter);
	} else {
		if (name.empty())
			util::app_error("TDetachedThread::run() failed: No thread method assigned.");
		else
			util::app_error("TDetachedThread::run() failed for \"" + name + "\" : No thread method assigned.");
	}
}

// Static thread function for executing tread routine
void* TDetachedThread::detachedThreadExecuter(void* cls) {
	app::PDetachedThread thread = static_cast<app::PDetachedThread>(cls);
	int retVal = EXIT_FAILURE;
	if (util::assigned(thread)) {
		try {
			thread->execute();
			retVal = EXIT_SUCCESS;
		} catch (const std::exception& e)	{
			thread->writeException(e, "TDetachedThread::detachedThreadExecuter()");
		}
	}
	return (void *)(long)(retVal);
}

} /* namespace app */
