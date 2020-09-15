/*
 * config.h
 *
 *  Created on: 25.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "../autoconf.h"

/*
 * Enable options by removing trailing underscores
 *   e.g. _USE_MSSQL_ODBC --> USE_MSSQL_ODBC
 */

// Use static memory for process locks instead of dynamic locked memory
// --> ENABLED by default
#if not defined USE_STATIC_SEMAPHORES
#  define USE_STATIC_SEMAPHORES
#endif

// Use mutual exclusion instead of NTPL R/W lock
// --> DISABLED by default
#if not defined USE_MUTEX_AS_RWLOCK
#  define _USE_MUTEX_AS_RWLOCK
#endif

// Use errorcheck mutexes for debug purposes
// --> DISABLED by default
#if not defined USE_DEFAULT_ERRORCHECK_MUTEX
#  define _USE_DEFAULT_ERRORCHECK_MUTEX
#endif

// Define stream output "handler"
// --> ENABLED by default
#if not defined USE_APPLICATION_AS_OUTPUT
#  define USE_APPLICATION_AS_OUTPUT
#endif

#ifdef USE_APPLICATION_AS_OUTPUT
#  define aout application
#else
#  define aout std::cout
#endif

// Use GLibC swap macros
// --> DISABLED by default
#if not defined USE_GLIBC_SWAP_MACROS
#  define _USE_GLIBC_SWAP_MACROS
#endif

// Use 'using' instead of 'typedef'
// --> ENABLED by default
#if not defined USE_TEMPLATE_ALIAS
#  define USE_TEMPLATE_ALIAS
#endif

// Boolean stream formatting
// --> ENABLED by default
#if not defined USE_BOOLALPHA
#  define USE_BOOLALPHA
#endif
#if not defined DEFAULT_BOOLEAN_FALSE_NAME
#  define DEFAULT_BOOLEAN_TRUE_NAME "True"
#endif
#if not defined DEFAULT_BOOLEAN_FALSE_NAME
#  define DEFAULT_BOOLEAN_FALSE_NAME "False"
#endif

// Database
// --> SQLite enabled by default
// --> MSSQL disabled by default
#if not defined USE_SQLITE3
#  define USE_SQLITE3
#endif

// Use native binary BLOBs
// --> if binary BLOBs are not enabled Base64 encoding is used
// --> Binary (non ASCII) BLOBs are not (!) supported by PostgreSQL
// --> Use Base64 encoded blobs instead, macro enabled when binary symbol not defined
#if not defined USE_BINARY_BLOBS
#  define _USE_BINARY_BLOBS
#endif
#if not defined USE_BINARY_BLOBS
#  define USE_BASE64_BLOBS
#endif

// Supported audio file types
// --> ALAC enabled by default
// --> AAC disabled by default
#if not defined SUPPORT_ALAC_FILES
#  define SUPPORT_ALAC_FILES
#endif
#if not defined SUPPORT_AAC_FILES
#  define _SUPPORT_AAC_FILES
#endif

// Inifiles parsing
// --> Parse by pointer arithmetic by default
#if not defined PARSE_SECTION_BY_INDEX
#define _PARSE_SECTION_BY_INDEX
#endif

// CSV file parsing
// --> Parse by string array index index by default
#if not defined PARSE_CSV_BY_INDEX
#define PARSE_CSV_BY_INDEX
#endif

// Use EPOLL on sockets
// --> ENABLED by default, no TLS sockets when disabled!
#if not defined HAS_EPOLL
#  define HAS_EPOLL
#endif

// Set C++ global locale in TLocationGuard
// --> DISABLED by default
#if not defined IMBUE_LOCATION_GUARD
#  define _IMBUE_LOCATION_GUARD
#endif

// Set C++ global locale in TLocationGuard
// --> DISABLED by default
#if not defined USE_SYSTEMD_NOTIFY
#  define _USE_SYSTEMD_NOTIFY
#endif

#endif /* CONFIG_H_ */
