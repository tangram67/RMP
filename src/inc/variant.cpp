/*
 * variant.cpp
 *
 *  Created on: 30.05.2015
 *      Author: Dirk Brinkmeier
 */

#include <algorithm>
#include "gcc.h"
#include "ansi.h"
#include "json.h"
#include "ASCII.h"
#include "convert.h"
#include "variant.h"
#include "encoding.h"
#include "htmlutils.h"
#include "stringutils.h"

extern app::TLocale syslocale;

namespace util {


TVariant::TVariant() : locale(&syslocale) {
	prime();
}

TVariant::~TVariant() {
	release();
}

#ifdef STL_HAS_DELEGATING_CTOR

TVariant::TVariant(EVariantType type) : TVariant() {
	varType = type;
}

TVariant::TVariant(const int8_t value) : TVariant() {
	varType = EVT_INTEGER8;
	this->value.integer = (int64_t)value;
}

TVariant::TVariant(const int16_t value) : TVariant() {
	varType = EVT_INTEGER16;
	this->value.integer = (int64_t)value;
}

TVariant::TVariant(const int32_t value) : TVariant() {
	varType = EVT_INTEGER32;
	this->value.integer = (int64_t)value;
}

TVariant::TVariant(const int64_t value) : TVariant() {
	varType = EVT_INTEGER64;
	this->value.integer = value;
}

TVariant::TVariant(const uint8_t value) : TVariant() {
	varType = EVT_UNSIGNED8;
	this->value.uinteger = (uint64_t)value;
}

TVariant::TVariant(const uint16_t value) : TVariant() {
	varType = EVT_UNSIGNED16;
	this->value.uinteger = (uint64_t)value;
}

TVariant::TVariant(const uint32_t value) : TVariant() {
	varType = EVT_UNSIGNED32;
	this->value.uinteger = (uint64_t)value;
}

TVariant::TVariant(const uint64_t value) : TVariant() {
	varType = EVT_UNSIGNED64;
	this->value.uinteger = value;
}

TVariant::TVariant(const TDateTime& value) : TVariant() {
	varType = EVT_TIME;
	this->value.time = value;
}

TVariant::TVariant(const double value) : TVariant() {
	varType = EVT_DOUBLE;
	this->value.decimal = value;
}

TVariant::TVariant(const float value) : TVariant() {
	varType = EVT_DOUBLE;
	this->value.decimal = value;
}

TVariant::TVariant(const bool value) : TVariant() {
	varType = EVT_BOOLEAN;
	this->value.boolean = value;
}

TVariant::TVariant(const char *value) : TVariant() {
	varType = EVT_STRING;
	this->value.astring = std::string(value);
}

TVariant::TVariant(const char *value, size_t size) : TVariant() {
	varType = EVT_STRING;
	this->value.astring = std::string(value, size);
}

TVariant::TVariant(const std::string &value) : TVariant() {
	varType = EVT_STRING;
	this->value.astring = std::string(value);
}

TVariant::TVariant(const wchar_t *value) : TVariant() {
	varType = EVT_WIDE_STRING;
	this->value.wstring = std::wstring(value);
}

TVariant::TVariant(const wchar_t *value, size_t size) : TVariant() {
	varType = EVT_WIDE_STRING;
	this->value.wstring = std::wstring(value, size);
}

TVariant::TVariant(const std::wstring &value) : TVariant() {
	varType = EVT_WIDE_STRING;
	this->value.wstring = std::wstring(value);
}

TVariant::TVariant(const util::TBlob &value) : TVariant() {
	varType = EVT_BLOB;
	this->value.blob = value;
}

TVariant::TVariant(util::TBlob &&value) : TVariant() {
	varType = EVT_BLOB;
	this->value.blob.move(value);
}

TVariant::TVariant(const TVariant &value) : TVariant() {
	*this = value;
}

#else

TVariant::TVariant(EVariantType type) : locale(&syslocale) {
	prime();
	varType = type;
}

TVariant::TVariant(const int8_t value) : locale(&syslocale) {
	prime();
	varType = EVT_INTEGER8;
	this->value.integer = (int64_t)value;
}

TVariant::TVariant(const int16_t value) : locale(&syslocale) {
	prime();
	varType = EVT_INTEGER16;
	this->value.integer = (int64_t)value;
}

TVariant::TVariant(const int32_t value) : locale(&syslocale) {
	prime();
	varType = EVT_INTEGER32;
	this->value.integer = (int64_t)value;
}

TVariant::TVariant(const int64_t value) : locale(&syslocale) {
	prime();
	varType = EVT_INTEGER64;
	this->value.integer = value;
}

TVariant::TVariant(const uint8_t value) : locale(&syslocale) {
	prime();
	varType = EVT_UNSIGNED8;
	this->value.uinteger = (uint64_t)value;
}

TVariant::TVariant(const uint16_t value) : locale(&syslocale) {
	prime();
	varType = EVT_UNSIGNED16;
	this->value.uinteger = (uint64_t)value;
}

TVariant::TVariant(const uint32_t value) : locale(&syslocale) {
	prime();
	varType = EVT_UNSIGNED32;
	this->value.uinteger = (uint64_t)value;
}

TVariant::TVariant(const uint64_t value) : locale(&syslocale) {
	prime();
	varType = EVT_UNSIGNED64;
	this->value.uinteger = value;
}

TVariant::TVariant(const double value) : locale(&syslocale) {
	prime();
	varType = EVT_DOUBLE;
	this->value.decimal = value;
}

TVariant::TVariant(const float value) : locale(&syslocale) {
	prime();
	varType = EVT_DOUBLE;
	this->value.decimal = value;
}

TVariant::TVariant(const bool value) : locale(&syslocale) {
	prime();
	varType = EVT_BOOLEAN;
	this->value.boolean = value;
}

TVariant::TVariant(const char *value) : locale(&syslocale) {
	prime();
	varType = EVT_STRING;
	this->value.astring = std::string(value);
}

TVariant::TVariant(const char *value, size_t size) : locale(&syslocale) {
	prime();
	varType = EVT_STRING;
	this->value.astring = std::string(value, size);
}

TVariant::TVariant(const std::string &value) : locale(&syslocale) {
	prime();
	varType = EVT_STRING;
	this->value.astring = std::string(value);
}

TVariant::TVariant(const wchar_t *value) : locale(&syslocale) {
	prime();
	varType = EVT_WIDE_STRING;
	this->value.wstring = std::wstring(value);
}

TVariant::TVariant(const wchar_t *value, size_t size) : locale(&syslocale) {
	prime();
	varType = EVT_WIDE_STRING;
	this->value.wstring = std::wstring(value, size);
}

TVariant::TVariant(const std::wstring &value) : locale(&syslocale) {
	prime();
	varType = EVT_WIDE_STRING;
	this->value.wstring = std::wstring(value);
}

TVariant::TVariant(const util::TBlob &value) : locale(&syslocale) {
	prime();
	varType = EVT_BLOB;
	this->value.blob = value;
}

TVariant::TVariant(const TDateTime& value) : locale(&syslocale) {
	prime();
	varType = EVT_TIME;
	this->value.time = value;
}

TVariant::TVariant(app::TObject &value) : locale(&syslocale) {
	prime();
	varType = EVT_WIDE_STRING;
	this->value.object = &value;
}

TVariant::TVariant(const TVariant &value) : locale(&syslocale) {
	prime();
	*this = value;
}

#endif


void TVariant::prime() {
	locked = false;
	onChange = nil;
	clear();
	varType = EVT_UNKNOWN;
	timeType = EDT_DEFAULT;
	timePrecision = ETP_DEFAULT;
	// boolType = util::assigned(locale) ? VBT_LOCALE : VBT_BLDEFAULT;
	boolType = VBT_BLDEFAULT;
	precision = 0;
	setPrecision(2);
	// value.time.setType(time);
}

void TVariant::release() {
	value.blob.clear();
}

void TVariant::clear() {
	if (!cstr.empty())
		cstr.clear();
	value.integer = INT64_C(0);
	value.uinteger = UINT64_C(0);
	value.boolean = false;
	value.decimal = 0.0l;
	value.astring.clear();
	value.wstring.clear();
	value.time.clear();
	svalue = uvalue = false;
	release();
}

void TVariant::imbue(const app::TLocale& locale) {
	this->locale = &locale;
	value.time.imbue(locale);
};

void TVariant::changed() {
	json.invalidate();
	if (nil != onChange)
		onChange(*this);
}

std::string TVariant::printf(const app::TLocale& locale, const std::string &fmt, ...) const {
	if (!fmt.empty()) {
		int n;
		util::TStringBuffer buf;
		buf.reserve(fmt.size() * 7, false);
		buf.resize(fmt.size() * 3, false);

		app::TLocationGuard<app::TLocale> location(locale);
		if (locale != syslocale)
			location.change();

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

std::wstring TVariant::printf(const app::TLocale& locale, const std::wstring &fmt, ...) const {
	if (!fmt.empty()) {
		int n;
		util::TWideBuffer buf;
		buf.reserve(fmt.size() * 7, false);
		buf.resize(fmt.size() * 3, false);

		app::TLocationGuard<app::TLocale> location(locale);
		if (locale != syslocale)
			location.change();

		va_list ap;
		while (true) {

			va_start(ap, fmt);
			n = vswprintf(buf.data(), buf.size(), fmt.c_str(), ap);
			va_end(ap);

			if ((n > -1) && ((size_t)n < buf.size())) {
				return std::wstring(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
	}
	return fmt;
}


std::string TVariant::printf(const std::string &fmt, ...) const {
	if (!fmt.empty()) {
		int n;
		util::TStringBuffer buf;
		buf.reserve(fmt.size() * 7, false);
		buf.resize(fmt.size() * 3, false);

		app::TLocationGuard<app::TLocale> location(*locale);
		if (*locale != syslocale)
			location.change();

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

std::wstring TVariant::printf(const std::wstring &fmt, ...) const {
	if (!fmt.empty()) {
		int n;
		util::TWideBuffer buf;
		buf.reserve(fmt.size() * 7, false);
		buf.resize(fmt.size() * 3, false);

		app::TLocationGuard<app::TLocale> location(*locale);
		if (*locale != syslocale)
			location.change();

		va_list ap;
		while (true) {

			va_start(ap, fmt);
			n = vswprintf(buf.data(), buf.size(), fmt.c_str(), ap);
			va_end(ap);

			if ((n > -1) && ((size_t)n < buf.size())) {
				return std::wstring(buf.data(), (size_t)n);
			}

			if ((size_t)n == buf.size()) buf.resize(n + 1, false);
			else buf.resize(buf.size() * 2, false);

		}
	}
	return fmt;
}


void TVariant::setInteger8(const int8_t value) {
	setType(EVT_INTEGER8);
	this->value.integer = (int64_t)value;
	changed();
}

void TVariant::setInteger8(const std::string& value) {
	setInteger8(util::strToInt(value, 0, *locale));
}

void TVariant::setInteger8(const std::wstring& value) {
	setInteger8(util::strToInt(value, 0, *locale));
}


void TVariant::setInteger16(const int16_t value) {
	setType(EVT_INTEGER16);
	this->value.integer = (int64_t)value;
	changed();
}

void TVariant::setInteger16(const std::string& value) {
	setInteger16(util::strToInt(value, 0, *locale));
}

void TVariant::setInteger16(const std::wstring& value) {
	setInteger16(util::strToInt(value, 0, *locale));
}


void TVariant::setInteger32(const int32_t value) {
	setType(EVT_INTEGER32);
	this->value.integer = (int64_t)value;
	changed();
}

void TVariant::setInteger32(const std::string& value) {
	setInteger32(util::strToInt(value, 0, *locale));
}

void TVariant::setInteger32(const std::wstring& value) {
	setInteger32(util::strToInt(value, 0, *locale));
}


void TVariant::setInteger64(const int64_t value) {
	setType(EVT_INTEGER64);
	this->value.integer = value;
	changed();
}

void TVariant::setInteger64(const std::string& value) {
	setInteger64(util::strToInt64(value, 0, *locale));
}

void TVariant::setInteger64(const std::wstring& value) {
	setInteger64(util::strToInt64(value, 0, *locale));
}


void TVariant::setUnsigned8(const uint8_t value) {
	setType(EVT_UNSIGNED8);
	this->value.uinteger = (uint64_t)value;
	changed();
}

void TVariant::setUnsigned8(const std::string& value) {
	setUnsigned32(util::strToUnsigned(value, 0, *locale));
}

void TVariant::setUnsigned8(const std::wstring& value) {
	setUnsigned32(util::strToUnsigned(value, 0, *locale));
}


void TVariant::setUnsigned16(const uint16_t value) {
	setType(EVT_UNSIGNED16);
	this->value.uinteger = (uint64_t)value;
	changed();
}

void TVariant::setUnsigned16(const std::string& value) {
	setUnsigned16(util::strToUnsigned(value, 0, *locale));
}

void TVariant::setUnsigned16(const std::wstring& value) {
	setUnsigned16(util::strToUnsigned(value, 0, *locale));
}


void TVariant::setUnsigned32(const uint32_t value) {
	setType(EVT_UNSIGNED32);
	this->value.uinteger = (uint64_t)value;
	changed();
}

void TVariant::setUnsigned32(const std::string& value) {
	setUnsigned32(util::strToUnsigned(value, 0, *locale));
}

void TVariant::setUnsigned32(const std::wstring& value) {
	setUnsigned32(util::strToUnsigned(value, 0, *locale));
}


void TVariant::setUnsigned64(const uint64_t value) {
	setType(EVT_UNSIGNED64);
	this->value.uinteger = value;
	changed();
}

void TVariant::setUnsigned64(const std::string& value) {
	setUnsigned64(util::strToUnsigned(value, 0, *locale));
}

void TVariant::setUnsigned64(const std::wstring& value) {
	setUnsigned64(util::strToUnsigned(value, 0, *locale));
}

void TVariant::setDateTime(const TDateTime& value) {
	setType(EVT_TIME);
	this->value.time = value;
	changed();
}

void TVariant::setDateTime(const TTimePart value) {
	setType(EVT_TIME);
	this->value.time = value;
	changed();
}

void TVariant::setDateTime(const TTimeNumeric value) {
	setType(EVT_TIME);
	this->value.time = value;
	changed();
}

void TVariant::setDateTime(const std::string& value) {
	setType(EVT_TIME);
	this->value.time = value;
	changed();
}

void TVariant::setDateTime(const std::wstring& value) {
	throw util::app_error(" TVariant::setDateTime(const std::wstring& value) not yet implemented");
}

void TVariant::setDateTime(const TTimePart seconds, const TTimePart microseconds) {
	setType(EVT_TIME);
	this->value.time.setTime(seconds, microseconds);
	changed();
}

void TVariant::setJulian(const TTimePart seconds, const TTimePart microseconds) {
	setType(EVT_TIME);
	this->value.time.setJulian(seconds, microseconds);
	changed();
}

void TVariant::setJulian(const TTimeNumeric value) {
	setType(EVT_TIME);
	this->value.time.setJulian(value);
	changed();
}

void TVariant::setJ2K(const TTimeNumeric value) {
	setType(EVT_TIME);
	this->value.time.setJ2K(value);
	changed();
}

void TVariant::setJ2K(const TTimePart seconds, const TTimePart microseconds) {
	setType(EVT_TIME);
	this->value.time.setJ2K(seconds, microseconds);
	changed();
}

void TVariant::setDouble(const double value) {
	setType(EVT_DOUBLE);
	this->value.decimal = value;
	changed();
}

void TVariant::setDouble(const float value) {
	setType(EVT_DOUBLE);
	this->value.decimal = value;
	changed();
}

void TVariant::setDouble(const std::string& value) {
	setDouble(util::strToDouble(value, 0.0, *locale));
}

void TVariant::setDouble(const std::wstring& value) {
	setDouble(util::strToDouble(value, 0.0, *locale));
}

void TVariant::setBoolean(const bool value) {
	setType(EVT_BOOLEAN);
	this->value.boolean = value;
	changed();
}

void TVariant::setBoolean(const std::string& value) {
	EBooleanType bt;
	setBoolean(readBoolValueAndTypeA(value, bt));
}

void TVariant::setBoolean(const std::wstring& value) {
	EBooleanType bt;
	setBoolean(readBoolValueAndTypeW(value, bt));
}

void TVariant::setString(const char *value) {
	setType(EVT_STRING);
	this->value.astring = std::string(value);
	changed();
}

void TVariant::setString(const char *value, size_t size) {
	setType(EVT_STRING);
	this->value.astring = std::string(value, size);
	changed();
}

void TVariant::setString(const std::string &value) {
	setType(EVT_STRING);
	this->value.astring = std::string(value);
	changed();
}

void TVariant::setWideString(const wchar_t *value) {
	setType(EVT_WIDE_STRING);
	this->value.wstring = std::wstring(value);
	changed();
}

void TVariant::setWideString(const wchar_t *value, size_t size) {
	setType(EVT_WIDE_STRING);
	this->value.wstring = std::wstring(value, size);
	changed();
}

void TVariant::setWideString(const std::wstring &value) {
	setType(EVT_WIDE_STRING);
	this->value.wstring = std::wstring(value);
	changed();
}

void TVariant::setBlob(const util::TBlob &value) {
	setType(EVT_BLOB);
	this->value.blob = value;
	changed();
}

void TVariant::setBlob(const void *const value, const size_t size) {
	setType(EVT_BLOB);
	this->value.blob.assign(value, size);
	changed();
}

void TVariant::setVariant(const TVariant &value) {
	*this = value;
	changed();
}

void TVariant::setValue(const std::string& value, EVariantType type) {
	switch (type) {
		case EVT_INTEGER8:
			setInteger8(value);
			break;
		case EVT_INTEGER16:
			setInteger16(value);
			break;
		case EVT_INTEGER32:
			setInteger32(value);
			break;
		case EVT_INTEGER64:
			setInteger64(value);
			break;
		case EVT_UNSIGNED8:
			setUnsigned8(value);
			break;
		case EVT_UNSIGNED16:
			setUnsigned16(value);
			break;
		case EVT_UNSIGNED32:
			setUnsigned32(value);
			break;
		case EVT_UNSIGNED64:
			setUnsigned64(value);
			break;
		case EVT_BOOLEAN:
			setBoolean(value);
			break;
		case EVT_DOUBLE:
			setDouble(value);
			break;
		case EVT_TIME:
			setDateTime(value);
			break;
		case EVT_STRING:
		default:
			// Treat value as native string
			setString(value);
			break;
	}
}

void TVariant::setValue(const std::wstring& value, EVariantType type) {
	switch (type) {
		case EVT_INTEGER8:
			setInteger8(value);
			break;
		case EVT_INTEGER16:
			setInteger16(value);
			break;
		case EVT_INTEGER32:
			setInteger32(value);
			break;
		case EVT_INTEGER64:
			setInteger64(value);
			break;
		case EVT_UNSIGNED8:
			setUnsigned8(value);
			break;
		case EVT_UNSIGNED16:
			setUnsigned16(value);
			break;
		case EVT_UNSIGNED32:
			setUnsigned32(value);
			break;
		case EVT_UNSIGNED64:
			setUnsigned64(value);
			break;
		case EVT_BOOLEAN:
			setBoolean(value);
			break;
		case EVT_DOUBLE:
			setDouble(value);
			break;
		case EVT_TIME:
			setDateTime(value);
			break;
		case EVT_STRING:
		default:
			// Treat value as wide string
			setWideString(value);
			break;
	}
}


TVariant &TVariant::operator=(const int8_t value) {
	setType(EVT_INTEGER8);
	this->value.integer = (int64_t)value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const uint8_t value) {
	setType(EVT_UNSIGNED8);
	this->value.uinteger = (uint64_t)value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const int16_t value) {
	setType(EVT_INTEGER16);
	this->value.integer = (int64_t)value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const uint16_t value) {
	setType(EVT_UNSIGNED16);
	this->value.uinteger = (uint64_t)value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const int32_t value) {
	setType(EVT_INTEGER32);
	this->value.integer = (int64_t)value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const uint32_t value) {
	setType(EVT_UNSIGNED32);
	this->value.uinteger = (uint64_t)value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const int64_t value) {
	setType(EVT_INTEGER64);
	this->value.integer = value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const uint64_t value) {
	setType(EVT_UNSIGNED64);
	this->value.uinteger = value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const TDateTime& value) {
	setType(EVT_TIME);
	this->value.time = value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const double value) {
	setType(EVT_DOUBLE);
	this->value.decimal = value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const float value) {
	setType(EVT_DOUBLE);
	this->value.decimal = value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const bool value) {
	setType(EVT_BOOLEAN);
	this->value.boolean = value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const char* value) {
	setType(EVT_STRING);
	if (util::assigned(value))
		this->value.astring = std::string(value);
	else
		this->value.astring.clear();
	changed();
	return *this;
}

TVariant &TVariant::operator=(const std::string& value) {
	setType(EVT_STRING);
	this->value.astring = value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const wchar_t* value) {
	setType(EVT_WIDE_STRING);
	if (util::assigned(value))
		this->value.wstring = std::wstring(value);
	else
		this->value.wstring.clear();
	changed();
	return *this;
}

TVariant &TVariant::operator=(const std::wstring& value) {
	setType(EVT_WIDE_STRING);
	this->value.wstring = value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const util::TBlob& value) {
	setType(EVT_BLOB);
	this->value.blob = value;
	changed();
	return *this;
}

TVariant &TVariant::operator=(const TVariant& value) {
	clear();
	switch (value.varType) {
		case EVT_INTEGER8:
			*this = value.asInteger8();
			break;
		case EVT_INTEGER16:
			*this = value.asInteger16();
			break;
		case EVT_INTEGER32:
			*this = value.asInteger32();
			break;
		case EVT_INTEGER64:
			*this = value.asInteger64();
			break;
		case EVT_UNSIGNED8:
			*this = value.asUnsigned8();
			break;
		case EVT_UNSIGNED16:
			*this = value.asUnsigned16();
			break;
		case EVT_UNSIGNED32:
			*this = value.asUnsigned32();
			break;
		case EVT_UNSIGNED64:
			*this = value.asUnsigned64();
			break;
		case EVT_BOOLEAN:
			*this = value.asBoolean();
			break;
		case EVT_STRING:
			*this = value.asString();
			break;
		case EVT_WIDE_STRING:
			*this = value.asWideString();
			break;
		case EVT_DOUBLE:
			*this = value.asDouble();
			break;
		case EVT_TIME:
			*this = value.asTime();
			break;
		case EVT_BLOB:
			*this = value.asBlob();
			break;
		case EVT_NULL:
			setType(EVT_NULL);
			break;
		default:
			break;
	}
	changed();
	return *this;
}


char* TVariant::getChar(const char** data, size_t& size) {
	*data = nil;
	char* p;
	size = 0;
	if (varType == EVT_STRING) {
		size = value.astring.size();
		p = size > 0 ? const_cast<char*>(value.astring.c_str()) : nil;
	} else {
		p = nil;
	}
	if (util::assigned(data))
		*data = p;
	return p;
}

wchar_t* TVariant::getWideChar(const wchar_t** data, size_t& size) {
	*data = nil;
	wchar_t* p;
	size = 0;
	if (varType == EVT_WIDE_STRING) {
		size = value.wstring.size();
		p = size > 0 ? const_cast<wchar_t*>(value.wstring.c_str()) : nil;
	} else {
		p = nil;
	}
	if (util::assigned(data))
		*data = p;
	return p;
}

TDateTime& TVariant::getTime() {
	setType(EVT_TIME);
	return value.time;
}

TBlob& TVariant::getBlob(const char** data, size_t& size) {
	setType(EVT_BLOB);
	*data = value.blob.data();
	size = value.blob.size();
	return value.blob;
}

TBlob& TVariant::getBlob() {
	setType(EVT_BLOB);
	return value.blob;
}


size_t TVariant::size() const {
	switch (varType) {
		case EVT_INTEGER8:
			return sizeof(int8_t);
		case EVT_INTEGER16:
			return sizeof(int16_t);
		case EVT_INTEGER32:
			return sizeof(int32_t);
		case EVT_INTEGER64:
			return sizeof(int64_t);
		case EVT_UNSIGNED8:
			return sizeof(uint8_t);
		case EVT_UNSIGNED16:
			return sizeof(uint16_t);
		case EVT_UNSIGNED32:
			return sizeof(uint32_t);
		case EVT_UNSIGNED64:
			return sizeof(uint64_t);
		case EVT_BOOLEAN:
			return sizeof(bool);
		case EVT_STRING:
			return value.astring.size() * sizeof(char);
		case EVT_WIDE_STRING:
			return value.wstring.size() * sizeof(wchar_t);
		case EVT_TIME:
			return sizeof(TTimePart);
		case EVT_DOUBLE:
			return sizeof(double);
		case EVT_BLOB:
			return value.blob.size();
		case EVT_NULL:
		default:
			break;
	}
	return (size_t)0;
}


int8_t TVariant::asInteger8(int8_t defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			if ((value.integer <= (int64_t)TLimits::LIMIT_INT8_MAX) &&
				(value.integer >= (int64_t)TLimits::LIMIT_INT8_MIN))
				return (int32_t)value.integer;
			break;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			if (value.uinteger <= (uint64_t)TLimits::LIMIT_INT8_MAX)
				return (int32_t)value.uinteger;
			break;
		case EVT_BOOLEAN:
			return (int32_t)value.boolean;
		case EVT_STRING:
			return util::strToInt(value.astring, defValue, *locale);
		case EVT_WIDE_STRING:
			return util::strToInt(value.wstring, defValue, *locale);
		case EVT_DOUBLE:
			if ((value.decimal <= (double)TLimits::LIMIT_INT8_MAX) &&
				(value.decimal >= (double)TLimits::LIMIT_INT8_MIN))
				return static_cast<int8_t>(value.decimal);
			break;	
		case EVT_TIME:
			if ((value.time.time() <= (int64_t)TLimits::LIMIT_INT8_MAX) &&
				(value.time.time() >= (int64_t)TLimits::LIMIT_INT8_MIN))
				return static_cast<int8_t>(value.time.time());
			break;
		case EVT_NULL:
			return INT8_C(0);
		default:
			break;
	}
	return defValue;
}


int16_t TVariant::asInteger16(int16_t defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			if ((value.integer <= (int64_t)TLimits::LIMIT_INT16_MAX) &&
				(value.integer >= (int64_t)TLimits::LIMIT_INT16_MIN))
				return (int32_t)value.integer;
			break;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			if (value.uinteger <= (uint64_t)TLimits::LIMIT_INT16_MAX)
				return (int32_t)value.uinteger;
			break;
		case EVT_BOOLEAN:
			return (int32_t)value.boolean;
		case EVT_STRING:
			return util::strToInt(value.astring, defValue, *locale);
		case EVT_WIDE_STRING:
			return util::strToInt(value.wstring, defValue, *locale);
		case EVT_DOUBLE:
			if ((value.decimal <= (double)TLimits::LIMIT_INT16_MAX) &&
				(value.decimal >= (double)TLimits::LIMIT_INT16_MIN))
				return static_cast<int16_t>(value.decimal);
			break;
		case EVT_TIME:
			if ((value.time.time() <= (int64_t)TLimits::LIMIT_INT16_MAX) &&
				(value.time.time() >= (int64_t)TLimits::LIMIT_INT16_MIN))
				return static_cast<int16_t>(value.time.time());
			break;
		case EVT_NULL:
			return INT16_C(0);
		default:
			break;
	}
	return defValue;
}


int32_t TVariant::asInteger32(int32_t defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			if ((value.integer <= (int64_t)TLimits::LIMIT_INT32_MAX) &&
				(value.integer >= (int64_t)TLimits::LIMIT_INT32_MIN))
				return (int32_t)value.integer;
			break;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			if (value.uinteger <= (uint64_t)TLimits::LIMIT_INT32_MAX)
				return (int32_t)value.uinteger;
			break;
		case EVT_BOOLEAN:
			return (int32_t)value.boolean;
		case EVT_STRING:
			return util::strToInt(value.astring, defValue, *locale);
		case EVT_WIDE_STRING:
			return util::strToInt(value.wstring, defValue, *locale);
		case EVT_DOUBLE:
			if ((value.decimal <= (double)TLimits::LIMIT_INT32_MAX) &&
				(value.decimal >= (double)TLimits::LIMIT_INT32_MIN))
				return static_cast<int32_t>(value.decimal);
			break;
		case EVT_TIME:
			if ((value.time.time() <= (int64_t)TLimits::LIMIT_INT32_MAX) &&
				(value.time.time() >= (int64_t)TLimits::LIMIT_INT32_MIN))
				return static_cast<int32_t>(value.time.time());
			break;
		case EVT_NULL:
			return INT32_C(0);
		default:
			break;
	}
	return defValue;
}


int64_t TVariant::asInteger64(int64_t defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			return value.integer;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			if (value.uinteger <= (uint64_t)TLimits::LIMIT_INT64_MAX)
				return (int64_t)value.uinteger;
			break;
		case EVT_BOOLEAN:
			return (int64_t)value.boolean;
		case EVT_STRING:
			return util::strToInt64(value.astring, defValue, *locale);
		case EVT_WIDE_STRING:
			return util::strToInt64(value.wstring, defValue, *locale);
		case EVT_DOUBLE:
			if ((value.decimal <= (double)TLimits::LIMIT_INT64_MAX) &&
				(value.decimal >= (double)TLimits::LIMIT_INT64_MIN))
				return static_cast<int64_t>(value.decimal);
			break;	
		case EVT_TIME:
			return (int64_t)value.time.time();
		case EVT_NULL:
			return INT64_C(0);
		default:
			break;
	}
	return defValue;
}


uint8_t TVariant::asUnsigned8(uint8_t defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			if ((value.integer <= (int64_t)TLimits::LIMIT_UINT8_MAX) &&
				(value.integer >= INT64_C(0)))
				return (uint8_t)value.integer;
			break;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			if ((value.uinteger <= (uint64_t)TLimits::LIMIT_UINT8_MAX) &&
				(value.uinteger >= (uint64_t)TLimits::LIMIT_UINT8_MIN))
				return (uint8_t)value.uinteger;
			break;
		case EVT_BOOLEAN:
			return (uint8_t)value.boolean;
		case EVT_STRING:
			return util::strToUnsigned(value.astring, defValue, *locale);
		case EVT_WIDE_STRING:
			return util::strToUnsigned(value.wstring, defValue, *locale);
		case EVT_DOUBLE:
			if ((value.decimal <= (double)TLimits::LIMIT_UINT8_MAX) &&
				(value.decimal >= 0.0l))
				return static_cast<uint8_t>(value.decimal);
			break;
		case EVT_TIME:
			if ((value.time.time() <= (TTimePart)TLimits::LIMIT_UINT8_MAX) &&
				(value.time.time() >= util::epoch()))
				return (uint8_t)value.time.time();
			break;
		case EVT_NULL:
			return UINT8_C(0);
		default:
			break;
	}
	return defValue;
}


uint16_t TVariant::asUnsigned16(uint16_t defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			if ((value.integer <= (int64_t)TLimits::LIMIT_UINT16_MAX) &&
				(value.integer >= INT64_C(0)))
				return (uint16_t)value.integer;
			break;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			if ((value.uinteger <= (uint64_t)TLimits::LIMIT_UINT16_MAX) &&
				(value.uinteger >= (uint64_t)TLimits::LIMIT_UINT16_MIN))
				return (uint16_t)value.uinteger;
			break;
		case EVT_BOOLEAN:
			return (uint16_t)value.boolean;
		case EVT_STRING:
			return util::strToUnsigned(value.astring, defValue, *locale);
		case EVT_WIDE_STRING:
			return util::strToUnsigned(value.wstring, defValue, *locale);
		case EVT_DOUBLE:
			if ((value.decimal <= (double)TLimits::LIMIT_UINT16_MAX) &&
				(value.decimal >= 0.0l))
				return static_cast<uint16_t>(value.decimal);
			break;
		case EVT_TIME:
			if ((value.time.time() <= (TTimePart)TLimits::LIMIT_UINT16_MAX) &&
				(value.time.time() >= util::epoch()))
				return (uint16_t)value.time.time();
			break;
		case EVT_NULL:
			return UINT16_C(0);
		default:
			break;
	}
	return defValue;
}


uint32_t TVariant::asUnsigned32(uint32_t defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			if ((value.integer <= (int64_t)TLimits::LIMIT_UINT32_MAX) &&
				(value.integer >= INT64_C(0)))
				return (uint32_t)value.integer;
			break;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			if ((value.uinteger <= (uint64_t)TLimits::LIMIT_UINT32_MAX) &&
				(value.uinteger >= (uint64_t)TLimits::LIMIT_UINT32_MIN))
				return (uint32_t)value.uinteger;
			break;
		case EVT_BOOLEAN:
			return (uint32_t)value.boolean;
		case EVT_STRING:
			return util::strToUnsigned(value.astring, defValue, *locale);
		case EVT_WIDE_STRING:
			return util::strToUnsigned(value.wstring, defValue, *locale);
		case EVT_DOUBLE:
			if ((value.decimal <= (double)TLimits::LIMIT_UINT32_MAX) &&
				(value.decimal >= 0.0l))
				return static_cast<uint32_t>(value.decimal);
			break;	
		case EVT_TIME:
			if ((value.time.time() <= (TTimePart)TLimits::LIMIT_UINT32_MAX) &&
				(value.time.time() >= util::epoch()))
				return (uint32_t)value.time.time();
			break;
		case EVT_NULL:
			return UINT32_C(0);
		default:
			break;
	}
	return defValue;
}


uint64_t TVariant::asUnsigned64(uint64_t defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			if (value.integer >= INT64_C(0))
				return (uint64_t)value.integer;
			break;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			return value.uinteger;
		case EVT_BOOLEAN:
			return (uint64_t)value.boolean;
		case EVT_STRING:
			return util::strToUnsigned64(value.astring, defValue, *locale);
		case EVT_WIDE_STRING:
			return util::strToUnsigned64(value.wstring, defValue, *locale);
		case EVT_DOUBLE:
			if ((value.decimal <= (double)TLimits::LIMIT_UINT64_MAX) &&
				(value.decimal >= 0.0))
				return static_cast<uint64_t>(value.decimal);
			break;	
		case EVT_TIME:
			if (value.time.time() >= util::epoch())
				return (uint64_t)value.time.time();
			break;
		case EVT_NULL:
			return UINT64_C(0);
		default:
			break;
	}
	return defValue;
}


