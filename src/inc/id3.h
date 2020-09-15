/*
 * id3.h
 *
 *  Created on: 18.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef ID3_H_
#define ID3_H_

#include "id3types.h"
#include "fileutils.h"
#include "audiofile.h"
#include "tagtypes.h"
#include "memory.h"
#include "ASCII.h"

#include "id3v2/id3v2lib.h"

namespace music {

enum ETagLoaderType {
	ETL_METADATA,
	ETL_PICTURE,
	ETL_ALL
};

class TID3TagParser : private util::TASCII, private TAudioConvert {
private:
	ID3v2_tag* tag;
	
	void clear();
	bool load(const TID3Buffer& buffer);

	ID3v2_frame* getFrame(int hashID);
	std::string getFrameAsText(ID3v2_frame* frame);
	std::string getFrameAsComment(ID3v2_frame* frame);
	std::string getTextFromContent(const char *const buffer, size_t size, const char encoding);
	std::string getText(int hashID);
	bool getPicture(TArtwork& cover);
	ssize_t getTags(CMetaData& tags);

public:
	ID3v2_tag* operator () () const { return tag; };
	bool valid() const { return util::assigned(tag); };
	
	ssize_t parse(const TID3Buffer& buffer, CMetaData& tags);
	ssize_t parse(const TID3Buffer& buffer, CMetaData& tags, CCoverData& artwork, ETagLoaderType mode);
	
	TID3TagParser();
	virtual ~TID3TagParser();
};


class TID3V1TagReader : private util::TASCII {
private:
	ssize_t readHeader(const util::TFile& file, TID3v1Header& header) const;
	std::string getTagAsString(const CHAR* buffer, size_t size) const;
	bool trimLength(const char* buffer, size_t& size) const;

public:
	bool hasTags(const util::TFile& file) const;
	ssize_t readTags(const util::TFile& file, CMetaData& tags) const;

	TID3V1TagReader();
	virtual ~TID3V1TagReader();
};


class TID3HeaderReader {
public:
	static size_t convertInt28ToSize(const TInt28 value);
	static bool convertSizeToInt28(const size_t value, TInt28& result);
	ssize_t readHeader(const util::TFile& file, size_t position, TID3v2Header& header, std::string& hint, const bool debug = false) const;

	TID3HeaderReader();
	virtual ~TID3HeaderReader();
};


class TID3TagReader : private TID3HeaderReader {
private:
	TID3Buffer buffer;
	bool debug;

	ssize_t readBuffer(util::TFile& file, size_t position, size_t size);

	ssize_t readID3V1Tags(util::TFile& file, CMetaData& tags) const;
	ssize_t readID3V1Header(util::TFile& file, TID3v1Header& header) const;
	std::string getID3V1TagAsString(const CHAR* buffer, size_t size) const;
	void trimLength(const char* buffer, size_t& size) const;

public:
	bool getDebug() const { return debug; };
	ssize_t readTags(util::TFile& file, size_t offset, CMetaData& tags);
	ssize_t readTags(util::TFile& file, size_t offset, CMetaData& tags, CCoverData& artwork, ETagLoaderType mode);

	TID3TagReader();
	virtual ~TID3TagReader();
};


template<typename T>
class TID3TextGuard {
private:
	typedef T pointer_t;
	typedef pointer_t ** pointer_p;
	pointer_p pointer;

	void free() {
		free_text_content(*pointer);
		*pointer = nil;
	}

public:

	TID3TextGuard& operator=(const TID3TextGuard&) = delete;
	TID3TextGuard(const TID3TextGuard&) = delete;

	explicit TID3TextGuard(pointer_p F) : pointer(F) {}
	~TID3TextGuard() { free(); }
};

template<typename T>
class TID3CommentGuard {
private:
	typedef T pointer_t;
	typedef pointer_t ** pointer_p;
	pointer_p pointer;

	void free() {
		free_comment_content(*pointer);
		*pointer = nil;
	}

public:

	TID3CommentGuard& operator=(const TID3CommentGuard&) = delete;
	TID3CommentGuard(const TID3CommentGuard&) = delete;

	explicit TID3CommentGuard(pointer_p F) : pointer(F) {}
	~TID3CommentGuard() { free(); }
};

template<typename T>
class TID3PictureGuard {
private:
	typedef T pointer_t;
	typedef pointer_t ** pointer_p;
	pointer_p pointer;

	void free() {
		free_apic_content(*pointer);
		*pointer = nil;
	}

public:

	TID3PictureGuard& operator=(const TID3PictureGuard&) = delete;
	TID3PictureGuard(const TID3PictureGuard&) = delete;

	explicit TID3PictureGuard(pointer_p F) : pointer(F) {}
	~TID3PictureGuard() { free(); }
};

} /* namespace music */

#endif /* ID3_H_ */
