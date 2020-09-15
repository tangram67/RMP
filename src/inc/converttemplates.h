/*
 * converttemplates.h
 *
 *  Created on: 18.06.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef CONVERTTEMPLATES_H_
#define CONVERTTEMPLATES_H_

#include <string>

namespace util {

template<typename value_t>
inline std::string valueToBinary(value_t value)
{
	value_t sh = 1;
	std::string s;
    ssize_t i, size = 8 * sizeof(value);
    s.resize(size, '0');
    for (i = util::pred(s.size()); i >= 0; i--) {
    	if (value & sh)
    		s[i] = '1';
    	sh <<= 1;
    }
    return s;
}

} /* namespace util */

#endif /* CONVERTTEMPLATES_H_ */
