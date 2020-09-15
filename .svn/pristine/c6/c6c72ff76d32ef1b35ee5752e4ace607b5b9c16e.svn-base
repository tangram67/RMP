/*
 * typeid.cpp
 *
 *  Created on: 15.03.2015
 *      Author: Dirk Brinkmeier
 */

#include <cxxabi.h>
#include <algorithm>
#include <functional>
#include "gcc.h"
#include "templates.h"
#include "memory.h"
#include "typeid.h"

using namespace util;


bool typeIdEraser(char c) {
	if (c <= ' ')
		return true;
	switch(c) {
		case '*':
		case '#':
			return true;
		default:
			return false;
	}
}


std::string util::demangleTypeID(const char* name)
{
#ifdef GCC_HAS_CXA_DEMANGLE

	// Demangle C++ symbols
	size_t len = strnlen(name, 255);
	if (len > 0) {
		std::string s(name, len);
		std::size_t size = 2 * s.size();
		TBuffer buffer(size);
		int status = EXIT_FAILURE;
		char *c_str = abi::__cxa_demangle(name, buffer.data(), &size, &status);
		if (util::assigned(c_str) && (status == EXIT_SUCCESS)) {
			s = std::string(buffer.data());
			if (!s.empty())
				s.erase(std::remove_if(s.begin(), s.end(), std::function<bool(char)>(typeIdEraser)), s.end());
		}
		return s;
	}
	return "<invalid>";

#else
	return name;
#endif
}


