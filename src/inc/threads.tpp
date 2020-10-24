/*
 * threads.tpp
 *
 *  Created on: 04.10.2018
 *      Author: Dirk Brinkmeier
 *      
 *  Template definition in conjunction with threads.h
 *  This file must NOT be included in project make/build files!
 *  
 *  Need includes via threads.h
 *        
 */

namespace app {


/*
 * TWorkerThread
 */
template<class T>
class TWorkerThread : public TBaseThread, protected TThreadAffinity {
friend class TThreadController;
protected:
	
#ifdef GCC_HAS_TEMPLATE_ALIAS
	
	using data_t = T;
	using data_p = data_t*;

	using TThreadMessageMethod = std::function<void(TWorkerThread<data_t>&, EThreadMessageType, data_t&)>;
	using TThreadSyncMethod = std::function<void(TWorkerThread<data_t>&, data_t&)>;
	using TThreadExecMethod = std::function<int(TWorkerThread<data_t>&, data_t&)>;

#else
	
	typedef T data_t;
	typedef data_t* data_p;

	typedef std::function<void(TWorkerThread<data_t>&, EThreadMessageType, data_t&)> TThreadMessageMethod;
	typedef std::function<void(TWorkerThread<data_t>&, data_t&)> TThreadSyncMethod;
	typedef std::function<int(TWorkerThread<data_t>&, data_t&)> TThreadExecMethod;
	
#endif
	
private:
	mutable app::TMutex execMtx;
	TThreadCreator creator;
	TMessageSlot msgSlot;
	PSemaphore sema;
	PCondition cond;
	pid_t tid;
	size_t affinity;
	bool executed;
	bool synced;
	
	TThreadExecMethod threadExecMethod;
	TThreadMessageMethod threadMessageMethod;
	TThreadSyncMethod threadSyncMethod;

	void initialize() {
		affinity = app::nsizet;
		tid = 0;
		owner = nil;
		synced = false;
		executed = false;
		threadExecMethod = nil;
		threadMessageMethod = nil;
		threadSyncMethod = nil;
		sema = new TSemaphore(0, LCK_PROC_PRIVATE, "");
		cond = new TCondition();
		debug = false;
	}

	void createBaseThread(PThreadCreator creator) {
		int retVal;
		retVal = checkProperties();
		if (util::checkFailed(retVal))
			throw util::app_error("TWorkerThread::createBaseThread() Thread properties not set.");
		const char* desc = !name.empty() ? name.c_str() : nil;
		if (!createPersistentThread(creator, desc))
			throw util::app_error("TWorkerThread::createBaseThread() Create thread failed.");
	}

	void createSignalThread(PThreadMessage message) {
		const char* desc = !name.empty() ? name.c_str() : nil;
		if (!createIntermittentThread(message, desc))
			throw util::app_error("TWorkerThread::createSignalThread() Create thread failed.");
	}

	void sendThreadMessage(EThreadMessageType message, data_p data) {
		if (debug) writeLog("DEBUG: sendThreadMessage() for thread \"" + name + "\"");
		if (hasMessageHandler()) {
			{	// Open locking scope --> lock_guard lock.lock()
				std::lock_guard<std::mutex> lock(*globalMtx);
				TThreadMessage msg;
				msg.msg = message;
				msg.data = data;
				msg.type = THD_MTT_SEND;
				sendSignal(msg);
			}	// Leaving scope --> lock.unlock();

			// Wait for asynchronous handling in detached signal thread finished
			if (debug) writeLog("DEBUG: sendThreadMessage() Wait for thread \"" + name + "\" (" + std::to_string((size_u)message) + ")");
			sema->wait();
			if (debug) writeLog("DEBUG: sendThreadMessage() Message for thread \"" + name + "\" was sent.");
		}
	}

	void postThreadMessage(EThreadMessageType message, data_p data) {
		if (hasMessageHandler()) {
			std::lock_guard<std::mutex> lock(*globalMtx);
			TThreadMessage msg;
			msg.msg = message;
			msg.data = data;
			msg.type = THD_MTT_POST;
			sendSignal(msg);
			// Return to sender, while detached signal thread is still working...
		}
	}

