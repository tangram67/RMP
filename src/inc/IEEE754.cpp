/*
 * IEEE754.cpp
 *
 *  Created on: 24.09.2016
 *      Author: Dirk Brinkmeier
 */

#include <fenv.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include "gcc.h"
#include "IEEE754.h"

// See http://www.onicos.com/staff/iz/formats/ieee.c

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /*HUGE_VAL*/

/*
 * C O N V E R T   T O   I E E E   E X T E N D E D
 */

/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#define FLOAT_TO_UNSIGNED(f) ((unsigned long)(((long)(f - 2147483648.0)) + 2147483647L) + 1)

bool util::convertToIeeeExtended(const double value, TIEEE754& bytes)
{
	int sign;
	int expon;
	double fMant, fsMant, fValue = value;
	unsigned long hiMant, loMant;

	for (size_t i=0; i<sizeof(TIEEE754); ++i)
		bytes[i] = 0;

	if (fValue < 0) {
		sign = 0x8000;
		fValue *= -1;
	} else {
		sign = 0;
	}

	if (fValue == 0) {
		expon = 0; hiMant = 0; loMant = 0;
	} else {

		feclearexcept(FE_ALL_EXCEPT);
		fMant = frexp(fValue, &expon);
		if (0 != fetestexcept(FE_ALL_EXCEPT))
			return false;

		if ((expon > 16384) || !(fMant < 1)) {    /* Infinity or NaN */
			expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
		} else {    /* Finite */
			expon += 16382;
			if (expon < 0) {    /* denormalized */

				feclearexcept(FE_ALL_EXCEPT);
				fMant = ldexp(fMant, expon);
				if (0 != fetestexcept(FE_ALL_EXCEPT))
					return false;

				expon = 0;
			}
			expon |= sign;

			feclearexcept(FE_ALL_EXCEPT);
			fMant = ldexp(fMant, 32);
			if (0 != fetestexcept(FE_ALL_EXCEPT))
				return false;

			fsMant = floor(fMant);
			hiMant = FLOAT_TO_UNSIGNED(fsMant);

			feclearexcept(FE_ALL_EXCEPT);
			fMant = ldexp(fMant - fsMant, 32);
			if (0 != fetestexcept(FE_ALL_EXCEPT))
				return false;

			fsMant = floor(fMant);
			loMant = FLOAT_TO_UNSIGNED(fsMant);
		}
	}

	bytes[0] = expon >> 8;
	bytes[1] = expon;
	bytes[2] = hiMant >> 24;
	bytes[3] = hiMant >> 16;
	bytes[4] = hiMant >> 8;
	bytes[5] = hiMant;
	bytes[6] = loMant >> 24;
	bytes[7] = loMant >> 16;
	bytes[8] = loMant >> 8;
	bytes[9] = loMant;

	return true;
}


/*
 * C O N V E R T   F R O M   I E E E   E X T E N D E D
 */

/*
 * Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#define UNSIGNED_TO_FLOAT(u) (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)

/****************************************************************
 * Extended precision IEEE floating-point conversion routine.
 ****************************************************************/

bool util::convertFromIeeeExtended(const TIEEE754& bytes, double& value)
{
	double f;
	int expon;
	unsigned long hiMant, loMant;
	value = 0.0;

	expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
	hiMant = ((unsigned long)(bytes[2] & 0xFF) << 24)
            | ((unsigned long)(bytes[3] & 0xFF) << 16)
            | ((unsigned long)(bytes[4] & 0xFF) << 8)
            | ((unsigned long)(bytes[5] & 0xFF));
	loMant = ((unsigned long)(bytes[6] & 0xFF) << 24)
            | ((unsigned long)(bytes[7] & 0xFF) << 16)
            | ((unsigned long)(bytes[8] & 0xFF) << 8)
            | ((unsigned long)(bytes[9] & 0xFF));

	if (expon == 0 && hiMant == 0 && loMant == 0) {
		f = 0;
	} else {
		if (expon == 0x7FFF) {    /* Infinity or NaN */
			f = HUGE_VAL;
		} else {
			expon -= 16383;

			feclearexcept(FE_ALL_EXCEPT);
			f  = ldexp(UNSIGNED_TO_FLOAT(hiMant), expon-=31);
			if (0 != fetestexcept(FE_ALL_EXCEPT))
				return false;

			feclearexcept(FE_ALL_EXCEPT);
			f += ldexp(UNSIGNED_TO_FLOAT(loMant), expon-=32);
			if (0 != fetestexcept(FE_ALL_EXCEPT))
				return false;

		}
	}

	if (bytes[0] & 0x80) {
		value = -f;
	} else {
		value = f;
	}

	return true;
}
