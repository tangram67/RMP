/*
 * tasks.h
 *
 *  Created on: 07.09.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef TASKS_H_
#define TASKS_H_

#include "inifile.h"
#include "timer.h"
#include "logger.h"
#include "gcc.h"

namespace app {


STATIC_CONST unsigned TA_TIME_RESOLUTION_MS = 83;

class TTaskController;
class TBaseTask;
class TTask;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PTaskController = TTaskController*;
using PBaseTask = TBaseTask*;
using PTask = TTask*;
using TTaskList = std::vector<app::PBaseTask>;

using TTaskState = unsigned int;
using TTaskMethod = std::function<void(TBaseTask&, TTaskState&)>;

#else

typedef TTaskController* PTaskController;
typedef TBaseTask* PBaseTask;
typedef TTask* PTask;
typedef std::vector<app::PBaseTask> TTaskList;

typedef unsigned int TTaskState;
typedef std::function<int(TBaseTask&, TTaskState&)> TTaskMethod;

#endif


class TTaskTimer final : public TSystemTimer {
private:
	TTaskController *owner;
	void onTimer();

public:
	TTaskTimer(std::mutex& mtx, int resolution, TTaskController& owner) : TSystemTimer(mtx, resolution), owner {&owner} {};
	virtual ~TTaskTimer() = default;
};

typedef TTaskTimer* PTaskTimer;


class TBaseTask : public TObject {
friend class TTaskController;

private:
	PTaskController owner;
	bool running;
	bool terminate;
	app::TTaskState state;
	app::PIniFile config;
	std::mutex* mtx;
	TTimerDelay start;
	TTimerDelay stop;
	TTimerDelay duration;
	TTimerDelay threshold;
	TTaskMethod taskMethod;
	void readConfig();
	void writeConfig();
	void reWriteConfig();

protected:
	TTimerDelay cycletime;
	TTimerDelay phase;

	explicit TBaseTask();

public:
	TTaskState getState() const;
	void setState(TTaskState state);
	bool isRunning() const;
	void setRunning(bool running);

	virtual void execute(TBaseTask& sender, TTaskState& state);
	void initialize(const std::string& name, TTimerDelay cycletime, app::TIniFile& config, std::mutex& mtx, TTaskController& owner);

	template<typename task_t, typename class_t>
		inline void bindTaskHandler(task_t &&taskMethod, class_t &&owner) {
			this->taskMethod = std::bind(taskMethod, owner, std::placeholders::_1, std::placeholders::_2);
		}

	virtual ~TBaseTask() = default;
};


class TTask : public TBaseTask {
public:
	virtual ~TTask() = default;
};


class TTaskController : TObject {
private:
	TTaskList taskList;
	std::mutex listMtx;
	std::mutex taskMtx;
	std::mutex timerMtx;
	app::PTaskTimer timer;
	app::PLogFile infoLog;
	app::PLogFile exceptionLog;
	TTimerDelay now;
	TTimerDelay resolution;
	std::string configFolder;
	std::string configFile;
	app::PIniFile config;
	bool terminated;
	void readConfig();
	void writeConfig();
	void reWriteConfig();
	void disable();
	void clear();

public:
	TTimerDelay time() const { return now; }

	const app::PLogFile getInfoLog() const { return infoLog; };
	const app::PLogFile getExceptionLog() const { return exceptionLog; };
	size_t find(const std::string& name);

	void addTask(const std::string& name, TTimerDelay cycletime, PTask task);

	template<typename task_t, typename class_t>
		inline void addTask(const std::string& name, TTimerDelay cycletime, task_t &&taskMethod, class_t &&owner) {
			PTask task = new TTask;
			task->bindTaskHandler(taskMethod, owner);
			addTask(name, cycletime, task);
		}

	void onTimer();
	void terminate();
	void waitFor();

	TTaskController(TTimerDelay resolution, const std::string& configFolder, TLogFile& infoLog, TLogFile& exceptionLog);
	virtual ~TTaskController();
};


} /* namespace app */

#endif /* TASKS_H_ */
