/*
 * application.cpp
 *
 *  Created on: 16.08.2014
 *      Author: Dirk Brinkmeier
 */
#include <fstream>
#include <vector>
#include <functional>
#include <pthread.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <grp.h>
#include <unistd.h>
#include <execinfo.h>
#include <ucontext.h>
#include <malloc.h>
#include "../config.h"
#include "application.h"
#include "database.h"
#include "stringutils.h"
#include "htmlutils.h"
#include "templates.h"
#include "mimetypes.h"
#include "encoding.h"
#include "sysutils.h"
#include "process.h"
#include "convert.h"
#include "compare.h"
#include "globals.h"
#include "typeid.h"
#include "system.h"
#include "memory.h"
#include "sockets.h"
#include "ASCII.h"
#include "gzip.h"
#include "gcc.h"
#include "version.h"
#ifdef USE_DEBUG
#  include "debug/addr2line.h"
#endif
#ifdef USE_SYSTEMD_NOTIFY
#  include <systemd/sd-daemon.h>
#endif

using namespace util;

static app::PApplication FApplication = nil;


static void signalDispatcher(int signal) {
	application.signalHandler(signal);
}

static void* signalThreadDispatcher(void *thread) {
	if (util::assigned(thread)) {
		return (void *)(long)(static_cast<app::TApplication*>(thread))->signalThreadHandler();
	} else {
		return (void *)(long)(EXIT_FAILURE);
	}
}

static void* watchThreadDispatcher(void *thread) {
	if (util::assigned(thread)) {
		return (void *)(long)(static_cast<app::TApplication*>(thread))->watchThreadHandler();
	} else {
		return (void *)(long)(EXIT_FAILURE);
	}
}

static void* udevThreadDispatcher(void *thread) {
	if (util::assigned(thread)) {
		return (void *)(long)(static_cast<app::TApplication*>(thread))->udevThreadHandler();
	} else {
		return (void *)(long)(EXIT_FAILURE);
	}
}

static void* commThreadDispatcher(void *thread) {
	if (util::assigned(thread)) {
		return (void *)(long)(static_cast<app::TApplication*>(thread))->commThreadHandler();
	} else {
		return (void *)(long)(EXIT_FAILURE);
	}
}


static void sigChildHandler(int signal) {}
static void sigPipeHandler(int signal) {}
static void sigAlarmHandler(int signal) {}


#ifdef USE_DEBUG
static void logBackTrace(FILE* file, const char * timestamp, void * address) {
	// Check for valid filename
	if (sysdat.app.appFileName.empty())
		return;

	static bool once = false;
	int size, i, res = EXIT_FAILURE;
	int index = 1;
	int offset = 0;
	void * array[MAX_DUMP_SIZE];
	char ** messages;
	char const * target;
	char func[PATH_MAX+1] = {0};
	char unit[PATH_MAX+1] = {0};

	// Prevent double call of demangling functions
	// ATTENTION: Flag is NOT thread save, but mutexes are not allowed in signal handlers!
	bool demangled;
	bool trace = true;
	if (once)
		trace = false;
	once = true;

	// Get the target type
#if defined(TARGET_X32)
	target = "i386-pc-linux-gnu";
#elif defined(TARGET_X64)
	target = "i686-pc-linux-gnu";
#else
#  error "Unsupported architecture in logBackTrace()"
#endif

	// Initialize gcc demangle library
	if (trace) {
		res = libtrace_init(sysdat.app.appFileName.c_str(), NULL, target);
	}

	// Get array of backtraced caller addresses
	size = backtrace(array, MAX_DUMP_SIZE);
	if (address != NULL) {
		array[index] = address;
	} else {
		index = 2;
		offset = 1;
	}

	// Get callstack addresses
	messages = backtrace_symbols(array, size);

	// Skip first stack frame (points here)
	for (i = index; i < size && messages != NULL; ++i)
	{
		int num = i - offset;
		demangled = false;
		if (EXIT_SUCCESS == res) {
			if (EXIT_SUCCESS == libtrace_demangle(array[i], func, PATH_MAX, unit, PATH_MAX))
				demangled = true;
		}
		if (demangled) {
			fprintf(stderr, "[%s] (%d) %s in [%s] called by [%s]\n", timestamp, num, messages[i], unit, func);
			if (file) {
				fprintf(file, "[%s] (%d) %s in [%s] called by [%s]\n", timestamp, num, messages[i], unit, func);
			}
		} else {
			fprintf(stderr, "[%s] (%d) %s\n", timestamp, num, messages[i]);
			if (file) {
				fprintf(file, "[%s] (%d) %s\n", timestamp, num, messages[i]);
			}
		}
	}
	if (file) {
		fwrite("\n", 1, 1, file);
	}
	if (EXIT_SUCCESS == res) {
		libtrace_close();
	}
	free(messages);
	once = false;
}
#endif


// This structure mirrors the one found in /usr/include/asm/ucontext
typedef struct _sig_ucontext {
	unsigned long uc_flags;
	struct ucontext * uc_link;
	stack_t uc_stack;
	struct sigcontext uc_mcontext;
	sigset_t uc_sigmask;
} sig_ucontext_t;


// Catch exception signals like SIGSEGV/SIGABRT and write "mini dump"
// --> Recommended gcc options: -rdynamic -O0
static void sigExceptionHandler(int signal, siginfo_t * info, void * ucontext) {
#ifdef USE_DEBUG
	void * address;
	const char* version = SVN_REV;
	util::TDateTime date;
	char const * timestamp = date.asString().c_str();
	sig_ucontext_t * uc = (sig_ucontext_t *)ucontext;
	FILE* file = fopen(SGNL_FILE, "a");

	// Get the address at the time the signal was raised
#if defined(TARGET_X32)
	address = (void *) uc->uc_mcontext.eip; // EIP: x86 specific
#elif defined(TARGET_X64)
	address = (void *) uc->uc_mcontext.rip; // RIP: x86_64 specific
#else
#  error "Unsupported architecture in SIGSEGV handler."
#endif

	fprintf(stderr, "[%s] [SVN%s] Signal %d (%s), address is %p from %p\n",
			timestamp, version, signal, strsignal(signal), info->si_addr, (void *)address);
	if (file) {
		fprintf(file, "[%s] [SVN%s] Signal %d (%s), address is %p from %p\n",
				timestamp, version, signal, strsignal(signal), info->si_addr, (void *)address);
	}

	logBackTrace(file, timestamp, address);

	if (file) {
		fclose(file);
	}
#endif
	exit(110);
}


static void appExitHandler() {
	if (util::assigned(FApplication))
		application.cleanup();
}

static void appTerminateHandler() {
	std::string sText, sExcept;
	const char* version = SVN_REV;
	util::TDateTime date;
	char const * timestamp = date.asString().c_str();
	char const * text = "Terminated due to unknown reason.";
	FILE* file = fopen(EXCP_FILE, "a");

	std::exception_ptr except = std::current_exception();
	if (util::assigned(except)) {
		// std::exception_ptr can be thrown again
		try {
			std::rethrow_exception(except);
		} catch (const std::exception& e)	{
			sExcept = e.what();
			sText = "Terminated by unhandled exception \"" + sExcept + "\"";
			text = sText.c_str();
		} catch (...) {
			text = "Terminated due to unknown exception.";
		}
	}

	fprintf(stderr, "[%s] [SVN%s] %s\n", timestamp, version, text);
	if (file) {
		fprintf(file, "[%s] [SVN%s] %s\n", timestamp, version, text);
	}

#ifdef USE_DEBUG
	logBackTrace(file, timestamp, nil);
#endif

	if (file) {
		fclose(file);
	}
	exit(111);
}


namespace app {

class TCyclicLogger : public TTask {
private:
	PTimeout toFlush;
	PLogFile log;
	size_t size;
	size_t index;

public:
	void execute(TBaseTask& sender, TTaskState& state) {
		switch (state) {
			case 0:
				if (toFlush->isSignaled()) {
					state = 10;
					size = sysdat.obj.logger->size();
					index = 0;
				}
				break;

			case 10:
				log = sysdat.obj.logger->at(index);
				if (util::assigned(log)) {
					log->flush();
					index++;
				} else {
					state = 20;
					toFlush->start();
				}
				break;

			case 20:
				if (application.getHeapDelay() > 0) {
					if (util::now() > (application.getDeallocateTime() + application.getHeapDelay())) {
						application.deallocateHeapMemory();
					}
				}
				state = 0;
				break;
		}
	}

	TCyclicLogger() {
		toFlush = application.addTimeout("CyclicFlushDelay", application.getLogFlushDelay());
		toFlush->start();
		index = 0;
		size = 0;
		log = nil;
	}
};

#ifdef USE_MQTT_CLIENT
class TMQTTConnector : public TTask {
private:
	util::TTimePart retry;
	util::TTimePart delay;
	bool initialzed;

public:
	void execute(TBaseTask& sender, TTaskState& state) {
		if (application.hasMQTTClient() && !application.isTerminated()) {
			switch (state) {
				case 0:
					// Alsways do firs conection try here!
					// --> Do reconnect only when client library reconnect feature is disabled by configuration
					if (!application.getMQTTClient().isConnected() && (!application.getMQTTClient().doReconnect() || !application.getMQTTClient().wasConnected())) {
						try {
							application.getApplicationLogger().write("[MQTT] [Connector Task] Connect to message broker...");
							application.getMQTTClient().start();
							retry = util::now() + delay;
							state = 10;
						} catch (const std::exception& e)	{
							std::string sExcept = e.what();
							std::string sText = "[MQTT] [Connector Task] Exception in connect task : " + sExcept;
							application.getApplicationLogger().write(sText);
						} catch (...)	{
							std::string sText = "[MQTT] [Connector Task] Unknown exception in connection task.";
							application.getApplicationLogger().write(sText);
						}
					}
					break;

				case 10:
					// Wait for client connected
					if (application.getMQTTClient().isConnected()) {
						application.getApplicationLogger().write("[MQTT] [Connector Task] Message broker connection established.");
						delay = 1;
						state = 20;
					} else {
						// Retry connect after increasing delay time
						if (retry < now()) {
							application.getApplicationLogger().write("[MQTT] [Connector Task] Retry connecting message broker.");
							++delay;
							if (delay > 30)
								delay = 30;
							state = 0;
						}
					}
					break;

				case 20:
					if (!application.getMQTTClient().isConnected()) {
						// Wait for client disconnected
						application.getApplicationLogger().write("[MQTT] [Connector Task] Message broker connection lost.");
						state = 0;
					} else {
						// Initialize MQTT client when connection is online
						if (!initialzed && application.areEventsEnabled()) {
							if (application.getMQTTClient().ready()) {
								initialzed = true;
							}
						}
					}
					break;
			}
		}
	}

