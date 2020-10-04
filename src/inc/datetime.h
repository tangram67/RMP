/*
 * datetime.h
 *
 *  Created on: 23.08.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef DATETIME_H_
#define DATETIME_H_

#ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#  define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>
#include <sys/time.h>
#include <sstream>
#include "timetypes.h"
#include "syslocale.h"
#include "gcc.h"

#if not defined USE_DST_TIMESTAMP
#  define DO_NOT_USE_DST_TIMESTAMP
#endif

namespace util {

//
// See http://pubs.opengroup.org/onlinepubs/9699919799/functions/strftime.html
//
//	The following conversion specifiers shall be supported:
//
//	a
//		Replaced by the locale's abbreviated weekday name. [ tm_wday]
//	A
//		Replaced by the locale's full weekday name. [ tm_wday]
//	b
//		Replaced by the locale's abbreviated month name. [ tm_mon]
//	B
//		Replaced by the locale's full month name. [ tm_mon]
//	c
//		Replaced by the locale's appropriate date and time representation. (See the Base Definitions volume of POSIX.1-2008, <time.h>.)
//	C
//		Replaced by the year divided by 100 and truncated to an integer, as a decimal number. [ tm_year]
//
//		If a minimum field width is not specified, the number of characters placed into the array pointed to by s will be the number
//      of digits in the year divided by 100 or two, whichever is greater. [CX] [Option Start]  If a minimum field width is specified, 
//      the number of characters placed into the array pointed to by s will be the number of digits in the year divided by 100 or 
//      the minimum field width, whichever is greater. [Option End]
//	d
//		Replaced by the day of the month as a decimal number [01,31]. [ tm_mday]
//	D
//		Equivalent to %m / %d / %y. [ tm_mon, tm_mday, tm_year]
//	e
//		Replaced by the day of the month as a decimal number [1,31]; a single digit is preceded by a space. [ tm_mday]
//	F
//		[CX] Equivalent to %+4[Option End]Y-%m-%d if no flag and no minimum field width are specified. [ tm_year, tm_mon, tm_mday]
//
//		[CX] [Option Start] If a minimum field width of x is specified, the year shall be output as if by the Y specifier (described below) 
//      with whatever flag was given and a minimum field width of x-6. If x is less than 6, the behavior shall be as if x equalled 6.
//
//		If the minimum field width is specified to be 10, and the year is four digits long, then the output string produced will match 
//      the ISO 8601:2000 standard subclause 4.1.2.2 complete representation, extended format date representation of a specific day. 
//      If a + flag is specified, a minimum field width of x is specified, and x-7 bytes are sufficient to hold the digits of 
//      the year (not including any needed sign character), then the output will match the ISO 8601:2000 standard subclause 4.1.2.4 complete representation, 
//      expanded format date representation of a specific day. [Option End]
//	g
//		Replaced by the last 2 digits of the week-based year (see below) as a decimal number [00,99]. [ tm_year, tm_wday, tm_yday]
//	G
//		Replaced by the week-based year (see below) as a decimal number (for example, 1977). [ tm_year, tm_wday, tm_yday]
//
//		[CX] [Option Start] If a minimum field width is specified, the number of characters placed into the array pointed to by s 
//      will be the number of digits and leading sign characters (if any) in the year, or the minimum field width, whichever is greater. [Option End]
//
//	h
//		Equivalent to %b. [ tm_mon]
//	H
//		Replaced by the hour (24-hour clock) as a decimal number [00,23]. [ tm_hour]
//	I
//		Replaced by the hour (12-hour clock) as a decimal number [01,12]. [ tm_hour]
//	j
//		Replaced by the day of the year as a decimal number [001,366]. [ tm_yday]
//	m
//		Replaced by the month as a decimal number [01,12]. [ tm_mon]
//	M
//		Replaced by the minute as a decimal number [00,59]. [ tm_min]
//	n
//		Replaced by a <newline>.
//	p
//		Replaced by the locale's equivalent of either a.m. or p.m. [ tm_hour]
//	r
//		Replaced by the time in a.m. and p.m. notation; [CX] [Option Start]  in the POSIX locale this shall be equivalent to
//      %I : %M : %S %p. [Option End] [ tm_hour, tm_min, tm_sec]
//	R
//		Replaced by the time in 24-hour notation ( %H : %M ). [ tm_hour, tm_min]
//	S
//		Replaced by the second as a decimal number [00,60]. [ tm_sec]
//	t
//		Replaced by a <tab>.
//	T
//		Replaced by the time ( %H : %M : %S ). [ tm_hour, tm_min, tm_sec]
//	u
//		Replaced by the weekday as a decimal number [1,7], with 1 representing Monday. [ tm_wday]
//	U
//		Replaced by the week number of the year as a decimal number [00,53]. 
//      The first Sunday of January is the first day of week 1; days in the new year before this are in week 0. 
//      [ tm_year, tm_wday, tm_yday]
//	V
//		Replaced by the week number of the year (Monday as the first day of the week) as a decimal number [01,53]. 
//      If the week containing 1 January has four or more days in the new year, then it is considered week 1. 
//      Otherwise, it is the last week of the previous year, and the next week is week 1. 
//      Both January 4th and the first Thursday of January are always in week 1. [ tm_year, tm_wday, tm_yday]
//	w
//		Replaced by the weekday as a decimal number [0,6], with 0 representing Sunday. [ tm_wday]
//	W
//		Replaced by the week number of the year as a decimal number [00,53]. The first Monday of January is the first day of week 1; 
//      days in the new year before this are in week 0. [ tm_year, tm_wday, tm_yday]
//	x
//		Replaced by the locale's appropriate date representation. (See the Base Definitions volume of POSIX.1-2008, <time.h>.)
//	X
//		Replaced by the locale's appropriate time representation. (See the Base Definitions volume of POSIX.1-2008, <time.h>.)
//	y
//		Replaced by the last two digits of the year as a decimal number [00,99]. [ tm_year]
//	Y
//		Replaced by the year as a decimal number (for example, 1997). [ tm_year]
//
//	    If a minimum field width is specified, the number of characters placed into the array pointed to by s 
//      will be the number of digits and leading sign characters (if any) in the year, or the minimum field width, whichever is greater.
//
//	z
//		Replaced by the offset from UTC in the ISO 8601:2000 standard format ( +hhmm or -hhmm ), 
//      or by no characters if no timezone is determinable. For example, "-0430" means 4 hours 30 minutes behind UTC (west of Greenwich
//
//      If tm_isdst is zero, the standard time offset is used. If tm_isdst is greater than zero, 
//      the daylight savings time offset is used. If tm_isdst is negative, no characters are returned. [ tm_isdst]
//
//	Z
//		Replaced by the timezone name or abbreviation, or by no bytes if no timezone information exists. [ tm_isdst]
//	%
//		Replaced by %.
//
//	    If a conversion specification does not correspond to any of the above, the behavior is undefined.
//
//      If a struct tm broken-down time structure is created by localtime() or localtime_r(), or modified by mktime(), 
//      and the value of TZ is subsequently modified, the results of the %Z and %z strftime() conversion specifiers are undefined, 
//      when strftime() is called with such a broken-down time structure.
//
//	    If a struct tm broken-down time structure is created or modified by gmtime() or gmtime_r(),
//      it is unspecified whether the result of the %Z and %z conversion specifiers shall refer to UTC or the current local timezone,
//      when strftime() is called with such a broken-down time structure.
//
//	    Modified Conversion Specifiers
//
//	    Some conversion specifiers can be modified by the E or O modifier characters to indicate that an alternative format or 
//      specification should be used rather than the one normally used by the unmodified conversion specifier.
//      If the alternative format or specification does not exist for the current locale (see ERA in XBD LC_TIME),
//      the behavior shall be as if the unmodified conversion specification were used.
//
//	%Ec
//		Replaced by the locale's alternative appropriate date and time representation.
//	%EC
//		Replaced by the name of the base year (period) in the locale's alternative representation.
//	%Ex
//		Replaced by the locale's alternative date representation.
//	%EX
//		Replaced by the locale's alternative time representation.
//	%Ey
//		Replaced by the offset from %EC (year only) in the locale's alternative representation.
//	%EY
//		Replaced by the full alternative year representation.
//	%Od
//		Replaced by the day of the month, using the locale's alternative numeric symbols, filled as needed with leading zeros if there is any alternative symbol for zero; otherwise, with leading <space> characters.
//	%Oe
//		Replaced by the day of the month, using the locale's alternative numeric symbols, filled as needed with leading <space> characters.
//	%OH
//		Replaced by the hour (24-hour clock) using the locale's alternative numeric symbols.
//	%OI
//		Replaced by the hour (12-hour clock) using the locale's alternative numeric symbols.
//	%Om
//		Replaced by the month using the locale's alternative numeric symbols.
//	%OM
//		Replaced by the minutes using the locale's alternative numeric symbols.
//	%OS
//		Replaced by the seconds using the locale's alternative numeric symbols.
//	%Ou
//		Replaced by the weekday as a number in the locale's alternative representation (Monday=1).
//	%OU
//		Replaced by the week number of the year (Sunday as the first day of the week, rules corresponding to %U ) using the locale's alternative numeric symbols.
//	%OV
//		Replaced by the week number of the year (Monday as the first day of the week, rules corresponding to %V ) using the locale's alternative numeric symbols.
//	%Ow
//		Replaced by the number of the weekday (Sunday=0) using the locale's alternative numeric symbols.
//	%OW
//		Replaced by the week number of the year (Monday as the first day of the week) using the locale's alternative numeric symbols.
//	%Oy
//		Replaced by the year (offset from %C ) using the locale's alternative numeric symbols.
//
//	%g, %G, and %V give values according to the ISO 8601:2000 standard week-based year. In this system, weeks begin on a Monday and week 1 of the year is the week 
//      that includes January 4th, which is also the week that includes the first Thursday of the year, and is also the first week that contains at least four days in the year.
//      If the first Monday of January is the 2nd, 3rd, or 4th, the preceding days are part of the last week of the preceding year; thus, for Saturday 2nd January 1999, %G
//      is replaced by 1998 and %V is replaced by 53. If December 29th, 30th, or 31st is a Monday, it and any following days are part of week 1 of the following year.
//      Thus, for Tuesday 30th December 1997, %G is replaced by 1998 and %V is replaced by 01.
//
//      If a conversion specifier is not one of the above, the behavior is undefined.
//

//
//	Comparison between different time representations
//
//	Type	 		Base type 						Resolution 1		Epoch			earliest date			latest date
//	time_t			(32 bit) 32 bit signed integer	1 second			1.1.1970		Dec 13, 1901 20:45:52	Jan 19, 2038 03:14:07
//	time_t			(64 bit) 64 bit signed integer	1 second			1.1.1970		time didn't exist 		the sun is gone
//	struct tm		struct (36 bytes)				1 second			1.1.1900		1.1.1900				31.12.3000 23:59:59
//	SYSTEMTIME		struct (16 bytes)				1 millisecond						1.1.1601				31.12.30827 23:59:59.999
//	OLE Date/Time	double							0.5 seconds 5		30.12.1899		1.1.100					31.12.9999
//	FILETIME		64 bit unsigned integer 3		100 nanoseconds		1.1.1601		1.1.1601				~year 60055 
//


// Leap years can be divided by four, but not by 100 unless it can also be divided by 400
#define isLeapYear(year) ((!(year % 4)) ? (((!(year % 400)) && (year % 100)) ? true : false) : false)


enum EDateTimeZone {
	ETZ_LOCAL,
	ETZ_UTC,
	ETZ_DEFAULT = ETZ_LOCAL
};

enum EDateTimePrecision {
	ETP_MICRON,    // 1.0E-6 seconds
	ETP_MILLISEC,  // 1.0E-3 seconds
	ETP_SECOND,    // 1.0E-0 seconds
	ETP_DEFAULT = ETP_SECOND
};

enum EDateTimeFormat {
	EDT_SYSTEM,      // System formatted timestamp
	EDT_STANDARD,    // 2004-06-14 23:34:30 (default)
	EDT_LONG,        // 2004-06-14 23:34:30.186
	EDT_ISO8601,     // 2004-06-14T23:34:30
	EDT_RFC1123,     // Mon, 16 Sep 2019 18:19:02 GMT
	EDT_LOCAL_SHORT, // Localized timestamp
	EDT_LOCAL_LONG,  // Localized timestamp + millisec .123
	EDT_LOCAL_DATE,  // Localized date
	EDT_LOCAL_TIME,  // Localized time
	EDT_LOCAL = EDT_LOCAL_SHORT,
	EDT_DEFAULT = EDT_STANDARD,
	EDT_INVALID
};


class TDateTime;
class TTimeStamp;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PDateTime = TDateTime*;
using PTimeStamp = TTimeStamp*;

#else

typedef TDateTime* PDateTime;
typedef TTimeStamp* PTimeStamp;

#endif


int wait(TTimePart ms);
void saveWait(TTimePart ms);
int sigwait(TTimePart ms);
void sigSaveWait(TTimePart ms);
unsigned int sleep(TTimePart sec);
void saveSleep(TTimePart sec);

TTimePart now();
inline TTimePart epoch() { return (TTimePart)0; };
TTimePart localTimeToUTC(const TTimePart time);

std::string dateTimeToStr(const TTimePart time);
std::wstring dateTimeToStrW(const TTimePart time);
inline std::string dateTimeToStrA(const TTimePart time) { return dateTimeToStr(time); };

std::string dateTimeToStr(const TTimePart time, const TTimePart millisecond, const EDateTimeFormat type, const app::TLocale& locale = syslocale);
std::wstring dateTimeToStrW(const TTimePart time, const TTimePart millisecond, const EDateTimeFormat type, const app::TLocale& locale = syslocale);
inline std::string dateTimeToStrA(const TTimePart time, const TTimePart millisecond, const EDateTimeFormat type, const app::TLocale& locale = syslocale) {
	return dateTimeToStr(time, millisecond, type, locale);
}

bool isInternationalTime(const std::string& time);
bool isUniversalTime(const std::string& time);

bool strToDateTime(const std::string& time, TTimePart& seconds, TTimePart& milliseconds);
TTimePart strToDateTime(const std::string& time);

bool RFC1123ToDateTime(const std::string& time, TTimePart& seconds);
TTimePart RFC1123ToDateTime(const std::string& time);

std::string RFC1123DateTimeToStr(const TTimePart time);
std::wstring RFC1123DateTimeToStrW(const TTimePart time);
inline std::string RFC1123DateTimeToStrA(const TTimePart time) { return RFC1123DateTimeToStr(time); };

std::string ISO8601DateTimeToStr(const TTimePart time);
std::wstring ISO8601DateTimeToStrW(const TTimePart time);
inline std::string ISO8601DateTimeToStrA(const TTimePart time) { return RFC1123DateTimeToStr(time); };

std::string systemDateTimeToStr(const TTimePart time, const app::TLocale& locale = syslocale);
std::wstring systemDateTimeToStrW(const TTimePart time, const app::TLocale& locale = syslocale);
inline std::string systemDateTimeToStrA(const TTimePart time, const app::TLocale& locale = syslocale) {
	return systemDateTimeToStr(time, locale);
};

TTimePart dateTimeFromTimeParts(const TTimeStamp time, const EDateTimeZone tz);
TTimePart dateTimeFromTimeParts(const TTimeStamp time, bool isUTC = false);
TTimePart dayOfYear(TTimePart year, TTimePart month, TTimePart day);

std::string timeToHuman(TTimePart time, int limit = 2, const app::ELocale locale = app::ELocale::sysloc);
std::wstring timeToHumanW(TTimePart time, int limit = 2, const app::ELocale locale = app::ELocale::sysloc);


typedef struct CTimeValue {
	TTimePart sec;
	TTimePart msec;
	TTimePart usec;
} TTimeValue;


class TTime {
private:
	TTimeNumeric tv;
	TTimeValue value;

public:
	void clear();
	TTimeNumeric time() const { return tv; };
	TTimePart seconds() const { return value.sec; };
	TTimePart mills() const { return value.msec; };
	TTimePart microns() const { return value.usec; };

	void set(const struct timespec& value);
	void set(const struct timeval& value);
	void set(const TTimePart seconds, const TTimePart microseconds);
	void set(const TTimeNumeric value);

	TTime& operator = (const struct timespec& value);
	TTime& operator = (const struct timeval& value);
	TTime& operator = (const TTime& value);
	TTime& operator = (const TTimePart& value);
	TTime& operator = (const TDateTime& value);
	TTime& operator = (const TTimeNumeric& value);

	TTimeNumeric operator() ();

	TTime();
	virtual ~TTime() {};
};



class TTimeStamp {
private:
	static TTimePart epoch() { return util::epoch(); };

	void prime() {
		year = 0;
		month = 0;
		day = 0;
		dayOfWeek = 0;
		dayOfYear = 0;
		hour = 0;
		minute = 0;
		second = 0;
		offset = 0;
		isDST = false;
	}

public:
	TTime time;
	TTimePart year;
	TTimePart month;
	TTimePart day;
	TTimePart dayOfWeek;
	TTimePart dayOfYear;
	TTimePart hour;
	TTimePart minute;
	TTimePart second;
	TTimePart offset;
	bool isDST;
	
	void clear() {
		time.clear();
		prime();
	}
	
	void assign(const TTimeStamp &value) {
		time = value.time;
		year = value.year;
		month = value.month;
		day = value.day;
		dayOfWeek = value.dayOfWeek;
		dayOfYear = value.dayOfYear;
		hour = value.hour;
		minute = value.minute;
		second = value.second;
		offset = value.offset;
		isDST = value.isDST;
	}

	TTimeStamp& operator = (const TTimeStamp &value) {
		assign(value);
		return *this;
	}

	TTimeStamp() { prime(); };
	virtual ~TTimeStamp() {};
};


class TDateTime {
private:
	mutable EDateTimeFormat type;
	mutable EDateTimePrecision precision;

	struct tm ctm;
	struct timeval tv;

	TTimeStamp ts;
	TTimeValue cts;

	const app::TLocale* locale;

	// Local storage to store calculated values
	// --> Speedup getter() methods by returning simple string reference!
	mutable std::string stime;
	mutable std::wstring wtime;
	mutable std::string srfc1123;
	mutable std::wstring wrfc1123;
	mutable std::string siso8601;
	mutable std::wstring wiso8601;
	mutable std::string shuman;
	mutable std::wstring whuman;
	mutable std::string sutcoffs;
	mutable std::wstring wutcoffs;
	mutable int slimit;
	mutable int wlimit;
	mutable bool utcOffsOK;
	mutable TTimePart utcOffs;
	mutable bool utcTimeOK;
	mutable TTimePart utcTime;
	EDateTimeZone timezone;
	bool valid;

	TTimePart udiff();
	TTimePart mdiff();
	TTimePart sdiff();
	
	void setTimeStamp();
	void setLocalTime(const EDateTimeZone tz);
	bool getLocalTime(const TTimePart& time);
	bool getUTCTime(const TTimePart& time);

	std::string formatLocalDateTime(const char* fmt, const app::TLocale& locale) const;
	std::string formatUTCDateTime(const char* fmt) const;
	std::wstring formatLocalDateTime(const wchar_t* fmt, const app::TLocale& locale) const;
	std::wstring formatUTCDateTime(const wchar_t* fmt) const;

	std::string dateTimeToStr(const EDateTimeFormat type) const;
	std::wstring dateTimeToWideStr(const EDateTimeFormat type) const;
	
	void invalidate() const;
	void change();
	void reset();
	void prime();

public:
	void clear();
	void sync(const EDateTimeZone tz = ETZ_DEFAULT);
	void start();
	TTimePart stop(const EDateTimePrecision precision = ETP_MILLISEC);

	TTimePart now(const EDateTimeZone tz = ETZ_DEFAULT);
	TTimePart time() const;
	TTimePart utc() const;

	void setTime(const struct timespec& value, const EDateTimeZone tz = ETZ_DEFAULT);
	void setTime(const struct timeval& value, const EDateTimeZone tz = ETZ_DEFAULT);
	void setTime(const TTimePart seconds, const TTimePart microseconds = 0, const EDateTimeZone tz = ETZ_DEFAULT);
	void setTime(const std::string& time, const EDateTimeZone tz = ETZ_DEFAULT);
	void setTime(const TTimeNumeric value, const EDateTimeZone tz = ETZ_DEFAULT);
	void setTime(const TTime value, const EDateTimeZone tz = ETZ_DEFAULT);

	void setJ2K(const TTimeNumeric value, const EDateTimeZone tz = ETZ_DEFAULT);
	void setJ2K(const TTimePart seconds, const TTimePart microseconds, const EDateTimeZone tz = ETZ_DEFAULT);

	void setHFS(const TTimeNumeric value, const EDateTimeZone tz = ETZ_DEFAULT);
	void setHFS(const TTimePart seconds, const TTimePart microseconds, const EDateTimeZone tz = ETZ_DEFAULT);

	void setLDAP(const TTimeLong value, const EDateTimeZone tz = ETZ_DEFAULT);
	void setLDAP(const TTimeNumeric value, const EDateTimeZone tz = ETZ_DEFAULT);
	void setLDAP(const TTimePart seconds, const TTimePart microseconds, const EDateTimeZone tz = ETZ_DEFAULT);

	void setJulian(const TTimeNumeric value); // Julian dates are UTC by design, but internally converted to local time!
	void setJulian(const TTimeLong seconds, const TTimePart microseconds);

	TTimePart utcTimeOffset() const;
	std::string utcTimeOffsetAsString(const bool longOffsetDate = true) const;
	std::wstring utcTimeOffsetAsWideString(const bool longOffsetDate = true) const;

	bool isValid() const { return valid; };
	TTimePart year() const { return ts.year; };
	TTimePart month() const { return ts.month; };
	TTimePart day() const { return ts.day; };
	TTimePart dayOfWeek() const { return ts.dayOfWeek; };
	TTimePart getDayOfYear();
	TTimePart hour() const { return ts.hour; };
	TTimePart minute() const { return ts.minute; };
	TTimePart second() const { return ts.second; };
	TTimePart mills() const { return ts.time.mills(); };
	TTimePart microns() const { return ts.time.microns(); };
	TTimePart utcOffset() const { return utcTimeOffset(); };

	bool isDST() const { return ts.isDST; };
	EDateTimeZone getTimeZone() const { return timezone; };

	void setFormat(const EDateTimeFormat value) const;
	EDateTimeFormat getFormat() const { return type; };

	void setPrecision(const EDateTimePrecision value) const;
	EDateTimePrecision getPrecision() const { return precision; };

	void imbue(const app::TLocale& locale);
	const app::TLocale& getLocale() const { return *locale; };

	TTimePart asTime() const { return time(); };
	TTimeNumeric asNumeric() const;
	TTimeNumeric asJulian() const;

	const std::string& asHuman(int limit = 2) const;
	const std::wstring& asWideHuman(int limit = 2) const;
	const std::string& asString(const EDateTimeFormat type = EDT_INVALID) const;
	const std::wstring& asWideString(const EDateTimeFormat type = EDT_INVALID) const;
	const std::string& asRFC1123() const;
	const std::wstring& asWideRFC1123() const;
	const std::string& asISO8601() const;
	const std::wstring& asWideISO8601() const;

	void asStream(std::ostream& os) const;

	TDateTime& operator = (const struct timespec& value);
	TDateTime& operator = (const struct timeval& value);
	TDateTime& operator = (const std::string& value);
	TDateTime& operator = (const TTimePart& value);
	TDateTime& operator = (const TTimeNumeric& value);
	TDateTime& operator = (const TDateTime& value);
	TDateTime& operator = (const TTime& value);

	bool operator == (const TDateTime& value) const;
	bool operator != (const TDateTime& value) const;
	bool operator >  (const TDateTime& value) const;
	bool operator <  (const TDateTime& value) const;
	bool operator >= (const TDateTime& value) const;
	bool operator <= (const TDateTime& value) const;

	bool operator == (const TTimePart& value) const;
	bool operator != (const TTimePart& value) const;
	bool operator >  (const TTimePart& value) const;
	bool operator <  (const TTimePart& value) const;
	bool operator >= (const TTimePart& value) const;
	bool operator <= (const TTimePart& value) const;

	friend std::ostream& operator << (std::ostream& out, TDateTime& o);

	TDateTime(const EDateTimeFormat type = EDT_DEFAULT);
	TDateTime(const TTimePart value);
	virtual ~TDateTime() {};
};

class TConstDelay {
private:
	TDateTime delay;
	TTimePart sleep;

public:
	TConstDelay(const TTimePart millisec) : sleep(millisec) {
		delay.start();
	};

	virtual ~TConstDelay() {
		TTimePart duration = delay.stop(util::ETP_MILLISEC);
		TTimePart max = sleep - (sleep / 10);
		if (duration < max) {
			util::wait(sleep - duration);
		}
	};
};

} /* namespace util */

#endif /* DATETIME_H_ */
