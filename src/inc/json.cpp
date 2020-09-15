/*
 * json.cpp
 *
 *  Created on: 28.06.2015
 *      Author: Dirk Brinkmeier
 */

#include <algorithm>
#include <iostream>
#include <fstream>
#include "json.h"
#include "ansi.h"
#include "convert.h"
#include "variant.h"
#include "fileutils.h"

namespace util {


TJsonValue::TJsonValue() {
	type = EVT_UNKNOWN;
}

TJsonValue::~TJsonValue() {
}

void TJsonValue::clear() {
	key.clear();
	value.clear();
	line.clear();
	type = EVT_UNKNOWN;
}

void TJsonValue::invalidate() {
	if (isValid())
		clear();
}

bool TJsonValue::compare(const std::string& s1, const std::string& s2) {
	if (s1.size() == s2.size() && (s1 == s2))
		return true;
	return false;
};

void TJsonValue::setKey(const std::string& key) {
	if (!compare(this->key, key)) {
		this->key = key;
		invalidate();
	}
};

void TJsonValue::setValue(const std::string& value) {
	if (!compare(this->value, value)) {
		this->value = value;
		invalidate();
	}
};

void TJsonValue::setType(const EVariantType type) {
	if (this->type != type) {
		this->type = type;
		invalidate();
	}
}

void TJsonValue::update(const std::string& key, const std::string& value, const EVariantType type) {
	bool invalid = false;
	if (!compare(this->key, key)) {
		this->key = key;
		invalid = true;
	}
	if (!compare(this->value, value)) {
		this->value = value;
		invalid = true;
	}
	if (this->type != type) {
		this->type = type;
		invalid = true;
	}
	if (invalid) {
		invalidate();
	}
};

std::string TJsonValue::valueToStr(const std::string& preamble, const std::string& key, const std::string& value, const EVariantType type) {
	std::string s;
	if (!key.empty()) {
		s.reserve(preamble.size() + key.size() + value.size() + 16);
		if (!value.empty()) {
			switch (type) {
				case EVT_TIME:
				case EVT_BLOB:
				case EVT_STRING:
				case EVT_WIDE_STRING:
					// Quoted values:
					//  "Value": "1234,98"
					//  "City": "Hill Valley"
					//  "Date": "2015-10-21 07:28:00.000"
					s = "\"" + key + "\" : " + "\"" + value + "\"";
					break;
				default:
					// Unquoted values:
					//  "Latitude": 37.371991
					//  "Latitude": null
					//  "Valid": false
					s = "\"" + key + "\" : " + value;
					break;
			}
		} else {
			s = "\"" + key + "\" : " + JSON_NULL;
		}
		if (!preamble.empty())
			s = preamble + s;
	}
	return s;
}

std::string TJsonValue::asJSON(const std::string& preamble) {
	if (line.empty())
		line = valueToStr(preamble, key, value, type);
	return line;
}

std::string TJsonValue::escape(const std::string& value) {
	return util::escape(value);
}

std::string TJsonValue::unescape(const std::string& value) {
	return util::unescape(value);
}

std::string TJsonValue::boolToStr(const bool value) {
	return value ? util::JSON_TRUE : util::JSON_FALSE;
}


TJsonList::TJsonList() {
	prime();
}

TJsonList::~TJsonList() {
}

void TJsonList::prime() {
	type = EJT_DEFAULT;
	mode = EEM_DEFAULT;
	opened = false;
}

void TJsonList::clear() {
	TStringList::clear();
	prime();
}

void TJsonList::push_back(const std::string& key, const std::string& value, EJsonEntryType type, const std::string& preamble) {
	if (EJE_LIST == type) {
		TStringList::add(preamble + "\"" + key + "\" : " + value + ",");
	} else {
		TStringList::add(preamble + "\"" + key + "\" : " + value);
	}
}


void TJsonList::add(const char* value) {
	if (util::assigned(value)) {
		std::string s(value);
		add(s);
	}
}

void TJsonList::add(const std::string& value) {
	TStringList::add(value);
}

void TJsonList::add(const std::string& key, const bool value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = value ? util::JSON_TRUE : util::JSON_FALSE;
		push_back(key, s, type, preamble);
	}
}

