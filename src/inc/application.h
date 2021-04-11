/*
 * application.h
 *
 *  Created on: 16.08.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <string>
#include <iostream>
#include <functional>
#include <vector>
#include <mutex>
#include <signal.h>
#include <termios.h>
#include <sys/capability.h>
#include "capabilities.h"
#include "translation.h"
#include "stringutils.h"
#include "dataclasses.h"
#include "exception.h"
#include "semaphore.h"
#include "webserver.h"
#include "datatypes.h"
#include "filetypes.h"
#include "ipctypes.h"
#include "timeout.h"
#include "variant.h"
#include "inifile.h"
#include "classes.h"
#include "locale.h"
#include "detach.h"
#include "tasks.h"
#include "rs232.h"
#include "udev.h"
#include "ipc.h"
#ifdef USE_MQTT_CLIENT
#  include "mqtt.h"
#endif
#ifdef USE_GPIO_CONTROL
#  include "GPIO.h"
#endif
#ifdef USE_KEYLOK_DONGLE
#  include "keylok.h"
#endif


STATIC_CONST char* APPLICATION_DATA_FOLDER = "/usr/local/dbApps/";
STATIC_CONST char* GLOB_SEMA   = "6418F001-0E15-F66A-718F-C8600089F30B";
STATIC_CONST char* SGNL_FILE   = "/tmp/termination.dmp";
STATIC_CONST char* EXCP_FILE   = "/tmp/exception.dmp";
STATIC_CONST char* APP_CONFIG  = "Global";
STATIC_CONST char* APP_LICENSE = "License";
STATIC_CONST char* LICENCE_BASE_URL = "https://www.dbrinkmeier.de/licenses/";
STATIC_CONST char* SUPPLEMENTAL_GROUP_LIST = "gpio;dialout;audio;spi;i2c;kmem";
STATIC_CONST char* DEFAULT_CAPABILITIES_LIST = "cap_ipc_lock;cap_net_bind_service;cap_sys_admin;cap_sys_nice;cap_sys_rawio;cap_net_admin";

STATIC_CONST int SOFT_LOCK_LIMIT = 0xFFFF;
STATIC_CONST int MAX_DUMP_SIZE = 50;


namespace app {


class TApplication;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PApplication = TApplication*;

using TAppSignalHandler = __sighandler_t;
using TAppActionHandler = void (*) (int, siginfo_t *, void *);

using THotplugNotifyHandler = std::function<void(const app::THotplugEvent&, const app::EHotplugAction)>;
using TSerialDataHandler = std::function<void(const app::TSerial&, const util::TByteBuffer& data)>;

using TWatchNotifyHandler = std::function<void(const std::string&, bool&)>;
using TWatchNotifyList = std::vector<app::TWatchNotifyHandler>;

using TAppModuleList = std::vector<app::TModule*>;
using TAppModuleMap = std::map<std::string, app::TModule*>;

#else

typedef TApplication* PApplication;

typedef __sighandler_t TAppSignalHandler;
typedef void (*TAppActionHandler) (int, siginfo_t *, void *);

typedef std::function<void(const app::THotplugEvent&, const app::EHotplugAction)> THotplugNotifyHandler;
typedef std::function<void(const app::TSerial&, const util::TByteBuffer& data)> TSerialDataHandler;

typedef std::function<void(const std::string&, bool&)> TWatchNotifyHandler;
typedef std::vector<app::TWatchNotifyHandler> TWatchNotifyList;

typedef std::vector<app::TModule&> TAppModuleList;
typedef std::map<std::string, app::TModule*> TAppModuleMap;

#endif


enum ELogBase { LOG_APP, LOG_EXCEPT, LOG_THREAD, LOG_SOCKET, LOG_TIMER, LOG_TASKS, LOG_WEB, LOG_DATABASE };
enum ESystemState { SYS_READY, SYS_RELOAD, SYS_STOP };


class TBaseApplication : public TModule, protected TThreadAffinity, protected TThreadUtil, protected TCapabilities {
public:
	virtual void signalHandler(int signal) = 0;
	virtual int signalThreadHandler() = 0;
	virtual int watchThreadHandler() = 0;
	virtual int udevThreadHandler() = 0;
	virtual int commThreadHandler() = 0;

	TBaseApplication() : TThreadAffinity() {};
	virtual ~TBaseApplication() {};
};


class TApplication : public TBaseApplication {
private:
	uid_t uid;
	pid_t tid;
	pid_t pid;
	FILE* fsin;
	FILE* fsout;
	FILE* fserr;
	pthread_t signalThd;
	pthread_t watchThd;
	pthread_t udevThd;
	pthread_t commThd;
	mutable app::TMutex heapMtx;
	mutable app::TMutex pathMtx;
	mutable app::TMutex terminateMtx;
	mutable app::TMutex configMtx;
	mutable app::TMutex systemMtx;
	mutable app::TMutex namesMtx;
	app::TMutex stopMtx;
	app::TMutex haltedMtx;
	app::TMutex shutdownMtx;
	app::TMutex unpreparedMtx;
	app::TMutex storageMtx;
	app::TMutex updateMtx;
	util::TStringList workingFolders;
	util::TTimePart heapTime;
	app::PIniFile config;
	app::TSemaphore signal;
	app::TFileWatch watch;
	app::THotplugWatch hotplug;
    struct termios console;
	std::stringstream line;
	mutable std::string licenseKey;
	mutable std::string licenseURL;
	util::TVariantValues licenses;
	util::TVariantValues storage;
	std::string serial36;
	uint64_t serial10;
    time_t start;
	size_t assignedCPU;
	std::string isolatedCPU;
	int niceLevel;
	int error;

	// Local text store
	mutable std::string appDisplayName;
	mutable std::string appBanner;
	mutable std::string appDescription;
	mutable std::string appJumbotron;

#ifdef USE_KEYLOK_DONGLE
	app::TKeyLok keylok;
#endif

	TEventHandler onSigint;
	TEventHandler onSigterm;
	TEventHandler onSigkill;
	TEventHandler onSighup;
	TEventHandler onSigusr1;
	TEventHandler onSigusr2;
	TEventHandler onSigdefault;

	THotplugNotifyHandler onHotplug;
	TSerialDataHandler onTerminal;

    size_t executed;
	TAppModuleList modules;
	TAppModuleList prepared;
	TAppModuleMap names;
	size_t changes;

	TWatchNotifyList watches;
	util::TVariantValues args;

	ssize_t cntSigdef;
	ssize_t cntSigint;
	ssize_t cntSigterm;
	ssize_t cntSighup;
	ssize_t cntSigusr1;
	ssize_t cntSigusr2;

	bool daemonize;
	bool daemonized;
    bool licensed;
    bool halted;
    bool stopped;
    bool unprepared;
	bool terminated;
	bool terminating;
	bool rebooting;
	bool multiple;
	bool skipCheck;
	bool useMQTT;

	bool inputModeSet;
	bool webServing;
	bool webControl;

	bool signalThreadRunning;
	bool signalThreadStarted;
	bool watchThreadRunning;
	bool watchThreadStarted;
	bool udevThreadRunning;
	bool udevThreadStarted;
	bool commThreadRunning;
	bool commThreadStarted;
	bool eventsEnabled;

	void aquireConsole();
	void restoreConsole();

	void daemonizer(const std::string& runAsUser, const std::string& runAsGroup,
			const app::TStringVector& supplementalGroups, const app::TStringVector& capabilityList,
			std::string& groupNames, std::string& capabilityNames, bool& userChanged);
	void parseCommandLine(int argc, char *argv[]);
	sql::EDatabaseType parseDefaultDatabaseType(const std::string& description) const;
	const std::string& getSystemTimeZoneWithNolock() const;
	std::string getSystemTimeZoneNameWithNolock() const;

	void installSignalHandlers();
	void installSignalHandler(int signal, TAppSignalHandler handler, struct sigaction * action, sigset_t * mask);
	void installSignalAction(int signal, TAppActionHandler handler, struct sigaction * action, sigset_t * mask);
	void installSignalDispatcher(int signal, struct sigaction * action , sigset_t * mask);
	inline void execSignalHandler(const TEventHandler& handler, int signal);

	void terminateSignalThread();
	bool signalThreadMethod();
	void terminateWatchThread();
	bool watchThreadMethod();
	void terminateUdevThread();
	bool udevThreadMethod();
	void terminateCommThread();
	bool commThreadMethod();

	bool writePidFile();
	void deletePidFile();
	long int readPidFile();
	bool createPidFolder();
	bool createStoreFolder();
	bool createDataFolder();
	bool createApplicationFolder(const std::string folder);

	void writeDebugFile(const std::string& fileName, const std::string& text);
	void notifySystemState(const ESystemState state);
	void flushApplicationSettings();
	void flushSystemSettings();

	void writeStream(std::stringstream& sstrm);
	PWebServer startWebServer(const std::string& name, const std::string& documentRoot, app::TTranslator& nls, const bool autostart);

	void onAcceptSocket(const std::string& addr, bool& ok);

	// Overwrite "TBaseApplication::execute() = 0"
	int execute() { return EXIT_SUCCESS; };
	void unprepare();
	void release();
	void suicide();
	void stop();
	void halt();
	void inhibit();
	void updateLanguageText();
	void update(const EUpdateReason reason);

	void setTerminated();
	bool getAndSetTerminated();
	bool getAndSetStopped();
	bool getAndSetHalted();
	bool getAndSetUnprepared();

	bool backupConfigurationFilesWithNolock();

	void signalHandler(int signal);
	int signalThreadHandler();
	int watchThreadHandler();
	int udevThreadHandler();
	int commThreadHandler();

	template<typename signal_t, typename class_t>
		TEventHandler bindSignalHandler(signal_t &&onSignal, class_t &&owner) {
			TEventHandler method = std::bind(onSignal, owner);
	    	return method;
		}

public:
	const util::TVariantValues& arguments() const { return args; };

	// Application signal event handlers
	template<typename signal_t, typename class_t>
		void bindSigintHandler(signal_t &&onSignal, class_t &&owner) {
			onSigint = bindSignalHandler(onSignal, owner);
		}

	template<typename signal_t, typename class_t>
		void bindSigtermHandler(signal_t &&onSignal, class_t &&owner) {
			onSigterm = bindSignalHandler(onSignal, owner);
		}

	template<typename signal_t, typename class_t>
		void bindSigkillHandler(signal_t &&onSignal, class_t &&owner) {
			onSigkill = bindSignalHandler(onSignal, owner);
		}

	template<typename signal_t, typename class_t>
		void bindSighupHandler(signal_t &&onSignal, class_t &&owner) {
			onSighup = bindSignalHandler(onSignal, owner);
		}

	template<typename signal_t, typename class_t>
		void bindSigusr1Handler(signal_t &&onSignal, class_t &&owner) {
			onSigusr1 = bindSignalHandler(onSignal, owner);
		}

	template<typename signal_t, typename class_t>
		void bindSigusr2Handler(signal_t &&onSignal, class_t &&owner) {
			onSigusr2 = bindSignalHandler(onSignal, owner);
		}

	template<typename signal_t, typename class_t>
		void bindSigdefaultHandler(signal_t &&onSignal, class_t &&owner) {
			onSigdefault = bindSignalHandler(onSignal, owner);
		}

	// Socket template wrappers
	template<class class_t, typename data_t, typename owner_t>
		inline class_t* addSocket(const std::string& name, data_t &&onSocketData, owner_t &&owner) {
			if (useSockets()) {
				return getSockets().addSocket<class_t>(name, onSocketData, owner);
			}
			throw util::app_error("TApplication::addSocket() Not allowed, sockets disabled by configuration.");
		};

	template<class class_t, typename data_t, typename connect_t, typename close_t, typename owner_t>
		inline class_t* addSocket(const std::string& name, data_t &&onSocketData, connect_t &&onSocketConnect, close_t &&onSocketClose, owner_t &&owner) {
			if (useSockets()) {
				return getSockets().addSocket<class_t>(name, onSocketData, onSocketConnect, onSocketClose, owner);
			}
			throw util::app_error("TApplication::addSocket() Not allowed, sockets disabled by configuration.");
		};

	// Webserver template wrappers
	template<typename member_t, typename class_t>
		inline void addWebAction(const std::string& url, member_t &&webAction, class_t &&owner, const EWebActionMode mode = WAM_ASYNC) {
			if (hasWebServer()) {
				getWebServer().addWebAction(url, webAction, owner, mode);
				return;
			}
			throw util::app_error("addWebAction::addWebAction() Not allowed, webserver disabled by configuration.");
		}

	template<typename request_t, typename class_t>
		inline void addWebLink(const std::string& url, request_t &&onDataRequest, class_t &&owner, bool zipped = false, bool cached = false) {
			if (hasWebServer()) {
				getWebServer().addWebLink(url, onDataRequest, owner, zipped, cached);
				return;
			}
			throw util::app_error("TApplication::addWebLink() Not allowed, webserver disabled by configuration.");
		}

	template<typename request_t, typename class_t>
		inline void addStatisticsEventHandler(request_t &&onDataRequest, class_t &&owner) {
			if (hasWebServer()) {
				getWebServer().addStatisticsEventHandler(onDataRequest, owner);
				return;
			}
			throw util::app_error("TApplication::addStatisticsEventHandler() Not allowed, webserver disabled by configuration.");
		}

	template<typename request_t, typename class_t>
		inline void addWebData(const std::string& url, request_t &&onDataReceive, class_t &&owner, const EWebActionMode mode = WAM_ASYNC) {
			if (hasWebServer()) {
				getWebServer().addWebData(url, onDataReceive, owner, mode);
				return;
			}
			throw util::app_error("TApplication::addWebData() Not allowed, webserver disabled by configuration.");
		}

	PWebToken addWebToken(const std::string& key, const std::string& value) {
		if (hasWebServer()) {
			return getWebServer().addWebToken(key, value);
		}
		throw util::app_error("TApplication::addWebToken() Not allowed, webserver disabled by configuration.");
	}

	PWebToken addWebText(const std::string& key, const size_t textID, const std::string& text) {
		if (hasWebServer()) {
			if (hasTranslator()) {
				PWebToken token = getWebServer().getWebToken(key);
				if (!util::assigned(token)) {
					// Create new web token with translated text
					token = getWebServer().addWebToken(key, getTranslator().text(textID, text));
				} else {
					// Update token text for existing token
					token->setValue(getTranslator().text(textID, text), true);
				}
				return token;
			}
			throw util::app_error("TApplication::addWebText() No translator instance found.");
		}
		throw util::app_error("TApplication::addWebText() Not allowed, webserver disabled by configuration.");
	}

	template<typename member_t, typename class_t>
		inline void addWebPrepareHandler(member_t &&webPrepare, class_t &&owner) {
			if (hasWebServer()) {
				getWebServer().addWebPrepareHandler(webPrepare, owner);
				return;
			}
			throw util::app_error("TApplication::addWebPrepareHandler() Not allowed, webserver disabled by configuration.");
		}

	template<typename member_t, typename class_t>
		inline void addFileUploadEventHandler(member_t &&webUpload, class_t &&owner) {
			if (hasWebServer()) {
				getWebServer().addFileUploadEventHandler(webUpload, owner);
				return;
			}
			throw util::app_error("TApplication::addFileUploadEventHandler() Not allowed, webserver disabled by configuration.");
		}

	template<typename member_t, typename class_t>
		inline void addDefaultWebAction(member_t &&webAction, class_t &&owner, const EWebActionMode mode = WAM_ASYNC) {
			if (hasWebServer()) {
				getWebServer().addDefaultWebAction(webAction, owner, mode);
				return;
			}
			throw util::app_error("TApplication::addDefaultWebAction() Not allowed, webserver disabled by configuration.");
		}

	template<typename data_t, typename class_t>
		inline void addWebSocketDataHandler(data_t &&onSocketData, class_t &&owner) {
			if (hasWebServer()) {
				getWebServer().addWebSocketDataHandler(onSocketData, owner);
				return;
			}
			throw util::app_error("TApplication::addWebSocketDataHandler() Not allowed, webserver disabled by configuration.");
		}

	template<typename data_t, typename class_t>
		inline void addWebSocketVariantHandler(data_t &&onSocketData, class_t &&owner) {
			if (hasWebServer()) {
				getWebServer().addWebSocketVariantHandler(onSocketData, owner);
				return;
			}
			throw util::app_error("TApplication::addWebSocketVariantHandler() Not allowed, webserver disabled by configuration.");
		}

	// Wrapper templates to create and add managed threads derived from TManagedThread
	template<typename exec_t, typename owner_t>
		inline TManagedThread* addThread(const std::string& name, exec_t &&threadExecMethod,
				owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			if (hasThreads()) {
				return getThreads().addThread(name, threadExecMethod, owner, execute, cpu);
			}
			throw util::app_error("TApplication::addThread() Not allowed, threading disabled by configuration.");
		}

	template<typename exec_t, typename message_t, typename owner_t>
		inline TManagedThread* addThread(const std::string& name, exec_t &&threadExecMethod, message_t &&onThreadMessage,
				owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			if (hasThreads()) {
				return getThreads().addThread(name, threadExecMethod, onThreadMessage, owner, execute, cpu);
			}
			throw util::app_error("TApplication::addThread() Not allowed, threading disabled by configuration.");
		}

	// Wrapper templates to create and add managed any threads derived from data template
	template<class class_t, typename data_t, typename exec_t, typename owner_t>
		inline TWorkerThread<class_t>* addThread(const std::string& name, data_t &&data, exec_t &&threadExecMethod,
				owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			if (hasThreads()) {
				return getThreads().addThread<class_t>(name, data, threadExecMethod, owner, execute, cpu);
			}
			throw util::app_error("TApplication::addThread() Not allowed, threading disabled by configuration.");
		}

	template<class class_t, typename data_t, typename exec_t, typename message_t, typename owner_t>
		inline TWorkerThread<class_t>* addThread(const std::string& name, data_t &&data,
				exec_t &&threadExecMethod, message_t &&onThreadMessage,
				owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			if (hasThreads()) {
				return getThreads().addThread<class_t>(name, data, threadExecMethod, onThreadMessage, owner, execute, cpu);
			}
			throw util::app_error("TApplication::addThread() Not allowed, threading disabled by configuration.");
		}

	template<class class_t, typename data_t, typename exec_t, typename message_t, typename sync_t, typename owner_t>
		inline TWorkerThread<class_t>* addThread(const std::string& name, data_t &&data,
				exec_t &&threadExecMethod, message_t &&onThreadMessage, sync_t &&threadSyncMethod,
				owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			if (hasThreads()) {
				return getThreads().addThread<class_t>(name, data, threadExecMethod, onThreadMessage, threadSyncMethod, owner, execute, cpu);
			}
			throw util::app_error("TApplication::addThread() Not allowed, threading disabled by configuration.");
		}

	// Timer template wrapper
	template<typename member_t, typename class_t>
		inline PTimer addTimer(const std::string& section, const std::string& name, TTimerDelay delay, member_t &&onTimer, class_t &&owner) {
			if (hasTimers()) {
				return getTimers().addTimer(section, name, delay, onTimer, owner);
			}
			throw util::app_error("TApplication::addTimer() Not allowed, timers disabled by configuration.");
		}


	// Timeout template wrapper
	template<typename event_t, typename class_t>
		inline PTimeout addTimeout(const std::string& name, const TTimerDelay timeout, event_t &&onTimeout, class_t &&owner) {
			if (hasTimeouts()) {
				return getTimeouts().addTimeout(name, timeout, onTimeout, owner);
			}
			throw util::app_error("TApplication::addTimeout() Not allowed, timers disabled by configuration.");
		}

	PTimeout addTimeout(const std::string& name, const TTimerDelay timeout, const bool signaled = false) {
		if (hasTimeouts()) {
			return getTimeouts().addTimeout(name, timeout, signaled);
		}
		throw util::app_error("TApplication::addTimeout() Not allowed, timers disabled by configuration.");
	}

	// Task template wrappers
	template<typename task_t, typename class_t>
		inline void addTask(const std::string& name, TTimerDelay cycletime, task_t &&taskMethod, class_t &&owner) {
			if (hasTasks()) {
				getTasks().addTask(name, cycletime, taskMethod, owner);
				return;
			}
			throw util::app_error("TApplication::addTask() Not allowed, tasks disabled by configuration.");
		}

	void addTask(const std::string& name, TTimerDelay cycletime, PTask task) {
		if (hasTasks()) {
			getTasks().addTask(name, cycletime, task);
			return;
		}
		throw util::app_error("TApplication::addTask() Not allowed, tasks disabled by configuration.");
	}


	// File logging templates
	template<typename value_t, typename... variadic_t>
		void logger(const ELogBase file, const std::string& text, const value_t value, variadic_t... args);

	template<typename value_t, typename... variadic_t>
		void logger(const std::string& text, const value_t value, variadic_t... args);


	// File/folder watch event
	template<typename event_t, typename class_t>
		void addWatchHandler(event_t &&onWatch, class_t &&owner) {
			TWatchNotifyHandler method = std::bind(onWatch, owner, std::placeholders::_1, std::placeholders::_2);
			watches.push_back(method);
		}

	// Add File/folder to watch
	bool addFileWatch(const std::string& fileName, const int flags = DEFAULT_WATCH_EVENTS) {
		return watch.addFile(fileName, flags);
	}
	bool addFolderWatch(const std::string& path, const util::ESearchDepth depth = util::SD_ROOT, const int flags = DEFAULT_WATCH_EVENTS) {
		return watch.addFolder(path, depth, flags);
	}
	std::string getWatchErrorMessage() {
		return watch.getLastErrorMessage();
	}
	
	// Bind hotplug event handler
	template<typename event_t, typename class_t>
		void bindHotplugHandler(event_t &&onEvent, class_t &&owner) {
			onHotplug = std::bind(onEvent, owner, std::placeholders::_1, std::placeholders::_2);
		}

	// Bind serial data event handler
	template<typename event_t, typename class_t>
		void bindTerminalHandler(event_t &&onEvent, class_t &&owner) {
			onTerminal = std::bind(onEvent, owner, std::placeholders::_1, std::placeholders::_2);
		}
		
	// Add webserver authentication exclusions
	void addUrlAuthExclusion(const std::string& url) {
		if (hasWebServer()) {
			getWebServer().addUrlAuthExclusion(url);
			return;
		}
		throw util::app_error("TApplication::addUrlAuthExclusion() Not allowed, webserver disabled by configuration.");
	}
	void addRestAuthExclusion(const std::string& api) {
		if (hasWebServer()) {
			getWebServer().addRestAuthExclusion(api);
			return;
		}
		throw util::app_error("TApplication::addRestAuthExclusion() Not allowed, webserver disabled by configuration.");
	}

	template<typename value_t>
		void writeLocalStore(const std::string& key, const value_t& value) {
			app::TLockGuard<app::TMutex> lock(storageMtx);
			storage.add(key, value);
		}

	template<typename value_t>
		void writeLocalStore(const char* key, const value_t& value) {
			app::TLockGuard<app::TMutex> lock(storageMtx);
			storage.add(key, value);
		}

	void readLocalStore(const std::string& key, util::TVariant& value) {
		app::TLockGuard<app::TMutex> lock(storageMtx);
		size_t idx = storage.find(key);
		if (app::nsizet != idx) {
			value = storage.value(idx);
			return;
		}
		value.clear();
	}

	void readLocalStore(const char* key, util::TVariant& value) {
		app::TLockGuard<app::TMutex> lock(storageMtx);
		size_t idx = storage.find(key);
		if (app::nsizet != idx) {
			value = storage.value(idx);
			return;
		}
		value.clear();
	}

	void logger(const ELogBase file, const std::string& text);
	void clogger(const ELogBase file, const std::string &fmt, ...);
	void writeLog(const std::string& s, bool addLineFeed = true);
	void errorLog(const std::string& s, bool addLineFeed = true);
	void infoLog(const std::string& s, bool addLineFeed = true);
	void memoryLog(const std::string& location);

	inline TApplication& operator<< (std::ostream&(*func)(std::ostream&)){
		line << func;
		writeStream(line);
		return *this;
	}

    template<typename stream_t>
		inline TApplication& operator<< (const stream_t& stream) {
			line << stream;
			return *this;
		}

    template<typename stream_t>
		inline TApplication& operator<< (stream_t& stream) {
			line << stream;
			return *this;
		}

	int getVerbosity() const;
	util::TTimePart getLogFlushDelay() const;

	// Read only application properties
	const std::string& getCurrentFolder() const;
	const std::string& getLogFolder() const;
	const std::string& getTempFolder() const;
	const std::string& getConfigFolder() const;
	const std::string& getDataRootFolder() const;
	const std::string& getDataBaseFolder() const;
	const std::string& getImportFolder() const;
	const std::string& getPIDFolder() const;
	const std::string& getPIDFile() const;
	const std::string& getBackupFile() const;

	const std::string& getFileName() const;
	const std::string& getFileBaseName() const;
	const std::string& getDisplayName() const;
	const std::string& getUserName() const;
	const std::string& getVersion() const;
	const std::string& getBanner() const;
	const std::string& getLogo() const;

	// Read/Write application properties
	const std::string& getDescription() const;
	void setDescription(const std::string& description);
	const std::string& getJumbotron() const;
	void setJumbotron(const std::string& description);
	const std::string& getHostName() const;
	void setHostName(const std::string& hostName);

	// Various log file failities
	app::TLogFile& getExceptionLogger() const;
	app::TLogFile& getApplicationLogger() const;
	app::TLogFile& getDatabaseLogger() const;
	app::TLogFile& getTaskLogger() const;
	app::TLogFile& getTimerLogger() const;
	app::TLogFile& getThreadLogger() const;
	app::TLogFile& getSocketLogger() const;
	app::TLogFile& getWebLogger() const;

	// System database and seession management
    sql::TSession& getSystemSession() const;
    sql::TContainer& getSystemDatabase() const;
    sql::TContainer& getApplicationDatabase() const;
	app::TCredentials& getCredentials() const;

	// Get controller refernces
	app::TTimeoutController& getTimeouts() const;
	inet::TSocketController& getSockets() const;
	app::TThreadController& getThreads() const;
	app::TTimerController& getTimers() const;
	app::TTaskController& getTasks() const;
	app::TWebServer& getWebServer() const;
	app::TSerial& getTerminal() const;
	app::TTranslator& getTranslator() const;
#ifdef USE_GPIO_CONTROL
	app::TGPIOController& getGPIOController() const;
#endif
#ifdef USE_MQTT_CLIENT
    inet::TMQTT& getMQTTClient() const;
#endif

	// Get system objects
	app::PTimeoutController timeouts() const;
	inet::PSocketController sockets() const;
	app::PThreadController threads() const;
	app::PTimerController timers() const;
	app::PTaskController tasks() const;
	app::PWebServer webserver() const;
	app::PSerial terminal() const;
	app::PTranslator translator() const;
#ifdef USE_GPIO_CONTROL
	app::PGPIOController gpio() const;
#endif
#ifdef USE_MQTT_CLIENT
    inet::PMQTT mqtt() const;
#endif

    bool hasMQTTClient() const;
    bool hasSystemSession() const;
    bool hasSystemDatabase() const;
    bool hasApplicationDatabase() const;

    sql::TContainer* getSystemContainer() const;
    sql::TContainer* getApplicationContainer() const;

    void setWorkingFolder(const std::string path);
	void setWorkingFolders(const util::TStringList& folders);
	void addWorkingFolders(const util::TStringList& folders);
	bool getWorkingFolders(util::TStringList& folders) const;
	std::string getWorkingFolder() const;

    uid_t getUID() const { return uid; };
    pid_t getTID() const { return tid; };
	pid_t getPID() const { return pid; };
	int getNiceLevel() const { return niceLevel; };
	ssize_t getAssignedCPU() const { return assignedCPU; };
	const std::string& getIsolatedCPUs() const { return isolatedCPU; };
	util::TTimePart uptime() const { return util::now() - start; };

	char getch(bool echo = true);

	bool useGPIO() const;
	bool useSockets() const;
	bool useConsole() const { return inputModeSet; };
	bool useTranslator() const;

	bool hasWebServer() const;
	bool hasThreads() const;
	bool hasTimers() const;
	bool hasTasks() const;
	bool hasTimeouts() const;
	bool hasTerminal() const;
	bool hasTranslator() const;
	bool hasGPIO() const;

	bool isDaemonized() const { return daemonized; };
	bool isTerminated() const;
	void terminate() { setTerminated(); };

	bool isLicensed() const { return licensed; };
	bool isLicensed(const std::string& name) const;
	const std::string& getLicenseKey() const;
	const std::string& getLicenseBaseURL() const;
	size_t getLicenseNumber() const;
	size_t getSerialNumber() const { return serial10; };
	std::string getSerialKey() const { return serial36; };
	std::string getNamedLicense(const std::string& name) const;
	bool checkNamedLicense(const std::string& name, const std::string& value) const;
	bool applyNamedLicense(const std::string& name, const std::string& value);
	void updateNamedLicenses();

	void enableEvents() { eventsEnabled = true; };
	bool areEventsEnabled() const { return eventsEnabled && !terminated; };

	void initialize(int argc, char *argv[], TTranslator& nls);
	int execute(TModule& module);
	void finalize();
	void cleanup();
	void update();

	bool reboot();
	bool shutdown();

	int result() const { return error; };

	TModule* find(const std::string& name) const;
	TModule* operator [] (const std::string& name) const;
	bool hasModule(const std::string& name) const;

	util::TTimePart getDeallocateTime() const;
	util::TTimePart getHeapDelay() const;
	void deallocateHeapMemory();

	bool setSystemTime(const util::TTimePart seconds, const util::TTimePart millis = 0);
	bool setSystemLocale(const ELocale locale);
	ELocale getSystemLocale();

	bool setSystemTimeZone(const std::string zone);
	const std::string& getSystemTimeZone() const;
	std::string getSystemTimeZoneName() const;

	void commitApplicationSettings();
	bool backupConfigurationFiles();

	TApplication();
	virtual ~TApplication();
};

} /* namespace app */

#endif /* APPLICATION_H_ */
