/*
 * compare.cpp
 *
 *  Created on: 08.10.2015
 *      Author: Dirk Brinkmeier
 */

#include <string.h>
#include "compare.h"
#include "nullptr.h"
#include "templates.h"

#define USE_GLIB_STRCMP

using namespace util;

// See: https://stuff.mit.edu/afs/sipb/project/tcl80/src/tcl8.0/compat/strstr.c
static char* strnstr_s(char *string, const char *substring, size_t length)
{
	register char *a;
	register const char *b;
	register size_t i = 0;

	/*
	 * First scan quickly through the two strings looking for a
	 * single-character match.  When it's found, then compare the
	 * rest of the substring.
	 */
	b = substring;

	if (*b == 0)
		return string;

	for ( ; *string != 0 && i <= length; ++string, ++i) {

		if (*string != *b)
			continue;

		a = string;
		while (true) {
			if (*b == 0)
				return string;
			i++;
			if (i > length)
				break;
			if (*a++ != *b++)
				break;
		}
		b = substring;

	}
	return nil;
}

char* util::strnstr(char *string, const char *substring, size_t length)
{
	if (!util::assigned(string) || !util::assigned(substring))
		return nil;
	return strnstr_s(string, substring, length);
}


static char* strncasestr_s(char *string, const char *substring, size_t length)
{
	register char *a;
	register const char *b;
    register unsigned char u1, u2;
    register size_t i = 0;

	/*
	 * First scan quickly through the two strings looking for a
	 * single-character match.  When it's found, then compare the
	 * rest of the substring.
	 */
	b = substring;

	if (*b == 0)
		return string;

	for ( ; *string != 0 && i <= length; ++string, ++i) {
		u1 = (unsigned char) *string;
		u2 = (unsigned char) *b;
		if (tolower(u1) != tolower(u2))
			continue;

		a = string;
		while (true) {
			i++;
			if (*a == 0)
				return nil;
			if (*b == 0)
				return string;
			if (i > length)
				break;
			u1 = (unsigned char) *a++;
			u2 = (unsigned char) *b++;
			if (tolower(u1) != tolower(u2))
				break;
		}
		b = substring;

	}
	return nil;
}

char* util::strncasestr(char *string, const char *substring, size_t length)
{
	if (!util::assigned(string) || !util::assigned(substring))
		return nil;
	return strncasestr_s(string, substring, length);
}


static size_t strncasestr_i(const char *const string, const char *substring, size_t length)
{
	register const char *a;
	register const char *b;
	register const char *p = string;
    register unsigned char u1, u2;
    register size_t k, i = 0;

	/*
	 * First scan quickly through the two strings looking for a
	 * single-character match.  When it's found, then compare the
	 * rest of the substring.
	 */
	b = substring;

	if (*b == 0)
		return std::string::npos;

	for ( ; *p != 0 && i <= length; ++p, ++i) {
		u1 = (unsigned char) *p;
		u2 = (unsigned char) *b;
		if (tolower(u1) != tolower(u2))
			continue;

		k = i;
		a = p;
		while (true) {
			i++;
			if (*a == 0)
				return std::string::npos;
			if (*b == 0)
				return k;
			if (i > length)
				break;
			u1 = (unsigned char) *a++;
			u2 = (unsigned char) *b++;
			if (tolower(u1) != tolower(u2))
				break;
		}
		b = substring;

	}
	return std::string::npos;
}

size_t util::strncaseidx(const char *const string, const char *substring, size_t length)
{
	if (!util::assigned(string) || !util::assigned(substring))
		return std::string::npos;
	return strncasestr_i(string, substring, length);
}


/*
 * strcasecmp
 * Compares two strings, ignoring case differences.
 *
 * Results:
 *	Compares two null-terminated strings s1 and s2, returning -1, 0,
 *	or 1 if s1 is lexicographically less than, equal to, or greater
 *	than s2.
 *
 */
static int strcasecmp_s(const char *s1, const char *s2)
{
#ifdef USE_GLIB_STRCMP
	return ::strcasecmp(s1, s2);
#else
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;
	int result;
	if (p1 == p2)
		return 0;
	while ((result = tolower(*p1) - tolower(*p2++)) == 0)
		if (*p1++ == '\0')
			break;
	return result;
#endif
}


int util::strcasecmp(const char *s1, const char *s2)
{
	if (!util::assigned(s1) && !util::assigned(s2))
		return 0;
	if (!util::assigned(s1))
		return -1;
	if (!util::assigned(s2))
		return 1;
	return strcasecmp_s(s1, s2);
}

