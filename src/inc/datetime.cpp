/*
 * datets.cpp
 *
 *  Created on: 23.08.2014
 *      Author: Dirk Brinkmeier
 */

#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <cwchar>
#include <ctime>
#include <unistd.h>
#include "locale.h"
#include "classes.h"
#include "datetime.h"
#include "exception.h"
#include "syslocale.h"
#include "stringutils.h"
#include "timeconsts.h"
#include "templates.h"
#include "memory.h"

extern app::TLocale syslocale;

namespace util {

int wait(TTimePart ms) {
	return ::usleep(ms * 1000);
}

void saveWait(TTimePart ms) {
	// Retry if usleep() is interrupted by a signal
	TDateTime ts;
	TTimePart t = ms * 1000;
	TTimePart dt = 0;
	int retVal;
	int errVal;
	int c = 0;
	do {
		ts.start();
		retVal = ::usleep(t);
		if (EXIT_SUCCESS == retVal)
			break;
		errVal = errno;
		dt = ts.stop(ETP_MICRON);
		if ( dt > (t * 95 / 100) )
			break;
		t -= dt;
		if (t < 1000)
			break;
		c++;
	} while (retVal == -1 && errVal == EINTR && c < 10);
}


int sigwait(TTimePart ms) {
	timeval t;
	t.tv_sec = ms / 1000;
	t.tv_usec = (ms % 1000) * 1000;
	return select(0, NULL, NULL, NULL, &t);
}

void sigSaveWait(TTimePart ms) {
	TDateTime ts;
	TTimePart t = ms;
	TTimePart dt = 0;
	int retVal;
	int c = 0;
	do {
		ts.start();
		retVal = sigwait(t);
		if (EXIT_SUCCESS == retVal)
			break;
		dt = ts.stop(ETP_MILLISEC);
		if ( dt > (t * 95 / 100) )
			break;
		t -= dt;
		if (t < 5)
			break;
		c++;
	} while (c < 10);
}


unsigned int sleep(TTimePart sec) {
	return ::sleep(sec);
}

void saveSleep(TTimePart sec) {
	// Retry if sleep() interrupted by a signal
	int retVal;
	int delay = sec;

	do {
		retVal = ::sleep(delay);
		if (EXIT_SUCCESS == retVal)
			break;
		delay = retVal;
	} while (retVal > 0 && errno == EINTR);
}


TTimePart now() {
#ifdef CLOCK_REALTIME
	struct timespec ts;
	if (EXIT_SUCCESS == ::clock_gettime(CLOCK_REALTIME, &ts))
		return (TTimePart)ts.tv_sec;
#endif
	return (TTimePart)::time(nil);
}


// Convert local time to UTC (a.k.a. GMT)
TTimePart localTimeToUTC(const TTimePart time) {
	struct tm ctm;
	if (assigned(gmtime_r(&time, &ctm)))
		return mktime(&ctm);
		// return timegm(&ctm);
	return (TTimePart)0;
}


std::string dateTimeToStr (
		const TTimePart time,
		const TTimePart millisecond,
		const EDateTimeFormat type,
		const app::TLocale& locale ) {
	std::string retVal = "";
	struct tm ctm;
	struct tm* t = nil;
	const char* fmt = nil;

	// Check whether to use Greenwich or local time
	switch (type) {
		case EDT_ISO8601:
		case EDT_RFC1123:
			// Use locale independent convert function
			// t = gmtime_r(&time, &ctm);
			break;
		default:
			t = localtime_r(&time, &ctm);
			break;
	}

	if (assigned(t) || type == EDT_RFC1123 || type == EDT_ISO8601) {

		switch (type) {
			case EDT_SYSTEM:
				fmt = SYSTEM_DATE_TIME_FORMAT_A;
				break;
			case EDT_STANDARD:
				fmt = STD_DATE_TIME_FORMAT_A;
				break;
			case EDT_LONG:
				fmt = STD_LONG_DATE_TIME_FORMAT_A;
				break;
			case EDT_LOCAL_SHORT:
			case EDT_LOCAL_LONG:
				fmt = LOCAL_DATE_TIME_FORMAT_A;
				break;
			case EDT_LOCAL_DATE:
				fmt = LOCAL_DATE_FORMAT_A;
				break;
			case EDT_LOCAL_TIME:
				fmt = LOCAL_TIME_FORMAT_A;
				break;
			case EDT_ISO8601:
				fmt = ISO_DATE_TIME_FORMAT_A;
				break;
			case EDT_RFC1123:
				fmt = RFC_DATE_TIME_FORMAT_A;
				break;
		}

		switch (type) {
			case EDT_STANDARD:
				return util::cprintf(fmt,
						ctm.tm_year + 1900,
						ctm.tm_mon + 1,
						ctm.tm_mday,
						ctm.tm_hour,
						ctm.tm_min,
						ctm.tm_sec);
				break;

			case EDT_LONG:
				return util::cprintf(fmt,
						ctm.tm_year + 1900,
						ctm.tm_mon + 1,
						ctm.tm_mday,
						ctm.tm_hour,
						ctm.tm_min,
						ctm.tm_sec,
						millisecond);
				break;

			case EDT_LOCAL_SHORT:
			case EDT_LOCAL_LONG:
			case EDT_LOCAL_DATE:
			case EDT_LOCAL_TIME:
			case EDT_SYSTEM:
				{
					// Calculate time string representation via strftime
					locale_t local = (locale_t)0;
					app::TLocaleGuard<locale_t> guard(local, locale.getType());
					locale.duplicate(locale, local);
					int r, n = 32;
					util::TStringBuffer buf(n);
					while (true) {
						r = strftime_l(buf.data(), buf.size(), fmt, &ctm, local);
						if (r > 0) {
							retVal = std::string(buf.data(), r);
							break;
						} else {
							if (buf.size() > 40)
								break;
							buf.resize(buf.size() + 4, false);
						}
					}
				}
				break;

			case EDT_ISO8601:
				retVal = ISO8601DateTimeToStr(time);
				break;

			case EDT_RFC1123:
				retVal = RFC1123DateTimeToStr(time);
				break;
		}

	}
	return retVal;
}


std::wstring dateTimeToStrW (
		const TTimePart time,
		const TTimePart millisecond,
		const EDateTimeFormat type,
		const app::TLocale& locale ) {
	std::wstring retVal = L"";
	struct tm ctm;
	struct tm* t = nil;
	const wchar_t* fmt = nil;

	// Check whether to use Greenwich or local time
	switch (type) {
		case EDT_ISO8601:
		case EDT_RFC1123:
			// Use locale independent convert function
			// t = gmtime_r(&time, &ctm);
			break;
		default:
			t = localtime_r(&time, &ctm);
			break;
	}

	if (assigned(t) || type == EDT_RFC1123 || type == EDT_ISO8601) {

		switch (type) {
			case EDT_SYSTEM:
				fmt = SYSTEM_DATE_TIME_FORMAT_W;
				break;
			case EDT_STANDARD:
				fmt = STD_DATE_TIME_FORMAT_W;
				break;
			case EDT_LONG:
				fmt = STD_LONG_DATE_TIME_FORMAT_W;
				break;
			case EDT_LOCAL_SHORT:
			case EDT_LOCAL_LONG:
				fmt = LOCAL_DATE_TIME_FORMAT_W;
				break;
			case EDT_LOCAL_DATE:
				fmt = LOCAL_DATE_FORMAT_W;
				break;
			case EDT_LOCAL_TIME:
				fmt = LOCAL_TIME_FORMAT_W;
				break;
			case EDT_ISO8601:
				fmt = ISO_DATE_TIME_FORMAT_W;
				break;
			case EDT_RFC1123:
				fmt = RFC_DATE_TIME_FORMAT_W;
				break;
		}

		switch (type) {
			case EDT_STANDARD:
				return util::cprintf(fmt,
						ctm.tm_year + 1900,
						ctm.tm_mon + 1,
						ctm.tm_mday,
						ctm.tm_hour,
						ctm.tm_min,
						ctm.tm_sec);
				break;

			case EDT_LONG:
				return util::cprintf(fmt,
						ctm.tm_year + 1900,
						ctm.tm_mon + 1,
						ctm.tm_mday,
						ctm.tm_hour,
						ctm.tm_min,
						ctm.tm_sec,
						millisecond);
				break;

			case EDT_LOCAL_SHORT:
			case EDT_LOCAL_LONG:
			case EDT_LOCAL_DATE:
			case EDT_LOCAL_TIME:
			case EDT_SYSTEM:
				{
					// Calculate time string representation via strftime
					locale_t local = (locale_t)0;
					app::TLocaleGuard<locale_t> guard(local, locale.getType());
					locale.duplicate(locale, local);
					int r, n = 32;
					util::TWideBuffer buf(n);
					while (true) {
						r = wcsftime_l(buf.data(), buf.size(), fmt, &ctm, local);
						if (r > 0) {
							retVal = std::wstring(buf.data(), r);
							break;
						} else {
							if (buf.size() > 40)
								break;
							buf.resize(buf.size() + 4, false);
						}
					}
				}
				break;

			case EDT_ISO8601:
				retVal = ISO8601DateTimeToStrW(time);
				break;

			case EDT_RFC1123:
				retVal = RFC1123DateTimeToStrW(time);
				break;
		}

	}
	return retVal;
}



//
// Convert local time to ISO8601 string representation
// Milliseconds suppressed here.
//
// -123456789-12345678
// 2015-05-17 20:50:24
//
std::string dateTimeToStr(const TTimePart time) {
    std::tm ctm;
	if (assigned(localtime_r(&time, &ctm))) {
		return util::cprintf(STD_DATE_TIME_FORMAT_A,
				ctm.tm_year + 1900,
				ctm.tm_mon + 1,
				ctm.tm_mday,
				ctm.tm_hour,
				ctm.tm_min,
				ctm.tm_sec);
	}
	return util::cprintf(STD_DATE_TIME_FORMAT_A, 1900, 1, 1, 0, 0, 0);
}

std::wstring dateTimeToStrW(const TTimePart time) {
    std::tm ctm;
	if (assigned(localtime_r(&time, &ctm))) {
		return util::cprintf(STD_DATE_TIME_FORMAT_W,
				ctm.tm_year + 1900,
				ctm.tm_mon + 1,
				ctm.tm_mday,
				ctm.tm_hour,
				ctm.tm_min,
				ctm.tm_sec);
	}
	return util::cprintf(STD_DATE_TIME_FORMAT_W, 1900, 1, 1, 0, 0, 0);
}


//
// Convert ISO8601 string representation to local time
// -123456789-12345678  -123456789-123456789-12
// 2015-05-17 20:50:24  2015-05-17 20:50:24.143
//
// Microsoft SQL Server format strings:
// SELECT convert(datetime, '2016-10-23 20:44:11', 120) -- yyyy-mm-dd hh:mm:ss(24h)
// SELECT convert(datetime, '2016-10-23 20:44:11.500', 121) -- yyyy-mm-dd hh:mm:ss.mmm
// See: http://www.sqlusa.com/bestpractices/datetimeconversion/
//
bool convertDateTimeString(const std::string& time, TTimeStamp& ts) {
	if (time.empty())
		return false;

	const char* p;
	const char* s;
	char* q;
	std::string t = time;
	util::trimLeft(t);

	// Consistency check
	if (!isInternationalTime(t))
		return false;

	errno = EXIT_SUCCESS;
	s = t.c_str();

	p = s + 0;
	ts.year = strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q)
		return false;