	TMQTTConnector() {
		initialzed = false;
		retry = now();
		delay = 1;
	}
};
#endif


TApplication::TApplication() : TThreadAffinity(), args(false) {
	if (util::assigned(FApplication))
		throw util::app_error("TApplication::TApplication(): Only one class instance allowed.");
	FApplication = this;
	start = util::now();
	config = nil;
	onSigint = nil;
	onSigterm = nil;
	onSigkill = nil;
	onSighup = nil;
	onSigdefault = nil;
	onSigusr1 = nil;
	onSigusr2 = nil;
	cntSigdef = 0;
	cntSigint = 0;
	cntSigterm = 0;
	cntSighup = 0;
	cntSigusr1 = 0;
	cntSigusr2 = 0;
	onHotplug = nil;
	onTerminal = nil;
	tid = TThreadUtil::gettid();
	pid = getpid();
	uid = -1;
	assignedCPU = app::nsizet;
	signalThd = 0;
	watchThd = 0;
	udevThd = 0;
	commThd = 0;
	rebooting = false;
	terminating = false;
	signalThreadRunning = false;
	signalThreadStarted = false;
	watchThreadRunning = false;
	watchThreadStarted = false;
	udevThreadRunning = false;
	udevThreadStarted = false;
	commThreadRunning = false;
	commThreadStarted = false;
	eventsEnabled = false;
	skipCheck = false;
	multiple = false;
	unprepared = false;
	terminated = false;
	daemonize = false;
	daemonized = false;
	stopped = false;
	halted = false;
	licensed = false;
	inputModeSet = false;
	webServing = false;
	webControl = false;
	useMQTT = false;
	heapTime = util::now();
	error = EXIT_SUCCESS;
	serial10 = 28237704192U; // Base36 --> CZ00000
	serial36 = util::TBase36::encode(serial10, true);
	executed = 0;
	niceLevel = 0;
	changes = 0;
}

TApplication::~TApplication() {
	util::freeAndNil(sysdat.obj.timers);
	util::freeAndNil(sysdat.obj.tasks);
	util::freeAndNil(sysdat.obj.timeouts);
	util::freeAndNil(sysdat.obj.threads);
	util::freeAndNil(sysdat.obj.serial);

#ifdef USE_GPIO_CONTROL
	util::freeAndNil(sysdat.obj.gpio);
#endif
#ifdef USE_MQTT_CLIENT
	util::freeAndNil(sysdat.obj.mqtt);
#endif

	util::freeAndNil(sysdat.obj.gpioConfig);
	util::freeAndNil(sysdat.obj.mqttConfig);
	util::freeAndNil(sysdat.obj.webServer);
	util::freeAndNil(sysdat.obj.webConfig);
	util::freeAndNil(sysdat.obj.session);
	util::freeAndNil(sysdat.obj.sockets);

	// Leave logger instantiated as long as possible
	// --> other objects may access logs during delete!
	util::freeAndNil(sysdat.obj.logger);

	// Free application confi file object
	util::freeAndNil(config);

	// Restore console input
	restoreConsole();
	cleanup();
}

void TApplication::cleanup() {
	deletePidFile();
}


void TApplication::terminateSignalThread() {
	if (signalThreadStarted) {
		signalThreadStarted = false;
		signal.post();
		int retVal = terminateThread(signalThd);
		signalThreadRunning = false;
		if (util::checkFailed(retVal))
			throw util::sys_error("TApplication::terminateSignalThread::pthread_join() failed.");
	}
}

void TApplication::terminateWatchThread() {
	if (watchThreadStarted) {
		watchThreadStarted = false;
		watch.notify();
		int retVal = terminateThread(watchThd);
		watchThreadRunning = false;
		if (util::checkFailed(retVal))
			throw util::sys_error("TApplication::terminateWatchThread::pthread_join() failed.");
	}
}

void TApplication::terminateUdevThread() {
	if (udevThreadStarted) {
		udevThreadStarted = false;
		hotplug.notify();
		int retVal = terminateThread(udevThd);
		udevThreadRunning = false;
		if (util::checkFailed(retVal))
			throw util::sys_error("TApplication::terminateUdevThread::pthread_join() failed.");
	}
}

void TApplication::terminateCommThread() {
	if (commThreadStarted) {
		commThreadStarted = false;
		if (hasTerminal())
			sysdat.obj.serial->notify();
		int retVal = terminateThread(commThd);
		commThreadRunning = false;
		if (util::checkFailed(retVal))
			throw util::sys_error("TApplication::terminateCommThread::pthread_join() failed.");
	}
}


bool TApplication::useGPIO() const {
#ifdef USE_GPIO_CONTROL
	return sysdat.gpio.useGPIO;
#else
	return false;
#endif
}

bool TApplication::hasGPIO() const {
#ifdef USE_GPIO_CONTROL
	return util::assigned(sysdat.obj.gpio);
#else
	return false;
#endif
}

bool TApplication::useSockets() const {
	return sysdat.sockets.useSockets;
}

bool TApplication::hasThreads() const {
	return util::assigned(sysdat.obj.threads);
}

bool TApplication::hasTimers() const {
	return util::assigned(sysdat.obj.timers);
}

bool TApplication::hasTasks() const {
	return util::assigned(sysdat.obj.tasks);
}

bool TApplication::hasTimeouts() const {
	return util::assigned(sysdat.obj.timeouts);
}

bool TApplication::hasWebServer() const {
	return util::assigned(sysdat.obj.webServer);
}

bool TApplication::hasTerminal() const {
	return util::assigned(sysdat.obj.serial);
}


inet::TSocketController& TApplication::getSockets() const {
	if (util::assigned(sysdat.obj.sockets)) {
		return *sysdat.obj.sockets;
	}
	throw util::app_error("TApplication::getSockets() Socket controller not available.");
}

app::TThreadController& TApplication::getThreads() const {
	if (util::assigned(sysdat.obj.threads)) {
		return *sysdat.obj.threads;
	}
	throw util::app_error("TApplication::getThreads() Thread controller not available.");
}

app::TTimerController& TApplication::getTimers() const {
	if (util::assigned(sysdat.obj.timers)) {
		return *sysdat.obj.timers;
	}
	throw util::app_error("TApplication::getTimers() System timer controller not available.");
}

app::TTaskController& TApplication::getTasks() const {
	if (util::assigned(sysdat.obj.tasks)) {
		return *sysdat.obj.tasks;
	}
	throw util::app_error("TApplication::getTasks() Task system not available.");
}

app::TTimeoutController& TApplication::getTimeouts() const {
	if (util::assigned(sysdat.obj.timeouts)) {
		return *sysdat.obj.timeouts;
	}
	throw util::app_error("TApplication::getTimeouts() Timeout controller not available.");
}


int TApplication::getVerbosity() const {
	return sysdat.log.verbosity;
}
util::TTimePart TApplication::getLogFlushDelay() const {
	return sysdat.log.cyclicLogFlush;
}

const std::string& TApplication::getCurrentFolder() const {
	return sysdat.app.currentFolder;
}
const std::string& TApplication::getLogFolder() const {
	return sysdat.app.logFolder;
}
const std::string& TApplication::getTempFolder() const {
	return sysdat.app.tmpFolder;
}
const std::string& TApplication::getConfigFolder() const {
	return sysdat.app.configFolder;
}
const std::string& TApplication::getDataRootFolder() const {
	return sysdat.app.dataFolder;
}
const std::string& TApplication::getPIDFolder() const {
	return sysdat.app.pidFolder;
}
const std::string& TApplication::getPIDFile() const {
	return sysdat.app.pidFile;
}
const std::string& TApplication::getBackupFile() const {
	return sysdat.app.backupFile;
}
const std::string& TApplication::getDataBaseFolder() const {
	if (util::assigned(sysdat.obj.session)) {
		return sysdat.obj.session->getDataFolder();
	}
	throw util::app_error("TApplication::getDataFolder() System session not available.");
}
const std::string& TApplication::getImportFolder() const {
	if (util::assigned(sysdat.obj.session)) {
		return sysdat.obj.session->getImportFolder();
	}
	throw util::app_error("TApplication::getImportFolder() System session not available.");
}

const std::string& TApplication::getFileName() const {
	return sysdat.app.appFileName;
}
const std::string& TApplication::getFileBaseName() const {
	return sysdat.app.appBaseName;
}
const std::string& TApplication::getDisplayName() const {
	return sysdat.app.appDisplayName;
}
const std::string& TApplication::getUserName() const {
	return sysdat.app.userName;
}
const std::string& TApplication::getVersion() const {
	return sysdat.app.appVersion;
}
const std::string& TApplication::getBanner() const {
	return sysdat.app.appBanner;
}
const std::string& TApplication::getLogo() const {
	return sysdat.app.appLogo;
}

const std::string& TApplication::getHostName() const {
	app::TLockGuard<app::TMutex> lock(configMtx);
	return sysdat.app.hostName;
}
void TApplication::setHostName(const std::string& hostName) {
	app::TLockGuard<app::TMutex> lock(configMtx);
	if (hostName != sysdat.app.hostName) {
		sysdat.app.hostName = hostName;
		++changes;
	}
}

const std::string& TApplication::getDescription() const {
	app::TLockGuard<app::TMutex> lock(configMtx);
	return sysdat.app.appDescription;
}
void TApplication::setDescription(const std::string& description) {
	app::TLockGuard<app::TMutex> lock(configMtx);
	if (description != sysdat.app.appDescription) {
		sysdat.app.appDescription = description;
		++changes;
	}
}

const std::string& TApplication::getJumbotron() const {
	app::TLockGuard<app::TMutex> lock(configMtx);
	return sysdat.app.appJumbotron;
}
void TApplication::setJumbotron(const std::string& description) {
	app::TLockGuard<app::TMutex> lock(configMtx);
	if (description != sysdat.app.appJumbotron) {
		sysdat.app.appJumbotron = description;
		++changes;
	}
}

TLogFile& TApplication::getExceptionLogger() const {
	if (util::assigned(sysdat.obj.exceptionLog)) {
		return *sysdat.obj.exceptionLog;
	}
	throw util::app_error("TApplication::getExceptionLogger() Exception logfile not available.");
}
TLogFile& TApplication::getApplicationLogger() const {
	if (util::assigned(sysdat.obj.applicationLog)) {
		return *sysdat.obj.applicationLog;
	}
	throw util::app_error("TApplication::getApplicationLogger() Application logfile not available.");
}
TLogFile& TApplication::getDatabaseLogger() const {
	if (util::assigned(sysdat.obj.datbaseLog)) {
		return *sysdat.obj.datbaseLog;
	}
	throw util::app_error("TApplication::getDatabaseLogger() Database logfile not available.");
}
TLogFile& TApplication::getTaskLogger() const {
	if (util::assigned(sysdat.obj.tasksLog)) {
		return *sysdat.obj.tasksLog;
	}
	throw util::app_error("TApplication::getTaskLogger() Task logfile not available.");
}
TLogFile& TApplication::getTimerLogger() const {
	if (util::assigned(sysdat.obj.timerLog)) {
		return *sysdat.obj.timerLog;
	}
	throw util::app_error("TApplication::getTimerLogger() Timer logfile not available.");
}
TLogFile& TApplication::getThreadLogger() const {
	if (util::assigned(sysdat.obj.threadLog)) {
		return *sysdat.obj.threadLog;
	}
	throw util::app_error("TApplication::getThreadLogger() Thread logfile not available.");
}
TLogFile& TApplication::getSocketLogger() const {
	if (util::assigned(sysdat.obj.socketLog)) {
		return *sysdat.obj.socketLog;
	}
	throw util::app_error("TApplication::getSocketLogger() Socket logfile not available.");
}
TLogFile& TApplication::getWebLogger() const {
	if (util::assigned(sysdat.obj.webLog)) {
		return *sysdat.obj.webLog;
	}
	throw util::app_error("TApplication::getWebLogger() Web service logfile not available.");
}

TWebServer& TApplication::getWebServer() const {
	if (util::assigned(sysdat.obj.webServer)) {
		return *sysdat.obj.webServer;
	}
	throw util::app_error("TApplication::getWebServer() Webserver not available.");
}
TSerial& TApplication::getTerminal() const {
	if (util::assigned(sysdat.obj.serial)) {
		return *sysdat.obj.serial;
	}
	throw util::app_error("TApplication::getSerialPort() Serial terminal not available.");
}

TCredentials& TApplication::getCredentials() const {
	return sysdat.users;
}

#ifdef USE_GPIO_CONTROL
TGPIOController& TApplication::getGPIOController() const {
	if (util::assigned(sysdat.obj.gpio)) {
		return *sysdat.obj.gpio;
	}
	throw util::app_error("TApplication::getGPIOController() GPIO subsystem not available.");
}
#endif
#ifdef USE_MQTT_CLIENT
inet::TMQTT& TApplication::getMQTTClient() const {
	if (util::assigned(sysdat.obj.mqtt)) {
		return *sysdat.obj.mqtt;
	}
	throw util::app_error("TApplication::getMQTTClient() MQTT client not available.");
}
#endif

sql::TSession& TApplication::getSystemSession() const {
	if (util::assigned(sysdat.obj.session)) {
		return *sysdat.obj.session;
	}
	throw util::app_error("TApplication::getSystemSession() System session not available.");
}
sql::TContainer& TApplication::getSystemDatabase() const {
	if (hasSystemDatabase()) {
		return *getSystemContainer();
	}
	throw util::app_error("TApplication::getSystemDatabase() System database not available.");
}
sql::TContainer& TApplication::getApplicationDatabase() const {
	if (hasApplicationDatabase()) {
		return *getApplicationContainer();
	}
	throw util::app_error("TApplication::getSystemDatabase() Application database not available.");
}


inet::PSocketController TApplication::sockets() const {
	if (util::assigned(sysdat.obj.sockets)) {
		return sysdat.obj.sockets;
	}
	throw util::app_error("TApplication::sockets() Socket controller not available.");
}

app::PThreadController TApplication::threads() const {
	if (util::assigned(sysdat.obj.threads)) {
		return sysdat.obj.threads;
	}
	throw util::app_error("TApplication::threads() Thread controller not available.");
}

app::PTimerController TApplication::timers() const {
	if (util::assigned(sysdat.obj.timers)) {
		return sysdat.obj.timers;
	}
	throw util::app_error("TApplication::timers() System timer controller not available.");
}

app::PTaskController TApplication::tasks() const {
	if (util::assigned(sysdat.obj.tasks)) {
		return sysdat.obj.tasks;
	}
	throw util::app_error("TApplication::tasks() Task system not available.");
}

app::PTimeoutController TApplication::timeouts() const {
	if (util::assigned(sysdat.obj.timeouts)) {
		return sysdat.obj.timeouts;
	}
	throw util::app_error("TApplication::timeouts() Timeout controller not available.");
}

app::PWebServer TApplication::webserver() const {
	if (util::assigned(sysdat.obj.webServer)) {
		return sysdat.obj.webServer;
	}
	throw util::app_error("TApplication::webserver() Webserver not available.");
}

app::PSerial TApplication::terminal() const {
	if (util::assigned(sysdat.obj.serial)) {
		return sysdat.obj.serial;
	}
	throw util::app_error("TApplication::terminal() Serial terminal not available.");
}

#ifdef USE_GPIO_CONTROL
app::PGPIOController TApplication::gpio() const {
	if (util::assigned(sysdat.obj.gpio)) {
		return sysdat.obj.gpio;
	}
	throw util::app_error("TApplication::gpio() GPIO subsystem not available.");
}
#endif
#ifdef USE_MQTT_CLIENT
inet::PMQTT TApplication::mqtt() const {
	if (util::assigned(sysdat.obj.mqtt)) {
		return sysdat.obj.mqtt;
	}
	throw util::app_error("TApplication::mqtt() MQTT client not available.");
}
#endif


bool TApplication::hasMQTTClient() const {
#ifdef USE_MQTT_CLIENT
	return util::assigned(sysdat.obj.mqtt);
#else
	return false;
#endif
}
bool TApplication::hasSystemSession() const {
	return util::assigned(sysdat.obj.session);
}
bool TApplication::hasSystemDatabase() const {
	return util::assigned(sysdat.obj.dbsys);
}
bool TApplication::hasApplicationDatabase() const {
	return util::assigned(sysdat.obj.dbsql);
}

sql::TContainer* TApplication::getSystemContainer() const {
	return sysdat.obj.dbsys;
}
sql::TContainer* TApplication::getApplicationContainer() const {
	return sysdat.obj.dbsql;
}


bool TApplication::isTerminated() const {
	app::TLockGuard<app::TMutex> lock(terminateMtx);
	return terminated;
}

void TApplication::setTerminated() {
	// Set termination flag and take first step to shut down application
	if (!getAndSetTerminated()) {
		if (!getAndSetStopped()) {
			stop();
		}
	}
}


bool TApplication::getAndSetTerminated() {
	app::TLockGuard<app::TMutex> lock(terminateMtx);
	bool r = terminated;
	terminated = true;
	return r;
}

bool TApplication::getAndSetStopped() {
	app::TLockGuard<app::TMutex> lock(stopMtx);
	bool r = stopped;
	stopped = true;
	return r;
}

bool TApplication::getAndSetHalted() {
	app::TLockGuard<app::TMutex> lock(haltedMtx);
	bool r = halted;
	halted = true;
	return r;
}

bool TApplication::getAndSetUnprepared() {
	app::TLockGuard<app::TMutex> lock(unpreparedMtx);
	bool r = unprepared;
	unprepared = true;
	return r;
}


const std::string& TApplication::getLicenseBaseURL() const {
	if (!licenseURL.empty())
		return licenseURL;
	licenseURL = LICENCE_BASE_URL;
	return licenseURL;
}

const std::string& TApplication::getLicenseKey() const {
	if (!licenseKey.empty())
		return licenseKey;
#ifdef USE_KEYLOK_DONGLE
	if (keylok.isValid()) {
		kvalue_t serial = keylok.getSerial();
		licenseKey =  util::cprintf("DBA01%05d", serial);
	} else {
		licenseKey = "Trial mode";
	}
#else
	licenseKey = "Unlimited";
#endif
	return licenseKey;
};

size_t TApplication::getLicenseNumber() const {
#ifdef USE_KEYLOK_DONGLE
	return (size_t)keylok.getSerial();
#else
	return serial10;
#endif
}

bool TApplication::checkNamedLicense(const std::string& name, const std::string& value) const {
	if (!name.empty() && !value.empty()) {
		if (value == getNamedLicense(name))
			return true;
	}
	return false;
}

bool TApplication::applyNamedLicense(const std::string& name, const std::string& value) {
	if (checkNamedLicense(name, value)) {
		licenses.add(name, value);
		return true;
	}
	return false;
}

void TApplication::updateNamedLicenses() {
	if (licenses.changed()) {
		config->deleteSection(APP_LICENSE);
		if (!licenses.empty()) {
			config->setSection(APP_LICENSE);
			for (size_t i=0; i<licenses.size(); i++) {
				const std::string& key = licenses.name(i);
				const std::string& value = licenses.value(i).asString();
				config->writeString(key, value);
			}
		}
		flushApplicationSettings();
		licenses.reset();
	}
}

std::string TApplication::getNamedLicense(const std::string& name) const {
	std::string key = util::tolower(name);
	std::string license = key + ":" + getLicenseKey();
	util::TDigest MD5(util::EDT_MD5);
	return MD5(license);
}

bool TApplication::isLicensed(const std::string& name) const {
#ifdef USE_KEYLOK_DONGLE
	if (isLicensed() && !name.empty() && !licenses.empty()) {
		size_t idx = licenses.find(name);
		if (app::nsizet != idx) {
			std::string license = licenses[idx].asString();
			if (checkNamedLicense(name, license))
				return true;
		}
	}
	return false;
#else
	return true;
#endif
}


void TApplication::initialize(int argc, char *argv[]) {
	sysdat.obj.self = this;
	sysdat.obj.exceptionLog = nil;
	sysdat.obj.applicationLog = nil;
	try {
		// Cleanup on terminate
		atexit(appExitHandler);

		// Install signal handler
		installSignalHandlers();

		// Read command line parameters
		parseCommandLine(argc, argv);

		// Get current user ID
		uid = getuid();

		// Real folder and name of application
		sysdat.app.currentFolder = util::realPath(util::filePath(argv[0]));
		sysdat.app.appBaseName = fileExtName(argv[0]);
		sysdat.app.appFileName = sysdat.app.currentFolder + sysdat.app.appBaseName;
		sysdat.app.appDisplayName = fileBaseName(argv[0]);
		sysdat.app.hostName = sysutil::getHostName();
		name = sysdat.app.appDisplayName;
		if (name.size() < 2)
			throw util::sys_error("TApplication::initialize() : Invalid application name <" + name + ">");

		// Add current folder to working set
		workingFolders.add(sysdat.app.currentFolder);

		// Configuration folder, usually /etc/dbApps/<AppName>/<AppName>.conf
		// or supplied by command line
		if (!fileExists(sysdat.app.configFile)) {
			sysdat.app.configFolder = CONFIG_BASE_FOLDER + sysdat.app.appDisplayName;
		} else {
			sysdat.app.configFolder = filePath(sysdat.app.configFile);
		}
		validPath(sysdat.app.configFolder);

		// Create configuration folder
		if (!util::folderExists(sysdat.app.configFolder))
			if (!createDirektory(sysdat.app.configFolder))
				throw util::sys_error("TApplication::initialize() : Creating config folder failed <" + sysdat.app.configFolder + ">");

		// Read configuration file
		if (!fileExists(sysdat.app.configFile))
			sysdat.app.configFile = sysdat.app.configFolder + sysdat.app.appDisplayName + ".conf";

		config = new app::TIniFile(sysdat.app.configFile);
		config->setSection(APP_CONFIG);

		// Try to get temporary folder from environment
		sysdat.app.tmpFolder = util::getEnvironmentVariable("TMP");
		if (sysdat.app.tmpFolder.empty())
			sysdat.app.tmpFolder = util::getEnvironmentVariable("TEMP");
		if (sysdat.app.tmpFolder.empty())
			sysdat.app.tmpFolder = "/tmp/";
		util::validPath(sysdat.app.tmpFolder);
		sysdat.app.tmpFolder = config->readPath("TempFolder", sysdat.app.tmpFolder);

		// Check KEYLOK dongle
#ifdef USE_KEYLOK_DONGLE
		keylok.detect();
		licensed = keylok.isValid();
#else
		licensed = true;
#endif

		// Read application description
		sysdat.app.hostName = config->readString("Hostname", sysdat.app.hostName);
		sysdat.app.appDescription = config->readString("Description", "Reference Music Player");
		sysdat.app.appJumbotron = config->readString("Jumbotron", html::THTML::applyFlowControl(sysdat.app.appDescription));
		sysdat.app.appBanner = config->readString("Banner", "Powered by db Application RAD.web<sup>&reg;</sup>");
		sysdat.app.appLogo = config->readString("Logo", "/images/logo72.png");

		// Read user to run under from configuration
		sysdat.app.userName = config->readString("RunAsUser", sysutil::getUserName(uid));
		sysdat.app.groupName = config->readString("RunAsGroup", sysutil::getUserName(uid));
		sysdat.app.groupList = config->readString("SupplementalGroups", SUPPLEMENTAL_GROUP_LIST);
		sysdat.app.capsList = config->readString("Capabilities", DEFAULT_CAPABILITIES_LIST);

		// Which folder to use as working directory (PWD)
		sysdat.app.setTempDir = config->readBool("UseTempDirAsPwd", sysdat.app.setTempDir);

		// Get data folder from configuration
		sysdat.app.dataFolder = config->readPath("DataFolder", util::validPath(DATA_BASE_FOLDER + sysdat.app.appDisplayName));
		if (!createDataFolder())
			throw util::sys_error("TApplication::initialize() : Creating application data folder failed <" + sysdat.app.dataFolder + ">");

		// Set systen configuration backup file (stored in system data folder)
		sysdat.app.backupFile = sysdat.app.dataFolder + "settings.tar.gz";

		// PID file storage:
		// Root user -> /var/run/<AppName>/<AppName>.pid
		// Non root  -> /tmp/<AppName>.pid
		if (uid == 0) sysdat.app.pidFile = "/var/run/" + sysdat.app.appDisplayName + "/" + sysdat.app.appDisplayName + ".pid";
		else sysdat.app.pidFile = "/tmp/" + sysdat.app.appDisplayName + ".pid";
		sysdat.app.pidFile = config->readString("PIDFile", sysdat.app.pidFile);
		sysdat.app.pidFolder = util::filePath(sysdat.app.pidFile);

		// Create PID folder for user to run under
		if (!createPidFolder())
			throw util::sys_error("TApplication::initialize() : Creating PID folder failed <" + sysdat.app.pidFolder + ">");

		// Create application store folder
		sysdat.app.storeFolder = util::validPath(sysdat.app.dataFolder + "store");
		if (!createStoreFolder())
			throw util::sys_error("TApplication::initialize() : Creating storage folder failed <" + sysdat.app.storeFolder + ">");
		sysdat.app.storeFile = sysdat.app.storeFolder + "content.json";

		// Check for single application instance
		sysdat.app.runOnce = config->readBool("RunOnce", sysdat.app.runOnce);
		if (sysdat.app.runOnce && !skipCheck) {
			if (util::fileExists(sysdat.app.pidFile)) {
				long int proc = readPidFile();
				if (sysutil::isProcessRunning(proc)) {
					multiple = true;
					throw util::app_error("Only one instance allowed, terminating application now.");
				}
			}
		}

		// Change nice level when started as root
		sysdat.app.nice = config->readInteger("NiceLevel", sysdat.app.nice);
		int niceres = EXIT_FAILURE;
		if (sysdat.app.nice != 0) {
			errno = EXIT_SUCCESS;
			int r = ::nice(sysdat.app.nice);
			if (r != sysdat.app.nice || EXIT_SUCCESS != errno) {
				niceLevel = 0;
				throw util::sys_error("TApplication::initialize() : Setting nice level (" + std::to_string(sysdat.app.nice) + ") failed.");
			} else {
				niceLevel = sysdat.app.nice;
				niceres = EXIT_SUCCESS;
			}
		}

		// Prevent RAM from swapping if RT nice level set when started as root
		int handles = 0;
		bool soft = false;
		bool hard = false;
		bool locked = false;
		bool niceset = niceres == EXIT_SUCCESS;
		bool noswap = config->readBool("DisableSwappiness", true);
		if (noswap && niceset && uid == 0) {
			// Set memory lock limit to infinity
			if (!sysutil::setHardRessourceLimit(sysutil::ERL_LIMIT_MEMLOCK, RLIM_INFINITY))
				throw util::sys_error("TApplication::initialize() : Setting hard locked memory limit to infinity failed.");

			// Hard memory locked was set
			hard = true;

			// Lock all current and future memory
			if (!sysutil::lockMemory(MCL_CURRENT | MCL_FUTURE))
				throw util::sys_error("TApplication::initialize() : Locking RAM failed.");

			// Memory locked now
			locked = true;
		}

		// Run as daemon?
		bool userChanged = false;
		std::string groupNames, capabilityNames;
		if (daemonize) {
			// Detach application from console and change user
			// --> Allow serial port, GPIO (et al.) access
			daemonizer(sysdat.app.userName, sysdat.app.groupName, sysdat.app.groupList, sysdat.app.capsList, groupNames, capabilityNames, userChanged);

			// Reopen configuration file as new user
			if (userChanged) {
				util::freeAndNil(config);
				config = new app::TIniFile(sysdat.app.configFile);
				uid = getuid();
			}
		}

		// Allow memory locking for user
		if ((!hard || userChanged) && uid != 0) {
			rlim_t limit = sysutil::getMaxRessourceLimit(sysutil::ERL_LIMIT_MEMLOCK);
			if (limit <= 0) {
				limit = (rlim_t)SOFT_LOCK_LIMIT;
			}
			// Set soft memlock limit
			if (!sysutil::setSoftRessourceLimit(sysutil::ERL_LIMIT_MEMLOCK, limit)) {
				throw util::sys_error("TApplication::initialize() : Setting soft locked memory limit failed.");
			}
			soft = true;
		}

		// Set affinity to specific processor (-1 = disabled, 0 = choose processor, 1..n fixed processor)
		// --> Result is processor 1..n or <= 0 on error
		config->setSection(APP_CONFIG);
		sysdat.app.affinity = config->readInteger("Affinity", sysdat.app.affinity);
		if (sysdat.app.affinity >= 0) {
			ssize_t r = setAffinity(sysdat.app.affinity, tid);
			if (r > 0)
				assignedCPU = (size_t)r;
		}

		// Get isolated CPU mask from kernel configuration
		if (sysutil::getIsolatedCPUs(isolatedCPU)) {
			// Normalize CPU mask
			int cpu = util::strToInt(isolatedCPU, -1);
			if (cpu > -1) {
				isolatedCPU = std::to_string(cpu + 1);
			}
		} else {
			// throw util::sys_error("TApplication::initialize()::getIsolatedCPUs() failed.");
		}

		// Set max. count of open file handles for process
		sysdat.app.handles = config->readInteger("Handles", sysdat.app.handles);
		if (sysdat.app.handles > 0) {
			rlim_t limit = (rlim_t)sysdat.app.handles;
			rlim_t soft = sysutil::getCurrentRessourceLimit(sysutil::ERL_LIMIT_NOFILE);
			if (soft <= 0) {
				soft = (rlim_t)1024;
			}
			rlim_t hard = sysutil::getMaxRessourceLimit(sysutil::ERL_LIMIT_NOFILE);
			if (hard <= 0) {
				hard = soft;
			}
			if (limit > hard) {
				limit = hard;
			}
			if (soft < limit) {
				if (!sysutil::setSoftRessourceLimit(sysutil::ERL_LIMIT_NOFILE, limit)) {
					throw util::sys_error("TApplication::initialize() : Setting soft file handle limit failed.");
				}
				handles = (int)limit;
			}
		}

		// Set locale for application
		std::string lang = syslocale.getName();
		ELocale sysloc = syslocale.find(lang);
		if (lang.size() < 5 || !syslocale.isValidLocale(sysloc)) {
			// Set default language
			lang = sysdat.app.language;
		}
		sysdat.app.language = config->readString("Locale", lang);
		config->writeString("Locale", sysdat.app.language);
		std::cout << "Application locale   [" << sysdat.app.language << "]" << std::endl;
		std::cout << "System locale        [" << syslocale.getSystemLocale() << "]" << std::endl;
		std::cout << "Locale information   " << syslocale.getInfo() << std::endl;
		std::cout << "Location information " << syslocale.getLocation() << std::endl;

		// Change locale settings if needed
		ELocale newloc = syslocale.find(sysdat.app.language);
		if (newloc != ELocale::nloc && newloc != ELocale::sysloc) {
			syslocale.set(newloc);
			syslocale.use();
		}
		sysdat.app.locale = newloc;
		std::cout << "System locale [" << syslocale.getSystemLocale() << "]" << std::endl;
		std::cout << "Locale information " << syslocale.getInfo() << std::endl;
		std::cout << "Location information " << syslocale.getLocation() << std::endl;

		// Read log folder from configuration
		sysdat.app.logFolder = LOG_BASE_FOLDER + sysdat.app.appDisplayName + "/";
		sysdat.app.logFolder = config->readPath("LoggingFolder", sysdat.app.logFolder);
		config->writePath("LoggingFolder", sysdat.app.logFolder);

		// Create log folder
		if (!util::folderExists(sysdat.app.logFolder))
			if (!createDirektory(sysdat.app.logFolder))
				throw util::sys_error("TApplication::initialize() : Creating logging folder failed <" + sysdat.app.logFolder + ">");

		// Write application configuration section after daemonizing process
		config->writePath("DataFolder", sysdat.app.dataFolder);
		config->writeString("Hostname", sysdat.app.hostName);
		config->writeString("Description",sysdat.app.appDescription);
		config->writeString("Jumbotron",sysdat.app.appJumbotron);
		config->writeString("Banner",sysdat.app.appBanner);
		config->writeString("Logo",sysdat.app.appLogo);
		config->writeBool("RunOnce", sysdat.app.runOnce, INI_BLYES);
		config->writeBool("UseDongle", sysdat.app.useDongle, INI_BLYES);
		config->writePath("TempFolder", sysdat.app.tmpFolder);
		config->writeBool("UseTempDirAsPwd", sysdat.app.setTempDir, INI_BLYES);
		config->writeString("PIDFile", sysdat.app.pidFile);
		config->writeInteger("NiceLevel", sysdat.app.nice);
		config->writeInteger("Affinity", sysdat.app.affinity);
		config->writeInteger("Handles", sysdat.app.handles);
		config->writeString("RunAsUser", sysdat.app.userName);
		config->writeString("RunAsGroup", sysdat.app.groupName);
		config->writeString("SupplementalGroups", sysdat.app.groupList.csv());
		config->writeString("Capabilities", sysdat.app.capsList.csv());
		config->writeBool("DisableSwappiness", noswap, INI_BLYES);

		// Create log files
		sysdat.obj.logger = new app::TLogController(sysdat.app.appDisplayName, sysdat.app.logFolder, sysdat.app.configFolder, !application.isDaemonized() && (sysdat.log.verbosity > 1));
		sysdat.obj.applicationLog = sysdat.obj.logger->addLogFile(LOG_APPLICATION_NAME);
		sysdat.obj.exceptionLog = sysdat.obj.logger->addLogFile(LOG_EXCEPTION_NAME);
		sysdat.obj.tasksLog = sysdat.obj.logger->addLogFile(LOG_TASKS_NAME);
		sysdat.obj.timerLog = sysdat.obj.logger->addLogFile(LOG_TIMER_NAME);
		sysdat.obj.threadLog = sysdat.obj.logger->addLogFile(LOG_THREADS_NAME);
		sysdat.obj.socketLog = sysdat.obj.logger->addLogFile(LOG_SOCKETS_NAME);
		sysdat.obj.webLog = sysdat.obj.logger->addLogFile(LOG_WEBSERVER_NAME);
		sysdat.obj.datbaseLog = sysdat.obj.logger->addLogFile(LOG_DATABASE_NAME);

		sysdat.obj.applicationLog->write("[Application] Open logfile <" + sysdat.obj.applicationLog->getName() + ">");
		sysdat.obj.exceptionLog->write("[Application] Open logfile <" + sysdat.obj.exceptionLog->getName() + ">");
		sysdat.obj.tasksLog->write("[Application] Open logfile <" + sysdat.obj.tasksLog->getName() + ">");
		sysdat.obj.timerLog->write("[Application] Open logfile <" + sysdat.obj.timerLog->getName() + ">");
		sysdat.obj.threadLog->write("[Application] Open logfile <" + sysdat.obj.threadLog->getName() + ">");
		sysdat.obj.socketLog->write("[Application] Open logfile <" + sysdat.obj.socketLog->getName() + ">");
		sysdat.obj.webLog->write("[Application] Open logfile <" + sysdat.obj.webLog->getName() + ">");
		sysdat.obj.datbaseLog->write("[Application] Open logfile <" + sysdat.obj.datbaseLog->getName() + ">");

		// Create object lists for timers, timeouts, tasks and threads and sockets
		sysdat.obj.timers = new app::TTimerController(sysdat.app.configFolder, *sysdat.obj.timerLog, *sysdat.obj.exceptionLog);
		sysdat.obj.timeouts = new app::TTimeoutController(TO_TIME_RESOLUTION_MS, sysdat.app.configFolder);
		sysdat.obj.tasks = new app::TTaskController(TA_TIME_RESOLUTION_MS, sysdat.app.configFolder, *sysdat.obj.tasksLog,  *sysdat.obj.exceptionLog);
		sysdat.obj.threads = new app::TThreadController(*sysdat.obj.threadLog, *sysdat.obj.exceptionLog);

		// Cyclic log flushing
		config->setSection(APP_CONFIG);
		sysdat.log.cyclicLogFlush = config->readInteger("CyclicFlushDelay", sysdat.log.cyclicLogFlush);
		config->writeInteger("CyclicFlushDelay", sysdat.log.cyclicLogFlush);
		if (sysdat.log.cyclicLogFlush > 0)
			sysdat.obj.tasks->addTask("CyclicLogFlusher", 2000, new TCyclicLogger());

		// Cyclic heap memory cleanup in seconds (0 == disabled)
		sysdat.app.heapDelay = config->readInteger("HeapDeallocateDelay", sysdat.app.heapDelay);
		config->writeInteger("HeapDeallocateDelay", sysdat.app.heapDelay);

		// Read socket communication parameters
		config->setSection("Sockets");
		sysdat.sockets.useSockets = config->readBool("Enabled", sysdat.sockets.useSockets);
		config->writeBool("Enabled", sysdat.sockets.useSockets, INI_BLYES);

		// Create socket object list
		sysdat.obj.sockets = new inet::TSocketController(sysdat.obj.threads, sysdat.obj.timers, sysdat.obj.socketLog, sysdat.app.configFolder);

		// Start socket polling only if enabled
		if (sysdat.sockets.useSockets) {
			sysdat.obj.sockets->setAcceptHandler(&app::TApplication::onAcceptSocket, this);
			sysdat.obj.sockets->enable();
			sysdat.obj.sockets->start();
		}
	
		// Read serial communication parameters
		config->setSection("Serial");
		sysdat.serial.useSerialPort = config->readBool("Enabled", sysdat.serial.useSerialPort);
		sysdat.serial.serialDevice = config->readString("Device", sysdat.serial.serialDevice);
		sysdat.serial.baudRate = config->readInteger("Baudrate", sysdat.serial.baudRate);
		sysdat.serial.blocking = config->readBool("Blocking", sysdat.serial.blocking);
		sysdat.serial.threaded = config->readBool("Threaded", sysdat.serial.threaded);
		sysdat.serial.chunk = config->readInteger("ChunkSize", sysdat.serial.chunk);
		config->writeBool("Enabled", sysdat.serial.useSerialPort, INI_BLYES);
		config->writeString("Device", sysdat.serial.serialDevice);
		config->writeInteger("Baudrate", sysdat.serial.baudRate);
		config->writeBool("Blocking", sysdat.serial.blocking, INI_BLYES);
		config->writeBool("Threaded", sysdat.serial.threaded, INI_BLYES);
		config->writeInteger("ChunkSize", sysdat.serial.chunk);

		// Open serial port if enabled
		if (sysdat.serial.useSerialPort) {
			sysdat.obj.serial = new TSerial();
			ESerialBlockingType blocking = sysdat.serial.blocking ? SER_DEV_BLOCKING : SER_DEV_NO_BLOCK;
			sysdat.obj.serial->open(sysdat.serial.serialDevice, sysdat.serial.baudRate, blocking);
		}

		// Read GPIO parameters and open GPIO ports
#ifdef USE_GPIO_CONTROL
		config->setSection("GPIO");
		sysdat.gpio.useGPIO = config->readBool("Enabled", sysdat.gpio.useGPIO);
		config->writeBool("Enabled", sysdat.gpio.useGPIO, INI_BLYES);
		config->deleteKey("Filesystem");

		// Initialize GPIO if enabled
		if (sysdat.gpio.useGPIO) {
			if (0 != getuid()) {
				logger(app::ELogBase::LOG_APP, "[Application] Warning: Enable GPIO might not work as non root user, trying anyway...");
			}
			sysdat.obj.gpio = new TGPIOController();
			std::string configFile = sysdat.app.configFolder + "GPIO.conf";
			sysdat.obj.gpioConfig = new app::TIniFile(configFile);
			if (!sysdat.obj.gpio->init(*sysdat.obj.gpioConfig, *sysdat.obj.exceptionLog))
				throw util::app_error("TApplication::initialize() Initializing GPIO subsystem failed <" + sysdat.obj.gpio->getSysFs() + ">");
		}
#endif

		// Open terminal for reading, if enabled and not running as daemon!
		config->setSection(APP_CONFIG);
		sysdat.app.useTerminalInput = config->readBool("UseTerminalInput", sysdat.app.useTerminalInput);
		config->writeBool("UseTerminalInput", sysdat.app.useTerminalInput, INI_BLYES);
		if ((sysdat.app.useTerminalInput) && !daemonized) {
			aquireConsole();
		}

		// Read MIME types from file
		config->setSection("MimeTypes");
		sysdat.app.mimeFile = config->readString("SystemMimeTypeFile", sysdat.app.mimeFile);
		config->writeString("SystemMimeTypeFile", sysdat.app.mimeFile);
		util::loadMimeTypesFromFile(sysdat.app.mimeFile);

		// Open new session for system database
		sysdat.obj.session = new sql::TSession(sysdat.app.configFolder, sysdat.app.dataFolder, *sysdat.obj.datbaseLog, *sysdat.obj.exceptionLog);
		sysdat.obj.session->imbue(syslocale);
		sysdat.obj.dbsys = sysdat.obj.session->open("system", sql::EDB_SQLITE3);

		// Initialize user database
		size_t users = 0;
		sysdat.users.setDatabase(sysdat.obj.dbsys);
		sysdat.users.setLogger(sysdat.obj.applicationLog);
		if (sysdat.users.initialize()) {
			users = sysdat.users.size();
			if (users > 0) {
				std::string s = users == 1 ? "user" : "users";
				logger(app::ELogBase::LOG_APP, "[Application] Found % % in credential database.", users, s);
			} else {
				logger(app::ELogBase::LOG_APP, "[Application] User credential database initialized, but no users in found.");
			}
		} else {
			logger(app::ELogBase::LOG_APP,      "[Application] Failed to initialize user credential database.");
			logger(app::ELogBase::LOG_EXCEPT,   "[Application] Failed to initialize user credential database.");
			logger(app::ELogBase::LOG_DATABASE, "[Application] Failed to initialize user credential database.");
		}

		// Start webserver if enabled
		config->setSection("Webserver");
		sysdat.app.startWebServer = config->readBool("Enabled", sysdat.app.startWebServer);
		bool autostart = config->readBool("Autostart", true);
		config->writeBool("Enabled", sysdat.app.startWebServer, INI_BLYES);
		config->writeBool("Autostart", autostart, INI_BLYES);
		if (sysdat.app.startWebServer) {
			sysdat.obj.webServer = startWebServer("AppWebServer", sysdat.app.currentFolder + "www", autostart);
		}

		if (util::assigned(sysdat.obj.webServer)) {
			sysdat.obj.webServer->setOwner(this);
			sysdat.users.setRealm(sysdat.obj.webServer->getRealm());

			// Use same digest authenication algorithm for app users as web users
			sysdat.users.setDigestType(sysdat.obj.webServer->getDigestType());

			// Setup license mode display
			std::string license = isLicensed() ? getLicenseKey() : "Trial mode";
			sysdat.obj.webServer->addWebToken("APP_LICENSE_MODE", license);

			// Initialze web user access
			if (users > 0) {
				// Use application user list for web server access
				TCredentialMap credentials;
				sysdat.users.getCredentialMap(credentials);
				sysdat.obj.webServer->assignUserCredentials(credentials);
			} else {
				// Add default web user if credential list is empty
				TCredential user;
				std::string credentials = sysdat.obj.webServer->getCredentials();
				util::TStringList sl(credentials, ':');
				if (sl.size() > 1) {
					user.username = sl[0];
					user.password = sl[1];
					user.realm = sysdat.obj.webServer->getRealm();
					user.level = sl.size() > 2 ? util::strToInt(sl[2], DEFAULT_USER_LEVEL) : DEFAULT_USER_LEVEL;
					if (sysdat.users.insert(user)) {
						logger(app::ELogBase::LOG_APP, "[Application] Added default web user <%>", user.username);
					} else {
						logger(app::ELogBase::LOG_APP,    "[Application] Failed to add default web user <%>", user.username);
						logger(app::ELogBase::LOG_EXCEPT, "[Application] Failed to add default web user <%>", user.username);
					}
				}
			}

			// Create hashes for resource integrity check
			if (util::assigned(sysdat.obj.session)) {
				std::string folder = sysdat.obj.session->getDataFolder();
				sysdat.obj.webServer->createSubresourceIntegrityFile(folder);
			}
		}

		// Connect to default application SQL database
		config->setSection("Database");
		sysdat.app.useDefaultDatabase = config->readBool("UseDefaultDatabase", sysdat.app.useDefaultDatabase);
		sysdat.app.defaultDatabaseName = config->readString("DefaultDatabaseName", sysdat.app.defaultDatabaseName);
		sysdat.app.defaultDatabaseType = config->readString("DefaultDatabaseType", sysdat.app.defaultDatabaseType);
		sql::EDatabaseType dbtype = parseDefaultDatabaseType(sysdat.app.defaultDatabaseType);
		if (dbtype != sql::EDB_UNKNOWN) {
			if (sysdat.app.useDefaultDatabase) {
				sysdat.obj.dbsql = sysdat.obj.session->open(sysdat.app.defaultDatabaseName, dbtype);
				if (!util::assigned(sysdat.obj.dbsql)) {
					logger(app::ELogBase::LOG_APP,    "[Database] Failed to create database object <%::%>", sysdat.app.defaultDatabaseType, sysdat.app.defaultDatabaseName);
					logger(app::ELogBase::LOG_EXCEPT, "[Database] Failed to create database object <%::%>", sysdat.app.defaultDatabaseType, sysdat.app.defaultDatabaseName);
				} else {
					logger(app::ELogBase::LOG_APP, "[Database] Default database <%::%> opened.", sysdat.app.defaultDatabaseType, sysdat.app.defaultDatabaseName);
				}
			} else {
				logger(app::ELogBase::LOG_APP, "[Database] Default database to <%::%> disabled by configuration.", sysdat.app.defaultDatabaseType, sysdat.app.defaultDatabaseName);
			}
		} else {
			// Set default to PostgeSQL database
			sysdat.app.defaultDatabaseType = "PGSQL";
			logger(app::ELogBase::LOG_APP, "[Database] Set default database to <%::%>", sysdat.app.defaultDatabaseType, sysdat.app.defaultDatabaseName);
		}

		// Write database connection values
		config->writeBool("UseDefaultDatabase", sysdat.app.useDefaultDatabase, app::INI_BLYES);
		config->writeString("DefaultDatabaseName", sysdat.app.defaultDatabaseName);
		config->writeString("DefaultDatabaseType", sysdat.app.defaultDatabaseType);

		// Connect to external MQTT message broker
#ifdef USE_MQTT_CLIENT
		config->setSection("Internet");
		sysdat.app.startMQTTClient = config->readBool("UseMQTTClient", sysdat.app.startMQTTClient);
		sysdat.app.defaultMQTTConnection = config->readString("MQTTConnectionName", sysdat.app.defaultMQTTConnection);
		if (sysdat.app.startMQTTClient) {

			// Create folder for MQTT persistence database
			std::string dataFolder = application.getDataBaseFolder() + "mqtt/";
			if (!util::folderExists(dataFolder))
				util::createDirektory(dataFolder);

			// Create MQTT configuration file
			std::string configFile = sysdat.app.configFolder + "MQTT.conf";
			sysdat.obj.mqttConfig = new app::TIniFile(configFile);

			// Create MQTT client object
			sysdat.obj.mqtt = new inet::TMQTT(sysdat.app.defaultMQTTConnection, dataFolder, *sysdat.obj.mqttConfig, *sysdat.obj.applicationLog, *sysdat.obj.exceptionLog);
			if (!util::assigned(sysdat.obj.mqtt)) {
				logger(app::ELogBase::LOG_APP,    "[MQTT] Failed to create MQTT object <%>", sysdat.app.defaultMQTTConnection);
				logger(app::ELogBase::LOG_EXCEPT, "[MQTT] Failed to create MQTT object <%>", sysdat.app.defaultMQTTConnection);
			} else {
				// Initialized MQTT client connection
				if (sysdat.obj.mqtt->initialize()) {
					// Start cyclic connection task
					logger(app::ELogBase::LOG_APP, "[MQTT] Client object <%> initialized.", sysdat.app.defaultMQTTConnection);
					sysdat.obj.tasks->addTask("MQTTConnectorTask", 800, new TMQTTConnector());
					useMQTT = true;
				} else {
					logger(app::ELogBase::LOG_APP, "[MQTT] Failed to initialize client object <%>", sysdat.app.defaultMQTTConnection);
				}
			}

		} else {
			logger(app::ELogBase::LOG_APP, "[MQTT] MQTT client <%> disabled by configuration.", sysdat.app.defaultMQTTConnection);
		}
#endif

		// Write MQTT configuration values
		config->writeBool("UseMQTTClient", sysdat.app.startMQTTClient, app::INI_BLYES);
		config->writeString("MQTTConnectionName", sysdat.app.defaultMQTTConnection);

		// Setup serial number and license keys
		bool serialok  = false;
#ifdef USE_KEYLOK_DONGLE
		if (isLicensed()) {
			// Base36 --> CZ00000 + Keyloc serial number
			serial10 += getLicenseNumber();
			serialok = true;
		}
#endif
		// Try to read processor serial number on ARM based systems
		if (!serialok) {
			size_t psn = sysutil::getProcessorSerial();
			if (psn > 0) {
				serial10 = psn;
				serialok = true;
			}
		}
		// Fallback to MAC address
		if (!serialok) {
			std::string eth0 = sysutil::getDefaultAdapter();
			if (!eth0.empty()) {
				std::string mac = sysutil::getMacAddress(eth0);
				if (!mac.empty()) {
					mac = util::replace(mac, ":", "");
					serial10 = util::strToUnsigned64(mac, 0, syslocale, 16);
					serialok = true;
				}
			}
		}
		serial36 = util::TBase36::encode(serial10, true);
		getLicenseBaseURL();
		getLicenseKey();

		// Open file system watch as current user!
		watch.open();

		// Open hotplug monitor for USB devices
		hotplug.open("usb");

		// Create event handler threads
		if (!createJoinableThread(signalThd, signalThreadDispatcher, this, "Process-Signals"))
			throw util::sys_error("TApplication::initialize() : Create signal thread failed.");
		if (!createJoinableThread(watchThd, watchThreadDispatcher, this, "File-Watch"))
			throw util::sys_error("TApplication::initialize() : Create watch thread failed.");
		if (!createJoinableThread(udevThd, udevThreadDispatcher, this, "UDEV-Events"))
			throw util::sys_error("TApplication::initialize() : Create hotplug monitoring thread failed.");
		if (hasTerminal() && sysdat.serial.threaded) {
			if (!createJoinableThread(commThd, commThreadDispatcher, this, "Console-Reader"))
				throw util::sys_error("TApplication::initialize() : Create serial communication thread failed.");
		}

		// Log some information about application startup state
		logger(app::ELogBase::LOG_APP, "[Application] Application [%] started with args %", getVersion(), arguments().asText(' '));
		logger(app::ELogBase::LOG_APP, "[Application] Application uses language %", syslocale.getInfo());
		if (inputModeSet)
			logger(app::ELogBase::LOG_APP, "[Application] Console terminal set to input mode.");
		if (assigned(sysdat.obj.serial))
			logger(app::ELogBase::LOG_APP, "[Application] Serial port <%> in use.", sysdat.serial.serialDevice);
#ifdef USE_GPIO_CONTROL
		if (assigned(sysdat.obj.gpio))
			logger(app::ELogBase::LOG_APP, "[Application] GPIO enabled for <%>", sysdat.obj.gpio->getSysFs());
#endif
#ifdef USE_MQTT_CLIENT
		if (assigned(sysdat.obj.mqtt))
			logger(app::ELogBase::LOG_APP, "[Application] MQTT client enabled for <%>", sysdat.obj.mqtt->getServer());
#endif
		if (daemonized)
			logger(app::ELogBase::LOG_APP, "[Application] Application is daemonized.");
		if (userChanged)
			logger(app::ELogBase::LOG_APP, "[Application] Application is running as user <%>", sysdat.app.userName);
		if (!groupNames.empty())
			logger(app::ELogBase::LOG_APP, "[Application] Application supplementary groups $ assigned.", groupNames);
		if (!capabilityNames.empty())
			logger(app::ELogBase::LOG_APP, "[Application] Application capabilities $ processed.", capabilityNames);
		if (niceset)
			logger(app::ELogBase::LOG_APP, "[Application] Application nice level changed to (%)", sysdat.app.nice);
		if (locked)
			logger(app::ELogBase::LOG_APP, "[Application] Application RAM is locked.");
		if (assignedCPU != app::nsizet)
			logger(app::ELogBase::LOG_APP, "[Application] Application core affinity is set to (%/%/%)", assignedCPU, sysdat.app.affinity, getCoreCount());
		if (!isolatedCPU.empty())
			logger(app::ELogBase::LOG_APP, "[Application] Kernel isolated CPU mask is (%)", isolatedCPU);
		if (hard)
			logger(app::ELogBase::LOG_APP, "[Application] Hard lock limit is set to unlimited for privileged user.");
		if (soft) {
			rlim_t limit = sysutil::getCurrentRessourceLimit(sysutil::ERL_LIMIT_MEMLOCK);
			if (limit == RLIM_INFINITY) {
				logger(app::ELogBase::LOG_APP, "[Application] Soft memory lock limit is set to unlimited for user <%>", sysdat.app.userName);
			} else {
				logger(app::ELogBase::LOG_APP, "[Application] Soft memory lock limit is set to % for user <%>", util::sizeToStr(limit), sysdat.app.userName);
			}
		}
		if (handles > 0) {
			if (handles == sysdat.app.handles)
				logger(app::ELogBase::LOG_APP, "[Application] Open file handle limit is set to % handles.", handles, sysdat.app.handles);
			else
				logger(app::ELogBase::LOG_APP, "[Application] Open file handle limit is increased to % of % configured handles.", handles, sysdat.app.handles);
		}
		if (!util::getMimeMapExtrn().empty()) {
			logger(app::ELogBase::LOG_APP, "[Application] Read % mime entries from file <%>", util::getMimeMapExtrn().size(), sysdat.app.mimeFile);
			if (sysdat.log.verbosity > 1) {
				util::TMimeMap::const_iterator it = util::getMimeMapExtrn().begin();
				for ( ; it != util::getMimeMapExtrn().end(); it++) {
					sysdat.obj.applicationLog->write("Extension [" + it->first + "] --> Mime [" + it->second + "]");
				}
			}
		}

#ifdef USE_KEYLOK_DONGLE
		if (keylok.isValid())
			logger(app::ELogBase::LOG_APP, "[Application] Dongle serial number % found.", keylok.getSerial());
		else
			logger(app::ELogBase::LOG_APP, "[Application] No valid KEYLOK dongle found.");
		if (isLicensed())
			logger(app::ELogBase::LOG_APP, "[Application] Application is licensed.");
		else
			logger(app::ELogBase::LOG_APP, "[Application] Application is in trial mode.");
#endif
		logger(app::ELogBase::LOG_APP, "[Application] Application serial number (%:%)", serial10, serial36);

		// Write changes to configuration files
		if (sysdat.log.verbosity > 0)
			config->debugOutput();
		flushApplicationSettings();
		flushSystemSettings();

		// Create backup of current configuration file settings
		backupConfigurationFiles();

		// Check for further license keys
		config->readSection(APP_LICENSE, licenses);
		licenses.asJSON().saveToFile(sysdat.app.pidFolder + "licenses.json");
		licenses.reset();

		// Load last saved application storage
		if (util::fileExists(sysdat.app.storeFile)) {
			storage.loadFromFile(sysdat.app.storeFile);
			logger(app::ELogBase::LOG_APP, "[Application] Read % entries from local storage <%>", storage.size(), sysdat.app.storeFile);
		} else {
			logger(app::ELogBase::LOG_APP, "[Application] Local storage <%> does not exists.", sysdat.app.storeFile);
		}

		// Save parameters and version to file
		TTimePart time = util::fileAge(sysdat.app.appFileName);
		TDateTime stamp(time);
		util::TVariantValues version;
		version.add("Name", sysdat.app.appBaseName);
		version.add("Command", sysdat.app.appFileName);
		version.add("Description", sysdat.app.appDescription);
		version.add("Parameter", application.args.asText(' '));
		version.add("License", getLicenseKey());
		version.add("Version", sysdat.app.appVersion);
		version.add("Date", stamp.asISO8601());
		version.asJSON().saveToFile(sysdat.app.pidFolder + "version.json");

		// Save given commandline arguments to file
		args.asJSON().saveToFile(sysdat.app.pidFolder + "cmdline.json");

		// Save application's capabilities to file
		saveCapabilitiesToFile(sysdat.app.pidFolder + application.getFileBaseName() + ".cap");

		// Write PID to file
		if (writePidFile()) {
			logger(app::ELogBase::LOG_APP, "[Application] PID [%] saved to file <%>", pid, sysdat.app.pidFile);
		} else {
			logger(app::ELogBase::LOG_APP, "[Application] Writing PID [%] to file <%> failed.", pid, sysdat.app.pidFile);
		}

		// Do trace on terminate
		std::set_terminate(appTerminateHandler);

		// Notify systemd
		notifySystemState(SYS_READY);

		// Flush all startup log entries
		sysdat.obj.logger->flush();

	} catch (const std::exception& e) {
		error = EXIT_FAILURE;
		if (util::assigned(sysdat.obj.applicationLog) && util::assigned(sysdat.obj.exceptionLog)) {
			std::string sExcept = e.what();
			std::string sText = "Exception in TApplication::initialize() \n" + sExcept + "\n";
			errorLog(sText);
		}
	} catch (...) {
		error = EXIT_FAILURE;
		if (util::assigned(sysdat.obj.applicationLog) && util::assigned(sysdat.obj.exceptionLog)) {
			std::string sText = "Unknown exception in TApplication::initialize()";
			errorLog(sText);
		}
	}
}


sql::EDatabaseType TApplication::parseDefaultDatabaseType(const std::string& description) const {
	// MSSQL, SQLITE, PGSQL, ...
	if (0 == util::strncasecmp(description, "PGSQL", 5))
		return sql::EDB_PGSQL;
	if (0 == util::strncasecmp(description, "MSSQL", 5))
		return sql::EDB_MSSQL;
	if (0 == util::strncasecmp(description, "MYSQL", 5))
		return sql::EDB_MYSQL;
	if (0 == util::strncasecmp(description, "SQLITE", 6))
		return sql::EDB_SQLITE3;
	return sql::EDB_UNKNOWN;
}

std::string TApplication::getWorkingFolder() const {
	app::TLockGuard<app::TMutex> lock(pathMtx);
	if (!workingFolders.empty()) {
		return workingFolders[0];
	}
	return std::string();
}

bool TApplication::getWorkingFolders(util::TStringList& folders) const {
	app::TLockGuard<app::TMutex> lock(pathMtx);
	folders = workingFolders;
	return !folders.empty();
}

void TApplication::setWorkingFolders(const util::TStringList& folders) {
	app::TLockGuard<app::TMutex> lock(pathMtx);
	workingFolders.clear();
	if (!folders.empty()) {
		TStringList::const_iterator it = folders.begin();
		for (; it != folders.end(); ++it)
			workingFolders.add(util::validPath(*it));
	}
}

void TApplication::addWorkingFolders(const util::TStringList& folders) {
	app::TLockGuard<app::TMutex> lock(pathMtx);
	if (!folders.empty()) {
		TStringList::const_iterator it = folders.begin();
		for (; it != folders.end(); ++it)
			workingFolders.add(util::validPath(*it));
	}
}

void TApplication::setWorkingFolder(const std::string folder) {
	app::TLockGuard<app::TMutex> lock(pathMtx);
	workingFolders.clear();
	if (!folder.empty()) {
		workingFolders.add(util::validPath(folder));
	}
}


template<typename value_t, typename... variadic_t>
void TApplication::logger(const ELogBase file, const std::string& text, const value_t value, variadic_t... args) {
	std::string s = util::csnprintf(text, value, std::forward<variadic_t>(args)...);
	logger(file, s);
}

template<typename value_t, typename... variadic_t>
void TApplication::logger(const std::string& text, const value_t value, variadic_t... args) {
	std::string s = util::csnprintf(text, value, std::forward<variadic_t>(args)...);
	logger(ELogBase::LOG_APP, s);
}

void TApplication::logger(const ELogBase file, const std::string& text) {
	PLogFile o = nil;

	// LOG_APP, LOG_EXCEPT, LOG_THREAD, LOG_TIMER, LOG_TASKS
	switch (file) {
		case ELogBase::LOG_APP:
			o = sysdat.obj.applicationLog;
			break;
		case ELogBase::LOG_EXCEPT:
			o = sysdat.obj.exceptionLog;
			break;
		case ELogBase::LOG_THREAD:
			o = sysdat.obj.threadLog;
			break;
		case ELogBase::LOG_SOCKET:
			o = sysdat.obj.socketLog;
			break;
		case ELogBase::LOG_TIMER:
			o = sysdat.obj.timerLog;
			break;
		case ELogBase::LOG_TASKS:
			o = sysdat.obj.tasksLog;
			break;
		case ELogBase::LOG_WEB:
			o = sysdat.obj.webLog;
			break;
		case ELogBase::LOG_DATABASE:
			o = sysdat.obj.datbaseLog;
			break;
	}

	if (util::assigned(o)) {
		o->write(text);
	} else {
		throw app_error("TApplication::Log : Invalid log file.");
	}
}


void TApplication::clogger(const ELogBase file, const std::string &fmt, ...) {
	if (fmt.empty())
		return;

	int n;
	std::string s;
	PLogFile o = nil;
	util::TStringBuffer buf;
	buf.reserve(fmt.size() * 10, false);
	buf.resize(fmt.size() * 3, false);
	va_list ap;

	// Format argument list
	while (true) {
		va_start(ap, fmt);
		n = vsnprintf(buf.data(), buf.size(), fmt.c_str(), ap);
		va_end(ap);

		if ((n > -1) && ((size_t)n < buf.size())) {
			s.assign(buf.data(), (size_t)n);
			break;
		}

		if ((size_t)n == buf.size()) buf.resize(n + 1, false);
		else buf.resize(buf.size() * 3, false);
	}

	// LOG_APP, LOG_EXCEPT, LOG_THREAD, LOG_TIMER, LOG_TASKS
	switch (file) {
		case ELogBase::LOG_APP:
			o = sysdat.obj.applicationLog;
			break;
		case ELogBase::LOG_EXCEPT:
			o = sysdat.obj.exceptionLog;
			break;
		case ELogBase::LOG_THREAD:
			o = sysdat.obj.threadLog;
			break;
		case ELogBase::LOG_SOCKET:
			o = sysdat.obj.socketLog;
			break;
		case ELogBase::LOG_TIMER:
			o = sysdat.obj.timerLog;
			break;
		case ELogBase::LOG_TASKS:
			o = sysdat.obj.tasksLog;
			break;
		case ELogBase::LOG_WEB:
			o = sysdat.obj.webLog;
			break;
		case ELogBase::LOG_DATABASE:
			o = sysdat.obj.datbaseLog;
			break;
	}

	if (util::assigned(o)) o->write(s);
	else throw app_error("TApplication::Log : Invalid log file.");
}

void TApplication::writeLog(const std::string& s, bool addLineFeed) {
	sysdat.obj.applicationLog->write(s, addLineFeed);
}

void TApplication::errorLog(const std::string& s, bool addLineFeed) {
	sysdat.obj.applicationLog->write("[Error] " + s, addLineFeed);
	sysdat.obj.exceptionLog->write("[Error] " + s, addLineFeed);
}

void TApplication::infoLog(const std::string& s, bool addLineFeed) {
	sysdat.obj.applicationLog->write("[Info] " + s, addLineFeed);
}

void TApplication::memoryLog(const std::string& location) {
	if (util::assigned(sysdat.obj.applicationLog)) {
		sysutil::TMemInfo sysmem;
		size_t mem = sysutil::getCurrentMemoryUsage();
		size_t max = sysutil::getPeakMemoryUsage();
		logger("[Heap] [%] Application: Current %, Peak %", location, util::sizeToStr(mem, 1), util::sizeToStr(max, 1));
		if (sysutil::getSystemMemory(sysmem)) {
			logger("[Heap] [%] System: Total %, Free %, Swap %, Swapped %, Shared %", location,
					util::sizeToStr(sysmem.memTotal, 1),
					util::sizeToStr(sysmem.memFree, 1),
					util::sizeToStr(sysmem.swapTotal, 1),
					util::sizeToStr(sysmem.swapTotal - sysmem.swapFree, 1),
					util::sizeToStr(sysmem.shmem, 1));
		}
		sysdat.obj.applicationLog->flush();
	}
}

void TApplication::writeStream(std::stringstream& sstrm) {
	sstrm.seekg(0, std::ios::end);
	if (sstrm.tellg() > 0) {
		if (!isDaemonized())
			std::cout << sstrm.str(); // << std::endl is included here by design!
		if (sysdat.log.verbosity > 1) {
			if (!sstrm.str().empty()) {
				if (sstrm.str().size() > 1 || sstrm.str()[0] != '\n') {
					writeLog("[Application] " + sstrm.str(), false);
				}
			}
		}
		line.copyfmt(std::ios(NULL));
		sstrm.str("");
#ifdef USE_BOOLALPHA
		sstrm << std::boolalpha;
#endif
	}
}

void TApplication::aquireConsole() {
	int err;
	struct termios settings;
	memset(&console, 0, sizeof(console));

	err = tcgetattr(STDIN_FILENO, &console);
	if (util::checkFailed(err))
		throw util::sys_error("TApplication::setConsole::tcgetattr() failed.", errno);

	memcpy(&settings, &console, sizeof(console));
	settings.c_lflag &= ~ICANON;
	settings.c_lflag &= ~ECHO;
	settings.c_cc[VMIN] = 0;
	settings.c_cc[VTIME] = 0;

	err = tcsetattr(STDIN_FILENO, TCSANOW, &settings);
	if (util::checkFailed(err))
		throw util::sys_error("TApplication::setConsole::tcsetattr() failed.", errno);

	inputModeSet = true;
}

void TApplication::restoreConsole() {
	if (inputModeSet) {
		int err;
		err = tcsetattr(0, TCSADRAIN, &console);
		if (util::checkFailed(err))
			throw util::sys_error("TApplication::restoreConsole::tcsetattr() failed.", errno);
		inputModeSet = false;
	}
}

char TApplication::getch(bool echo) {
	char retVal = NUL;
	char buffer = NUL;
	if (inputModeSet && !daemonized && (STDIN_FILENO >= 0)) {
		ssize_t r;
		do {
			r = ::read(STDIN_FILENO, &buffer, 1);
		} while (r == (ssize_t)EXIT_ERROR && errno == EINTR);

		if (r > (ssize_t)0) {
			retVal = buffer;
			if (echo && !daemonized)
				std::cout << retVal << std::flush;
		}
	}
	return retVal;
}


int TApplication::execute(TModule& module) {
	std::string sName = util::nameOf(module);
	bool ok = util::checkSucceeded(error);
	if (!terminated && ok) {

		// Prepare given module
		try {
			error = module.prepare();
			if (util::checkFailed(error)) {
				std::string sError = std::to_string((size_s)error);
				std::string sText = "Preparing module [" + sName + "] failed, Error code = " + sError;
				errorLog(sText);
			} else {
				writeLog("[Application] Prepared module <" + sName + ">");
			}
		} catch (const std::exception& e) {
			std::string sExcept = e.what();
			std::string sText = "Exception on unprepare of module [" + sName + "] \"" + sExcept + "\"";
			errorLog(sText);
			error = EXIT_FAILURE;
		} catch (...) {
			std::string sText = "Unknown exception on unprepare of module [" + sName + "]";
			errorLog(sText);
			error = EXIT_FAILURE;
		}
		if (!isDaemonized()) {
			if (!util::checkSucceeded(error)) {
				std::cout << app::red << "[" << executed << "] Preparing module <" << sName << "> failed." << app::reset << std::endl;
			}
		}

		// Execute given module when successfully prepared
		if (util::checkSucceeded(error)) {
			try {
				writeLog("[Application] Execute module <" + sName + ">");
				if (!isDaemonized())
					std::cout << app::green << "[" << executed << "] Execute module <" << sName << ">" << app::reset << std::endl;
				error = module.execute();
				if (util::checkFailed(error)) {
					std::string sError = std::to_string((size_s)error);
					std::string sText = "Module [" + sName + "] failed, Error code = " + sError;
					errorLog(sText);
				} else {
					writeLog("[Application] Executed module <" + sName + ">");
				}
			} catch (const std::exception& e) {
				std::string sExcept = e.what();
				std::string sText = "Exception in module [" + sName + "] \"" + sExcept + "\"";
				errorLog(sText);
				error = EXIT_FAILURE;
			} catch (...) {
				std::string sText = "Unknown exception in module [" + sName + "]";
				errorLog(sText);
				error = EXIT_FAILURE;
			}
			if (!isDaemonized()) {
				if (util::checkSucceeded(error)) {
					std::cout << app::magenta << "[" << executed << "] Executed module <" << sName << ">" << app::reset << std::endl;
				} else {
					std::cout << app::red << "[" << executed << "] Module <" << sName << "> failed." << app::reset << std::endl;
				}
			}
		}

		// Queue up module for cleanup
		++executed;
		modules.push_back(&module);

	} else {
		if (executed > 0) {
			std::string sText = "Module [" + sName + "] ";
			sText += ok ? "omitted by premature program termination." : "omitted by previous error.";
			errorLog(sText);
		}
	}
	return error;
}

void TApplication::unprepare() {
	// Execute "last will and testament" actions
	if (!modules.empty()) {
		for (ssize_t i = util::pred(modules.size()); i >= 0; --i) {
			TModule* module = modules[i];
			std::string sName = util::nameOf(*module);
			writeLog("[Application] Unrepare module <" + sName + ">");

			// Call unprepare method for given module
			try {
				module->unprepare();
			} catch (const std::exception& e) {
				std::string sExcept = e.what();
				std::string sText = "Exception on unprepare of module [" + sName + "] \n" + sExcept + "\n";
				errorLog(sText);
				if (error == EXIT_SUCCESS)
					error = EXIT_FAILURE;
			} catch (...) {
				std::string sText = "Unknown exception on unprepare of module [" + sName + "]";
				errorLog(sText);
				if (error == EXIT_SUCCESS)
					error = EXIT_FAILURE;
			}
		}
	}
}

void TApplication::release() {
	// Execute last minute cleanups for modules in reverse initialization order!
	if (!modules.empty()) {
		for (ssize_t i = util::pred(modules.size()); i >= 0; --i) {
			TModule* module = modules[i];
			std::string sName = util::nameOf(*module);
			writeLog("[Application] Cleanup module <" + sName + ">");

			if (!isDaemonized())
				std::cout << app::yellow << "[" << executed << "] Cleanup module <" << sName << ">" << app::reset << std::endl;

			// Call "last minute" cleanup method for given module
			try {
				module->cleanup();
			} catch (const std::exception& e) {
				std::string sExcept = e.what();
				std::string sText = "Exception on cleanup of module [" + sName + "] \n" + sExcept + "\n";
				errorLog(sText);
				if (error == EXIT_SUCCESS)
					error = EXIT_FAILURE;
			} catch (...) {
				std::string sText = "Unknown exception on cleanup of module [" + sName + "]";
				errorLog(sText);
				if (error == EXIT_SUCCESS)
					error = EXIT_FAILURE;
			}

			if (!isDaemonized())
				std::cout << app::cyan << "[" << executed << "] Terminated module <" << sName << ">" << app::reset << std::endl;
			--executed;
		}
	}
}


PWebServer TApplication::startWebServer(const std::string& name, const std::string& documentRoot, const bool autostart) {
	// Open config file for webserver(s)
	if (!util::assigned(sysdat.obj.webConfig)) {
		std::string configFile = sysdat.app.configFolder + "webserver.conf";
		sysdat.obj.webConfig = new app::TIniFile(configFile);
	}

	// Create and start a new webserver instance
	PWebServer web = new TWebServer(name, documentRoot, sysdat.obj.webConfig, sysdat.obj.threads, sysdat.obj.timers, sysdat.obj.webLog, sysdat.obj.exceptionLog);
	if (util::assigned(web)) {
		if (web->start(autostart)) {
			logger(app::ELogBase::LOG_APP, "[Application] Webserver <%> started.", name);
		} else {
			logger(app::ELogBase::LOG_APP, "[Application] Starting webserver <%> failed.", name);
		}
	}

	return web;
}


void TApplication::update() {
	sysdat.obj.logger->flush();
	if (util::assigned(sysdat.obj.webServer))
		sysdat.obj.webServer->update();
	deallocateHeapMemory();
	storage.saveToFile(sysdat.app.storeFile);
	notifySystemState(SYS_RELOAD);
}


void TApplication::deallocateHeapMemory() {
	app::TLockGuard<app::TMutex> lock(heapMtx, false);
	if (!lock.tryLock()) {
		return;
	}
	heapTime = util::now();
	util::TDateTime duration;
	duration.start();
	size_t peak = sysutil::getPeakMemoryUsage();
	size_t memory = sysutil::getCurrentMemoryUsage();
	size_t padding = (peak + memory) / 10;
	int r = malloc_trim(padding);
	if (r > 0) {
		std::string ts;
		util::TTimePart time = duration.stop(ETP_MICRON);
		if (time > 999) {
			ts = std::to_string((size_u)(time / 1000)) + " ms";
		} else {
			ts = std::to_string((size_u)time) + " us";
		}
		size_t current = sysutil::getCurrentMemoryUsage();
		size_t freed = (memory > current) ? memory - current : (size_t)0;
		writeLog("[Heap Memory Manager] Memory trimmed from " + util::sizeToStr(memory) + " to " + util::sizeToStr(current) + ", freed " + util::sizeToStr(freed) + " (" + ts + ")");
	}
}

util::TTimePart TApplication::getDeallocateTime() const {
	app::TLockGuard<app::TMutex> lock(heapMtx);
	return heapTime;
}

util::TTimePart TApplication::getHeapDelay() const {
	return sysdat.app.heapDelay;
}

void TApplication::stop() {
	// Stop webserver from responding
	if (util::assigned(sysdat.obj.webServer)) sysdat.obj.webServer->pause();

	// Terminate execution of asynchronous system objects
	if (util::assigned(sysdat.obj.timers))   sysdat.obj.timers->terminate();
	if (util::assigned(sysdat.obj.tasks))    sysdat.obj.tasks->terminate();
	if (util::assigned(sysdat.obj.timeouts)) sysdat.obj.timeouts->terminate();
}

void TApplication::halt() {
	// Release all MQTT subscriptions an stop sending messages
#ifdef USE_MQTT_CLIENT
	if (util::assigned(sysdat.obj.mqtt)) sysdat.obj.mqtt->halt();
#endif
#ifdef USE_GPIO_CONTROL
	if (util::assigned(sysdat.obj.gpio)) sysdat.obj.gpio->terminate();
#endif

	// Terminate execution of asynchronous system objects
	if (util::assigned(sysdat.obj.logger)) sysdat.obj.logger->halt();
}

void TApplication::inhibit() {
	if (!getAndSetStopped()) {
		stop();
	}
	if (!getAndSetUnprepared()) {
		unprepare();
	}
	if (!getAndSetHalted()) {
		halt();
	}
}

void TApplication::finalize() {
	terminated = true;
	eventsEnabled = false;
	notifySystemState(SYS_STOP);

	// Terminate execution of system objet lists
	inhibit();

	// Terminate asynchrous worker threads
	terminateSignalThread();
	terminateWatchThread();
	terminateUdevThread();
	terminateCommThread();

	// Call module cleanup methods
	release();

	// Disconnect MQTT client connection to message broker
#ifdef USE_MQTT_CLIENT
	if (util::assigned(sysdat.obj.mqtt)) sysdat.obj.mqtt->stop();
#endif

	// Terminate execution of system objects
	if (util::assigned(sysdat.obj.threads))   sysdat.obj.threads->terminate();
	if (util::assigned(sysdat.obj.sockets))   sysdat.obj.sockets->terminate();
	if (util::assigned(sysdat.obj.webServer)) sysdat.obj.webServer->terminate();

	// Wait for termination of system object lists
	if (util::assigned(sysdat.obj.tasks))    sysdat.obj.tasks->waitFor();
	if (util::assigned(sysdat.obj.timers))   sysdat.obj.timers->waitFor();
	if (util::assigned(sysdat.obj.timeouts)) sysdat.obj.timeouts->waitFor();
	if (util::assigned(sysdat.obj.threads))  sysdat.obj.threads->waitFor();
	if (util::assigned(sysdat.obj.sockets))  sysdat.obj.sockets->waitFor();

	// Wait for detached threads
	TIntermittentThread::waitFor();

	// Shutdown application services
#ifdef USE_MQTT_CLIENT
	if (util::assigned(sysdat.obj.mqtt))      sysdat.obj.mqtt->waitFor();
#endif
	if (util::assigned(sysdat.obj.webServer)) sysdat.obj.webServer->waitFor();
	if (util::assigned(sysdat.obj.serial))    sysdat.obj.serial->close();
	if (util::assigned(sysdat.obj.session))   sysdat.obj.session->close();

	// End of logging...
	infoLog("Application runtime : " + util::timeToHuman(uptime(), 0, ELocale::en_US));
	logger(app::ELogBase::LOG_APP,      "[Application] Close logfile <%> : % lines logged.",      sysdat.obj.applicationLog->getName(), sysdat.obj.applicationLog->getRowsLogged());
	logger(app::ELogBase::LOG_TASKS,    "[Application] Close logfile <%> : % lines logged.",      sysdat.obj.tasksLog->getName(),       sysdat.obj.tasksLog->getRowsLogged());
	logger(app::ELogBase::LOG_TIMER,    "[Application] Close logfile <%> : % lines logged.",      sysdat.obj.timerLog->getName(),       sysdat.obj.timerLog->getRowsLogged());
	logger(app::ELogBase::LOG_THREAD,   "[Application] Close logfile <%> : % lines logged.",      sysdat.obj.threadLog->getName(),      sysdat.obj.threadLog->getRowsLogged());
	logger(app::ELogBase::LOG_SOCKET,   "[Application] Close logfile <%> : % lines logged.",      sysdat.obj.socketLog->getName(),      sysdat.obj.socketLog->getRowsLogged());
	logger(app::ELogBase::LOG_WEB,      "[Application] Close logfile <%> : % lines logged.",      sysdat.obj.webLog->getName(),         sysdat.obj.webLog->getRowsLogged());
	logger(app::ELogBase::LOG_DATABASE, "[Application] Close logfile <%> : % lines logged.",      sysdat.obj.datbaseLog->getName(),     sysdat.obj.datbaseLog->getRowsLogged());
	logger(app::ELogBase::LOG_EXCEPT,   "[Application] Close logfile <%> : % exceptions logged.", sysdat.obj.exceptionLog->getName(),   sysdat.obj.exceptionLog->getRowsLogged() - 1);
	sysdat.obj.logger->flush();

	// Flush configuration files
	flushSystemSettings();

	// Check for change in license entries
	updateNamedLicenses();

	// Save configuration changes to disk
	commitApplicationSettings();

	// Save application storage
	storage.saveToFile(sysdat.app.storeFile);

	// Delete PID file
	cleanup();
}

void TApplication::flushApplicationSettings() {
	app::TLockGuard<app::TMutex> lock(configMtx);
	config->setSection(APP_CONFIG);

	// Write changed properties to config
	config->writeString("Hostname", sysdat.app.hostName);
	config->writeString("Description", sysdat.app.appDescription);
	config->writeString("Jumbotron", sysdat.app.appJumbotron);

	config->flush();
	changes = 0;
}

void TApplication::flushSystemSettings() {
	app::TLockGuard<app::TMutex> lock(configMtx);
	if (util::assigned(sysdat.obj.gpioConfig)) sysdat.obj.gpioConfig->flush();
	if (util::assigned(sysdat.obj.mqttConfig)) sysdat.obj.mqttConfig->flush();
	if (util::assigned(sysdat.obj.webConfig))  sysdat.obj.webConfig->flush();
}


void TApplication::commitApplicationSettings() {
	if (changes > 0) {
		flushApplicationSettings();
	}
}

bool TApplication::backupConfigurationFiles() {
	app::TLockGuard<app::TMutex> lock(configMtx);
	return backupConfigurationFilesWithNolock();
}

bool TApplication::backupConfigurationFilesWithNolock() {
	bool r = false;
	int retVal;
	std::string output;
	util::TStringList result;
	const std::string& fileName = getBackupFile();
	const std::string& folderName = getConfigFolder();
	util::TStringList content;
	util::readDirektoryContent(folderName, "*.conf", content);
	if (!content.empty()) {
		std::string files = content.asString(' ');
		std::string commandLine = util::csnprintf("tar -czf % -C % %", fileName, folderName, files);
		logger(app::ELogBase::LOG_APP, "[Application] Execute backup command $", commandLine);
		util::deleteFile(fileName);
		util::executeCommandLine(commandLine, output, retVal, false, 10);
		r = util::fileExists(fileName);
		if (r) {
			logger(app::ELogBase::LOG_APP, "[Application] Configuration file backup saved as <%>", fileName);
		} else {
			logger(app::ELogBase::LOG_APP, "[Application] [Error] Saving Configuration file backup as <%> failed.", fileName);
		}
	} else {
		logger(app::ELogBase::LOG_APP, "[Application] [Error] Configuration folder <%> is empty.", folderName);
	}
	return r;
}

void TApplication::parseCommandLine(int argc, char *argv[]) {
	int i;
	std::string k,s;
	std::string sqlite = "NOSQLITE";
#ifdef 	USE_SQLITE3
	sqlite = "/SQLITE" + std::to_string((size_u)SQLITE_VERSION_NUMBER);
#endif

	// Add application name to parameter list
	args.clear();
	args.add(".", argv[0]);

	// Lower case options expect parameters (":") to get added to list (apart from d,h,r and v used by application!)
	// Upper case are single options without parameters
	opterr = 0;
	while ((i = getopt(argc, argv, "a:b:c:de:f:g:hij:k:l:m:n:o:p:q:r:s:t:u:vwx:y:z:ABCDEFGHIJKLMNOPQRSTUVWXYZ")) != EOF)
	{
		// Add parameter + argument to internal list
		s.clear();
		if (assigned(optarg)) {
			s.assign(optarg);
			trim(s);
		}
		k = (char)i;
		args.add(k, s);

		switch (i)
		{
			case 'd':
				daemonize = true;
				break;

			case 'c': 
				if (fileExists(s)) {
					sysdat.app.configFile = s;
					std::cout << "Using configuration file <" << s << ">" << std::endl;
				} else {
					std::cout << "Configuration file not found <" << s << ">" << std::endl;
					exit(100);
				}	
				break;

			case 'v':
				sysdat.log.verbosity++;
				break;

			case 'w':
				webControl = true;
				break;

			case 'i':
				skipCheck = true;
				break;

			case 'h':
				std::cout << std::endl;
				std::cout << "Usage: " << fileBaseName(argv[0]) << " [-dcvrh <...>] " << getVersion() \
						<< " (GCC"   << GCC_VERSION \
						<< "/GLIBC"  << GLIBC_VERSION \
						<< "/MHD"    << TWebServer::mhdVersion() \
						<< sqlite \
						<< "/ZLIB"   << GZIP_VERSION << ")" << std::endl;
				std::cout << std::endl;
				std::cout << "\t-d ... Daemonize process" << std::endl;
				std::cout << "\t-c ... Configuration file (e.g. /etc/app.conf)" << std::endl;
				std::cout << "\t-v ... Increase verbosity on command line" << std::endl;
				std::cout << "\t-w ... Allow webserver pause and restart by SIGUSR1 and SIGUSR2" << std::endl;
				std::cout << "\t-i ... Ignore PID file for single instance check" << std::endl;
				std::cout << "\t-h ... Print this help" << std::endl;
				std::cout << std::endl;
				exit(EXIT_SUCCESS);
				break;

			default:
				// Iterate through all given parameters...
				break;
		}
	}
}


bool TApplication::writePidFile() {
	return util::writeFile(sysdat.app.pidFile, std::to_string((size_s)pid) + "\n");
}

void TApplication::deletePidFile() {
	if (!multiple)
		util::deleteFile(sysdat.app.pidFile);
}

long int TApplication::readPidFile() {
	long int pid = 0L;
	TStdioFile file;
	file.open(sysdat.app.pidFile, "r");
	if (file.isOpen()) {
		if (fscanf(file(), "%ld", &pid) == 1) {
			return pid;
		}
	}
	return pid;
}


bool TApplication::createPidFolder() {
	return createApplicationFolder(sysdat.app.pidFolder);
}

bool TApplication::createStoreFolder() {
	return createApplicationFolder(sysdat.app.storeFolder);
}

bool TApplication::createDataFolder() {
	return createApplicationFolder(sysdat.app.dataFolder);
}

bool TApplication::createApplicationFolder(const std::string folder) {
	bool r = true;
	if (!util::folderExists(folder)) {
		r = util::createDirektory(folder);
		if (r) {
			r = util::setFileOwner(folder, sysdat.app.userName);
		}
	}
	return r;
}


void TApplication::notifySystemState(const ESystemState state) {
#ifdef USE_SYSTEMD_NOTIFY
	switch (state) {
		case SYS_READY:
			sd_notifyf(0, "READY=1\n"
					"STATUS=%s started...\n"
					"MAINPID=%lu",
					sysdat.app.appDescription.c_str(),
					(unsigned long) pid);
			break;
		case SYS_RELOAD:
			sd_notifyf(0, "RELOADING=1\n"
					"STATUS=%s reloaded.\n",
					sysdat.app.appDescription.c_str());
			break;
		case SYS_STOP:
			sd_notifyf(0, "STOPPING=1\n"
					"STATUS=%s shutting down...s\n",
					sysdat.app.appDescription.c_str());
			break;
		default:
			break;
	}
#endif
}

void TApplication::installSignalHandler(int signal, TAppSignalHandler handler, struct sigaction * action, sigset_t * mask) {
	// Install signal handler
	action->sa_sigaction = nil;
	action->sa_handler = handler;
	action->sa_flags = SA_RESTART;
	installSignalDispatcher(signal, action, mask);
}

void TApplication::installSignalAction(int signal, TAppActionHandler handler, struct sigaction * action, sigset_t * mask) {
	// Install signal action
	action->sa_handler = nil;
	action->sa_sigaction = handler;
	action->sa_flags = SA_RESTART | SA_SIGINFO;
	installSignalDispatcher(signal, action, mask);
}

void TApplication::installSignalDispatcher(int signal, struct sigaction * action, sigset_t * mask) {
	int errnum;
	std::string name = sysutil::getSignalName(signal);

	if (::sigaction(signal, action, NULL) < 0)
		throw util::app_error("TApplication::installSignalDispatcher: Could not register <" + name + "> handler");

	errnum = sigaddset(mask, signal);
	if (util::checkFailed(errnum))
		throw util::app_error("TApplication::installSignalDispatcher: sigaddset() for <" + name + "> failed.");
}


void TApplication::installSignalHandlers() {
	struct sigaction action;
	sigset_t systemMask, signalMask;
	int errnum;

	errnum = sigfillset(&signalMask);
	if (util::checkFailed(errnum))
		throw util::sys_error("TApplication::installSignalHandlers: sigfillset failed", errnum);

	errnum = pthread_sigmask(SIG_SETMASK, &signalMask, &systemMask);
	if (util::checkFailed(errnum))
		throw util::sys_error("TApplication::installSignalHandlers: pthread_sigmask failed", errnum);

	errnum = sigemptyset(&signalMask);
	if (util::checkFailed(errnum))
		throw util::sys_error("TApplication::installSignalHandlers: sigemptyset failed", errnum);

	/* Don't block signals in signal handler --> use sigemptyset! */
	memset(&action, 0, sizeof(action));
	errnum = sigemptyset(&action.sa_mask);
	if (util::checkFailed(errnum))
		throw util::sys_error("TApplication::installSignalHandlers: sigemptyset failed", errnum);

	// POSIX.1-1990 disallowed setting the action for SIGCHLD to SIG_IGN.
	// POSIX.1-2001 allows this possibility, so that ignoring SIGCHLD can be
	// used to prevent the creation of zombies (see wait(2)).  Nevertheless,
	// the historical BSD and System V behaviors for ignoring SIGCHLD
	// differ, so that the only completely portable method of ensuring that
	// terminated children do not become zombies is to catch the SIGCHLD
	// signal and perform a wait(2) or similar.
	installSignalHandler(SIGCHLD, sigChildHandler, &action, &signalMask);

	// Handle SIGALRM from timers
	installSignalHandler(SIGALRM, sigAlarmHandler, &action, &signalMask);

	// Handle SIGPIPE from sockets
	installSignalHandler(SIGPIPE, sigPipeHandler, &action, &signalMask);

	// Handle SIGSEGV segmentation faults
	installSignalAction(SIGSEGV, sigExceptionHandler, &action, &signalMask);

	// Handle SIGABRT and SIGIOT (both are synonyms for signal 6) unexpected program termination
	installSignalAction(SIGABRT, sigExceptionHandler, &action, &signalMask);

	// Handle SIGBUS shared memory access errors
	installSignalAction(SIGBUS, sigExceptionHandler, &action, &signalMask);

	// Handle SIGFPE division by zero exceptions
	installSignalAction(SIGFPE, sigExceptionHandler, &action, &signalMask);

	// Handle SIGILL illegal instructions
	installSignalAction(SIGILL, sigExceptionHandler, &action, &signalMask);

	// If a blocked call to an IO operation is interrupted
	// by a signal handler, then the call will be automatically restarted
	// after the signal handler returns if the SA_RESTART flag was used;
	// otherwise the call will fail with the error EINTR.
	// Here SIGINT is changed and program waits for termination of all
	// asynchronous operations, so better restart all io ops.
	// unnecessary to use SA_SIGINFO here, since message is not send
	// by sigqueue with sigval.sival_ptr set, but to for type
	// compatibility (3 parameters of event handler) it is used here
	action.sa_sigaction = nil;
	action.sa_handler = signalDispatcher;
	action.sa_flags = SA_RESTART;

	installSignalDispatcher(SIGINT,  &action, &signalMask);
	installSignalDispatcher(SIGTERM, &action, &signalMask);
	installSignalDispatcher(SIGHUP,  &action, &signalMask);
	installSignalDispatcher(SIGUSR1, &action, &signalMask);
	installSignalDispatcher(SIGUSR2, &action, &signalMask);

	errnum = pthread_sigmask(SIG_UNBLOCK, &signalMask, NULL);
	if (util::checkFailed(errnum))
		throw util::sys_error("TApplication::installSignalHandlers: pthread_sigmask failed", errnum);

	errnum = pthread_sigmask(SIG_SETMASK, &systemMask, NULL);
	if (util::checkFailed(errnum))
		throw util::sys_error("TApplication::installSignalHandlers: pthread_sigmask failed", errnum);
}


// Wrapper to call event handler property of TApplication
inline void TApplication::execSignalHandler(const TEventHandler& handler, int signal) {
	if (handler != nil) {
		try {
			handler();
		} catch (const std::exception& e) {
			std::string sName = sysutil::getSignalName(signal);
			std::string sExcept = e.what();
			std::string sText = "Exception in TApplication::execSignalHandler(" + sName + ") " + sExcept;
			errorLog(sText);
		} catch (...) {
			std::string sName = sysutil::getSignalName(signal);
			std::string sText = "Unknown exception in TApplication::execSignalHandler(" + sName + ")";
			errorLog(sText);
		}
	}
}

void TApplication::signalHandler(int signal) {
	switch (signal) {
		case SIGINT:
			suicide();
			if (signalThreadRunning) ++cntSigint;
			break;

		case SIGTERM:
			suicide();
			if (signalThreadRunning) ++cntSigterm;
			break;

		case SIGKILL:
			// Never ends up here by kernel design...
			break;

		case SIGHUP:
			if (signalThreadRunning) ++cntSighup;
			break;

		case SIGUSR1:
			if (signalThreadRunning) ++cntSigusr1;
			break;

		case SIGUSR2:
			if (signalThreadRunning) ++cntSigusr2;
			break;

		default:
			if (signalThreadRunning) ++cntSigdef;
			break;
	}
	if (signalThreadRunning) {
		this->signal.post();
	}
}

void TApplication::suicide() {
	// Force instant terminate on second try!
	if (terminating) {
		restoreConsole();
		deletePidFile();
		std::terminate();
	}
	terminating = true;
}

int TApplication::signalThreadHandler() {
	signalThreadStarted = signalThreadRunning = true;
	writeLog("[Application] Signal handler thread started.");
	while (signal.wait()) {
		if (!signalThreadMethod())
			break;
	}
	signalThreadRunning = false;
	writeLog("[Application] Signal handler thread terminated.");
	return EXIT_SUCCESS;
}


bool TApplication::signalThreadMethod() {
	bool bTerminate = false;
	try {
		// Serialize callbacks for given signals
		while (!terminated && (cntSigint || cntSigterm || cntSighup || cntSigusr1 || cntSigusr2 || cntSigdef)) {
			if (cntSigint) {
				--cntSigint;
				bTerminate = true;
				execSignalHandler(onSigint, SIGINT);
			}
			if (cntSigterm) {
				--cntSigterm;
				bTerminate = true;
				execSignalHandler(onSigterm, SIGTERM);
			}
			if (cntSighup) {
				--cntSighup;
				execSignalHandler(onSighup, SIGHUP);
				update();
			}
			if (cntSigusr1) {
				--cntSigusr1;
				if (webControl) {
					if (util::assigned(sysdat.obj.webServer)) {
						if (sysdat.obj.webServer->pause())
							logger(app::ELogBase::LOG_APP, "[Application] Webserver set to paused state.");
					}
				} else {
					execSignalHandler(onSigusr1, SIGUSR1);
				}
			}
			if (cntSigusr2) {
				--cntSigusr2;
				if (webControl) {
					if (util::assigned(sysdat.obj.webServer)) {
						if (sysdat.obj.webServer->resume())
							logger(app::ELogBase::LOG_APP, "[Application] Webserver resumed normal operation.");
					}
				} else {
					execSignalHandler(onSigusr2, SIGUSR2);
				}
			}
			if (cntSigdef) {
				--cntSigdef;
				execSignalHandler(onSigdefault, SIGSYS);
			}
		}
	} catch (const std::exception& e) {
		std::string sExcept = e.what();
		std::string sText = "Exception in TApplication::signalThreadMethod() " + sExcept;
		errorLog(sText);
	} catch (...) {
		std::string sText = "Unknown exception in TApplication::signalThreadMethod()";
		errorLog(sText);
	}

	// Application terminates now, immidiately stop all running tasks (timers, webserver, timeouts, etc.)
	// Otherwise deferred event callbacks might use objects that were just getting released!
	if (bTerminate) {
		terminate();
	}

	return !terminated;
}


int TApplication::watchThreadHandler() {
	watchThreadStarted = watchThreadRunning = true;
	writeLog("[Application] Watch event thread started.");
	while (true) {
		if (!watchThreadMethod())
			break;
	}
	terminate();
	watchThreadRunning = false;
	writeLog("[Application] Watch event thread terminated.");
	return EXIT_SUCCESS;
}


bool TApplication::watchThreadMethod() {
	bool exit = false;
	try {
		util::TStringList files;
		watch.wait();
		if (!terminated) {
			TEventResult r = watch.read(files);
			if (EV_SIGNALED == r && !files.empty()) {
				for (size_t i=0; i<files.size(); ++i) {
					const std::string& file = files[i];
					if (!watches.empty()) {
						bool proceed = true;
#ifdef STL_HAS_RANGE_FOR
						for (TWatchNotifyHandler method : watches) {
							try {
								method(file, proceed);
								if (!proceed)
									break;
							} catch (const std::exception& e) {
								std::string sExcept = e.what();
								std::string sText = "Exception in TApplication::watchThreadMethod::event() " + sExcept;
								errorLog(sText);
							} catch (...) {
								std::string sText = "Unknown exception in TApplication::watchThreadMethod::event()";
								errorLog(sText);
							}
						}
#else
						TWatchNotifyHandler method;
						for (size_t j=0; j<watches.size(); j++) {
							method = watches[j];
							try {
								method(file, proceed);
								if (!proceed)
									break;
							} catch (const std::exception& e) {
								std::string sExcept = e.what();
								std::string sText = "Exception in TApplication::watchThreadMethod::event() " + sExcept;
								errorLog(sText);
							} catch (...) {
								std::string sText = "Unknown exception in TApplication::watchThreadMethod::event()";
								errorLog(sText);
							}
						}
#endif
					}
				}
			}
			if (EV_TERMINATE == r)
				exit = true;
		}
	} catch (const std::exception& e) {
		std::string sExcept = e.what();
		std::string sText = "Exception in TApplication::watchThreadMethod() " + sExcept;
		errorLog(sText);
	} catch (...) {
		std::string sText = "Unknown exception in TApplication::watchThreadMethod()";
		errorLog(sText);
	}
	return !(terminated || exit);
}


int TApplication::udevThreadHandler() {
	udevThreadStarted = udevThreadRunning = true;
	writeLog("[Application] UDEV monitoring thread started.");
	while (true) {
		if (!udevThreadMethod())
			break;
	}
	terminate();
	udevThreadRunning = false;
	writeLog("[Application] UDEV monitoring thread terminated.");
	return EXIT_SUCCESS;
}


bool TApplication::udevThreadMethod() {
	bool exit = false;
	try {
		THotplugEvent event;
		TEventResult ev = hotplug.wait(event, 0);
		if (!terminated) {
			switch (ev) {
				case EV_SIGNALED:
					if (event.isAssigned()) {
						if (event.getProduct().empty()) {
							writeLog("[Hotplug] Event [" + event.getAction() + "][" + event.getPath() + "]");
						} else {
							writeLog("[Hotplug] Event [" + event.getAction() + "][" + event.getProduct() + "][" + event.getPath() + "]");
						}
						if (onHotplug != nil) {
							try {
								onHotplug(event, event.getEvent());
							} catch (const std::exception& e) {
								std::string sExcept = e.what();
								std::string sText = "Exception in TApplication::udevThreadMethod::event() " + sExcept;
								errorLog(sText);
							} catch (...) {
								std::string sText = "Unknown exception in TApplication::udevThreadMethod::event()";
								errorLog(sText);
							}
						}
					}
					break;
				case EV_TERMINATE:
					exit = true;
					break;
				default:
					break;
			}
		}
	} catch (const std::exception& e) {
		std::string sExcept = e.what();
		std::string sText = "Exception in TApplication::udevThreadMethod() " + sExcept;
		errorLog(sText);
	} catch (...) {
		std::string sText = "Unknown exception in TApplication::udevThreadMethod()";
		errorLog(sText);
	}
	return !(terminated || exit);
}


int TApplication::commThreadHandler() {
	commThreadStarted = commThreadRunning = true;
	writeLog("[Application] Serial communication thread started.");
	while (true) {
		if (!commThreadMethod())
			break;
	}
	terminate();
	commThreadRunning = false;
	writeLog("[Application] Serial communication thread terminated.");
	return EXIT_SUCCESS;
}


bool TApplication::commThreadMethod() {
	bool exit = false;
	try {
		TByteBuffer data;
		TEventResult ev = sysdat.obj.serial->wait(data, sysdat.serial.chunk, 0);
		if (!terminated) {
			switch (ev) {
				case EV_SIGNALED:
					if (!data.empty()) {
						writeLog(util::csnprintf("[Terminal] % of % Bytes received [%]", data.size(), sysdat.serial.chunk, util::TBinaryConvert::binToAsciiA(data(), data.size(), true)));
						if (onTerminal != nil) {
							try {
								onTerminal(*sysdat.obj.serial, data);
							} catch (const std::exception& e) {
								std::string sExcept = e.what();
								std::string sText = "Exception in TApplication::commThreadMethod::event() " + sExcept;
								errorLog(sText);
							} catch (...) {
								std::string sText = "Unknown exception in TApplication::commThreadMethod::event()";
								errorLog(sText);
							}
						}
					}
					break;
				case EV_CLOSED:
					util::wait(500);
					break;
				case EV_CHANGED:
					util::wait(250);
					break;
				case EV_TERMINATE:
					exit = true;
					break;
				default:
					break;
			}
		}
	} catch (const std::exception& e) {
		std::string sExcept = e.what();
		std::string sText = "Exception in TApplication::commThreadMethod() " + sExcept;
		errorLog(sText);
	} catch (...) {
		std::string sText = "Unknown exception in TApplication::commThreadMethod()";
		errorLog(sText);
	}
	return !(terminated || exit);
}


void TApplication::writeDebugFile(const std::string& fileName, const std::string& text) {
	if (!text.empty()) {
		util::TStdioFile file;
		file.open(fileName, "a");
		if (file.isOpen()) {
			file.write(text + "\n");
		}
	}
}

void TApplication::daemonizer(const std::string& runAsUser, const std::string& runAsGroup,
		const app::TStringVector& supplementalGroups, const app::TStringVector& capabilityList,
		std::string& groupNames, std::string& capabilityNames, bool& userChanged) {
	// The daemon startup code was taken in parts from the Linux Daemon Writing HOWTO
	// Written by Devin Watson <dmwatson@comcast.net>
	// The HOWTO is Copyright by Devin Watson, under the terms of the BSD License.
	// See http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html
	uid_t uid;
	gid_t gid;
	int errnum;
	userChanged = false;
	capabilityNames.clear();
	groupNames.clear();

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0)
		throw app_error("TApplication::daemonizer::fork() failed.");

	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/*
	 * Change the file mode mask:
	 * Octal value : Permission
	 *   0 : read, write and execute
	 *   1 : read and write
	 *   2 : read and execute
	 *   3 : read only
	 *   4 : write and execute
	 *   5 : write only
	 *   6 : execute only
	 *   7 : no permissions
	 */
	umask(022);

