/*
 * logger.h
 *
 *  Created on: 17.08.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <list>
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include "templates.h"
#include "datetime.h"
#include "classes.h"
#include "inifile.h"
#include "detach.h"
#include "logtypes.h"
#include "semaphores.h"
#include "stringutils.h"
#include "stringtemplates.h"

#define LOG_MAX_FILE_SIZE 512000
#define LOG_MAX_BUFFER_SIZE 8

#define LOG_APPLICATION_NAME "application.log"
#define LOG_EXCEPTION_NAME   "exception.log"
#define LOG_TASKS_NAME       "tasks.log"
#define LOG_TIMER_NAME       "timer.log"
#define LOG_THREADS_NAME     "threads.log"
#define LOG_SOCKETS_NAME     "sockets.log"
#define LOG_WEBSERVER_NAME   "webserver.log"
#define LOG_DATABASE_NAME    "database.log"

namespace app {

class TLogFile : public TObject {
friend class TLogController;

private:
	std::string name;
	std::string logFile;
	std::string logFolder;
	std::string fileName;
	std::string baseName;
	std::list<std::string> lines;
	util::TStringList history;
	app::PIniFile config;
	app::TMutex* globalMtx;
	mutable app::TMutex localMtx;
	app::TMutex threadMtx;
	util::TDateTime time;
	app::TDetachedThread thread;
	bool folderExists;
	bool fileExists;
	bool debugOutput;
	bool localEnabled;
	bool globalEnabled;
	bool encodeHistory;
	bool useSyslog;
	int fileSize;
	int maxSize;
	int backupCount;
	std::size_t bufferSize;
	std::size_t histSize;
	std::ofstream stream;
	std::stringstream output;
	std::size_t cntRowsLogged;
	bool execute;

	void readConfig();
	void writeConfig();
	void reWriteConfig();

	void init();
	void open();
	void close();
	void writeFile();
	void writeLine(const std::string& text, bool addLineFeed);
	void writeHistory(const std::string& text);

	void backup();
	void append(std::string& line, bool addLineFeed);
	void halt() { execute = false; };

	void createUnlinkThread();
	void unlinkThreadMethod(TDetachedThread& thread);

public:
	const std::string& getName();
	const bool isEnabled() const { return (localEnabled && globalEnabled); };
	const unsigned long int getRowsLogged() const { return cntRowsLogged; }
	const util::TStringList& getHistory() const;
	void setHistoryDepth(const size_t rows);
	size_t getHistoryDepth() const  { return histSize; };
	void setHistoryEncoded(const bool value);
	bool getHistoryEncoded() const  { return encodeHistory; };
	void setTimeFormat(const util::EDateTimeFormat type);
	util::EDateTimeFormat getTimeFormat() const;

	template<typename value_t, typename... variadic_t>
		void write(const std::string& str, const value_t value, variadic_t... args) {
			write(util::csnprintf(str, value, std::forward<variadic_t>(args)...));
		}

	void write(const std::string& text, bool addLineFeed = true);
	void write(std::stringstream& sstrm, bool addLineFeed = false);
	void flush();
	int unlink();

    void resetios() {
    	output.copyfmt(std::ios(NULL));
    }

	inline TLogFile& operator<< (std::ostream&(*func)(std::ostream&)){
		output << func;
		write(output);
		return *this;
	}

    template<typename stream_t>
    inline TLogFile& operator<< (const stream_t& stream) {
    	output << stream;
        return *this;
    }

    template<typename stream_t>
    inline TLogFile& operator<< (stream_t& stream) {
    	output << stream;
        return *this;
    }

	TLogFile(const std::string& logFile, const std::string& name, bool enabled, bool debugOutput,
			   app::TIniFile& config, app::TMutex& mutex);
	virtual ~TLogFile();
};


class TLogController : public TObject {
private:
	TLogList logList;
	bool enabled;
	bool useISO8601;
	bool debugOutput;
	std::string logFolder;
	std::string configFolder;
	std::string configFile;
	app::TMutex listMtx;
	app::TMutex globalMtx;
	app::PIniFile config;

	void readConfig();
	void writeConfig();
	void reWriteConfig();
	std::size_t find(const std::string& name);
	void clear();
	bool isFileOpen(int fd);

public:
	std::size_t size();
	bool validIndex(std::size_t index);
	PLogFile at(std::size_t index);
	PLogFile addLogFile(const std::string& fileName);
	void flush();
	void unlink();
	void halt();

	TLogController(const std::string& name, const std::string& logFolder, const std::string& configFolder, bool debugOutput);
	virtual ~TLogController();
};

} /* namespace app */


template<typename log_t>
	inline void writeLog(const std::string& text, log_t&& logger) {
#ifdef STL_HAS_CONSTEXPR
		static_assert(!util::isClass<app::TLogFile>(logger), "Template writeLog() : Argument <logger> is not type of TLogFile.");
#endif
		if (logger->isEnabled())
			logger->write(text);
	}


#endif /* LOGGER_H_ */
