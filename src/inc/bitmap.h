/*
 * bitmap.h
 *
 *  Created on: 23.09.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef BITMAP_H_
#define BITMAP_H_

#define PNG_DEBUG 3
#include <png.h>
#include <string>
#include <csetjmp>
#include "jpegtypes.h"
#include "templates.h"
#include "fileutils.h"
#include "endianutils.h"
#include "windows.h"
#include "memory.h"
#include "gcc.h"
#include "bmp.h"
#include "cie/colortypes.h"

namespace util {

class TJpegAction;
class TJpegDecoder;
class TJpegEncoder;
class TPNGAction;
class TColor;

#ifdef STL_HAS_TEMPLATE_ALIAS

using THeaderID = CHAR[4];
using TFillerID = CHAR[2];
using PJpegAction = TJpegAction*;
using PPNGAction = TPNGAction*;
using TFilterMatrix = std::vector<double>;
using TColorMap = std::map<std::string, TColor>;
using TColorItem = std::pair<std::string, TColor>;

#else

typedef CHAR THeaderID[2];
typedef CHAR TFillerID[2];
typedef TJpegAction* PJpegAction;
typedef TPNGAction* PPNGAction;
typedef std::vector<double> TFilterMatrix;
typedef std::map<std::string, CColor> TColorMap;
typedef std::pair<std::string, CColor> TColorItem;

#endif


#define MAX_BITMAP_SIZE 5000
#define BYTES_PER_PIXEL 3
#define JPEG_ERROR_MSG_LEN JMSG_LENGTH_MAX+1
#define STD_PNG_BIT_DEPTH 8
#define STD_PNG_COLOR_TYPE 2
#define MAX_EXIF_HEADER_SIZE 16536


enum EScalingMethod {
	ESM_SIMPLE,
	ESM_BILINEAR,
	ESM_DITHER,
	ESM_DEFAULT
};


enum EOverlapMode {
	EOM_OVERLAP_NONE,   // No line overlap, like in standard Bresenham
	EOM_OVERLAP_MAJOR,  // Overlap: First go major then minor direction, overlapping pixel is drawn after current line
	EOM_OVERLAP_MINOR,  // Overlap: First go minor then major direction, overlapping pixel is drawn before next line
	EOM_OVERLAP_BOTH,   // Overlap: Use both methods
	EOM_OVERLAP_DEFAULT = EOM_OVERLAP_NONE
};

enum ELineMode {
	ELM_DRAW_MIDDLE,            // Start point is on the line at center of the thick line
	ELM_DRAW_CLOCKWISE,         // Start point is on the counter clockwise border line
	ELM_DRAW_COUNTERCLOCKWISE,  // Start point is on the clockwise border line
	ELM_DRAW_DEFAULT = ELM_DRAW_MIDDLE
};


// Exif file header
typedef struct CExifFileHeader {
	THeaderID	exJpegID;
	WORD		exSize;
	THeaderID	exExifID;
	TFillerID	exFiller;
} PACKED TExifFileHeader;


// "Inherit" own error manager from jpeg_error_mgr
struct CJpegErrorManager {
    struct jpeg_error_mgr manager;		// Standard libjpeg error manager
    jmp_buf longjump;					// Buffer to execute longjmp()
    char message[JPEG_ERROR_MSG_LEN];	// Last libjpeg error message
    PJpegAction caller;					// Pointer to calling class

    CJpegErrorManager() {
    	memset(message, 0, JPEG_ERROR_MSG_LEN);
    	caller = nil;
    }
};


struct CPNGErrorManager {
    jmp_buf longjump;		// Buffer to execute longjmp()
    std::string message;	// Last libjpng error message
    PPNGAction caller;		// Pointer to calling class

    CPNGErrorManager() {
    	caller = nil;
    }
};


// RGBA Pixel properties
class TColor {
public:
	TRGB red;
	TRGB green;
	TRGB blue;
	TRGB alpha;

	bool compare(const TColor& value) const {
		return red == value.red &&
			   green == value.green &&
			   blue == value.blue &&
			   alpha == value.alpha;
	}

	bool operator == (const TColor& value) const {
		return compare(value);
	}

	bool operator != (const TColor& value) const {
		return !compare(value);
	}

	TColor& operator = (const TColor& value) {
		red   = value.red;
		green = value.green;
		blue  = value.blue;
		alpha = value.alpha;
		return *this;
	}
};


STATIC_CONST TColor CL_NONE    = { 0x01, 0x02, 0x03, 0x04 };

STATIC_CONST TColor CL_BLACK   = { 0x00, 0x00, 0x00, 0x00 };
STATIC_CONST TColor CL_GRAY    = { 0xD4, 0xD4, 0xD4, 0x00 };
STATIC_CONST TColor CL_WHITE   = { 0xFF, 0xFF, 0xFF, 0x00 };
STATIC_CONST TColor CL_RED     = { 0xFF, 0x00, 0x00, 0x00 };
STATIC_CONST TColor CL_YELLOW  = { 0xFF, 0xFF, 0x00, 0x00 };
STATIC_CONST TColor CL_GREEN   = { 0x00, 0xFF, 0x00, 0x00 };
STATIC_CONST TColor CL_CYAN    = { 0x00, 0xFF, 0xFF, 0x00 };
STATIC_CONST TColor CL_BLUE    = { 0x00, 0x00, 0xFF, 0x00 };

STATIC_CONST TColor CL_DEFAULT = { 0x77, 0x77, 0x77, 0x00 };
STATIC_CONST TColor CL_PRIMARY = { 0x33, 0x7A, 0xB7, 0x00 };
STATIC_CONST TColor CL_SUCCESS = { 0x5C, 0xB8, 0x5C, 0x00 };
STATIC_CONST TColor CL_INFO    = { 0x5B, 0xC0, 0xDE, 0x00 };
STATIC_CONST TColor CL_WARNING = { 0xF0, 0xAD, 0x4E, 0x00 };
STATIC_CONST TColor CL_DANGER  = { 0xD9, 0x53, 0x4F, 0x00 };

STATIC_CONST TColor colors[] = { CL_BLACK, CL_GRAY, CL_WHITE, CL_RED, CL_YELLOW, CL_GREEN, CL_CYAN, CL_BLUE,
                                 CL_DEFAULT, CL_PRIMARY, CL_SUCCESS, CL_INFO, CL_WARNING, CL_DANGER };

bool getColorByName(const std::string& name, TColor& color);
bool addColorByName(const std::string& name, const TColor& color);


class TBitmapPainter {
private:
	void pixel555to888(const uint16_t *image16, TRGB *image24);
	void pixel565to888(const uint16_t *image16, TRGB *image24);

	void drawOverlappedLine(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, const EOverlapMode overlap, const TColor& color);
	void drawThickLineSimple(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, int outline, const ELineMode mode, const TColor& color);
	void drawThickLine(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, int outline, ELineMode mode, const TColor& color);

protected:
	bool validate(int x, int y, int sx, int sy);
	void ink(TRGB*& p, const TColor& color);
	void plot(TRGBData& image, int x, int y, int sx, int sy, const TColor& color);
	void bezier2s(TRGBData& image, int x0, int y0, int x1, int y1, int x2, int y2, int sx, int sy, const TColor& color);

public:
	void fill(TRGBData& image, int width, int height, const TColor& color);
	void line(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, const TColor& color);
	void line(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, int outline, const TColor& color);
	void circle(TRGBData& image, int x0, int y0, int radius, int sx, int sy, const TColor& color);
	void ellipse(TRGBData& image, int x0, int y0, int a, int b, int sx, int sy, const TColor& color);
	void bezier2(TRGBData& image, int x0, int y0, int x1, int y1, int x2, int y2, int sx, int sy, const TColor& color);
	void rect(TRGBData& image, int x0, int y0, int width, int height, int sx, int sy, int outline, const TColor& color);
	void square(TRGBData& image, int x0, int y0, int width, int height, int sx, int sy, const TColor& color);
	void draw(TRGBData& image, int x, int y, int sx, int sy, const TColor& color);

	bool convert555to888(const TRGBData& image16, TRGBData& image24);
	bool convert565to888(const TRGBData& image16, TRGBData& image24);

	TBitmapPainter();
	virtual ~TBitmapPainter() = default;
};


class TColorPicker {
public:
	TColor gradient(const double percent, const TColor& from, const TColor& to);
	TColor rainbow(const double percent);
	TColor colors(const double percent);
	TColor circular(const double percent);

	const CColorSystem * getColorSystem(const char* system);
	const CColorSystem * getColorSystem(const std::string& system);
	TColor temperature(const double percent, const double from, const double to, const CColorSystem *system);
	TColor temperature(const double percent, const double from, const double to, const std::string& name = "");
	TColor temperature(const double percent, const double from, const double to, const char* name = nil);
	TColor temperature(const double temperature, const CColorSystem *system);
	TColor temperature(const double temperature, const std::string& system);
	TColor temperature(const double temperature, const char* system);
};


class TBitmapScaler : protected TBitmapPainter {
private:
	bool checkBuffer(const TRGBData& buffer, int width, int height, const std::string location);
	void convert32to15(TRGB r, TRGB g , TRGB b , TRGB* d);
	void copy(TRGB* dst, const TRGB* src, size_t n);
	void normalizePixelValue(TRGB *pixel, const double& component, const double& sum);

public:
	void rgbToAlpha(const TRGBData& src, TRGBData& dst, int width, int height);
	void reverse(const TRGBData& src, TRGBData& dst, int width, int height);
	void mirrorX(const TRGBData& src, TRGBData& dst, int width, int height);
	void mirrorY(const TRGBData& src, TRGBData& dst, int width, int height);
	void rotate90(const TRGBData& src, TRGBData& dst, int width, int height);
	void rotate180(const TRGBData& src, TRGBData& dst, int width, int height);
	void rotate270(const TRGBData& src, TRGBData& dst, int width, int height);

	PRGB resizeSimple(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy);
	PRGB resizeBilinear(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy);
	PRGB resizeColorAverage(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy);
	PRGB overlay(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy, const TColor& color);

	PRGB convert888to555Simple(const TRGBData& src, TRGBData& dst, int sx, int sy);
	PRGB convert888to555Dithered(const TRGBData& src, TRGBData& dst, int sx, int sy);

	int getColorRange(const TRGBData& src, int sx, int sy);

	PRGB filter(const TRGBData& src, TRGBData& dst, int sx, int sy, const TFilterMatrix& matrix);
	PRGB contrast(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor = 0.333);
	PRGB brightness(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor = 0.2);
	PRGB saturation(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor);
	PRGB blur(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor = 0.2);
	PRGB edgy(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor = 1.0);
	PRGB monochrome(const TRGBData& src, TRGBData& dst, int sx, int sy);
	PRGB negative(const TRGBData& src, TRGBData& dst, int sx, int sy);

	TBitmapScaler();
	virtual ~TBitmapScaler() = default;
};


class TImageData {
protected:
	TRGBData image;
	TRGBSize imageWidth;
	TRGBSize imageHeight;

public:
	void clear();

	const TRGBData& getImage() const { return image; };
	size_t getSize() const { return image.size(); };
	TRGBSize width() const { return imageWidth; };
	TRGBSize height() const { return imageHeight; };

	bool hasImage() const { return (image.size() > 0); };
	operator bool () const { return hasImage(); };

	TImageData();
	virtual ~TImageData() = default;
};


class TPicture : public TImageData, public TBitmapScaler {
private:
	bool downsampling;
	EScalingMethod scaling;

	void prime();
	double ratio(const TRGBSize width, const TRGBSize height);
	PRGB resizer(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy);

protected:
	virtual size_t decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height);
	virtual size_t encode(const std::string& fileName, const TRGBData& data, const TRGBSize width, const TRGBSize height);

public:
	void move(TPicture& picture);
	void assign(const TPicture& picture);
	double getRatio() { return ratio(imageWidth, imageHeight); };
	int getColorRange();

	bool getDownsampling() const { return downsampling; };
	EScalingMethod getScalingMethod() const { return scaling; };
	void setDownsampling(const bool value) { downsampling = value; };
	void setScalingMethod(const EScalingMethod value) { scaling = value; };

	bool resizeX(const TRGBSize targetWidth);
	bool resizeY(const TRGBSize targetHeight);
	bool resize(const TRGBSize targetWidth, const TRGBSize targetHeight);

	bool align(const TRGBSize dimension, const TColor& color);
	bool scale(const TRGBSize dimension, const TColor& color);

	void mirrorX();
	void mirrorY();
	void rotate90();
	void rotate180();
	void rotate270();

	void filter(const TFilterMatrix& matrix);
	void contrast(double factor = 0.333);
	void brightness(double factor = 0.2);
	void saturation(double factor = 1.2);
	void blur(double factor = 0.2);
	void edgy(double factor = 1.0);
	void monochrome();
	void negative();

	TPicture& operator = (const TPicture& picture);

	TPicture();
	TPicture(const TPicture& o);
	TPicture(TPicture&& o);
	virtual ~TPicture() = default;
};



class TPictureFile : public TPicture, public TColorPicker {
private:
	std::string file;

public:
	std::string getFile() const { return file; };
	bool validFile() const { return !file.empty(); };
	void setFile(const std::string& fileName) { file = fileName; };

	void fill(int width, int height, const TColor& color);
	void line(int x0, int y0, int x1, int y1, const TColor& color);
	void line(int x0, int y0, int x1, int y1, int outline, const TColor& color);
	void circle(int x0, int y0, int radius, const TColor& color);
	void rect(int x0, int y0, int width, int height, int outline, const TColor& color);
	void square(int x0, int y0, int width, int height, const TColor& color);
	void ellipse(int x0, int y0, int a, int b, const TColor& color);
	void bezier2(int x0, int y0, int x1, int y1, int x2, int y2, const TColor& color);
	void draw(int x, int y, const TColor& color);

	void loadFromFile();
	void loadFromFile(const std::string& fileName);
	size_t saveToFile();
	size_t saveToFile(const std::string& fileName);

	TPictureFile();
	TPictureFile(const std::string& file);
	virtual ~TPictureFile() = default;
};



class TBitmap : public TPictureFile, private TEndian {
private:
	bool isMSDIB(const BITMAPFILEHEADER& header) const;
	void setHeaderEndian(BITMAPINFOHEADER& header) const;

public:
	size_t decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height);
	size_t encode(const std::string& fileName, const TRGBData& data, const TRGBSize width, const TRGBSize height);
	size_t encode(char*& buffer, size_t& size, const TRGBData& data, const TRGBSize width, const TRGBSize height);
	size_t encode(char*& buffer, size_t& size);

	TBitmap();
	TBitmap(const std::string& file);
	virtual ~TBitmap() = default;
};




class TJpegError {
protected:
	struct jpeg_error_mgr * manager;
	struct CJpegErrorManager error;

public:
	struct jpeg_error_mgr * getErrorManager() { return manager; };
	std::string getLastError();

	jmp_buf& getLongjump() { return error.longjump; };
	bool setLongjump() { return EXIT_SUCCESS == setjmp(getLongjump()); };

	TJpegError();
	virtual ~TJpegError() = default;
};


class TJpegAction : public TJpegError {
protected:
	bool created;

public:
	bool isCreated() const { return created; };
	virtual void clear() = 0;

	TJpegAction();
	virtual ~TJpegAction() = default;
};


class TJpegDecoder : public TJpegAction {
private:
	struct jpeg_decompress_struct decoder;
	JDIMENSION limit;

public:
	void clear();
	bool isValid();
	JDIMENSION getLimit() const { return limit; };
	void setLimit(const JDIMENSION value) { limit = value; };
	struct jpeg_decompress_struct * operator () () { return &decoder; };

	TJpegDecoder();
	virtual ~TJpegDecoder();
};


class TJpegEncoder : public TJpegAction {
private:
	struct jpeg_compress_struct encoder;

public:
	struct jpeg_compress_struct * operator () () { return &encoder; };
	void clear();

	TJpegEncoder();
	virtual ~TJpegEncoder();
};


class TJpeg : public TPictureFile, private TEndian {
private:
	int quality;

	void prime();
	bool isJPEG(const TRGBData& data) const;
	bool read(const util::TBaseFile& file, void *const data, const size_t size) const;
	bool seek(const util::TBaseFile& file, const size_t offset) const;

public:
	static bool isJPEG(const TRGB* data, size_t size);
	static bool isJFIF(const TRGB* data, size_t size);
	static bool isExif(const TRGB* data, size_t size);

	int readExifOrientation(const std::string& fileName) const;
	int readExifOrientation() const;
	bool realign(int& orientation);

	void setQuality(int value) { quality = value; };
	int getQuality() const { return quality; };

	size_t decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height);
	size_t decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height, const TRGBSize targetWidth, const TRGBSize targetHeight);
	size_t decode(const char* buffer, const size_t size, TRGBData& data, TRGBSize& width, TRGBSize& height, const TRGBSize targetWidth, const TRGBSize targetHeight);
	size_t decode(const char* buffer, const size_t size);
	size_t decode(const TBuffer& buffer);
	size_t decode(const TFile& file);

	size_t encode(const std::string& fileName, const TRGBData& data, const TRGBSize width, const TRGBSize height);
	size_t encode(char*& buffer, size_t& size, const TRGBData& data, const TRGBSize width, const TRGBSize height);
	size_t encode(char*& buffer, size_t& size, const TPicture& bitmap);
	size_t encode(char*& buffer, size_t& size);

	void loadPrescaledFromFile(const std::string& fileName, const TRGBSize targetWidth, const TRGBSize targetHeight);

	void loadFromBitmap(const std::string& fileName);
	void saveAsBitmap(const std::string& fileName);
	void saveAsBitmap();

	TJpeg();
	TJpeg(const std::string& file);
	virtual ~TJpeg() = default;
};



class TPNGError {
protected:
	CPNGErrorManager error;

public:
	std::string getLastError();

	jmp_buf& getLongjump() { return error.longjump; };
	bool setLongjump() { return EXIT_SUCCESS == setjmp(getLongjump()); };

	TPNGError();
	virtual ~TPNGError() = default;
};


class TPNGAction : public TPNGError {
protected:
	bool created;
	png_structp pimage;
	png_infop pinfo;
	virtual void clear() = 0;

public:
	bool isCreated() const { return created; };
	png_structp image() { return pimage; };
	png_infop info() { return pinfo; };

	TPNGAction();
	virtual ~TPNGAction() = default;
};



class TPNGDecoder : public TPNGAction {
public:
	void clear();
	TPNGDecoder();
	virtual ~TPNGDecoder();
};


class TPNGEncoder : public TPNGAction {
public:
	void clear();
	TPNGEncoder();
	virtual ~TPNGEncoder();
};


class TPNGData {
	size_t sx, height;
	png_bytepp ptr;

	void prime();
	void destroy();
	bool validIndex(const size_t index) const;
	void create(const size_t rowSize, const size_t imageHeigth);

public:
	size_t size() const;
	bool hasData() const { return util::assigned(ptr); }
	png_bytepp operator () () { return ptr; };
	png_bytep operator [] (const std::size_t index) const;
	void resize(const size_t rowSize, const size_t imageHeigth);
	void clear();

	TPNGData();
	TPNGData(const size_t rowSize, const size_t imageHeigth);
	virtual ~TPNGData();
};


struct TPNGDataHandler {
	TStdioFile* file;

	// Raw RGB data
	TRGB* data;
	size_t size;

	// Read/write counter
	size_t read;
	size_t written;

	void clear() {
		file = nil;
		data = nil;
		size = 0;
		read = 0;
		written = 0;
	}

	TPNGDataHandler() {
		clear();
	}
};


class TPNG : public TPictureFile {
private:
	static bool isPNG(const TRGBData& data);
	static bool isPNG(const png_bytep data, const png_size_t size);

	void rowcopy(png_bytep dst, const TRGB* src, size_t n);

	size_t decode(TPNGDecoder& decoder, TPNGDataHandler& reader, TRGBData& data, TPNGData& rows, int& px, int& py);
	size_t encode(TPNGEncoder& encoder, TPNGDataHandler& writer, const TRGBData& data, TPNGData& rows, const int px, const int py);

public:
	static bool isPNG(const TRGB* data, const size_t size);

	size_t decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height);
	size_t decode(const char* buffer, const size_t size, TRGBData& data, TRGBSize& width, TRGBSize& height);
	size_t decode(const char* buffer, const size_t size);
	size_t decode(const TBuffer& buffer);
	size_t decode(const TFile& file);

	size_t encode(const std::string& fileName, const TRGBData& data, const TRGBSize width, const TRGBSize height);
	size_t encode(char*& buffer, size_t& size, const TRGBData& data, const TRGBSize width, const TRGBSize height);
	size_t encode(char*& buffer, size_t& size, const TPicture& bitmap);
	size_t encode(char*& buffer, size_t& size);

	void loadFromBitmap(const std::string& fileName);
	void saveAsBitmap(const std::string& fileName);
	void saveAsBitmap();

	TPNG();
	TPNG(const std::string& file);
	virtual ~TPNG() = default;
};



template<typename T>
class TArrayBufferGuard {
private:
	typedef T pointer_t;
	typedef pointer_t ** pointer_p;
	pointer_p pointer;

	void free() {
		if (*pointer != nil) {
			delete[] *pointer;
			*pointer = nil;
		}
	}

public:
	TArrayBufferGuard& operator=(const TArrayBufferGuard&) = delete;
	TArrayBufferGuard(const TArrayBufferGuard&) = delete;

	explicit TArrayBufferGuard(pointer_p F) : pointer {F} {}
	~TArrayBufferGuard() { free(); }
};


} /* namespace util */

#endif /* BITMAP_H_ */