const TDateTime& TVariant::asTime(const EDateTimeZone tz, TTimePart defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			time.setTime((TTimePart)value.integer, 0, tz);
			return time;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			if (value.uinteger <= (uint64_t)TLimits::LIMIT_INT64_MAX) {
				time.setTime((TTimePart)value.uinteger, 0, tz);
				return time;
			}
			break;
		case EVT_BOOLEAN:
			if (value.boolean) {
				time.now(tz);
				return time;
			}
			break;
		case EVT_STRING:
			time = value.astring;
			return time;
		case EVT_WIDE_STRING:
			// TODO
			// time = value.wstring;
			// return value.time;
			break;
		case EVT_DOUBLE:
			// Treat double as numerical date time representation
			if ((value.decimal <= (double)TLimits::LIMIT_INT64_MAX) &&
				(value.decimal >= (double)TLimits::LIMIT_INT64_MIN)) {
				time.setTime(value.decimal, tz);
				return time;
			}
			break;
		case EVT_TIME:
			// Return variant time object!
			return value.time;
			break;
		case EVT_NULL:
			break;
		default:
			break;
	}
	time.setTime(defValue);
	return time;
}


double TVariant::asDouble(double defValue, const app::TLocale& locale) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			return static_cast<double>(value.integer);
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			return static_cast<double>(value.uinteger);
		case EVT_BOOLEAN:
			return static_cast<double>(value.boolean);
		case EVT_STRING:
			if (syslocale != locale)
				return util::strToDouble(value.astring, defValue, locale);
			else
				return util::strToDouble(value.astring, defValue, *this->locale);
		case EVT_WIDE_STRING:
			if (syslocale != locale)
				return util::strToDouble(value.wstring, defValue, locale);
			else
				return util::strToDouble(value.wstring, defValue, *this->locale);
		case EVT_DOUBLE:
			return value.decimal;
		case EVT_TIME:
			return value.time.asNumeric();
		case EVT_NULL:
			return 0.0;
		default:
			break;
	}
	return defValue;
}