	p = s + 5;
	ts.month = strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q)
		return false;
	if (ts.month < 1 || ts.month > 12)
		return false;

	p = s + 8;
	ts.day = strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q)
		return false;
	if (ts.day < 1 || ts.day > 31)
		return false;

	// Parse time part if existing...
	if (t.size() > 10) {
		p = s + 11;
		ts.hour = strtol(p, &q, 10);
		if (EXIT_SUCCESS != errno || p == q)
			return false;
		if (ts.hour < 0 || ts.hour > 23)
			return false;

		p = s + 14;
		ts.minute = strtol(p, &q, 10);
		if (EXIT_SUCCESS != errno || p == q)
			return false;
		if (ts.minute < 0 || ts.minute > 59)
			return false;

		p = s + 17;
		ts.second = strtol(p, &q, 10);
		if (EXIT_SUCCESS != errno || p == q)
			return false;
		if (ts.second < 0 || ts.second > 59)
			return false;
	}

	// Try to read 3 digits milliseconds, ignore errors
	if (t.size() >= 23) {
		p = s + 20;
		TTimePart t = strtol(p, &q, 10);
		if (EXIT_SUCCESS == errno || p != q) {
			if (t >= epoch() && t < (TTimePart)1000) {
				ts.millis = t;
			}
		}
	}

	return true;
}

TTimePart strToDateTime(const std::string& time) {
	TTimePart t, m;
	if (strToDateTime(time, t, m))
		return t;
	return epoch();
}

bool strToDateTime(const std::string& time, TTimePart& seconds, TTimePart& milliseconds) {
	milliseconds = 0;
	seconds = 0;
	TTimeStamp ts;

	// Convert given time to timestamp
	if (convertDateTimeString(time, ts)) {
		seconds = dateTimeFromTimeParts(ts, false);
		milliseconds = ts.millis;
		return true;
	}

	return false;
}


bool setSystemTime(const std::string& time) {
	// Time is given for UTC time zone
	// Setting system time requires superuser priviledges
	// or CAP_SYS_TIME capabilities
	struct timespec utc;
	TTimeStamp ts;

	// Get seconds from epoch for UTC time zone
	if (convertDateTimeString(time, ts)) {
		utc.tv_sec = dateTimeFromTimeParts(ts, true);
		utc.tv_nsec = ts.millis * MICRO_JIFFIES;

		// Set clock for given timestamp
		return EXIT_SUCCESS == clock_settime(CLOCK_REALTIME, &utc);
	}

	return false;
}

bool setSystemTime(const TTimePart seconds, const TTimePart millis) {
	// Time is given for UTC time zone
	// Setting system time requires superuser priviledges
	// or CAP_SYS_TIME capabilities
	struct timespec ts;
	ts.tv_sec = seconds;
	ts.tv_nsec = millis * MICRO_JIFFIES;

	// Set clock for given timestamp
	return EXIT_SUCCESS == clock_settime(CLOCK_REALTIME, &ts);
}


bool isInternationalTime(const std::string& time) {
	// Check timstamps like "2016-10-23 20:44:11", "2016-10-23T20:44:11" or "2016-10-23" (date only!)
	//                       123456789-123456789
	if (time.size() > 10) {

		// Is date and time string?
		if (time.size() < 19)
			return false;

		if (time[4] != '-' || time[7] != '-' || !(time[10] == ' ' || time[10] == 'T') || time[13] != ':' || time[16] != ':')
			return false;

	} else {

		// Is date only string?
		if (time.size() < 10)
			return false;

		if (time[4] != '-' || time[7] != '-')
			return false;

	}

	return true;
}


// Convert local time to UTC RFC1123 time string
// -123456789-123456789-123456789
// Sat, 04 Apr 2015 18:52:55 GMT
std::string RFC1123DateTimeToStr(const TTimePart time) {
	std::string retVal = "";
	struct tm ctm;
	if (assigned(gmtime_r(&time, &ctm))) {
	    int r, n = 32;
	    util::TStringBuffer buf(n);
	    while (true) {
	    	r = strftime_l(buf.data(), buf.size(), RFC_DATE_TIME_FORMAT_A, &ctm, app::en_US());
	    	if (r > 0) {
	    		retVal = std::string(buf.data(), r);
	    		break;
	    	} else {
	    		if (buf.size() > 40)
	    			break;
	    		buf.resize(buf.size() + 4);
	    	}
	    }
	}
	return retVal;
}

