/*
 * logger.cpp
 *
 *  Created on: 17.08.2014
 *      Author: Dirk Brinkmeier
 */

#include <sys/syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <algorithm>
#include <iterator>
#include "gcc.h"
#include "ansi.h"
#include "logger.h"
#include "exception.h"
#include "fileutils.h"
#include "htmlutils.h"
#include "../config.h"


namespace app {

/*
 * Class definition TLogFile
 *
 *  Created on: 17.08.2014
 *      Author: Dirk Brinkmeier
 */
TLogFile::TLogFile(const std::string& logFile, const std::string& name, bool enabled, bool debugOutput,
					   app::TIniFile& config, app::TMutex& mutex) {
	this->logFile = logFile;
	this->globalMtx = &mutex;
	this->globalEnabled = enabled;
	this->debugOutput = debugOutput;
	this->name = name;
	this->config = &config;
	init();
}


TLogFile::~TLogFile() {
	close();
}


void TLogFile::init() {
	time.setFormat(util::EDT_LONG);

	localEnabled = true;
	encodeHistory = true;
	maxSize = LOG_MAX_FILE_SIZE;
	bufferSize = LOG_MAX_BUFFER_SIZE;
	histSize = 0;
	useSyslog = false;
	cntRowsLogged = 0;
	backupCount = 5;
	execute = true;

	reWriteConfig();

	thread.setExecHandler(&app::TLogFile::unlinkThreadMethod, this);
	thread.setName("app::TLogFile::unlink()");
	thread.setErrorLog(*this);

	logFolder = util::filePath(logFile);
	fileName = util::fileExtName(logFile);
	baseName = util::fileBaseName(logFile);
	folderExists = util::fileExists(logFolder);

	if (folderExists)
		open();
}


void TLogFile::open() {
	stream.open(logFile, std::ios::out | std::ios::app );
	fileExists = stream.good();
	fileSize = stream.tellp();
}

void TLogFile::close() {
	stream.flush();
	stream.close();
	fileSize = 0;
}

void TLogFile::setHistoryDepth(const size_t rows) {
	if (histSize < rows)
		histSize = rows;
}

void TLogFile::setHistoryEncoded(const bool value) {
	encodeHistory = value;
}

void TLogFile::setTimeFormat(const util::EDateTimeFormat type) {
	if (time.getFormat() != type) {
		time.setFormat(type);
		time.setPrecision(type == util::EDateTimeFormat::EDT_ISO8601 ?
				util::EDateTimePrecision::ETP_MICRON : util::EDateTimePrecision::ETP_MILLISEC);
	}
}

util::EDateTimeFormat TLogFile::getTimeFormat() const {
	return time.getFormat();
}

void TLogFile::write(std::stringstream& sstrm, bool addLineFeed) {
	sstrm.seekg(0, std::ios::end);
	int size = sstrm.tellg();
	if (size > 0) {
		write(sstrm.str(), addLineFeed);
		sstrm.str("");
		resetios();
#ifdef USE_BOOLALPHA
		sstrm << std::boolalpha;
#endif
	}
}

void TLogFile::write(const std::string& text, bool addLineFeed) {
	if (globalEnabled || debugOutput) {

		app::TLockGuard<app::TMutex> lock(localMtx);
		bool parseLines = false;
		cntRowsLogged++;

		// Parse for lines if last char is \n
		if (text.size() > 1)
			parseLines = (text[util::pred(text.size())] == '\n');

		// Log each line separately
		if (parseLines) {
			util::TStringList list(text, '\n');
			util::TStringList::const_iterator it = list.begin();
			while (it != util::pred(list.end()))
				writeLine(*it++, true);
		} else {
			writeLine(text, addLineFeed);
		}
	}
}

void TLogFile::writeLine(const std::string& text, bool addLineFeed) {
	if (text.empty())
		return;

	// Write single line of text
	if (globalEnabled && useSyslog)
		syslog(LOG_NOTICE, "%s", text.c_str());

	if (localEnabled || debugOutput) {
		// Create complete line with time stamp
		time.sync();
		const std::string& t = time.asString();
		size_t size = t.size() + text.size() + 6; // 6 = " : " + "\n" + reserve
		std::string s;
		s.reserve(size);
		s = t + " : " + text;

		// Console output
		if (debugOutput) {
			app::TLockGuard<app::TMutex> lock(*globalMtx);
			std::cout << app::red << "[" << std::setw(11) << std::left << std::setfill(' ') << this->name << "] " << s << app::reset << std::endl;
		}

		if (localEnabled) {
			append(s, addLineFeed);
		}
	}
}


void TLogFile::writeHistory(const std::string& text) {
	if (histSize > 0) {
		history.push_front(text);
		if (history.size() > (histSize + (histSize / 5)))
			history.shrink(histSize);
		history.invalidate();
	}
}


void TLogFile::append(std::string& line, bool addLineFeed) {
	bool written = false;
	if (encodeHistory) {
		writeHistory(html::THTML::encode(line));
		written = true;
	}
	if (addLineFeed)
		line += "\n";
	if (folderExists && fileExists) {
		lines.push_back(line);
		if (lines.size() > bufferSize) {
			writeFile();
		}
	}
	if (!written) {
		writeHistory(line);
	}
}


const util::TStringList& TLogFile::getHistory() const {
	app::TLockGuard<app::TMutex> lock(localMtx);
	return history;
};


void TLogFile::writeFile() {
	if (!lines.empty()) {
		std::copy(lines.begin(), lines.end(), std::ostream_iterator<std::string>(stream));
		lines.clear();
		stream.flush();
		fileSize = stream.tellp();
		if (fileSize >= maxSize) {
			backup();
		}
	}
}

void TLogFile::backup() {
	close();
	std::string sBackup = util::uniqueFileName(logFolder + baseName + ".log", util::UN_TIME);
	util::deleteFile(sBackup);
	fileExists = false;
	if (util::moveFile(logFile, sBackup)) {
		open();
	}
	if (execute)
		thread.run();
}


int TLogFile::unlink() {
	app::TLockGuard<app::TMutex> lock(threadMtx);
	util::TFolderList content;
	return content.deleteOldest(logFolder + baseName + ".*.log", backupCount);
}


void TLogFile::unlinkThreadMethod(TDetachedThread& thread) {
	unlink();
}


void TLogFile::flush() {
	app::TLockGuard<app::TMutex> lock(localMtx);
	writeFile();
}


void TLogFile::readConfig() {
	config->setSection(name);
	localEnabled = config->readBool("LoggingEnabled", localEnabled);
	maxSize = config->readInteger("MaxFileSize", maxSize);
	bufferSize = config->readInteger("LinesInBuffer", bufferSize);
	histSize = config->readInteger("HistoryLineCount", histSize);
	backupCount = config->readInteger("BackupFileCount", backupCount);
	useSyslog = config->readBool("UseSyslog", useSyslog);
}


void TLogFile::writeConfig() {
	config->setSection(name);
	config->writeBool("LoggingEnabled", localEnabled, INI_BLYES);
	config->writeInteger("MaxFileSize", maxSize);
	config->writeInteger("LinesInBuffer", bufferSize);
	config->writeInteger("HistoryLineCount", histSize);
	config->writeInteger("BackupFileCount", backupCount);
	config->writeBool("UseSyslog", useSyslog, INI_BLYES);
}


void TLogFile::reWriteConfig() {
	readConfig();
	writeConfig();
}


const std::string& TLogFile::getName() {
	app::TLockGuard<app::TMutex> lock(localMtx);
	return name;
}




/*
 * Class definition TLogger
 *
 *  Created on: 17.08.2014
 *      Author: Dirk Brinkmeier
 */
TLogController::TLogController(const std::string& name, const std::string& logFolder, const std::string& configFolder, bool debugOutput) {
	this->enabled = true;
	this->useISO8601 = false;
	this->logFolder = logFolder;
	this->debugOutput = debugOutput;
	this->configFolder = configFolder;
	this->configFile = this->configFolder + "logger.conf";
	config = new app::TIniFile(configFile);
	openlog(name.c_str(), LOG_ODELAY | LOG_PID, LOG_LOCAL0);
	reWriteConfig();
}

TLogController::~TLogController() {
	closelog();
	clear();
	config->flush();
	delete config;
}


bool TLogController::isFileOpen(int fd) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = fcntl(fd, F_GETFD);
	} while (r == EXIT_ERROR && errno == EINTR);
	return (r != EXIT_ERROR || errno != EBADF);
}


