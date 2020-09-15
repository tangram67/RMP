/*
 * timer.h
 *
 *  Created on: 30.08.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <signal.h>
#include <time.h>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <thread>
#include <type_traits>
#include "classes.h"
#include "logger.h"
#include "inifile.h"

#ifdef CLOCK_REALTIME
#  define CLOCKIDR CLOCK_REALTIME
#else
#  define CLOCKIDR 0
#endif

#ifdef CLOCK_MONOTONIC
#  define CLOCKIDM CLOCK_MONOTONIC
#else
#  define CLOCKIDM 1
#endif


namespace app {


class TTimer;
class TTimerController;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PTimer = TTimer*;
using PTimerController = TTimerController*;
using TTimerList = std::vector<app::PTimer>;

using TTimerDelay = long int;

#else

typedef TTimer* PTimer;
typedef TTimerController* PTimerController;
typedef std::vector<app::PTimer> TTimerList;

typedef long int TTimerDelay;

#endif

typedef struct CTimerProperties {
	std::string name;
	std::string section;
	TTimerDelay delay;
	TEventHandler onTimer;
} TTimerProperties;


class TBaseTimer : public TObject {
protected:
	std::mutex* mtx;
	TEventList evList;
	TTimerDelay delay;

	void createTimer();
	void startTimer();
	void stopTimer();
	void destroyTimer();

	TBaseTimer(std::mutex& mtx);

private:
	pthread_attr_t* attr;
	timer_t id;

public:
	static const TTimerDelay ndelay = static_cast<TTimerDelay>(-1);

	virtual void onTimer();

	void addEvent(const app::TEventHandler& handler);

	int getOverrun() const;
	bool hasOverrun() const { return getOverrun() > 0; };

	virtual ~TBaseTimer();
};


class TSystemTimer : public TBaseTimer {
protected:
	TSystemTimer(const std::mutex& mtx);

private:
	bool started;

public:
	void stop();
	void start();
	void destroy();

	virtual void onTimer() override {};

	TSystemTimer(std::mutex& mtx, int resolution);
	virtual ~TSystemTimer();
};


class TTimer final : public TBaseTimer {
friend class TTimerController;

protected:
	TTimer(std::mutex& mtx);

private:
	std::mutex* localMtx;
	std::string section;
	bool enabled;
	bool running;
	bool terminate;
	bool cyclicViolation;
	app::PLogFile infoLog;
	app::PLogFile exceptionLog;
	app::PIniFile config;
	void onTimer() override;
	void readConfig();
	void writeConfig();
	void reWriteConfig();

	void writeInfo(const std::string text);
	void writeError(const std::string text);

public:
	TTimerDelay getDelay() const;
	void setDelay(TTimerDelay delay);

	bool isEnabled() const;
	void setEnabled(bool value);

	bool isRunning() const;
	void start() { setEnabled(true); };
	void stop() { setEnabled(false); };

	template<typename member_t, typename class_t>
		inline TEventHandler bindEventHandler(member_t &&onTimer, class_t &&owner) {
			TEventHandler method = std::bind(onTimer, owner);
	    	return method;
		}

	template<typename member_t, typename class_t>
		inline void addEventHandler(member_t &&onTimer, class_t &&owner) {
			static_assert(std::is_reference<decltype(onTimer)>::value, "TTimer::addEventHandler : Argument <onTimer> is not a reference.");
			static_assert(std::is_reference<decltype(owner)>::value, "TTimer::addEventHandler : Argument <owner> is not a reference.");
			TEventHandler handler = bindEventHandler(onTimer, owner);
			addEvent(handler);
		}

	TTimer();
	TTimer(const std::string& section, const std::string& name, TTimerDelay delay, const TEventHandler& onTimer,
			 std::mutex& mtx, app::TIniFile& config, TLogFile& infoLog, TLogFile& exceptionLog);
	virtual ~TTimer();
};


class TTimerController : public TObject {
private:
	TTimerList timerList;
	std::mutex timerMtx;
	std::mutex listMtx;
	std::string configFolder;
	std::string configFile;
	app::PLogFile infoLog;
	app::PLogFile exceptionLog;
	app::PIniFile config;
	bool terminated;

	size_t find(const std::string& name);
	void clear();
	void destroy();

public:
	PTimer addTimer(const TTimerProperties timer);
	PTimer addTimer(const std::string& section, const std::string& name, TTimerDelay delay, const TEventHandler& onTimer);

	template<typename member_t, typename class_t>
		inline PTimer addTimer(const std::string& section, const std::string& name, TTimerDelay delay, member_t &&onTimer, class_t &&owner) {
			static_assert(std::is_reference<decltype(onTimer)>::value, "TTimerList::addTimer : Argument <onTimer> is not a reference.");
			static_assert(std::is_reference<decltype(owner)>::value, "TTimerList::addTimer : Argument <owner> is not a reference.");
			TEventHandler method = std::bind(onTimer, owner);
			return addTimer(section, name, delay, method);
		}

	void terminate();
	void waitFor();

	TTimerController(const std::string& configFolder, TLogFile& infoLog, TLogFile& exceptionLog);
	virtual ~TTimerController();
};


} /* namespace app */

#endif /* TIMER_H_ */
