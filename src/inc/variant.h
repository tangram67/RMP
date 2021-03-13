/*
 * variant.h
 *
 *  Created on: 30.05.2015
 *      Author: Dirk Brinkmeier
 */


/*
 * As an example for possible data types see:
 * https://msdn.microsoft.com/en-us/library/ms710150(v=vs.85).aspx
 *
 * SQL_CHAR				CHAR(n)				Character string of fixed string length n.
 * SQL_VARCHAR			VARCHAR(n)			Variable-length character string with a maximum string length n.
 * SQL_LONGVARCHAR		LONG VARCHAR		Variable length character data. Maximum length is data source–dependent.[9]
 * SQL_WCHAR			WCHAR(n)			Unicode character string of fixed string length n
 * SQL_WVARCHAR			VARWCHAR(n)			Unicode variable-length character string with a maximum string length n
 * SQL_WLONGVARCHAR		LONGWVARCHAR		Unicode variable-length character data. Maximum length is data source–dependent
 * SQL_DECIMAL			DECIMAL(p,s)		Signed, exact, numeric value with a precision of at least p and scale s. (The maximum precision is driver-defined.) (1 <= p <= 15; s <= p).[4]
 * SQL_NUMERIC			NUMERIC(p,s)		Signed, exact, numeric value with a precision p and scale s  (1 <= p <= 15; s <= p).[4]
 * SQL_SMALLINT			SMALLINT			Exact numeric value with precision 5 and scale 0 (signed: –32,768 <= n <= 32,767, unsigned: 0 <= n <= 65,535)[3].
 * SQL_INTEGER			INTEGER				Exact numeric value with precision 10 and scale 0 (signed: –2[31] <= n <= 2[31] – 1, unsigned: 0 <= n <= 2[32] – 1)[3].
 * SQL_REAL				REAL				Signed, approximate, numeric value with a binary precision 24 (zero or absolute value 10[–38] to 10[38]).
 * SQL_FLOAT			FLOAT(p)			Signed, approximate, numeric value with a binary precision of at least p. (The maximum precision is driver-defined.)[5]
 * SQL_DOUBLE			DOUBLE PRECISION	Signed, approximate, numeric value with a binary precision 53 (zero or absolute value 10[–308] to 10[308]).
 * SQL_BIT				BIT					Single bit binary data.[8]
 * SQL_TINYINT			TINYINT				Exact numeric value with precision 3 and scale 0 (signed: –128 <= n <= 127, unsigned: 0 <= n <= 255)[3].
 * SQL_BIGINT			BIGINT				Exact numeric value with precision 19 (if signed) or 20 (if unsigned) and scale 0 (signed: –2[63] <= n <= 2[63] – 1, unsigned: 0 <= n <= 2[64] – 1)[3],[9].
 * SQL_BINARY			BINARY(n)			Binary data of fixed length n.[9]
 * SQL_VARBINARY		VARBINARY(n)		Variable length binary data of maximum length n. The maximum is set by the user.[9]
 * SQL_LONGVARBINARY	LONG VARBINARY		Variable length binary data. Maximum length is data source–dependent.[9]
 * SQL_TYPE_DATE[6]			DATE			Year, month, and day fields, conforming to the rules of the Gregorian calendar. (See Constraints of the Gregorian Calendar, later in this appendix.)
 * SQL_TYPE_TIME[6]			TIME(p)			Hour, minute, and second fields, with valid values for hours of 00 to 23, valid values for minutes of 00 to 59, and valid values for seconds of 00 to 61. Precision p indicates the seconds precision.
 * SQL_TYPE_TIMESTAMP[6]	TIMESTAMP(p)	Year, month, day, hour, minute, and second fields, with valid values as defined for the DATE and TIME data types.
 * SQL_TYPE_UTCDATETIME		UTCDATETIME		Year, month, day, hour, minute, second, utchour, and utcminute fields. The utchour and utcminute fields have 1/10 microsecond precision.
 * SQL_TYPE_UTCTIME			UTCTIME			Hour, minute, second, utchour, and utcminute fields. The utchour and utcminute fields have 1/10 microsecond precision..
 * SQL_INTERVAL_MONTH[7]				INTERVAL MONTH(p)				Number of months between two dates; p is the interval leading precision.
 * SQL_INTERVAL_YEAR[7]					INTERVAL YEAR(p)				Number of years between two dates; p is the interval leading precision.
 * SQL_INTERVAL_YEAR_TO_MONTH[7]		INTERVAL YEAR(p) TO MONTH		Number of years and months between two dates; p is the interval leading precision.
 * SQL_INTERVAL_DAY[7]					INTERVAL DAY(p)					Number of days between two dates; p is the interval leading precision.
 * SQL_INTERVAL_HOUR[7]					INTERVAL HOUR(p)				Number of hours between two date/times; p is the interval leading precision.
 * SQL_INTERVAL_MINUTE[7]				INTERVAL MINUTE(p)				Number of minutes between two date/times; p is the interval leading precision.
 * SQL_INTERVAL_SECOND[7]				INTERVAL SECOND(p,q)			Number of seconds between two date/times; p is the interval leading precision and q is the interval seconds precision.
 * SQL_INTERVAL_DAY_TO_HOUR[7]			INTERVAL DAY(p) TO HOUR			Number of days/hours between two date/times; p is the interval leading precision.
 * SQL_INTERVAL_DAY_TO_MINUTE[7]		INTERVAL DAY(p) TO MINUTE		Number of days/hours/minutes between two date/times; p is the interval leading precision.
 * SQL_INTERVAL_DAY_TO_SECOND[7]		INTERVAL DAY(p) TO SECOND(q)	Number of days/hours/minutes/seconds between two date/times; p is the interval leading precision and q is the interval seconds precision.
 * SQL_INTERVAL_HOUR_TO_MINUTE[7]		INTERVAL HOUR(p) TO MINUTE		Number of hours/minutes between two date/times; p is the interval leading precision.
 * SQL_INTERVAL_HOUR_TO_SECOND[7]		INTERVAL HOUR(p) TO SECOND(q)	Number of hours/minutes/seconds between two date/times; p is the interval leading precision and q is the interval seconds precision.
 * SQL_INTERVAL_MINUTE_TO_SECOND[7]		INTERVAL MINUTE(p) TO SECOND(q)	Number of minutes/seconds between two date/times; p is the interval leading precision and q is the interval seconds precision.
 * SQL_GUID				GUID				Fixed length GUID.
 *
 *
 * Example data types for sqlite3:
 * https://www.sqlite.org/datatype3.html
 *
 * INT
 * INTEGER
 * TINYINT
 * SMALLINT
 * MEDIUMINT
 * BIGINT
 * UNSIGNED BIG INT
 * INT2
 * INT8					INTEGER	1
 *
 * CHARACTER(20)
 * VARCHAR(255)
 * VARYING CHARACTER(255)
 * NCHAR(55)
 * NATIVE CHARACTER(70)
 * NVARCHAR(100)
 * TEXT
 * CLOB	TEXT	2
 *
 * BLOB					no datatype specified	NONE	3
 *
 * REAL
 * DOUBLE
 * DOUBLE PRECISION
 * FLOAT				REAL	4
 *
 * NUMERIC
 * DECIMAL(10,5)
 * BOOLEAN
 * DATE
 * DATETIME				NUMERIC	5
 *
 */


