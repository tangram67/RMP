/*
 * compare.h
 *
 *  Created on: 08.10.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef COMPARE_H_
#define COMPARE_H_

#include <string>

namespace util {

char* strnstr(char *string, const char *substring, size_t length);
char* strncasestr(char *string, const char *substring, size_t length);
size_t strncaseidx(const char *const string, const char *substring, size_t length);

int strcasecmp(const char *s1, const char *s2);
int strcasecmp(const std::string& s1, const std::string& s2);
int strcasecmp(const std::string& s1, const char *s2);
int strcasecmp(const char *s1, const std::string& s2);

int strncasecmp(const char *s1, const char *s2, size_t length);
int strncasecmp(const std::string& s1, const std::string& s2, size_t length);
int strncasecmp(const std::string& s1, const char *s2, size_t length);
int strncasecmp(const char *s1, const std::string& s2, size_t length);

bool strstr(const char *haystack, const char *needle);
bool strstr(const std::string& haystack, const std::string& needle);
bool strstr(const std::string& haystack, const char *needle);
bool strstr(const char *haystack, const std::string& needle);

bool strcasestr(const char *haystack, const char *needle);
bool strcasestr(const std::string& haystack, const std::string& needle);
bool strcasestr(const std::string& haystack, const char *needle);
bool strcasestr(const char *haystack, const std::string& needle);

int strnatcmp(const char *s1, const char *s2);
int strnatcmp(const std::string& s1, const std::string& s2);
int strnatcmp(const std::string& s1, const char *s2);
int strnatcmp(const char *s1, const std::string& s2);

int strnatcasecmp(const char *s1, const char *s2);
int strnatcasecmp(const std::string& s1, const std::string& s2);
int strnatcasecmp(const std::string& s1, const char *s2);
int strnatcasecmp(const char *s1, const std::string& s2);

bool strnatsort(const char *s1, const char *s2);
bool strnatsort(const std::string& s1, const std::string& s2);
bool strnatsort(const std::string& s1, const char *s2);
bool strnatsort(const char *s1, const std::string& s2);

bool strnatcasesort(const char *s1, const char *s2);
bool strnatcasesort(const std::string& s1, const std::string& s2);
bool strnatcasesort(const std::string& s1, const char *s2);
bool strnatcasesort(const char *s1, const std::string& s2);

} /* namespace util */

#endif /* COMPARE_H_ */
