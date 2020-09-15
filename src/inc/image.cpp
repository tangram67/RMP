/*
 * image.cpp
 *
 *  Created on: 19.10.2015
 *      Author: Dirk Brinkmeier
 */

#include "image.h"
#include "fileutils.h"
#include "mimetypes.h"

namespace util {

TImage::TImage() {
	type = IMG_UNKOWN;
}

TImage::TImage(const std::string& file) : file(file) {
	type = getImageType(file);
}

TImage::~TImage() {
}


std::string TImage::getFileExt(const TImage::EImageType type) {
	switch (type) {
		case IMG_JPEG:
			return "jpg";
		case IMG_PNG:
 			return "png";
		case IMG_BMP:
			return "bmp";
		default:
			return "";
	}
}


TImage::EImageType TImage::getImageType(const std::string& fileName) {
	std::string ext = fileExt(fileName);
	if (!ext.empty()) {
		std::string mime = getMimeType(ext);
		if (mime == JPEG_MIME_TYPE)
			return IMG_JPEG;
		if (mime == PNG_MIME_TYPE)
			return IMG_PNG;
		if (mime == BMP_MIME_TYPE)
			return IMG_BMP;
	}
	return IMG_UNKOWN;
}

void TImage::clear() {
	TImageData::clear();
	jpg.clear();
	png.clear();
	dib.clear();
}

void TImage::setFile(const std::string& fileName) {
	clear();
	file = fileName;
	type = getImageType(file);
};


bool TImage::loadFromFile() {
	clear();
	bool retVal = false;
	if (isValid() && fileExists(file)) {
		switch (type) {
			case IMG_JPEG:
				retVal = (0 < jpg.decode(file, image, imageWidth, imageHeight));
				break;
			case IMG_PNG:
				retVal = (0 < png.decode(file, image, imageWidth, imageHeight));
				break;
			case IMG_BMP:
				retVal = (0 < dib.decode(file, image, imageWidth, imageHeight));
				break;
			default:
				break;
		}
	}
	return retVal;
}

bool TImage::loadFromFile(const std::string& fileName) {
	setFile(fileName);
	return loadFromFile();
}

bool TImage::saveToFile(const EImageType type) {
	bool retVal = false;
	if (!file.empty() && hasImage()) {
		retVal = saveToFile(file, type);
	}
	return retVal;
}

bool TImage::saveToFile(const std::string& fileName, const EImageType type) {
	bool retVal = false;
	if (!fileName.empty() && hasImage()) {
		std::string fn = (type == IMG_DEFAULT) ? fileName : util::fileReplaceExt(fileName, getFileExt(type));
		EImageType tp = (type == IMG_DEFAULT) ? this->type : type;
		switch (tp) {
			case IMG_JPEG:
				retVal = ((size_t)0 < jpg.encode(fn, image, imageWidth, imageHeight));
				break;
			case IMG_PNG:
				retVal = ((size_t)0 < png.encode(fn, image, imageWidth, imageHeight));
				break;
			case IMG_BMP:
				retVal = ((size_t)0 < dib.encode(fn, image, imageWidth, imageHeight));
				break;
			default:
				break;
		}
	}
	return retVal;
}

int TImage::getColorRange() {
	util::TBitmapScaler bs;
	return bs.getColorRange(image, imageWidth, imageHeight);
}

const TExif& TImage::getExifData() {
	readExifData(file);
	return exif;
}

const TExif& TImage::getExifData(const std::string& fileName) {
	readExifData(fileName);
	return exif;
}

void TImage::readExifData(const std::string& fileName) {
	exif.clear();
	if (fileExists(fileName)) {
		exif.loadFromFile(fileName);
	}
}


} /* namespace util */