void TJsonList::add(const std::string& key, const double value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = util::cprintf(app::en_US, "%.4f", value);
		push_back(key, s, type, preamble);
	}
}


void TJsonList::add(const std::string& key, const int8_t value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = util::cprintf(app::en_US, "%d", value);
		push_back(key, s, type, preamble);
	}
}

void TJsonList::add(const std::string& key, const uint8_t  value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = util::cprintf(app::en_US, "%u", value);
		push_back(key, s, type, preamble);
	}
}


void TJsonList::add(const std::string& key, const int16_t value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = util::cprintf(app::en_US, "%d", value);
		push_back(key, s, type, preamble);
	}
}

void TJsonList::add(const std::string& key, const uint16_t  value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = util::cprintf(app::en_US, "%u", value);
		push_back(key, s, type, preamble);
	}
}


void TJsonList::add(const std::string& key, const int32_t value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = util::cprintf(app::en_US, "%d", value);
		push_back(key, s, type, preamble);
	}
}

void TJsonList::add(const std::string& key, const uint32_t  value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = util::cprintf(app::en_US, "%u", value);
		push_back(key, s, type, preamble);
	}
}


void TJsonList::add(const std::string& key, const int64_t value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = util::cprintf(app::en_US, PRI64_FMT_A, value);
		push_back(key, s, type, preamble);
	}
}

void TJsonList::add(const std::string& key, const uint64_t  value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		std::string s = util::cprintf(app::en_US, PRIu64_FMT_A, value);
		push_back(key, s, type, preamble);
	}
}


void TJsonList::add(const std::string& key, const std::string& value, EJsonEntryType type, const std::string& preamble) {
	if (!key.empty()) {
		if (!value.empty()) {
			if (EEM_ESCAPED == mode) {
				if (EJE_LIST == type) {
					TStringList::add(preamble + "\"" + key + "\" : \"" + TJsonValue::escape(value) + "\",");
				} else {
					TStringList::add(preamble + "\"" + key + "\" : \"" + TJsonValue::escape(value) + "\"");
				}
			} else {
				if (EJE_LIST == type) {
					TStringList::add(preamble + "\"" + key + "\" : \"" + value + "\",");
				} else {
					TStringList::add(preamble + "\"" + key + "\" : \"" + value + "\"");
				}
			}
		} else {
			append(key, JSON_NULL, type, preamble);
		}
	}
}

void TJsonList::add(const std::string& key, const char* value, EJsonEntryType type, const std::string& preamble) {
	if (util::assigned(value)) {
		std::string s(value);
		add(key, s, type, preamble);
	}
}

void TJsonList::append(const std::string& key, const std::string& value, EJsonEntryType type, const std::string& preamble) {
	// Same as add() but not quoted and not escaped
	// e.g. for special values like true, false, null, ...
	if (!key.empty()) {
		if (!value.empty()) {
			push_back(key, value, type, preamble);
		} else {
			push_back(key, JSON_NULL, type, preamble);
		}
	}
}


void TJsonList::open(const EJsonArrayType type) {
	if (!opened) {
		this->type = type;
		switch (type) {
			case EJT_ARRAY:
				TStringList::add("[");
				break;
			case EJT_OBJECT:
				TStringList::add("{");
				break;
			default:
				break;
		}
		opened = true;
	}
}

void TJsonList::close(const bool last) {
	close(last ? EJE_LAST : EJE_LIST);
}

void TJsonList::close(const EJsonEntryType type) {
	if (opened) {
		switch (this->type) {
			case EJT_ARRAY:
				if (EJE_LAST == type) {
					TStringList::add("]");
				} else {
					TStringList::add("],");
				}
				break;
			case EJT_OBJECT:
				if (EJE_LAST == type) {
					TStringList::add("}");
				} else {
					TStringList::add("},");
				}
				break;
			default:
				break;
		}
		opened = false;
	}
}


void TJsonList::openArray(const std::string name) {
	if (name.empty()) {
		TStringList::add("[");
	} else {
		TStringList::add(util::quote(name) +  " : [");
	}
}

