/*
 * helper.h
 *
 *  Created on: 15.03.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef TEMPLATES_H_
#define TEMPLATES_H_

#include <unistd.h>
#include <array>
#include <vector>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iostream>
#include "classes.h"
#include "nullptr.h"
#include "gcc.h"
#include "../config.h"

namespace util {

template<typename T>
class TObjectGuard {
private:
	typedef T object_t;
	typedef object_t * object_p;
	object_p * object;

	void clear() {
		if (*object != nil)
			delete *object;
		*object = nil;
	}

public:
	TObjectGuard& operator=(const TObjectGuard&) = delete;
	TObjectGuard(const TObjectGuard&) = delete;

	explicit TObjectGuard(object_p * F) : object {F} {}
	~TObjectGuard() { clear(); }
};


template<typename T>
class TPointerGuard {
private:
	typedef T pointer_t;
	typedef pointer_t const * pointer_p;
	pointer_p * pointer;

	void reset() {
		*pointer = nil;
	}

public:
	TPointerGuard& operator=(const TPointerGuard&) = delete;
	TPointerGuard(const TPointerGuard&) = delete;

	explicit TPointerGuard(pointer_p * F) : pointer {F} {}
	~TPointerGuard() { reset(); }
};


template<typename T>
class TBooleanGuard {
private:
	typedef T bool_t;
	bool_t& value;
	bool leave;

	void setState() {
		if (leave) set();
		else reset();
	}

public:
	bool get() const { return value; }
	void set() { value = true; }
	void reset() { value = false; }
	void toggle() { value = !value; }

	TBooleanGuard& operator=(const TBooleanGuard&) = delete;
	TBooleanGuard(const TBooleanGuard&) = delete;

	explicit TBooleanGuard(bool_t& F, bool state) : value {F}, leave {state} {}
	explicit TBooleanGuard(bool_t& F) : value {F}, leave {false} {}
	~TBooleanGuard() { setState(); }
};


template<typename T>
class TArgumentGuard {
private:
	typedef T param_t;
	va_list ap;
	param_t param;
	bool started;

public:
	void start() {
		if (!started) {
			va_start(ap, param);
			started = true;
		}
	}
	void end() {
		if (started) {
			va_end(ap);
			started = false;
		}
	}

	va_list& operator () () { return ap; }

	TArgumentGuard& operator=(const TArgumentGuard&) = delete;
	TArgumentGuard(const TArgumentGuard&) = delete;

	explicit TArgumentGuard(const param_t& param) : param {param}, started {false} { start(); }
	~TArgumentGuard() { end(); }
};


template<typename T>
class TStreamGuard {
private:
	typedef T iostream_t;
	iostream_t&  instance;
	bool opened;

public:
	void open(const std::string& fileName, std::ios_base::openmode mode) {
		instance.open(fileName, mode);
		opened = true;
	}
	void close() {
		if (opened) {
			instance.flush();
			instance.close();
			opened = false;
		}
	}

	TStreamGuard& operator=(const TStreamGuard&) = delete;
	TStreamGuard(const TStreamGuard&) = delete;

	explicit TStreamGuard(iostream_t& F) : instance {F} {
		opened = false;
	}
	~TStreamGuard() {
		if (opened)
			instance.close();
	}
};


template<typename T>
class TFileGuard
{
private:
	typedef T file_t;
	file_t&  instance;

public:
	TFileGuard& operator=(const TFileGuard&) = delete;

	bool isOpen() const {
		return instance.isOpen();
	}

	void open(int flags, mode_t mode = 0644) {
		if (!isOpen())
			instance.open(flags, mode);
	}
	void close() {
		if (isOpen())
			instance.close();
	}

	explicit TFileGuard(file_t& F, int flags, mode_t mode) : instance {F} {
		instance.open(flags, mode);
	}
	explicit TFileGuard(file_t& F, int flags) : instance {F} {
		instance.open(flags);
	}

	TFileGuard(const TFileGuard&) = delete;

	TFileGuard(file_t& F) : instance {F} {}
	~TFileGuard() {
		close();
	}
};


template<typename T>
class TDescriptorGuard
{
private:
	typedef T fd_t;
	fd_t fd;

public:
	TDescriptorGuard& operator=(const TDescriptorGuard&) = delete;

	int close() {
		int r = EXIT_SUCCESS;
		if (fd >= 0) {
			do {
				errno = EXIT_SUCCESS;
				r = ::close(fd);
			} while (r == EXIT_ERROR && errno == EINTR);
		}
		if (r >= 0) {
			fd = -1;
		}
		return r;
	}
	ssize_t read(void* data, size_t size) {
		ssize_t r = 0;
		if (fd >= 0) {
			do {
				errno = EXIT_SUCCESS;
				r = ::read(fd, data, size);
			} while (r == EXIT_ERROR && errno == EINTR);
			return r;
		}
		errno = EBADF;
		return EXIT_ERROR;
	}
	ssize_t write(const void* data, size_t size) {
		ssize_t r = 0;
		if (fd >= 0) {
			do {
				errno = EXIT_SUCCESS;
				r = ::write(fd, data, size);
			} while (r == EXIT_ERROR && errno == EINTR);
			return r;
		}
		errno = EBADF;
		return EXIT_ERROR;
	}

	TDescriptorGuard(const TDescriptorGuard&) = delete;

	explicit TDescriptorGuard(fd_t fd) : fd  {fd} {}
	~TDescriptorGuard() { close(); }
};


template<typename value_t>
	void hexout(const std::string& text, const value_t& value) {
		if (!text.empty()) {
			if (text[text.size() - 1] == ' ')
				std::cout << text << "0x" << std::hex << std::setw(2) << std::setfill('0') << (uint64_t)value << std::endl;
			else
				std::cout << text << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint64_t)value << std::endl;
		} else
			std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (uint64_t)value << std::endl;
		std::cout.copyfmt(std::ios(nil));
#ifdef USE_BOOLALPHA
		std::cout << std::boolalpha;
#endif
	}


// Example for "Substitution failure is not an error" (SFINAE)
template<typename iterator_t, typename = void>
	struct is_iterator {
#ifdef STL_HAS_CONSTEXPR
	   static constexpr bool value = false;
#else
	   static const bool value = false;
#endif
	};

template<typename iterator_t>
	struct is_iterator<iterator_t, typename std::enable_if<!std::is_same<typename std::iterator_traits<iterator_t>::value_type, void>::value>::type> {
#ifdef STL_HAS_CONSTEXPR
	   static constexpr bool value = true;
#else
	   static const bool value = true;
#endif
	};


template<typename value_t>
	inline value_t pred(value_t value) {
#ifdef STL_HAS_STATIC_ASSERT
		static_assert(std::is_scalar<decltype(value)>::value || is_iterator<decltype(value)>::value, "util::pred() Argument <value> is neither arithmetic, nor iterator type.");
#endif
		return --value;
	}

template<typename value_t>
	inline value_t succ(value_t value) {
#ifdef STL_HAS_STATIC_ASSERT
		static_assert(std::is_scalar<decltype(value)>::value || is_iterator<decltype(value)>::value, "util::succ() Argument <value> is neither arithmetic, nor iterator type.");
#endif
		return ++value;
	}

template<typename value_t>
	inline void exchange(value_t& s, value_t& d) {
		value_t& t = s;
		s = d;
		d = t;
	}

template<typename value_t>
	inline value_t align(value_t value, const unsigned char byte) {
#ifdef STL_HAS_STATIC_ASSERT
		static_assert(std::is_scalar<decltype(value)>::value || is_iterator<decltype(value)>::value, "util::align() Argument <value> is neither arithmetic, nor iterator type.");
#endif
		if (byte == 0)
			return value;
		value_t r = value % (value_t)byte;
		return (value > r) ? value - r : 0;
	}

template<typename value_t>
	value_t roundUp(value_t value, const size_t multiple) {
#ifdef STL_HAS_STATIC_ASSERT
		static_assert(std::is_scalar<decltype(value)>::value || is_iterator<decltype(value)>::value, "util::roundUp() Argument <value> is neither arithmetic, nor iterator type.");
#endif
		if (multiple == 0)
			return value;
		value_t r = value % (value_t)multiple;
		return value + multiple - r;
	}

template<typename value_t>
	value_t roundDown(value_t value, const size_t multiple) {
#ifdef STL_HAS_STATIC_ASSERT
		static_assert(std::is_scalar<decltype(value)>::value || is_iterator<decltype(value)>::value, "util::roundDown() Argument <value> is neither arithmetic, nor iterator type.");
#endif
		if (multiple == 0)
			return value;
		value_t r = value % (value_t)multiple;
		return (value > r) ? value - r : 0;
	}

template<typename object_t>
	inline bool assigned(object_t&& object) {
		return (nil != object);
	}

template<typename object_t>
	inline void freeAndNil(object_t&& object) {
		// Prevent segmentation fault...
		if (assigned(object)) {
			delete object;
			object = nil;
		}
	}


template <class class_t, typename object_t>
	class_t* asClass(object_t* object) {
		return dynamic_cast<class_t*>(object);
	}

template <class class_t, typename object_t>
	bool isClass(const object_t* object) {
//		if (std::is_polymorphic<decltype(*object)>::value)
//			return assigned(dynamic_cast<const class_t*>(object));
//		else
//			return false;
		return assigned(asClass<const class_t>(object));
	}



template<typename value_t>
	bool isMemberOf(const value_t value) {
		return false;
	}

template<typename value_t>
	bool isMemberOf(const value_t value, value_t prime) {
		return (value == prime);
	}

template<typename value_t, typename... variadic_t>
	bool isMemberOf(const value_t value, const value_t prime, variadic_t... set) {
		if (value == prime)
			return true;
		return isMemberOf(value, set...);
	}


template<typename array_t, size_t size>
#ifdef STL_HAS_CONSTEXPR
	constexpr size_t sizeOfArray(array_t (&) [size]) {
#else
	const size_t sizeOfArray(array_t (&) [size]) {
#endif
		return size;
	}


template <typename list_t>
	inline bool validListIndex(list_t&& list, size_t index) {
		return (index >= 0 && index < list.size());
	}

template <typename list_t>
	inline void clearObjectList(list_t&& list) {
#ifdef STL_HAS_RANGE_FOR
		for (auto o : list)
			freeAndNil(o);
#else
		size_t i,n;
		n = list.size();
		for (i=0; i<n; i++) {
			auto o = list[i];
			freeAndNil(o);
		}
#endif
		list.clear();
	}

template <typename list_t>
	inline void shiftVectorList(list_t&& list) {
		if (list.size() > 1) {
			size_t n = list.size();
			for (int i=n-2; i>=0; --i) {
				list[i+1] = list[i];
			}
		}
	}

} /* namespace util */

#endif /* TEMPLATES_H_ */
