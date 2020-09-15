/*
 * stringutil.cpp
 *
 *  Created on: 30.05.2015
 *      Author: Dirk Brinkmeier
 *
 */

#include "stringconsts.h"
#include "stringutils.h"
#include "fileutils.h"
#include "htmlutils.h"
#include "exception.h"
#include "compare.h"
#include "windows.h"
#include "locale.h"
#include "../config.h"
#include <fstream>
#include <iterator>

namespace util {

TStringList::TStringList() {
}

TStringList::~TStringList() {
}

TStringList::TStringList(const TStringList& list) {
	assign(list);
}

TStringList::TStringList(const std::string& csv, const char delimiter) {
	util::split(csv, *this, delimiter);
}

void TStringList::add(const std::string& value) {
	app::TStringVector::push_back(value);
}

void TStringList::add(const char* data, const size_t size) {
	if (util::assigned(data) && size > 0) {
		add(std::string(data, size));
	}
}

void TStringList::add(const util::TStringList& list) {
	if (!list.empty()) {
		app::TStringVector::const_iterator it = list.begin();
		for(; it != list.end(); ++it) {
			add(*it);
		}
	}
}

bool TStringList::remove(const std::string& value) {
	app::TStringVector::const_iterator it = begin();
	for(; it != end(); ++it) {
		if (*it == value) {
			erase(it);
			return true;
		}
	}
	return false;
}

void TStringList::push_front(const std::string& value) {
	insert(begin(), value);
}

std::string TStringList::asString(const char delimiter, const EExportType type) const {
	std::string s;
	if (size() > 0) {
		bool html = type == ET_EXPORT_HTML;
		bool trimmed = util::isMemberOf(type, ET_EXPORT_TRIMMED,ET_EXPORT_HTML);
		bool isWindows = delimiter == '\r';
		bool isNone = delimiter == NUL;
		const_iterator last = this->last();
		const_iterator it = begin();

		// Calculate estimated destination size
		size_t n = 0;
		for (; it != end(); ++it)
			n += (*it).size();
		s.reserve(n + 2 * size());

		// Add trimmed strings
		it = begin();
		if (size() > 1) {
			for(; it != last; ++it) {
				if (html) {
					if (isWindows) {
						s += html::THTML::encode(trim(*it)) + "\r\n";
					} else {
						s += isNone ? html::THTML::encode(trim(*it)) : html::THTML::encode(trim(*it)) + delimiter;
					}
				} else {
					if (isWindows) {
						if (trimmed) {
							s += trim(*it) + "\r\n";
						} else {
							s += *it + "\r\n";
						}
					} else {
						if (trimmed) {
							s += isNone ? trim(*it) : trim(*it) + delimiter;
						} else {
							s += isNone ? *it : *it + delimiter;
						}
					}
				}
			}
		}
		if (html) {
			if (isWindows) {
				s += html::THTML::encode(trim(*it)) + "\r\n";
			} else {
				s += isNone ? html::THTML::encode(trim(*it)) : html::THTML::encode(trim(*it)) + delimiter;
			}
		} else {
			if (trimmed) {
				s += trim(*it);
			} else {
				s += *it;
			}
		}
	}
	return s;
}


size_t TStringList::readText(void const * const data, size_t const size) {
	clear();
	if (util::assigned(data) && size > 0) {
		const char* p = (const char*)data;
		const char* q = nil;
		size_t len = 0;
		int state = 0;
		for (size_t i=0; i<size; ++i, ++p) {
			switch (state) {
				case 0:
					if (!isLineBrakeA(*p)) {
						// First ASCII char found
						len = 1;
						state = 10;
						q = p;
					}
					break;
				case 10:
					if (isLineBrakeA(*p)) {
						// EOL found
						add(q, len);
						state = 0;
					}
					++len;
					break;
				default:
					break;
			}
		}
		if (state == 10 && len > 0) {
			// Add last string without line break
			add(q, len);
		}
		compress();
	}
	return this->size();
}

size_t TStringList::readBinary(void const * const data, size_t const size) {
	clear();
	if (util::assigned(data) && size > 0) {
		const char* p = (const char*)data;
		std::string line;
		bool next = false;
		bool wrap;
		char c;
		for (size_t i=0; i<size; ++i, ++p) {
			c = *p;
			wrap = next;
			next = false;
			if (c == '\r' || c == '\n') {
				line += util::TBinaryConvert::binToAscii(c);
				next = true;
			}
			if (wrap && !next) {
				add(line);
				line.clear();
			}
			if (!next) {
				line += util::TBinaryConvert::binToAscii(c);
			}
		}
		if (!line.empty())
			add(line);
	}
	return this->size();
}


void TStringList::clear() {
	app::TStringVector::clear();
	invalidate();
}

void TStringList::invalidate() {
	if (!strText.empty())
		strText.clear();
	if (!strRaw.empty())
		strRaw.clear();
	if (!strCSV.empty())
		strCSV.clear();
	if (!strTSV.empty())
		strTSV.clear();
	if (!strHTML.empty())
		strHTML.clear();
	if (!strTable.empty())
		strTable.clear();
}

const std::string& TStringList::csv(const char delimiter) const {
	if (strCSV.empty() && !empty()) {
		strCSV = asString(delimiter, ET_EXPORT_TRIMMED);
	}
	return strCSV;
}

const std::string& TStringList::tsv(const char delimiter) const {
	if (strTSV.empty() && !empty()) {
		strTSV = asString(delimiter, ET_EXPORT_TRIMMED);
	}
	return strTSV;
}

const std::string& TStringList::text(const char delimiter) const {
	if (strText.empty() && !empty())
		strText = asString(delimiter, ET_EXPORT_TRIMMED);
	return strText;
}

const std::string& TStringList::raw(const char delimiter) const {
	if (strRaw.empty() && !empty())
		strRaw = asString(delimiter, ET_EXPORT_PLAIN);
	return strRaw;
}

const std::string& TStringList::html() const {
	if (strHTML.empty() && !empty())
		strHTML = "<!-- TStringList::html::begin -->\n" + asString('\n', ET_EXPORT_PLAIN) + "<!-- TStringList::html::end -->\n";
	return strHTML;
}

const std::string& TStringList::table(const size_t emphasize) const {
	if (strTable.empty() && !empty()) {
		const_iterator it;

		// Calculate estimated destination size
		size_t n = 0;
		for (it = begin(); it != end(); ++it)
			n += (*it).size();
		strTable.reserve(n + 2 * size());

		// Create HTML table
		size_t i = 0;
		strTable += "<table style=\"display: table; width: 100%;\">\n";
		strTable += " <tbody>\n";
		for (it = begin(); it != end(); ++it, ++i) {
			strTable += "  <tr>\n";
			strTable += "   <td>\n";
			if (emphasize == i) {
				strTable += "    <strong>" + html::THTML::encode(trim(*it)) + "</strong>\n";
			} else {
				strTable += "    " + html::THTML::encode(trim(*it)) + "\n";
			}
			strTable += "   </td>\n";
			strTable += "  </tr>\n";
		}
		strTable += "  </tbody>\n";
		strTable += "</table>\n";

	}
	return strTable;
}

void TStringList::saveToFile(const std::string fileName) const {
	util::deleteFile(fileName);
	if (!empty()) {
		util::saveToFile(*this, fileName);
	}
}

void TStringList::loadFromFile(const std::string fileName, const app::ECodepage codepage) {
	util::loadFromFile(*this, fileName, codepage);
}

void TStringList::debugOutput() const {
	util::debugOutput(*this);
}

size_t TStringList::length() const {
	size_t r = 0;
	const_iterator it = begin();
	while (it != end()) {
		r += (*it).size();
		++it;
	}
	return r;
}

void TStringList::select(const size_t index, const size_t count, util::TStringList& target) {
	target.clear();
	const size_t last = index + count;
	for (size_t i=index; i<last; ++i) {
		if (!validIndex(i))
			break;
		target.add(at(i));
	}
}

void TStringList::split(const std::string& csv, const char delimiter) {
	util::split(csv, *this, delimiter);
}

void TStringList::split(const std::string& csv, const char* delimiters, TIsWhiteSpaceA isSpace) {
	util::tokenize(csv, *this, delimiters, ET_IMPORT_QUOTED, isSpace);
}

void TStringList::split(const std::string& csv, const std::string& delimiters, TIsWhiteSpaceA isSpace) {
	split(csv, delimiters.c_str(), isSpace);
}

bool TStringList::validIndex(size_t index) const {
	return (index >= 0 && index < size());
}

size_t TStringList::find(const std::string& value, const util::ECompareType partial) const {
	if (!empty() && !value.empty()) {
		for(size_t i=0; i<size(); ++i) {
			const std::string& s = at(i);
			switch (partial) {
				case EC_COMPARE_FULL:
					if ((s.size() == value.size()) && (0 == util::strncasecmp(s, value, value.size()))) {
						return i;
					}
					break;
				case EC_COMPARE_HEADING:
					if (s.size() > value.size()) {
						if (0 == util::strncasecmp(s, value, value.size())) {
							return i;
						}
					} else {
						if (0 == util::strncasecmp(value, s, s.size())) {
							return i;
						}
					}
					break;
				case EC_COMPARE_PARTIAL:
					if (s.size() > value.size()) {
						if (util::strcasestr(s, value)) {
							return i;
						}
					} else {
						if (util::strcasestr(value, s)) {
							return i;
						}
					}
					break;
				case EC_COMPARE_VALUE_IN_LIST:
					if (s.size() >= value.size()) {
						if (util::strcasestr(s, value)) {
							return i;
						}
					}
					break;
				case EC_COMPARE_LIST_IN_VALUE:
					if (value.size() >= s.size()) {
						if (util::strcasestr(value, s)) {
							return i;
						}
					}
					break;
			}
		}
	}
	return std::string::npos;
}


void TStringList::shrink(size_t rows) {
	bool shrinked = false;
	while (size() > rows) {
		pop_back();
		shrinked = true;
	}
	if (shrinked)
		invalidate();
}

struct CStringListCompressor {
    bool operator() (std::string& s) const {
		return trim(s).empty();
    }
};

size_t TStringList::compress() {
	size_t retVal = 0;
	if (!empty()) {
		// Delete all empty items from list
		retVal = size();
		erase(std::remove_if(begin(), end(), CStringListCompressor()), end());
		retVal -= size();
	}
	return retVal;
}

void TStringList::distinct(const util::ECompareType partial) {
	if (!empty()) {
		TStringList target;
		bool found;
		for (size_t i=0; i<size(); ++i) {
			const std::string& s1 = at(i);
			found = false;
			for (size_t j=0; j<size(); ++j) {
				if (i != j) {
					const std::string& s2 = at(j);
					switch (partial) {
						case EC_COMPARE_FULL:
							if ((s1.size() == s2.size()) && (0 == util::strncasecmp(s1, s2, s2.size()))) {
								found = true;
							}
							break;
						case EC_COMPARE_PARTIAL:
						case EC_COMPARE_HEADING:
						case EC_COMPARE_VALUE_IN_LIST:
						case EC_COMPARE_LIST_IN_VALUE:
							if (s2.size() >= s1.size()) {
								if (util::strcasestr(s2, s1)) {
									found = true;
								}
							}
							//std::cout << "TStringList::distinct() s1 = <" << s1 << "> s2 = <" << s2 << "> found = " << found << std::endl;
							break;
					}
				}
				if (found) {
					break;
				}
			}
			if (!found) {
				target.add(s1);
			}
		}
		if (!target.empty()) {
			assign(target);
		} else {
			clear();
		}
	}
}


bool stringSorterAsc(const std::string& s1, const std::string& s2) {
	return util::strnatcasesort(s1, s2);
}

bool stringSorterDesc(const std::string& s1, const std::string& s2) {
	return util::strnatcasesort(s2, s1);
}

void TStringList::sort(TStringSorter sorter) {
	std::sort(begin(), end(), sorter);
}

void TStringList::sort(util::ESortOrder order, TStringSorter asc, TStringSorter desc) {
	TStringSorter sorter;
	switch(order) {
		case util::SO_DESC:
			sorter = desc;
			break;
		case util::SO_ASC:
		default:
			sorter = asc;
			break;
	}
	sort(sorter);
}

void TStringList::sort(util::ESortOrder order) {
	sort(order, stringSorterAsc, stringSorterDesc);
}


void TStringList::assign(const util::TStringList& list) {
	clear();
	if (!list.empty()) {
		TStringList::const_iterator it = list.begin();
		for (; it != list.end(); ++it)
			add(*it);
	}
}

void TStringList::assign(const std::string& csv) {
	clear();
	split(csv, ';');
}

void TStringList::assign(const std::string& csv, const char delimiter) {
	clear();
	split(csv, delimiter);
}

void TStringList::assign(const std::string& csv, const char* delimiters) {
	clear();
	split(csv, delimiters);
}

void TStringList::assign(const std::string& csv, const std::string& delimiters) {
	clear();
	split(csv, delimiters);
}

TStringList& TStringList::operator = (const TStringList& list) {
	assign(list);
	return *this;
}

TStringList& TStringList::operator = (const std::string& csv) {
	assign(csv);
	return *this;
}


} /* namespace util */


