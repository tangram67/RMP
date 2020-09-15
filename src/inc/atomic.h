/*
 * atomic.h
 *
 *  Created on: 14.06.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

#include <functional>
#include <mutex>
#include "gcc.h"
#include "templates.h"

#ifdef STL_HAS_TEMPLATE_ALIAS

// Atomic integer types
using atomic_uint64 = uint64_t; // unsigned long int;
using atomic_int64  = int64_t;  // signed long int;
using atomic_bool   = bool;

#else

// Atomic integer types
typedef uint64_t atomic_uint64;
typedef int64_t  atomic_int64;
typedef bool     atomic_bool;

#endif

namespace util {

inline void atomicInc(atomic_uint64& value) {
	__sync_fetch_and_add(&value, 1);
}
inline void atomicDec(atomic_uint64& value) {
	__sync_fetch_and_sub(&value, 1);
}
inline atomic_uint64 atomicGet(atomic_uint64& value) {
	return __sync_fetch_and_add(&value, 0);
}

inline void atomicInc(atomic_int64& value) {
	__sync_fetch_and_add(&value, 1);
}
inline void atomicDec(atomic_int64& value) {
	__sync_fetch_and_sub(&value, 1);
}
inline atomic_int64 atomicGet(atomic_int64& value) {
	return __sync_fetch_and_add(&value, 0);
}

inline void atomicSet(atomic_bool& value) {
	__sync_fetch_and_or((int *)&value, true);
}
inline void atomicReset(atomic_bool& value) {
	__sync_fetch_and_and((int *)&value, false);
}
inline void atomicBool(atomic_bool& value, bool set) {
	if (set) atomicSet(value);
	else atomicReset(value);
}
inline atomic_bool atomicGet(atomic_bool& value) {
	return (atomic_bool)__sync_fetch_and_add((int *)&value, 0);
}


// Check native Compiler support for atomic builtins
// Example for type traits
template <typename T>
struct is_native_atomic_value {
#ifdef	GCC_HAS_ATOMICS
	static const bool value = true;
#else
	static const bool value = false;
#endif
};

// Specialized version for signed 64 Bit type int64_t
template <>
struct is_native_atomic_value<int64_t> {
#if defined	GCC_HAS_ATOMICS && defined TARGET_X64
	static const bool value = true;
#else
	static const bool value = false;
#endif
};

// Specialized version for unsigned 64 Bit type uint64_t
template <>
struct is_native_atomic_value<uint64_t> {
	static const bool value = is_native_atomic_value<int64_t>::value;
};


template<typename T>
class TAtomicOperator {
private:
	typedef T value_t;

#ifdef	STL_HAS_CONSTEXPR
	// Remark: static constexpr is set during compile time!
	static constexpr bool hasNativeCompilerSupport = is_native_atomic_value<value_t>::value;
#else
	static const bool hasNativeCompilerSupport = is_native_atomic_value<value_t>::value;
#endif

	value_t val;
	value_t* ptr;
	std::mutex mtx;

public:
	TAtomicOperator& operator=(const TAtomicOperator&) = delete;

	void assign(value_t& value) {
		val = value;
		ptr = &value;
	}

	value_t inc(const value_t value = 1) {
		if (hasNativeCompilerSupport) {
			return __sync_fetch_and_add(ptr, value);
		} else {
			std::lock_guard<std::mutex> lock(mtx);
			val += value;
			return val;
		}
	}

	value_t dec(const value_t value = 1) {
		if (hasNativeCompilerSupport) {
			return __sync_fetch_and_sub(ptr, value);
		} else {
			std::lock_guard<std::mutex> lock(mtx);
			val -= value;
			return val;
		}
	}

	value_t get() {
		if (hasNativeCompilerSupport) {
			return __sync_fetch_and_add(ptr, 0);
		} else {
			std::lock_guard<std::mutex> lock(mtx);
			return val;
		}
	}

	value_t set(const value_t value) {
		if (hasNativeCompilerSupport) {
			return __sync_val_compare_and_swap(ptr, *ptr, value);
		} else {
			std::lock_guard<std::mutex> lock(mtx);
			val = value;
			return val;
		}
	}

	explicit TAtomicOperator() { val = (value_t)0; ptr = nil; }
	TAtomicOperator(value_t& F) : val(F), ptr(&F) {}
	TAtomicOperator(const TAtomicOperator&) = delete;
	~TAtomicOperator() {}
};


// Example for ADT (Abstract Data Type)
template<typename T>
class TAtomicValue {
private:
	typedef T value_t;
	value_t value;
	TAtomicOperator<value_t> op;

public:
	value_t inc(const value_t value = 1) { return op.inc(value); }
	value_t dec(const value_t value = 1) { return op.dec(value); }
	value_t get() { return op.get(); }
	value_t set(const value_t value) { return op.set(value); }

	value_t operator () () { return op.get(); };

	TAtomicValue() { value = 0; op.assign(value); }
	virtual ~TAtomicValue() {}
};

#ifdef STL_HAS_TEMPLATE_ALIAS

using TAtomicInt  = TAtomicValue<int>;
using TAtomicLong = TAtomicValue<long>;
using TAtomic64   = TAtomicValue<int64_t>;

#else

typedef TAtomicValue<int> TAtomicInt;
typedef TAtomicValue<long> TAtomicLong;
typedef TAtomicValue<int64_t> TAtomic64;

#endif

} // namespace util

#endif /* ATOMIC_H_ */
