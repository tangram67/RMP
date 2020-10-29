/*
 * fileconsts.h
 *
 *  Created on: 01.10.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef FILECONSTS_H_
#define FILECONSTS_H_

#include "gcc.h"
#include "filetypes.h"

#define DEF_URL_SEPERATOR '/';
#define DEF_CHUNK_SIZE 65535;

namespace util {

#ifdef STL_HAS_CONSTEXPR

static constexpr TInodeHandle INVALID_INODE_HANDLE = (TInodeHandle)-1;

static constexpr char URL_SEPERATOR = DEF_URL_SEPERATOR;
static constexpr size_t CHUNK_SIZE = DEF_CHUNK_SIZE;
static constexpr size_t SCAN_BUFFER_SIZE = 32 * 1024;

#else

static const TInodeHandle INVALID_INODE_HANDLE = (TInodeHandle)-1;

static const char URL_SEPERATOR = DEF_URL_SEPERATOR
static const size_t CHUNK_SIZE = DEF_CHUNK_SIZE;
static const size_t SCAN_BUFFER_SIZE = 32 * 1024;

#endif

} /* namespace util */

#endif /* FILECONSTS_H_ */
