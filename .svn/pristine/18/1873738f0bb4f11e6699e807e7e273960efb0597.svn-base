/*
 * limits.h
 *
 *  Created on: 21.08.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef NUMLIMITS_H_
#define NUMLIMITS_H_

#ifndef __STDC_LIMIT_MACROS
#	define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <limits.h>
#include <limits>
#include "gcc.h"

#ifdef STL_HAS_NUMERIC_LIMITS

#	define VAR_INT8_MAX (std::numeric_limits<int8_t>::max())
#	define VAR_INT8_MIN (std::numeric_limits<int8_t>::min())

#	define VAR_UINT8_MAX (std::numeric_limits<uint8_t>::max())
#	define VAR_UINT8_MIN (std::numeric_limits<uint8_t>::min())

#	define VAR_INT16_MAX (std::numeric_limits<int16_t>::max())
#	define VAR_INT16_MIN (std::numeric_limits<int16_t>::min())

#	define VAR_UINT16_MAX (std::numeric_limits<uint16_t>::max())
#	define VAR_UINT16_MIN (std::numeric_limits<uint16_t>::min())

#	define VAR_INT32_MAX (std::numeric_limits<int32_t>::max())
#	define VAR_INT32_MIN (std::numeric_limits<int32_t>::min())

#	define VAR_UINT32_MAX (std::numeric_limits<uint32_t>::max())
#	define VAR_UINT32_MIN (std::numeric_limits<uint32_t>::min())

#	define VAR_INT64_MAX (std::numeric_limits<int64_t>::max())
#	define VAR_INT64_MIN (std::numeric_limits<int64_t>::min())

#	define VAR_UINT64_MAX (std::numeric_limits<uint64_t>::max())
#	define VAR_UINT64_MIN (std::numeric_limits<uint64_t>::min())

#else

#	define VAR_INT8_MAX (CHAR_MAX)
#	define VAR_INT8_MIN (CHAR_MIN)

#	define VAR_UINT8_MAX (UCHAR_MAX)
#	define VAR_UINT8_MIN (UINT8_C(0))

#	define VAR_INT16_MAX (SHRT_MAX)
#	define VAR_INT16_MIN (SHRT_MIN)

#	define VAR_UINT16_MAX (USHRT_MAX)
#	define VAR_UINT16_MIN (UINT16_C(0))

#	define VAR_INT32_MAX (INT_MAX)
#	define VAR_INT32_MIN (INT_MIN)

#	define VAR_UINT32_MAX (UINT_MAX)
#	define VAR_UINT32_MIN (UINT32_C(0))

#	define VAR_INT64_MAX (INT64_MAX)
#	define VAR_INT64_MIN (INT64_MIN)

#	define VAR_UINT64_MAX (UINT64_MAX)
#	define VAR_UINT64_MIN (UINT64_C(0))

#endif

namespace util {

class TLimits {
public:
	STATIC_CONST int8_t LIMIT_INT8_MAX = static_cast<int8_t>(VAR_INT8_MAX);
	STATIC_CONST int8_t LIMIT_INT8_MIN = static_cast<int8_t>(VAR_INT8_MIN);

	STATIC_CONST uint8_t LIMIT_UINT8_MAX = static_cast<uint8_t>(VAR_UINT8_MAX);
	STATIC_CONST uint8_t LIMIT_UINT8_MIN = static_cast<uint8_t>(VAR_UINT8_MIN);

	STATIC_CONST int16_t LIMIT_INT16_MAX = static_cast<int16_t>(VAR_INT16_MAX);
	STATIC_CONST int16_t LIMIT_INT16_MIN = static_cast<int16_t>(VAR_INT16_MIN);

	STATIC_CONST uint16_t LIMIT_UINT16_MAX = static_cast<uint16_t>(VAR_UINT16_MAX);
	STATIC_CONST uint16_t LIMIT_UINT16_MIN = static_cast<uint16_t>(VAR_UINT16_MIN);

	STATIC_CONST int32_t LIMIT_INT32_MAX = static_cast<int32_t>(VAR_INT32_MAX);
	STATIC_CONST int32_t LIMIT_INT32_MIN = static_cast<int32_t>(VAR_INT32_MIN);

	STATIC_CONST uint32_t LIMIT_UINT32_MAX = static_cast<uint32_t>(VAR_UINT32_MAX);
	STATIC_CONST uint32_t LIMIT_UINT32_MIN = static_cast<uint32_t>(VAR_UINT32_MIN);

	STATIC_CONST int64_t LIMIT_INT64_MAX = static_cast<int64_t>(VAR_INT64_MAX);
	STATIC_CONST int64_t LIMIT_INT64_MIN = static_cast<int64_t>(VAR_INT64_MIN);

	STATIC_CONST uint64_t LIMIT_UINT64_MAX = static_cast<uint64_t>(VAR_UINT64_MAX);
	STATIC_CONST uint64_t LIMIT_UINT64_MIN = static_cast<uint64_t>(VAR_UINT64_MIN);

	// "Not A Number" (nan) default constants
	STATIC_CONST int8_t  nan8 = LIMIT_INT8_MAX;
	STATIC_CONST uint8_t unan8 = LIMIT_UINT8_MAX;
	STATIC_CONST int16_t  nan16 = LIMIT_INT16_MAX;
	STATIC_CONST uint16_t unan16 = LIMIT_UINT16_MAX;
	STATIC_CONST int32_t  nan32 = LIMIT_INT32_MAX;
	STATIC_CONST uint32_t unan32 = LIMIT_UINT32_MAX;
	STATIC_CONST int64_t  nan64 = LIMIT_INT64_MAX;
	STATIC_CONST uint64_t unan64 = LIMIT_UINT64_MAX;

	// Default constants for 32 bit
	STATIC_CONST int32_t  nan = nan32;
	STATIC_CONST uint32_t unan = unan32;
};

} /* namespace util */

#endif /* NUMLIMITS_H_ */
