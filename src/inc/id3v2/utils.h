/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef id3v2lib_utils_h
#define id3v2lib_utils_h

// this piece of code makes this header usable under MSVC
// without downloading msinttypes
#ifndef _MSC_VER
  #include <inttypes.h>
#else
  typedef unsigned short uint16_t;
#endif

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t btoi(char* bytes, size_t size, size_t offset);
char* itob(size_t value);
size_t syncint_encode(size_t value);
size_t syncint_decode(size_t value);
void add_to_list(ID3v2_frame_list* list, ID3v2_frame* frame);
ID3v2_frame* get_from_list(ID3v2_frame_list* list, const char* frame_id);
void free_tag(ID3v2_tag* tag);
char* get_mime_type_from_filename(const char* filename);
int calc_hash_id(const char* ID, size_t size);

// String functions
int has_bom_c(const char *const str, const size_t size);
int has_bom_w(uint16_t* string);
void str_swap_16(char* str, size_t* size);
uint16_t* char_to_utf16(char* string, int size);
void println_utf16(uint16_t* string, int size);
char* get_path_to_file(const char* file);

#ifdef __cplusplus
}
#endif

#endif