#ifndef VARIANT_H_
#define VARIANT_H_

#include <string>
#include <type_traits>
#include "vartypes.h"
#include "fileutils.h"
#include "endianutils.h"
#include "numlimits.h"
#include "datetime.h"
#include "classes.h"
#include "locale.h"
#include "blob.h"
#include "json.h"
#include "gcc.h"

namespace util {


class TVariant;
class TNamedVariant;


#ifdef STL_HAS_TEMPLATE_ALIAS

using PNamedVariant = TNamedVariant*;
using TVariantList = std::vector<util::PNamedVariant>;
using TOnVariantChanged = std::function<void(const util::TVariant&)>;

#else

typedef TNamedVariant* PNamedVariant;
typedef std::vector<util::PNamedVariant> TVariantList;
typedef std::function<void(const util::TVariant&)> TOnVariantChanged;

#endif



struct CVariantValue {
	int64_t integer;
	uint64_t uinteger;
	bool boolean;
	double decimal;
	TDateTime time;
	std::string astring;
	std::wstring wstring;
	util::TBlob blob;
};

struct CBinaryValue {
	int8_t int8;
	int16_t int16;
	int32_t int32;
	int64_t int64;
	uint8_t uint8;
	uint16_t uint16;
	uint32_t uint32;
	uint64_t uint64;
};


class TVariant {
private:
	CVariantValue value;
	EVariantType varType;
	EBooleanType boolType;
	EDateTimeFormat timeType;
	EDateTimePrecision timePrecision;
	TDoublePrecision precision;
	std::string formatA;
	std::wstring formatW;

