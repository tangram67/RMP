/*
 * typeid.h
 *
 *  Created on: 15.03.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef TYPEID_H_
#define TYPEID_H_

#include <string>

namespace util {

std::string demangleTypeID(const char* name);

template<typename object_t>
	inline const std::string nameOf(object_t&& object) {
		return demangleTypeID(typeid(object).name());
	}


} /* namespace util */

#endif /* TYPEID_H_ */