std::string util::strToStr(const std::string& str, const char *defValue) {
	if (!str.empty())
		return str;
	return std::string(defValue);
}

std::wstring util::strToStr(const std::wstring& str, const wchar_t *defValue) {
	if (!str.empty())
		return str;
	return std::wstring(defValue);
}

const std::string& util::strToStr(const std::string& str, const std::string& defValue) {
	if (!str.empty())
		return str;
	return defValue;
}

const std::wstring& util::strToStr(const std::wstring& str, const std::wstring& defValue) {
	if (!str.empty())
		return str;
	return defValue;
}



std::string util::charToStr(const char *str, const char *defValue) {
	std::string retVal;
	if (util::assigned(str))
		retVal = std::string(str);
	if (!retVal.empty())
		return retVal;
	return std::string(defValue);
}

std::wstring util::charToStr(const wchar_t *str, const wchar_t *defValue) {
	std::wstring retVal;
	if (util::assigned(str))
		retVal = std::wstring(str);
	if (!retVal.empty())
		return retVal;
	return std::wstring(defValue);
}

std::string util::charToStr(const char *str, const std::string& defValue) {
	std::string retVal;
	if (util::assigned(str))
		retVal = std::string(str);
	if (!retVal.empty())
		return retVal;
	return defValue;
}

