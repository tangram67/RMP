/*
 * mimetypes.h
 *
 *  Created on: 05.02.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef MIMETYPES_H_
#define MIMETYPES_H_

#include "gcc.h"
#include "nullptr.h"
#include <string>
#include <map>


STATIC_CONST char CSS_MIME_TYPE[]     = "text/css";
STATIC_CONST char HTML_MIME_TYPE[]    = "text/html";
STATIC_CONST char TEXT_MIME_TYPE[]    = "text/plain";
STATIC_CONST char XML_MIME_TYPE[]     = "text/xml";
STATIC_CONST char JAVA_MIME_TYPE[]    = "application/javascript";
STATIC_CONST char JSON_MIME_TYPE[]    = "application/json";
STATIC_CONST char JPEG_MIME_TYPE[]    = "image/jpeg";
STATIC_CONST char PNG_MIME_TYPE[]     = "image/png";
STATIC_CONST char BMP_MIME_TYPE[]     = "image/bmp";
STATIC_CONST char OGG_MIME_TYPE[]     = "audio/ogg";
STATIC_CONST char MP3_MIME_TYPE[]     = "audio/mpeg3";
STATIC_CONST char M3U_MIME_TYPE[]     = "audio/x-mpequrl";
STATIC_CONST char PLS_MIME_TYPE[]     = "audio/x-scpls";
STATIC_CONST char XSPF_MIME_TYPE[]    = "application/xspf+xml";
STATIC_CONST char DEFAULT_MIME_TYPE[] = "application/octet-stream";


namespace util {

class TFile;

#ifdef STL_HAS_TEMPLATE_ALIAS

using TMimeMap = std::map<std::string, std::string>;
using TMimeItem = std::pair<std::string, std::string>;

#else

typedef std::map<std::string, std::string> TMimeMap;
typedef std::pair<std::string, std::string> TMimeItem;

#endif

struct TWebMediaType {
	const char* extension;
	const char* type;
	const char* codec;
};

static const struct TWebMediaType multimediatypes[] = {
	{ "mp3",  "audio/mpeg",    nil           },
	{ "ogg",  "audio/ogg",    "vorbis, opus" },
	{ "webm", "audio/webm",   "vorbis"       },
	{ "flac", "audio/x-flac",  nil           },
	{ "wav",  "audio/wav",    "1"            },
	{ "m4a",  "audio/mp4",    "mp4a.40.2"    },
	{ "aac",  "audio/mp4",    "mp4a.40.2"    },
	{  nil,    nil,            nil           }
};

bool isMimeType(const std::string& extension, const char* mimetype);
bool isMimeType(const std::string& extension, const std::string& mimetype);
std::string getMimeType(const std::string& extension);
std::string getMimeType(const util::TFile& file);
int loadMimeTypesFromFile(const std::string& fileName);
const TMimeMap& getMimeMapLocal();
const TMimeMap& getMimeMapExtrn();

bool getAudioCodec(const std::string extension, std::string& type, std::string& codec);

} /* namespace app */

#endif /* MIMETYPES_H_ */