std::wstring RFC1123DateTimeToStrW(const TTimePart time) {
	std::wstring retVal(L"");
	struct tm ctm;
	if (assigned(gmtime_r(&time, &ctm))) {
	    int r, n = 32;
	    util::TWideBuffer buf(n);
	    while (true) {
	    	r = wcsftime_l(buf.data(), buf.size(), RFC_DATE_TIME_FORMAT_W, &ctm, app::en_US());
	    	if (r > 0) {
	    		retVal = std::wstring(buf.data(), r);
	    		break;
	    	} else {
	    		if (buf.size() > 40)
	    			break;
	    		buf.resize(buf.size() + 4);
	    	}
	    }
	}
	return retVal;
}


// Convert local time to UTC ISO-8601 time string
// -123456789-123456
// 2007-12-24T18:21Z
std::string ISO8601DateTimeToStr(const TTimePart time) {
	std::string retVal = "";
	struct tm ctm;
	if (assigned(gmtime_r(&time, &ctm))) {
	    int r, n = 32;
	    util::TStringBuffer buf(n);
	    while (true) {
	    	r = strftime_l(buf.data(), buf.size(), ISO_GMT_DATE_TIME_FORMAT_A, &ctm, app::en_US());
	    	if (r > 0) {
	    		retVal = std::string(buf.data(), r);
	    		break;
	    	} else {
	    		if (buf.size() > 30)
	    			break;
	    		buf.resize(buf.size() + 4, false);
	    	}
	    }
	}
	return retVal;
}

std::wstring ISO8601DateTimeToStrW(const TTimePart time) {
	std::wstring retVal(L"");
	struct tm ctm;
	if (assigned(gmtime_r(&time, &ctm))) {
	    int r, n = 32;
	    util::TWideBuffer buf(n);
	    while (true) {
	    	r = wcsftime_l(buf.data(), buf.size(), ISO_GMT_DATE_TIME_FORMAT_W, &ctm, app::en_US());
	    	if (r > 0) {
	    		retVal = std::wstring(buf.data(), r);
	    		break;
	    	} else {
	    		if (buf.size() > 30)
	    			break;
	    		buf.resize(buf.size() + 4);
	    	}
	    }
	}
	return retVal;
}


std::string systemDateTimeToStr(const TTimePart time, const app::TLocale& locale) {
	std::string retVal = "";
	struct tm ctm;
	if (assigned(localtime_r(&time, &ctm))) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
	    int r, n = 32;
	    util::TStringBuffer buf(n);
	    while (true) {
	    	r = strftime_l(buf.data(), buf.size(), SYSTEM_DATE_TIME_FORMAT_A, &ctm, local);
	    	if (r > 0) {
	    		retVal = std::string(buf.data(), r);
	    		break;
	    	} else {
	    		if (buf.size() > 40)
	    			break;
	    		buf.resize(buf.size() + 4, false);
	    	}
	    }
	}
	return retVal;
}

std::wstring systemDateTimeToStrW(const TTimePart time, const app::TLocale& locale) {
	std::wstring retVal(L"");
	struct tm ctm;
	if (assigned(localtime_r(&time, &ctm))) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
	    int r, n = 32;
	    util::TWideBuffer buf(n);
	    while (true) {
	    	r = wcsftime_l(buf.data(), buf.size(), SYSTEM_DATE_TIME_FORMAT_W, &ctm, local);
	    	if (r > 0) {
	    		retVal = std::wstring(buf.data(), r);
	    		break;
	    	} else {
	    		if (buf.size() > 40)
	    			break;
	    	}
	    }
	}
	return retVal;
}



const std::string monthNameLookupTable[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

//
// Convert UTC/RFC1123 time string to local time
// -123456789-123456789-12345678
// Sat, 04 Apr 2015 18:52:55 GMT
//
TTimePart RFC1123ToDateTime(const std::string& time) {
	TTimePart t;
	if (RFC1123ToDateTime(time, t))
		return t;
	return epoch();
}

bool RFC1123ToDateTime(const std::string& time, TTimePart& seconds) {
	seconds = 0;

	if (time.empty())
		return false;

	const char* p;
	const char* s;
	char* q;
	TTimeStamp ts;
	std::string t = util::trim(time);

	if (!isUniversalTime(t))
		return false;

	errno = EXIT_SUCCESS;
	s = t.c_str();

	p = s + 5;
	ts.day = strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q)
		return false;
	if (ts.day < 1 || ts.day > 31)
		return false;

	std::string sMonth = t.substr(8, 3);
	ts.month = 0;
	for (int i=0; i<12; i++) {
		if (sMonth == monthNameLookupTable[i]) {
			ts.month = util::succ(i);
			break;
		}
	}
	if (ts.month < 1)
		return false;

	p = s + 12;
	ts.year = strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q)
		return false;

	p = s + 17;
	ts.hour = strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q)
		return false;
	if (ts.hour < 0 || ts.hour > 23)
		return false;

	p = s + 20;
	ts.minute = strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q)
		return false;
	if (ts.minute < 0 || ts.minute > 59)
		return false;

	p = s + 23;
	ts.second = strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q)
		return false;
	if (ts.second < 0 || ts.second > 59)
		return false;

	seconds = dateTimeFromTimeParts(ts, true);
	return true;
}

bool isUniversalTime(const std::string& time) {
	// Consistency check
	if (time.size() != 29)
		return false;

	if (time.compare(26, 3, "GMT") != 0)
		return false;

	return true;
}


TTimePart dateTimeFromTimeParts(const TTimeStamp time, const EDateTimeZone tz) {
	return dateTimeFromTimeParts(time, ETZ_UTC == tz);
}

// Convert time parts year, month, day, hour, minute and second to local time
// Parameter UTC = true if source time is UTC (a.k.a. GMT)
TTimePart dateTimeFromTimeParts(const TTimeStamp time, bool isUTC) {
	struct tm t;
	memset(&t, 0, sizeof(tm));

	/*
	 * The original values of the tm_wday and tm_yday components of the structure
	 * are ignored, and the original values of the other components are not
	 * restricted to their normal ranges, and will be normalized if needed.  For
	 * example, October 40 is changed into November 9, a tm_hour of -1 means 1
	 * hour before midnight, tm_mday of 0 means the day preceding the current
	 * month, and tm_mon of -2 means 2 months before January of tm_year.
 	 */
	t.tm_year = time.year - 1900;
	t.tm_mon  = time.month - 1;
	t.tm_mday = time.day;
	t.tm_hour = time.hour;
	t.tm_min  = time.minute;
	t.tm_sec  = time.second;
	t.tm_wday = 0;
	t.tm_yday = 0;

	/*
	 * The value specified in the tm_isdst field informs
	 * mktime() whether or not daylight saving time (DST) is in effect for
	 * the time supplied in the tm structure: a positive value means DST is
	 * in effect; zero means that DST is not in effect; and a negative value
	 * means that mktime() should (use timezone information and system
	 * databases to) attempt to determine whether DST is in effect at the
	 * specified ts.
	 */
	t.tm_isdst = -1;

	// Increment days past 28th of Febuary for leap years
	if (isLeapYear(time.year) && (t.tm_yday > 58))
		t.tm_yday++;

	if (isUTC)
		return timegm(&t);

	return mktime(&t);
}


