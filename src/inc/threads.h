/*
 * threads.h
 *
 *  Created on: 19.09.2014
 *      Author: Dirk Brinkmeier
 */

/*
 *				 Workflow of thread data processing
 *				 ==================================
 *
 * 		 Process					 			   Thread
 * 			|										  |
 * 			|-------->	      start()				  | Create thread via pthread_create()
 * 			|			threadExecMethod()	 -------->| and call method via threadDispatcher()
 * 			|										  | and transfer given data to thread execution loop
 * 			|				   do {}		 -------->|
 * 			|										  | Wait for condition signaled by
 * 			|				waitForSync()	 -------->| main process via calling sync functions
 * 			|										  | and wait for thread mutex free
 * 			|			  Wait for data...	 <--------|
 * 			|										  |
 * 			|-------->     synchronize()	 		  |
 * 			|			threadSyncMethod()	 -------->| Called by synchronize()
 * 			|										  | Set threads data structures.
 * 			|										  |
 * 			|			   Work on data		 <--------| Do threaded job....
 * 			|			   ============				  |
 * 			|										  |
 * 			|										  | Thread processing finished.
 * 			|			   sendMessage()	 <--------| Called by signalExecuter()
 * 			|<-------- threadMessageMethod()		  | of message thread.
 * 			|										  |
 * 			|		    while (!isTerminate) -------->| Repeat until terminate...
 * 			|										  |
 * 			|-------->		 waitFor()				  | Calls pthread_join()
 * 			|										  |
 *
 */

#ifndef THREADS_H_
#define THREADS_H_

#include "threadtypes.h"
#include "semaphore.h"
#include "datetime.h"
#include "classes.h"
#include "logger.h"
#include "gzip.h"
#include <signal.h>

#define SIGRTBTHREAD (SIGRTMIN + 3)
#define THREAD_STACK_SIZE (PTHREAD_STACK_MIN + 0x4000)
#define THREAD_DEFAULT_STACK 0
#define THREAD_DEFAULT_PRIO 0


namespace app {


struct TWebDataReceiver;

#ifdef STL_HAS_TEMPLATE_ALIAS

using TWebDataThread = TDataThread<app::TWebDataReceiver>;

using PWebDataThread = TWebDataThread*;
using PWebDataReceiver = TWebDataReceiver*;
using PThreadData = TThreadData*;

using TWebReceiverList = std::vector<app::PWebDataReceiver>;

#else

typedef TDataThread<app::TWebDataReceiver> TWebDataThread;

typedef TWebDataThread* PWebDataThread;
typedef TWebDataReceiver* PWebDataReceiver;
typedef TThreadData* PThreadData;

typedef std::vector<app::PWebDataReceiver> TWebReceiverList;

#endif


typedef struct CThreadMessage {
	int id;
	PBaseThread sender;
	void *data;
	EThreadMessageType msg;
	EMessageTransferType type;
	bool executed;
} TThreadMessage;

typedef struct CThreadCreator {
	PBaseThread sender;
	void *data;
} TThreadCreator;

typedef struct CThreadProperties {
	std::string name;
	pid_t process;
	std::mutex* mtx;
	PThreadController owner;
	PLogFile infoLog;
	PLogFile exceptionLog;
} TThreadProperties;


#ifdef STL_HAS_TEMPLATE_ALIAS

using PThreadMessage = TThreadMessage*;
using PThreadCreator = TThreadCreator*;
using PThreadProperties = TThreadProperties*;

#else

typedef TThreadMessage* PThreadMessage;
typedef TThreadCreator* PThreadCreator;
typedef TThreadProperties* PThreadProperties;

#endif


class TMessageSlot : public TObject {
private:
	mutable std::mutex mtx;
	std::vector<app::PThreadMessage> msgList;

public:
	PThreadMessage enqueue();
	PThreadMessage find(int id);
	void remove(int id);
	void release(PThreadMessage msg);
	bool empty() const ;
	void clear();

	TMessageSlot();
	virtual ~TMessageSlot();
};


class TThreadAffinity {
private:
	cpu_set_t* cpuset;
	size_t cpusize;
	size_t numa;
	void initCpuMask();
	void releaseCpuMask();

public:
	ssize_t setAffinity(size_t cpu = 0);
	ssize_t setAffinity(size_t cpu, pid_t tid);
	size_t getCoreCount() const { return numa; };

