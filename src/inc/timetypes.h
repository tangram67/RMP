/*
 * timetypes.h
 *
 *  Created on: 04.07.2020
 *      Author: dirk
 */

#ifndef INC_TIMETYPES_H_
#define INC_TIMETYPES_H_

#include "gcc.h"

namespace util {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TTimePart = time_t;
#if defined(TARGET_X64)
using TTimeLong = TTimePart;
#else
using TTimeLong = int64_t;
#endif
using PTimePart = TTimePart*;
using TTimeNumeric = long double;

#else

typedef time_t TTimePart;
#if defined(TARGET_X64)
typedef TTimePart TTimeLong;
#else
typedef int64_t TTimeLong;
#endif
typedef TTimePart* PTimePart;
typedef long double TTimeNumeric;

#endif

} /* namespace util */

#endif /* INC_TIMETYPES_H_ */