	const app::TLocale* locale;
	TOnVariantChanged onChange;

	bool locked;
	bool svalue, uvalue;
	mutable std::string cstr;
	mutable std::wstring wstr;
	mutable TDateTime time;
	mutable util::TBlob blob;
	mutable TJsonValue json;

	void prime();
	void release();
	void changed();
	bool readBoolValueAndTypeA(const std::string& value, EBooleanType& type) const;
	bool readBoolValueAndTypeW(const std::wstring& value, EBooleanType& type) const;
	std::string writeBoolValueForTypeA(const bool value, const EBooleanType type) const;
	std::wstring writeBoolValueForTypeW(const bool value, const EBooleanType type) const;
	std::string printf(const std::string &fmt, ...) const;
	std::wstring printf(const std::wstring &fmt, ...) const;
	std::string printf(const app::TLocale& locale, const std::string &fmt, ...) const;
	std::wstring printf(const app::TLocale& locale, const std::wstring &fmt, ...) const;

public:
	size_t size() const ;

	void clear();
	void lock() { locked = true; };
	void unlock() { locked = false; };

	static EVariantType guessType(const std::string& value, const size_t depth = 5, const bool debug = false);
	std::string getTypeAsString() const;
	const EVariantType getType() const { return varType; };
	void setType(EVariantType type);
	const bool hasData() const { return (varType != EVT_UNKNOWN); };
	const bool isSigned() const { return svalue; };
	const bool isUnsigned() const { return uvalue; };
	const bool isBoolean() const { return (varType == EVT_BOOLEAN); };
	const bool isString() const { return (varType == EVT_STRING); };
	const bool isWideString() const { return (varType == EVT_WIDE_STRING); };
	const bool isNumeric() const { return (varType == EVT_DOUBLE); };
	const bool isBlob() const { return (varType == EVT_BLOB); };
	const bool isTime() const { return (varType == EVT_TIME); };
	const bool isNull() const { return (varType == EVT_NULL); };

	static bool isValidType(const EVariantType type) { return (type != util::EVT_INVALID && type != util::EVT_UNKNOWN); }
	const bool isValid() const { return isValidType(varType); };
	// operator bool() const { return isValidType(varType); };

	void setBoolType(const EBooleanType type) { boolType = type; };
	void setTimeFormat(const EDateTimeFormat type);
	void setTimePrecision(const util::EDateTimePrecision value);
	void setPrecision(const TDoublePrecision precision);

	void now();
	void epoch();

	void imbue(const app::TLocale& locale);
	const app::TLocale& getLocale() const { return *locale; };

	template<typename method_t, typename class_t>
	inline void bindOnChanged(method_t &&onChangeMethod, class_t &&owner) {
		onChange = std::bind(onChangeMethod, owner, std::placeholders::_1);
	}

