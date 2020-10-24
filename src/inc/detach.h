/*
 * detach.h
 *
 *  Created on: 18.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef DETACH_H_
#define DETACH_H_

#include <limits.h>
#include <pthread.h>
#include <functional>
#include <algorithm>
#include <string>
#include <mutex>
#include "gcc.h"
#include "ansi.h"
#include "classes.h"
#include "logtypes.h"
#include "templates.h"
#include "semaphore.h"
#include "exception.h"

namespace app {

class TDetachedThread;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PDetachedThread = TDetachedThread*;
using TThreadDetachedMethod = std::function<void(TDetachedThread&)>;

#else

typedef TDetachedThread* PDetachedThread;
typedef std::function<void(TDetachedThread&)> TThreadDetachedMethod;

#endif


class TIntermittentThread : public TObject {
private:
	void init();

protected:
	PLogFile logger;
	void createDetachedThread(TThreadHandler method);
	static void decThreadCount();
	static void incThreadCount();

public:
	static size_t getThreadCount();
	static bool hasThreads() { return getThreadCount() > 0; };
	static void waitFor();

	void writeException(const std::exception& e, const std::string& info);
	void writeException(const std::string& info);

	void setErrorLog(TLogFile& logfile) { logger = &logfile; };
	void write(const std::string& text);

	TIntermittentThread();
	TIntermittentThread(const std::string& name);
	virtual ~TIntermittentThread();
};


class TDetachedThread : public TIntermittentThread {
private:
	TThreadDetachedMethod threadExecMethod;

	void init();
	void execute();
	static void* detachedThreadExecuter(void* cls);

public:
	template<typename exec_t, typename class_t>
		inline void setExecHandler(exec_t &&threadExecMethod, class_t &&owner) {
			this->threadExecMethod = std::bind(threadExecMethod, owner, std::placeholders::_1);
		}

	void run();

	TDetachedThread();
	TDetachedThread(const std::string& name);
	virtual ~TDetachedThread();
};

} /* namespace app */

// Include template implementation of TDataThread
#include "detach.tpp"

#endif /* DETACH_H_ */