std::wstring util::charToStr(const wchar_t *str, const std::wstring& defValue) {
	std::wstring retVal;
	if (util::assigned(str))
		retVal = std::wstring(str);
	if (!retVal.empty())
		return retVal;
	return defValue;
}


void util::fill(std::string& str, const char fillchar, const size_t size) {
	str.reserve(size + 1);
	do {
		str.push_back(fillchar);
	} while (str.size() < size);

}


void util::fill(std::wstring& str, const wchar_t fillchar, const size_t size) {
	str.reserve(size + 1);
	do {
		str.push_back(fillchar);
	} while (str.size() < size);

}


std::string util::ellipsis(const std::string& str, const size_t size) {
	std::string s = str;
	if (s.size() > size + 3) {
		size_t ofs = size / 2;
		if (ofs < 1)
			ofs = 1;

		size_t left = ofs;
		size_t right = ofs;
		size_t range = ofs * 3 / 2;
		size_t lrange = left + range;
		size_t rrange = s.size() - right - range;

		// Find first left non alpha numeric char
		for (size_t i = util::pred(left); i<s.size() && i<lrange; i++) {
			if (util::isNonAlphaNumericA(s[i])) {
				left = util::succ(i);
				break;
			}
		}

		// Find first right non alpha numeric char
		for (size_t i = util::pred(s.size() - util::pred(right)); i>left && i>rrange; i--) {
			if (util::isNonAlphaNumericA(s[i])) {
				right = s.size() - i;
				break;
			}
		}

		if (s.size() > left + right + 3)
			s = s.substr(0, left) + "..." + s.substr(s.size() - right);
	}
	return s;
}

