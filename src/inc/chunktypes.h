/*
 * chunktypes.h
 *
 *  Created on: 25.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef CHUNKTYPES_H_
#define CHUNKTYPES_H_

#include "gcc.h"
#include "windows.h"

namespace music {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TInt16 = BYTE[2]; // 16 Bit unsigned Integer used in DSDIFF
using TInt28 = BYTE[4]; // 28 Bit unsigned Integer used in ID3v2
using TInt32 = BYTE[4]; // 32 Bit unsigned Integer in DSD Header
using TInt64 = BYTE[8]; // 64 Bit unsigned Integer in DSD Header

using TChunkID = CHAR[4];
using TTypeID  = CHAR[4];

#else

typedef BYTE TInt16[2]; // 16 Bit unsigned Integer used in DSDIFF
typedef BYTE TInt28[4]; // 28 Bit unsigned Integer used in ID3v2
typedef BYTE TInt32[4]; // 32 Bit unsigned Integer in DSD Header
typedef BYTE TInt64[8]; // 64 Bit unsigned Integer in DSD Header

typedef CHAR TChunkID[4];
typedef CHAR TTypeID[4];

#endif

} /* namespace music */

#endif /* CHUNKTYPES_H_ */
