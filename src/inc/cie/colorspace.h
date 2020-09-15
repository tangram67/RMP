/*
 * specrend.h
 *
 *  Created on: 18.08.2019
 *      Author: dirk
 */

#ifndef INC_CIE_COLORSPACE_H_
#define INC_CIE_COLORSPACE_H_

#include "colortypes.h"

#ifdef __cplusplus
extern "C" {
#endif

const CColorSystem * get_color_system_by_name(const char *name);
int spectrum_to_rgb(const CColorSystem *cs, double *r, double *g, double *b, const double temperature);
void render_color_spectrum_by_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* INC_CIE_COLORSPACE_H_ */
