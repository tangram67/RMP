/*
 * id3.cpp
 *
 *  Created on: 18.09.2016
 *      Author: Dirk Brinkmeier
 */

#include "id3.h"
#include "id3genres.h"
#include "convert.h"
#include "stringutils.h"
#include "jpegtypes.h"
#include "bitmap.h"
#include <cstdlib>
#include <locale>

namespace music {

TID3TagParser::TID3TagParser() {
	tag = nil;
}

TID3TagParser::~TID3TagParser() {
	clear();
}

void TID3TagParser::clear() {
	if (util::assigned(tag)) 
		free_tag(tag);
	tag = nil;	
}

bool TID3TagParser::load(const TID3Buffer& buffer) {
	tag = load_tag_with_buffer(buffer.data(), buffer.size());
	return valid();
}

ssize_t TID3TagParser::parse(const TID3Buffer& buffer, CMetaData& tags) {
	CCoverData artwork;
	return parse(buffer, tags, artwork, ETL_METADATA);
}

ssize_t TID3TagParser::parse(const TID3Buffer& buffer, CMetaData& tags, CCoverData& artwork, ETagLoaderType mode) {
	ssize_t retVal = 0;
	
	if (valid())
		return (ssize_t)-1;

	if (!load(buffer))
		return (ssize_t)-2;

	// Read all needed tags
	if (mode == ETL_ALL || mode == ETL_METADATA) {
		retVal = getTags(tags);
		if (retVal <= 0)
			return (ssize_t)-3;

		if (tags.track.tracknumber <= 0)
			return (ssize_t)-4;
	}

	if (mode == ETL_ALL || mode == ETL_PICTURE) {
		if (getPicture(artwork.artwork))
			++retVal;
	}

	if (!tags.text.artist.empty() &&
		!tags.text.album.empty() &&
		!tags.text.title.empty() &&
		!tags.text.track.empty())
		return retVal;

	return (ssize_t)-5;
}

ssize_t TID3TagParser::getTags(CMetaData& tags) {
	ssize_t retVal = 0;
	if (valid()) {
		ID3v2_frame_list* it = tag->frames;
		if (util::assigned(it)) {
			bool found;
			bool years = false;
			bool recorded = false;
			ID3v2_frame* frame = it->frame;

			while (util::assigned(it) && util::assigned(frame)) {
				found = false;

				// Album
				if(!found && frame->hash_id == ID3HID_ALBUM) {
					tags.text.album = getFrameAsText(frame);
					found = true;
					++retVal;
				}

				// Artist tags:
				// ID3HID_LEADARTIST --> "TPE1" ... Lead performer(s)/Soloist(s)
				// ID3HID_BAND       --> "TPE2" ... Band/orchestra/accompaniment/albumartist
				// ID3HID_CONDUCTOR  --> "TPE3" ... Conductor/performer refinement
				// ID3HID_MIXARTIST  --> "TPE4" ... Interpreted, remixed, or otherwise modified by
				// ID3FID_COMPOSER   --> "TCOM" ... Composer
				if(!found && frame->hash_id == ID3HID_LEADARTIST) {
					tags.text.artist = getFrameAsText(frame);
					found = true;
					++retVal;
				}
				if(!found && frame->hash_id == ID3HID_BAND) {
					// Overwrite existing tag...
					tags.text.albumartist = getFrameAsText(frame);
					found = true;
					++retVal;
				}
				if(!found && frame->hash_id == ID3HID_MIXARTIST) {
					// Do not overwrite existing band tag!
					if (tags.text.albumartist.empty())
						tags.text.albumartist = getFrameAsText(frame);
					found = true;
					++retVal;
				}
				if(!found && frame->hash_id == ID3HID_COMPOSER) {
					tags.text.composer = getFrameAsText(frame);
					found = true;
					++retVal;
				}
				if(!found && frame->hash_id == ID3HID_CONDUCTOR) {
					tags.text.conductor = getFrameAsText(frame);
					found = true;
					++retVal;
				}

				// Song title
				if(!found && frame->hash_id == ID3HID_TITLE) {
					tags.text.title = getFrameAsText(frame);
					found = true;
					++retVal;
				}

				// Year tags (released, recorded, etc.)
				// ID3FID_YEAR     --> "TYER" ... Year
				// ID3FID_ALT_YEAR --> "TDRC" ... Year as used by dBpoweramp Release 14.1
				// ID3FID_ORIGYEAR --> "TORY" ... Original release year
				if(!found && frame->hash_id == ID3HID_YEAR) {
					if (!years) {
						std::string year = getFrameAsText(frame);
						year = util::cprintf("%04.4d", util::strToInt(year, 1900));
						if (!tags.text.year.empty()) {
							if (tags.text.year != year)
								tags.text.year = year + "/" + tags.text.year;
						} else {
							tags.text.year = year;
						}
						years = true;
					}
					found = true;
					++retVal;
				}
				if(!found && tags.text.year.empty() && frame->hash_id == ID3HID_ALT_YEAR) {
					if (!years) {
						std::string year = getFrameAsText(frame);
						year = util::cprintf("%04.4d", util::strToInt(year, 1900));
						if (!tags.text.year.empty()) {
							if (tags.text.year != year)
								tags.text.year = year + "/" + tags.text.year;
						} else {
							tags.text.year = year;
						}
						years = true;
					}
					found = true;
					++retVal;
				}
				if(!found && frame->hash_id == ID3HID_ORIGYEAR) {
					std::string year = getFrameAsText(frame);
					if (!recorded) {
						if (!year.empty()) {
							year = util::cprintf("%04.4d", util::strToInt(year, 1900));
							if (!tags.text.year.empty()) {
								if (tags.text.year != year)
									tags.text.year = tags.text.year + "/" + year;
							}
							else {
								tags.text.year = year;
							}
							recorded = true;
						}
					}
					found = true;
					++retVal;
				}

				// Recording date(s)
				if(!found && frame->hash_id == ID3HID_RECORDINGDATES) {
					tags.text.date = getFrameAsText(frame);
					found = true;
					++retVal;
				}
				if(!found && frame->hash_id == ID3HID_DATE) {
					if (tags.text.date.empty())
						tags.text.date = getFrameAsText(frame);
					found = true;
					++retVal;
				}

				// Song genre
				if(!found && frame->hash_id == ID3HID_CONTENTTYPE) {
					tags.text.genre = getFrameAsText(frame);
					found = true;
					++retVal;
				}

				// Comment
				if(!found && frame->hash_id == ID3HID_COMMENT) {
					tags.text.comment = getFrameAsComment(frame);
					found = true;
					++retVal;
				}

				// Encoder hint
				if(!found && frame->hash_id == ID3HID_ENCODEDBY) {
					tags.text.description = getFrameAsText(frame);
					found = true;
					++retVal;
				}

				// Track, disk and compilation tags
				if(!found && frame->hash_id == ID3HID_TRACKNUM) {
					music::CTagValues r = tagToValues(getFrameAsText(frame), 1);
					tags.track.tracknumber = r.value;
					tags.track.trackcount = r.count;
					tags.text.track = util::cprintf("%02.2d", tags.track.tracknumber);
					found = true;
					++retVal;
				}
				if(!found && frame->hash_id == ID3HID_PARTOFSET) {
					music::CTagValues r = tagToValues(getFrameAsText(frame), 0);
					tags.track.disknumber = r.value;
					tags.track.diskcount = r.count;
					tags.text.disk = util::cprintf("%02.2d", tags.track.disknumber);
					found = true;
					++retVal;
				}
				if(!found && frame->hash_id == ID3HID_COMPILATION) {
					int val = util::strToInt(getFrameAsText(frame), 0);
					if (val > 0)
						tags.track.compilation = true;
					found = true;
					++retVal;
				}

				// All tags read?
				if (retVal >= 14)
					break;

				// Next in list....
				it = it->next;
				frame = util::assigned(it) ? it->frame : nil;
			}
		}
	}
    return retVal;
}

std::string TID3TagParser::getFrameAsText(ID3v2_frame* frame) {
	if (util::assigned(frame)) {
		ID3v2_frame_text_content* content = parse_text_frame_content(frame);
		if (util::assigned(content)) {
			TID3TextGuard<ID3v2_frame_text_content> guard(&content);
			return getTextFromContent(content->data, content->size, content->encoding);
		}
	}
    return "";
}

std::string TID3TagParser::getFrameAsComment(ID3v2_frame* frame) {
	if (util::assigned(frame)) {
		ID3v2_frame_comment_content* content = parse_comment_frame_content(frame);
		if (util::assigned(content)) {
			TID3CommentGuard<ID3v2_frame_comment_content> guard(&content);
			if (util::assigned(content->text)) {
				return getTextFromContent(content->text->data, content->text->size, content->text->encoding);
			}
		}
   	}
    return "";
}


std::string TID3TagParser::getTextFromContent(const char *const buffer, size_t size, const char encoding) {
	if (util::assigned(buffer) && size > 0) {
		// $00 – ISO-8859-1 (LATIN-1, Identical to ASCII for values smaller than 0x80u).
		// $01 – UCS-2 (UTF-16 encoded Unicode with BOM), in ID3v2.2 and ID3v2.3.
		// $02 – UTF-16BE encoded Unicode without BOM, in ID3v2.4.
		// $03 – UTF-8 encoded Unicode, in ID3v2.4.
		switch (encoding) {
			case 0x00:
				// Convert from ANSI / ASCII
				return AnsiToMultiByteStr(buffer, size);
				break;
			case 0x01:
			case 0x02:
				// Convert from UTF-16 LE and BE, with or without BOM
				return UTF16ToMultiByteStr(buffer, size);
				break;
			case 0x03:
				// String is already (or should be!) UTF-8, convert to valid multibyte string
				return UTF8ToMultiByteStr(buffer, size);
				break;
			default:
				break;
		}
	}
    return "";
}


std::string TID3TagParser::getText(int hashID) {
    if (valid())
		return getFrameAsText(getFrame(hashID));
    return "";
}

bool TID3TagParser::getPicture(TArtwork& cover) {
	cover.clear();
	ID3v2_frame* frame = getFrame(ID3HID_PICTURE);
	if (util::assigned(frame)) {
		ID3v2_frame_apic_content* content = parse_apic_frame_content(frame);
		if (util::assigned(content)) {
			TID3PictureGuard<ID3v2_frame_apic_content> guard(&content);
			cover.assign(content->data, content->picture_size);
		}
	}
    return !cover.empty();
}

ID3v2_frame* TID3TagParser::getFrame(int hashID) {
    if (valid()) {
		ID3v2_frame_list* it = tag->frames; 
		while(util::assigned(it) && util::assigned(it->frame)) {
			if(it->frame->hash_id == hashID) {
				return it->frame;
			}
			it = it->next;
		}
	}
    return nil;
}



TID3V1TagReader::TID3V1TagReader() {
}

TID3V1TagReader::~TID3V1TagReader() {
}

ssize_t TID3V1TagReader::readTags(const util::TFile& file, CMetaData& tags) const {
	TID3v1Header header;
	ssize_t r = readHeader(file, header);
	if (r == (ssize_t)EXIT_SUCCESS) {
		// CHAR* idTitle[30];
		// CHAR* idArtist[30];
		// CHAR* idAlbum[30];
		// CHAR* idYear[4];
		// CHAR* idComment[29];
		// BYTE  idTrack;
		// BYTE  idGenre;
		tags.text.title   = getTagAsString(header.idTitle, 30);
		tags.text.artist  = getTagAsString(header.idArtist, 30);
		tags.text.album   = getTagAsString(header.idAlbum, 30);
		tags.text.comment = getTagAsString(header.idComment, 29);

		tags.text.year = getTagAsString(header.idYear, 4);
		tags.text.year = util::cprintf("%04.4d", util::strToInt(tags.text.year));

		tags.track.tracknumber = header.idTrack;
		tags.text.track = util::cprintf("%02.2d", tags.track.tracknumber);
		r = 6;

		const char* p = ID3_GENRE_NAME(header.idGenre);
		if (util::assigned(p)) {
			tags.text.genre = p;
			++r;
		}

	}
	return r;
}

ssize_t TID3V1TagReader::readHeader(const util::TFile& file, TID3v1Header& header) const {
	size_t hsize = sizeof(TID3v1Header);
	if (file.isOpen() && (file.getSize() > hsize)) {
		off_t offset = file.getSize() - hsize;
		ssize_t r = file.seek(offset);
		if (r == offset) {
			r = file.read((char*)&header, hsize);
			if (r == (ssize_t)hsize) {
				if (0 == strncmp((char*)&header.id3Tag, "TAG", sizeof(TID3Tag))) {
					return (ssize_t)EXIT_SUCCESS;
				}
				return (ssize_t)-23;
			}
			return (ssize_t)-22;
		}
		return (ssize_t)-21;
	}
	return (ssize_t)-20;
}

bool TID3V1TagReader::hasTags(const util::TFile& file) const {
	TID3v1Header header;
	return ((ssize_t)EXIT_SUCCESS == readHeader(file, header));
}

bool TID3V1TagReader::trimLength(const CHAR* buffer, size_t& size) const {
	while ((BYTE)buffer[size - 1] <= USPC && size > 0)
		--size;
	return size > 0;
}

std::string TID3V1TagReader::getTagAsString(const CHAR* buffer, size_t size) const {
	if (util::assigned(buffer) && size > 0) {
		// Convert from ANSI / ASCII and skip all trailing whitechars including spaces
		if (trimLength(buffer, size)) {
			if (isValidUTF8MultiByteStr(buffer, size)) {
				return UTF8ToMultiByteStr(buffer, size);
			} else {
				return AnsiToMultiByteStr(buffer, size);
			}
		}
	}
    return "";
}



TID3HeaderReader::TID3HeaderReader() {
}

TID3HeaderReader::~TID3HeaderReader() {
}

ssize_t TID3HeaderReader::readHeader(const util::TFile& file, size_t position, TID3v2Header& header, std::string& hint, const bool debug) const {
	size_t hsize = sizeof(TID3v2Header);
	if (file.isOpen() && (position + hsize) < file.getSize()) {
		ssize_t r = file.seek(position);
		if (r == (ssize_t)position) {
			r = file.read((char*)&header, hsize);
			if (r == (ssize_t)hsize) {
				if (0 == strncmp((char*)&header.id3Tag, "ID3", sizeof(TID3Tag))) {
					if (util::isMemberOf((int)header.idVersion, 2,3,4)) {
						r = convertInt28ToSize(header.idTagSize);
						if ((position + r + hsize) <= file.getSize()) {
							// Return offset to first byte after ID3 tags
							// --> tag size + ID3 header size
							if (debug) std::cout << "TID3HeaderReader::readHeader() Offset = " << r + hsize << std::endl;
							return (ssize_t)(r + hsize);
						}
						hint = "File corrupted or to small for valid ID3 header";
						return (ssize_t)-15;
					}
					if (debug) std::cout << "TID3HeaderReader::readHeader() Version = " << header.idVersion << std::endl;
					hint = "Invalid ID3 tag version, not in 2,3 or 4";
					return (ssize_t)-14;
				}
				if (debug) std::cout << "TID3HeaderReader::readHeader() Tag = " << header.id3Tag << std::endl;
				hint = "Invalid ID3 header tag";
				return (ssize_t)-13;
			}
			if (debug) std::cout << "TID3HeaderReader::readHeader() Read = (" << r << "/" << hsize << std::endl;
			hint = "Could not read header from file";
			return (ssize_t)-12;
		}
		if (debug) std::cout << "TID3HeaderReader::readHeader() Position = (" << r << "/" << position << std::endl;
		hint = "Seek to header position failed";
		return (ssize_t)-11;
	}
	if (debug) std::cout << "TID3HeaderReader::readHeader() Invalid file." << std::endl;
	hint = "File not readable or too small for ID3 header";
	return (ssize_t)-10;
}

size_t TID3HeaderReader::convertInt28ToSize(const TInt28 value) {
	// Converts a 28bit integer used in ID3v2 unsynchronized scheme to integer:
	// Take the rightmost byte and let it there,
	// take the second rightmost byte and move it 7bit to left
	return (value[3] << 0) |
			(value[2] << 7) |
			(value[1] << 14) |
			(value[0] << 21);
}

bool TID3HeaderReader::convertSizeToInt28(const size_t value, TInt28& result) {
	// Move every byte in value to the right, take the 7 LSBs
	// and assign them to the result
	if (value < (size_t)((uint32_t)-1)) {
		uint32_t v = (uint32_t)value;
		result[3] = (v >>  0) & 0x7F;
		result[2] = (v >>  7) & 0x7F;
		result[1] = (v >> 14) & 0x7F;
		result[0] = (v >> 21) & 0x7F;
		return true;
	}
	return false;
}



TID3TagReader::TID3TagReader() {
	debug = false;
}

TID3TagReader::~TID3TagReader() {
}

ssize_t TID3TagReader::readTags(util::TFile& file, size_t offset, CMetaData& tags) {
	CCoverData artwork;
	return readTags(file, offset, tags, artwork, ETL_METADATA);
}


ssize_t TID3TagReader::readTags(util::TFile& file, size_t offset, CMetaData& tags, CCoverData& artwork, ETagLoaderType mode) {
	TID3v2Header header;
	ssize_t r;

	// Read and check for valid ID3 header
	std::string hint;
	r = readHeader(file, offset, header, hint, debug);
	if (debug) {
		std::cout << "TID3TagReader::readID3Tags() File <" << file.getName() << "> ID3 tag valid: " << (bool)(r > 0)
					<< " (" << r << "/" << file.getSize() - offset << ")" << std::endl;
	}

	// Read ID3 metadata into buffer
	if (r > 0) {
		r = readBuffer(file, offset, (size_t)r);
		if (debug) std::cout << "TID3TagReader::readID3Tags() File <" << file.getName() << "> Read from buffer = " << r << std::endl;
	}

	// Read ID3v2 tags from buffer
	if (r > 0) {
		TID3TagParser parser;
		r = parser.parse(buffer, tags, artwork, mode);
		if (mode == ETL_ALL || mode == ETL_PICTURE) {
			if (!artwork.artwork.empty())
				artwork.filename = file.getFile();
		}
		if (debug) std::cout << "TID3TagReader::readID3Tags() File <" << file.getName() << "> Parsed ID3v2 tags = " << r << std::endl;
	}

	// Fallback to ID3v1 tags...
	if (r == (ssize_t)0) {
		TID3V1TagReader reader;
		r = reader.readTags(file, tags);
		if (debug) std::cout << "TID3TagReader::readID3Tags() File <" << file.getName() << "> Read ID3v1 tags = " << r << std::endl;
	}

	if (debug) std::cout << std::endl;
	buffer.clear();
	return r;
}

ssize_t TID3TagReader::readBuffer(util::TFile& file, size_t position, size_t size) {
	if (file.isOpen() && (position + size) <= file.getSize()) {
		size_t r = file.seek(position);
		if (r == position) {
			buffer.resize(size, false);
			r = file.read((char*)buffer.data(), size);
			if (r == size) {
				return r;
			}
			return (ssize_t)-32;
		}
		return (ssize_t)-31;
	}
	return (ssize_t)-30;
}


} /* namespace music */
