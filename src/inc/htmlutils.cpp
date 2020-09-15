/*
 * htmlutils.cpp
 *
 *  Created on: 25.10.2019
 *      Author: dirk
 */

#include <string>
#include <algorithm>
#include "htmlutils.h"
#include "htmlconsts.h"
#include "templates.h"

namespace html {

static const std::string STR_WORD_BREAK = html::STR_ZWSP + html::STR_WRAP;

std::string THTML::applyFlowControl(const std::string& str, const size_t pos) {
	size_t idx = 0;
	size_t count = 0;
	size_t offset = str.size();
	std::string s;
	if (!str.empty()) {
		s.reserve(3 * str.size());
		for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
			char c = *it;
			uint8_t u = (uint8_t)c;
			if (u < UINT8_C(128)) {
				if (idx >= pos && util::isMemberOf(c, '/','.',':',',',';','#','&','%','?','_')) {
					s += (c + STR_WORD_BREAK);
					count = 0;
				} else {
					++count;
					if (u == UINT8_C(0x20)) {
						count = 0;
					}
					if (count < 10) {
						s.push_back(c);
					} else {
						if (offset > 4) {
							s += (c + STR_WORD_BREAK);
							count = 0;
						} else {
							s.push_back(c);
						}
					}
				}
			}
			++idx;
			--offset;
		}
	}
	return s;
}

std::string THTML::encode(const std::string& str) {
	// See: https://wiki.selfhtml.org/wiki/HTML/Regeln/Zeichencodierung
	std::string s;
	if (!str.empty()) {
		bool cr = false;
		s.reserve(3 * str.size());
		for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
			char c = *it;
			uint8_t u = (uint8_t)c;
			if (u < UINT8_C(128)) {
				switch (c) {
					case '\"':
						s += STR_QUOTE;
						cr = false;
						break;
					case '\'':
						s += STR_APOS;
						cr = false;
						break;
					case '&':
						s += STR_AMPER;
						cr = false;
						break;
					case '<':
						s += STR_LESS;
						cr = false;
						break;
					case '>':
						s += STR_GREATER;
						cr = false;
						break;
					case '\n':
					case '\r':
						// Ignore multiple CR/LF combinations
						if (!cr) {
							s += STR_PARA;
							cr = true;
						}
						break;
					default:
						if (u < UINT8_C(0x20)) {
							s += STR_REPL;
						} else {
							s.push_back(c);
						}
						cr = false;
						break;
				}
			} else {
				cr = false;
				s.push_back(c);
			}
		}
	}
	return s;
}

} /* namespace html */
