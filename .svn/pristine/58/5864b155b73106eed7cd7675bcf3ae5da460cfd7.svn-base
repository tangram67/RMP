/*
 * colortypes.h
 *
 *  Created on: 20.08.2019
 *      Author: dirk
 */

#ifndef INC_CIE_COLORTYPES_H_
#define INC_CIE_COLORTYPES_H_

/*
   A color system is defined by the CIE x and y coordinates of
   its three primary illuminants and the x and y coordinates of
   the white point.
*/
typedef struct SColorSystem {
    const char *name;        /* color system name */
    const char *description; /* color system descrition */
    double xRed, yRed,       /* Red x, y */
           xGreen, yGreen,   /* Green x, y */
           xBlue, yBlue,     /* Blue x, y */
           xWhite, yWhite,   /* White point x, y */
           gamma;            /* Gamma correction for system */
} CColorSystem;

#endif /* INC_CIE_COLORTYPES_H_ */