	TVariant& operator = (const int8_t value);
	TVariant& operator = (const uint8_t value);
	TVariant& operator = (const int16_t value);
	TVariant& operator = (const uint16_t value);
	TVariant& operator = (const int32_t value);
	TVariant& operator = (const uint32_t value);
	TVariant& operator = (const int64_t value);
	TVariant& operator = (const uint64_t value);
	TVariant& operator = (const double value);
	TVariant& operator = (const float value);
	TVariant& operator = (const bool value);
	TVariant& operator = (const char* value);
	TVariant& operator = (const std::string& value);
	TVariant& operator = (const wchar_t* value);
	TVariant& operator = (const std::wstring& value);
	TVariant& operator = (const TBlob& value);
	TVariant& operator = (const TDateTime& value);
	TVariant& operator = (const TVariant& value);
	
	void setInteger8(const int8_t value);
	void setInteger16(const int16_t value);
	void setInteger32(const int32_t value);
	void setInteger64(const int64_t value);

	void setUnsigned8(const uint8_t value);
	void setUnsigned16(const uint16_t value);
	void setUnsigned32(const uint32_t value);
	void setUnsigned64(const uint64_t value);

	void setDateTime(const TDateTime& value);
	void setDateTime(const TTimePart value);
	void setDateTime(const TTimeNumeric value);
	void setDateTime(const TTimePart seconds, const TTimePart microseconds);

	void setJulian(const TTimeNumeric value);
	void setJulian(const TTimePart seconds, const TTimePart microseconds);

	void setJ2K(const TTimeNumeric value);
	void setJ2K(const TTimePart seconds, const TTimePart microseconds);

	void setDouble(const double value);
	void setDouble(const float value);
	void setBoolean(const bool value);
	void setString(const char *value);
	void setString(const char *value, size_t size);
	void setString(const std::string& value);
	void setWideString(const wchar_t* value);
	void setWideString(const wchar_t* value, size_t size);
	void setWideString(const std::wstring& value);
	void setBlob(const util::TBlob& value);
	void setBlob(const void *const value, const size_t size);
	void setVariant(const TVariant& value);

	void setValue(const std::string& value, EVariantType type);
	void setInteger8(const std::string& value);
	void setInteger16(const std::string& value);
	void setInteger32(const std::string& value);
	void setInteger64(const std::string& value);
	void setUnsigned8(const std::string& value);
	void setUnsigned16(const std::string& value);
	void setUnsigned32(const std::string& value);
	void setUnsigned64(const std::string& value);
	void setDateTime(const std::string& value);
	void setDouble(const std::string& value);
	void setBoolean(const std::string& value);

	void setValue(const std::wstring& value, EVariantType type);
	void setInteger8(const std::wstring& value);
	void setInteger16(const std::wstring& value);
	void setInteger32(const std::wstring& value);
	void setInteger64(const std::wstring& value);
	void setUnsigned8(const std::wstring& value);
	void setUnsigned16(const std::wstring& value);
	void setUnsigned32(const std::wstring& value);
	void setUnsigned64(const std::wstring& value);
	void setDateTime(const std::wstring& value);
	void setDouble(const std::wstring& value);
	void setBoolean(const std::wstring& value);

	const char* c_str() const;
	const wchar_t* w_str() const;

	char* getChar(const char** data, size_t& size);
	wchar_t* getWideChar(const wchar_t** data, size_t& size);

	TBlob& getBlob(const char** data, size_t& size);
	TBlob& getBlob();

	TDateTime& getTime();