const char* TVariant::c_str() const {
	// Store permanent value in local string!
	cstr = asString();
	return cstr.c_str();
}


std::string TVariant::asString(const std::string& defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			return printf(PRI64_FMT_A, value.integer);
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			return printf(PRIu64_FMT_A, value.uinteger);
		case EVT_BOOLEAN:
			return writeBoolValueForTypeA(value.boolean, boolType);
		case EVT_STRING:
			return value.astring;
		case EVT_WIDE_STRING:
			if (!value.wstring.empty())
				return TStringConvert::WideToMultiByteString(value.wstring);
			break;
		case EVT_DOUBLE:
			return printf(formatA, value.decimal);
		case EVT_TIME:
			return value.time.asString();
			break;
		case EVT_BLOB:
			if (!value.blob.empty())
				return "0x" + util::TBinaryConvert::binToHex(value.blob.data(), value.blob.size());
				//return util::TBase64::encode(value.blob.data(), value.blob.size());
			break;
		case EVT_NULL:
			return JSON_NULL;
		default:
			break;
	}
	return defValue;
}


std::wstring TVariant::asWideString(const std::wstring& defValue) const {
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			return printf(PRI64_FMT_W, value.integer);
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			return printf(PRIu64_FMT_W, value.uinteger);
		case EVT_BOOLEAN:
			return writeBoolValueForTypeW(value.boolean, boolType);
		case EVT_STRING:
			if (!value.astring.empty())
				return TStringConvert::MultiByteToWideString(value.astring);
			break;
		case EVT_WIDE_STRING:
			return value.wstring;
		case EVT_DOUBLE:
			return printf(formatW, value.decimal);
		case EVT_TIME:
			return value.time.asWideString();
		case EVT_BLOB:
			if (!value.blob.empty())
				return TStringConvert::MultiByteToWideString(util::TBase64::encode(value.blob.data(), value.blob.size()));
			break;
		case EVT_NULL:
			return JSON_NULL_W;
		default:
			break;
	}
	return defValue;
}