void TJsonList::openObject(const std::string name) {
	if (name.empty()) {
		TStringList::add("{");
	} else {
		TStringList::add(util::quote(name) +  " : {");
	}
}

void TJsonList::closeArray(const bool last) {
	closeArray(last ? EJE_LAST : EJE_LIST);
}

void TJsonList::closeObject(const bool last) {
	closeObject(last ? EJE_LAST : EJE_LIST);
}

void TJsonList::closeArray(const EJsonEntryType type) {
	if (EJE_LAST == type) {
		TStringList::add("]");
	} else {
		TStringList::add("],");
	}
}

void TJsonList::closeObject(const EJsonEntryType type) {
	if (EJE_LAST == type) {
		TStringList::add("}");
	} else {
		TStringList::add("},");
	}
}


const std::string& TJsonList::text() const {
	return TStringList::text(' ');
}

void TJsonList::saveToFile(const std::string fileName) const {
	TStringList::saveToFile(fileName);
}

void TJsonList::debugOutput() const {
	TStringList::debugOutput();
}



TJsonParser::TJsonParser() {
	debug = false;
	init();
}

TJsonParser::~TJsonParser() {
}

void TJsonParser::init() {
	onHeaderColumnMethod = nil;
	onDataFieldMethod = nil;
}

void TJsonParser::clear() {
	json.clear();
}

void TJsonParser::loadFromFile(const std::string& fileName, const app::ECodepage codepage) {
	clear();
	if (fileExists(fileName)) {

		// Resize line buffer
		size_t size = util::fileSize(fileName);
		if (size > 0) {
			json.reserve(codepage != app::ECodepage::CP_DEFAULT ? (size * 3 / 2) : size);
		}

		std::string line;
		std::ifstream is;
		TStreamGuard<std::ifstream> strm(is);
		strm.open(fileName, std::ios_base::in);

		// Read all lines until EOF
		while (is.good()) {
			std::getline(is, line);
			if (!line.empty()) {
				if (codepage != app::ECodepage::CP_DEFAULT)
					json += util::TStringConvert::SingleByteToMultiByteString(line, codepage);
				else
					json += line;
			}
		}
	}
}


void TJsonParser::parse(size_t rows) {
	parse(json, rows);
}