	bool asBoolean(const bool defValue = false) const;
	int32_t asInteger(int32_t defValue = TLimits::nan) const { return asInteger32(defValue); };
	uint32_t asUnsigned(uint32_t defValue = TLimits::unan) const { return asUnsigned32(defValue); };
	int8_t asInteger8(int8_t defValue = TLimits::nan8) const;
	int16_t asInteger16(int16_t defValue = TLimits::nan16) const;
	int32_t asInteger32(int32_t defValue = TLimits::nan32) const;
	int64_t asInteger64(int64_t defValue = TLimits::nan64) const;
	uint8_t asUnsigned8(uint8_t defValue = TLimits::unan8) const;
	uint16_t asUnsigned16(uint16_t defValue = TLimits::unan16) const;
	uint32_t asUnsigned32(uint32_t defValue = TLimits::unan32) const;
	uint64_t asUnsigned64(uint64_t defValue = TLimits::unan64) const;
	double asDouble(double defValue = 0.0, const app::TLocale& locale = syslocale) const;
	std::string asString(const std::string& defValue = "") const;
	std::wstring asWideString(const std::wstring& defValue = L"") const;
	const util::TDateTime& asTime(const EDateTimeZone = ETZ_DEFAULT, TTimePart defValue = util::epoch()) const;
	std::string asJSON(const std::string& preamble = "", const std::string& name = "Variant") const;
	const util::TBlob& asBlob() const;


	// Only POD types are allwed as non-type template parameters
	// --> not used here, because operator () needs typed templates
	//	template <bool> bool as() const { return asBoolean(); };
	//	template <int8_t> int8_t as() const { return asInteger16(); };
	//	template <int16_t> int16_t as() const { return asInteger16(); };
	//	template <int32_t> int32_t as() const { return asInteger32(); };
	//	template <int64_t> int64_t as() const { return asInteger64(); };
	//	template <uint8_t> uint8_t as() const { return asUnsigned8(); };
	//	template <uint16_t> uint16_t as() const { return asUnsigned16(); };
	//	template <uint32_t> uint32_t as() const { return asUnsigned32(); };
	//	template <uint64_t> uint64_t as() const { return asUnsigned64(); };
	//	template <char*> const char* as() const { return c_str(); };
	//	template <wchar_t*> const wchar_t* as() const { return w_str(); };
	//	template <const char*> const char* as() const { return c_str(); };
	//	template <const wchar_t*> const wchar_t* as() const { return w_str(); };

