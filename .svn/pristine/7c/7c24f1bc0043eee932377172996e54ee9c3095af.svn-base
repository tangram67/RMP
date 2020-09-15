/*
 * hash.h
 *
 *  Created on: 24.01.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef HASH_H_
#define HASH_H_

#include "gcc.h"
#include <algorithm>

namespace util {

enum EHashOffset { HASH_STRING = 0, HASH_VALUE = 1 };


#ifdef STL_HAS_CONSTEXPR

using hash_type = std::string::size_type;

# ifdef GLIBC_HAS_TUPLE
using THashKey = std::tuple<std::string, hash_type>;
# else
struct CTuple {
	std::string key;
	hash_type hash;
};
using THashKey = CTuple;
# endif

#else

typedef std::string::size_type hash_type;

# ifdef GLIBC_HAS_TUPLE
typedef std::tuple<std::string, hash_type> THashKey;
# else
typedef struct CTuple {
	std::string key;
	hash_type hash;
} THashKey;
# endif

#endif


inline hash_type calcHash(std::string value, bool tolower = true) {
	hash_type h = 0;
	if (value.size()) {
		if (tolower)
			std::transform(value.begin(), value.end(), value.begin(), ::tolower);
#ifdef STL_HAS_HASH_VALUE
		std::hash<std::string> oh;
		h = oh(value);
#else
		for (std::string::size_type i=0; i<value.size(); ++i) {
			// h = (31 * h) + s[i]
			// -->  31 * h = (32-1) * h = ((2^5)-1) * h = (h<<5) - h
			h = (h << 5) - h + value[i];
		}
#endif
	}
	return h;
}

inline hash_type calcHash(const char* value) {
	std::string s(value);
	return calcHash(s);
}

#ifdef GLIBC_HAS_TUPLE

struct CHashString {
	THashKey value;

	void setString(const std::string& s) {
		std::get<HASH_STRING>(value) = s;
		std::get<HASH_VALUE>(value) = util::calcHash(s);
	}
	std::string getString() const { return std::get<HASH_STRING>(value); }
	hash_type getHash() const { return std::get<HASH_VALUE>(value); }
	void assign(const THashKey& value) { this->value = value; }

	void clear()  {
		std::get<HASH_STRING>(value).clear();
		std::get<HASH_VALUE>(value) = 0;
	}

	CHashString() { clear(); }
	~CHashString() {}
};

#else

struct CHashString {
	THashKey value;

	void setString(const std::string& s) {
		value.key = s;
		value.hash = util::calcHash(s);
	}
	std::string getString() const { return value.key; }
	hash_type getHash() const { return value.hash; }
	void assign(const THashKey& value) { this->value = value; }

	void clear()  {
		value.key.clear();
		value.hash = 0;
	}

	CHashString() { clear(); }
	~CHashString() {}
};

#endif

#ifdef STL_HAS_CONSTEXPR
using THashString = CHashString;
#else
typedef CHashString THashString;
#endif


} // namespace util

#endif /* HASH_H_ */