std::string TVariant::asJSON(const std::string& preamble, const std::string& name) const {
	std::string vs;
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			vs = printf(app::en_US, PRI64_FMT_A, value.integer);
			break;
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			vs = printf(app::en_US, PRIu64_FMT_A, value.uinteger);
			break;
		case EVT_BOOLEAN:
			vs = writeBoolValueForTypeA(value.boolean, VBT_BLTRUE);
			break;
		case EVT_STRING:
			vs = value.astring;
			break;
		case EVT_WIDE_STRING:
			if (!value.wstring.empty())
				vs = TStringConvert::WideToMultiByteString(value.wstring);
			break;
		case EVT_DOUBLE:
			vs = printf(app::en_US, formatA, value.decimal);
			break;
		case EVT_TIME:
			vs = value.time.asString();
			break;
		case EVT_BLOB:
			if (!value.blob.empty())
				vs = util::TBase64::encode(value.blob.data(), value.blob.size());
			break;
		case EVT_NULL:
			vs = JSON_NULL;
			break;
		default:
			break;
	}

	// Set JSON properties
	json.update(name, vs, varType);

	// Return JSON value
	return json.asJSON(preamble);
}


const util::TBlob& TVariant::asBlob() const {
	TTimePart t;
	int8_t c;
	uint8_t uc;
	int16_t s;
	uint16_t us;
	int32_t i;
	uint32_t ui;
	int64_t li;
	uint64_t lui;
	bool b;
	double d;

	// Clear local blob before loading with variant data
	if (varType != EVT_BLOB) {
		blob.clear();
	}

	switch (varType) {
		case EVT_BLOB:
			// Return variant blob object!
			return value.blob;
		case EVT_INTEGER8:
			c = (int8_t)value.integer;
			blob.assign(&c, sizeof(int8_t));
			return blob;
		case EVT_INTEGER16:
			s = (int16_t)value.integer;
			blob.assign(&s, sizeof(int16_t));
			return blob;
		case EVT_INTEGER32:
			i = (int32_t)value.integer;
			blob.assign(&i, sizeof(int32_t));
			return blob;
		case EVT_INTEGER64:
			li = value.integer;
			blob.assign(&li, sizeof(int64_t));
			return blob;
		case EVT_UNSIGNED8:
			uc = (uint8_t)value.uinteger;
			blob.assign(&uc, sizeof(uint8_t));
			return blob;
		case EVT_UNSIGNED16:
			us = (uint16_t)value.uinteger;
			blob.assign(&us, sizeof(uint16_t));
			return blob;
		case EVT_UNSIGNED32:
			ui = (uint32_t)value.uinteger;
			blob.assign(&ui, sizeof(uint32_t));
			return blob;
		case EVT_UNSIGNED64:
			lui = value.uinteger;
			blob.assign(&lui, sizeof(uint64_t));
			return blob;
		case EVT_BOOLEAN:
			b = value.boolean;
			blob.assign(&b, sizeof(bool));
			return blob;
		case EVT_STRING:
			blob.assign(value.astring.c_str(), value.astring.size());
			return blob;
		case EVT_WIDE_STRING:
			blob.assign(value.wstring.c_str(), value.wstring.size() * sizeof(wchar_t));
			return blob;
		case EVT_TIME:
			t = value.time.time();
			blob.assign(&t, sizeof(TTimePart));
			return blob;
		case EVT_DOUBLE:
			d = value.decimal;
			blob.assign(&d, sizeof(double));
			return blob;
		case EVT_NULL:
		default:
			break;
	}

	// Return local blob by default
	return blob;
}