	TThreadAffinity();
	virtual ~TThreadAffinity();
};


class TThreadUtil {
protected:
	static bool createObjectThread(pthread_t& thread, TThreadHandler handler, EThreadType type, void *object,
			int priority = THREAD_DEFAULT_PRIO, size_t stack = THREAD_DEFAULT_STACK);

public:
	template<typename class_t>
		static bool createThread(pthread_t& thread, TThreadHandler handler, EThreadType type, class_t &&owner,
			int priority = THREAD_DEFAULT_PRIO, size_t stack = THREAD_DEFAULT_STACK) {
			return createObjectThread(thread, handler, type, (void*)owner, priority, stack);
		}
	template<typename class_t>
		static bool createJoinableThread(pthread_t& thread, TThreadHandler handler, class_t &&owner,
			int priority = THREAD_DEFAULT_PRIO, size_t stack = THREAD_DEFAULT_STACK) {
			return createObjectThread(thread, handler, THD_CREATE_JOINABLE, (void*)owner, priority, stack);
		}
	template<typename class_t>
		static bool createDetachedThread(TThreadHandler handler, class_t &&owner,
			int priority = THREAD_DEFAULT_PRIO, size_t stack = THREAD_DEFAULT_STACK) {
			pthread_t thread = 0;
			return createObjectThread(thread, handler, THD_CREATE_DETACHED, (void*)owner, priority, stack);
		}

	static int terminateThread(pthread_t& thread);
	static pid_t gettid();

	TThreadUtil();
	virtual ~TThreadUtil();
};


class TBaseThread : public TObject, private TThreadUtil {
friend class TThreadController;
private:
	bool createIntermittentObjectThread(void *object);
	bool createPersistentObjectThread(void *object);

protected:
	mutable std::mutex* globalMtx; // Protect thread message calls
	mutable std::mutex statusMtx;  // Protect thread properties

	PThreadController owner;
	PLogFile infoLog;
	PLogFile exceptionLog;
	pid_t process;

	pthread_t thread;
	bool started;
	bool terminated;
	bool terminate;
	bool debug;

	template<typename class_t>
		bool createPersistentThread(class_t &&owner) {
			return createPersistentObjectThread((void*)owner);
		}
	template<typename class_t>
		bool createIntermittentThread(class_t &&owner) {
			return createIntermittentObjectThread((void*)owner);
		}
	int terminatePersistentThread();

	pid_t gettid();
	int checkProperties();

	void setStarted(const bool value);

	virtual int run(void* creator) = 0;
	virtual void start(void* data, size_t cpu) = 0;
	virtual void prepare(void* data, size_t cpu) = 0;
	virtual int receive(void* message) = 0;
	virtual void unlockThreadSync() = 0;
	virtual void waitFor() = 0;

	TBaseThread();

public:
	int receiver(void* message);
	int dispatcher(void* data);

	void setProperties(const TThreadProperties thread);
	pthread_t getthd() const { return thread; };

	bool isTerminating() const;
	void setTerminate(bool terminate);
	bool isTerminated() const;
	void setTerminated(bool terminated);
	bool isStarted() const;

	void writeLog(const std::string& s);
	void debugLog(const std::string& s);
	void errorLog(const std::string& s);

	void wait();
	void unlock();

	virtual ~TBaseThread();
};


class TManagedThread final : public TBaseThread, protected TThreadAffinity {
friend class TThreadController;
private:
	mutable app::TMutex execMtx;
	TThreadCreator creator;
	TMessageSlot msgSlot;
	PSemaphore sema;
	pid_t tid;
	size_t affinity;
	bool executed;
	
	TThreadExecMethod threadExecMethod;
	TThreadMessageMethod threadMessageMethod;
	
	void initialize();
	
	void createBaseThread(PThreadCreator creator);
	void sendThreadMessage(EThreadMessageType message, void *data);
	void postThreadMessage(EThreadMessageType message, void *data);

	void prepare(void* data, size_t cpu = app::nsizet) override;
	void start(void* data, size_t cpu = app::nsizet) override;
	int run(void* creator) override;
	int receive(void* message) override;
	void unlockThreadSync() override;
	void waitFor() override;

	bool hasMessageHandler() const;
	bool hasExecHandler() const;

public:
	pid_t gettid() { return tid; };
	
	void execute();
	bool isExecuted() const;
	
	void sendSignal(const TThreadMessage& message);
	void receiveSignal(PThreadMessage message);
	void createSignalThread(PThreadMessage message);

	// Synchronize data from thread to main process and wait for processing
	void sendMessage(EThreadMessageType message);

	// Post data from thread to main process and return immediately
	void postMessage(EThreadMessageType message);

	// Same as above witch some parameters
	template<typename class_t>
		void sendMessage(EThreadMessageType message, class_t &&data) {
			sendThreadMessage(message, (void*)data);
		}

	// Same as above witch some parameters
	template<typename class_t>
		void postMessage(EThreadMessageType message, class_t &&data) {
			postThreadMessage(message, (void*)data);
		}

