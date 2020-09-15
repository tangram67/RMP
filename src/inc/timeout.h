/*
 * timeout.h
 *
 *  Created on: 07.09.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef TIMEOUT_H_
#define TIMEOUT_H_

#include "inifile.h"
#include "timer.h"
#include "gcc.h"


namespace app {

class TTimeout;
class TTimeoutController;

typedef TTimeout* PTimeout;
typedef TTimeoutController* PTimeoutController;


#ifdef STL_HAS_CONSTEXPR
static constexpr unsigned TO_TIME_RESOLUTION_MS = 72;
#else
#define TO_TIME_RESOLUTION_MS 72
#endif


class TTimeoutTimer final : public TSystemTimer {
private:
	TTimeoutController *owner;
	void onTimer();

public:
	TTimeoutTimer(std::mutex& mtx, int resolution, TTimeoutController& owner);
	virtual ~TTimeoutTimer() = default;
};


class TTimeout : public TObject {
friend class TTimeoutController;
private:
	bool signaled;
	bool started;
	bool changed;
	std::mutex* valueMtx;
	std::mutex execMtx;
	TTimeoutController* owner;
	TTimerDelay timeout;
	TTimerDelay phase;
	app::PIniFile config;

	TEventHandler onTimeoutHandler;
	void onTimeout();

	void readConfig();
	void writeConfig();
	void reWriteConfig();

public:
	bool isTriggered();
	bool isSignaled() const;
	void setSignaled(bool signaled);

	TTimerDelay getPhase() const;
	TTimerDelay getTimeout() const;
	void adjust(const TTimerDelay timeout);

	TTimerDelay restart(const TTimerDelay timeout = 0);
	void start();
	void stop();

	template<typename event_t, typename class_t>
		inline void bindEventHandler(event_t &&onTimeout, class_t &&owner) {
			onTimeoutHandler = std::bind(onTimeout, owner);
		}

	TTimeout(const std::string& name, TTimerDelay timeout, std::mutex& mtx, app::TIniFile& config, TTimeoutController& owner, const bool signaled = false);
	virtual ~TTimeout();
};


class TTimeoutController : TObject {
private:
	std::vector<app::PTimeout> toList;
	std::mutex listMtx;
	std::mutex timerMtx;
	std::mutex timeoutMtx;
	std::string configFolder;
	std::string configFile;
	app::PIniFile config;
	app::TTimeoutTimer *timer;
	TTimerDelay resolution;
	TTimerDelay now;
	bool terminated;
	bool busy;

	void readConfig();
	void writeConfig();
	void reWriteConfig();
	void disable();

public:
	TTimerDelay getTime() const { return now; }
	size_t find(const std::string& name);

	void terminate();
	void waitFor();
	void onTimer();

	PTimeout addTimeout(const std::string& name, const TTimerDelay timeout, const bool signaled = false);

	template<typename event_t, typename class_t>
		inline PTimeout addTimeout(const std::string& name, const TTimerDelay timeout, event_t &&onTimeout, class_t &&owner) {
			static_assert(std::is_reference<decltype(onTimeout)>::value, "TTimeoutList::addTimeout : Argument <onTimer> is not a reference.");
			static_assert(std::is_reference<decltype(owner)>::value, "TTimeoutList::addTimeout : Argument <owner> is not a reference.");
			PTimeout o = addTimeout(name, timeout, false);
			if (util::assigned(o))
				o->bindEventHandler(onTimeout, owner);
			return o;
		}

	TTimeoutController(TTimerDelay resolution, const std::string& configFolder);
	virtual ~TTimeoutController();
};


} /* namespace app */

#endif /* TIMEOUT_H_ */
