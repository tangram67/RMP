/*
 * version.h
 *
 *  Created on: 14.01.2020
 *      Author: dirk
 */

#ifndef INC_VERSION_H_
#define INC_VERSION_H_

#ifdef HAVE_CONFIG_H
#  include "../../autoconf.h"
#endif

// The version string is derived from 2 sources:
// 1. Version macro set by Eclipse during build time (SVN_REV)
// 2. Derived from Autotools configuration header (APP_VER)
#ifdef APP_VER
#  ifndef SVN_REV
#    define SVN_REV APP_VER
#    define SVN_BLD "A"
#  endif
#else
#  ifndef SVN_REV
#    define SVN_REV "0001"
#    define SVN_BLD "N"
#  endif
#endif

#endif /* INC_VERSION_H_ */
