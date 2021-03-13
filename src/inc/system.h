/*
 * system.h
 *
 *  Created on: 31.08.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "gcc.h"
#include "logger.h"
#include "tasks.h"
#include "timer.h"
#include "tasks.h"
#include "rs232.h"
#include "threads.h"
#include "nullptr.h"
#include "timeout.h"
#include "sockettypes.h"
#include "translation.h"
#include "webserver.h"
#include "datatypes.h"
#include "inifile.h"
#include "locale.h"
#include "version.h"
#include "credentials.h"
#include "../config.h"
#ifdef USE_GPIO_CONTROL
#  include "GPIO.h"
#endif
#ifdef USE_MQTT_CLIENT
#  include "mqtt.h"
#endif

namespace app {

class TApplication;

#ifdef STL_HAS_TEMPLATE_ALIAS
using PApplication = TApplication*;
#else
typedef TApplication* PApplication;
#endif

STATIC_CONST char LOG_BASE_FOLDER[] = "/var/log/dbApps/";
STATIC_CONST char DATA_BASE_FOLDER[] = "/usr/local/dbApps/";
STATIC_CONST char CONFIG_BASE_FOLDER[] = "/etc/dbApps/";

struct CApplicationConfig {
	std::string appVersion;
	std::string appFileName;
	std::string appBaseName;
	std::string appDisplayName;
	std::string appDescription;
	std::string appJumbotron;
	std::string appBanner;
	std::string appLogo;
	std::string hostName;
	std::string userName;
	std::string groupName;
	util::TStringList groupList;
	util::TStringList capsList;
	std::string currentFolder;
	std::string dataFolder;
	std::string configFolder;
	std::string configFile;
	std::string backupFile;
	std::string pidFile;
	std::string pidFolder;
	std::string storeFolder;
	std::string storeFile;
	std::string tmpFolder;
	std::string logFolder;
	std::string language;
	std::string defaultMQTTConnection;
	std::string defaultDatabaseName;
	std::string defaultDatabaseType;
	app::ELocale locale;
	std::string mimeFile;
	util::TTimePart heapDelay;
	bool useTerminalInput;
	bool useDefaultDatabase;
	bool useMulitLanguageSupport;
	bool startMQTTClient;
	bool startWebServer;
	bool setTempDir;
	bool runOnce;
	bool useDongle;
	int affinity;
	int handles;
	int nice;

	CApplicationConfig() {
		appVersion = std::string(SVN_BLD) + std::string(SVN_REV);
		appFileName.clear();
		appBaseName.clear();
		appDisplayName.clear();
		appDescription.clear();
		appJumbotron.clear();
		appBanner.clear();
		appLogo.clear();
		hostName.clear();
		userName.clear();
		groupName.clear();
		currentFolder.clear();
		configFolder.clear();
		configFile.clear();
		backupFile.clear();
		pidFile.clear();
		tmpFolder.clear();
		logFolder.clear();
		language = "de_DE.UTF-8";
		locale = app::ELocale::sysloc;
		mimeFile = "/etc/mime.types";
		dataFolder = DATA_BASE_FOLDER;
		runOnce = true;
		useDongle = true;
		setTempDir = false;
		startWebServer = false;
		startMQTTClient = false;
		useTerminalInput = false;
		useDefaultDatabase = false;
		useMulitLanguageSupport = false;
		defaultMQTTConnection = "application";
		defaultDatabaseName = "application";
		defaultDatabaseType = "PGSQL"; // MSSQL, SQLITE, PGSQL, ...
		affinity = -1;
		handles = 0;
		heapDelay = 0;
		nice = 0;
	}
};

struct CSerialConfig {
	bool useSerialPort;
	std::string serialDevice;
	TBaudRate baudRate;
	bool blocking;
	bool threaded;
	size_t chunk;

	CSerialConfig() {
		useSerialPort = false;
		serialDevice = "/dev/ttyS0";
		baudRate = 19200;
		blocking = false;
		threaded = false;
		chunk = 128;
	}
};

struct CSocketConfig {
	bool useSockets;

	CSocketConfig() {
		useSockets = false;
	}
};

struct CLogConfig {
	int verbosity;
	TTimerDelay cyclicLogFlush;

	CLogConfig() {
		verbosity = 0;
		cyclicLogFlush = 30000;
	}
};

struct CGPIOConfig {
	bool useGPIO;

	CGPIOConfig() {
		useGPIO = false;
	}
};

struct CSystemObjects {
	app::PApplication self;
	app::PLogFile exceptionLog;
	app::PLogFile applicationLog;
	app::PLogFile tasksLog;
	app::PLogFile timerLog;
	app::PLogFile threadLog;
	app::PLogFile socketLog;
	app::PLogFile webLog;
	app::PLogFile datbaseLog;
	app::PLogController logger;
	app::PTimerController timers;
	app::PTaskController tasks;
	app::PTimeoutController timeouts;
	app::PThreadController threads;
	inet::PSocketController sockets;
	app::PTranslator nls;
	app::PSerial serial;
	app::PWebServer webServer;
	app::PIniFile webConfig;
	app::PIniFile gpioConfig;
	app::PIniFile mqttConfig;
	sql::PSession session;
	sql::PContainer dbsys;
	sql::PContainer dbsql;
#ifdef USE_GPIO_CONTROL
	app::PGPIOController gpio;
#endif
#ifdef USE_MQTT_CLIENT
	inet::PMQTT mqtt;
#endif

	CSystemObjects() {
		self = nil;
		exceptionLog = nil;
		applicationLog = nil;
		tasksLog = nil;
		timerLog = nil;
		threadLog = nil;
		socketLog = nil;
		webLog = nil;
		datbaseLog = nil;
		logger = nil;
		timers = nil;
		tasks = nil;
		timeouts = nil;
		threads = nil;
		serial = nil;
		nls = nil;
		webServer = nil;
		webConfig = nil;
		gpioConfig = nil;
		mqttConfig = nil;
		session = nil;
		sockets = nil;
		dbsys = nil;
		dbsql = nil;
#ifdef USE_GPIO_CONTROL
		gpio = nil;
#endif
#ifdef USE_MQTT_CLIENT
		mqtt = nil;
#endif
	}
};


#ifdef STL_HAS_TEMPLATE_ALIAS

using TApplicationConfig = CApplicationConfig;
using TSerialConfig = CSerialConfig;
using TSocketConfig = CSocketConfig;
using TLogConfig = CLogConfig;
using TGPIOConfig = CGPIOConfig;
using TSystemObjects = CSystemObjects;

#else

typedef CApplicationConfig TApplicationConfig;
typedef CSerialConfig TSerialConfig;
typedef CSocketConfig TSocketConfig;
typedef CLogConfig TLogConfig;
typedef CGPIOConfig TGPIOConfig;
typedef CSystemObjects TSystemObjects;

#endif


struct CSystemData {
	TApplicationConfig app;
	TSerialConfig serial;
	TSocketConfig sockets;
	TLogConfig log;
	TSystemObjects obj;
	TGPIOConfig gpio;
	TCredentials users;
};

#ifdef STL_HAS_TEMPLATE_ALIAS

using TSystemData = CSystemData;

#else

typedef CSystemData TSystemData;

#endif

} /* namespace app */


#endif /* SYSTEM_H_ */