std::wstring util::ellipsis(const std::wstring& str, const size_t size) {
	std::wstring ws = str;
	if (ws.size() > size + 3) {
		size_t ofs = size / 2;
		if (ofs < 1)
			ofs = 1;

		size_t left = ofs;
		size_t right = ofs;
		size_t range = ofs * 3 / 2;
		size_t lrange = left + range;
		size_t rrange = ws.size() - right - range;

		// Find first left non alpha numeric char
		for (size_t i = util::pred(left); i<ws.size() && i<lrange; i++) {
			if (util::isNonAlphaNumericW(ws[i])) {
				left = util::succ(i);
				break;
			}
		}

		// Find first right non alpha numeric char
		for (size_t i = util::pred(ws.size() - util::pred(right)); i>left && i>rrange; i--) {
			if (util::isNonAlphaNumericW(ws[i])) {
				right = ws.size() - i;
				break;
			}
		}

		if (ws.size() > left + right + 3)
			ws = ws.substr(0, left) + L"..." + ws.substr(ws.size() - right);
	}
	return ws;
}


std::string util::shorten(const std::string& str, const size_t size) {
	std::string s;

	if (size <= 0)
		return std::string();

	if (size >= str.size())
		return str;

	// Find first ASCII (non UTF-8) char left from size
	unsigned char c;
	size_t pos = 0;
	ssize_t i;
	for (i=(ssize_t)util::pred(size); i>0; i--) {
		c = (unsigned char)str[i];
		if (c < 0x80u) {
			pos = i;
			break;
		}
	}

	// Look for next nearest space
	if (pos > 5) {
		for (i=pos; i>(ssize_t)(pos-3); i--) {
			c = (unsigned char)str[i];
			if (c == 0x20u) {
				pos = util::pred(i);
				break;
			}
		}
	}

	if (pos > 0) {
		s = str.substr(0, pos + 1);
	} else {
		s = "...";
	}

	return s;
}

std::wstring util::shorten(const std::wstring& str, const size_t size) {
	std::wstring ws;

	if (size <= 0)
		return std::wstring();

	if (size >= str.size())
		return str;

	// Find first ASCII (non UTF-8) char left from size
	wchar_t c;
	size_t pos = 0;
	ssize_t i;
	for (i=(ssize_t)util::pred(size); i>0; i--) {
		// TODO Check if this test is OK for wide strings...
		c = str[i];
		if (c < (wchar_t)0x80) {
			pos = i;
			break;
		}
	}

	// Look for next nearest space
	if (pos > 5) {
		for (i=pos; i>(ssize_t)(pos-3); i--) {
			c = str[i];
			if (c == (wchar_t)0x20) {
				pos = util::pred(i);
				break;
			}
		}
	}

	if (pos > 0) {
		ws = str.substr(0, pos + 1);
	} else {
		ws = L"...";
	}

	return ws;

}



std::string& util::trimLeft(std::string& str, TIsWhiteSpaceA isSpace) {
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::function<int(char)>(isSpace))));
	return str;
}

std::string& util::trimRight(std::string& str, TIsWhiteSpaceA isSpace) {
	str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::function<int(char)>(isSpace))).base(), str.end());
	return str;
}

std::string& util::trim(std::string& str, TIsWhiteSpaceA isSpace) {
	trimLeft(str, isSpace);
	trimRight(str, isSpace);
	return str;
}

std::string util::trim(const std::string& str, TIsWhiteSpaceA isSpace) {
	std::string s = str;
	trimLeft(s, isSpace);
	trimRight(s, isSpace);
	return s;
}


std::wstring& util::trimLeft(std::wstring& str, TIsWhiteSpaceW isSpace) {
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(isSpace)));
	return str;
}

std::wstring& util::trimRight(std::wstring& str, TIsWhiteSpaceW isSpace) {
	str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(isSpace)).base(), str.end());
	return str;
}

std::wstring& util::trim(std::wstring& str, TIsWhiteSpaceW isSpace) {
	trimLeft(str, isSpace);
	trimRight(str, isSpace);
	return str;
}

std::wstring util::trim(const std::wstring& str, TIsWhiteSpaceW isSpace) {
	std::wstring s = str;
	trimLeft(s, isSpace);
	trimRight(s, isSpace);
	return s;
}


int util::replacer(std::string& str, char oldChar, char newChar, const size_t startPos) {
	size_t pos = 0;
	int replacedChars = 0;
	for (std::string::iterator it = str.begin(); it != str.end(); ++it) {
		if (*it == oldChar && pos >= startPos) {
			*it = newChar;
			replacedChars++;
		}
		++pos;
	}

	return replacedChars;
}

int util::replacer(std::wstring& str, wchar_t oldChar, wchar_t newChar, const size_t startPos) {
	size_t pos = 0;
	int replacedChars = 0;
	for (std::wstring::iterator it = str.begin(); it != str.end(); ++it) {
		if (*it == oldChar && pos >= startPos) {
			*it = newChar;
			replacedChars++;
		}
		++pos;
	}

	return replacedChars;
}

int util::replacer(std::string& str, const std::string& oldStr, const std::string& newStr, const size_t startPos) {
	if (oldStr.empty())
		return 0;

	int replacedChars = 0;
	size_t index = startPos;

	while (index < str.size() && (index = str.find(oldStr, index)) != std::string::npos) {
		str.replace(index, oldStr.size(), newStr);
		index += newStr.size();
		replacedChars += oldStr.size();
	}

	return replacedChars;
}

int util::replacer(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr, const size_t startPos) {
	if (oldStr.empty())
		return 0;

	int replacedChars = 0;
	size_t index = startPos;

	while (index < str.size() && (index = str.find(oldStr, index)) != std::string::npos) {
		str.replace(index, oldStr.size(), newStr);
		index += newStr.size();
		replacedChars += oldStr.size();
	}

	return replacedChars;
}


