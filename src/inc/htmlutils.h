/*
 * htmlutils.h
 *
 *  Created on: 25.10.2019
 *      Author: dirk
 */

#ifndef INC_HTMLUTILS_H_
#define INC_HTMLUTILS_H_

namespace html {

class THTML {
public:
	static std::string applyFlowControl(const std::string& str, const size_t pos = 0);
	static std::string encode(const std::string& str);
};

} /* namespace html */

#endif /* INC_HTMLUTILS_H_ */
