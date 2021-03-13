/*
 * inifile.cpp
 *
 *  Created on: 21.01.2015
 *      Author: Dirk Brinkmeier
 */

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <limits.h>
#include "inifile.h"
#include "stringutils.h"
#include "templates.h"
#include "locale.h"
#include "convert.h"
#include "compare.h"
#include "sysconsts.h"
#include "fileutils.h"
#include "exception.h"
#include "../config.h"

namespace app {

TIniValue::TIniValue(const std::string& line, const size_t index, PIniValue const anchor, const bool debug) {
	this->line = line;
	this->index = index;
	this->type = INI_NONE;
	this->debug = debug;
	forceOrder(std::string::npos);
	setSection(false);
	setEmpty(true);
	if (util::assigned(anchor)) {
		section = anchor->section;
	} else {
		section.clear();
	}
	parseLine();
}

TIniValue::~TIniValue() {
}

bool TIniValue::extractValue() {
	bool retVal = false, found = false;

	if (line.size() > 0) {
		std::string sKey;

		// Use iterator for pointer access to string:
		std::string::const_iterator p, q, eol;
		eol = line.end();
		p = line.begin();
		q = p;

		// Suppress leading whitespaces + space ' '
		while (((uint8_t)*p <= 0x20) and (p != eol)) p++;
		while (p != eol) {
			// Store start position of key string
			q = p;

			// Check for comment '#'
			if (*p == '#') {
				// Leave line as it is, mark as found (= non empty!)
				type = INI_COMMENT;
				retVal = true;
				break;
			}

			// Check for section identifier '['
			if (*p == '[') {
				// Section found and you're done!
				retVal = extractSection(p);
				if (retVal)
					type = INI_SECTION;
				break;
			}

			// Check for broken section like ']<some text>'
			if (*p == ']') {
				// All is invalid up from here to next section key!
				section.clear();
				key.clear();
				break;
			}

			// Check if valid section was set before
			// --> otherwise looking further through this line is pretty useless
			if (!section.getHash())
				break;

			// Look for trailing whitespace or equal '=' char
			while (((uint8_t)*p > 0x20) and (*p != '=') and (*p != ']') and (p != eol)) p++;

			// Check for broken section like '<some text>]'
			if (*p == ']') {
				// All is invalid up from here to next section key!
				section.clear();
				key.clear();
				break;
			}

			// Key string with length > 0 found?
			if (p > q) {

				// Key string is valid!
				sKey = std::string(q, p);

				// Suppress trailing whitespace
				while (((uint8_t)*p <= 0x20) and (*p != '=') and (p != eol)) p++;

				// Value string found?
				if (*p == '=') {

					// Ignore key if no '=' found!
					found = true;

					// Start with next char after '='
					p++;

					// Suppress trailing whitespace
					while (((uint8_t)*p <= 0x20) and (p != eol)) p++;
					if (p != eol) {
						q = p;

						// Look for trailing whitespace
						// Allow SPC in strings, trim afterwards!
						// while ((*p > 0x20) and (p != eol)) p++;
						while (((uint8_t)*p >= 0x20) and (p != eol)) p++;

						// Value string with length > 0 found?
						if (p > q) {
							value = util::trim(std::string(q, p));
							if (debug) std::cout << "TIniValue::extractValue() Value \"" << value << "\" --> \"" << std::string(q, p) << "\"" << std::endl;
						}
					}
				}

				// A valid key=value string was found...
				if (found) {
					key.setString(sKey);
					retVal = true;
				} else {
					// Key was invalid, '=' missing!
					key.clear();
				}

			} else
				break;
		}
	}
	return retVal;
}

bool TIniValue::extractSection(const size_t& pos) {
	bool retVal = false;
	std::string s;
	size_t p, q;
	if (pos == std::string::npos)
		p = line.find_first_of('[');
	else p = pos;
	if (p != std::string::npos) {
		q = line.find_last_of(']');
		if (q != std::string::npos) {
			// Valid section, not empty like "[]"?
			if (q > p + 1) {
				s = line.substr(p + 1, q - p - 1);
				section.setString(s);
				key.clear();
				retVal = true;
			}
		}
	}
	if (!retVal) {
		// Invalid section identifier found like "[<some text>"
		section.clear();
		key.clear();
	}
	setSection(retVal);
	return retVal;
}

bool TIniValue::extractSection(const std::string::const_iterator& pos) {
	bool retVal = false;
	std::string s;
	size_t idx;
	std::string::const_iterator p = pos, q, eol = line.end();

	if (p == eol) {
		idx = line.find_first_of('[');
		if (idx != std::string::npos)
			p = line.begin() + idx;
	}

	if (p != eol) {
		idx = line.find_last_of(']');
		if (idx != std::string::npos) {
			q = line.begin() + idx;

			// Valid section, not empty like "[]"?
			if (q > p + 1) {
				s = std::string(p + 1, q);
				section.setString(s);
				key.clear();
				retVal = true;
			}
		}
	}
	if (!retVal) {
		// Invalid section identifier found like "[<some text>"
		section.clear();
		key.clear();
	}
	setSection(retVal);
	return retVal;
}

void TIniValue::parseLine() {
	if (line.size()) {
		setEmpty(!extractValue());
	} else {
		setEmpty(true);
	}
}

void TIniValue::setValue(const std::string& value) {
	this->value = value;
	// Rebuild proper line, wait for flush with parameter compress to do that...
	// rebuildLine();
}

void TIniValue::rebuildLine() {
	if (!isEmpty()) {
		switch (type) {
			case INI_NONE:
			case INI_EMPTY:
				break;
			case INI_SECTION:
				line = "[" + section.getString() + "]";
			break;
			case INI_COMMENT:
				util::trim(line);
				break;
			default:
				line = key.getString() + EQUAL_SIGN + value;
				break;
		}
	}
}



TIniFile::TIniFile() {
	prime();
}

TIniFile::TIniFile(const std::string& fileName) {
	prime();
	open(fileName);
}

TIniFile::~TIniFile() {
	clear();
}

void TIniFile::prime() {
	order = 0;
	fileExists = false;
	debug = false;
}

void TIniFile::open(const std::string& fileName) {
	this->fileName = fileName;
	this->filePath = util::filePath(fileName);
	if (!util::fileExists(filePath))
		throw util::app_error("TIniFile::TIniFile(): Folder <" + filePath + "> does not exists.");
	fileExists = util::fileExists(fileName);
	readIniFile();
}

void TIniFile::close() {
	fileExists = false;
	fileName.clear();
	filePath.clear();
	clear();
}

void TIniFile::clear() {
	order = 0;
	sections.clear();
	section.clear();
	section.setIndex(std::string::npos);
	util::clearObjectList(lines);
}

PIniValue TIniFile::addLineValue(std::string& line, const size_t index, const PIniValue anchor) {
	app::PIniValue o;
	o = new TIniValue(line, index, anchor, debug);
	lines.push_back(o);
	addSectionMap(o);
	return o;
}

void TIniFile::addSectionMap(const PIniValue o) {
	// Add section to map
	if (o->isSection()) {
		std::string s = o->getSection();
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		sections.insert(TSectionMapItem(s, o->getIndex()));
	}
}

void TIniFile::readIniFile() {
	if (fileExists) {
		std::ifstream strm;
		std::string line;
		app::PIniValue o, prev = nil;
		strm.open(fileName, std::ios_base::in);
		try {

			// Read first line
			std::getline(strm, line);
			if (strm.good()) {
				prev = addLineValue(line, lines.size(), prev);
			}

			// Read all lines until EOF
			while (strm.good()) {
				std::getline(strm, line);
				prev = addLineValue(line, lines.size(), prev);
			}

			// Check last read line for valid entry:
			// 1. <line>\n  --> valid entry
			// 2. <line>EOF --> valid entry
			// 3. EOF --> delete last read empty string entry
			if (line.empty() && !lines.empty()) {
				o = lines[util::pred(lines.size())];
				util::freeAndNil(o);
				lines.pop_back();
			}

		} catch (const std::exception& e)	{
			strm.close();
			throw;
		}
		strm.close();
	}
}

void TIniFile::flush(const bool compress) {
	if (!lines.size())
		return;
	/*
	if (util::fileExists(fileName))
		util::copyFile(fileName, fileName + ".bak");
	if (compress)
		writeIniFile(fileName + ".raw", false);
	*/
	writeIniFile(fileName, compress);
}

void TIniFile::writeIniFile(const std::string fileName, bool compress) {
	std::ofstream strm;
	std::string line;
	app::PIniValue o, prev = nil;
	strm.open(fileName, std::ios_base::out);
	try {
		for(size_t i = 0; i<lines.size(); i++) {
			o = lines[i];
			if (util::assigned(o)) {
				if (o->isEmpty()) {
					continue;
				}
				// Skip empty lines for output
				if (compress) {
					o->rebuildLine();
				}
				// Add single empty line on change of section
				if (util::assigned(prev)) {
					if ( prev->getSectionHash() != o->getSectionHash() &&
						 //prev->getType() != INI_COMMENT &&
						 !prev->isEmpty() ) {
						strm << std::endl;
					}
				}
				line = o->getLine();
				if (debug) std::cout << "TIniFile::writeIniFile() Wite line \"" << line << "\"" << std::endl;
				strm << line << std::endl;
			}
			prev = o;
		}
	} catch (const std::exception& e)	{
		strm.close();
		std::string sExcept = e.what();
		throw util::app_error("TIniFile::writeIniFile() failed \"" + sExcept + "\" for file <" + fileName + ">");;
	}
	strm.close();
}

void TIniFile::sort(const std::string section) {
	// Natural ascending sort method
	auto sorter = [] (const TIniValue* o1, const TIniValue* o2) {
		return util::strnatcasesort(o1->getKey(), o2->getKey());
	};
	sort(section, sorter);
}

void TIniFile::sort() {
	// Sort all sections by natural ascending sort method
	auto sorter = [this] (const TSectionValue& value) {
		sort(value.first);
	};
	std::for_each(sections.begin(), sections.end(), sorter);
}

void TIniFile::reorder(const std::string section) {
	// Order items by read sequence
	auto sorter = [] (const TIniValue* o1, const TIniValue* o2) {
		return o1->getOrder() < o2->getOrder();
	};
	sort(section, sorter);
}

void TIniFile::reorder() {
	// Reorder all sections by read sequence
	auto sorter = [this] (const TSectionValue& value) {
		reorder(value.first);
	};
	std::for_each(sections.begin(), sections.end(), sorter);
}

void TIniFile::debugOutput() {
	app::PIniValue o;
	std::cout << "Contents of <" << fileName << ">" << std::endl;
	for (size_t i=0; i<lines.size(); i++) {
		o = lines[i];
		if (!o->isSection()) {
			std::string sep;
			sep.reserve(10);
			sep = "\t";
			//if (o->getKey().size() <= 24)
			//	sep += "\t";
			if (o->getKey().size() <= 16)
				sep.append("\t");
			if (o->getKey().size() <= 8)
				sep.append("\t");
			if (o->isEmpty())
				sep.append("\t");
			std::cout << "Index " << o->getIndex() << " Section " << o->isSection() << " Hash " << o->getSectionHash() << "\t" << " Empty " << o->isEmpty() << " Key \"" << o->getKey() << "\"" << sep << " Value \"" << o->getValue() << "\"" << std::endl;
		} else {
			std::cout << "Index " << o->getIndex() << " Section " << o->isSection() << " Hash " << o->getSectionHash() << "\t" << " Empty " << o->isEmpty() << " Section [" << o->getSection() << "] :" << std::endl;
		}
	}
	std::cout << std::endl;
	if (sections.size() > 0) {
		std::cout << "Sections map of <" << fileName << ">" << std::endl;
		TSectionMap::const_iterator it = sections.begin();
		while (it != sections.end()) {
			std::cout << "Section " << it->first << " Index " << it->second << std::endl;
			it++;
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

void TIniFile::setLineOrder(app::PIniValue value) const {
	++order;
	value->setOrder(order);
}

int TIniFile::readInteger(const std::string& key, int defValue) const {
	long int retVal = defValue;
	const char* p;
	char* q;
	app::PIniValue o;
	size_t idx = findKey(key);
	if (idx != std::string::npos) {
		o = lines[idx];
		if (util::assigned(o)) {
			setLineOrder(o);
			o->setType(INI_INT);
			if (o->getValue().size() > 0) {
				p = o->getValue().c_str();
				errno = EXIT_SUCCESS;
				if (syslocale != cc_CC)
					retVal = strtol_l(p, &q, 0, cc_CC());
				else
					retVal = strtol(p, &q, 0);
				if (errno != EXIT_SUCCESS || p == q)
					retVal = defValue;
#ifdef STL_HAS_NUMERIC_LIMITS
				if (retVal > std::numeric_limits<int>::max() ||
					retVal < std::numeric_limits<int>::min())
					retVal = defValue;
#else
				if (retVal > INT_MAX || retVal < INT_MIN)
					retVal = defValue;
#endif
			}
		}
	}
	return retVal;
}

bool TIniFile::readBool(const std::string& key, bool defValue) const {
	bool retVal = defValue;
	std::string s;
	app::PIniValue o;
	size_t idx = findKey(key);
	if (idx != std::string::npos) {
		o = lines[idx];
		if (util::assigned(o)) {
			setLineOrder(o);
			EEntryType type;
			retVal = readBoolValueAndType(o->getValue(), type);
			o->setType(type);
		}
	}
	return retVal;
}

bool TIniFile::readBoolValueAndType(std::string value, EEntryType& type) {
	bool retVal;
	std::transform(value.begin(), value.end(), value.begin(), ::tolower);
	do {
		retVal = true;
		if (value == "true") {
			type = INI_BLTRUE;
			continue;
		}
		if (value == "yes") {
			type = INI_BLYES;
			continue;
		}
		if (value == "on") {
			type = INI_BLON;
			continue;
		}
		if (value == "1") {
			type = INI_BL01;
			continue;
		}
		retVal = false;
		if (value == "false") {
			type = INI_BLTRUE;
			continue;
		}
		if (value == "no") {
			type = INI_BLYES;
			continue;
		}
		if (value == "off") {
			type = INI_BLON;
			continue;
		}
		if (value == "0") {
			type = INI_BL01;
			continue;
		}
	} while (false);
	return retVal;
}

std::string TIniFile::writeBoolValueForType(const bool value, const EEntryType type) {
	switch (type) {
		case INI_BLTRUE:
			if (value) return "true";
			else return "false";
			break;
		case INI_BLYES:
			if (value) return "yes";
			else return "no";
			break;
		case INI_BLON:
			if (value) return "on";
			else return "off";
			break;
		case INI_BL01:
			if (value) return "1";
			else return "0";
			break;
		default:
			if (value) return "true";
			else return "false";
			break;
	}
}

double TIniFile::readDouble(const std::string& key, double defValue) const {
	double retVal = defValue;
	const char* p;
	char* q;
	app::PIniValue o;
	size_t idx = findKey(key);
	if (idx != std::string::npos) {
		o = lines[idx];
		if (util::assigned(o)) {
			setLineOrder(o);
			o->setType(INI_DOUBLE);
			if (o->getValue().size() > 0) {
				p = o->getValue().c_str();
				errno = EXIT_SUCCESS;
				if (syslocale != cc_CC)
					retVal = strtod_l(p, &q, cc_CC());
				else
					retVal = strtod(p, &q);
				if (errno != EXIT_SUCCESS || p == q)
					retVal = defValue;
			}
		}
	}
	return retVal;
}

bool TIniFile::readText(const std::string& key, std::string& value) const {
	value.clear();
	app::PIniValue o;
	size_t idx = findKey(key);
	if (idx != std::string::npos) {
		o = lines[idx];
		if (util::assigned(o)) {
			setLineOrder(o);
			o->setType(INI_STRING);
			if (o->getValue().size() > 0) {
				value = o->getValue();
				return true;
			}
		}
	}
	return false;
}

std::string TIniFile::readString(const std::string& key, const std::string& defValue) const {
	std::string retVal(defValue);
	app::PIniValue o;
	size_t idx = findKey(key);
	if (idx != std::string::npos) {
		o = lines[idx];
		if (util::assigned(o)) {
			setLineOrder(o);
			o->setType(INI_STRING);
			if (o->getValue().size() > 0)
				retVal = o->getValue();
		}
	}
	return retVal;
}

std::string TIniFile::readPath(const std::string& key, const std::string& defValue) const {
	std::string retVal(defValue);
	app::PIniValue o;
	size_t idx = findKey(key);
	if (idx != std::string::npos) {
		o = lines[idx];
		if (util::assigned(o)) {
			setLineOrder(o);
			o->setType(INI_PATH);
			if (o->getValue().size() > 0)
				retVal = o->getValue();
		}
	}
	return validPath(retVal);
}

size_t TIniFile::readSize(const std::string& key, size_t defValue) const {
	size_t retVal = defValue;
	std::string s(readString(key, ""));
	if (!s.empty()) {
		retVal = util::strToSize(s);
	}
	return retVal;
}

uint64_t TIniFile::readHex(const std::string& key, uint64_t defValue) const {
	size_t retVal = defValue;
	std::string s(readString(key, ""));
	if (s.size() > 2) {
		if (0 == util::strncasecmp("0x", s, 2)) {
			s = s.substr(2);
			if (!s.empty()) {
				retVal = util::strToUnsigned64(s, defValue, syslocale, 16);
			}
		}
		retVal = util::strToSize(s);
	}
	return retVal;
}

void TIniFile::writeInteger(const std::string& key, int value) {
	std::string s;
	if (syslocale != cc_CC)
		s = util::cprintf(cc_CC, "%d", value);
	else
		s = util::cprintf("%d", value);
	writeItem(key, s, INI_INT);
}

void TIniFile::writeBool(const std::string& key, bool value, const EEntryType type) {
	std::string s = writeBoolValueForType(value, type);
	writeItem(key, s, type);
}

void TIniFile::writeDouble(const std::string& key, double value) {
	std::string s;
	if (syslocale != cc_CC)
		s = util::cprintf(cc_CC, "%.12f", value);
	else
		s = util::cprintf("%.12f", value);
	writeItem(key, s, INI_DOUBLE);
}

void TIniFile::writeString(const std::string& key, const std::string& value) {
	writeItem(key, value, INI_STRING);
}

void TIniFile::writePath(const std::string& key, const std::string& value) {
	writeItem(key, validPath(value), INI_PATH);
}

void TIniFile::writeSize(const std::string& key, size_t value) {
	// Attention: sizeToStr(value, 0, VD_BINARY) cuts the decimal digits!
	// --> Value should be modulo of 1024!
	std::string s = util::sizeToStr(value, 0, util::VD_BINARY);
	writeString(key, s);
}

void TIniFile::writeHex(const std::string& key, uint64_t value) {
	std::string s = "0x" + printf(PRIx64_FMT_A, value);
	writeString(key, s);
}

std::string TIniFile::printf(const std::string &fmt, ...) const {
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

void TIniFile::writeItem(const std::string& key, const std::string& value, const EEntryType type) {
	app::PIniValue o;
	size_t idx = findKey(key);
	if (debug) std::cout << "TIniFile::writeItem(0) Write [" << key << " = " << value << "]" << std::endl;

	// Overwrite existing item value?
	if (idx != std::string::npos) {
		o = lines[idx];
		if (util::assigned(o)) {
			// Take boolean format string from existing entry
			// and replace given format string with user changed format from file
			if (util::isMemberOf(type, INI_BLYES, INI_BLTRUE, INI_BL01, INI_BLON, INI_BLDEFAULT)) {
				EEntryType ot, et;
				std::string s;
				ot = o->getType();
				bool b = readBoolValueAndType(value, et);
				s = writeBoolValueForType(b, ot);
				o->setValue(s);
			} else {
				// Write for all other types unchanged to existing object
				if (debug) std::cout << "TIniFile::writeItem(1) Write [" << key << " = " << value << "]" << std::endl;
				o->setValue(value);
				o->setType(type);
			}
		}
	} else {
		// Create new item!
		std::string line = createLine(key, value);
		if (debug) std::cout << "TIniFile::writeItem(2) Line created [" << line << "]" << std::endl;
		idx = findInsertIndex();
		if (idx != std::string::npos) {
			// Add line at the end of file
			if (idx >= lines.size()) {
				addNewLine(line, type);
			} else {
				// Insert Item at index position
				insertNewLine(line, idx, type);
			}
		}
	}
}

bool TIniFile::deleteKey(const std::string& key) {
	bool retVal = false;
	app::PIniValue o;
	size_t idx = findKey(key);
	if (idx != std::string::npos) {
		o = lines[idx];
		lines.erase(lines.begin() + idx);
		util::freeAndNil(o);
		rebuildIndex(idx);
		retVal = true;
	}
	return retVal;
}

std::string TIniFile::validPath(const std::string& path) const {
	std::string s(path);
	util::trim(s);
	if (!s.empty())
		if (s[0] != sysutil::PATH_SEPERATOR)
			s = sysutil::PATH_SEPERATOR + s;
	if (s.size() >= 1)
		if (s[util::pred(s.size())] != sysutil::PATH_SEPERATOR)
			s += sysutil::PATH_SEPERATOR;
	return s;
}

size_t TIniFile::findInsertIndex() {
	app::PIniValue o = nil;
	app::PIniValue prev = nil;
	size_t retVal = std::string::npos;
	size_t p = 0;

	// Section index not set!!!
	if (section.getIndex() == std::string::npos)
		return retVal;

	// Find last not empty entry in section
	// --> Insert new item at position + 1
	p = section.getIndex();
	if (p > 0)
		prev = lines[util::pred(p)];

	while (p < lines.size()) {
		o = lines[p];
		if (util::assigned(o)) {
			if (o->isEmpty() && o->getSectionHash() == section.getHash()) {
				retVal = p;
				break;
			}
			if (util::assigned(prev)) {
				if (prev->getSectionHash() == section.getHash() && prev->getSectionHash() != o->getSectionHash()) {
					retVal = p;
					break;
				}
			}
		}
		prev = o;
		p++;
	}

	// EOF reached, but no index found
	// Add entry at new position --> lines.size()
	if (retVal == std::string::npos && util::assigned(o) && p >= lines.size()) {
		if (o->getSectionHash() == section.getHash())
			retVal = lines.size();
	}

	if (retVal == std::string::npos) {
		debugOutput();
		throw util::app_error("TIniFile::findInsertIndex(): Failed to get index for hash <" + std::to_string((size_u)section.getHash()) + ">");
	}

	return retVal;
}

size_t TIniFile::findKey(const std::string& key) const {
	app::PIniValue o;
	size_t i = 0;
	if (section.getIndex() != std::string::npos)
		i = section.getIndex();
	util::hash_type hash = util::calcHash(key);
	for (; i < lines.size(); i++) {
		o = lines[i];
		if (!o->isSection() && o->getSectionHash() == section.getHash() && o->getKeyHash() == hash)
			return i;
	}
	return std::string::npos;
}

void TIniFile::setSection(const std::string& section) {
	size_t idx = findSection(section);
	if (idx == std::string::npos) {
		// Section not found --> create a new one
		idx = createSection(section);
	}
	// Simply take reference of THashKey, do not recalculate hash...
	// --> avoid this->section.setString(lines[idx]->getSection());
	this->section.assign(lines[idx]->getSectionHashKey());
	this->section.setIndex(idx);
}

int TIniFile::deleteSection(const std::string& section) {
	util::hash_type hash = util::calcHash(section);
	size_t idx = this->section.getIndex();
	int retVal = 0;

	// Find section if not set before
	if (this->section.getHash() != hash) {
		idx = findSection(section);
		// Is given section valid?
		if (idx == std::string::npos)
			return retVal;
	}

	size_t i = idx;
	app::PIniValue o;

	while (i < lines.size()) {
		o = lines[i];
		if (o->getSectionHash() == hash) {
			if (o->isSection()) {
				// Remove section from map
				std::string s = lines[i]->getSection();
				std::transform(s.begin(), s.end(), s.begin(), ::tolower);
				sections.erase(s);
			}
			lines.erase(lines.begin() + i);
			util::freeAndNil(o);
			retVal++;
		} else
			break; // i++; --> break here because new section reached.
	}

	// Rebuild index and set section to first section (index 0)
	if (!lines.empty()) {
		rebuildIndex(idx);
		idx = 0;
		this->section.assign(lines[idx]->getSectionHashKey());
		this->section.setIndex(idx);
	} else {
		this->section.clear();
		this->section.setIndex(std::string::npos);
		sections.clear();
	}

	return retVal;
}

int TIniFile::readSection(const std::string& section, util::TVariantValues& values) {
	values.clear();
	size_t idx = findSection(section);
	if (std::string::npos != idx) {
		util::hash_type hash = util::calcHash(section);
		app::PIniValue o;
		size_t i = idx;
		while(i < lines.size()) {
			o = lines[i];
			if (o->getSectionHash() == hash) {
				if (!(o->isSection() || o->isEmpty())) {
					// Read key/value pair
					const std::string& key = o->getKey();
					const std::string& value = o->getValue();
					values.add(key, value);
				}
			} else
				break;
			i++;
		}
	}
	return values.size();
}

size_t TIniFile::findSection(const std::string& section) {
	std::string s = section;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);

	TSectionMap::const_iterator it = sections.find(s);
	if (it != sections.end())
		return it->second;

	return std::string::npos;
}

size_t TIniFile::createSection(const std::string& section) {
	std::string line = "[" + section + "]";
	addEmptyLine();
	addNewLine(line, INI_SECTION);
	return util::pred(lines.size());
}

std::string TIniFile::createLine(const std::string& Key, const std::string& value) {
	return Key + EQUAL_SIGN + value;
}

// Add empty line at the end of list
void TIniFile::addEmptyLine() {
	if (lines.size() > 0) {
		app::PIniValue o;
		o = lines[util::pred(lines.size())];
		if (util::assigned(o)) {
			if (!o->isEmpty())
				addNewLine("", INI_EMPTY);
		}
	}
}

// Add new item at the end of list
void TIniFile::addNewLine(const std::string& line, const EEntryType type) {
	app::PIniValue o, prev = nil;
	if (lines.size())
		prev = lines[util::pred(lines.size())];
	o = new TIniValue(line, lines.size(), prev, debug);
	o->setType(type);
	lines.push_back(o);
	addSectionMap(o);
}

// Insert new item at given position
void TIniFile::insertNewLine(const std::string& line, size_t pos, const EEntryType type) {
	if (pos > 0 && pos < lines.size()) {
		TIniValues::iterator it;
		it = lines.begin();
		app::PIniValue o, prev = nil;

		// Insert item at given position
		if (pos < lines.size() && pos >= 0)
			prev = lines[util::pred(pos)];
		o = new TIniValue(line, pos, prev, debug);
		o->setType(type);
		lines.insert(it+pos, o);
		addSectionMap(o);

		// Rebuild index up from pos
		rebuildIndex(pos);

	}
}

void TIniFile::rebuildIndex(size_t idx) {
	// Rebuild index up from idx
	if (!lines.empty()) {
		if (idx >= lines.size())
			idx = 0;
		for (size_t i=idx; i<lines.size(); i++) {
			lines[i]->setIndex(i);
			if (lines[i]->isSection()) {
				// Reindex section map
				std::string s = lines[i]->getSection();
				std::transform(s.begin(), s.end(), s.begin(), ::tolower);
				TSectionMap::iterator it = sections.find(s);
				if (it != sections.end()) {
					it->second = i;
				} else {
					throw util::app_error("TIniFile::rebuildIndex() failed, section <" + s + "> not found.");
				}
				// Short form without error detection
				// sections[s] = i;
			}
		}
	}
}

} /* namespace app */
