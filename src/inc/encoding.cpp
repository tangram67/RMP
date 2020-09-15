/*
 * encoding.cpp
 *
 *  Created on: 20.04.2020
 *      Author: dirk
 */

#include "gcc.h"
#include "encoding.h"
#include "stringutils.h"

namespace util {

STATIC_CONST char base36LookupTableU[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
STATIC_CONST char base36LookupTableL[37] = "0123456789abcdefghijklmnopqrstuvwxyz";

std::string TBase36::randomize(const std::string& preamble, const size_t digits, const bool upper) {
	std::string id;
	id.reserve(preamble.size() + digits);
	const char * lookup = upper ? base36LookupTableU : base36LookupTableL;
	for (size_t i=0; i<digits; ++i)
		id += lookup[rand() % 36];
	if (preamble.empty())
		return id;
	return preamble + id;
}

std::string TBase36::encode(const uint64_t value, const bool upper) {
	std::string dst;
	encode(value, dst, upper);
	return dst;
}

void TBase36::encode(const uint64_t value, std::string& dst, const bool upper) {
	// log(2**64) / log(36) = 12.38 => max 13 char + '\0'
	dst.clear();
	dst.reserve(14);
	const char * lookup = upper ? base36LookupTableU : base36LookupTableL;
	uint64_t val = value;
	do {
		// Natural display as big endian...
		dst = lookup[val % 36] + dst;
	} while (val /= 36);
}

uint64_t TBase36::decode(const char *src, const size_t size, const uint64_t defVal) {
	if (util::assigned(src)) {
		char* p;
		errno = EXIT_SUCCESS;
		uint64_t r = (uint64_t)strtoul(src, &p, 36);
		if (EXIT_SUCCESS == errno && ((src + size) == p)) {
			return r;
		}
	}
	return defVal;
}

uint64_t TBase36::decode(const std::string& src, const uint64_t defVal) {
	if (!src.empty())
		return decode(src.c_str(), src.size(), defVal);
	return defVal;
}



std::string TBase64::encode(const char* src, size_t size) {
	std::string dst;
	encode(src, size, dst);
	return dst;
}

void TBase64::encode(const std::string& src, std::string& dst) {
	if (!src.empty())
		encode(src.c_str(), src.size(), dst);
	dst.clear();
}

std::string TBase64::encode(const std::string& src) {
	std::string dst;
	encode(src, dst);
	return dst;
}

std::string TBase64::decode(const char* src, size_t size) {
	std::string dst;
	decode(src, size, dst);
	return dst;
}

void TBase64::decode(const std::string& src, std::string& dst) {
	if (!src.empty()) {
		size_t size = src.find_last_of(BASE64_PADDING_CHAR);
		if (size == std::string::npos)
			size = src.size();
		decode(src.c_str(), size, dst);
	} else {
		dst.clear();
	}
}

std::string TBase64::decode(const std::string& src) {
	std::string dst;
	decode(src, dst);
	return dst;
}





bool TURL::isValidRFC3986Delimiter(const char c) {
	return (URI_UNRESERVED.find(c) != std::string::npos);
}


bool TURL::isUriDelimiter(const char c, const EEncodeType type) {
	if (type == URL_RFC3986)
		return false; // Be RFC3986 friendly

	/*
	 * Leave syntax components as they are:
	 * https://tools.ietf.org/html/rfc3986#section-3
	 *
	 *    foo://example.com:8042/over/there?name=ferret#nose
	 *    \_/   \______________/\_________/ \_________/ \__/
	 *     |           |            |            |        |
	 *  scheme     authority       path        query   fragment
	 *     |   _____________________|__
	 *    / \ /                        \
	 *    urn:example:animal:ferret:nose
	 *
	 * Examples:
	 *   http://joe:passwd@www.example.net:8080/index.html?action=something&session=A54C6FE2#info
	 *   ldap://[2001:db8::7]/c=GB?objectClass?one
	 */
	return (URI_EXTENDED.find(c) != std::string::npos);
}

bool TURL::isWhiteSpace(const char c, const ESpaceTrait trait) {
	if (c == ' ' && trait == UST_REPLACED)
		return false; // Encode ' ' to '+'
	return util::isWhiteSpaceA(c);
}

bool TURL::isAlphaNumeric(const char c) {
	unsigned char u = (unsigned char)c;
	if ((u >= (unsigned char)'0') && (u <= (unsigned char)'9'))
		return true;
	if ((u >= (unsigned char)'A') && (u <= (unsigned char)'Z'))
		return true;
	if ((u >= (unsigned char)'a') && (u <= (unsigned char)'z'))
		return true;
	return false;
}


std::string TURL::decode(const std::string& url) {

	// Result size is always less than source size
	std::string retVal;
	retVal.reserve(url.length());

	for (size_t i = 0; i < url.size(); i++) {
		int c = (unsigned char)url[i];
		if (c == '+') {
			retVal += ' ';
		} else {
			if (c == '%') {
				if (i < url.size() - 2) {
					int value = -1;
					std::string s;
					s.assign(url.substr(i + 1, 2));
					sscanf(s.c_str(), "%x", &value);
					if (value < 0 || value > 255) {
						retVal += c;
					} else {
						retVal += (char)value;
						i += 2;
					}
				} // else ... ignore invalid %x at end of string!
			} else
				retVal += c;
		}
	}

	return retVal;
}


std::string TURL::encode(const std::string& url, const EEncodeType type, const ESpaceTrait trait) {
	std::string retVal;
	retVal.reserve(url.length() * 2);

	for (size_t i = 0; i < url.size(); i++) {
		const char c = url[i];

		//
		// Leave URI delimiters - _ . and ~ as they are (according to RFC3986)
		// All input characters that are not a-z, A-Z, 0-9, '-', '.', '_' or '~'
		// are converted to their "URL escaped" version (%NN where NN is a two-digit hexadecimal number).
		//
		// https://en.wikipedia.org/wiki/Percent-encoding
		// RFC 3986 section 2.2 Reserved Characters (January 2005)
		//  ! * ' ( ) ; : @ & = + $ , / ? # [ ]
		//
		// RFC 3986 section 2.3 Unreserved Characters (January 2005)
		//  letters [A-Z, a-z], digits [0-9] and - _ . ~
		//
		// Space a.k.a. blank " " is encoded as %20, but:
	    // http://example.com/blue+light%20blue?blue%2Blight+blue (TODO?)
		// --> Replace blank/space with %20 before the query separator ? and with "+" after
		//
		if ((isAlphaNumeric(c) ||
			isValidRFC3986Delimiter(c))
			&& !isUriDelimiter(c, type)
			&& !isWhiteSpace(c, trait))
		{
			retVal.push_back(c);
		} else {
			if (trait == UST_ENCODED || c != ' ')
				retVal += util::cprintf("%%%02.2X", (unsigned int)((unsigned char)c));
			else
				retVal.push_back('+');
		}
	}

	return retVal;
}

} /* namespace util */