static const int dayOfYearLookupTable[12] = { 0, 31, 59, 90, 120, 151, 182, 212, 243, 273, 304, 334 };
static const int dayOfMonthLookupTable[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

TTimePart dayOfYear(TTimePart year, TTimePart month, TTimePart day) {
	TTimePart doy = 0;
	if (month > 0 && month <= 12) {
		int days = dayOfMonthLookupTable[util::pred(month)];
		if (isLeapYear(year) && month == 2)
			++days;
		if (day > 0 && day <= days) {
			doy =  dayOfYearLookupTable[month] + util::pred(day);
		}
	}
	return doy;
}


const std::string timeUnitsCC[]  = { "Y", "M", "D", ":", ":", "" };
const std::string timeUnitsSI[]  = { "Y", "M", "D", "h", "m", "s" };
const std::string timeUnitsEN1[] = { "year",  "month",  "day",  "hour",    "minute",  "second"   };
const std::string timeUnitsEN2[] = { "years", "months", "days", "hours",   "minutes", "seconds"  };
const std::string timeUnitsDE1[] = { "Jahr",  "Monat",  "Tag",  "Stunde",  "Minute",  "Sekunde"  };
const std::string timeUnitsDE2[] = { "Jahre", "Monate", "Tage", "Stunden", "Minuten", "Sekunden" };
const time_t timeDivisor[]       = { 60 * 60 * 24 * 356, 60 * 60 * 24 * 30, 60 * 60 * 24, 60 * 60, 60, 1 };

//
// Human readable time difference format
//
// "Human" representation : 2 hours 25 minutes 9 seconds
// "SI" representation    : 2h 25m 9s
// "CC" representation    : 2:25:09
//
// With set limit to "2"
// "Human" representation : 2 hours 25 minutes
// "SI" representation    : 2h 25m
// "CC" representation    : 2:25:00
//
// "Human" representation : 3 years 5 months 8 days 1 hour 5 minutes 30 seconds
// "CC" representation    : 3Y 5M 8D 1:05:30
//
//                          123456789-123456789-123456789-123456789-123456789-123456789-
//                                   1         2         3         4         5         60
//
std::string timeToHuman(TTimePart time, int limit, const app::ELocale locale) {
    std::string s;
	time_t d, e;
	int c = 0, t = 0;
	bool be = false;
	bool found = false;
	app::ELocale loc = locale;
	app::TLanguage language;
	app::ERegion region = app::ERegion::nreg;
	s.reserve(75);

	if (limit == 0)
		limit = 6;

	if (time < 0) {
		be = true;
		time *= -1;
		//time += now();
	}

	// Default: Read current locale from system locale object
	if (locale == app::ELocale::sysloc) {
		loc = syslocale.getLocale();
	}

	if (app::TLocale::find(loc, language)) {
		region = app::TLocale::area(language.region);
	}

	if (time > 0) {
		for(int i=0; i<6; i++) {
			if (c >= limit)
				break;
			d = timeDivisor[i];
			e = time / d;
			if (e > 0 || (region == app::ERegion::creg)) {
				time %= d;
				switch (region) {
					case app::ERegion::de:
						if (e > 1)
							s += std::to_string((size_u)e) + " " + timeUnitsDE2[i];
						else
							s += std::to_string((size_u)e) + " " + timeUnitsDE1[i];
            			if (i < 5)
            				s.append(" ");
						++c;
						break;
					case app::ERegion::en:
						if (e > 1)
							s += std::to_string((size_u)e) + " " + timeUnitsEN2[i];
						else
							s += std::to_string((size_u)e) + " " + timeUnitsEN1[i];
            			if (i < 5)
            				s.append(" ");
						++c;
						break;
					case app::ERegion::sireg:
						s += std::to_string((size_u)e) + timeUnitsSI[i];
            			if (i < 5)
            				s.append(" ");
						++c;
						break;
					case app::ERegion::creg:
                    default:
                    	// Display all values after first valid part
                    	if (!found) {
                    		if (e > 0)
                    			found = true;
                    	}
                    	if (found) {
                    		if (t == 0) {
                    			if (i < 5)
                    				s += util::cprintf("%d" + timeUnitsCC[i], e);
                    			else
                    				s += util::cprintf("0:%02.2d" + timeUnitsCC[i], e);
                    		} else {
                    			if (i < 4)
                    				s += util::cprintf("%d" + timeUnitsCC[i], e);
                    			else
                    				s += util::cprintf("%02.2d" + timeUnitsCC[i], e);
                    		}
                			if (i < 3)
                				s.append(" ");
                    		++t;
                        	++c;
                    	}
                    	break;
				}
			}
		}
		if (be) {
			switch (region) {
				case app::ERegion::de:
					s += " vor der Epoche";
					break;
				case app::ERegion::en:
					s += " before epoch time";
					break;
				case app::ERegion::sireg:
				default:
					s += " BE";
					break;
			}
		}
	} else {
		switch (region) {
			case app::ERegion::de:
				s = "0 " + timeUnitsDE2[5];
				break;
			case app::ERegion::en:
				s = "0 " + timeUnitsEN2[5];
				break;
			case app::ERegion::sireg:
				s = "0" + timeUnitsSI[5];
				break;
			case app::ERegion::creg:
			default:
				s = "0:00";
				break;
		}
	}
   	return s;
}

std::wstring timeToHumanW(TTimePart time, int limit, const app::ELocale locale) {
	std::string s = timeToHuman(time, limit, locale);
	return std::wstring(s.begin(), s.end());
}



TTime::TTime() {
	clear();
};

void TTime::clear() {
	value.sec  = epoch();
	value.msec = 0;
	value.usec = 0;
	tv = 0.0;
}


TTime& TTime::operator = (const struct timespec& value) {
	set(value);
	return *this;
}

TTime& TTime::operator = (const struct timeval& value) {
	set(value);
	return *this;
}

TTime& TTime::operator = (const TTime &value) {
	tv = value.tv;
	this->value.sec  = value.value.sec;
	this->value.msec = value.value.msec;
	this->value.usec = value.value.usec;
	return *this;
}

TTime& TTime::operator = (const TDateTime &value) {
	tv = value.asNumeric();
	this->value.sec  = value.second();
	this->value.msec = value.mills();
	this->value.usec = value.microns();
	return *this;
}

TTime& TTime::operator = (const TTimePart &value) {
	set(value);
	return *this;
}

TTime& TTime::operator = (const TTimeNumeric& value) {
	set(value);
	return *this;
}

TTimeNumeric TTime::operator() () {
	return tv;
}

void TTime::set(const TTimePart seconds, const TTimePart microseconds) {
	value.sec  = seconds;
	value.usec = microseconds;      // microseconds
	value.msec = value.usec / 1000; // milliseconds
	tv = (TTimeNumeric)value.sec + (TTimeNumeric(value.usec) / 1000000.0l);
}

void TTime::set(const TTimeNumeric value) {
	tv = value;
	this->value.sec  = (TTimePart)value;
	this->value.usec = (TTimePart)((value - (TTimeNumeric)this->value.sec) * 1000000.0l);
	this->value.msec = this->value.usec / 1000;
}

void TTime::set(const struct timespec& value) {
	this->value.sec  = (TTimePart)value.tv_sec;
	this->value.usec = (TTimePart)value.tv_nsec / 1000;
	this->value.msec = this->value.usec / 1000;
	tv = (TTimeNumeric)this->value.sec + (TTimeNumeric(this->value.usec) / 1000000.0l);
}

void TTime::set(const struct timeval& value) {
	this->value.sec  = (TTimePart)value.tv_sec;
	this->value.usec = (TTimePart)value.tv_usec;
	this->value.msec = this->value.usec / 1000;
	tv = (TTimeNumeric)this->value.sec + (TTimeNumeric(this->value.usec) / 1000000.0l);
}


TDateTime::TDateTime(const EDateTimeFormat type) : locale(&syslocale) {
	prime();
	setFormat(type);
	sync();
}

TDateTime::TDateTime(const TTimePart value) : locale(&syslocale) {
	prime();
	setTime(value);
}

void TDateTime::clear() {
	reset();
	ts.clear();
	cts.msec = ts.time.mills();
	cts.usec = ts.time.microns();
	cts.sec  = ts.time.seconds();
}

void TDateTime::reset() {
	valid = false;
	slimit = -1;
	wlimit = -1;
	srfc1123.clear();
	wrfc1123.clear();
	siso8601.clear();
	wiso8601.clear();
	shuman.clear();
	whuman.clear();
	invalidate();
}

void TDateTime::invalidate() const {
	stime.clear();
	wtime.clear();
}

void TDateTime::change() {
	utcOffsOK = false;
	utcTimeOK = false;
	utcOffs = 0;
	utcTime = 0;
	sutcoffs.clear();
	wutcoffs.clear();
}

void TDateTime::prime() {
	type = EDT_DEFAULT;
	valid = false;
	slimit = -1;
	wlimit = -1;
	timezone = ETZ_DEFAULT;
	precision = ETP_DEFAULT;
	change();
}

void TDateTime::sync(const EDateTimeZone tz) {
	if (gettimeofday(&tv, NULL) == 0) {
		ts.time.set((TTimePart)tv.tv_sec, (TTimePart)tv.tv_usec);
		setLocalTime(tz);
	} else {
		clear();
	}
}

void TDateTime::setTimeStamp() {
	ts.year      = ctm.tm_year + 1900;
	ts.month     = ctm.tm_mon + 1;
	ts.day       = ctm.tm_mday;
	ts.dayOfYear = ctm.tm_yday;
	ts.dayOfWeek = ctm.tm_wday;
	ts.hour      = ctm.tm_hour;
	ts.minute    = ctm.tm_min;
	ts.second    = ctm.tm_sec;
	ts.isDST = (ctm.tm_isdst > 0);
	ts.offset = (timezone != ETZ_UTC && ts.isDST) ? 3600 : 0;
	reset();
}

// Remark: Only valid for dates from 1971 to 2035 (?)
bool TDateTime::getLocalTime(const TTimePart& time) {
	if (localtime_r(&time, &ctm) != 0) {
		setTimeStamp();
		return true;
	}
	return false;
}

bool TDateTime::getUTCTime(const TTimePart& time) {
	if (gmtime_r(&time, &ctm) != 0) {
		setTimeStamp();
		return true;
	}
	return false;
}

TTimePart TDateTime::utcTimeOffset() const {
	if (!utcOffsOK) {
		TTimePart now = ts.time.seconds();
		TTimePart utc = util::localTimeToUTC(now);
#ifdef USE_DST_TIMESTAMP
		utcOffs = now - utc + ts.offset;
#else
		utcOffs = now - utc;
#endif
		utcOffsOK = true;
	}
	return utcOffs;
}

std::string TDateTime::utcTimeOffsetAsString(const bool longOffsetDate) const {
	//
	// Possible return values:
	//   +01:30
	//   -01:00
	//   Z for UTC or zero time offset
	//
	if (sutcoffs.empty()) {
		TTimePart utcOffs = utcTimeOffset();
		if (utcOffs != 0) {
			bool sign = (utcOffs > 0) ? false : true;
			TTimePart seconds = abs(utcOffs);
			TTimePart minutes = seconds / 60;
			TTimePart hours =  minutes / 60;
			if (hours > 0)
				minutes = minutes % (hours * 60);
			sutcoffs = (minutes > 0 || longOffsetDate) ?
				util::cprintf("%s%02.2d:%02.2d", sign ? "-" : "+", hours, minutes) :
				util::cprintf("%s%02.2d", sign ? "-" : "+", hours);
		} else {
			sutcoffs = "Z";
		}
#ifdef USE_DST_TIMESTAMP
		if (ts.isDST) {
			sutcoffs += " DST";
		}
#endif
	}
	return sutcoffs;
}


std::wstring TDateTime::utcTimeOffsetAsWideString(const bool longOffsetDate) const {
	if (wutcoffs.empty()) {
		if (sutcoffs.empty())
			utcTimeOffsetAsString(longOffsetDate);
		wutcoffs = std::wstring(sutcoffs.begin(), sutcoffs.end());
	}
	return wutcoffs;
}


void TDateTime::start() {
	sync(timezone);
}

TTimePart TDateTime::stop(const EDateTimePrecision precision) {
	switch (precision) {
		case ETP_MICRON:
			return udiff();
		case ETP_MILLISEC:
			return mdiff();
		case ETP_SECOND:
			return sdiff();
		default:
			break;
	}
	return epoch();
}

TTimePart TDateTime::udiff() {
	TTimePart usec = cts.usec;
	TTimePart sec  = cts.sec;
	sync(timezone);

	TTimePart dsec = cts.sec - sec;
	TTimePart dusec = cts.usec - usec;
	TTimePart retVal = (sec == cts.sec) ? cts.usec - usec : (dsec * 1000000) + (cts.usec - usec);
	setTime(dsec, dusec);

	return retVal;
}

TTimePart TDateTime::mdiff() {
	TTimePart usec = cts.usec;
	TTimePart msec = cts.msec;
	TTimePart sec  = cts.sec;
	sync(timezone);

	TTimePart dsec = cts.sec - sec;
	TTimePart dusec = cts.usec - usec;
	TTimePart retVal = (sec == cts.sec) ? cts.msec - msec : (dsec * 1000) + (cts.msec - msec);
	setTime(dsec, dusec);

	return retVal;
}

TTimePart TDateTime::sdiff() {
	TTimePart usec = cts.usec;
	TTimePart sec = cts.sec;
	sync(timezone);

	TTimePart dsec = cts.sec - sec;
	TTimePart dusec = cts.usec - usec;
	setTime(dsec, dusec);

	return dsec;
}


TTimePart TDateTime::now(const EDateTimeZone tz) {
	sync(timezone);
	return time();
};

TTimePart TDateTime::time() const {
	return ts.time.seconds();
};

TTimePart TDateTime::utc() const {
	if (!utcTimeOK) {
		utcTime = localTimeToUTC(time());
		utcTimeOK = true;
	}
	return utcTime;
};


void TDateTime::setFormat(const EDateTimeFormat value) const {
	if (type != value) {
		invalidate();
	}
	type = value;
	if (type == EDT_LONG) {
		// Long type implies millisecond output
		if (precision == ETP_SECOND) {
			setPrecision(ETP_MILLISEC);
		}
	}
}

void TDateTime::setPrecision(const EDateTimePrecision value) const {
	if (precision != value) {
		invalidate();
	}
	precision = value;
}


void TDateTime::setLocalTime(const EDateTimeZone tz) {
	cts.msec = ts.time.mills();
	cts.usec = ts.time.microns();
	cts.sec  = ts.time.seconds();

	// Timezone changed?
	if (timezone != tz) {
		timezone = tz;
		change();
	}

	// Set "local" time depending on UTC or local
	errno = EXIT_SUCCESS;
	switch (timezone) {
		case ETZ_UTC:
			valid = getUTCTime(ts.time.seconds());
			if (valid) {
				utcOffs = 0;
				utcTime = time();
				utcTimeOK = true;
				utcOffsOK = true;
			}
			break;
		case ETZ_LOCAL:
		default:
			valid = getLocalTime(ts.time.seconds());
			break;
	}

	if (!valid)
		util::sys_error("TDateTime::setTime() failed: Invalid time stamp (" + std::to_string((size_s)ts.time.seconds()) + ")");
}


void TDateTime::setTime(const struct timespec& value, const EDateTimeZone tz) {
	clear();
	ts.time = value;
	setLocalTime(tz);
}

void TDateTime::setTime(const struct timeval& value, const EDateTimeZone tz) {
	clear();
	ts.time = value;
	setLocalTime(tz);
}

void TDateTime::setTime(const TTimePart seconds, const TTimePart microseconds, const EDateTimeZone tz)
{
	clear();
	ts.time.set(seconds, microseconds);
	setLocalTime(tz);
}

void TDateTime::setTime(const TTimeNumeric value, const EDateTimeZone tz) {
	clear();
	ts.time = value;
	setLocalTime(tz);
}

void TDateTime::setTime(const TTime value, const EDateTimeZone tz) {
	clear();
	ts.time = value;
	setLocalTime(tz);
}

void TDateTime::setTime(const std::string& time, const EDateTimeZone tz) {
	bool retVal;
	TTimePart t, m = 0;
	EDateTimeZone ctz = tz;
	retVal = strToDateTime(time, t, m);
	if (!retVal) {
		// Convert UTC time format
		retVal = RFC1123ToDateTime(time, t);
		if (retVal)
			ctz = ETZ_UTC;
	}
	if (retVal) {
		setTime(t, m * 1000, ctz);
	}
}

void TDateTime::setJ2K(const TTimeNumeric value, const EDateTimeZone tz) {
	TTimeNumeric t = (value + J2K_TO_UTC_OFFSET_NUM) * SECONDS_PER_DAY;
	if (ETZ_LOCAL == tz) {
		utcTimeOffset();
		if (utcOffs != 0)
			t -= (TTimeNumeric)utcOffs;
	}
	setTime(t, tz);
}

void TDateTime::setJ2K(const TTimePart seconds, const TTimePart microseconds, const EDateTimeZone tz) {
	TTimePart t = seconds + J2K_TO_UTC_OFFSET_INT;
	if (ETZ_LOCAL == tz) {
		utcTimeOffset();
		if (utcOffs != 0)
			t -= utcOffs;
	}
	setTime(t, microseconds, tz);
}

void TDateTime::setHFS(const TTimeNumeric value, const EDateTimeZone tz) {
	TTimeNumeric t = (value + HFS_TO_UTC_OFFSET_NUM) * SECONDS_PER_DAY;
	if (ETZ_LOCAL == tz) {
		utcTimeOffset();
		if (utcOffs != 0)
			t -= (TTimeNumeric)utcOffs;
	}
	setTime(t, tz);
}

void TDateTime::setHFS(const TTimePart seconds, const TTimePart microseconds, const EDateTimeZone tz) {
	TTimePart t = seconds + HFS_TO_UTC_OFFSET_INT;
	if (ETZ_LOCAL == tz) {
		utcTimeOffset();
		if (utcOffs != 0)
			t -= utcOffs;
	}
	setTime(t, microseconds, tz);
}

void TDateTime::setLDAP(const TTimeLong value, const EDateTimeZone tz) {
	TTimePart seconds = value / LDAP_JIFFIES;
	TTimePart microseconds = value % LDAP_JIFFIES / (TTimeLong)10;
	setLDAP(seconds, microseconds, tz);
}

void TDateTime::setLDAP(const TTimeNumeric value, const EDateTimeZone tz) {
	TTimeNumeric t = (value + LDAP_TO_UTC_OFFSET_NUM) * SECONDS_PER_DAY;
	if (ETZ_LOCAL == tz) {
		utcTimeOffset();
		if (utcOffs != 0)
			t -= (TTimeNumeric)utcOffs;
	}
	setTime(t, tz);
}

void TDateTime::setLDAP(const TTimePart seconds, const TTimePart microseconds, const EDateTimeZone tz) {
	TTimePart t = seconds + LDAP_TO_UTC_OFFSET_INT;
	if (ETZ_LOCAL == tz) {
		utcTimeOffset();
		if (utcOffs != 0)
			t -= utcOffs;
	}
	setTime(t, microseconds, tz);
}

void TDateTime::setJulian(const TTimeNumeric value) {
	TTimeNumeric t = (value - JULIAN_TO_UTC_OFFSET_NUM) * SECONDS_PER_DAY;
	utcTimeOffset();
	if (utcOffs != 0) // Julian time is UTC --> Add offset to UTC for local time
		t += (TTimeNumeric)utcOffs;
	setTime(t, ETZ_LOCAL);
}

void TDateTime::setJulian(const TTimeLong seconds, const TTimePart microseconds) {
	TTimePart t = (TTimePart)(seconds - JULIAN_TO_UTC_OFFSET_INT);
	utcTimeOffset();
	if (utcOffs != 0) // Julian time is UTC --> Add offset to UTC for local time
		t += utcOffs;
	setTime(t, microseconds, ETZ_LOCAL);
}

TDateTime& TDateTime::operator = (const struct timespec& value) {
	setTime(value);
	return *this;
}

TDateTime& TDateTime::operator = (const struct timeval& value) {
	setTime(value);
	return *this;
}

TDateTime& TDateTime::operator = (const std::string& value) {
	setTime(value);
	return *this;
}

TDateTime& TDateTime::operator = (const TDateTime &value)
{
	setFormat(value.type);
	ts.time = value.ts.time;
	timezone = value.getTimeZone();
	setLocalTime(timezone);
	return *this;
}

TDateTime& TDateTime::operator = (const TTime &value)
{
	setTime(value);
	return *this;
}

TDateTime& TDateTime::operator = (const TTimePart& value) {
	setTime(value, 0);
	return *this;
}

TDateTime& TDateTime::operator = (const TTimeNumeric& value) {
	setTime(value);
	return *this;
}


bool TDateTime::operator == (const TDateTime& value) const {
	return (time() == value.time() && microns() == value.microns());
}

bool TDateTime::operator != (const TDateTime& value) const {
	if (time() != value.time())
		return true;
	if (time() == value.time())
		if (microns() != value.microns())
			return true;
	return false;
}

bool TDateTime::operator > (const TDateTime& value) const {
	if (time() > value.time())
		return true;
	if (time() == value.time())
		if (microns() > value.microns())
			return true;
	return false;
}

bool TDateTime::operator < (const TDateTime& value) const {
	if (time() < value.time())
		return true;
	if (time() == value.time())
		if (microns() < value.microns())
			return true;
	return false;
}

bool TDateTime::operator >= (const TDateTime& value) const {
	if (time() > value.time())
		return true;
	if (time() == value.time() && microns() == value.microns())
		return true;
	if (time() == value.time())
		if (microns() > value.microns())
			return true;
	return false;
}

bool TDateTime::operator <= (const TDateTime& value) const {
	if (time() < value.time())
		return true;
	if (time() == value.time() && microns() == value.microns())
		return true;
	if (time() == value.time())
		if (microns() < value.microns())
			return true;
	return false;
}


bool TDateTime::operator == (const TTimePart& value) const {
	return time() == value;
}

bool TDateTime::operator != (const TTimePart& value) const {
	return time() != value;
}

bool TDateTime::operator > (const TTimePart& value) const {
	return time() > value;
}

bool TDateTime::operator < (const TTimePart& value) const {
	return time() < value;
}

bool TDateTime::operator >= (const TTimePart& value) const {
	return time() >= value;
}

bool TDateTime::operator <= (const TTimePart& value) const {
	return time() <= value;
}


TTimeNumeric TDateTime::asNumeric() const {
	return ts.time.time();
};

TTimeNumeric TDateTime::asJulian() const {
	// See https://www.giss.nasa.gov/tools/mars24/help/algorithm.html
	utcTimeOffset();
	TTimeNumeric t = asNumeric();
	if (utcOffs != 0)
		t -= (TTimeNumeric)utcOffs;
	return JULIAN_TO_UTC_OFFSET_NUM + (t / SECONDS_PER_DAY);
};

const std::string& TDateTime::asHuman(int limit) const {
	if (slimit != limit || shuman.empty()) {
		slimit = limit;
		shuman = timeToHuman(time(), limit, locale->getLocale());
	}
	return shuman;
}

const std::wstring& TDateTime::asWideHuman(int limit) const {
	if (wlimit != limit || shuman.empty()) {
		wlimit = limit;
		whuman = timeToHumanW(time(), limit, locale->getLocale());
	}
	return whuman;
}

TTimePart TDateTime::getDayOfYear() {
	if (ts.dayOfYear > 0 && ts.dayOfYear < 367)
		ts.dayOfYear = dayOfYear(ts.year, ts.month, ts.day);
	return ts.dayOfYear;
}


std::string TDateTime::formatLocalDateTime(const char* fmt, const app::TLocale& locale) const {
	// Calculate local time string representation via strftime
	std::string retVal;
	if (util::assigned(fmt)) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		int r, n = 32;
		util::TStringBuffer buf(n);
		while (true) {
			r = strftime_l(buf.data(), buf.size(), fmt, &ctm, local);
			if (r > 0) {
				retVal = std::string(buf.data(), r);
				break;
			} else {
				if (buf.size() > 40)
					break;
				buf.resize(buf.size() + 4, false);
			}
		}
	}
	return retVal;
}

