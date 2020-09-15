/*
 * detach.tpp
 *
 *  Created on: 19.07.2015
 *      Author: Dirk Brinkmeier
 *      
 *  Template definition in conjunction with detach.h
 *  This file must NOT be included in project make/build files!
 *  
 *  Need includes via detach.h
 *        
 */
namespace app {

template<typename T>
struct CParameterObject {
	typedef T param_t;

	bool valid;
	param_t& param;
	
	CParameterObject(param_t& param) : param(param) {
		valid = true;
	}
};

template<typename T>
class TDataThread : public TIntermittentThread {
private:
	
#ifdef GCC_HAS_TEMPLATE_ALIAS

	using param_t = T;
	using TThreadDataMethod = std::function<void(param_t&)>;
	using PDataThread = TDataThread<param_t>*;
	
	using TParameterObject = CParameterObject<param_t>;
	using PParameterObject = TParameterObject*;
	using TParamList = std::vector<PParameterObject>;

#else

	typedef T param_t;
	typedef std::function<void(param_t&)> TThreadDataMethod;
	typedef TDataThread<param_t>* PDataThread;
	
	typedef CParameterObject<param_t> TParameterObject;
	typedef TParameterObject* PParameterObject;
	typedef std::vector<PParameterObject> TParamList;

#endif
	
	mutable std::mutex mtx;
	TThreadDataMethod threadExecMethod;
	TParamList params;
	
	void prime() {
		threadExecMethod = nil;
	}
	
	void addParam(param_t& param) {
		std::lock_guard<std::mutex> lock(mtx);
		PParameterObject o = new TParameterObject(param);
		params.push_back(o);
		garbageCollector();
	}
	
	// Call only once since parameter entry is invalidated by flag!
	param_t& getParam() {
		std::lock_guard<std::mutex> lock(mtx);
		size_t n = params.size();
		if (!params.empty()) {
			PParameterObject o;
			for (size_t i=0; i<n; i++) {
				o = params[i];
				if (util::assigned(o)) {
					if (o->valid) {
						o->valid = false;
						return o->param;
					}
				}
			}
		}
		// Nothing to return, should never ever happen...
		write("TDataThread::getParam() : No valid entries in parameter list.");
		throw util::app_error("TDataThread::getParam() : No valid entries in parameter list.");
	}
	
	// Static thread function for executing tread routine
	static void* dataThreadExecuter(void* cls) {
		int retVal = EXIT_FAILURE;
		auto thread = static_cast<PDataThread>(cls);
		if (util::assigned(thread)) {
			try {
				thread->execute();
				retVal = EXIT_SUCCESS;
			} catch (const std::exception& e)	{
				thread->writeException(e, "TDataThread::dataThreadExecuter()");
			} catch (...) {
				thread->writeException("TDataThread::dataThreadExecuter()");
			}
		}
		decThreadCount();
		return (void *)(long)(retVal);
	}
	
	struct CDataEraser	{
		CDataEraser() {}
	    bool operator()(PParameterObject o) const {
	    	bool retVal = false;
	    	if (util::assigned(o)) {
				if (!o->valid) {
					util::freeAndNil(o);
					retVal = true;
				}	
	    	}
	    	return retVal;
	    }
	};

	void garbageCollector() {
		if (params.size() > 0) {
			params.erase(std::remove_if(params.begin(), params.end(), CDataEraser()), params.end());
		}
	}
	
	void clear() {
		util::clearObjectList(params);
	}
	
	
public:
	template<typename exec_t, typename class_t>
		inline void setExecHandler(exec_t&& threadMethod, class_t&& owner) {
			threadExecMethod = std::bind(threadMethod, owner, std::placeholders::_1);
		}
	
	void run(param_t& param) {
		if (threadExecMethod != nil) {
			incThreadCount();
			addParam(param);
			createDetachedThread(dataThreadExecuter);
		} else {
			if (name.empty())
				util::app_error("TDataThread::run() failed: No thread method assigned.");
			else
				util::app_error("TDataThread::run() failed for \"" + name + "\" : No thread method assigned.");
		}
	}
	
	void execute() {
		if (threadExecMethod != nil) {
			threadExecMethod(getParam());
		}	
	};

	template<typename exec_t, typename class_t>
		TDataThread(exec_t&& threadExecMethod, class_t&& owner) {
			this->threadExecMethod = std::bind(threadExecMethod, owner, std::placeholders::_1);
		}

	TDataThread() { prime(); }
	~TDataThread() { clear(); }
};

}