int util::strcasecmp(const std::string& s1, const std::string& s2)
{
	if (s1.empty() && s2.empty())
		return 0;
	if (s1.empty())
		return -1;
	if (s2.empty())
		return 1;
	const char *p1 = s1.c_str();
	const char *p2 = s2.c_str();
	return strcasecmp_s(p1, p2);
}

int util::strcasecmp(const std::string& s1, const char *s2)
{
	if (s1.empty() && !util::assigned(s2))
		return 0;
	if (s1.empty())
		return -1;
	if (!util::assigned(s2))
		return 1;
	const char *p1 = s1.c_str();
	return strcasecmp_s(p1, s2);
}

int util::strcasecmp(const char *s1, const std::string& s2)
{
	if (!util::assigned(s1) && s2.empty())
		return 0;
	if (!util::assigned(s1))
		return -1;
	if (s2.empty())
		return 1;
	const char *p2 = s2.c_str();
	return strcasecmp_s(s1, p2);
}


/*
 * strncasecmp
 *	Compares two strings, ignoring case differences.
 *
 * Results:
 *	Compares up to length chars of s1 and s2, returning -1, 0, or 1
 *	if s1 is lexicographically less than, equal to, or greater
 *	than s2 over those characters.
 *
 */
static int strncasecmp_s(const char *s1, const char *s2, size_t length)
{
#ifdef USE_GLIB_STRCMP
	return ::strncasecmp(s1, s2, length);
#else
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;
	int result;
	if (p1 == p2 || length == 0)
		return 0;
	while ((result = tolower(*p1) - tolower(*p2++)) == 0)
		if (*p1++ == '\0' || --length == 0)
			break;
	return result;
#endif
}


int util::strncasecmp(const char *s1, const char *s2, size_t length)
{
	if (!util::assigned(s1) && !util::assigned(s2))
		return 0;
	if (!util::assigned(s1))
		return -1;
	if (!util::assigned(s2))
		return 1;
	return strncasecmp_s(s1, s2, length);
}

int util::strncasecmp(const std::string& s1, const std::string& s2, size_t length)
{
	if (s1.empty() && s2.empty())
		return 0;
	if (s1.empty())
		return -1;
	if (s2.empty())
		return 1;
	const char *p1 = s1.c_str();
	const char *p2 = s2.c_str();
	return strncasecmp_s(p1, p2, length);
}

int util::strncasecmp(const std::string& s1, const char *s2, size_t length)
{
	if (s1.empty() && !util::assigned(s2))
		return 0;
	if (s1.empty())
		return -1;
	if (!util::assigned(s2))
		return 1;
	const char *p1 = s1.c_str();
	return strncasecmp_s(p1, s2, length);
}

int util::strncasecmp(const char *s1, const std::string& s2, size_t length)
{
	if (!util::assigned(s1) && s2.empty())
		return 0;
	if (!util::assigned(s1))
		return -1;
	if (s2.empty())
		return 1;
	const char *p2 = s2.c_str();
	return strncasecmp_s(s1, p2, length);
}


bool util::strstr(const char *haystack, const char *needle) {
	if (!util::assigned(haystack) || !util::assigned(haystack))
		return false;
	return util::assigned(::strstr(haystack, needle));
}

bool util::strstr(const std::string& haystack, const std::string& needle)
{
	if (haystack.empty() || needle.empty())
		return false;
	const char *p1 = haystack.c_str();
	const char *p2 = needle.c_str();
	return util::assigned(::strstr(p1, p2));
}

bool util::strstr(const std::string& haystack, const char *needle)
{
	if (haystack.empty())
		return false;
	if (!util::assigned(needle))
		return false;
	const char *p1 = haystack.c_str();
	return util::assigned(::strstr(p1, needle));
}

bool util::strstr(const char *haystack, const std::string& needle)
{
	if (!util::assigned(haystack))
		return false;
	if (needle.empty())
		return false;
	const char *p2 = needle.c_str();
	return util::assigned(::strstr(haystack, p2));
}


bool util::strcasestr(const char *haystack, const char *needle) {
	if (!util::assigned(haystack) || !util::assigned(haystack))
		return false;
	return util::assigned(::strcasestr(haystack, needle));
}

bool util::strcasestr(const std::string& haystack, const std::string& needle)
{
	if (haystack.empty() || needle.empty())
		return false;
	const char *p1 = haystack.c_str();
	const char *p2 = needle.c_str();
	return util::assigned(::strcasestr(p1, p2));
}