PLogFile TLogController::addLogFile(const std::string& fileName) {
	app::TLockGuard<app::TMutex> lock(listMtx);

	std::string fullName;
	std::string baseName;
	app::PLogFile o;
	size_t idx;
	bool debug;

	fullName = logFolder + fileName;
	baseName = util::fileBaseName(fullName);

	idx = find(baseName);
	if (idx == nsizet) {
		debug = debugOutput;
		if (0 == fileName.compare(LOG_EXCEPTION_NAME) && isFileOpen(STDOUT_FILENO))
		  debug = true;
		o = new app::TLogFile(fullName, baseName, enabled, debug, *config, globalMtx);
	} else {
		throw util::app_error_fmt("TLogger::addLogFile() failed: Logfile <%> duplicated.", baseName);
	}

	if (util::assigned(o)) {
		if (useISO8601) o->setTimeFormat(util::EDT_ISO8601);
		logList.push_back(o);
	}

	return o;
}


void TLogController::flush() {
	app::TLockGuard<app::TMutex> lock(listMtx);
#ifndef STL_HAS_RANGE_FOR
	PLogFile o;
	size_t i,n;
	n = logList.size();
	for (i=0; i<n; i++) {
		o = logList[i];
		if (util::assigned(o))
			o->flush();
	}
#else
	for (PLogFile o : logList) {
		if (util::assigned(o))
			o->flush();
	}
#endif
}