std::string TDateTime::formatUTCDateTime(const char* fmt) const {
	// Calculate UTC time string representation via strftime
	std::string retVal;
	if (util::assigned(fmt)) {
		if (timezone == ETZ_LOCAL) {
			struct tm gtm;
			TTimePart t = time();
			if (assigned(gmtime_r(&t, &gtm))) {
				int r, n = 32;
				util::TStringBuffer buf(n);
				while (true) {
					r = strftime_l(buf.data(), buf.size(), fmt, &gtm, app::en_US());
					if (r > 0) {
						retVal = std::string(buf.data(), r);
						break;
					} else {
						if (buf.size() > 40)
							break;
						buf.resize(buf.size() + 4, false);
					}
				}
			}
		} else {
			retVal = formatLocalDateTime(fmt, app::en_US);
		}
	}
	return retVal;
}

std::wstring TDateTime::formatLocalDateTime(const wchar_t* fmt, const app::TLocale& locale) const {
	// Calculate local time string representation via strftime
	std::wstring retVal;
	if (util::assigned(fmt)) {
		locale_t local = (locale_t)0;
		app::TLocaleGuard<locale_t> guard(local, locale.getType());
		locale.duplicate(locale, local);
		int r, n = 32;
		util::TWideBuffer buf(n);
		while (true) {
			r = wcsftime_l(buf.data(), buf.size(), fmt, &ctm, local);
			if (r > 0) {
				retVal = std::wstring(buf.data(), r);
				break;
			} else {
				if (buf.size() > 40)
					break;
				buf.resize(buf.size() + 4, false);
			}
		}
	}
	return retVal;
}

