/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef id3v2lib_types_h
#define id3v2lib_types_h

#include <stdint.h>

#include "constants.h"

typedef struct
{
    char tag[ID3_HEADER_TAG];
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t flags;
    size_t tag_size;
    size_t extended_header_size;
} ID3v2_header;

typedef struct
{
    size_t size;
    char encoding;
    char* data;
} ID3v2_frame_text_content;

typedef struct
{
    char* language;
    char* short_description;
    ID3v2_frame_text_content* text;
} ID3v2_frame_comment_content;

typedef struct
{
    char encoding;
    char* mime_type;
    char picture_type;
    char* description;
    size_t picture_size;
    char* data;
} ID3v2_frame_apic_content;

typedef struct
{
    char frame_id[ID3_FRAME_ID];
    int hash_id;
    size_t size;
    char flags[ID3_FRAME_FLAGS];
    char* data;
} ID3v2_frame;

typedef struct _ID3v2_frame_list
{
    ID3v2_frame* frame;
    struct _ID3v2_frame_list* start;
    struct _ID3v2_frame_list* last;
    struct _ID3v2_frame_list* next;
} ID3v2_frame_list;

typedef struct
{
    char* raw;
    ID3v2_header* tag_header;
    ID3v2_frame_list* frames;
} ID3v2_tag;

#ifdef __cplusplus
extern "C" {
#endif

// Constructor functions
ID3v2_header* new_header();
ID3v2_tag* new_tag();
ID3v2_frame* new_frame();
ID3v2_frame_list* new_frame_list();

ID3v2_frame_text_content* new_text_content(size_t size);
ID3v2_frame_comment_content* new_comment_content(size_t size);
ID3v2_frame_apic_content* new_apic_content();

void free_text_content(ID3v2_frame_text_content* content);
void free_comment_content(ID3v2_frame_comment_content* content);
void free_apic_content(ID3v2_frame_apic_content* content);

#ifdef __cplusplus
} // end of extern C
#endif

#endif
