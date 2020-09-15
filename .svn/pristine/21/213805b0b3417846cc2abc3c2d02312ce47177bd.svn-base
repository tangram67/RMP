/*
 * stringtemplates.h
 *
 *  Created on: 18.06.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef STRINGTEMPLATES_H_
#define STRINGTEMPLATES_H_

#include <sstream>
#include "functors.h"
#include "../config.h"


#define DEFAULT_FORMAT_LENGTH 128


namespace util {


inline std::string removeFormatDuplicates(const char* format) {
	const char* src = format;
	const char* next = format+1;
    size_t size = 0;
    bool found = false;

    while (*src != '\0') {
		if (isDuplicatedFormatQualifierA(*src, *next)) {
			found = true;
		}
		++size; ++src; ++next;
    }

    if (size == 0)
    	return "";

    if (!found)
    	return std::string(format, size);

    src = format;
	next = format+1;
    std::string s;
    s.reserve(size+1);

    while (*src != '\0') {
		if (isDuplicatedFormatQualifierA(*src, *next)) {
			++src; ++next;
		}
		s += *src;
		++src; ++next;
    }

    return s;
}


// Parameter recursion ends here...
inline std::string csnprintf(const char* format) {
	return removeFormatDuplicates(format);
}

// Using stringstream for formatting values
// Pros: Only placeholder "%" needed, no format qualifier, elegant solution
// Cons: High costs: many string copies + stringstream overhead
template<typename value_t, typename... variadic_t>
std::string csnprintf(const char* format, const value_t& value, variadic_t... args)
{
	std::string s;
	std::stringstream ss;
    size_t reserved = DEFAULT_FORMAT_LENGTH;
	s.reserve(reserved);
#ifdef USE_BOOLALPHA
	ss << std::boolalpha;
#endif
	const char* next = format+1;
	for ( ; *format != '\0'; ++format, ++next ) {
		if (isValidFormatQualifierA(*format, *next)) {
			if (*format == '$') {
				ss << value;
				s += "\"" + ss.str() + "\"" + csnprintf(format+1, args...);
			} else if (*format == '@') {
				ss << "0x" << std::hex << std::setw(2) << std::setfill('0') << value;
				s += ss.str() + csnprintf(format+1, args...);
				ss.copyfmt(std::ios(nil));
#ifdef USE_BOOLALPHA
				ss << std::boolalpha;
#endif
			} else {
				ss << value;
				s += ss.str() + csnprintf(format+1, args...);
			}
			return s;
		}
		if (isDuplicatedFormatQualifierA(*format, *next)) {
			++format;
			++next;
		}
		s += *format;
		if (s.size() >= reserved) {
			reserved += reserved;
		    s.reserve(reserved);
		}
	}
	return s;
}

template<typename value_t, typename... variadic_t>
std::string csnprintf(const std::string& format, const value_t& value, variadic_t... args)
{
	if (!format.empty())
		return csnprintf(format.c_str(), value, std::forward<variadic_t>(args)...);
	return std::string();
}



inline std::wstring removeFormatDuplicates(const wchar_t* format) {
	const wchar_t* src = format;
	const wchar_t* next = format+1;
    size_t size = 0;
    bool found = false;

    while (*src != L'\0') {
		if (isDuplicatedFormatQualifierW(*src, *next)) {
			found = true;
		}
		++size; ++src; ++next;
    }

    if (size == 0)
    	return L"";

    if (!found)
    	return std::wstring(format, size);

    src = format;
	next = format+1;
    std::wstring s;
    s.reserve(size+1);

    while (*src != L'\0') {
		if (isDuplicatedFormatQualifierW(*src, *next)) {
			++src; ++next;
		}
		s += *src;
		++src; ++next;
    }

    return s;
}

// Parameter recursion ends here...
inline std::wstring csnprintf(const wchar_t* format) {
	return removeFormatDuplicates(format);
}

template<typename value_t, typename... variadic_t>
std::wstring csnprintf(const wchar_t* format, const value_t& value, variadic_t... args)
{
	std::wstring s;
	std::wstringstream ss;
    size_t reserved = DEFAULT_FORMAT_LENGTH;
	s.reserve(reserved);
#ifdef USE_BOOLALPHA
	ss << std::boolalpha;
#endif
	const wchar_t* next = format+1;
	for ( ; *format != L'\0'; ++format, ++next ) {
		if (isValidFormatQualifierW(*format, *next)) {
			ss << value;
			if (*format == L'$')
				s += L"\"" + ss.str() + L"\"" + csnprintf(format+1, args...);
			else
				s += ss.str() + csnprintf(format+1, args...);
			return s;
		}
		if (isDuplicatedFormatQualifierW(*format, *next)) {
			++format;
			++next;
		}
		s += *format;
		if (s.size() >= reserved) {
			reserved += reserved;
		    s.reserve(reserved);
		}
	}
	return s;
}

template<typename value_t, typename... variadic_t>
std::wstring csnprintf(const std::wstring& format, const value_t& value, variadic_t... args)
{
	if (!format.empty())
		return csnprintf(format.c_str(), value, std::forward<variadic_t>(args)...);
	return std::wstring();
}


} /* namespace util */

#endif /* STRINGTEMPLATES_H_ */
