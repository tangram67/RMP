/*
 * atoms.h
 *
 *  Created on: 16.12.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef ATOMS_H_
#define ATOMS_H_

#include "gcc.h"

#ifdef TARGET_BIG_ENDIAN
# define MAKE_ATOM( a, b, c, d ) ( ( (a) << 24 ) | ( (b) << 16 ) | ( (c) << 8 ) | (d) )
#else
# define MAKE_ATOM( a, b, c, d ) ( (a) | ( (b) << 8 ) | ( (c) << 16 ) | ( (d) << 24 ) )
#endif

#endif /* ATOMS_H_ */