	template<typename message_t, typename class_t>
		inline void bindMessageHandler(message_t &&onThreadMessage, class_t &&owner) {
			this->threadMessageMethod = std::bind(onThreadMessage, owner,
						std::placeholders::_1, std::placeholders::_2);
		}

	template<typename exec_t, typename class_t>
		inline void bindExecHandler(exec_t &&threadExecMethod, class_t &&owner) {
			this->threadExecMethod = std::bind(threadExecMethod, owner, std::placeholders::_1);
		}

	explicit TManagedThread(const std::string& name);
	TManagedThread(const std::string& name, const TThreadProperties thread);
	TManagedThread(const std::string& name, std::mutex& mtx, pid_t process, TThreadController& owner, TLogFile& infoLog, TLogFile& exceptionLog);

	// No copy and move constructors
	TManagedThread(TManagedThread&) = delete;
	TManagedThread(const TManagedThread&) = delete;
	TManagedThread(const TManagedThread&&) = delete;

	virtual ~TManagedThread();
};



class TThreadController : public TObject {
private:
	TThreadList threadList;
	std::mutex mtx;
	app::TLogFile *infoLog;
	app::TLogFile *exceptionLog;

	void clear();
	void initialize();
	void addSignalHandler(TSignalHandler handler, int signal);

public:
	template<typename class_t>
		void addThread(class_t* thread, void* data, EThreadStartType execute, size_t cpu) {
			TThreadProperties tp;
			tp.owner = this;
			tp.infoLog = infoLog;
			tp.exceptionLog = exceptionLog;
			tp.mtx = &mtx;
			tp.process = getpid();
			thread->setProperties(tp);
			threadList.push_back(thread);
			if (THD_START_ON_CREATE == execute && thread->hasExecHandler()) {
				thread->start(data, cpu);
			} else {
				thread->prepare(data, cpu);
			}
		}

	// Specialized templates to create and add managed threads derived from TManagedThread
	template<typename exec_t, typename owner_t>
		inline TManagedThread* addThread(const std::string& name, exec_t &&threadExecMethod, owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			TManagedThread* thread = new TManagedThread(name);
			thread->bindExecHandler(threadExecMethod, owner);
			addThread(thread, nil, execute, cpu);
			return thread;
		}

	template<typename exec_t, typename message_t, typename owner_t>
		inline TManagedThread* addThread(const std::string& name, exec_t &&threadExecMethod, message_t &&onThreadMessage,
				owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			TManagedThread* thread = new TManagedThread(name);
			thread->bindExecHandler(threadExecMethod, owner);
			thread->bindMessageHandler(onThreadMessage, owner);
			addThread(thread, nil, execute, cpu);
			return thread;
		}

	// Create and add managed any threads derived from data template
	template<class class_t, typename data_t, typename exec_t, typename owner_t>
		inline TWorkerThread<class_t>* addThread(const std::string& name, data_t &&data, exec_t &&threadExecMethod,
				owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			TWorkerThread<class_t>* thread = new TWorkerThread<class_t>(name);
			thread->bindExecHandler(threadExecMethod, owner);
			class_t* p = static_cast<class_t*>(&data); // data_t is type of class_t!
			addThread(thread, p, execute, cpu);
			return thread;
		}

	template<class class_t, typename data_t, typename exec_t, typename message_t, typename owner_t>
		inline TWorkerThread<class_t>* addThread(const std::string& name, data_t &&data,
				exec_t &&threadExecMethod, message_t &&onThreadMessage,
				owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			TWorkerThread<class_t>* thread = new TWorkerThread<class_t>(name);
			thread->bindExecHandler(threadExecMethod, owner);
			thread->bindMessageHandler(onThreadMessage, owner);
			class_t* p = static_cast<class_t*>(&data); // data_t is type of class_t!
			addThread(thread, p, execute, cpu);
			return thread;
		}

	template<class class_t, typename data_t, typename exec_t, typename message_t, typename sync_t, typename owner_t>
		inline TWorkerThread<class_t>* addThread(const std::string& name, data_t &&data,
				exec_t &&threadExecMethod, message_t &&onThreadMessage, sync_t &&threadSyncMethod,
				owner_t &&owner, EThreadStartType execute = THD_START_ON_CREATE, size_t cpu = app::nsizet) {
			TWorkerThread<class_t>* thread = new TWorkerThread<class_t>(name);
			thread->bindExecHandler(threadExecMethod, owner);
			thread->bindMessageHandler(onThreadMessage, owner);
			thread->bindSyncHandler(threadSyncMethod, owner);
			class_t* p = static_cast<class_t*>(&data); // data_t is type of class_t!
			addThread(thread, p, execute, cpu);
			return thread;
		}

	void terminate();
	void waitFor();

