/*
 * IEEE754.h
 *
 *  Created on: 24.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef IEEE754_H_
#define IEEE754_H_

#include "gcc.h"
#include "windows.h"

namespace util {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TIEEE754 = BYTE[10];

#else

typedef BYTE TIEEE754[10];

#endif

bool convertToIeeeExtended(const double value, TIEEE754& bytes);
bool convertFromIeeeExtended(const TIEEE754& bytes, double& value);

} /* namespace util */

#endif /* IEEE754_H_ */
