/*
 * image.h
 *
 *  Created on: 19.10.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef IMAGE_H_
#define IMAGE_H_

#include "bitmap.h"
#include "exif.h"

namespace util {

class TImage : public TImageData {
public:
	enum EImageType { IMG_UNKOWN, IMG_DEFAULT, IMG_BMP, IMG_JPEG, IMG_PNG };

private:
	std::string file;
	EImageType type;

	TBitmap dib;
	TJpeg jpg;
	TPNG png;
	TExif exif;

	EImageType getImageType(const std::string& fileName);
	std::string getFileExt(const TImage::EImageType type);
	void readExifData(const std::string& fileName);

public:
	void clear();

	void setFile(const std::string& fileName);
	std::string getFile() const { return file; };
	EImageType getType() const { return type; };

	int getColorRange();
	const TExif& getExifData();
	const TExif& getExifData(const std::string& fileName);

	bool validFileName() const { return !file.empty(); };
	bool validFileType() const { return type != IMG_UNKOWN; };
	bool isValid() const { return validFileName() && validFileType(); };

	bool loadFromFile();
	bool loadFromFile(const std::string& fileName);
	bool saveToFile(const EImageType type = IMG_DEFAULT);
	bool saveToFile(const std::string& fileName, const EImageType type = IMG_DEFAULT);

	TImage();
	TImage(const std::string& file);
	virtual ~TImage();
};


} /* namespace util */

#endif /* IMAGE_H_ */