	// Implement thread execution method
	int run(void* creator) override {
		int retVal = EXIT_FAILURE;
		{
			TThreadSaveBooleanGuard<bool> btm(terminate, statusMtx, true);
			TThreadSaveBooleanGuard<bool> btd(terminated, statusMtx, true);
			try {

				// Set thread's CPU affinity
				tid = TBaseThread::gettid();
				if (affinity != app::nsizet)
					setAffinity(affinity, tid);

				// Signal start of thread to calling process
				sendMessage(THD_MSG_INIT);

				// Call thread's execution method
				if (hasExecHandler()) {
					data_t& data = *((data_p)(((PThreadCreator)creator)->data));
					retVal = threadExecMethod(*this, data);
				}
				if (util::checkFailed(retVal)) {
					std::string sText = util::csnprintf("TWorkerThread::run() : threadExecMethod() for [" + name + "] failed on error code = %", retVal);
					writeLog(sText);
				}

			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				std::string sText = "TWorkerThread::run() : Exception in threadExecMethod() of [" + name + "] " + sExcept;
				errorLog(sText);
			} catch (...)	{
				std::string sText = "TWorkerThread::run() : Unknown exception in threadExecMethod() of [" + name + "]";
				errorLog(sText);
			}
		}
		// Thread is done...
		sendMessage(THD_MSG_QUIT);
		return retVal;
	}

	void waitFor() override {
		if (tid > 0) {
			int retVal, c = 0;
			while (!msgSlot.empty() && c < 60) {
				c++;
				util::saveWait(30);
			}

			retVal = terminatePersistentThread();
			if (util::checkFailed(retVal))
				throw util::sys_error("TWorkerThread::waitFor::pthread_join() failed.", errno);

			msgSlot.clear();
		}
	}

	void unlockThreadSync() override {
		app::TConditionGuard<app::TCondition> lock(*cond, false);
		if (lock.tryLock()) {
			lock.signal();
		}
	}

	int receive(void* message) override {
		if (util::assigned(message)) {
			PThreadMessage msg = (PThreadMessage)message;
			receiveSignal(msg);
			return EXIT_SUCCESS;
		}
		return EXIT_FAILURE;
	}

	void receiveSignal(PThreadMessage message) {
		if (debug) writeLog("DEBUG: receiveSignal() for thread \"" + name + "\"");
		try {

			// Use current thread data for simple messages...
			if (!util::assigned(message->data)) {
				message->data = creator.data;
			}

			// Dispatch message
			if (hasMessageHandler() && util::assigned(message->data)) {
				data_t& data = *(data_p)message->data;
				threadMessageMethod(*this, message->msg, data);
			}

		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::string sText = "TWorkerThread::receiveSignal : Exception in threadMessageMethod() for [" + name + "] " + sExcept;
			errorLog(sText);
		}

		if (message->type == THD_MTT_SEND)
			sema->post();

		msgSlot.release(message);
	}

	void sendSignal(const TThreadMessage& message) {
		if (hasMessageHandler()) {

			// Internal message parameter
			PThreadMessage msg = msgSlot.enqueue();
			msg->sender = this;
			msg->type = message.type;

			// User message parameter
			msg->msg = message.msg;
			msg->data = message.data;

			try {
				createSignalThread(msg);
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				std::string sText = "TWorkerThread::sendSignal() Exception in createSignalThread() for [" + name + "] " + sExcept;
				errorLog(sText);
			}
		}
	}

	bool hasMessageHandler() const {
		return threadMessageMethod != nil;
	}
	bool hasExecHandler() const {
		return threadExecMethod != nil;
	}
	bool hasSyncHandler() const {
		return threadSyncMethod != nil;
	}

	void prepare(void* data, size_t cpu = app::nsizet) override {
		app::TLockGuard<app::TMutex> lock(execMtx);
		if (cpu != app::nsizet)
			affinity = cpu;
		creator.data = data;
	}

	void start(void* data, size_t cpu = app::nsizet) override {
		app::TLockGuard<app::TMutex> lock(execMtx);
		if (!executed && hasExecHandler()) {
			executed = true;
			lock.unlock();
			terminate = false;
			terminated = false;
			if (cpu != app::nsizet)
				affinity = cpu;
			creator.data = data;
			creator.sender = this;
			createBaseThread(&creator);
		}
	}

public:
	pid_t gettid() { return tid; };

