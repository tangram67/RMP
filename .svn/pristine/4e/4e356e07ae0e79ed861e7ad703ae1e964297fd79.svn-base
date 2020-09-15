/*
 * functors.h
 *
 *  Created on: 14.06.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef FUNCTORS_H_
#define FUNCTORS_H_

#include "gcc.h"
#include "ASCII.h"
#include <cstdlib>

#ifdef STL_HAS_TEMPLATE_ALIAS
using TIsWhiteSpaceA = std::function<int(char)>;
using TIsWhiteSpaceW = std::function<int(wchar_t)>;
#else
typedef std::function<int(char)> TIsWhiteSpaceA;
typedef std::function<int(wchar_t)> TIsWhiteSpaceW;
#endif

namespace util {

inline bool checkFailed(const int errnum) {
    return (EXIT_SUCCESS != errnum);
}

inline bool checkSucceeded(const int errnum) {
    return (EXIT_SUCCESS == errnum);
}

// Not really the correct name for this purpose...
inline int isControlCharA(unsigned char c) {
	return (c < USPC && c != ULF && c != UHT);
}
inline int isControlChar(unsigned char c) { return isControlCharA(c); }

inline int isControlCharA(char c) {
	unsigned char u = (unsigned char)c;
	return isControlCharA(u);
}
inline int isControlChar(char c) { return isControlCharA(c); }


inline int isLineBrakeA(char c) {
	unsigned char u = (unsigned char)c;
	return (u < USPC);
}
inline int isLineBrake(char c) { return isLineBrakeA(c); }


inline int isWhiteSpaceA(char c) {
	unsigned char u = (unsigned char)c;
	return (u <= USPC);
}
inline int isWhiteSpace(char c) { return isWhiteSpaceA(c); }


inline int isUnsignedA(char c) {
	unsigned char u = (unsigned char)c;
	if ((u >= (unsigned char)'0') && (u <= (unsigned char)'9'))
		return true;
	return false;
}
inline int isUnsigned(char c) { return isUnsignedA(c); }


inline int isPrefixA(char c) {
	unsigned char u = (unsigned char)c;
	if (u == (unsigned char)'-')
		return true;
	if (u == (unsigned char)'+')
		return true;
	return false;
}
inline int isPrefix(char c) { return isPrefixA(c); }


inline int isIntegerA(char c) {
	if (isUnsignedA(c))
		return true;
	if (isPrefixA(c))
		return true;
	return false;
}
inline int isInteger(char c) { return isIntegerA(c); }


inline int isNumericA(char c) {
	unsigned char u = (unsigned char)c;
	if (isIntegerA(c))
		return true;
	if (u == (unsigned char)'.')
		return true;
	if (u == (unsigned char)'E')
		return true;
	if (u == (unsigned char)'e')
		return true;
	return false;
}
inline int isNumeric(char c) { return isNumericA(c); }


inline int isAlphaA(char c) {
	unsigned char u = (unsigned char)c;
	if ((u >= (unsigned char)'A') && (u <= (unsigned char)'Z'))
		return true;
	if ((u >= (unsigned char)'a') && (u <= (unsigned char)'z'))
		return true;
	return false;
}
inline int isAlpha(char c) { return isAlphaA(c); }


inline int isHexA(char c) {
	unsigned char u = (unsigned char)c;
	if ((u >= (unsigned char)'A') && (u <= (unsigned char)'F'))
		return true;
	if ((u >= (unsigned char)'a') && (u <= (unsigned char)'f'))
		return true;
	return false;
}
inline int isHex(char c) { return isHexA(c); }


inline int isAlphaNumericA(char c) {
	if (isNumericA(c))
		return true;
	if (isAlphaA(c))
		return true;
	return false;
}
inline int isAlphaNumeric(char c) { return isAlphaNumericA(c); }


inline int isHexaDecimalA(char c) {
	if (isUnsignedA(c))
		return true;
	if (isHexA(c))
		return true;
	return false;
}
inline int isHexaDecimal(char c) { return isHexaDecimalA(c); }


inline int isNonAlphaNumericA(char c) {
	unsigned char u = (unsigned char)c;
	if (isUnsignedA(c))
		return false;
	return (u < (unsigned char)'A');
}
inline int isNonAlphaNumeric(char c) { return isNonAlphaNumericA(c); }



inline int isControlCharW(char c) {
	return (c < WSPC && c != WLF && c != WHT);
}

inline int isWhiteSpaceW(wchar_t c) {
	return (c <= WSPC);
}

inline int isUnsignedW(wchar_t c) {
	if ((c >= L'0') && (c <= L'9'))
		return true;
	return false;
}

inline int isPrefixW(wchar_t c) {
	if (c == L'-')
		return true;
	if (c == L'+')
		return true;
	return false;
}

inline int isIntegerW(wchar_t c) {
	if (isUnsignedW(c))
		return true;
	if (isPrefixW(c))
		return true;
	return false;
}

inline int isNumericW(wchar_t c) {
	if (isIntegerW(c))
		return true;
	if (c == L'.')
		return true;
	if (c == L'E')
		return true;
	if (c == L'e')
		return true;
	return false;
}

inline int isAlphaW(wchar_t c) {
	if (isNumericW(c))
		return true;
	if ((c >= L'A') && (c <= L'Z'))
		return true;
	if ((c >= L'a') && (c <= L'z'))
		return true;
	return false;
}

inline int isNonAlphaNumericW(wchar_t c) {
	if ((c >= L'0') && (c <= L'9'))
		return false;
	return (c < L'A');
}


inline int isValidFormatQualifierA(char format, char next) {
	return ((format == '%' && next != '%') || (format == '$' && next != '$') || (format == '@' && next != '@'));
}

inline int isValidFormatQualifierW(wchar_t format, wchar_t next) {
	return ((format == L'%' && next != L'%') || (format == L'$' && next != L'$'));
}

inline int isDuplicatedFormatQualifierA(char format, char next) {
	return ((format == '%' && next == '%') || (format == '$' && next == '$') || (format == '@' && next == '@'));
}

inline int isDuplicatedFormatQualifierW(wchar_t format, wchar_t next) {
	return ((format == L'%' && next == L'%') || (format == L'$' && next == L'$'));
}

} // namespace util

#endif /* FUNCTORS_H_ */