void TLogController::unlink() {
#ifndef STL_HAS_RANGE_FOR
	PLogFile o;
	size_t i,n;
	n = logList.size();
	for (i=0; i<n; i++) {
		o = logList[i];
		if (util::assigned(o))
			o->unlink();
	}
#else
	for (PLogFile o : logList) {
		if (util::assigned(o))
			o->unlink();
	}
#endif
}


void TLogController::halt() {
	app::TLockGuard<app::TMutex> lock(listMtx);
#ifndef STL_HAS_RANGE_FOR
	PLogFile o;
	size_t i,n;
	n = logList.size();
	for (i=0; i<n; i++) {
		o = logList[i];
		if (util::assigned(o))
			o->halt();
	}
#else
	for (PLogFile o : logList) {
		if (util::assigned(o))
			o->halt();
	}
#endif
}


size_t TLogController::find(const std::string& name) {
	PLogFile o;
	size_t i,n;
	n = logList.size();
	for (i=0; i<n; i++) {
		o = logList[i];
		if (util::assigned(o)) {
			if (o->name == name)
				return i;
		}
	}
	return app::nsizet;
}


bool TLogController::validIndex(std::size_t index) {
	return  ( (index >= 0) && (index < logList.size()) );
}

size_t TLogController::size() {
	return logList.size();
}


PLogFile TLogController::at(std::size_t index) {
	if (validIndex(index))
		return logList[index];
	return nil;
}


void TLogController::clear() {
	app::TLockGuard<app::TMutex> lock(listMtx);
#ifndef STL_HAS_RANGE_FOR
	PLogFile o;
	size_t i,n;
	n = logList.size();
	for (i=0; i<n; i++) {
		o = logList[i];
		if (util::assigned(o))
			util::freeAndNil(o);
	}
#else
	for (PLogFile o : logList) {
		if (util::assigned(o)) {
			util::freeAndNil(o);
		}
	}
#endif
	logList.clear();
}


void TLogController::readConfig() {
	config->setSection("Global");
	enabled = config->readBool("LoggingEnabled", enabled);
	useISO8601 = config->readBool("UseISO8601", useISO8601);
}


void TLogController::writeConfig() {
	config->setSection("Global");
	config->writeBool("LoggingEnabled", enabled, INI_BLYES);
	config->writeBool("UseISO8601", useISO8601, INI_BLYES);
}


void TLogController::reWriteConfig() {
	readConfig();
	writeConfig();
}


} /* namespace app */
