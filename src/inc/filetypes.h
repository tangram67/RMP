/*
 * filetypes.h
 *
 *  Created on: 03.11.2018
 *      Author: dirk
 */

#ifndef INC_FILETYPES_H_
#define INC_FILETYPES_H_

#include <fcntl.h>
#include "gcc.h"

namespace util {

enum EFileListKey { FLK_DEFAULT, FLK_NAME, FLK_FILE, FLK_URL };
enum ESeekOffset  { SO_FROM_END = SEEK_END, SO_FROM_CURRENT = SEEK_CUR, SO_FROM_START = SEEK_SET };
enum EUniqueName  { UN_COUNT, UN_TIME };
enum ELoadType    { LT_BINARY, LT_MINIMIZED };
enum EMountType   { MT_ALL, MT_DISK };
enum EFileType    { FT_FILE, FT_FOLDER, FT_UNDEFINED };
enum ESearchDepth { SD_ROOT, SD_RECURSIVE };

#ifdef GCC_MATCHES_DIRENT64
using TInodeHandle = __ino64_t ;
#else
using TInodeHandle = __ino_t;
#endif

} /* namespace util */

#endif /* INC_FILETYPES_H_ */