bool TVariant::asBoolean(const bool defValue) const {
	EBooleanType b = boolType;
	switch (varType) {
		case EVT_INTEGER8:
		case EVT_INTEGER16:
		case EVT_INTEGER32:
		case EVT_INTEGER64:
			return (value.integer > INT64_C(0));
		case EVT_UNSIGNED8:
		case EVT_UNSIGNED16:
		case EVT_UNSIGNED32:
		case EVT_UNSIGNED64:
			return (value.uinteger > UINT64_C(0));
		case EVT_BOOLEAN:
			return value.boolean;
		case EVT_STRING:
			return readBoolValueAndTypeA(value.astring, b);
		case EVT_WIDE_STRING:
			return readBoolValueAndTypeW(value.wstring, b);
		case EVT_DOUBLE:
			return (value.decimal > 0.0l);
		case EVT_TIME:
			return (value.time.time() != util::epoch());
		case EVT_NULL:
			return false;
		default:
			break;
	}
	return defValue;
}


bool TVariant::readBoolValueAndTypeA(const std::string& value, EBooleanType& type) const {
	bool retVal;
	std::string s(util::trim(value));
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	do {
		retVal = true;
		if (value == JSON_TRUE_A) {
			type = VBT_BLTRUE;
			continue;
		}
		if (value == "yes") {
			type = VBT_BLYES;
			continue;
		}
		if (value == "ja") {
			type = VBT_BLGER;
			continue;
		}
		if (value == "on") {
			type = VBT_BLON;
			continue;
		}
		if (value == "1") {
			type = VBT_BL01;
			continue;
		}
		retVal = false;
		if (value == JSON_FALSE_A) {
			type = VBT_BLTRUE;
			continue;
		}
		if (value == "no") {
			type = VBT_BLYES;
			continue;
		}
		if (value == "nein") {
			type = VBT_BLGER;
			continue;
		}
		if (value == "off") {
			type = VBT_BLON;
			continue;
		}
		if (value == "0") {
			type = VBT_BL01;
			continue;
		}
	} while (false);
	return retVal;
}


bool TVariant::readBoolValueAndTypeW(const std::wstring& value, EBooleanType& type) const {
	bool retVal;
	std::wstring s(util::trim(value));
	std::transform(s.begin(), s.end(), s.begin(), ::towlower);
	do {
		retVal = true;
		if (value == JSON_TRUE_W) {
			type = VBT_BLTRUE;
			continue;
		}
		if (value == L"yes") {
			type = VBT_BLYES;
			continue;
		}
		if (value == L"ja") {
			type = VBT_BLGER;
			continue;
		}
		if (value == L"on") {
			type = VBT_BLON;
			continue;
		}
		if (value == L"1") {
			type = VBT_BL01;
			continue;
		}
		retVal = false;
		if (value == JSON_FALSE_W) {
			type = VBT_BLTRUE;
			continue;
		}
		if (value == L"no") {
			type = VBT_BLYES;
			continue;
		}
		if (value == L"nein") {
			type = VBT_BLGER;
			continue;
		}
		if (value == L"off") {
			type = VBT_BLON;
			continue;
		}
		if (value == L"0") {
			type = VBT_BL01;
			continue;
		}
	} while (false);
	return retVal;
}


std::string TVariant::writeBoolValueForTypeA(const bool value, const EBooleanType type) const {
	switch (type) {
		case VBT_BLYES:
			if (value) return "yes";
			else return "no";
			break;
		case VBT_BLGER:
			if (value) return "ja";
			else return "nein";
			break;
		case VBT_BLON:
			if (value) return "on";
			else return "off";
			break;
		case VBT_BL01:
			if (value) return "1";
			else return "0";
			break;
		case VBT_LOCALE:
			if (util::assigned(locale)) {
				if (value) return locale->getBooleanTrueName();
				else return locale->getBooleanFalseName();
				break;
			}
		// NO BREAK here...
		case VBT_BLTRUE:
		default:
			if (value) return JSON_TRUE_A;
			else return JSON_FALSE_A;
			break;
	}
}


std::wstring TVariant::writeBoolValueForTypeW(const bool value, const EBooleanType type) const {
	switch (type) {
		case VBT_BLYES:
			if (value) return L"yes";
			else return L"no";
			break;
		case VBT_BLGER:
			if (value) return L"ja";
			else return L"nein";
			break;
		case VBT_BLON:
			if (value) return L"on";
			else return L"off";
			break;
		case VBT_BL01:
			if (value) return L"1";
			else return L"0";
			break;
		case VBT_LOCALE:
			if (util::assigned(locale)) {
				if (value) return locale->getBooleanTrueNameW();
				else return locale->getBooleanFalseNameW();
				break;
			}
		// NO BREAK here...
		case VBT_BLTRUE:
		default:
			if (value) return JSON_TRUE_W;
			else return JSON_FALSE_W;
			break;
	}
}

EVariantType TVariant::guessType(const std::string& value, const size_t depth, const bool debug) {
	bool OK = false;
	// bool d = debug;
	EVariantType retVal = EVT_INVALID;

	if (value.empty())
		return retVal;

	retVal = EVT_STRING;

	if (!OK && util::isInternationalTime(value)) {
		OK = true;
		retVal = EVT_TIME;
		// if (d) std::cout << "TVariant::guessType(EVT_TIME) for \"" << value << "\"" << std::endl;
	}

	if (!OK && util::isUniversalTime(value)) {
		OK = true;
		retVal = EVT_TIME;
		// if (d) std::cout << "TVariant::guessType(EVT_TIME_UTC) for \"" << value << "\"" << std::endl;
	}

	if (!OK && value.size() > 0) {

		bool uval = true;
		bool sval = true;
		bool numeric = true;
		bool alpha = true;

		for (size_t i=0; i<value.size() && i<depth; i++) {

			if (alpha) {
				if (!isAlphaNumericA(value[i])) {
					alpha = false;
					// if (d) std::cout << "TVariant::guessType(NO_STRING) for \"" << value << "\"" << std::endl;
				}
			}

			if (numeric) {
				if (!isNumericA(value[i])) {
					numeric = false;
					// if (d) std::cout << "TVariant::guessType(NO_NUMERIC) for \"" << value << "\"" << std::endl;
				}
			}

			if (sval) {
				if (!isIntegerA(value[i])) {
					sval = false;
					// if (d) std::cout << "TVariant::guessType(NO_INTEGER) for \"" << value << "\"" << std::endl;
				}
			}

			if (uval) {
				if (!isUnsignedA(value[i])) {
					uval = false;
					// if (d) std::cout << "TVariant::guessType(NO_UNSIGNED) for \"" << value << "\"" << std::endl;
				}
			}

		}

		// Sequence order matters here!
		if (!OK && uval) {
			retVal = EVT_UNSIGNED;
			OK = true;
			// if (d) std::cout << "TVariant::guessType(IS_UNSIGNED) for \"" << value << "\"" << std::endl;
		}

		if (!OK && sval) {
			retVal = EVT_INTEGER;
			OK = true;
			// if (d) std::cout << "TVariant::guessType(IS_INTEGER) for \"" << value << "\"" << std::endl;
		}

		if (!OK && numeric) {
			retVal = EVT_DOUBLE;
			OK = true;
			// if (d) std::cout << "TVariant::guessType(IS_NUMERIC) for \"" << value << "\"" << std::endl;
		}

		if (!OK && alpha) {
			retVal = EVT_STRING;
			OK = true;
			// if (d) std::cout << "TVariant::guessType(IS_STRING) for \"" << value << "\"" << std::endl;
		}

		if (retVal == EVT_UNSIGNED || retVal == EVT_INTEGER) {
			if (value.size() > 1) {
				// Treat values like '000013242' as string!
				if (value[0] == '0') {
					retVal = EVT_STRING;
					// if (d) std::cout << "TVariant::guessType(IS_ZERONUM_STRING) for \"" << value << "\"" << std::endl;
				}
			}
		}

	}

	return retVal;
}