std::wstring TDateTime::formatUTCDateTime(const wchar_t* fmt) const {
	// Calculate UTC time string representation via strftime
	std::wstring retVal;
	if (util::assigned(fmt)) {
		if (timezone == ETZ_LOCAL) {
			struct tm gtm;
			TTimePart t = time();
			if (assigned(gmtime_r(&t, &gtm))) {
				int r, n = 32;
				util::TWideBuffer buf(n);
				while (true) {
					r = wcsftime_l(buf.data(), buf.size(), fmt, &gtm, app::en_US());
					if (r > 0) {
						retVal = std::wstring(buf.data(), r);
						break;
					} else {
						if (buf.size() > 40)
							break;
						buf.resize(buf.size() + 4, false);
					}
				}
			}
		} else {
			retVal = formatLocalDateTime(fmt, app::en_US);
		}
	}
	return retVal;
}

std::string TDateTime::dateTimeToStr(const EDateTimeFormat type) const {
	std::string retVal = "";
	const char* fmt = nil;
	EDateTimeFormat format = (app::ELocale::siloc == locale->getLocale()) ? EDT_STANDARD : type;

	switch (format) {
		case EDT_SYSTEM:
			fmt = SYSTEM_DATE_TIME_FORMAT_A;
			break;

		case EDT_STANDARD:
			switch (precision) {
				case ETP_MICRON:
					fmt = STD_LONG_LONG_DATE_TIME_FORMAT_A;
					break;
				case ETP_MILLISEC:
					fmt = STD_LONG_DATE_TIME_FORMAT_A;
					break;
				case ETP_SECOND:
				default:
					fmt = STD_DATE_TIME_FORMAT_A;
					break;
			}
			break;

		case EDT_LONG:
			switch (precision) {
				case ETP_MICRON:
					fmt = STD_LONG_LONG_DATE_TIME_FORMAT_A;
					break;
				case ETP_MILLISEC:
				case ETP_SECOND:
				default:
					fmt = STD_LONG_DATE_TIME_FORMAT_A;
					break;
			}
			break;

		case EDT_LOCAL_DATE:
			fmt = LOCAL_DATE_FORMAT_A;
			break;

		case EDT_LOCAL_TIME:
			fmt = LOCAL_TIME_FORMAT_A;
			break;

		case EDT_LOCAL_SHORT:
			fmt = util::assigned(locale) ? locale->getTimeFormat() : LOCAL_DATE_TIME_FORMAT_A;
			break;

		case EDT_LOCAL_LONG:
			fmt = LOCAL_DATE_TIME_FORMAT_A;
			break;

		case EDT_ISO8601:
			switch (precision) {
				case ETP_MICRON:
					fmt = ISO_LONG_LONG_DATE_TIME_FORMAT_A;
					break;
				case ETP_MILLISEC:
					fmt = ISO_LONG_DATE_TIME_FORMAT_A;
					break;
				case ETP_SECOND:
				default:
					fmt = ISO_DATE_TIME_FORMAT_A;
					break;
			}
			break;

		case EDT_RFC1123:
			fmt = RFC_DATE_TIME_FORMAT_A;
			break;
	}

	switch (format) {
		case EDT_STANDARD:
			if (valid) {
				switch (precision) {
					case ETP_MICRON:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec,
											microns());
						break;
					case ETP_MILLISEC:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec,
											mills());
						break;
					case ETP_SECOND:
					default:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec);
						break;
				}
			} else {
				retVal = util::cprintf(STD_DATE_TIME_FORMAT_A, 1900, 1, 1, 0, 0, 0);
			}
			break;

		case EDT_LONG:
			if (valid) {
				switch (precision) {
					case ETP_MICRON:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec,
											microns());
						break;
					case ETP_MILLISEC:
					case ETP_SECOND:
					default:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec,
											mills());
						break;
				}
			} else {
				retVal = util::cprintf(STD_LONG_DATE_TIME_FORMAT_A, 1900, 1, 1, 0, 0, 0, 0);
			}
			break;

		case EDT_ISO8601:
			if (sutcoffs.empty())
				utcTimeOffsetAsString();
			if (!sutcoffs.empty()) {
				if (valid) {
					// -123456789-123456789
					// 2007-12-24T18:21:33Z
					// 2007-12-24T18:21:33,167534Z --> Add fraction of a second in microseconds (6 digits)
					// Use "." as fraction separator, allowed but not 100% ISO conform
					switch (precision) {
						case ETP_MICRON:
							retVal = util::cprintf(fmt,
												ctm.tm_year + 1900,
												ctm.tm_mon + 1,
												ctm.tm_mday,
												ctm.tm_hour,
												ctm.tm_min,
												ctm.tm_sec,
												microns(),
												sutcoffs.c_str());
							break;
						case ETP_MILLISEC:
							retVal = util::cprintf(fmt,
												ctm.tm_year + 1900,
												ctm.tm_mon + 1,
												ctm.tm_mday,
												ctm.tm_hour,
												ctm.tm_min,
												ctm.tm_sec,
												mills(),
												sutcoffs.c_str());
							break;
						case ETP_SECOND:
						default:
							retVal = util::cprintf(fmt,
												ctm.tm_year + 1900,
												ctm.tm_mon + 1,
												ctm.tm_mday,
												ctm.tm_hour,
												ctm.tm_min,
												ctm.tm_sec,
												sutcoffs.c_str());
							break;
					}
				} else {
					retVal = util::cprintf(ISO_DATE_TIME_FORMAT_A, 1900, 1, 1, 0, 0, 0, sutcoffs.c_str());
				}
			} else {
				retVal = util::cprintf(ISO_DATE_TIME_FORMAT_A, 1900, 1, 1, 0, 0, 0, "Z");
			}
			break;

		case EDT_LOCAL_SHORT:
		case EDT_LOCAL_LONG:
		case EDT_LOCAL_DATE:
		case EDT_LOCAL_TIME:
		case EDT_SYSTEM:
			if (util::assigned(locale) && valid) {
				retVal = formatLocalDateTime(fmt, *locale);
			}
			break;

		case EDT_RFC1123:
			if (valid) {
				retVal = formatUTCDateTime(fmt);
				if (retVal.size() > 25) {
					// -123456789-123456789-123456789
					// Sat, 04 Apr 2015 18:52:55 GMT
					// Sat, 04 Apr 2015 18:52:55.267 GMT --> Add milliseconds or microseconds, non RFC conform!
					switch (precision) {
						case ETP_MICRON:
							retVal.insert(25, util::cprintf(".%.6d", microns()));
							break;
						case ETP_MILLISEC:
							retVal.insert(25, util::cprintf(".%.3d", mills()));
							break;
						default:
							break;
					}
				}
			}	
			break;

	}

	if (retVal.empty())
		retVal = util::cprintf(STD_DATE_TIME_FORMAT_A, 1900, 1, 1, 0, 0, 0);

	return retVal;
}


