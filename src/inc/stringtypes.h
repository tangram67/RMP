/*
 * stringtypes.h
 *
 *  Created on: 09.07.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef STRINGTYPES_H_
#define STRINGTYPES_H_

#include <functional>
#include "gcc.h"

namespace util {

enum ESortOrder { SO_ASC, SO_DESC };

#ifdef STL_HAS_TEMPLATE_ALIAS

using TStringSorter = std::function<bool(const std::string&, const std::string&)>;

#else

typedef std::function<bool(const std::string&, const std::string&)> TStringSorter;

#endif

} /* namespace util */

#endif /* STRINGTYPES_H_ */