std::string TVariant::getTypeAsString() const {
	switch (varType) {
	case EVT_BOOLEAN:
		return "Boolean Value (" + std::to_string((size_u)(8*sizeof(bool))) + " Bit)";
		case EVT_INTEGER8:
			return "Signed Integer (" + std::to_string((size_u)(8*sizeof(int8_t))) + " Bit)";
		case EVT_INTEGER16:
			return "Signed Integer (" + std::to_string((size_u)(8*sizeof(int16_t))) + " Bit)";
		case EVT_INTEGER32:
			return "Signed Integer (" + std::to_string((size_u)(8*sizeof(int32_t))) + " Bit)";
		case EVT_INTEGER64:
			return "Signed Integer (" + std::to_string((size_u)(8*sizeof(int64_t))) + " Bit)";
		case EVT_UNSIGNED8:
			return "Unsigned Integer (" + std::to_string((size_u)(8*sizeof(uint8_t))) + " Bit)";
		case EVT_UNSIGNED16:
			return "Unsigned Integer (" + std::to_string((size_u)(8*sizeof(uint16_t))) + " Bit)";
		case EVT_UNSIGNED32:
			return "Unsigned Integer (" + std::to_string((size_u)(8*sizeof(uint32_t))) + " Bit)";
		case EVT_UNSIGNED64:
			return "Unsigned Integer (" + std::to_string((size_u)(8*sizeof(uint64_t))) + " Bit)";
		case EVT_STRING:
			return "ASCII UTF-8 String (" + std::to_string((size_u)value.astring.size()) + " Bytes)";
		case EVT_WIDE_STRING:
			return "Wide UTF-16 String (" + std::to_string((size_u)(value.wstring.size() * sizeof(wchar_t))) + " Bytes)";
		case EVT_DOUBLE:
			return "Numeric Value (" + std::to_string((size_u)(8*sizeof(double))) + " Bit)";
		case EVT_TIME:
			return "Date time value";
		case EVT_BLOB:
			return "Binary large object (" + std::to_string((size_u)value.blob.size()) + " Bytes)";
		default:
			break;
	}
	return "<unassigned>";
}

void TVariant::setType(EVariantType type) {
	if ((!locked || varType == EVT_UNKNOWN) && varType != type) {
		clear();
		varType = type;
		svalue = util::isMemberOf(varType, EVT_INTEGER8,EVT_INTEGER16,EVT_INTEGER32,EVT_INTEGER64);
		uvalue = util::isMemberOf(varType, EVT_UNSIGNED8,EVT_UNSIGNED16,EVT_UNSIGNED32,EVT_UNSIGNED64);
	}
	//std::cout << app::yellow << "setType(" << type << ") <" << getTypeAsString() << ">" << app::reset << std::endl;
}

void TVariant::setPrecision(const TDoublePrecision precision) {
	if (precision > 0 && precision <= 12 && precision != this->precision) {
		this->precision = precision;
		formatA = "%." + std::to_string((size_u)precision) + "f";
		formatW = L"%." + std::to_wstring((size_u)precision) + L"f";
	}
};

void TVariant::setTimeFormat(const EDateTimeFormat type) {
	timeType = type;
	value.time.setFormat(timeType);
}

void TVariant::setTimePrecision(const util::EDateTimePrecision value) {
	timePrecision = value;
	this->value.time.setPrecision(timePrecision);
}

void TVariant::now() {
	setType(EVT_TIME);
	value.time = util::now();
	changed();
}

void TVariant::epoch() {
	setType(EVT_TIME);
	value.time = util::epoch();
	changed();
}



TVariantValues::TVariantValues() {
	prime();
}

TVariantValues::TVariantValues(const bool tolower) {
	prime();
	this->tolower = tolower;
}

TVariantValues::TVariantValues(const TVariantValues& values) {
	prime();
	*this = values;
	tolower = values.tolower;
}

TVariantValues::~TVariantValues() {
	clear();
}

void TVariantValues::prime() {
	tolower = true;
	invalidated = false;
	bindDataHandler(&TVariantValues::onJsonDataField, this);
}

void TVariantValues::clear() {
	if (!empty())
		invalidated = true;
	csv.clear();
	json.clear();
	TJsonParser::clear();
	util::clearObjectList(variants);
}

void TVariantValues::reset() {
	invalidated = false;
}

void TVariantValues::invalidate() {
	invalidated = true;
}

void TVariantValues::lock() {
	for (size_t idx=0; idx<size(); idx++) {
		const PNamedVariant o = variants[idx];
		if (util::assigned(o)) {
			o->value().lock();
		}
	}
}

void TVariantValues::unlock() {
	for (size_t idx=0; idx<size(); idx++) {
		const PNamedVariant o = variants[idx];
		if (util::assigned(o)) {
			o->value().unlock();
		}
	}
}

void TVariantValues::addEntry(const char* value) {
	// Create new entry and add in list
	if (util::assigned(value)) {
		PNamedVariant o = new TNamedVariant(std::to_string((size_s)util::succ(size())));
		*o = value;
		variants.push_back(o);
		invalidated = true;
	}
}

void TVariantValues::addEntry(const std::string& value) {
	// Create new entry and add in list
	if (!value.empty()) {
		PNamedVariant o = new TNamedVariant(std::to_string((size_s)util::succ(size())));
		*o = value;
		variants.push_back(o);
		invalidated = true;
	}
}

bool TVariantValues::validIndex(const size_t index) const {
	return (variants.size() >= 0 && index < variants.size());
}

bool TVariantValues::hasKey(const std::string& key) const {
	return app::nsizet != find(key);
}

bool TVariantValues::hasKey(const char* key) const {
	return app::nsizet != find(key);
}

size_t TVariantValues::find(const char* key, size_t size) const {
	PNamedVariant o;
	size_t i,n;
	n = variants.size();
	for (i=0; i<n; i++) {
		o = variants[i];
		if (util::assigned(o)) {
			if (tolower) {
				// Ignore case...
				if (size == o->name().size() && 0 == strncasecmp(o->name().c_str(), key, size)) {
					return i;
				}
			} else {
				if (size == o->name().size() && 0 == strncmp(o->name().c_str(), key, size)) {
					return i;
				}
			}
		}
	}
	return app::nsizet;
}

size_t TVariantValues::find(const std::string& key) const {
	if (!key.empty())
		return find(key.c_str(), key.size());
	return app::nsizet;
}

size_t TVariantValues::find(const char* key) const {
	if (util::assigned(key)) {
		size_t length = strnlen(key, 1024);
		if (length > 0) {
			return find(key, length);
		}
	}
	return app::nsizet;
}


const std::string& TVariantValues::name(const int index) const {
	if (index >= 0) {
		return name((size_t)index);
	}
	return defStr;
}

const std::string& TVariantValues::name(const size_t index) const {
	if (validIndex(index))
		return variants[index]->name();
	return defStr;
}


util::TVariant& TVariantValues::value(const int index) {
	if (index >= 0) {
		return value((size_t)index);
	}
	defVar.clear();
	return defVar;
}

util::TVariant& TVariantValues::value(const size_t index) {
	if (validIndex(index)) {
		return variants[index]->value();
	}
	defVar.clear();
	return defVar;
}

util::TVariant& TVariantValues::value(const std::string& key) {
	size_t idx = find(key);
	if (idx != app::nsizet)
		return variants[idx]->value();
	defVar.clear();
	return defVar;
}

util::TVariant& TVariantValues::value(const char* key) {
	size_t idx = find(key);
	if (idx != app::nsizet) {
		return variants[idx]->value();
	}
	defVar.clear();
	return defVar;
}


const util::TVariant& TVariantValues::value(const int index) const {
	if (index >= 0) {
		return value((size_t)index);
	}
	defVar.clear();
	return defVar;
}

const util::TVariant& TVariantValues::value(const size_t index) const {
	if (validIndex(index)) {
		return variants[index]->value();
	}
	defVar.clear();
	return defVar;
}

const util::TVariant& TVariantValues::value(const std::string& key) const {
	size_t idx = find(key);
	if (idx != app::nsizet)
		return variants[idx]->value();
	defVar.clear();
	return defVar;
}

const util::TVariant& TVariantValues::value(const char* key) const {
	size_t idx = find(key);
	if (idx != app::nsizet) {
		return variants[idx]->value();
	}
	defVar.clear();
	return defVar;
}


const util::TNamedVariant& TVariantValues::variant(const int index) const {
	if (index >= 0) {
		return variant((size_t)index);
	}
	defVal.clear();
	return defVal;
}

const TNamedVariant& TVariantValues::variant(const size_t index) const {
	if (validIndex(index)) {
		PNamedVariant o = variants[index];
		if (util::assigned(o))
			return *o;
	}
	defVal.clear();
	return defVal;
}

const TNamedVariant& TVariantValues::variant(const std::string& key) const {
	size_t idx = find(key);
	if (idx != app::nsizet)
		return *variants[idx];
	defVal.clear();
	return defVal;
}

const TNamedVariant& TVariantValues::variant(const char* key) const {
	size_t idx = find(key);
	if (idx != app::nsizet)
		return *variants[idx];
	defVal.clear();
	return defVal;
}



const PNamedVariant TVariantValues::getVariant(const size_t index) const {
	if (validIndex(index))
		return variants[index];
	return nil;
}

const PNamedVariant TVariantValues::getVariant(const std::string& key) const {
	size_t idx = find(key);
	if (idx != app::nsizet)
		return variants[idx];
	return nil;
}

const PNamedVariant TVariantValues::getVariant(const char* key) const {
	size_t idx = find(key);
	if (idx != app::nsizet)
		return variants[idx];
	return nil;
}


util::TVariant& TVariantValues::operator [] (const int index) {
	return value(index);
}

util::TVariant& TVariantValues::operator [] (const size_t index) {
	return value(index);
}

util::TVariant& TVariantValues::operator [] (const std::string& key) {
	return value(key);
}

