/*
 * macros.h
 *
 *  Created on: 04.10.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef MACROS_H_
#define MACROS_H_

#include "gcc.h"

#ifdef TARGET_BIG_ENDIAN
# define MAKE_ATOM( a, b, c, d ) ( ( (a) << 24 ) | ( (b) << 16 ) | ( (c) << 8 ) | (d) )
#else
# define MAKE_ATOM( a, b, c, d ) ( (a) | ( (b) << 8 ) | ( (c) << 16 ) | ( (d) << 24 ) )
#endif

#endif /* MACROS_H_ */