bool util::strcasestr(const std::string& haystack, const char *needle)
{
	if (haystack.empty())
		return false;
	if (!util::assigned(needle))
		return false;
	const char *p1 = haystack.c_str();
	return util::assigned(::strcasestr(p1, needle));
}

bool util::strcasestr(const char *haystack, const std::string& needle)
{
	if (!util::assigned(haystack))
		return false;
	if (needle.empty())
		return false;
	const char *p2 = needle.c_str();
	return util::assigned(::strcasestr(haystack, p2));
}


/* SQLite natural sort collation function
 *
 * Natural sort of strings, ignoring leading whitespaces.
 * Alan Swanson <swanson@ukfsn.org> 2011
 *
 * strverscasecmp() function derived from uClibc 0.9.31 strverscmp() function
 * by Hai Zaar, Codefidence Ltd <haizaar@codefidence.com>
 * which in turn derived from from glibc 2.3.2 strverscmp() function
 * by Jean-François Bignolles <bignolle@ecoledoc.ibp.fr>
 * Copyright (C) 1997, 2002 Free Software Foundation, Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* states: S_N: normal, S_I: comparing integral part, S_F: comparing
   fractional parts, S_Z: idem but with leading Zeroes only */
#define  S_N    0x0
#define  S_I    0x4
#define  S_F    0x8
#define  S_Z    0xC

/* result_type: CMP: return diff; LEN: compare using len_diff/diff */
#define  CMP    2
#define  LEN    3

/* using more efficient isdigit() */
#undef isdigit
#define isdigit(a) ((unsigned)((a) - '0') < 10)

/* Compare S1 and S2 as strings holding indices/version numbers,
   returning less than, equal to or greater than zero if S1 is less than,
   equal to or greater than S2 (for more info, see the texinfo doc).
 */
static int strnatcmp_s(const char *s1, const char *s2, bool casecmp = false)
{
	const unsigned char *p1 = (const unsigned char *) s1;
	const unsigned char *p2 = (const unsigned char *) s2;
	unsigned char c1, c2;
	int state;
	int diff;

	/* Symbol(s)    0       [1-9]   others  (padding)
       Transition   (10) 0  (01) d  (00) x  (11) -   */
	static const uint8_t next_state[] =
	{
			/* state    x    d    0    - */
			/* S_N */  S_N, S_I, S_Z, S_N,
			/* S_I */  S_N, S_I, S_I, S_I,
			/* S_F */  S_N, S_F, S_F, S_F,
			/* S_Z */  S_N, S_F, S_Z, S_Z
	};

	static const int8_t result_type[] =
	{
			/* state   x/x  x/d  x/0  x/-  d/x  d/d  d/0  d/-
                 0/x  0/d  0/0  0/-  -/x  -/d  -/0  -/- */

			/* S_N */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
					   CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
			/* S_I */  CMP, -1,  -1,  CMP, +1,  LEN, LEN, CMP,
					   +1,  LEN, LEN, CMP, CMP, CMP, CMP, CMP,
			/* S_F */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
					   CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
			/* S_Z */  CMP, +1,  +1,  CMP, -1,  CMP, CMP, CMP,
					   -1,  CMP, CMP, CMP
	};

	if (p1 == p2)
		return 0;

	c1 = *p1;
	c2 = *p2;

	/* Skip leading spaces. */
	while (isspace(c1))
		c1 = *++p1;
	while (isspace(c2))
		c2 = *++p2;

	if (casecmp) {
		c1 = tolower(*p1++);
		c2 = tolower(*p2++);
	} else {
		c1 = *p1++;
		c2 = *p2++;
	}

	/* Hint: '0' is a digit too.  */
	state = S_N | ((c1 == '0') + (isdigit (c1) != 0));

	while ((diff = c1 - c2) == 0 && c1 != '\0') {
		state = next_state[state];
		if (casecmp) {
			c1 = tolower(*p1++);
			c2 = tolower(*p2++);
		} else {
			c1 = *p1++;
			c2 = *p2++;
		}
		state |= (c1 == '0') + (isdigit (c1) != 0);
	}

	state = result_type[state << 2 | (((c2 == '0') + (isdigit (c2) != 0)))];

	switch (state) {
		case CMP:
			return diff;

		case LEN:
			while (isdigit (*p1++))
				if (!isdigit (*p2++))
					return 1;
			return isdigit (*p2) ? -1 : diff;

		default:
			return state;
	}
}


int util::strnatcmp(const char *s1, const char *s2) {
	if (!util::assigned(s1) && !util::assigned(s2))
		return 0;
	if (!util::assigned(s1))
		return -1;
	if (!util::assigned(s2))
		return 1;
	return strnatcmp_s(s1, s2, false);
}