util::TVariant& TVariantValues::operator [] (const char* key) {
	return value(key);
}


const util::TVariant& TVariantValues::operator [] (const int index) const {
	return value(index);
}

const util::TVariant& TVariantValues::operator [] (const size_t index) const {
	return value(index);
}

const util::TVariant& TVariantValues::operator [] (const std::string& key) const {
	return value(key);
}

const util::TVariant& TVariantValues::operator [] (const char* key) const {
	return value(key);
}


void TVariantValues::assign(util::TCharPointerArray& argv, const util::TStringList& header) const {
	size_t idx, idv;
	size_t n = header.size() + size();
	argv.resize(n+1, false);

	// Add header entries
	for (idx=0; idx<header.size(); idx++) {
		argv[idx] = header[idx].c_str();
	}

	// Add variant entries
	for (idx=header.size(), idv=0; idv<size(); idx++, idv++) {
		const PNamedVariant o = variants[idv];
		if (util::assigned(o)) {
			argv[idx] = o->value().c_str();
		} else {
			argv[idx] = (char*)NULL;
		}
	}

	// Set last entry to null!
	argv[n] = (char*)NULL;
}

void TVariantValues::debugOutput(const std::string& name, const std::string& preamble) const {
	if (!empty()) {
		for (size_t idx=0; idx<size(); idx++) {
			const PNamedVariant o = variants[idx];

			if (!name.empty())
				std::cout << app::white << preamble << util::succ(idx) << ". " << name << app::reset << std::endl;
			else
				std::cout << app::white << preamble << util::succ(idx) << ". Variant" << app::reset << std::endl;

			std:: string s = o->value().asString();
			std::cout << preamble << "  Name    : " << o->name() << std::endl;
			std::cout << preamble << "  Value   : " << app::blue << (s.empty() ? "<empty>" : s) << app::reset << std::endl;
			std::cout << preamble << "  Variant : " << o->value().getTypeAsString() << std::endl;
			std::cout << preamble << "  Size    : " << o->value().size() << " bytes" << std::endl;
			std::cout << preamble << "  Length  : " << o->length() << " bytes" << std::endl;

			std::cout << std::endl;
		}
	} else {
		if (!name.empty())
			std::cout << app::yellow << preamble << " Variant list <" << name << "> is empty" << app::reset << std::endl;
		else
			std::cout << app::yellow << preamble << " Variant list is empty" << app::reset << std::endl;
	}
}


const std::string& TVariantValues::asText(const char delimiter, const std::string separator) const {
	if (!empty()) {
		if (csv.empty()) {
			PNamedVariant o;
			std::string value;
			size_t last = util::pred(size());
			bool isWindows = delimiter == '\r';
			for (size_t idx=1; idx<size(); ++idx) {
				o = variants[idx];
				if (util::assigned(o)) {
					value = o->value().asString();
					if (!value.empty())
						value = separator + value;
					if (idx < last) {
						if (isWindows)
							csv += o->name() + value + "\r\n";
						else
							csv += o->name() + value + delimiter;
					} else {
						csv += o->name() + value;
					}
				}
			}
		}
	} else {
		csv.clear();
	}
	return csv;
}


const util::TJsonList& TVariantValues::asJSON(const std::string& name) const {
	if (json.empty())
		asJSON(json, name);
	return json;
}

void TVariantValues::asJSON(util::TJsonList& list, const std::string& name, const EJsonArrayType type) const {
	if (!empty()) {
		if (!list.empty())
			list.clear();

		bool namedTable = !name.empty();

		// Begin new JSON object
		std::string preamble;
		if (EJT_OBJECT == type) {
			preamble = "  ";
			list.add("{");
		} else {
			preamble = "";
		}

		// Begin new JSON array
		if (namedTable)
			list.add(preamble + "\"" + name + "\": [");
		else
			list.add(preamble + "[");

		PNamedVariant o;
		size_t last = util::pred(size());
		for (size_t idx=0; idx<size(); idx++) {
			o = variants[idx];
			if (util::assigned(o)) {
				if (idx < last)
					list.add(o->value().asJSON("  ", o->name()) + ",");
				else
					list.add(o->value().asJSON("  ", o->name()));
			}
		}

		// Close JSON array and object
		list.add(preamble + "]");
		if (EJT_OBJECT == type)
			list.add("}");
	}
}

const util::TJsonList& TVariantValues::asJSON() const {
	if (json.empty())
		asJSON(json);
	return json;
}

void TVariantValues::asJSON(util::TJsonList& list) const {
	if (!list.empty())
		list.clear();

	if (!empty()) {
		// Begin new JSON object
		list.add("{");

		PNamedVariant o;
		size_t last = util::pred(size());
		for (size_t idx=0; idx<size(); idx++) {
			o = variants[idx];
			if (util::assigned(o)) {
				if (idx < last)
					list.add(o->value().asJSON("  ", o->name()) + ",");
				else
					list.add(o->value().asJSON("  ", o->name()));
			}
		}

		// Close JSON object
		list.add("}");
	}
}


const util::TStringList& TVariantValues::asHTML() const {
	if (html.empty())
		asHTML(html);
	return html;
}

void TVariantValues::asHTML(util::TStringList& list) const {
	if (!list.empty())
		list.clear();

	// Begin new HTML table
	list.add("<table class=\"table table-bordered\">");
	list.add("  <thead>");
	list.add("    <tr>");
	list.add("      <th>Name</th>");
	list.add("      <th>Value</th>");
	list.add("    </tr>");
	list.add("  </thead>");
	list.add("  <tbody>");

	// Add rows
	if (!empty()) {
		PNamedVariant o;
		for (size_t idx=0; idx<size(); idx++) {
			o = variants[idx];
			if (util::assigned(o)) {
				std::string value = util::replace(o->value().asString(), ";", "; ");
				list.add("    <tr>");
				list.add("      <td align=\"left\">");
				list.add("        <div style=\"word-wrap: break-all; overflow-wrap: break-all;\">" + html::THTML::applyFlowControl(o->name()) + "</div>");
				list.add("      </td>");
				list.add("      <td align=\"left\">");
				list.add("        <div style=\"word-wrap: break-all; overflow-wrap: break-all;\">" + html::THTML::applyFlowControl(value) + "</div>");
				list.add("      </td>");
				list.add("    </tr>");
			}
		}
	}

	// Close HTML table
	list.add("  </tbody>");
	list.add("</table>");
}


TVariantValues& TVariantValues::operator = (const TVariantValues& values) {
	clear();
	if (!values.empty()) {
		PNamedVariant o;
		size_t i,n;
		n = values.variants.size();
		for (i=0; i<n; i++) {
			o = values.variants[i];
			if (util::assigned(o)) {
				PNamedVariant p = new TNamedVariant;
				*p = *o;
				variants.push_back(p);
			}
		}
		invalidated = true;
	}
	return *this;
}

void TVariantValues::merge(const TVariantValues& values) {
	if (!values.empty()) {
		PNamedVariant o;
		size_t i,n,k;
		n = values.variants.size();
		for (i=0; i<n; i++) {
			o = values.variants[i];
			if (util::assigned(o)) {
				k = find(o->name());
				if (k == app::nsizet) {
					// Add new variant to list
					PNamedVariant p = new TNamedVariant;
					// Assign name and value
					*p = *o;
					variants.push_back(p);
				} else {
					// Update existing variant value
					PNamedVariant p = variants[k];
					if (util::assigned(p)) {
						// Assign only value to existing variant
						*p = o->value();
					}
				}
			}
		}
		invalidated = true;
	}
}

void TVariantValues::update(const std::string& value, size_t& hash, size_t& pos) const {
	for (size_t i=0; i<value.size(); ++i)
		hash += ++pos + (size_t)value[i];
}

size_t TVariantValues::hash(const char exclude) const {
	// Exclude parameters beginning with with '_' (= default value) from hashing
	size_t hash = 0;
	size_t pos = 0;
	if (!empty()) {
		PNamedVariant o;
		size_t i,n;
		n = variants.size();
		for (i=0; i<n; i++) {
			o = variants[i];
			if (util::assigned(o)) {
				if (o->name().size() >= 1) {
					if (exclude == NUL || o->name()[0] != exclude) {
						pos += i;
						update(o->name(), hash, pos);
						update(o->value().asString(), hash, pos);
					}
				}
			}
		}
	}
	return hash;
}

bool TVariantValues::compare(const TVariantValues& values) {
	return hash() == values.hash();
}

size_t TVariantValues::parseCSV(const char* data, const size_t size, const char delimiter) {
	clear();
	util::TStringList lines;
	lines.readText(data, size);
	if (!lines.empty()) {
		for (size_t i=0; i< lines.size(); ++i) {
			const std::string s = lines[i];
			size_t pos = s.find_first_of(delimiter);
			if (pos != std::string::npos) {
				if (pos > 0) {
					std::string key = util::trim(s.substr(0, pos));
					std::string value = pos < util::pred(s.size()) ? util::trim(s.substr(pos+1, std::string::npos)) : JSON_NULL;
					if (!key.empty()) {
						if (!value.empty()) {
							add(key, value);
						} else {
							add(key, JSON_NULL);
						}
					}
				}
			} else {
				// Empty key found, add as "null" value
				std::string key = util::trim(s);
				if (!key.empty()) {
					add(key, JSON_NULL);
				}
			}
		}
	}
	return this->size();
}

size_t TVariantValues::parseJSON(const std::string& json) {
	clear();
	parse(json, 1);
	return this->size();
}

void TVariantValues::onJsonDataField(const std::string& key, const std::string& value, const size_t row, const util::EVariantType type) {
	if (!key.empty()) {
		if (!value.empty()) {
			add(key, value);
		} else {
			add(key, JSON_NULL);
		}
	}
}

bool TVariantValues::saveToFile(const std::string& fileName) {
	util::deleteFile(fileName);
	if (!empty()) {
		util::TJsonList list;
		asJSON(list);
		if (!list.empty()) {
			list.saveToFile(fileName);
			return util::fileExists(fileName);
		}

	}
	return false;
}