std::wstring TDateTime::dateTimeToWideStr(const EDateTimeFormat type) const {
	std::wstring retVal = L"";
	const wchar_t* fmt = nil;
	EDateTimeFormat format = (app::ELocale::siloc == locale->getLocale()) ? EDT_STANDARD : type;

	switch (format) {
		case EDT_SYSTEM:
			fmt = SYSTEM_DATE_TIME_FORMAT_W;
			break;
		case EDT_STANDARD:
			switch (precision) {
				case ETP_MICRON:
					fmt = STD_LONG_LONG_DATE_TIME_FORMAT_W;
					break;
				case ETP_MILLISEC:
					fmt = STD_LONG_DATE_TIME_FORMAT_W;
					break;
				case ETP_SECOND:
				default:
					fmt = STD_DATE_TIME_FORMAT_W;
					break;
			}
			break;

		case EDT_LONG:
			switch (precision) {
				case ETP_MICRON:
					fmt = STD_LONG_LONG_DATE_TIME_FORMAT_W;
					break;
				case ETP_MILLISEC:
				case ETP_SECOND:
				default:
					fmt = STD_LONG_DATE_TIME_FORMAT_W;
					break;
			}
			break;

		case EDT_LOCAL_DATE:
			fmt = LOCAL_DATE_FORMAT_W;
			break;

		case EDT_LOCAL_TIME:
			fmt = LOCAL_TIME_FORMAT_W;
			break;

		case EDT_LOCAL_SHORT:
			fmt = util::assigned(locale) ? locale->getWideTimeFormat() : LOCAL_DATE_TIME_FORMAT_W;
			break;

		case EDT_LOCAL_LONG:
			fmt = LOCAL_DATE_TIME_FORMAT_W;
			break;

		case EDT_ISO8601:
			switch (precision) {
				case ETP_MICRON:
					fmt = ISO_LONG_LONG_DATE_TIME_FORMAT_W;
					break;
				case ETP_MILLISEC:
					fmt = ISO_LONG_DATE_TIME_FORMAT_W;
					break;
				case ETP_SECOND:
				default:
					fmt = ISO_DATE_TIME_FORMAT_W;
					break;
			}
			break;

		case EDT_RFC1123:
			fmt = RFC_DATE_TIME_FORMAT_W;
			break;
	}

	switch (format) {
		case EDT_STANDARD:
			if (valid) {
				switch (precision) {
					case ETP_MICRON:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec,
											microns());
						break;
					case ETP_MILLISEC:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec,
											mills());
						break;
					case ETP_SECOND:
					default:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec);
						break;
				}
			} else {
				retVal = util::cprintf(STD_DATE_TIME_FORMAT_W, 1900, 1, 1, 0, 0, 0);
			}
			break;

		case EDT_LONG:
			if (valid) {
				switch (precision) {
					case ETP_MICRON:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec,
											microns());
						break;
					case ETP_MILLISEC:
					case ETP_SECOND:
					default:
						retVal = util::cprintf(fmt,
											ctm.tm_year + 1900,
											ctm.tm_mon + 1,
											ctm.tm_mday,
											ctm.tm_hour,
											ctm.tm_min,
											ctm.tm_sec,
											mills());
						break;
				}
			} else {
				retVal = util::cprintf(STD_LONG_DATE_TIME_FORMAT_W, 1900, 1, 1, 0, 0, 0, 0);
			}
			break;

		case EDT_ISO8601:
			if (wutcoffs.empty())
				utcTimeOffsetAsString();
			if (!wutcoffs.empty()) {
				if (valid) {
					// -123456789-123456789
					// 2007-12-24T18:21:33Z
					// 2007-12-24T18:21:33,167534Z --> Add fraction of a second in microseconds (6 digits)
					// Use "." as fraction separator, allowed but not 100% ISO conform
					switch (precision) {
						case ETP_MICRON:
							retVal = util::cprintf(fmt,
												ctm.tm_year + 1900,
												ctm.tm_mon + 1,
												ctm.tm_mday,
												ctm.tm_hour,
												ctm.tm_min,
												ctm.tm_sec,
												microns(),
												wutcoffs.c_str());
							break;
						case ETP_MILLISEC:
							retVal = util::cprintf(fmt,
												ctm.tm_year + 1900,
												ctm.tm_mon + 1,
												ctm.tm_mday,
												ctm.tm_hour,
												ctm.tm_min,
												ctm.tm_sec,
												mills(),
												wutcoffs.c_str());
							break;
						case ETP_SECOND:
						default:
							retVal = util::cprintf(fmt,
												ctm.tm_year + 1900,
												ctm.tm_mon + 1,
												ctm.tm_mday,
												ctm.tm_hour,
												ctm.tm_min,
												ctm.tm_sec,
												wutcoffs.c_str());
							break;
					}
				} else {
					retVal = util::cprintf(ISO_DATE_TIME_FORMAT_W, 1900, 1, 1, 0, 0, 0, wutcoffs.c_str());
				}
			} else {
				retVal = util::cprintf(ISO_DATE_TIME_FORMAT_W, 1900, 1, 1, 0, 0, 0, "Z");
			}
			break;

		case EDT_LOCAL_SHORT:
		case EDT_LOCAL_LONG:
		case EDT_LOCAL_DATE:
		case EDT_LOCAL_TIME:
		case EDT_SYSTEM:
			if (util::assigned(locale) && valid) {
				retVal = formatLocalDateTime(fmt, *locale);
			}
			break;

		case EDT_RFC1123:
			if (valid) {
				retVal = formatUTCDateTime(fmt);
				if (retVal.size() > 25) {
					// -123456789-123456789-123456789
					// Sat, 04 Apr 2015 18:52:55 GMT
					// Sat, 04 Apr 2015 18:52:55.267 GMT --> Add milliseconds or microseconds, non RFC conform!
					switch (precision) {
						case ETP_MICRON:
							retVal.insert(25, util::cprintf(L".%.6d", microns()));
							break;
						case ETP_MILLISEC:
							retVal.insert(25, util::cprintf(L".%.3d", mills()));
							break;
						default:
							break;
					}
				}
			}	
			break;

	}

	if (retVal.empty())
		retVal = util::cprintf(STD_DATE_TIME_FORMAT_W, 1900, 1, 1, 0, 0, 0);

	return retVal;
}


