/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 *
 * Modified by Dirk Brinkmeier, 2015-17
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "frame.h"
#include "utils.h"
#include "constants.h"

ID3v2_frame* parse_frame(char* bytes, size_t offset, int version, size_t length)
{
	size_t frame_size;
	ID3v2_frame* frame = new_frame();

	// Parse frame header
	memcpy(frame->frame_id, bytes + offset, ID3_FRAME_SIZE);
	// printf("ID3v2:parse_frame() Frame \"%.*s\" \n", ID3_FRAME_SIZE, frame->frame_id);

	// Check for valid frame
	if(frame->frame_id[0] == '\0') {
		free(frame);
		return NULL;
	}

	frame_size = frame->size = btoi(bytes, ID3_FRAME_SIZE, offset += ID3_FRAME_SIZE);
	if(version == ID3v24) {
		frame->size = syncint_decode(frame->size);
		if (frame->size != frame_size) {};
		//	printf("ID3v2:parse_frame() ID3v24 tag detected: frame->size = %ld, frame_size = %ld\n", frame->size, frame_size);
	}

	// Detect erroneous frame length
	if ((frame->size + offset) >= length) {
		printf("ID3v2:parse_frame() Erroneous frame length for \"%.*s\" \n", 4, frame->frame_id);
		free(frame);
		return NULL;
	}


	memcpy(frame->flags, bytes + (offset += ID3_FRAME_SIZE), 2);
	frame->hash_id = calc_hash_id(frame->frame_id, ID3_FRAME_SIZE);

	// Load frame data
	frame->data = (char*) malloc(frame->size * sizeof(char));
	memcpy(frame->data, bytes + (offset += ID3_FRAME_FLAGS), frame->size);

	return frame;
}

ID3v2_frame_type get_frame_type(char* frame_id)
{
	switch(frame_id[0]) {
		case 'T': // All "Txxx" ID3 IDs
			return EF_TEXT_FRAME;
		case 'C':
			if (0 == strncmp(frame_id, COMMENT_FRAME_ID, 4))
				return EF_COMMENT_FRAME;
			break;
		case 'A':
			if (0 == strncmp(frame_id, ALBUM_COVER_FRAME_ID, 4))
				return EF_APIC_FRAME;
			break;
		default:
			break;
	}
	return EF_INVALID_FRAME;
}

void trim_length(const char *const data, size_t* size)
{
	while (data[(*size) - 1] == '\0' && ((*size) > 0))
		--(*size);
}

void strip_contol_chars(char *const data, size_t* size)
{
	unsigned char c;
	if (*size > 2) {
		size_t length = *size;
		for (ssize_t i = (*size) - 2; i >= 0; --i) {
			c = (unsigned char)data[i];
			if (c < 0x20u && c != 0x0Au) {
				memmove(data+i, data+i+1, length-i-1);
				--length;
			}
		}
		*size = length;
	}
}

void normalize_text_content(char** text, size_t* size, char encoding)
{
	int bom = has_bom_c(*text, *size);
	if (encoding == 0x00 && bom == 0) {
		trim_length(*text, size);
		strip_contol_chars(*text, size);
	}
}

ID3v2_frame_text_content* parse_text_frame_content(ID3v2_frame* frame)
{
	ID3v2_frame_text_content* content;
	if(frame == NULL) {
		return NULL;
	}

	content = new_text_content(frame->size);
	content->encoding = frame->data[0];
	content->size = frame->size - ID3_FRAME_ENCODING;
	memcpy(content->data, frame->data + ID3_FRAME_ENCODING, content->size);
	normalize_text_content(&content->data, &content->size, content->encoding);
	return content;
}

ID3v2_frame_comment_content* parse_comment_frame_content(ID3v2_frame* frame)
{
	ID3v2_frame_comment_content *content;
	if(frame == NULL) {
		return NULL;
	}

	content = new_comment_content(frame->size);

	content->text->encoding = frame->data[0];
	content->text->size = frame->size - ID3_FRAME_ENCODING - ID3_FRAME_LANGUAGE - ID3_FRAME_SHORT_DESCRIPTION;
	memcpy(content->language, frame->data + ID3_FRAME_ENCODING, ID3_FRAME_LANGUAGE);
	content->short_description = "\0"; // Ignore short description
	memcpy(content->text->data, frame->data + ID3_FRAME_ENCODING + ID3_FRAME_LANGUAGE + 1, content->text->size);
	normalize_text_content(&content->text->data, &content->text->size, content->text->encoding);
	return content;
}

char* parse_mime_type(char* data, size_t* size)
{
	char* mime_type = (char*) malloc(30 * sizeof(char));

	while(data[*size] != '\0') {
		mime_type[*size - 1] = data[*size];
		(*size)++;
	}

	return mime_type;
}

ID3v2_frame_apic_content* parse_apic_frame_content(ID3v2_frame* frame)
{
	ID3v2_frame_apic_content *content;
	size_t offset = 1; // Skip ID3_FRAME_ENCODING

	if(frame == NULL) {
		return NULL;
	}

	content = new_apic_content();
	content->picture_size = 0;
	content->data = NULL;

	content->encoding = frame->data[0];

	content->mime_type = parse_mime_type(frame->data, &offset);
	content->picture_type = frame->data[++offset];
	content->description = &frame->data[++offset];

	if (content->encoding == 0x01 || content->encoding == 0x02) {
		/* skip UTF-16 description */
		for ( ; * (uint16_t *) (frame->data + offset); offset += 2);
		offset += 2;
	} else {
		/* skip UTF-8 or Latin-1 description */
		for ( ; frame->data[offset] != '\0'; offset++);
		offset += 1;
	}

	if (frame->size > offset) {
		content->picture_size = frame->size - offset;
		content->data = (char*) malloc(content->picture_size);
		memcpy(content->data, frame->data + offset, content->picture_size);
	}

	return content;
}
