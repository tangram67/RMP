/*
 * exif.cpp
 *
 *  Created on: 11.10.2015
 *      Author: Dirk Brinkmeier
 */

#include <cmath>
#include "exif.h"
#include "locale.h"
#include "nullptr.h"
#include "convert.h"
#include "exception.h"
#include "fileutils.h"

namespace util {


TExifReader::TExifReader(const std::string& fileName) {
	if (fileName.empty())
		throw app_error("TExifReader::TExifReader() : Empty filename.");

	if (!util::fileExists(fileName))
		throw app_error("TExifReader::TExifReader() : File <" + fileName + "> does not exist.");

	data = exif_data_new_from_file(fileName.c_str());
}

TExifReader::~TExifReader() {
	clear();
}

void TExifReader::clear() {
	if (util::assigned(data)) {
		exif_data_unref(data);
		data = nil;
	}
}



TExif::TExif() {
	align = 0;
	count = 0;
}

TExif::TExif(const std::string& fileName) {
	align = 0;
	count = 0;
	loadFromFile(fileName);
}

TExif::~TExif() {
}

void TExif::clear() {
	if (text.size())
		text.clear();
	if (tags.size())
		tags.clear();
	if (file.size())
		file.clear();
}


bool TExif::readTags(TExifReader& reader) {
	CExifTag tag;
	std::string value;
	tags.clear();
	count = 0;
	align = 0;

	for (size_t i=0; EXIF_TAG_NAMES[i].descrition; i++) {
		tag = EXIF_TAG_NAMES[i];
		// if (readTag(reader, EXIF_IFD_INTEROPERABILITY, tag.tag, value)) {
		//	addTag(tag, value);
		// }
		if (readTag(reader, EXIF_IFD_GPS, tag.tag, value)) {
			addTag(tag, value);
		}
		if (readTag(reader, EXIF_IFD_EXIF, tag.tag, value)) {
			addTag(tag, value);
		}
		if (readTag(reader, EXIF_IFD_1, tag.tag, value)) {
			addTag(tag, value);
		}
		if (readTag(reader, EXIF_IFD_0, tag.tag, value)) {
			addTag(tag, value);
		}
	}

	return !tags.empty();
}

bool TExif::readTag(TExifReader& reader, ExifIfd ifd, ExifTag tid, std::string& value) {
	value.clear();

	/* See if this tag exists */
	ExifEntry *entry = exif_content_get_entry(reader()->ifd[ifd], tid);
	if (entry) {
		char buf[1024];

		/* Get the contents of the tag in human-readable form */
		exif_entry_get_value(entry, buf, sizeof(buf));
		size_t size = strnlen(buf, 1024);
		if (size) {
			value.assign(buf, size);
			util::trim(value);
		}
	}

	return !value.empty();
}

void TExif::addTag(const CExifTag& tag, const std::string& value) {
	CExifItem item;
	item.tag = tag;
	item.value = value;
	item.numeric = util::strToInt(value, -1); //, app::localeUS);
	tags.insert(TExifMapItem(tag.tag, item));
	count++;

	// Calculate alignment offset for description
	size_t len = strlen(tag.descrition);
	if (len > align)
		align = len;
}


bool TExif::readMakerNotes(TExifReader& reader) {
	std::string title;
	std::string value;
	char buf[1024];
	notes.clear();

	ExifEntry* entry = exif_content_get_entry(reader()->ifd[EXIF_IFD_0], EXIF_TAG_MAKE);
	if (entry) {
		char maker[64];

		/* Get the contents of the manufacturer tag as a string */
		if (exif_entry_get_value(entry, maker, sizeof(buf))) {
			size_t size = strnlen(maker, 64);
			value.assign(maker, size);
			util::trim(value);
			if (value != "Panasonic") {
				//return false;
			}
		}
	}

	// Panasonic seems not to be supported!
	ExifMnoteData *mn = exif_data_get_mnote_data(reader());
	if (mn) {
		unsigned int max = exif_mnote_data_count(mn);
		TMakerNoteIndex mid;

		/* Loop through all MakerNote tags, searching for the desired one */
		for (unsigned int i=0; i < max; ++i) {
			mid = exif_mnote_data_get_id(mn, i);
			if (exif_mnote_data_get_value(mn, i, buf, sizeof(buf))) {
				size_t size = strnlen(buf, 1024);
				if (size) {
					// Value as string representation
					value.assign(buf, size);
					util::trim(value);

					const char* p = exif_mnote_data_get_title(mn, i);
					if (util::assigned(p)) {
						size = strnlen(buf, 1024);
						title.assign(buf, size);
						util::trim(title);
					}

					addNote(mid, title, value);
				}
			}

		}
	}

	return !notes.empty();
}

void TExif::addNote(const TMakerNoteIndex mid,const std::string& title, const std::string& value) {
	CMakerNote item;
	item.id = mid;
	item.value = value;
	item.numeric = util::strToInt(value, 0); //, app::localeUS);
	item.title = title;
	item.description = title;
	std::cout << "Maker note no. " << mid << " : " << title << " = " << value << std::endl;
	notes.insert(TMakerNoteMapItem(item.id, item));
}


size_t TExif::loadFromFile(const std::string& fileName) {
	clear();
	file = fileName;

	TExifReader reader(fileName);
	// if (!reader.isValid())
	//	throw app_error("TExif::loadFromFile() : Could not read Exif data from <" + fileName + ">");

	// Try to read all known Exif tags
	if (reader.isValid())
		readTags(reader);

	// Try to read all maker notes
	// readMakerNotes(reader);

	return tags.size();
}

size_t TExif::saveFilesAsText(const std::string& files, const ESearchDepth depth) {
	TFolderList content;
	content.scan(files, depth);
	if (!content.empty()) {
		TFolderList::const_iterator it = content.begin();
		while (it != content.end()) {
			loadFromFile((*it)->getFile());
			if (hasTags())
				saveAsText(util::fileReplaceExt((*it)->getFile(), "txt"));
			it++;
		}
	}
	return content.size();
}



bool TExif::getTag(CExifItem& item, ExifTag tid) const {
	bool retVal = false;

	if (tags.empty())
		return retVal;

	TExifMap::const_iterator it = tags.find(tid);
	if (it != tags.end()) {
		item = it->second;
		retVal = true;
	}

	return retVal;
}


bool TExif::getNote(CMakerNote& item, TMakerNoteIndex mid) const {
	bool retVal = false;

	if (notes.empty())
		return retVal;

	TMakerNoteMap::const_iterator it = notes.find(mid);
	if (it != notes.end()) {
		item = it->second;
		retVal = true;
	}

	return retVal;
}


bool TExif::isExifTime(const std::string& value) const {
	// Consistency check
	if (value.size() < 19)
		return false;

	if (value[4] != ':' || value[7] != ':' || value[10] != ' ' || value[13] != ':' || value[16] != ':')
		return false;

	return true;
}

void TExif::normalizeExifTime(std::string& value) const {
	if (value.size() > 7) {
		value[4] = '-';
		value[7] = '-';
	}
}

std::string TExif::formatAsText(const CExifItem& item, const std::string separator, size_t align, bool asText) const {
	std::string k,s;
	k = item.tag.descrition;
	s = item.value;
	if (k.empty())
		k = "<Unknown key>";
	if (s.empty())
		s = "<Empty value>";
	else {
		if (asText && (item.numeric >= 0))
			s = util::quote(s);
		else {
			if (isExifTime(s))
				normalizeExifTime(s);
		}
	}
	if (align)
		util::fill(k, ' ', align);
	return k + separator + s;
}

std::string TExif::formatAsJson(const CExifItem& item, const std::string preamble) const {
	// Example:
	// line = "City": "Hill Valley"
	// line = "Date": "2015-10-21 07:28:00.000"
	// line = "Latitude": 34.1340455
	// line = "Longitude": -118.3520114
	std::string d,k,s;
	d = item.tag.descrition;
	s = item.value;

	if (d.empty())
		k = "\"Unknown key\"";
	else
		k = util::quote(d);

	if (s.empty())
		s = "null";
	else {
		if (isExifTime(s)) {
			normalizeExifTime(s);
			s = util::quote(s);
		} else {
			if (util::strToDouble(item.value, HUGE_VAL/*, app::localeUS*/) == HUGE_VAL) {
				s = util::quote(s);
			}
		}
	}
	return preamble + k + ": " + s;
}

void TExif::debugOutput() {
	if (tags.empty()) {
		std::cout << "File <" << file << "> contains no Exif data." << std::endl;
		return;
	}

	text.clear();
	text.add("Exif tags for <" + file + ">");
	writeExifDataAsText(text, EXF_TEXT, "  ", alignment() + 1);
	text.debugOutput();
}


void TExif::saveToFile(util::TStringList& list, const std::string& fileName, const EExifFormat fmt, const std::string& preamble) {
	if (fileName.empty())
		throw app_error("TExif::saveToFile() : Empty filename.");

	if (tags.empty()) {
		return;
	}

	writeExifDataAsText(list, fmt, preamble, (fmt != EXF_TEXT ? 0 : alignment()));
	list.saveToFile(fileName);
}


void TExif::saveAsCsv(const std::string& fileName) {
	text.clear();
	text.add("Filename;" + file);
	saveToFile(text, fileName, EXF_CSV, "");
}


void TExif::saveAsText(const std::string& fileName) {
	text.clear();
	text.add("Exif tags for <" + file + ">");
	saveToFile(text, fileName, EXF_TEXT, "  ");
}


const TStringList& TExif::asText(const EExifFormat fmt, const std::string& preamble) const {
	text.clear();
	text.add("Exif tags for <" + file + ">");
	writeExifDataAsText(text, fmt, preamble, (fmt != EXF_TEXT ? 0 : alignment()));
	return text;
}


const TStringList& TExif::asJSON(const std::string& name, const bool asObject) const {
	json.clear();
	writeExifDataAsJSON(json, name, asObject);
	return json;
}


bool TExif::asVariants(util::TVariantValues& variants) const {
	variants.clear();
	CExifTag tag;
	CExifItem item;
	for (size_t i=0; EXIF_TAG_NAMES[i].descrition; i++) {
		tag = EXIF_TAG_NAMES[i];
		if (getTag(item, tag.tag)) {
			if (item.numeric >= 0) {
				variants.add(tag.descrition, item.numeric);
			} else {
				variants.add(tag.descrition, item.value);
			}
		}
	}
	return !variants.empty();
}

void TExif::writeExifDataAsText(util::TStringList& list, const EExifFormat fmt, const std::string& preamble, size_t align) const {
	CExifTag tag;
	CExifItem item;
	for (size_t i=0; EXIF_TAG_NAMES[i].descrition; i++) {
		tag = EXIF_TAG_NAMES[i];
		if (getTag(item, tag.tag)) {
			switch (fmt) {
			case EXF_CSV:
				list.add(formatAsText(item, ";", 0, true));
				break;
			case EXF_TEXT:
			default:
				list.add(preamble + formatAsText(item, " : ", align));
				break;
			}
		}
	}
}


void TExif::writeExifDataAsJSON(util::TStringList& list, const std::string& name, const bool asObject) const {
	if (count > 0) {
		if (!list.empty())
			list.clear();

		// Begin new JSON object
		std::string preamble;
		if (asObject) {
			preamble = "  ";
			list.add("{");
		} else {
			preamble = "";
		}

		// Begin new JSON array
		if (!name.empty())
			list.add(preamble + "\"" + name + "\": [");
		else
			list.add(preamble + "[");

		// Convert tags to JSON strings
		CExifTag tag;
		CExifItem item;
		for (size_t i=0; EXIF_TAG_NAMES[i].descrition; i++) {
			tag = EXIF_TAG_NAMES[i];
			if (getTag(item, tag.tag)) {
				list.add(formatAsJson(item, preamble + "  "));
			}
		}

		// Close JSON array and object
		list.add(preamble + "]");
		if (asObject)
			list.add("}");
	}
}



} /* namespace util */