void TJsonParser::parse(const std::string& json, size_t rows) {
	if (json.size() > 0) {
		std::string key, value;
		EParserState state = ST_PREAMBLE;
		EVariantType type = EVT_UNKNOWN;
		size_t count = std::max(rows, (size_t)1);
		size_t ndx = 0;
		size_t p = 0;
		do {
			switch (state) {
				case ST_PREAMBLE:
					if (debug) std::cout << app::blue << "ST_PREAMBLE : <" << json[p] << ">"<< app::reset << std::endl;
					// Ignore everything prior to object or array
					if (json[p] == '{') {
						state = ST_OBJECT_KEY;
						break;
					}
					if (json[p] == '[') {
						state = ST_ARRAY;
						break;
					}
					break;

				case ST_OBJECT_KEY:
					if (debug) std::cout << app::blue << "ST_OBJECT_KEY : <" << json[p] << ">"<< app::reset << std::endl;
					if (json[p] == '"') {
						key = extractQuotedStr(json, p);
						if (debug) std::cout << app::magenta << "ST_OBJECT_KEY : <" << key << ">"<< app::reset << std::endl;
						type = EVT_STRING;
						state = ST_OBJECT_KEY_END;
						p--; // Compensate p++ at the end of loop!
						break;
					}
					if (util::isAlphaNumeric(json[p])) {
						key = extractNativeStr(json, p);
						if (debug) std::cout << app::magenta << "ST_OBJECT_KEY : <" << key << ">"<< app::reset << std::endl;
						state = ST_OBJECT_KEY_END;
						p--;
						break;
					}
					if (json[p] == '[') {
						state = ST_ARRAY;
						break;
					}
					break;

				case ST_OBJECT_KEY_END:
					if (debug) std::cout << app::blue << "ST_OBJECT_KEY_END : <" << json[p] << ">"<< app::reset << std::endl;
					if (json[p] == ':') {
						state = ST_OBJECT_VALUE;
						break;
					}
					break;

				case ST_OBJECT_VALUE:
					if (debug) std::cout << app::blue << "ST_OBJECT_VALUE : <" << json[p] << ">"<< app::reset << std::endl;
					if (json[p] == '"') {
						value = extractQuotedStr(json, p);
						if (debug) std::cout << app::magenta << "ST_OBJECT_VALUE : <" << value << ">"<< app::reset << std::endl;
						state = ST_OBJECT_VALUE_END;
						p--;
						break;
					}
					if (!util::isWhiteSpace(json[p])) {
						value = extractNativeStr(json, p);
						if (debug) std::cout << app::magenta << "ST_OBJECT_VALUE : <" << value << ">"<< app::reset << std::endl;
						state = ST_OBJECT_VALUE_END;
						p--;
						break;
					}
					break;

				case ST_OBJECT_VALUE_END:
					if (debug) std::cout << app::yellow << "ST_OBJECT_VALUE_END : <" << json[p] << ">"<< app::reset << std::endl;
					if (json[p] == ',') {
						// Another key/value pair is expected
						state = ST_OBJECT_KEY;
					} else {
						if (json[p] == '}') {
							// End of object reached
							state = ST_PREAMBLE;
						}
					}
					if (state != ST_OBJECT_VALUE_END) {
						if (!key.empty()) {
							if (value == JSON_NULL) {
								value.clear();
								type = EVT_NULL;
							}
							if (type != EVT_NULL) {
								type = util::TVariant::guessType(value, 5);
							}
							if (ndx == 0) {
								onHeaderColumn(key, type);
							}
							if (ndx < count) {
								onDataField(key, value, ndx, type);
							}
						}
						key.clear();
						value.clear();
						type = EVT_UNKNOWN;
					}
					if (state == ST_PREAMBLE) {
						ndx++;
						if (debug) std::cout << app::yellow << "ST_PREAMBLE : Next row = " << ndx << app::reset << std::endl;
						if (ndx >= count) {
							break;
						}
					}
					break;

				case ST_ARRAY:
					if (json[p] == '{') {
						state = ST_OBJECT_KEY;
						break;
					}
					if (json[p] == ']') {
						state = ST_PREAMBLE;
						break;
					}
					break;

				default:
					break;

			}
			p++;
		} while (p < json.size() && ndx < count);
	}
}

std::string TJsonParser::extractQuotedStr(const std::string& s, size_t& pos) {
	// Caution: function may return parameter pos >= s.size()
	return util::extractQuotedStr(s, pos, '"');
}

std::string TJsonParser::extractNativeStr(const std::string& s, size_t& pos) {
	size_t q = pos;
	do {
		if (debug) std::cout << app::green << "TJsonParser::extractNativeStr(" << s[pos] << ")" << app::reset << std::endl;
		if (!util::isAlphaNumeric(s[pos])) {
			if (debug) std::cout << app::red << "TJsonParser::extractNativeStr() --> Found!" << app::reset << std::endl;
			if (q < pos) {
				if (debug) std::cout << app::red << "TJsonParser::extractNativeStr(" << s.substr(q, pos - q) << ")" << app::reset << std::endl;
				return s.substr(q, pos - q);
			} else {
				if (debug) std::cout << app::red << "TJsonParser::extractNativeStr() --> INVALID!" << app::reset << std::endl;
				return "";
			}
		}
		pos++;
	} while (pos < s.size());
	return "";
}

void TJsonParser::onHeaderColumn(const std::string& column, const util::EVariantType type) {
	if (nil != onHeaderColumnMethod)
		onHeaderColumnMethod(column, type);
	if (debug) std::cout << app::blue << "TJsonParser::onHeaderColumn(\"" + column + "\")" << app::reset << std::endl;
}

void TJsonParser::onDataField(const std::string& key, const std::string& value, const size_t row, const EVariantType type) {
	if (nil != onDataFieldMethod)
		onDataFieldMethod(key, value, row, type);
	if (debug) std::cout << app::magenta << "TJsonParser::onDataField(\"" + key + "\", \"" + value + "\", " + \
			std::to_string((size_s)row) + ", " + std::to_string((size_s)type) +  ")" << app::reset << std::endl;
}

} /* namespace app */
