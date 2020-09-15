/*
 * digesttypes.h
 *
 *  Created on: 27.05.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef DIGESTTYPES_H_
#define DIGESTTYPES_H_

#include "gcc.h"
#include "nullptr.h"

namespace util {

class TSSLContext;
class TSSLConnection;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PSSLContext = TSSLContext*;
using PSSLConnection = TSSLConnection*;

#else

typedef TSSLContext* PSSLContext;
typedef TSSLConnection* PSSLConnection;

#endif


enum EContextType
{
	ECT_UNKNOWN,
	ECT_SERVER,
	ECT_CLIENT
};

enum EReportType
{
	ERT_DIGIT,
	ERT_BASE64,
	ERT_HEX,
	ERT_HEX_SHORT,
	ERT_HEX_NCASE,
	ERT_HEX_SHORT_NCASE
};

enum EDigestType
{
	EDT_UNKNOWN = 0,
	EDT_MD5     = 128,
	EDT_SHA1    = 160,
	EDT_SHA224  = 224,
	EDT_SHA256  = 256,
	EDT_SHA384  = 384,
	EDT_SHA512  = 512
};

enum EDigestSize
{
	EDS_MD5     = EDT_MD5 / 8,
	EDS_SHA1    = EDT_SHA1 / 8,
	EDS_SHA224  = EDT_SHA224 / 8,
	EDS_SHA256  = EDT_SHA256 / 8,
	EDS_SHA384  = EDT_SHA384 / 8,
	EDS_SHA512  = EDT_SHA512 / 8
};

struct CDigestNames {
	EDigestType type;
	const char* name;
};

// "MD5", "SHA1", "SHA224", "SHA256", "SHA384", "SHA512"
STATIC_CONST CDigestNames DIGEST_NAMES[] = {
	{ EDT_UNKNOWN, "UNKNOWN" },	// 0
	{ EDT_MD5,     "MD5"     },	// 1
	{ EDT_SHA1,    "SHA1"    },	// 2
	{ EDT_SHA224,  "SHA224"  },	// 3
	{ EDT_SHA256,  "SHA256"  },	// 4
	{ EDT_SHA384,  "SHA384"  },	// 5
	{ EDT_SHA512,  "SHA512"  },	// 6
	{ EDT_UNKNOWN,  nil      }  // EOT
};

} /* namespace util */

#endif /* DIGESTTYPES_H_ */