	/* Create a new SID for the child process */
	pid_t sid = setsid();
	if (sid < 0)
		throw app_error("TApplication::daemonizer::setsid() failed.");

	/* Change the current working directory */
	const char* pwd = sysdat.app.setTempDir ? sysdat.app.tmpFolder.c_str() : sysdat.app.currentFolder.c_str();
	if (chdir(pwd) < 0)
		throw app_error("TApplication::daemonizer::chdir() Failed to set current directory <" + std::string(pwd) + ">");

	/* Redirect C++ stdin, stdout, and stderr streams to /dev/null */
	FILE* file UNUSED;
	file = freopen("/dev/null", "r", stdin);
	file = freopen("/dev/null", "a", stdout);
	file = freopen("/dev/null", "a", stderr);

	/* Get new PID (after spawn) */
	pid = getpid();
	tid = TThreadUtil::gettid();

	/* Change user if started as root */
	if (!runAsUser.empty()) {
		uid = getuid();
		if (uid == 0) {
			if (!sysutil::getUserID(runAsUser, uid, gid))
				throw app_error("TApplication::daemonizer::getUserID() Failed to get UID for <" + runAsUser + ">");

			if (!sysutil::getGroupID(runAsGroup, gid))
				throw app_error("TApplication::daemonizer::getGroupID() Failed to get UID for <" + runAsGroup + ">");

		    // Avoid changing from root to root...
			if (uid != 0) {

				// Inherit capability flags when changing user
				cap_t caps;
				cap_value_t capabilities[util::succ(capabilityList.size())];
				size_t count = 0;

				if (!capabilityList.empty()) {

					// Add named capabilities as capability values to array
					for (size_t i=0; i<capabilityList.size(); ++i) {
						const std::string& cname = capabilityList[i];
						cap_value_t cap = getCapabilityByName(cname);
						if (INVALID_CAP_VALUE != cap) {
							capabilities[count] = cap;
							++count;
							capabilityNames += capabilityNames.empty() ? cname : "," + cname;
						}
					}

					// Set permitted capabilities for spawned process
					if (count > 0) {
						caps = cap_get_proc();
						if (util::assigned(caps)) {
							errnum = cap_set_flag(caps, CAP_PERMITTED, count, capabilities, CAP_SET);
							if (util::checkFailed(errnum))
								throw sys_error("TApplication::daemonizer::cap_set_flag(1) failed.", errnum);
							errnum = cap_set_proc(caps);
							if (util::checkFailed(errnum))
								throw sys_error("TApplication::daemonizer::cap_set_proc(1) failed.", errnum);
							errnum = prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
							if (errnum < 0)
								throw sys_error("TApplication::daemonizer::prctl(1) failed.", errnum);
							errnum = cap_free(caps);
							if (util::checkFailed(errnum))
								throw sys_error("TApplication::daemonizer::cap_free(1) failed.", errnum);
						} else {
							throw sys_error("TApplication::daemonizer::cap_get_proc(1) failed.");
						}
					}
				}

				// Set supplemental groups
				if (!supplementalGroups.empty()) {
					gid_t sgid;
					gid_t sgroups[util::succ(supplementalGroups.size())];
					size_t scount = 0;

					// Add named groups as supplemental groups to list
					for (size_t i=0; i<supplementalGroups.size(); ++i) {
						const std::string& gname = supplementalGroups[i];
						if (runAsGroup != gname) {
							if (sysutil::getGroupID(gname, sgid)) {
								if (sgid != gid) {
									sgroups[scount] = sgid;
									scount++;
									groupNames += groupNames.empty() ? gname : "," + gname;
								}
							}
						}
					}

					// Attach supplemental group list to current process
					if (scount > 0) {
						errnum = setgroups(scount, sgroups);
						if (errnum < EXIT_SUCCESS) {
							throw sys_error("TApplication::daemonizer::setgroups() failed.");
						}
					}
				}

				// Change user and group ID
				errnum = setgid(gid);
				if (util::checkFailed(errnum))
					throw sys_error("TApplication::daemonizer::setgid() failed for GID <" + std::to_string((size_u)gid) + ">", errnum);
				errnum = setuid(uid);
				if (util::checkFailed(errnum))
					throw sys_error("TApplication::daemonizer::setuid() failed for UID <" + std::to_string((size_u)uid) + ">", errnum);
				userChanged = true;

				// Reestablish needed effective capabilities
				if (count > 0) {
					caps = cap_get_proc();
					if (util::assigned(caps)) {
						errnum = cap_set_flag(caps, CAP_EFFECTIVE, count, capabilities, CAP_SET);
						if (util::checkFailed(errnum))
							throw sys_error("TApplication::daemonizer::cap_set_flag(2) failed.", errnum);
						errnum = cap_set_proc(caps);
						if (util::checkFailed(errnum))
							throw sys_error("TApplication::daemonizer::cap_set_proc(2) failed.", errnum);
						errnum = cap_free(caps);
						if (util::checkFailed(errnum))
							throw sys_error("TApplication::daemonizer::cap_free(2) failed.", errnum);
					} else {
						throw sys_error("TApplication::daemonizer::cap_get_proc(2) failed.");
					}
				}
			}
		}
	}

