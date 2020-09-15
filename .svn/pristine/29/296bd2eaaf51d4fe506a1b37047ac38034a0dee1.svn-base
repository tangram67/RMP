/*
 * vartypes.h
 *
 *  Created on: 28.06.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef VARTYPES_H_
#define VARTYPES_H_

#include "gcc.h"

namespace util {

class TVariant;
class TBlob;

#ifdef STL_HAS_TEMPLATE_ALIAS

using TDoublePrecision = uint;
using PVariant = TVariant*;
using PBlob = TBlob*;

#else

typedef uint TDoublePrecision;
typedef TVariant* PVariant;
typedef TBlob* PBlob;

#endif


enum EVariantType
{
	EVT_INVALID,
	EVT_UNKNOWN,
	EVT_INTEGER8,
	EVT_INTEGER16,
	EVT_INTEGER32,
	EVT_UNSIGNED8,
	EVT_UNSIGNED16,
	EVT_UNSIGNED32,
	EVT_INTEGER64,
	EVT_UNSIGNED64,
	EVT_BOOLEAN,
	EVT_STRING,
	EVT_WIDE_STRING,
	EVT_DOUBLE,
	EVT_FLOAT = EVT_DOUBLE,
	EVT_TIME,
	EVT_BLOB,
	EVT_NULL,
	EVT_INTEGER = EVT_INTEGER32,
	EVT_UNSIGNED = EVT_INTEGER32
};

enum EBooleanType {
	VBT_BLYES,
	VBT_BLGER,
	VBT_BLTRUE,
	VBT_BL01,
	VBT_BLON,
	VBT_LOCALE,
	VBT_BLDEFAULT = VBT_BLTRUE
};

} /* namespace util */

#endif /* VARTYPES_H_ */