const std::string& TDateTime::asString(const EDateTimeFormat type) const {
	if (type != EDT_INVALID) {
		setFormat(type);
	}
	if (stime.empty()) {
		stime = dateTimeToStr(this->type);
	}
	return stime;
}

const std::wstring& TDateTime::asWideString(const EDateTimeFormat type) const {
	if (type != EDT_INVALID) {
		setFormat(type);
	}
	if (wtime.empty())
		wtime = dateTimeToWideStr(this->type);
	return wtime;
}

const std::string& TDateTime::asRFC1123() const {
	if (srfc1123.empty()) {
		srfc1123 = dateTimeToStr(EDT_RFC1123);
	}
	return srfc1123;
}

const std::wstring& TDateTime::asWideRFC1123() const {
	if (wrfc1123.empty()) {
		wrfc1123 = dateTimeToWideStr(EDT_RFC1123);
	}
	return wrfc1123;
}


const std::string& TDateTime::asISO8601() const {
	if (siso8601.empty())
		siso8601 = dateTimeToStr(EDT_ISO8601);
	return siso8601;
}

const std::wstring& TDateTime::asWideISO8601() const {
	if (wiso8601.empty())
		wiso8601 = dateTimeToWideStr(EDT_ISO8601);
	return wiso8601;
}


void TDateTime::asStream(std::ostream& os) const {
	os << asString();
	return;
}

void TDateTime::imbue(const app::TLocale& locale) {
	this->locale = &locale;
	invalidate();
};


std::ostream& operator << (std::ostream& out, TDateTime& o) {
	o.sync(o.getTimeZone());
	o.asStream(out);
	return out;
}


} /* namespace util */