bool TVariantValues::loadFromFile(const std::string& fileName) {
	clear();
	if (util::fileExists(fileName)) {
		util::TJsonParser::loadFromFile(fileName);
		if (!util::TJsonParser::empty()) {
			parse(1); // Parse one element/row only!
			return !empty();
		}
	}
	return false;
}



TBinaryValues::TBinaryValues() {
	prime();
}

TBinaryValues::~TBinaryValues() {
	clear();
}

void TBinaryValues::prime() {
	TVariantValues::prime();
	endian = util::TEndian::getSystemEndian();
	target = EE_NETWORK_BYTE_ORDER;
	debug = false;
	lock(); // Lock values by default, first assignent sets fixed type!
}


void TBinaryValues::add(const std::string& key, const char* value) {
	throw util::app_error("Adding plain string to TBinaryValues not allowed, apply with fixed length.");
}

void TBinaryValues::add(const char* key, const char* value) {
	throw util::app_error("Adding plain string to TBinaryValues not allowed, apply with fixed length.");
}

void TBinaryValues::add(const std::string& key, const std::string& value) {
	throw util::app_error("Adding plain string to TBinaryValues not allowed, apply with fixed length.");
}

void TBinaryValues::add(const char* key, const std::string& value) {
	throw util::app_error("Adding plain string to TBinaryValues not allowed, apply with fixed length.");
}


void TBinaryValues::add(const std::string& key, const char* value, const size_t length) {
	PNamedVariant o = getVariant(key);
	if (!assigned(o)) {
		o = new TNamedVariant(key);
		variants.push_back(o);
	}
	*o = value;
	o->reserve(length);
	invalidated = true;
}

void TBinaryValues::add(const char* key, const char* value, const size_t length) {
	PNamedVariant o = getVariant(key);
	if (!assigned(o)) {
		o = new TNamedVariant(key);
		variants.push_back(o);
	}
	*o = value;
	o->reserve(length);
	invalidated = true;
}

void TBinaryValues::add(const std::string& key, const std::string& value, const size_t length) {
	PNamedVariant o = getVariant(key);
	if (!assigned(o)) {
		o = new TNamedVariant(key);
		variants.push_back(o);
	}
	*o = value;
	o->reserve(length);
	invalidated = true;
}

void TBinaryValues::add(const char* key, const std::string& value, const size_t length) {
	PNamedVariant o = getVariant(key);
	if (!assigned(o)) {
		o = new TNamedVariant(key);
		variants.push_back(o);
	}
	*o = value;
	o->reserve(length);
	invalidated = true;
}

size_t TBinaryValues::getBinarySize() const {
	size_t length = 0;
	if (!empty()) {
		for (size_t idx=0; idx<size(); idx++) {
			const PNamedVariant o = variants[idx];
			if (util::assigned(o)) {
				if (o->length() > 0) {
					length += o->length();
				} else {
					// Invalid object length detected!
					if (debug) std::cout << "TBinaryValues::getBufferSize() Invalid object length." << std::endl;
					return (size_t)0;
				}
			}
		}
	}
	return length;
}

void TBinaryValues::copy(void *const dst, const void *const src, const size_t size) {
	memcpy(dst, src, size);
	if (debug) std::cout << "TBinaryValues::copy() Copied " << size << " bytes." << std::endl;
}

size_t TBinaryValues::serialize(util::TByteBuffer& buffer) {
	buffer.clear();
	size_t size = getBinarySize();
	if (size > 0) {
		bool convert = doEndianConversion();
		std::string s;

		// Fill buffer for needed length
		buffer.resize(size);
		buffer.fill(0);
		if (debug) std::cout << "TBinaryValues::serialize() Buffer size " << size << " bytes, endian conversion = " << convert << std::endl;

		// Store values in correct endian to buffer
		uint8_t* p = buffer.data();
		for (size_t idx=0; idx<variants.size(); idx++) {
			const PNamedVariant o = variants[idx];
			if (util::assigned(o)) {
				size_t length = o->length();
				const TVariant& value = o->value();
				EVariantType type = value.getType();
				switch (type) {
					case EVT_INTEGER8:
						storage.int8 = value.asInteger8();
						copy(p, &storage.int8, length);
						p += length;
						break;
					case EVT_INTEGER16:
						storage.int16 = value.asInteger16();
						if (debug) std::cout << "TBinaryValues::serialize() INT16 = " << storage.int16 << std::endl;
						if (convert) {
							storage.int16 = swap16(storage.int16);
							if (debug) std::cout << "TBinaryValues::serialize() INT16 after conversion= " << storage.int16 << std::endl;
						}
						copy(p, &storage.int16, length);
						p += length;
						break;
					case EVT_INTEGER32:
						storage.int32 = value.asInteger32();
						if (convert) {
							storage.int32 = swap32(storage.int32);
						}
						copy(p, &storage.int32, length);
						p += length;
						break;
					case EVT_INTEGER64:
						storage.int64 = value.asInteger64();
						if (convert) {
							storage.int64 = swap64(storage.int64);
						}
						copy(p, &storage.int64, length);
						p += length;
						break;
					case EVT_UNSIGNED8:
						storage.uint8 = value.asUnsigned8();
						copy(p, &storage.uint8, length);
						p += length;
						break;
					case EVT_UNSIGNED16:
						storage.uint16 = value.asUnsigned16();
						if (convert) {
							storage.uint16 = swap16(storage.uint16);
						}
						copy(p, &storage.uint16, length);
						p += length;
						break;
					case EVT_UNSIGNED32:
						storage.uint32 = value.asUnsigned32();
						if (convert) {
							storage.uint32 = swap32(storage.uint32);
						}
						copy(p, &storage.uint32, length);
						p += length;
						break;
					case EVT_UNSIGNED64:
						storage.uint64 = value.asUnsigned64();
						if (convert) {
							storage.uint64 = swap64(storage.uint64);
						}
						copy(p, &storage.uint64, length);
						p += length;
						break;
					case EVT_BOOLEAN:
						storage.int8 = value.asBoolean() ? 1 : 0;
						copy(p, &storage.int8, length);
						p += length;
						break;
					case EVT_STRING:
						s = value.asString();
						if (!s.empty()) {
							if (s.size() > length)
								s = s.substr(0, length);
							copy(p, s.c_str(), s.size());
						}
						p += length;
						break;
					default:
						break;
				}
			}
		}

	}
	return size;
}

size_t TBinaryValues::deserialize(util::TByteBuffer& buffer) {
	size_t size = getBinarySize();
	if (!buffer.empty() && size > 0) {
		// Check data definition --> buffer sizes should fit!
		if (buffer.size() == size) {
			bool convert = doEndianConversion();
			std::string s;
			size_t len;

			// Read values in correct endian from buffer
			const uint8_t* p = buffer.data();
			for (size_t idx=0; idx<variants.size(); idx++) {
				const PNamedVariant o = variants[idx];
				if (util::assigned(o)) {
					size_t length = o->length();
					TVariant& value = o->value();
					EVariantType type = value.getType();
					switch (type) {
						case EVT_INTEGER8:
							copy(&storage.int8, p, length);
							value = storage.int8;
							p += length;
							break;
						case EVT_INTEGER16:
							copy(&storage.int16, p, length);
							if (convert) {
								storage.int16 = swap16(storage.int16);
							}
							value = storage.int16;
							p += length;
							break;
						case EVT_INTEGER32:
							copy(&storage.int32, p, length);
							if (convert) {
								storage.int32 = swap32(storage.int32);
							}
							value = storage.int32;
							p += length;
							break;
						case EVT_INTEGER64:
							copy(&storage.int64, p, length);
							if (convert) {
								storage.int64 = swap64(storage.int64);
							}
							value = storage.int64;
							p += length;
							break;
						case EVT_UNSIGNED8:
							copy(&storage.uint8, p, length);
							value = storage.uint8;
							p += length;
							break;
						case EVT_UNSIGNED16:
							copy(&storage.uint16, p, length);
							if (convert) {
								storage.uint16 = swap16(storage.uint16);
							}
							value = storage.uint16;
							p += length;
							break;
						case EVT_UNSIGNED32:
							copy(&storage.uint32, p, length);
							if (convert) {
								storage.uint32 = swap32(storage.uint32);
							}
							value = storage.uint32;
							p += length;
							break;
						case EVT_UNSIGNED64:
							copy(&storage.uint64, p, length);
							if (convert) {
								storage.uint64 = swap64(storage.uint64);
							}
							value = storage.uint64;
							p += length;
							break;
						case EVT_BOOLEAN:
							copy(&storage.int8, p, length);
							value = (bool)(storage.int8 > 0);
							p += length;
							break;
						case EVT_STRING:
							len = strnlen((char*)p, length);
							if (len > 0) {
								s.assign((char*)p, len);
							}
							if (!s.empty()) {
								value = s;
							}
							p += length;
							break;
						default:
							break;
					}
				}
			}
		}
	}
	return size;
}

bool TBinaryValues::saveToFile(const std::string& fileName) {
	util::TByteBuffer buffer;
	serialize(buffer);
	if (debug) std::cout << "TBinaryValues::saveToFile() Save " << buffer.size() << " bytes to file <" << fileName << ">" << std::endl;
	if (!buffer.empty()) {
		bool r = util::writeFile(fileName, buffer.data(), buffer.size());
		int errval = errno;
		if (!r && debug) {
			std::cout << "TBinaryValues::saveToFile() Save file <" << fileName << "> failed: \"" << sysutil::getSysErrorMessage(errval) << std::endl;
		}
		return r;
	}
	return false;
}

bool TBinaryValues::loadFromFile(const std::string& fileName) {
	util::TByteBuffer buffer;
	size_t size = util::fileSize(fileName);
	if (size > 0) {
		// Read file to buffer
		buffer.resize(size);
		ssize_t r = util::readFile(fileName, buffer.data(), buffer.size());
		if (r == (ssize_t)size) {
			size_t read = deserialize(buffer);
			if (read == size)
				return true;
		}
	}
	return false;
}



} /* namespace util */
