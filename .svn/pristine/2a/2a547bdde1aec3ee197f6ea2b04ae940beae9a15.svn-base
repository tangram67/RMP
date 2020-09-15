/*
 * gcc.h
 *
 *  Created on: 31.08.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef GCC_H_
#define GCC_H_

#ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#  define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>
#include <endian.h>
#include <features.h>
#include <gnutls/gnutls.h>
#include <openssl/opensslv.h>
#include <tuple>

#include "../config.h"

#define PACKED __attribute__((__packed__))
#define UNUSED __attribute__((unused))

#if defined (__stdcall__)
#  define __stdcall __attribute__((__stdcall__))
#elif defined (stdcall)
#  define __stdcall __attribute__((stdcall))
#else
#  define __stdcall
#endif

// Use predefined macro __x86_64__
// or __WORDSIZE from wordsize.h instead
#if UINTPTR_MAX == 0xFFFFFFFFUL
#  define TARGET_X32
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFUL
#  define TARGET_X64
#else
#  if defined (__WORDSIZE)
#    if __WORDSIZE == 64
#      define TARGET_X64
#    elif __WORDSIZE == 32
#      define TARGET_X32
#    else
#      error "PREPROCESSOR ERROR in gcc.h: Unknown target word size."
#    endif
#  else
#    error "PREPROCESSOR ERROR in gcc.h: Unknown target word size."
#  endif
#endif


// Wordsize dependent format strings
#if defined TARGET_X64
#  define PRI64_FMT_A "%ld"
#  define PRIu64_FMT_A "%lu"
#  define PRIx64_FMT_A "%lX"
#  define PRI64_FMT_W L"%ld"
#  define PRIu64_FMT_W L"%lu"
#  define PRIx64_FMT_W L"%lX"
#else
#  define PRI64_FMT_A "%lld"
#  define PRIu64_FMT_A "%llu"
#  define PRIx64_FMT_A "%llX"
#  define PRI64_FMT_W L"%lld"
#  define PRIu64_FMT_W L"%llu"
#  define PRIx64_FMT_W L"%llX"
#endif


// OpenSSL feature tests
#if OPENSSL_VERSION_NUMBER > 0x1000000fL
#  define SSL_HAS_CONST_DECALRATION
#endif

#if OPENSSL_VERSION_NUMBER > 0x1000200fL
#  define SSL_HAS_ECDH_AUTO
#endif

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
#  define SSL_HAS_PROTO_VERSION
#endif

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
#  define SSL_HAS_ACCESS_GETTERS
#endif

// GnuTLS feature tests
#if defined GNUTLS_VERSION_MAJOR && defined GNUTLS_VERSION_MINOR && defined GNUTLS_VERSION_PATCH
#  define GNU_TLS_VERSION ( GNUTLS_VERSION_MAJOR * 10000 \
					       + GNUTLS_VERSION_MINOR * 100 \
					       + GNUTLS_VERSION_PATCH )
#else
#  define GNU_TLS_VERSION 0
#endif


// GLibC version macros
#if defined __GNU_LIBRARY__ && defined __GLIBC__ && defined __GLIBC_MINOR__
#  define GLIBC_VERSION ( __GNU_LIBRARY__ * 10000 \
					     + __GLIBC__ * 100 \
					     + __GLIBC_MINOR__ )
#else
#  define GLIBC_VERSION 0
#endif


#if GLIBC_VERSION > 60211
#  define GLIBC_HAS_VFORK
#endif

#ifdef _GLIBCXX_TUPLE
#  define GLIBC_HAS_TUPLE
#endif


#ifdef __USE_LARGEFILE64
#  define GCC_HAS_LARGEFILE64
#endif

#ifdef __USE_FILE_OFFSET64
#  define GCC_HAS_FILEOFFSET64
#endif

// Platform check
#ifdef __arm__
#  define GNU_ARCH_ARM
#endif


// Endianess
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
  defined(__BIG_ENDIAN__) || \
  defined(__ARMEB__) || \
  defined(__THUMBEB__) || \
  defined(__AARCH64EB__) || \
  defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#  define TARGET_BIG_ENDIAN
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
  defined(__LITTLE_ENDIAN__) || \
  defined(__ARMEL__) || \
  defined(__THUMBEL__) || \
  defined(__AARCH64EL__) || \
  defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#  define TARGET_LITTLE_ENDIAN
#else
#  error "PREPROCESSOR ERROR in gcc.h: No endianess defined."
#endif


#if defined(__FLOAT_WORD_ORDER) && __FLOAT_WORD_ORDER == __BIG_ENDIAN
#  define TARGET_FLOAT_BIG_ENDIAN
#elif defined(__FLOAT_WORD_ORDER) && __FLOAT_WORD_ORDER == __LITTLE_ENDIAN
#  define TARGET_FLOAT_LITTLE_ENDIAN
#else
#  if defined(TARGET_BIG_ENDIAN)
#    define TARGET_FLOAT_BIG_ENDIAN
#  elif defined(TARGET_LITTLE_ENDIAN)
#    define TARGET_FLOAT_LITTLE_ENDIAN
#  else
#    error "PREPROCESSOR ERROR in gcc.h: No float endianess defined."
#  endif
#endif


// Gnu complier GCC version and feature checks
#if defined __GNUC__ && defined __GNUC_MINOR__ && defined __GNUC_PATCHLEVEL__
#  define GCC_VERSION ( __GNUC__ * 10000 \
					   + __GNUC_MINOR__ * 100 \
					   + __GNUC_PATCHLEVEL__ )
#else
#  define GCC_VERSION 0
#endif


#if defined __GNUC__ && defined __GNUC_MINOR__ && defined __GNUC_PATCHLEVEL__
#  define GCC_VERSION ( __GNUC__ * 10000 \
					   + __GNUC_MINOR__ * 100 \
					   + __GNUC_PATCHLEVEL__ )
#else
#  define GCC_VERSION 0
#endif


#if defined __GNUC__ && defined __GNUC_MINOR__
#define GCC_VERSION_CHECK(M, m) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((M) << 16) + (m))
#else
#  define GCC_VERSION_CHECK(M, m) 0
#endif

// C++ 2011 check
#if __cplusplus >= 201103L
#  define GCC_IS_CXX
#  define GCC_IS_C11
#endif

// C++ 2014 check
#if __cplusplus >= 201402L
#  define GCC_IS_C14
#endif


// STL feature checks
#ifdef GCC_IS_CXX
#  define STL_HAS_HASH_VALUE
#endif

#if GCC_VERSION > 40700
#  define STL_HAS_DELEGATING_CTOR
#  define STL_HAS_RANDOM_ENGINE
#  ifdef USE_TEMPLATE_ALIAS
#    define STL_HAS_TEMPLATE_ALIAS
#  endif
#endif

#if GCC_VERSION > 40603
#  define STL_HAS_CONSTEXPR
#  define STL_HAS_STRING_POPS
#  define STL_HAS_STATIC_ASSERT
#endif

#if GCC_VERSION >= 40600
#  define STL_HAS_NULLPTR
#  define STL_HAS_RANGE_FOR
#  define STL_HAS_NUMERIC_LIMITS
#endif

#if GCC_VERSION >= 40500
#  define STL_HAS_LAMBDAS
#endif

#if GCC_VERSION >= 40400
#  define STL_HAS_ENUM_CLASS
#endif

#if GCC_VERSION >= 40102
#  define GCC_HAS_ATOMICS
#endif

#if GCC_VERSION >= 30100
#  define GCC_HAS_CXA_DEMANGLE
#endif

#ifdef STL_HAS_CONSTEXPR
#  define STATIC_CONST static constexpr const
#else
#  define STATIC_CONST static const
#endif

#if not defined STL_HAS_RANGE_FOR
#  error "PREPROCESSOR ERROR in gcc.h: Compiler does not support ranged FOR loops."
#endif

#endif /* GCC_H_ */
