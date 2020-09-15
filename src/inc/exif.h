/*
 * exif.h
 *
 *  Created on: 11.10.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef EXIF_H_
#define EXIF_H_

#ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#  define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>
#include <libexif/exif-data.h>
#include <string>
#include <map>
#include "templates.h"
#include "stringutils.h"
#include "variant.h"

namespace util {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TMakerNoteIndex = unsigned int;

#else

typedef unsigned int TMakerNoteIndex;

#endif

struct CExifTag {
	ExifTag tag;
	const char* descrition;
};

struct CExifItem {
	CExifTag tag;
	std::string value;
	int numeric;
};

struct CMakerNote {
	TMakerNoteIndex id;
	std::string title;
	std::string description;
	std::string value;
	int numeric;
};

#ifdef STL_HAS_TEMPLATE_ALIAS

using TExifMap = std::map<ExifTag, CExifItem>;
using TExifMapItem = std::pair<ExifTag, CExifItem>;
using TMakerNoteMap = std::map<TMakerNoteIndex, CMakerNote>;
using TMakerNoteMapItem = std::pair<TMakerNoteIndex, CMakerNote>;

#else

typedef std::map<ExifTag, CExifItem> TExifMap;
typedef std::pair<ExifTag, CExifItem> TExifMapItem;
typedef std::map<TMakerNoteIndex, CMakerNote> TMakerNoteMap;
typedef std::pair<TMakerNoteIndex, CMakerNote> TMakerNoteMapItem;

#endif


static const CExifTag EXIF_TAG_NAMES[] = {
	{ EXIF_TAG_MAKE,							"Make" },
	{ EXIF_TAG_MODEL,							"Model" },
	{ EXIF_TAG_PIXEL_X_DIMENSION,				"Pixel X Dimension" },
	{ EXIF_TAG_PIXEL_Y_DIMENSION,				"Pixel Y Dimension" },
	{ EXIF_TAG_DATE_TIME,						"Date time" },
	{ EXIF_TAG_DATE_TIME_ORIGINAL,				"Date time original" },
	{ EXIF_TAG_DATE_TIME_DIGITIZED,				"Date time digitized" },
	{ EXIF_TAG_FNUMBER,							"F-Number" },
	{ EXIF_TAG_MAX_APERTURE_VALUE,				"Max aperture value" },
	{ EXIF_TAG_FOCAL_LENGTH,					"Focal length" },
	{ EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM,		"Focal length in 35mm film" },
	{ EXIF_TAG_EXPOSURE_TIME,					"Exposure time" },
	{ EXIF_TAG_EXPOSURE_PROGRAM,				"Exposure program" },
	{ EXIF_TAG_ISO_SPEED_RATINGS,				"ISO speed ratings" },
	{ EXIF_TAG_ORIENTATION,						"Orientation" },
	{ EXIF_TAG_EXPOSURE_MODE,					"Exposure mode" },
	{ EXIF_TAG_WHITE_BALANCE,					"White balance" },
	{ EXIF_TAG_NEW_SUBFILE_TYPE,				"New subfile type" },
	{ EXIF_TAG_IMAGE_WIDTH,						"Image width" },
	{ EXIF_TAG_IMAGE_LENGTH,					"Image length" },
	{ EXIF_TAG_BITS_PER_SAMPLE,					"Bits per sample" },
	{ EXIF_TAG_COMPRESSION,						"Compression" },
	{ EXIF_TAG_PHOTOMETRIC_INTERPRETATION,		"Photometric interpretation" },
	{ EXIF_TAG_FILL_ORDER,						"Fill order" },
	{ EXIF_TAG_DOCUMENT_NAME,					"Document name" },
	{ EXIF_TAG_IMAGE_DESCRIPTION,				"Image description" },
	{ EXIF_TAG_STRIP_OFFSETS,					"Strip offsets" },
	{ EXIF_TAG_SAMPLES_PER_PIXEL,				"Samples per pixel" },
	{ EXIF_TAG_ROWS_PER_STRIP,					"Rows per strip" },
	{ EXIF_TAG_STRIP_BYTE_COUNTS,				"Strip byte counts" },
	{ EXIF_TAG_X_RESOLUTION,					"X Resolution" },
	{ EXIF_TAG_Y_RESOLUTION,					"Y Resolution" },
	{ EXIF_TAG_PLANAR_CONFIGURATION,			"Planar configuration" },
	{ EXIF_TAG_RESOLUTION_UNIT,					"Resolution unit" },
	{ EXIF_TAG_TRANSFER_FUNCTION,				"Transfer function" },
	{ EXIF_TAG_SOFTWARE,						"Software" },
	{ EXIF_TAG_ARTIST,							"Artist" },
	{ EXIF_TAG_WHITE_POINT,						"White point" },
	{ EXIF_TAG_PRIMARY_CHROMATICITIES,			"Primary chromaticities" },
	{ EXIF_TAG_SUB_IFDS,						"Sub IFDS" },
	{ EXIF_TAG_TRANSFER_RANGE,					"Transfer range" },
	{ EXIF_TAG_JPEG_PROC,						"JPEG proc" },
	{ EXIF_TAG_JPEG_INTERCHANGE_FORMAT,			"JPEG interchange format" },
	{ EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH,	"JPEG interchange format length" },
	{ EXIF_TAG_YCBCR_COEFFICIENTS,				"YCbCr coefficients" },
	{ EXIF_TAG_YCBCR_SUB_SAMPLING,				"YCbCr sub sampling" },
	{ EXIF_TAG_YCBCR_POSITIONING,				"YCbCr positioning" },
	{ EXIF_TAG_REFERENCE_BLACK_WHITE,			"Reference black white" },
	{ EXIF_TAG_XML_PACKET,						"XML packet" },
	{ EXIF_TAG_RELATED_IMAGE_FILE_FORMAT,		"Related image file format" },
	{ EXIF_TAG_RELATED_IMAGE_WIDTH,				"Related image width" },
	{ EXIF_TAG_RELATED_IMAGE_LENGTH,			"Related image length" },
	{ EXIF_TAG_CFA_REPEAT_PATTERN_DIM,			"CFA repeat pattern dim" },
	{ EXIF_TAG_CFA_PATTERN,						"CFA pattern" },
	{ EXIF_TAG_BATTERY_LEVEL,					"Battery level" },
	{ EXIF_TAG_COPYRIGHT,						"Copyright" },
	{ EXIF_TAG_IPTC_NAA,						"IPTC NAA" },
	{ EXIF_TAG_IMAGE_RESOURCES,					"Image resources" },
	{ EXIF_TAG_EXIF_IFD_POINTER,				"Exif IFD pointer" },
	{ EXIF_TAG_INTER_COLOR_PROFILE,				"Inter color profile" },
	{ EXIF_TAG_SPECTRAL_SENSITIVITY,			"Spectral sensitivity" },
	{ EXIF_TAG_GPS_INFO_IFD_POINTER,			"GPS info IFD pointer" },
	{ EXIF_TAG_OECF,							"OECF" },
	{ EXIF_TAG_TIME_ZONE_OFFSET,				"Time zone offset" },
//	{ EXIF_TAG_EXIF_VERSION,					"Exif version" },
	{ EXIF_TAG_COMPONENTS_CONFIGURATION,		"Components configuration" },
	{ EXIF_TAG_COMPRESSED_BITS_PER_PIXEL,		"Compressed bits per pixel" },
	{ EXIF_TAG_SHUTTER_SPEED_VALUE,				"Shutter speed value" },
	{ EXIF_TAG_APERTURE_VALUE,					"Aperture value" },
	{ EXIF_TAG_BRIGHTNESS_VALUE,				"Brightness value" },
	{ EXIF_TAG_EXPOSURE_BIAS_VALUE,				"Exposure bias value" },
	{ EXIF_TAG_SUBJECT_DISTANCE,				"Subject distance" },
	{ EXIF_TAG_METERING_MODE,					"Metering mode" },
	{ EXIF_TAG_LIGHT_SOURCE,					"Light source" },
	{ EXIF_TAG_FLASH,							"Flash" },
	{ EXIF_TAG_SUBJECT_AREA,					"Subject area" },
	{ EXIF_TAG_TIFF_EP_STANDARD_ID,				"TIFF EP standard ID" },
//	{ EXIF_TAG_MAKER_NOTE,						"Maker note" },
	{ EXIF_TAG_USER_COMMENT,					"User comment" },
	{ EXIF_TAG_SUB_SEC_TIME,					"Sub sec time" },
	{ EXIF_TAG_SUB_SEC_TIME_ORIGINAL,			"Sub sec time original" },
	{ EXIF_TAG_SUB_SEC_TIME_DIGITIZED,			"Sub sec time digitized" },
	{ EXIF_TAG_XP_TITLE,						"XP title" },
	{ EXIF_TAG_XP_COMMENT,						"XP comment" },
	{ EXIF_TAG_XP_AUTHOR,						"XP author" },
	{ EXIF_TAG_XP_KEYWORDS,						"XP keywords" },
	{ EXIF_TAG_XP_SUBJECT,						"XP subject" },
	{ EXIF_TAG_FLASH_PIX_VERSION,				"FlashPix version" },
	{ EXIF_TAG_COLOR_SPACE,						"Color space" },
	{ EXIF_TAG_RELATED_SOUND_FILE,				"Related sound file" },
	{ EXIF_TAG_INTEROPERABILITY_IFD_POINTER,	"Interoperability IFD pointer" },
	{ EXIF_TAG_FLASH_ENERGY,					"Flash energy" },
	{ EXIF_TAG_SPATIAL_FREQUENCY_RESPONSE,		"Spatial frequency response" },
	{ EXIF_TAG_FOCAL_PLANE_X_RESOLUTION,		"Focal plane X resolution" },
	{ EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION,		"Focal plane Y resolution" },
	{ EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT,		"Focal plane resolution unit" },
	{ EXIF_TAG_SUBJECT_LOCATION,				"Subject location" },
	{ EXIF_TAG_EXPOSURE_INDEX,					"Exposure index" },
	{ EXIF_TAG_SENSING_METHOD,					"Sensing method" },
	{ EXIF_TAG_FILE_SOURCE,						"File source" },
	{ EXIF_TAG_SCENE_TYPE,						"Scene type" },
	{ EXIF_TAG_NEW_CFA_PATTERN,					"New CFA pattern" },
	{ EXIF_TAG_CUSTOM_RENDERED,					"Custom rendered" },
	{ EXIF_TAG_DIGITAL_ZOOM_RATIO,				"Digital zoom ratio" },
	{ EXIF_TAG_SCENE_CAPTURE_TYPE,				"Scene capture type" },
	{ EXIF_TAG_GAIN_CONTROL,					"Gain control" },
	{ EXIF_TAG_CONTRAST,						"Contrast" },
	{ EXIF_TAG_SATURATION,						"Saturation" },
	{ EXIF_TAG_SHARPNESS,						"Sharpness" },
	{ EXIF_TAG_DEVICE_SETTING_DESCRIPTION,		"Device setting description" },
	{ EXIF_TAG_SUBJECT_DISTANCE_RANGE,			"Subject distance range" },
	{ EXIF_TAG_IMAGE_UNIQUE_ID,					"Image unique ID" },
	{ EXIF_TAG_GAMMA,							"Gamma" },
//	{ EXIF_TAG_PRINT_IMAGE_MATCHING,			"Print image matching" },
	{ EXIF_TAG_INTEROPERABILITY_INDEX,			"Interoperability index" },
	{ EXIF_TAG_INTEROPERABILITY_VERSION,		"Interoperability version" },
//	{ EXIF_TAG_PADDING,							"Padding" },
	{ (ExifTag)-1,								nil },
};


class TExifReader {
private:
	ExifData *data;

public:
	bool isValid() const { return util::assigned(data); };
	ExifData * operator () () { return data; };
	void clear();

	TExifReader(const std::string& fileName);
	virtual ~TExifReader();
};


class TExif {
private:
	enum EExifFormat { EXF_CSV, EXF_TEXT, EXF_DEFAULT = EXF_TEXT };

	std::string file;
	TExifMap tags;
	TMakerNoteMap notes;
	mutable TStringList text;
	mutable TStringList json;
	size_t align;
	size_t count;

	bool readTags(TExifReader& reader);
	bool readTag(TExifReader& reader, ExifIfd ifd, ExifTag tid, std::string& value);
	void addTag(const CExifTag& tag, const std::string& value);

	size_t alignment() const { return align; };
	bool isExifTime(const std::string& value) const;
	void normalizeExifTime(std::string& value) const ;

	std::string formatAsText(const CExifItem& item, const std::string separator, size_t align = 0, bool asText = false) const;
	std::string formatAsJson(const CExifItem& item, const std::string preamble = "") const;

	bool readMakerNoteTag(TExifReader& reader, ExifIfd ifd, TMakerNoteIndex mid, std::string& key, std::string& value);
	bool readMakerNotes(TExifReader& reader);
	void addNote(const TMakerNoteIndex mid,const std::string& title, const std::string& value);

	void writeExifDataAsText(util::TStringList& list, const EExifFormat fmt, const std::string& preamble, size_t align) const;
	void writeExifDataAsJSON(util::TStringList& list, const std::string& name = "", const bool asObject = false) const;
	void saveToFile(util::TStringList& list, const std::string& fileName, const EExifFormat fmt, const std::string& preamble);

public:
	void clear();

	bool hasTags() const { return !tags.empty(); };
	bool hasNotes() const { return !notes.empty(); } ;
	bool getCount() const { return count; };
	bool getTag(CExifItem& item, ExifTag tid) const;
	bool getNote(CMakerNote& item, TMakerNoteIndex mid) const;

	const TStringList& asText(const EExifFormat fmt = EXF_TEXT, const std::string& preamble = "") const;
	const TStringList& asJSON(const std::string& name = "", const bool asObject = false) const;
	bool asVariants(util::TVariantValues& variants) const;

	size_t loadFromFile(const std::string& fileName);
	void saveAsCsv(const std::string& fileName);
	void saveAsText(const std::string& fileName);
	size_t saveFilesAsText(const std::string& files, const ESearchDepth depth = SD_ROOT);

	void debugOutput();

	TExif();
	TExif(const std::string& fileName);
	virtual ~TExif();
};

} /* namespace util */

#endif /* EXIF_H_ */