	// Check types via enable_if and is_same
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, bool>::value, bool> = true>
	const variant_t as() { return asBoolean(); };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int8_t>::value, bool> = true>
	const variant_t as() { return asInteger8(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int16_t>::value, bool> = true>
	const variant_t as() { return asInteger16(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int32_t>::value, bool> = true>
	const variant_t as() { return asInteger32(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int64_t>::value, bool> = true>
	const variant_t as() { return asInteger64(); };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint8_t>::value, bool> = true>
	const variant_t as() { return asUnsigned8(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint16_t>::value, bool> = true>
	const variant_t as() { return asUnsigned16(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint32_t>::value, bool> = true>
	const variant_t as() { return asUnsigned32(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint64_t>::value, bool> = true>
	const variant_t as() { return asUnsigned64(); };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, char*>::value, bool> = true>
	const variant_t as() { return c_str(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, wchar_t*>::value, bool> = true>
	const variant_t as() { return w_str(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, const char*>::value, bool> = true>
	const variant_t as() { return c_str(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, const wchar_t*>::value, bool> = true>
	const variant_t as() { return w_str(); };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, util::TDateTime>::value, bool> = true>
	const variant_t& as() { return asTime(); };
	template<typename variant_t, util::enable_if_type<std::is_same<variant_t, std::string>::value, bool> = true>
	variant_t as() { return asString(); };
	template<typename variant_t, util::enable_if_type<std::is_same<variant_t, std::wstring>::value, bool> = true>
	variant_t as() { return asWideString(); };
	template<typename variant_t, util::enable_if_type<std::is_same<variant_t, double>::value, bool> = true>
	variant_t as() { return asDouble(); };


	// Check operator type via enable_if and is_same
	// --> e.g. assignments like double value = variant;
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, bool>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int8_t>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int16_t>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int32_t>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int64_t>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint8_t>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint16_t>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint32_t>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint64_t>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, char*>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, wchar_t*>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, const char*>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, const wchar_t*>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, util::TDateTime>::value, bool> = true>
	operator const variant_t& () { return as<variant_t>(); };
	template<typename variant_t, util::enable_if_type<std::is_same<variant_t, std::string>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template<typename variant_t, util::enable_if_type<std::is_same<variant_t, std::wstring>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };
	template<typename variant_t, util::enable_if_type<std::is_same<variant_t, double>::value, bool> = true>
	operator variant_t () { return as<variant_t>(); };


	// Only POD types are allwed as non-type template parameters
	// --> Not used here to maintain compatibility
	//	template <bool> bool is() const { return varType == EVT_BOOLEAN; };
	//	template <int8_t> bool is() const { return varType == EVT_INTEGER8; };
	//	template <int16_t> bool is() const { return varType == EVT_INTEGER16; };
	//	template <int32_t> bool is() const { return varType == EVT_INTEGER32; };
	//	template <int64_t> bool is() const { return varType == EVT_INTEGER64; };
	//	template <uint8_t> bool is() const { return varType == EVT_UNSIGNED8; };
	//	template <uint16_t> bool is() const { return varType == EVT_UNSIGNED16; };
	//	template <uint32_t> bool is() const { return varType == EVT_UNSIGNED32; };
	//	template <uint64_t> bool is() const { return varType == EVT_UNSIGNED64; };
	//	template <char*> bool is() const { return varType == EVT_STRING; };
	//	template <wchar_t*> bool is() const { return varType == EVT_WIDE_STRING; };
	//	template <const char*> bool is() const { return varType == EVT_STRING; };
	//	template <const wchar_t*> bool is() const { return varType == EVT_WIDE_STRING; };

	// Check types via enable_if and is_same
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, bool>::value, bool> = true>
	bool is() const { return varType == EVT_BOOLEAN; };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int8_t>::value, bool> = true>
	bool is() const { return varType == EVT_INTEGER8; };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int16_t>::value, bool> = true>
	bool is() const { return varType == EVT_INTEGER16; };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int32_t>::value, bool> = true>
	bool is() const { return varType == EVT_INTEGER32; };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, int64_t>::value, bool> = true>
	bool is() const { return varType == EVT_INTEGER64; };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint8_t>::value, bool> = true>
	bool is() const { return varType == EVT_UNSIGNED8; };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint16_t>::value, bool> = true>
	bool is() const { return varType == EVT_UNSIGNED16; };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint32_t>::value, bool> = true>
	bool is() const { return varType == EVT_UNSIGNED32; };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, uint64_t>::value, bool> = true>
	bool is() const { return varType == EVT_UNSIGNED64; };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, char*>::value, bool> = true>
	bool is() const { return varType == EVT_STRING; };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, wchar_t*>::value, bool> = true>
	bool is() const { return varType == EVT_WIDE_STRING; };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, const char*>::value, bool> = true>
	bool is() const { return varType == EVT_STRING; };
	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, const wchar_t*>::value, bool> = true>
	bool is() const { return varType == EVT_WIDE_STRING; };

	template <typename variant_t, util::enable_if_type<std::is_same<variant_t, util::TDateTime>::value, bool> = true>
	bool is() const { return varType == EVT_TIME; };
	template<typename variant_t, util::enable_if_type<std::is_same<variant_t, std::string>::value, bool> = true>
	bool is() const { return varType == EVT_STRING; };
	template<typename variant_t, util::enable_if_type<std::is_same<variant_t, std::wstring>::value, bool> = true>
	bool is() const { return varType == EVT_WIDE_STRING; };
	template<typename variant_t, util::enable_if_type<std::is_same<variant_t, double>::value, bool> = true>
	bool is() const { return varType == EVT_DOUBLE; };


	TVariant(const EVariantType type);
	TVariant(const int8_t value);
	TVariant(const int16_t value);
	TVariant(const int32_t value);
	TVariant(const int64_t value);
	TVariant(const uint8_t value);
	TVariant(const uint16_t value);
	TVariant(const uint32_t value);
	TVariant(const uint64_t value);
	TVariant(const double value);
	TVariant(const float value);
	TVariant(const bool value);
	TVariant(const char *value);
	TVariant(const char *value, size_t size);
	TVariant(const std::string &value);
	TVariant(const wchar_t *value);
	TVariant(const wchar_t *value, size_t size);
	TVariant(const std::wstring &value);
	TVariant(const TBlob &value);
	TVariant(const TDateTime& value);
	TVariant(const TVariant &value);
	TVariant(app::TObject &value);
	TVariant(TBlob &&value);

	TVariant();
	virtual ~TVariant();
};



class TNamedVariant {
private:
	size_t m_length;
	std::string m_name;
	util::TVariant m_value;

	void prime() {
		m_length = 0;
	}

public:
	void assign(const TNamedVariant& value) {
		m_name = value.name();
		m_value = value.value();
		m_length = value.length();
	}

	// Take over both name and value from given variant
	TNamedVariant& operator = (const TNamedVariant& value) {
		assign(value);
		return *this;
	}

	// Assign only the given value to variant
	template<typename value_t>
	TNamedVariant&  operator = (const value_t& value) {
		m_value = value;
		m_length = m_value.size();
		return *this;
	}

	void reserve(const size_t length) {
		m_length = length;
	}

	void clear() {
		m_name.clear();
		m_value.clear();
		prime();
	}

	size_t length() const { return m_length; };
	const std::string& name() const { return m_name; };
	const util::TVariant& value() const { return m_value; };
	util::TVariant& value() { return m_value; };

	TNamedVariant() { prime(); };
	TNamedVariant(const std::string name) : m_name(name) { prime(); };
	virtual ~TNamedVariant() = default;
};


class TVariantValues : private util::TJsonParser  {
private:
	mutable util::TVariant defVar;
	mutable util::TNamedVariant defVal;
	mutable util::TStringList html;
	mutable util::TJsonList json;
	mutable std::string csv;
	std::string defStr;
	bool tolower;

	void onJsonDataField(const std::string& key, const std::string& value, const size_t row, const util::EVariantType type);

	bool compare(const TVariantValues& values);
	void update(const std::string& value, size_t& hash, size_t& pos) const;

protected:
	TVariantList variants;
	bool invalidated;

	void prime();

	const PNamedVariant getVariant(const size_t index) const;
	const PNamedVariant getVariant(const std::string& key) const;
	const PNamedVariant getVariant(const char* key) const;

public:
	typedef TVariantList::const_iterator const_iterator;

	const_iterator begin() const { return variants.begin(); };
	const_iterator end() const { return variants.end(); };

	void clear();
	void reset();
	void invalidate();
	void lock();
	void unlock();

	size_t hash(const char exclude = '_') const;
	size_t size() const { return variants.size(); };
	bool empty() const { return variants.empty(); };
	bool changed() const { return invalidated; };

	void addEntry(const char* value);
	void addEntry(const std::string& value);

	template<typename value_t>
	inline void add(const std::string& key, const value_t& value) {
		PNamedVariant o = getVariant(key);
		if (!assigned(o)) {
			o = new TNamedVariant(key);
			variants.push_back(o);
		}
		*o = value;
		invalidated = true;
	}

	template<typename value_t>
	inline void add(const char* key, const value_t& value) {
		PNamedVariant o = getVariant(key);
		if (!assigned(o)) {
			o = new TNamedVariant(key);
			variants.push_back(o);
		}
		*o = value;
		invalidated = true;
	}

	bool validIndex(const size_t index) const;
	bool hasKey(const std::string& key) const;
	bool hasKey(const char* key) const;

	size_t find(const std::string& key) const;
	size_t find(const char* key, size_t size) const;
	size_t find(const char* key) const;

	void debugOutput(const std::string& name = "Variant", const std::string& preamble = "") const;

	const std::string& asText(const char delimiter = ' ', const std::string separator = "=") const;
	void asJSON(util::TJsonList& list, const std::string& name, const EJsonArrayType type = EJT_ARRAY) const;
	void asJSON(util::TJsonList& list) const;
	const util::TJsonList& asJSON(const std::string& name) const;
	const util::TJsonList& asJSON() const;
	void asHTML(util::TStringList& list) const;
	const util::TStringList& asHTML() const;

	const std::string& name(const int index) const;
	const std::string& name(const size_t index) const ;

	const util::TNamedVariant& variant(const int index) const;
	const util::TNamedVariant& variant(const size_t index) const;
	const util::TNamedVariant& variant(const std::string& key) const;
	const util::TNamedVariant& variant(const char* key) const;

	util::TVariant& value(const int index);
	util::TVariant& value(const size_t index);
	util::TVariant& value(const std::string& key);
	util::TVariant& value(const char* key);

	const util::TVariant& value(const int index) const;
	const util::TVariant& value(const size_t index) const;
	const util::TVariant& value(const std::string& key) const;
	const util::TVariant& value(const char* key) const;

	util::TVariant& operator [] (const int index);
	util::TVariant& operator [] (const size_t index);
	util::TVariant& operator [] (const std::string& key);
	util::TVariant& operator [] (const char* key);

	const util::TVariant& operator [] (const int index) const;
	const util::TVariant& operator [] (const size_t index) const;
	const util::TVariant& operator [] (const std::string& key) const;
	const util::TVariant& operator [] (const char* key) const;

	void merge(const TVariantValues& values);
	void assign(util::TCharPointerArray& argv, const util::TStringList& header) const;
	size_t parseCSV(const char* data, const size_t size, const char delimiter);
	size_t parseJSON(const std::string& json);

	bool saveToFile(const std::string& fileName);
	bool loadFromFile(const std::string& fileName);

	TVariantValues& operator = (const TVariantValues& values);
	inline bool operator == (const TVariantValues& values) { return compare(values); };
	inline bool operator != (const TVariantValues& values) { return !compare(values); };

	TVariantValues();
	TVariantValues(const bool tolower);
	TVariantValues(const TVariantValues& values);
	virtual ~TVariantValues();
};


class TBinaryValues : public util::TVariantValues, private TEndian  {
private:
	bool debug;
	EEndianType endian;
	EEndianType target;
	CBinaryValue storage;

	void prime();
	bool doEndianConversion() const { return endian != target; };

	// Overwrite plain string add methods and change visibility...
	void add(const std::string& key, const char* value);
	void add(const char* key, const char* value);
	void add(const std::string& key, const std::string& value);
	void add(const char* key, const std::string& value);

	void copy(void *const dst, const void *const src, const size_t size);

public:
	EEndianType getEndian() const { return target; };
	void setEndian(const EEndianType value) { target = value; };

	// Add string with reserved length information
	void add(const std::string& key, const char* value, const size_t length);
	void add(const char* key, const char* value, const size_t length);
	void add(const std::string& key, const std::string& value, const size_t length);
	void add(const char* key, const std::string& value, const size_t length);

	template<typename value_t>
	inline void add(const std::string& key, const value_t& value) {
		TVariantValues::add(key, value);
	}

	template<typename value_t>
	inline void add(const char* key, const value_t& value) {
		TVariantValues::add(key, value);
	}

	size_t serialize(util::TByteBuffer &buffer);
	size_t deserialize(util::TByteBuffer& buffer);
	size_t getBinarySize() const;

	bool saveToFile(const std::string& fileName);
	bool loadFromFile(const std::string& fileName);

	TBinaryValues();
	virtual ~TBinaryValues();
};

} /* namespace util */

#endif /* VARIANT_H_ */
