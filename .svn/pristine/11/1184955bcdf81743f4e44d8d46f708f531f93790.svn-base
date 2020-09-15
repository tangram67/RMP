/*
 * tags.cpp
 *
 *  Created on: 02.07.2016
 *      Author: Dirk Brinkmeier
 */

#include "tags.h"
#include "json.h"
#include "tables.h"
#include "datetime.h"
#include "htmlutils.h"


namespace music {

TFileTag::TFileTag() {
}

TFileTag::~TFileTag() {
}

void TFileTag::clear() {
	meta.clear();
	sort.clear();
	stream.clear();
	file.clear();
	cover.clear();
	error.clear();
}

static const std::string STRIP_TAG_CHARS   = "\";<>";
static const std::string REPLACE_TAG_CHARS = "',[]";

static char replaceChar(char value) {
    size_t pos = STRIP_TAG_CHARS.find(value);
	if(pos != std::string::npos)
         return REPLACE_TAG_CHARS[pos];
	if ((unsigned char)value < UINT8_C(0x20))
		return ' ';
    return value;
}

void TFileTag::sanitize(std::string& tag) {
	// Simple 1:1 character replace for whitespaces and '";<>'
	// --> Prevent problems with quoted HTML text properties, HTML tags and failed CSV conversions
	std::transform(tag.begin(), tag.end(), tag.begin(), replaceChar);
}

bool TFileTag::parseJSON(const std::string& json) {
	bool retVal = false;
	clear();
	data::TTable table;
	table.loadAsJSON(json);
	table.debugOutputData();
	if (!table.empty()) {

		// Read first row only...
		size_t idx = 0;

		// Music meta data
		meta.text.artist      = table[idx]["Artist"].asString();
		meta.text.albumartist = table[idx]["Albumartist"].asString();
		meta.text.album       = table[idx]["Album"].asString();
		meta.text.title       = table[idx]["Title"].asString();
		meta.text.genre       = table[idx]["Genre"].asString();
		meta.text.composer    = table[idx]["Composer"].asString();
		meta.text.comment     = table[idx]["Comment"].asString();
		meta.text.description = table[idx]["Description"].asString();
		meta.hash.title       = table[idx]["Titlehash"].asString();
		meta.hash.album       = table[idx]["Albumhash"].asString();

		// Track meta data
		meta.track.tracknumber = table[idx]["Track"].asInteger();
		meta.track.disknumber  = table[idx]["Disk"].asInteger();
		meta.text.track        = table[idx]["Track"].asString();
		meta.text.disk         = table[idx]["Disk"].asString();
		meta.text.year         = table[idx]["Year"].asString();
		meta.text.date         = table[idx]["Date"].asString();

		// Stream data
		stream.sampleCount    = table[idx]["SampleCount"].asInteger64();
		stream.sampleSize     = table[idx]["SampleSize"].asInteger64();
		stream.sampleRate     = table[idx]["SampleRate"].asInteger();
		stream.bitsPerSample  = table[idx]["BitsPerSample"].asInteger();
		stream.bytesPerSample = table[idx]["BytesPerSample"].asInteger();
		stream.channels       = table[idx]["Channels"].asInteger();
		stream.bitRate        = table[idx]["BitRate"].asInteger();
		stream.chunkSize      = table[idx]["ChunkSize"].asInteger();
		stream.duration       = table[idx]["Duration"].asInteger();
		stream.seconds        = table[idx]["Seconds"].asInteger();

		// File properties
		file.filename  = table[idx]["Filename"].asString();
		file.basename  = table[idx]["Basename"].asString();
		file.extension = table[idx]["Extension"].asString();
		file.folder    = table[idx]["Folder"].asString();
		file.url       = table[idx]["URL"].asString();
		file.timestamp = table[idx]["Filetime"].asString();
		file.time      = util::strToDateTime(file.timestamp);
		file.size      = table[idx]["Filesize"].asInteger64();
		file.hash      = table[idx]["Filehash"].asString();

		retVal = isValid();
	}

	return retVal;
}

bool TFileTag::isValid() const {
	return meta.isValid() && stream.isValid() && file.isValid();
}

TFileTag& TFileTag::operator = (const std::string &value) {
	parseJSON(value);
	return *this;
}

TFileTag& TFileTag::operator = (const music::TFileTag &value) {
	clear();
	meta = value.meta;
	sort = value.sort;
	stream = value.stream;
	file = value.file;
	cover = value.cover;
	return *this;
}


} /* namespace music */
