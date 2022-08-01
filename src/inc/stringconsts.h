/*
 * stringconsts.h
 *
 *  Created on: 25.04.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef STRINGCONSTS_H_
#define STRINGCONSTS_H_

#include "gcc.h"
#include "windows.h"

#include <string>

namespace util {

STATIC_CONST int DEFAULT_STRING_TAG = 0;
STATIC_CONST char BASE64_PADDING_CHAR = '=';

// See:
// http://base64.sourceforge.net/b64.c
// http://www.experts-exchange.com/articles/3216/Fast-Base64-Encode-and-Decode.html
STATIC_CONST UCHAR BASE64_LOOKUP_TABLE[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..15: gap: ctrl chars
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 16..31: gap: ctrl chars
	0,0,0,0,0,0,0,0,0,0,0,           // 32..42: gap: spc,!"#$%'()*
	62,                              // 43: +
	 0, 0, 0,                        // gap ,-.
	63,                              // /
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, // 0-9
	 0, 0, 0,                        // gap: :;<
	99,                              //  = (end padding)
	 0, 0, 0,                        // gap: >?@
	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,25,      // A-Z
	 0, 0, 0, 0, 0, 0,               // gap: [\]^_`
	26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,
	43,44,45,46,47,48,49,50,51,      // a-z
	 0, 0, 0, 0,                     // gap: {|}~ (and the rest...)
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const std::string BASE64_CODE_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// According to https://tools.ietf.org/html/rfc3986#section-2.2
static const std::string URI_GENERIC    = ":/?#[]@";
//static const std::string URI_SUB      = "!$&'()/*+,;=";
//static const std::string URI_RESERVED = URI_GENERIC + URI_SUB;
static const std::string URI_UNRESERVED = "-._~";

// Leave query separators in URIs as they are (not RFC3986 friendly)
static const std::string URI_EXTENDED   = URI_GENERIC + "&=";

}

#endif /* STRINGCONSTS_H_ */