int util::strnatcmp(const std::string& s1, const std::string& s2) {
	if (s1.empty() && s2.empty())
		return 0;
	if (s1.empty())
		return -1;
	if (s2.empty())
		return 1;
	const char *p1 = s1.c_str();
	const char *p2 = s2.c_str();
	return strnatcmp_s(p1, p2, false);
}

int util::strnatcmp(const std::string& s1, const char *s2) {
	if (s1.empty() && !util::assigned(s2))
		return 0;
	if (s1.empty())
		return -1;
	if (!util::assigned(s2))
		return 1;
	const char *p1 = s1.c_str();
	return strnatcmp_s(p1, s2, false);
}

int util::strnatcmp(const char *s1, const std::string& s2) {
	if (!util::assigned(s1) && s2.empty())
		return 0;
	if (!util::assigned(s1))
		return -1;
	if (s2.empty())
		return 1;
	const char *p2 = s2.c_str();
	return strnatcmp_s(s1, p2, false);
}


int util::strnatcasecmp(const char *s1, const char *s2) {
	if (!util::assigned(s1) && !util::assigned(s2))
		return 0;
	if (!util::assigned(s1))
		return -1;
	if (!util::assigned(s2))
		return 1;
	return strnatcmp_s(s1, s2, true);
}

int util::strnatcasecmp(const std::string& s1, const std::string& s2) {
	if (s1.empty() && s2.empty())
		return 0;
	if (s1.empty())
		return -1;
	if (s2.empty())
		return 1;
	const char *p1 = s1.c_str();
	const char *p2 = s2.c_str();
	return strnatcmp_s(p1, p2, true);
}

int util::strnatcasecmp(const std::string& s1, const char *s2) {
	if (s1.empty() && !util::assigned(s2))
		return 0;
	if (s1.empty())
		return -1;
	if (!util::assigned(s2))
		return 1;
	const char *p1 = s1.c_str();
	return strnatcmp_s(p1, s2, true);
}

int util::strnatcasecmp(const char *s1, const std::string& s2) {
	if (!util::assigned(s1) && s2.empty())
		return 0;
	if (!util::assigned(s1))
		return -1;
	if (s2.empty())
		return 1;
	const char *p2 = s2.c_str();
	return strnatcmp_s(s1, p2, true);
}


static bool strnatsort_s(const char *s1, const char *s2, bool casecmp) {
	return strnatcmp_s(s1, s2, casecmp) < 0;
}

bool util::strnatsort(const char *s1, const char *s2) {
	if (!util::assigned(s1))
		return true;
	if (!util::assigned(s2))
		return false;
	return strnatsort_s(s1, s2, false);
}

bool util::strnatsort(const std::string& s1, const std::string& s2) {
	if (s1.empty())
		return false;
	if (s2.empty())
		return true;
	const char *p1 = s1.c_str();
	const char *p2 = s2.c_str();
	return strnatsort_s(p1, p2, false);
}

bool util::strnatsort(const std::string& s1, const char *s2) {
	if (s1.empty())
		return false;
	if (!util::assigned(s2))
		return true;
	const char *p1 = s1.c_str();
	return strnatsort_s(p1, s2, false);
}

bool util::strnatsort(const char *s1, const std::string& s2) {
	if (!util::assigned(s1))
		return false;
	if (s2.empty())
		return true;
	const char *p2 = s2.c_str();
	return strnatsort_s(s1, p2, false);
}

bool util::strnatcasesort(const char *s1, const char *s2) {
	if (!util::assigned(s1))
		return true;
	if (!util::assigned(s2))
		return false;
	return strnatsort_s(s1, s2, true);
}

bool util::strnatcasesort(const std::string& s1, const std::string& s2) {
	if (s1.empty())
		return false;
	if (s2.empty())
		return true;
	const char *p1 = s1.c_str();
	const char *p2 = s2.c_str();
	return strnatsort_s(p1, p2, true);
}

bool util::strnatcasesort(const std::string& s1, const char *s2) {
	if (s1.empty())
		return false;
	if (!util::assigned(s2))
		return true;
	const char *p1 = s1.c_str();
	return strnatsort_s(p1, s2, true);
}

bool util::strnatcasesort(const char *s1, const std::string& s2) {
	if (!util::assigned(s1))
		return false;
	if (s2.empty())
		return true;
	const char *p2 = s2.c_str();
	return strnatsort_s(s1, p2, true);
}

