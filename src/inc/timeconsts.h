/*
 * timeconsts.h
 *
 *  Created on: 04.07.2020
 *      Author: dirk
 */

#ifndef INC_TIMECONSTS_H_
#define INC_TIMECONSTS_H_

#include "gcc.h"
#include "timetypes.h"

namespace util {

STATIC_CONST TTimeNumeric SECONDS_PER_DAY = 86400.0; // 60 * 60 * 24 = 86400 seconds

STATIC_CONST TTimeNumeric JULIAN_TO_UTC_OFFSET_NUM = 2440587.5;   // 12:00 Jan 1, 4713 BC (Julian Epoch at noon!) to 0:00 Jan 1, 1970 (UNIX Epoch) = 2440587.5 days
STATIC_CONST TTimeLong JULIAN_TO_UTC_OFFSET_INT = (TTimeLong)2440587 * (TTimeLong)86400 + (TTimeLong)43200;

STATIC_CONST TTimePart J2K_TO_UTC_OFFSET_INT = (TTimePart)946684800; // Seconds from UNIX Epoch 01.01.1970 to 01.01.2000 00:00:00
STATIC_CONST TTimeNumeric J2K_TO_UTC_OFFSET_NUM = (TTimeNumeric)10957.0; // Days from UNIX Epoch 01.01.1970 to 01.01.2000 00:00:00 = 946684800 seconds = 10957 days

STATIC_CONST TTimePart LDAP_TO_UTC_OFFSET_INT = (TTimePart)11644473600; // Seconds from UNIX Epoch 01.01.1970 to 01.01.1601 00:00:00 (Windows file time and LDAP timstamp offset)
STATIC_CONST TTimeNumeric LDAP_TO_UTC_OFFSET_NUM = (TTimeNumeric)134774.0; // Days from UNIX Epoch 01.01.1970 to 01.01.1601 00:00:00 = 11644473600 seconds = 134774 days
STATIC_CONST TTimeLong LDAP_JIFFIES = UINT64_C(10000000); // 100 nanoseconds per tick value = 10.000.000 ticks per second

STATIC_CONST TTimePart HFS_TO_UTC_OFFSET_INT = (TTimePart)2082844800; // Seconds from UNIX Epoch 01.01.1970 to 01.01.1904 24:00:00 (HFS+ timstamp offset)
STATIC_CONST TTimeNumeric HFS_TO_UTC_OFFSET_NUM = (TTimeNumeric)24107.0; // Days from UNIX Epoch 01.01.1970 to 01.01.1904 24:00:00 = 2082844800 seconds = 24107 days

STATIC_CONST TTimePart CENTURY_TO_UTC_OFFSET_INT = (TTimePart)2208988800; // Seconds from UNIX Epoch 01.01.1970 to 01.01.1900 00:00:00
STATIC_CONST TTimePart CENTURY_TO_UTC_TIMESTAMP_INT = CENTURY_TO_UTC_OFFSET_INT * (TTimePart)-1; // Timestamp in days from UNIX Epoch 01.01.1970 to 01.01.1900 00:00:00 = -2208988800 seconds = -25567 days
STATIC_CONST TTimePart CENTURY_TO_UTC_TIMESTAMP_CMP = CENTURY_TO_UTC_TIMESTAMP_INT + (TTimePart)660; // Timestamp in days from UNIX Epoch 01.01.1970 to 01.01.1900 00:00:00 = -2208988800 seconds = -25567 days
STATIC_CONST TTimeNumeric CENTURY_TO_UTC_OFFSET_NUM = (TTimeNumeric)25567.0; // Days from UNIX Epoch 01.01.1970 to 01.01.1900 00:00:00 = 2208988800 seconds = 25567 days
STATIC_CONST TTimeNumeric CENTURY_TO_UTC_TIMESTAMP_NUM = CENTURY_TO_UTC_OFFSET_NUM * (TTimeNumeric)-1.0; // Timestamp in days from UNIX Epoch 01.01.1970 to 01.01.1900 00:00:00 = -2208988800 seconds = -25567 days

STATIC_CONST TTimePart MILLI_JIFFIES = 1000L;      // Microseconds
STATIC_CONST TTimePart MICRO_JIFFIES = 1000000L;   // Microseconds
STATIC_CONST TTimePart NANO_JIFFIES = 1000000000L; // Nanoseconds

STATIC_CONST uint32_t NTP_TO_EPOCH_OFFSET = UINT32_C(2208988800); // 01.01.1900 = 70 years to UNIX Epoch 01.01.1970 = 2208988800 seconds

//
// ISO8601 time format strings
//
STATIC_CONST char STD_DATE_TIME_FORMAT_A[] = "%.4d-%.2d-%.2d %.2d:%.2d:%.2d";
STATIC_CONST wchar_t STD_DATE_TIME_FORMAT_W[] = L"%.4d-%.2d-%.2d %.2d:%.2d:%.2d";

STATIC_CONST char STD_LONG_DATE_TIME_FORMAT_A[] = "%.4d-%.2d-%.2d %.2d:%.2d:%.2d.%.3d";
STATIC_CONST wchar_t STD_LONG_DATE_TIME_FORMAT_W[] = L"%.4d-%.2d-%.2d %.2d:%.2d:%2d.%.3d";

STATIC_CONST char STD_LONG_LONG_DATE_TIME_FORMAT_A[] = "%.4d-%.2d-%.2d %.2d:%.2d:%.2d.%.6d";
STATIC_CONST wchar_t STD_LONG_LONG_DATE_TIME_FORMAT_W[] = L"%.4d-%.2d-%.2d %.2d:%.2d:%2d.%.6d";

STATIC_CONST char RFC_DATE_TIME_FORMAT_A[] = "%a, %d %b %Y %H:%M:%S GMT";
STATIC_CONST wchar_t RFC_DATE_TIME_FORMAT_W[] = L"%a, %d %b %Y %H:%M:%S GMT";

STATIC_CONST char ISO_GMT_DATE_TIME_FORMAT_A[] = "%Y-%m-%dT%H:%M:%SZ";
STATIC_CONST wchar_t ISO_GMT_DATE_TIME_FORMAT_W[] = L"%Y-%m-%dT%H:%M:%SZ";

STATIC_CONST char ISO_DATE_TIME_FORMAT_A[] = "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d%s";
STATIC_CONST wchar_t ISO_DATE_TIME_FORMAT_W[] = L"%.4d-%.2d-%.2dT%.2d:%.2d:%.2d%s";

STATIC_CONST char ISO_LONG_DATE_TIME_FORMAT_A[] = "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d%s";
STATIC_CONST wchar_t ISO_LONG_DATE_TIME_FORMAT_W[] = L"%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3d%s";

STATIC_CONST char ISO_LONG_LONG_DATE_TIME_FORMAT_A[] = "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.6d%s";
STATIC_CONST wchar_t ISO_LONG_LONG_DATE_TIME_FORMAT_W[] = L"%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.6d%s";

//STATIC_CONST char LOCAL_DATE_TIME_FORMAT_A[] = "%e. %B %Y %H:%M:%S";
//STATIC_CONST wchar_t LOCAL_DATE_TIME_FORMAT_W[] = L"%e. %B %Y %H:%M:%S";
STATIC_CONST char LOCAL_DATE_TIME_FORMAT_A[] = "%c";
STATIC_CONST wchar_t LOCAL_DATE_TIME_FORMAT_W[] = L"%c";

STATIC_CONST char LOCAL_DATE_FORMAT_A[] = "%x";
STATIC_CONST wchar_t LOCAL_DATE_FORMAT_W[] = L"%x";

STATIC_CONST char LOCAL_TIME_FORMAT_A[] = "%X";
STATIC_CONST wchar_t LOCAL_TIME_FORMAT_W[] = L"%X";

STATIC_CONST char SYSTEM_DATE_TIME_FORMAT_A[] = "%xT%X";
STATIC_CONST wchar_t SYSTEM_DATE_TIME_FORMAT_W[] = L"%xT%X";

} /* namespace util */

#endif /* INC_TIMECONSTS_H_ */