	bool isExecuted() const {
		app::TLockGuard<app::TMutex> lock(execMtx);
		return executed;
	}
	void execute() {
		start(creator.data, affinity);
	}

	// Synchronize data from thread to main process and wait for processing
	void sendMessage(EThreadMessageType message) {
		sendThreadMessage(message, nil);
	}
	void sendMessage(EThreadMessageType message, data_t& data) {
		sendThreadMessage(message, &data);
	}
	void sendMessage(EThreadMessageType message, data_p data) {
		sendThreadMessage(message, data);
	}

	// Post data from thread to main process and return immediately
	void postMessage(EThreadMessageType message) {
		postThreadMessage(message, nil);
	}
	void postMessage(EThreadMessageType message, data_t& data) {
		postThreadMessage(message, &data);
	}
	void postMessage(EThreadMessageType message, data_p data) {
		postThreadMessage(message, data);
	}

	// Synchronize data from main process to thread via virtual thread method synchronizer()
	bool sync(bool blocking = true) {
		app::TConditionGuard<app::TCondition> lock(*cond, blocking);
		bool blocked = true;
		if (!blocking) {
			blocked = lock.tryLock();
		}
		if (blocked) {
			try {
				if (hasSyncHandler()) {
					data_t& data = *(data_p)creator.data;
					threadSyncMethod(*this, data);
				}
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				std::string sText = "TWorkerThread::synchronize() : Exception in synchronize() for [" + name + "] " + sExcept;
				errorLog(sText);
			} catch (...)	{
				std::string sText = "TWorkerThread::synchronize() : Unknown exception in synchronize() for [" + name + "]";
				errorLog(sText);
			}
			synced = true;
			lock.signal();
		}
		return blocked;
	}

	// Try to synchronize data from main process to thread via virtual thread method synchronizer()
	bool trySync() {
		return sync(false);
	}

	// Use in thread to wait for synchronized data from main process, implicit locks internal condition mutex
	// Returns true if data is synched (otherwise false if end of thread via setter terminate)
	bool waitForSync() {
		synced = false;
		cond->wait();
		return synced;
	}

	void releaseSync() {
		// Deprecated!
		// syncMtx.unlock();
	}

	template<typename message_t, typename class_t>
		inline void bindMessageHandler(message_t &&onThreadMessage, class_t &&owner) {
			this->threadMessageMethod = std::bind(onThreadMessage, owner,
						std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		}

	template<typename sync_t, typename class_t>
		inline void bindSyncHandler(sync_t &&threadSyncMethod, class_t &&owner) {
			this->threadSyncMethod = std::bind(threadSyncMethod, owner, std::placeholders::_1, std::placeholders::_2);
		}

	template<typename exec_t, typename class_t>
		inline void bindExecHandler(exec_t &&threadExecMethod, class_t &&owner) {
			this->threadExecMethod = std::bind(threadExecMethod, owner, std::placeholders::_1, std::placeholders::_2);
		}


	explicit TWorkerThread(const std::string& name) : TBaseThread(), TThreadAffinity() {
		initialize();
		this->name = name;
	}

	TWorkerThread(const std::string& name, const TThreadProperties thread) : TBaseThread(), TThreadAffinity() {
		initialize();
		this->name = name;
		this->owner = thread.owner;
		this->globalMtx = thread.mtx;
		this->infoLog = thread.infoLog;
		this->exceptionLog = thread.exceptionLog;
		this->process = thread.process;
	}

	TWorkerThread(const std::string& name, std::mutex& mtx, pid_t process, TThreadController& owner,
				TLogFile& infoLog, TLogFile& exceptionLog) : TBaseThread(), TThreadAffinity() {
		initialize();
		this->name = name;
		this->owner = &owner;
		this->globalMtx = &mtx;
		this->infoLog = &infoLog;
		this->exceptionLog = &exceptionLog;
		this->process = process;
	}

	// No copy and move constructors
	TWorkerThread(TWorkerThread&) = delete;
	TWorkerThread(const TWorkerThread&) = delete;
	TWorkerThread(const TWorkerThread&&) = delete;

	virtual ~TWorkerThread() {
		util::freeAndNil(sema);
		util::freeAndNil(cond);
	}
};


}
