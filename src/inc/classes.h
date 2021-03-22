/*
 * classes.h
 *
 *  Created on  : 02.09.2014
 *  Reworked on : 09.01.2020 Removed unused types
 *  Author      : Dirk Brinkmeier
 */

#ifndef CLASSES_H_
#define CLASSES_H_

#include <functional>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <signal.h>
#include "nullptr.h"

// EXIT_SUCCESS = 0
// EXIT_FAILURE = 1
// EXIT_ERROR   = -1
#ifndef EXIT_ERROR
#  define EXIT_ERROR (INT32_C(-1))
#endif
#ifndef INVALID_HANDLE_VALUE
#  define INVALID_HANDLE_VALUE (INT32_C(-1))
#endif

#ifdef STL_HAS_TEMPLATE_ALIAS

using size_u  = long long unsigned int;
using size_s  = long long int;
using byte_t  = uint8_t;
using word_t  = uint16_t;
using dword_t = uint32_t;
using qword_t = uint64_t;
using int_t   = int16_t;
using long_t  = int32_t;

#else

typedef long long unsigned int size_u;
typedef long long int size_s;
typedef uint8_t  byte_t;
typedef uint16_t word_t;
typedef uint32_t dword_t;
typedef uint64_t qword_t;
typedef int16_t  int_t;
typedef int32_t  long_t;

#endif

namespace app {

static const size_t nsizet = static_cast<size_t>(-1);

enum EUpdateReason { ER_DATETIME, ER_LANGUAGE, ER_REFRESH };

class TObject;
class TModule;

#ifdef STL_HAS_TEMPLATE_ALIAS

using THandle = int;

// Event handlers
using TEventHandler = std::function<void()>;
using TEventList = std::vector<app::TEventHandler>;

// Signal handlers
using TSignalHandler = void (*) (int, siginfo_t *, void *);
using TThreadHandler = void* (*) (void*);

// Basic reference types
using PObject = TObject*;
using PModule = TModule*;

// String based key/value list
using TValueMap = std::map<std::string, std::string>;
using TValueMapItem = std::pair<std::string, std::string>;
using TStringVector = std::vector<std::string>;

// String + object based key/value list
template<typename T>
using TObjectItem = std::pair<std::string, T*>;

template<typename T>
using TObjectVector = std::vector<TObjectItem<T>>;

#else

typedef int THandle;

// Event handlers
typedef std::function<void()> TEventHandler;
typedef std::vector<app::TEventHandler> TEventList;

// Signal handlers
typedef void (*TSignalHandler)(int, siginfo_t *, void *);
typedef void* (*TThreadHandler)(void*);

// Basic reference types
typedef TObject* PObject;
typedef TModule* PModule;

// String based key/value list
typedef std::map<std::string, std::string> TValueMap;
typedef std::pair<std::string, std::string> TValueMapItem;
typedef std::vector<std::string> TStringVector;

template <typename T>
struct TObjectItem {
    typedef std::pair<std::string, T*> type_t;
};

template <typename T>
struct TObjectVector {
    typedef std::vector<TObjectItem<T>> type_t;
};

#endif

class TObject {
protected:
	std::string name;
	PObject owner;

public:
	void setName(const std::string& name) { this->name = name; };
	const std::string& getName() const { return name; };

	void setOwner(PObject owner) { this->owner = owner; };
	PObject getOwner() const { return owner; };
	bool hasOwner() const { return nil != owner; };

	TObject& self() { return *this; };
	TObject& operator () () { return self(); };

	template<typename class_t>
	TObject(class_t &&owner) : owner(owner) {};

	explicit TObject() : owner(nil) {};
	virtual ~TObject() = default;
};

class TModule : public TObject {
public:
	virtual int prepare() { return EXIT_SUCCESS; };
	virtual void unprepare() {};

	virtual void update(const EUpdateReason reason) {};

	virtual int execute() = 0;
	virtual void cleanup() = 0;

	explicit TModule() {};
	virtual ~TModule() = default;
};

} /* namespace app */

#endif /* CLASSES_H_ */