std::string util::replace(const std::string& str, char oldChar, char newChar, const size_t startPos) {
	std::string s = str;
	replacer(s, oldChar, newChar, startPos);
	return s;
}

std::wstring util::replace(const std::wstring& str, wchar_t oldChar, wchar_t newChar, const size_t startPos) {
	std::wstring ws = str;
	replacer(ws, oldChar, newChar, startPos);
	return ws;
}

std::string util::replace(const std::string& str, const std::string& oldStr, const std::string& newStr, const size_t startPos) {
	std::string s = str;
	replacer(s, oldStr, newStr, startPos);
	return s;
}

std::wstring util::replace(const std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr, const size_t startPos) {
	std::wstring ws = str;
	replacer(ws, oldStr, newStr, startPos);
	return ws;
}



std::string util::cprintf(const char* fmt, ...) {
    if (util::assigned(fmt)) {
		int n;
		util::TStringBuffer buf;
		size_t len = strnlen(fmt, 1024);
		buf.reserve(len * 7, false);
		buf.resize(len * 3, false);
		va_list ap;

		while (true) {
			va_start(ap, fmt);
			n = vsnprintf(buf.data(), buf.size(), fmt, ap);
			va_end(ap);

			if ((n > -1) && ((size_t)n < buf.size())) {
				return std::string(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
		return fmt;
    }
    return std::string();
}

std::string util::cprintf(const std::string &fmt, ...) {
	if (!fmt.empty()) {
		int n;
		util::TStringBuffer buf;
		buf.reserve(fmt.size() * 7, false);
		buf.resize(fmt.size() * 3, false);
		va_list ap;

		while (true) {
			va_start(ap, fmt);
			n = vsnprintf(buf.data(), buf.size(), fmt.c_str(), ap);
			va_end(ap);

			if ((n > -1) && ((size_t)n < buf.size())) {
				return std::string(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
	}
	return fmt;
}

std::wstring util::cprintf(const wchar_t* fmt, ...) {
    if (util::assigned(fmt)) {
		int n;
		util::TWideBuffer buf;
		size_t len = wcsnlen(fmt, 1024);
		buf.reserve(len * 7, false);
		buf.resize(len * 3, false);
		va_list ap;

		while (true) {
			va_start(ap, fmt);
			n = vswprintf(buf.data(), buf.size(), fmt, ap);
			va_end(ap);

			if ((n > -1) && (size_t(n) < buf.size())) {
				return std::wstring(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
		return fmt;
    }
    return std::wstring();
}

std::wstring util::cprintf(const std::wstring &fmt, ...) {
	if (!fmt.empty()) {
		int n;
		util::TWideBuffer buf;
		buf.reserve(fmt.size() * 7, false);
		buf.resize(fmt.size() * 3, false);
		va_list ap;

		while (true) {
			va_start(ap, fmt);
			n = vswprintf(buf.data(), buf.size(), fmt.c_str(), ap);
			va_end(ap);

			if ((n > -1) && (size_t(n) < buf.size())) {
				return std::wstring(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
	}
	return fmt;
}



std::string util::cprintf(const app::TLocale& locale, const char* fmt, ...) {
    if (util::assigned(fmt)) {
		int n;
		util::TStringBuffer buf;
		size_t len = strnlen(fmt, 1024);
		buf.reserve(len * 7, false);
		buf.resize(len * 3, false);
		va_list ap;

		app::TLocationGuard<app::TLocale> location(locale);
		location.change();

		while (true) {

			va_start(ap, fmt);
			n = vsnprintf(buf.data(), buf.size(), fmt, ap);
			va_end(ap);

			if ((n > -1) && ((size_t)n < buf.size())) {
				return std::string(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
		return fmt;
    }
    return std::string();
}

std::string util::cprintf(const app::TLocale& locale, const std::string &fmt, ...) {
	if (!fmt.empty()) {
		int n;
		util::TStringBuffer buf;
		buf.reserve(fmt.size() * 7, false);
		buf.resize(fmt.size() * 3, false);
		va_list ap;

		app::TLocationGuard<app::TLocale> location(locale);
		location.change();

		while (true) {

			va_start(ap, fmt);
			n = vsnprintf(buf.data(), buf.size(), fmt.c_str(), ap);
			va_end(ap);

			if ((n > -1) && ((size_t)n < buf.size())) {
				return std::string(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
	}
	return fmt;
}

std::wstring util::cprintf(const app::TLocale& locale, const wchar_t* fmt, ...) {
    if (util::assigned(fmt)) {
		int n;
		util::TWideBuffer buf;
		size_t len = wcsnlen(fmt, 1024);
		buf.reserve(len * 7, false);
		buf.resize(len * 3, false);
		va_list ap;

		app::TLocationGuard<app::TLocale> location(locale);
		location.change();

		while (true) {

			va_start(ap, fmt);
			n = vswprintf(buf.data(), buf.size(), fmt, ap);
			va_end(ap);

			if ((n > -1) && (size_t(n) < buf.size())) {
				return std::wstring(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
		return fmt;
    }
    return std::wstring();
}

std::wstring util::cprintf(const app::TLocale& locale, const std::wstring &fmt, ...) {
	if (!fmt.empty()) {
		int n;
		util::TWideBuffer buf;
		buf.reserve(fmt.size() * 7, false);
		buf.resize(fmt.size() * 3, false);
		va_list ap;

		app::TLocationGuard<app::TLocale> location(locale);
		location.change();

		while (true) {

			va_start(ap, fmt);
			n = vswprintf(buf.data(), buf.size(), fmt.c_str(), ap);
			va_end(ap);

			if ((n > -1) && (size_t(n) < buf.size())) {
				return std::wstring(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
	}
    return fmt;
}


std::string util::quote(const std::string &str) {
	// Avoid string copy by using this* reference
	std::string s;
	s.reserve(str.size() + 2);
	s.append("\"").append(str).append("\"");
	return s;
}

std::wstring util::quote(const std::wstring &str) {
	// Avoid string copy by using this* reference
	std::wstring ws;
	ws.reserve(str.size() + 2);
	ws.append(L"\"").append(str).append(L"\"");
	return ws;
}


std::string util::unquote(const std::string &str) {
	std::string s = str;
	if (s.size() > 2) {
		size_t offs = util::pred(s.size());
		char end = s[offs];
		if (end == '\"' || end == '\'') {
			s.erase(offs);
			offs = 0;
			char begin = s[offs];
			if (end == '\'') {
				if (begin == '\'' || begin == '\"')
					s.erase(offs, 1);
			} else {
				if (begin == '\"')
					s.erase(offs, 1);
			}
		}
	}
	return s;
}

std::wstring util::unquote(const std::wstring &str) {
	std::wstring ws = str;
	if (ws.size() > 2) {
		size_t offs = util::pred(ws.size());
		char end = ws[offs];
		if (end == L'\"' || end == L'\'') {
			ws.erase(offs);
			offs = 0;
			char begin = ws[offs];
			if (end == L'\'') {
				if (begin == L'\'' || begin == L'\"')
					ws.erase(offs, 1);
			} else {
				if (begin == L'\"')
					ws.erase(offs, 1);
			}
		}
	}
	return ws;
}

inline bool isControlCharacterA(unsigned char c) {
	return (c < USPC);
}

std::string util::escape(const std::string& value) {
	std::string s;
	s.reserve(10 * value.size() / 8);
	for (size_t i=0; i<value.size(); ++i) {
		char c = value[i];
		if (isControlCharacterA(c)) {
			switch (c) {
				case '\r':
					s += "\\r";
					break;
				case '\n':
					s += "\\n";
					break;
				case '\t':
					s += "\\t";
					break;
				default:
					break;
			}
		} else {
			switch (c) {
				case '\"':
				case '\\':
					s.push_back('\\');
					break;
				default:
					break;
			}
			s.push_back(c);
		}
	}
	return s;
}

std::string util::unescape(const std::string& value) {
	std::string s;
	s.reserve(value.size());
	for (size_t i=0; i<value.size(); ++i) {
		char c = value[i];
		if ('\\' == c) {
			size_t next = util::succ(i);
			if (next < value.size()) {
				char n = value[next];
				switch (n) {
					case 'r':
						s.push_back('\r');
						break;
					case 'n':
						s.push_back('\n');
						break;
					case 't':
						s.push_back('\t');
						break;
					default:
						s.push_back(n);
						break;
				}
			}
			++i;
		} else {
			s.push_back(c);
		}
	}
	return s;
}


inline bool isControlCharacterW(wchar_t c) {
	return (c < util::WSPC);
}

std::wstring util::escape(const std::wstring& value) {
	std::wstring ws;
	ws.reserve(10 * value.size() / 8);
	for (size_t i=0; i<value.size(); ++i) {
		wchar_t c = value[i];
		if (isControlCharacterW(c)) {
			switch (c) {
				case L'\r':
					ws += L"\\r";
					break;
				case L'\n':
					ws += L"\\n";
					break;
				case L'\t':
					ws += L"\\t";
					break;
				default:
					break;
			}
		} else {
			switch (c) {
				case L'\"':
				case L'\\':
					ws.push_back(L'\\');
					break;
				default:
					break;
			}
			ws.push_back(c);
		}
	}
	return ws;
}

std::wstring util::unescape(const std::wstring& value) {
	std::wstring ws;
	ws.reserve(value.size());
	for (size_t i=0; i<value.size(); ++i) {
		wchar_t c = value[i];
		if (L'\\' == c) {
			size_t next = util::succ(i);
			if (next < value.size()) {
				wchar_t n = value[next];
				switch (n) {
					case L'r':
						ws.push_back(L'\r');
						break;
					case L'n':
						ws.push_back(L'\n');
						break;
					case L't':
						ws.push_back(L'\t');
						break;
					default:
						ws.push_back(n);
						break;
				}
			}
			++i;
		} else {
			ws.push_back(c);
		}
	}
	return ws;
}


std::string util::capitalize(const std::string &str, const std::locale& locale) {
	std::string s(str);
	if (!s.empty()) {
		char c;
		bool capitalize = true;
		for (size_t i=0; i<s.size(); ++i) {
			c = s[i];
			if (capitalize && isAlphaA(c)) {
				s[i] = std::toupper(c, locale);
				capitalize = false;
			} else {
				if (!capitalize && isWhiteSpaceA(c)) {
					capitalize = true;
				}
			}
		}
	}
	return s;
}


std::wstring util::capitalize(const std::wstring &str, const std::locale& locale) {
	std::wstring ws(str);
	if (!ws.empty()) {
		wchar_t c;
		bool capitalize = true;
		for (size_t i=0; i<ws.size(); ++i) {
			c = ws[i];
			if (capitalize && isAlphaW(c)) {
				ws[i] = std::toupper(c, locale);
				capitalize = false;
			} else {
				if (!capitalize && isWhiteSpaceW(c)) {
					capitalize = true;
				}
			}
		}
	}
	return ws;
}


template<typename charT>
struct m_tolower {
private:
	const std::locale& loc;

public:
	charT operator() (charT c) {
		return std::tolower(c, loc);
	}
	m_tolower(const std::locale& locale) : loc(locale) {}
};

template<typename charT>
struct m_toupper {
private:
	const std::locale& loc;

public:
	charT operator() (charT c) {
		return std::toupper(c, loc);
	}
	m_toupper(const std::locale& locale) : loc(locale) {}
};


std::string util::tolower(const std::string &str, const std::locale& locale) {
	std::string s(str);
	std::transform(s.begin(), s.end(), s.begin(), m_tolower<char>(locale));
	return s;
}

std::wstring util::tolower(const std::wstring &str, const std::locale& locale) {
	std::wstring ws(str);
	std::transform(ws.begin(), ws.end(), ws.begin(), m_tolower<wchar_t>(locale));
	return ws;
}

std::string util::toupper(const std::string &str, const std::locale& locale) {
	std::string s(str);
	std::transform(s.begin(), s.end(), s.begin(), m_toupper<char>(locale));
	return s;
}

std::wstring util::toupper(const std::wstring &str, const std::locale& locale) {
	std::wstring ws(str);
	std::transform(ws.begin(), ws.end(), ws.begin(), m_toupper<wchar_t>(locale));
	return ws;
}


// Extract string between quotes via std::string::find() methods
// First and last char after trimmming whitespaces must be quote char!
std::string util::extractQuotedStr(const std::string& str, const char quote, TIsWhiteSpaceA isSpace) {
	std::string s1(trim(str, isSpace));
	if (!s1.empty()) {
		if (s1[0] == quote) {
			std::string::size_type p = s1.find_first_of(quote, 1);
			if (p != std::string::npos) {
				// Trailing quote missing
				// --> return string[1..p-1]
				if (p > 2)
					return s1.substr(1, pred(p));
			} else {
				// Trailing quote missing
				// --> return string[1..size()-1]
				if (s1.size() > 1)
					return s1.substr(1);
			}
		    return std::string();
		}
	}
	return s1;
}

// Extract next quoted string up from pos via std::string::find() methods
// Caution: function may return parameter pos >= s.size()
std::string util::extractQuotedStr(const std::string& str, size_t& pos, const char quote) {
	// Find leading quote char
	std::string::size_type p = str.find_first_of(quote, pos);
	if (p != std::string::npos) {
	    // Search for trailing quote from next char
		p++;
		if (p < str.size()) {
			std::string::size_type q = str.find_first_of(quote, p);
			// Trailing quote found?
			if (q != std::string::npos) {
				// Set offset to next next char if possible
				// pos = q + 1;
				pos = succ(q);
				// Check for empty string like ""
				if (q > p)
					return str.substr(p, q - p);
			} else {
				// No valid trailing quote found!!!
				// --> Simply return offset to next char
				pos++;
			}
		}
	}
    return std::string();
}

// Extract next quoted string up from pos via std::string::iterator
// Caution: function may change parameter pos >= s.end()
std::string util::extractQuotedStr(const std::string& str, std::string::const_iterator& pos, const char quote) {
	// Find leading quote char
	std::string::size_type idx;
	std::string::const_iterator p = pos, q, eol = str.end();

	if (p == eol) {
		idx = str.find_first_of(quote);
		if (idx != std::string::npos)
			p = str.begin() + idx;
	}

	if (p != eol) {
	    // Search for trailing quote from next char
		p++;
		if (p != eol) {

			idx = p - str.begin(); // --> / sizeof(char)
			idx = str.find_first_of(quote, idx);

			// Trailing quote found?
			if (idx != std::string::npos) {
				q = str.begin() + idx;

				// Set offset to next next char if possible
				pos = succ(q);

				// Check for empty string like ""
				if (q > p) {
					// Constructor of std::string from p to q
					return std::string(p, q);
				}

			} else {
				// No valid trailing quote found!!!
				// --> Simply return offset to next char
				pos++;
			}
		}
	}
    return std::string();
}


// Split string by delimiters char via strtok()
// Return list of (unquoted by parameter) strings between delimiter
// Disadvantage (or advantage...) of using strtok() here:
//   ";;;" results in one empty entry in list!
//   --> empty strings between delimiters are ignored by strtok()
void util::tokenize(const std::string& str, app::TStringVector& list, const char* delimiters, const EImportType type, TIsWhiteSpaceA isSpace) {
	list.clear();
	if (!str.empty()) {
		TBuffer buffer(str.size() + 1);
		buffer.assign(str.c_str(), str.size());
		bool quoted = ET_IMPORT_QUOTED == type;

		char* p = strtok(buffer.data(), delimiters);
		while (p) {
			if (quoted) {
				std::string s1(extractQuotedStr(std::string(p), '"', isSpace));
				if (!s1.empty())
					list.push_back(s1);
			} else {
				list.push_back(p);
			}
			p = strtok(NULL, delimiters);
		}
	}
}

#ifdef PARSE_CSV_BY_INDEX

// Split string by delimiter character via iterating through array of char by index
void util::split(const std::string& str, app::TStringVector& list, const char delimiter) {
	list.clear();
	if (!str.empty()) {
		std::string::size_type p = 0, q = 0;
		// Suppress leading whitespaces and spaces
		while (((unsigned char)str[p] <= USPC) && (str[p] != delimiter) && (p < str.size())) p++;
		while (p < str.size()) {
			q = p;
			if (str[p] == '"') {
				// Everything between quotes is valid!
				list.push_back(extractQuotedStr(str, p, '"'));
			} else {
				// Search for next delimiter, space is valid in string!
				// Or stop on first trailing whitespace!
				while (((unsigned char)str[p] >= USPC) && (str[p] != delimiter) && (p < str.size())) p++;

				// Valid string found:
				// --> Remove trailing spaces from end
				if ((p > q) && ((unsigned char)str[pred(p)] <= USPC) && (p < str.size())) {
					do {
						p--;
					} while ((unsigned char)str[p] <= USPC && p > q);
					// Set offset to space char
					p++;
				}

				// Add trimmed string to list
				list.push_back(str.substr(q, p - q));
			}
			// Suppress trailing whitespaces
			while (((unsigned char)str[p] <= USPC) && (str[p] != delimiter) && (p < str.size())) p++;
			if ((str[p] == delimiter) && (p < str.size())) {
				do {
					// Remember value of p in q for delimiter check afterwards
					q = p;
					p++;
				} while (((unsigned char)str[p] <= USPC) && (str[p] != delimiter) && (p < str.size()));
			}
			// Add empty string if last char in string is delimiter
			if ((str[q] == delimiter) && (p >= str.size()))
				list.push_back(std::string());
		}
	}
}

#else

// Split string by delimiter character using std::string_iterator
void util::split(const std::string& str, app::TStringVector& list, const char delimiter) {
	list.clear();
	if (!str.empty()) {
		// Use iterator for pointer access to string:
		std::string::const_iterator p = str.begin(), q = str.begin(), eol = str.end();

		// Suppress leading whitespaces and spaces
		while (((unsigned char)*p <= USPC) && (*p != delimiter) && (p != eol)) p++;
		while (p != eol) {
			q = p;
			if (*p == '"') {
				// Everything between quotes is valid!
				list.push_back(extractQuotedStr(str, p, '"'));
			} else {
				// Search for next delimiter, space is valid in string!
				// Or stop on first trailing whitespace!
				while (((unsigned char)*p >= USPC) && (*p != delimiter) && (p != eol)) p++;

				// Valid string found
				// Remove trailing spaces from end
				if ((p > q) && ((unsigned char)*pred(p) <= USPC) && (p != eol)) {
					while ((unsigned char)*p <= USPC && p > q) p--;
					p++;
				}

				// Add entry to list from q to p-1 (!)
				list.push_back(std::string(q, p));

			}
			// Suppress trailing whitespaces
			while (((unsigned char)*p <= USPC) && (*p != delimiter) && (p != eol)) p++;
			if ((*p == delimiter) && (p != eol)) {
				do {
					// Remember value of p in q for delimiter check afterwards
					q = p;
					p++;
				} while (((unsigned char)*p <= USPC) && (*p != delimiter) && (p != eol));
			}
			// Add empty string if last char in string is delimiter
			if ((*q == delimiter) && (p == eol))
				list.push_back(std::string());
		}
	}
}

#endif

// Split string on every whitespace (c <= SPC)
// Return list of unquoted (!) strings between delimiter
void util::split(const std::string& str, app::TStringVector& list) {
	list.clear();
	if (!str.empty()) {
		std::string::size_type p = 0, q = 0;
		while (p < str.size()) {
			// Suppress leading whitespaces and spaces
			while (((unsigned char)str[p] <= USPC) && (p < str.size())) p++;
			if (p < str.size()) {
				q = p;
				if (str[p] == '"') {
					// Everything between quotes is valid!
					list.push_back(extractQuotedStr(str, p, '"'));
				} else {
					// Stop on first trailing whitespace!
					while (((unsigned char)str[p] > USPC) && (p < str.size())) p++;
					// Valid string found
					if ((p - q) > 0) {
						list.push_back(str.substr(q, p - q));
					}
				}
			}
		}
	}
}


void util::join(const app::TStringVector& list, std::string& str, const char delimiter) {
	str.clear();
	if (!list.empty()) {
		app::TStringVector::const_iterator it = list.begin();
		app::TStringVector::const_iterator last = pred(list.end());

		// Calculate estimated destination size
		size_t size = 0;
		for (; it != list.end(); ++it)
			size += it->size();
		str.reserve(size + 2 * list.size());

		// Copy content
		it = list.begin();
		if (list.size() > 1) {
			for(; it != last; ++it)
				str += (*it) + delimiter;
		}
		str += (*it);
	}
}


void util::loadFromFile(app::TStringVector& list, const std::string& fileName, const app::ECodepage codepage) {
	if (fileExists(fileName)) {
		list.clear();
		std::string line;
		std::ifstream is;
		TStreamGuard<std::ifstream> strm(is);
		strm.open(fileName, std::ios_base::in);

		// Read all lines until EOF
		while (is.good()) {
			std::getline(is, line);
			if (!line.empty()) {
				list.push_back(app::ECodepage::UTF_8 == codepage ? line : util::TStringConvert::SingleByteToMultiByteString(line, codepage));
			}
		}
	}
}


void util::saveToFile(const app::TStringVector& list, const std::string& fileName) {
	std::ofstream os;
	TStreamGuard<std::ofstream> strm(os);
	strm.open(fileName, std::ios_base::out);
	std::copy(list.begin(), list.end(), std::ostream_iterator<std::string>(os, "\n"));
}


void util::debugOutput(const app::TStringVector& list) {
	std::copy(list.begin(), list.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
}