	daemonized = true;
}

bool TApplication::shutdown() {
	app::TLockGuard<app::TMutex> lock(shutdownMtx);
	if (!rebooting) {
		//
		// Execute shutdown by command line
		// Edit "sudoers" with "sudo visudo" with the following content:
		//
		//  # Cmnd alias specification
		//  Cmnd_Alias SYSTEM = /sbin/shutdown, /sbin/halt, /sbin/reboot, /sbin/poweroff
		//
		//  # User privilege specification
		//  root    ALL=(ALL:ALL) ALL
		//  pi      ALL=(root) NOPASSWD: SYSTEM
		//
		int retVal;
		std::string output;
		util::TStringList result;
		std::string commandLine = "sudo poweroff -p";
		if (util::executeCommandLine(commandLine, output, retVal, false, 10) ) {
			rebooting = true;
		}
	}
	return rebooting;
}

bool TApplication::reboot() {
	app::TLockGuard<app::TMutex> lock(shutdownMtx);
	if (!rebooting) {
		//
		// Execute reboot by command line
		// Edit "sudoers" with "sudo visudo" with the following content:
		//
		//  # Cmnd alias specification
		//  Cmnd_Alias SYSTEM = /sbin/shutdown, /sbin/halt, /sbin/reboot, /sbin/poweroff
		//
		//  # User privilege specification
		//  root    ALL=(ALL:ALL) ALL
		//  pi      ALL=(root) NOPASSWD: SYSTEM
		//
		int retVal;
		std::string output;
		util::TStringList result;
		std::string commandLine = "sudo reboot --reboot";
		if (util::executeCommandLine(commandLine, output, retVal, false, 10) ) {
			rebooting = true;
		}
	}
	return rebooting;
}

void TApplication::onAcceptSocket(const std::string& addr, bool& accept) {
	if (rebooting || terminated || terminating) {
		accept = false;
	}
}

} /* namespace app */
