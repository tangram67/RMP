/*
 * capabilities.h
 *
 *  Created on: 06.11.2020
 *      Author: dirk
 */

#ifndef SRC_INC_CAPABILITIES_H_
#define SRC_INC_CAPABILITIES_H_

#include <string>
#include <map>
#include <sys/capability.h>
#include "gcc.h"


namespace app {


#ifdef STL_HAS_TEMPLATE_ALIAS

using TCapabilityMap = std::map<std::string, cap_value_t>;
using TCapabilityItem = std::pair<std::string, cap_value_t>;

#else

typedef std::map<std::string, cap_value_t> TCapabilityMap;
typedef std::pair<std::string, cap_value_t> TCapabilityItem;

#endif

STATIC_CONST cap_value_t INVALID_CAP_VALUE = (cap_value_t)-1;

class TCapabilities {
public:
	cap_value_t getCapabilityByName(const std::string& name);

	TCapabilities();
	virtual ~TCapabilities();
};

} /* namespace app */

#endif /* SRC_INC_CAPABILITIES_H_ */