	TThreadController(TLogFile& logger, TLogFile& exeptionLog);
	virtual ~TThreadController();
};



struct TWebDataReceiver {
	app::TThreadDataReceived handler;
	app::PWebDataThread thread;
	app::PThreadData sender;
	std::mutex* mtx;
	util::TBuffer data;
	util::TVariantValues params;
	util::TVariantValues session;
	std::string url;
	int error;
	bool zipped;
	bool running;

	void setRunning(const bool running) {
		std::lock_guard<std::mutex> lock(*mtx);
		this->running = running;
	}
	bool getRunning() const {
		std::lock_guard<std::mutex> lock(*mtx);
		return running;
	}
	void prime() {
		mtx = nil;
		handler = nil;
		sender = nil;
		zipped = false;
		running = false;
		error = 0;
	}
	void reset() {
		params.clear();
		session.clear();
		data.clear();
		url.clear();
	}
	void clear() {
		prime();
		reset();
	}

	TWebDataReceiver() {
		prime();
	}
};


class TThreadDataItem : public TObject {
private:
	util::TBuffer data;
	int refc;
	bool bIsZipped;
	util::TTimePart timestamp;
	void prime();

public:
	bool hasData() const { return !data.empty(); };
	bool isValid() const { return hasData(); };
	bool isZipped() const { return bIsZipped; };
	void setZipped(const bool value) { bIsZipped = value; };
	int getRefCount() const { return refc; };
	void decRefCount() { if (refc > 0) --refc; };
	util::TTimePart getTimeStamp() const { return timestamp; };
	void getData(const void*& data, size_t& size);
	void getData(util::TBuffer& data);
	util::TBuffer& getData();
	size_t getSize() const { return data.size(); };
	void initialize(const void *const data, size_t size);
	void initialize(util::TBuffer& data);
	void finalize(bool free = false);

	TThreadDataItem();
	TThreadDataItem(util::TBuffer& data);
	TThreadDataItem(const void *const data, size_t size);
	virtual ~TThreadDataItem();
};


class TThreadData : public TObject {
private:
	mutable std::mutex threadMtx;
	mutable std::mutex dataMtx;
	TThreadDataList list;
	util::TZLib zip;
	size_t hash;
	bool bUseZip;
	util::TTimePart delay;
	TThreadDataRequest onDataNeeded;
	TThreadDataReceived onDataReceived;
	TWebReceiverList threadList;
	TWebDataThread thread;

	void prime();
	PThreadDataItem dataNeeded(const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers,
			bool& zipped, bool& cached, int& error, bool forceUpdate = false);
	bool compareParams(const util::TVariantValues& params);
	size_t release(bool force = false);

	void executeAction(const TThreadDataReceived& handler, util::TBuffer& data, const std::string& url,
			const util::TVariantValues& params, const util::TVariantValues& session, const bool deferred, const bool zipped, int& error);
	void threadExecuter(TWebDataReceiver& receiver);

	PWebDataReceiver findReceiverSlot();
	void clearReceiverList();
	void waitFor();

public:
	void clear();
	void setZipped(const bool value) { bUseZip = value; };
	bool useZip() const { return bUseZip; };
	void setDelay(util::TTimePart delay);
	void finalize(PThreadDataItem& data);
	size_t garbageCollector();

	PThreadDataItem setData(util::TBuffer& data, const std::string& url,
			const util::TVariantValues& params, const util::TVariantValues& session,
			bool deferred, bool zipped, int& error);
	PThreadDataItem getData(const void*& data, size_t& size,
			const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers,
			bool& zipped, bool& cached, int& error, bool forceUpdate = false);

	template<typename request_t, typename class_t>
		inline void bindDataRequestHandler(request_t &&onDataRequest, class_t &&owner) {
			this->onDataNeeded = std::bind(onDataRequest, owner,
						std::placeholders::_1, std::placeholders::_2,
						std::placeholders::_3, std::placeholders::_4,
						std::placeholders::_5, std::placeholders::_6,
						std::placeholders::_7, std::placeholders::_8,
						std::placeholders::_9);
		}

	template<typename request_t, typename class_t>
		inline void bindDataReceivedHandler(request_t &&onDataReceived, class_t &&owner) {
			this->onDataReceived = std::bind(onDataReceived, owner,
						std::placeholders::_1, std::placeholders::_2,
						std::placeholders::_3, std::placeholders::_4,
						std::placeholders::_5, std::placeholders::_6,
						std::placeholders::_7);
		}

	TThreadData(util::TTimePart delay = 0);
	virtual ~TThreadData();
};


} /* namespace app */

// Include template implementation of TPersistentThread
#include "threads.tpp"

#endif /* THREADS_H_ */
