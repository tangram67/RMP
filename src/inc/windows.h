/*
 * windows.h
 *
 *  Created on: 25.09.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef WINDOWS_H_
#define WINDOWS_H_

/****************************************************************************
 * windows.h
 *
 * Windows related data types based on Microsoft's own.
 ***************************************************************************/

/*
 * Common Data Types
 *
 * The data types in this section are essentially aliases for C/C++
 * primitive data types.
 *
 * Adapted from http://msdn.microsoft.com/en-us/library/cc230309(PROT.10).aspx.
 * See http://en.wikipedia.org/wiki/Stdint.h for more on stdint.h.
 */

#include "gcc.h"

#ifdef STL_HAS_TEMPLATE_ALIAS

using CHAR = char;
using INT  = int;
using BOOL = int;

using UCHAR = unsigned char;
using UINT  = unsigned int;
using ULONG = unsigned long;

using BYTE    = uint8_t;
using WORD    = uint16_t;
using DWORD   = uint32_t;
using QWORD   = uint64_t;
using DWORD64 = uint64_t;

using INT8     = int8_t;
using INT16    = int16_t;
using LONG     = int32_t;
using LONG64   = int64_t;
using LONGLONG = int64_t;

#else

typedef char CHAR;
typedef int  INT;
typedef int  BOOL;

typedef unsigned char UCHAR;
typedef unsigned int  UINT;
typedef unsigned long ULONG;

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef uint64_t DWORD64;

typedef int8_t	INT8;
typedef int16_t	INT16;
typedef int32_t LONG;
typedef int64_t LONG64;
typedef int64_t LONGLONG;

#endif

#endif /* WINDOWS_H_ */
