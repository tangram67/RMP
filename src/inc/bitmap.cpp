/*
 * bitmap.cpp
 *
 *  Created on: 23.09.2015
 *      Author: Dirk Brinkmeier
 */


#include <memory>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <jpeglib.h>
#include "ansi.h"
#include "exif.h"
#include "bitmap.h"
#include "templates.h"
#include "exception.h"
#include "endianutils.h"
#include "cie/colorspace.h"


void jpegErrorCallback(j_common_ptr cinfo) {
    // Get error manager, set before by caller
    util::CJpegErrorManager* error = (util::CJpegErrorManager*)cinfo->err;

    // Get libjpeg error message information
    (*(cinfo->err->format_message))(cinfo, error->message);

    // Get access to calling class...
    error->caller = reinterpret_cast<util::PJpegAction>(cinfo->client_data);

    // Return to calling class method
    longjmp(error->longjump, 1);
}


void pngErrorCallback(png_structp image, png_const_charp msg) {
	png_voidp error = png_get_error_ptr(image);
	if (util::assigned(error)) {
	    util::CPNGErrorManager* manager = (util::CPNGErrorManager*)error;
	    size_t size = strnlen(msg, 1024);
	    if (size)
	    	manager->message = std::string(msg, size);
	    else
	    	manager->message = "Missing libPNG message.";

	    // Return to calling class method
	    longjmp(manager->longjump, 1);
	}

	// Fatal error, no access to caller!
	std::cout << app::red << "[libPNG] Fatal: Missing caller reference, error callback failed." << app::reset << std::endl;
	throw util::app_error("[libPNG] Fatal: Missing caller reference, error callback failed.");
}

void pngWarningCallback(png_structp image, png_const_charp msg) {
	png_voidp error = png_get_error_ptr(image);
	if (util::assigned(error)) {
	    util::CPNGErrorManager* manager = (util::CPNGErrorManager*)error;
	    size_t size = strnlen(msg, 1024);
	    if (size)
	    	manager->message = std::string(msg, size);
	    else
	    	manager->message = "Missing libPNG message.";

		// Log to stdout
    	std::cout << app::yellow << "[libPNG] Warning: " << manager->message << app::reset << std::endl;
	}
	return;
}


namespace util {

/*
 * See: http://www.graficaobscura.com/matrix/index.html
 * Where rwgt is 0.3086, gwgt is 0.6094, and bwgt is 0.0820. This is the luminance vector.
 * Notice here that we do not use the standard NTSC weights of 0.299, 0.587, and 0.114.
 * The NTSC weights are only applicable to RGB colors in a gamma 2.2 color space.
 * For linear RGB colors the values above are better.
 */
#define USE_LINEAR_COLORS

// Weighted average based on linear RGB colors
STATIC_CONST double COLOR_WEIGHT_LINEAR_R UNUSED = 0.3086;
STATIC_CONST double COLOR_WEIGHT_LINEAR_G UNUSED = 0.6094;
STATIC_CONST double COLOR_WEIGHT_LINEAR_B UNUSED = 0.0820;

// Weighted average based on ITU Rec.709
STATIC_CONST double COLOR_WEIGHT_ITU709_R UNUSED = 0.2126;
STATIC_CONST double COLOR_WEIGHT_ITU709_G UNUSED = 0.7152;
STATIC_CONST double COLOR_WEIGHT_ITU709_B UNUSED = 0.0722;

// Weighted average based on NTSC standard
STATIC_CONST double COLOR_WEIGHT_NTSC_R UNUSED = 0.299;
STATIC_CONST double COLOR_WEIGHT_NTSC_G UNUSED = 0.587;
STATIC_CONST double COLOR_WEIGHT_NTSC_B UNUSED = 0.114;

#ifdef USE_LINEAR_COLORS

STATIC_CONST double COLOR_WEIGHT_R UNUSED = COLOR_WEIGHT_LINEAR_R;
STATIC_CONST double COLOR_WEIGHT_G UNUSED = COLOR_WEIGHT_LINEAR_G;
STATIC_CONST double COLOR_WEIGHT_B UNUSED = COLOR_WEIGHT_LINEAR_B;

#else

STATIC_CONST double COLOR_WEIGHT_R UNUSED = COLOR_WEIGHT_ITU709_R;
STATIC_CONST double COLOR_WEIGHT_G UNUSED = COLOR_WEIGHT_ITU709_G;
STATIC_CONST double COLOR_WEIGHT_B UNUSED = COLOR_WEIGHT_ITU709_B;

#endif


TColorMap fillColorMap() {
	TColorMap colors;
	colors.insert(TColorItem("none",    CL_NONE));
	colors.insert(TColorItem("black",   CL_BLACK));
	colors.insert(TColorItem("gray",    CL_GRAY));
	colors.insert(TColorItem("white",   CL_WHITE));
	colors.insert(TColorItem("red",     CL_RED));
	colors.insert(TColorItem("yellow",  CL_YELLOW));
	colors.insert(TColorItem("green",   CL_GREEN));
	colors.insert(TColorItem("cyan",    CL_CYAN));
	colors.insert(TColorItem("blue",    CL_BLUE));
	colors.insert(TColorItem("default", CL_DEFAULT));
	colors.insert(TColorItem("primary", CL_PRIMARY));
	colors.insert(TColorItem("success", CL_SUCCESS));
	colors.insert(TColorItem("info",    CL_INFO));
	colors.insert(TColorItem("warning", CL_WARNING));
	colors.insert(TColorItem("danger",  CL_DANGER));
	return colors;
}

static TColorMap localColorMap = fillColorMap();

bool getColorByName(const std::string& name, TColor& color) {
	if (!name.empty()) {
		std::string s = name;
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);

		TColorMap::const_iterator it = localColorMap.find(s);
		if (it != localColorMap.end()) {
			color = it->second;
			return true;
		}
	}
	return false;
}

bool addColorByName(const std::string& name, const TColor& color) {
	if (!name.empty()) {
		std::string s = name;
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);

		TColorMap::const_iterator it = localColorMap.find(s);
		if (it == localColorMap.end()) {
			localColorMap.insert(TColorItem(s, color));
			return true;
		}
	}
	return false;
}


TImageData::TImageData() : imageWidth(0), imageHeight(0) {
}

void TImageData::clear() {
	image.clear();
	imageWidth = 0;
	imageHeight = 0;
}



TBitmapPainter::TBitmapPainter() {
}


void TBitmapPainter::fill(TRGBData& image, int width, int height, const TColor& color) {
	if (width <= 0 || height <= 0)
		return;

	int px = width * height;
	int sz = px * 3;
	image.resize(sz, false);
	TRGB *p = image.data();

	if (color != CL_NONE) {
		for (int i=0; i<px; ++i) {
			ink(p, color);
		}
	}
}

void TBitmapPainter::line(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, const TColor& color) {
	if (!validate(x0, y0, sx, sy))
		return;
	if (!validate(x1, y1, sx, sy))
		return;

	// Bresenham-Algorithmus:
	// https://de.wikipedia.org/wiki/Bresenham-Algorithmus
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int px = x0 < x1 ? 1 : -1;
	int py = y0 < y1 ? 1 : -1;
	int e1 = (dx > dy ? dx : -dy) / 2;
	int e2;

	for (;;) {
		e2 = e1;
		plot(image, x0, y0, sx, sy, color);
		if (x0 == x1 && y0 == y1)
			break;
		if (e2 > -dx) {
			e1 -= dy;
			x0 += px;
		}
		if (e2 < dy) {
			e1 += dx;
			y0 += py;
		}
	}
}

void TBitmapPainter::line(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, int outline, const TColor& color) {
	if (outline > 0) {
		drawThickLine(image, x0, y0, x1, y1, sx, sy, outline, ELM_DRAW_DEFAULT, color);
	} else {
		line(image, x0, y0, x1, y1, sx, sy, color);
	}
}

/**
 * Modified Bresenham with optional overlap (esp. for drawThickLine())
 * Overlap draws additional pixel when changing minor direction - for standard bresenham overlap = EOM_OVERLAP_NONE
 * See: https://github.com/ArminJo/STMF3-Discovery-Demos/blob/master/lib/graphics/src/thickLine.cpp
 *
 *  Sample line:
 *
 *    00+
 *     -0000+
 *         -0000+
 *             -00
 *
 *  0 pixels are drawn for normal line without any overlap
 *  + pixels are drawn if EOM_OVERLAP_MAJOR
 *  - pixels are drawn if EOM_OVERLAP_MINOR
 */
void TBitmapPainter::drawOverlappedLine(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, const EOverlapMode overlap, const TColor& color) {

	// Check ranges
	if (!validate(x0, y0, sx, sy))
		return;

	int tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

	/*
	 * Clip to display size
	 */
	if (x0 >= sx) {
		x0 = util::pred(sx);
	}
	if (x0 < 0) {
		x0 = 0;
	}
	if (x1 >= sx) {
		x1 = util::pred(sx);
	}
	if (x1 < 0) {
		x1 = 0;
	}
	if (y0 >= sy) {
		y0 = util::pred(sy);
	}
	if (y0 < 0) {
		y0 = 0;
	}
	if (y1 >= sy) {
		y1 = util::pred(sy);
	}
	if (y1 < 0) {
		y1 = 0;
	}

	if ((x0 == x1) || (y0 == y1)) {
		// Horizontal or vertical line
		line(image, x0, y0, x1, y1, sx, sy, color);
	} else {

		// Calculate direction
		tDeltaX = x1 - x0;
		tDeltaY = y1 - y0;
		if (tDeltaX < 0) {
			tDeltaX = -tDeltaX;
			tStepX = -1;
		} else {
			tStepX = +1;
		}
		if (tDeltaY < 0) {
			tDeltaY = -tDeltaY;
			tStepY = -1;
		} else {
			tStepY = +1;
		}
		tDeltaXTimes2 = tDeltaX << 1;
		tDeltaYTimes2 = tDeltaY << 1;

		// Draw start pixel
		plot(image, x0, y0, sx, sy, color);
		if (tDeltaX > tDeltaY) {

			// Start value represents a half step in Y direction
			tError = tDeltaYTimes2 - tDeltaX;
			while (x0 != x1) {

				// Step in main direction
				x0 += tStepX;
				if (tError >= 0) {
					if (overlap == EOM_OVERLAP_MAJOR || overlap == EOM_OVERLAP_BOTH) {
						// Draw pixel in main direction before changing
						plot(image, x0, y0, sx, sy, color);
					}
					// Change Y
					y0 += tStepY;
					if (overlap == EOM_OVERLAP_MINOR || overlap == EOM_OVERLAP_BOTH) {
						// Draw pixel in minor direction before changing
						plot(image, x0 - tStepX, y0, sx, sy, color);
					}
					tError -= tDeltaXTimes2;
				}
				tError += tDeltaYTimes2;
				plot(image, x0, y0, sx, sy, color);
			}
		} else {
			tError = tDeltaXTimes2 - tDeltaY;
			while (y0 != y1) {
				y0 += tStepY;
				if (tError >= 0) {
					if (overlap == EOM_OVERLAP_MAJOR || overlap == EOM_OVERLAP_BOTH) {
						// draw pixel in main direction before changing
						plot(image, x0, y0, sx, sy, color);
					}
					x0 += tStepX;
					if (overlap == EOM_OVERLAP_MINOR || overlap == EOM_OVERLAP_BOTH) {
						// draw pixel in minor direction before changing
						plot(image, x0, y0 - tStepY, sx, sy, color);
					}
					tError -= tDeltaYTimes2;
				}
				tError += tDeltaXTimes2;
				plot(image, x0 - tStepX, y0, sx, sy, color);
			}
		}
	}
}

/**
 * Bresenham with thickness
 * no pixel missed and every pixel only drawn once!
 */
void TBitmapPainter::drawThickLine(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, int outline, ELineMode mode, const TColor& color) {

	// 1 Pixel outlined
	if (outline <= 1) {
		drawOverlappedLine(image, x0, y0, x1, y1, sx, sy, EOM_OVERLAP_NONE, color);
	}

	// Check ranges
	if (!validate(x0, y0, sx, sy))
		return;

	int i, tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

	/*
	 * Clip to display size
	 */
	if (x0 >= sx) {
		x0 = util::pred(sx);
	}
	if (x0 < 0) {
		x0 = 0;
	}
	if (x1 >= sx) {
		x1 = util::pred(sx);
	}
	if (x1 < 0) {
		x1 = 0;
	}
	if (y0 >= sy) {
		y0 = util::pred(sy);
	}
	if (y0 < 0) {
		y0 = 0;
	}
	if (y1 >= sy) {
		y1 = util::pred(sy);
	}
	if (y1 < 0) {
		y1 = 0;
	}

	/**
	 * For coordinate system with 0.0 top left
	 * Swap X and Y delta and calculate clockwise (new delta X inverted)
	 * or counterclockwise (new delta Y inverted) rectangular direction.
	 * The right rectangular direction for LINE_OVERLAP_MAJOR toggles with each qudrant
	 */
	tDeltaY = x1 - x0;
	tDeltaX = y1 - y0;

	// Mirror 4 quadrants to one and adjust deltas and stepping direction
	bool tSwap = true; // count effective mirroring
	if (tDeltaX < 0) {
		tDeltaX = -tDeltaX;
		tStepX = -1;
		tSwap = !tSwap;
	} else {
		tStepX = +1;
	}
	if (tDeltaY < 0) {
		tDeltaY = -tDeltaY;
		tStepY = -1;
		tSwap = !tSwap;
	} else {
		tStepY = +1;
	}
	tDeltaXTimes2 = tDeltaX << 1;
	tDeltaYTimes2 = tDeltaY << 1;
	EOverlapMode tOverlap;

	// adjust for right direction of thickness from line origin
	int tDrawStartAdjustCount = outline / 2;
	if (mode == ELM_DRAW_COUNTERCLOCKWISE) {
		tDrawStartAdjustCount = outline - 1;
	} else if (mode == ELM_DRAW_CLOCKWISE) {
		tDrawStartAdjustCount = 0;
	}

	// which qudrant are we now
	if (tDeltaX >= tDeltaY) {
		if (tSwap) {
			tDrawStartAdjustCount = (outline - 1) - tDrawStartAdjustCount;
			tStepY = -tStepY;
		} else {
			tStepX = -tStepX;
		}
		/*
		 * Vector for draw direction of lines is rectangular and counterclockwise to original line
		 * Therefore no pixel will be missed if LINE_OVERLAP_MAJOR is used
		 * on changing in minor rectangular direction
		 */
		// adjust draw start point
		tError = tDeltaYTimes2 - tDeltaX;
		for (i = tDrawStartAdjustCount; i > 0; i--) {
			// change X (main direction here)
			x0 -= tStepX;
			x1 -= tStepX;
			if (tError >= 0) {
				// change Y
				y0 -= tStepY;
				y1 -= tStepY;
				tError -= tDeltaXTimes2;
			}
			tError += tDeltaYTimes2;
		}

		// Draw start line
		line(image, x0, y0, x1, y1, sx, sy, color);

		// draw width lines
		tError = tDeltaYTimes2 - tDeltaX;
		for (i = outline; i > 1; i--) {

			// Change X (main direction here)
			x0 += tStepX;
			x1 += tStepX;
			tOverlap = EOM_OVERLAP_NONE;
			if (tError >= 0) {
				// change Y
				y0 += tStepY;
				y1 += tStepY;
				tError -= tDeltaXTimes2;
				/*
				 * change in minor direction reverse to line (main) direction
				 * because of choosing the right (counter)clockwise draw vector
				 * use LINE_OVERLAP_MAJOR to fill all pixel
				 *
				 * EXAMPLE:
				 * 1,2 = Pixel of first lines
				 * 3 = Pixel of third line in normal line mode
				 * - = Pixel which will additionally be drawn in LINE_OVERLAP_MAJOR mode
				 *           33
				 *       3333-22
				 *   3333-222211                         ^
				 * 33-22221111                           |
				 *  221111                               |
				 *  11                          Main direction of draw vector
				 *  -> Line main direction
				 *  <- Minor direction of counterclockwise draw vector
				 */
				tOverlap = EOM_OVERLAP_MAJOR;
			}
			tError += tDeltaYTimes2;
			drawOverlappedLine(image, x0, y0, x1, y1, sx, sy, tOverlap, color);
		}
	} else {
		// The other qudrant
		if (tSwap) {
			tStepX = -tStepX;
		} else {
			tDrawStartAdjustCount = (outline - 1) - tDrawStartAdjustCount;
			tStepY = -tStepY;
		}

		// Adjust draw start point
		tError = tDeltaXTimes2 - tDeltaY;
		for (i = tDrawStartAdjustCount; i > 0; i--) {
			y0 -= tStepY;
			y1 -= tStepY;
			if (tError >= 0) {
				x0 -= tStepX;
				x1 -= tStepX;
				tError -= tDeltaYTimes2;
			}
			tError += tDeltaXTimes2;
		}

		// Draw start line
		line(image, x0, y0, x1, y1, sx, sy, color);
		tError = tDeltaXTimes2 - tDeltaY;
		for (i = outline; i > 1; i--) {
			y0 += tStepY;
			y1 += tStepY;
			tOverlap = EOM_OVERLAP_NONE;
			if (tError >= 0) {
				x0 += tStepX;
				x1 += tStepX;
				tError -= tDeltaYTimes2;
				tOverlap = EOM_OVERLAP_MAJOR;
			}
			tError += tDeltaXTimes2;
			drawOverlappedLine(image, x0, y0, x1, y1, sx, sy, tOverlap, color);
		}
	}
}

/**
 * The same as before, but no clipping to display range, some pixel are drawn twice (because of using LINE_OVERLAP_BOTH)
 * and direction of thickness changes for each qudrant (except for LINE_THICKNESS_MIDDLE and width value is odd)
 */
void TBitmapPainter::drawThickLineSimple(TRGBData& image, int x0, int y0, int x1, int y1, int sx, int sy, int outline, const ELineMode mode, const TColor& color) {

	// Check ranges
	if (!validate(x0, y0, sx, sy))
		return;

	int i, tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

	if (outline < 2)
		outline = 2;

	tDeltaY = x0 - x1;
	tDeltaX = y1 - y0;

	// mirror 4 quadrants to one and adjust deltas and stepping direction
	if (tDeltaX < 0) {
		tDeltaX = -tDeltaX;
		tStepX = -1;
	} else {
		tStepX = +1;
	}
	if (tDeltaY < 0) {
		tDeltaY = -tDeltaY;
		tStepY = -1;
	} else {
		tStepY = +1;
	}
	tDeltaXTimes2 = tDeltaX << 1;
	tDeltaYTimes2 = tDeltaY << 1;
	EOverlapMode tOverlap;

	// which qudrant are we now
	if (tDeltaX > tDeltaY) {
		if (mode == ELM_DRAW_MIDDLE) {
			// Adjust draw start point
			tError = tDeltaYTimes2 - tDeltaX;
			for (i = outline / 2; i > 0; i--) {
				// Change X (main direction here)
				x0 -= tStepX;
				x1 -= tStepX;
				if (tError >= 0) {
					// change Y
					y0 -= tStepY;
					y1 -= tStepY;
					tError -= tDeltaXTimes2;
				}
				tError += tDeltaYTimes2;
			}
		}

		// Draw start line
		line(image, x0, y0, x1, y1, sx, sy, color);

		// Draw width lines
		tError = tDeltaYTimes2 - tDeltaX;
		for (i = outline; i > 1; i--) {
			// Change X (main direction here)
			x0 += tStepX;
			x1 += tStepX;
			tOverlap = EOM_OVERLAP_NONE;
			if (tError >= 0) {
				// change Y
				y0 += tStepY;
				y1 += tStepY;
				tError -= tDeltaXTimes2;
				tOverlap = EOM_OVERLAP_BOTH;
			}
			tError += tDeltaYTimes2;
			//drawLineOverlap(x0, y0, x1, y1, tOverlap, color);
			drawOverlappedLine(image, x0, y0, x1, y1, sx, sy, tOverlap, color);
		}
	} else {
		// adjust draw start point
		if (mode == ELM_DRAW_MIDDLE) {
			tError = tDeltaXTimes2 - tDeltaY;
			for (i = outline / 2; i > 0; i--) {
				y0 -= tStepY;
				y1 -= tStepY;
				if (tError >= 0) {
					x0 -= tStepX;
					x1 -= tStepX;
					tError -= tDeltaYTimes2;
				}
				tError += tDeltaXTimes2;
			}
		}

		// Draw start line
		line(image, x0, y0, x1, y1, sx, sy, color);
		tError = tDeltaXTimes2 - tDeltaY;
		for (i = outline; i > 1; i--) {
			y0 += tStepY;
			y1 += tStepY;
			tOverlap = EOM_OVERLAP_NONE;
			if (tError >= 0) {
				x0 += tStepX;
				x1 += tStepX;
				tError -= tDeltaYTimes2;
				tOverlap = EOM_OVERLAP_BOTH;
			}
			tError += tDeltaXTimes2;
			drawOverlappedLine(image, x0, y0, x1, y1, sx, sy, tOverlap, color);
		}
	}
}

void TBitmapPainter::rect(TRGBData& image, int x0, int y0, int width, int height, int sx, int sy, int outline, const TColor& color) {

	// Check ranges
	if (x0 < 0 || y0 < 0 || width <= 0 || height <= 0 || outline < 1)
		return;
	if (!validate(x0, y0, sx, sy))
		return;

	// Normalize values
	int xLast = x0 + width;
	if (xLast > sx)
		xLast = sx;

	int yLast = y0 + height;
	if (yLast > sy)
		xLast = sy;

	int yOutline = y0 + outline;
	int xOutline = x0 + outline;

	int yBorder = yLast - outline;
	int xBorder = xLast - outline;

	// Draw upper border
	for (int y=y0; y<yOutline && y<yLast; y++) {
		line(image, x0,y, xLast,y, sx,sy, color);
	}
	// Draw lower border
	for (int y=yLast; y>=yBorder && y>=0; y--) {
		line(image, x0,y, xLast,y, sx,sy, color);
	}

	// Draw left border
	for (int x=x0; x<xOutline && x<xLast; x++) {
		line(image, x,y0, x,yLast, sx,sy, color);
	}
	// Draw right border
	for (int x=xLast; x>=xBorder && x>=0; x--) {
		line(image, x,y0, x,yLast, sx,sy, color);
	}

}

void TBitmapPainter::square(TRGBData& image, int x0, int y0, int width, int height, int sx, int sy, const TColor& color) {

	// Check ranges
	if (x0 < 0 || y0 < 0 || width <= 0 || height <= 0)
		return;
	if (!validate(x0, y0, sx, sy))
		return;

	// Max. boundries
	int xLast = x0 + width;
	if (xLast > sx)
		xLast = sx;

	int yLast = y0 + height;
	if (yLast > sy)
		xLast = sy;

	// Draw lines
	for (int y=y0; y<yLast; y++) {
		line(image, x0,y, xLast,y, sx,sy, color);
	}

}

void TBitmapPainter::circle(TRGBData& image, int x0, int y0, int radius, int sx, int sy, const TColor& color) {
	if (!validate(x0, y0, sx, sy))
		return;

	int f = 1 - radius;
	int dx = 0;
	int dy = -2 * radius;
	int x = 0;
	int y = radius;

	draw(image, x0, y0 + radius, sx, sy, color);
	draw(image, x0, y0 - radius, sx, sy, color);
	draw(image, x0 + radius, y0, sx, sy, color);
	draw(image, x0 - radius, y0, sx, sy, color);

	while (x < y) {
		if(f >= 0) {
			--y;
			dy += 2;
			f += dy;
		}
		++x;
		dx += 2;
		f += dx + 1;

		draw(image, x0 + x, y0 + y, sx, sy, color);
		draw(image, x0 - x, y0 + y, sx, sy, color);
		draw(image, x0 + x, y0 - y, sx, sy, color);
		draw(image, x0 - x, y0 - y, sx, sy, color);
		draw(image, x0 + y, y0 + x, sx, sy, color);
		draw(image, x0 - y, y0 + x, sx, sy, color);
		draw(image, x0 + y, y0 - x, sx, sy, color);
		draw(image, x0 - y, y0 - x, sx, sy, color);
	}
}

void TBitmapPainter::ellipse(TRGBData& image, int x0, int y0, int a, int b, int sx, int sy, const TColor& color) {
	if (!validate(x0, y0, sx, sy))
		return;

	/* Im I. Quadranten von links oben nach rechts unten */
	int dx = 0;
	int dy = b;
	long a2 = a * a;
	long b2 = b * b;
	long e1 = b2 - (2 * b - 1) * a2, e2; /* Fehler im 1. Schritt */

	do {
		draw(image, x0 + dx, y0 + dy, sx, sy, color); /* I. Quadrant */
		draw(image, x0 - dx, y0 + dy, sx, sy, color); /* II. Quadrant */
		draw(image, x0 - dx, y0 - dy, sx, sy, color); /* III. Quadrant */
		draw(image, x0 + dx, y0 - dy, sx, sy, color); /* IV. Quadrant */

		e2 = 2 * e1;
		if (e2 <  (2 * dx + 1) * b2) {
			dx++;
			e1 += (2 * dx + 1) * b2;
		}
		if (e2 > -(2 * dy - 1) * a2) {
			dy--;
			e1 -= (2 * dy - 1) * a2;
		}

	} while (dy >= 0);

	/* Fehlerhafter Abbruch bei flachen Ellipsen (b = 1) */
	while (dx++ < a) {
		draw(image, x0 + dx, y0, sx, sy, color); /* --> Spitze der Ellipse vollenden */
		draw(image, x0 - dx, y0, sx, sy, color);
	}
}

void TBitmapPainter::bezier2s(TRGBData& image, int x0, int y0, int x1, int y1, int x2, int y2, int sx, int sy, const TColor& color) {
	//	if (!validate(x0, y0, sx, sy))
	//		return;
	//	if (!validate(x1, y1, sx, sy))
	//		return;
	//	if (!validate(x1, y2, sx, sy))
	//		return;

	// http://members.chello.at/~easyfilter/bresenham.html
	int px = x2-x1, py = y2-y1;
	long xx = x0-x1, yy = y0-y1, xy;            /* relative values for checks */
	double dx, dy, err, cur = xx*py-yy*px;                       /* curvature */

	//	if (!(xx*px <= 0 && yy*py <= 0))      /* sign of gradient must not change */
	//		return;

	if (px*(long)px+py*(long)py > xx*xx+yy*yy) {    /* begin with longer part */
		x2 = x0; x0 = px+x1; y2 = y0; y0 = py+y1; cur = -cur;   /* swap P0 P2 */
	}
	if (cur != 0) {                                        /* no straight line */
		xx += px; xx *= px = x0 < x2 ? 1 : -1;             /* x step direction */
		yy += py; yy *= py = y0 < y2 ? 1 : -1;             /* y step direction */
		xy = 2*xx*yy; xx *= xx; yy *= yy;            /* differences 2nd degree */
		if (cur*px*py < 0) {                             /* negated curvature? */
			xx = -xx; yy = -yy; xy = -xy; cur = -cur;
		}
		dx = 4.0*py*cur*(x1-x0)+xx-xy;               /* differences 1st degree */
		dy = 4.0*px*cur*(y0-y1)+yy-xy;
		xx += xx; yy += yy; err = dx+dy+xy;                  /* error 1st step */
		do {
			draw(image, x0,y0, sx,sy, color);                    /* plot curve */
			if (x0 == x2 && y0 == y2) return;  /* last pixel -> curve finished */
			y1 = 2*err < dx;                  /* save value for test of y step */
			if (2*err > dy) { x0 += px; dx -= xy; err += dy += yy; } /* x step */
			if (    y1    ) { y0 += py; dy -= xy; err += dx += xx; } /* y step */
		} while (dy < dx );             /* gradient negates -> algorithm fails */
	}
	line(image, x0,y0, x2,y2, sx,sy, color);     /* plot remaining part to end */
}

void TBitmapPainter::bezier2(TRGBData& image, int x0, int y0, int x1, int y1, int x2, int y2, int sx, int sy, const TColor& color) {
	/* plot any quadratic Bezier curve */
	if (!validate(x0, y0, sx, sy))
		return;
	if (!validate(x1, y1, sx, sy))
		return;
	if (!validate(x1, y2, sx, sy))
		return;

	// http://members.chello.at/~easyfilter/bresenham.html
	int x = x0-x1, y = y0-y1;
	double t = x0-2*x1+x2, r;

	if ((long)x*(x2-x1) > 0) {                         /* horizontal cut at P4? */
		if ((long)y*(y2-y1) > 0)                     /* vertical cut at P6 too? */
			if (fabs((y0-2*y1+y2)/t*x) > abs(y)) {              /* which first? */
				x0 = x2; x2 = x+x1; y0 = y2; y2 = y+y1;          /* swap points */
			}                           /* now horizontal cut at P4 comes first */
		t = (x0-x1)/t;
		r = (1-t)*((1-t)*y0+2.0*t*y1)+t*t*y2;                       /* By(t=P4) */
		t = (x0*x2-x1*x1)*t/(x0-x1);                       /* gradient dP4/dx=0 */
		x = floor(t+0.5); y = floor(r+0.5);
		r = (y1-y0)*(t-x0)/(x1-x0)+y0;                  /* intersect P3 | P0 P1 */
		bezier2s(image, x0,y0, x,floor(r+0.5), x,y, sx,sy, color);
		r = (y1-y2)*(t-x2)/(x1-x2)+y2;                  /* intersect P4 | P1 P2 */
		x0 = x1 = x; y0 = y; y1 = floor(r+0.5);             /* P0 = P4, P1 = P8 */
	}
	if ((long)(y0-y1)*(y2-y1) > 0) {                     /* vertical cut at P6? */
		t = y0-2*y1+y2; t = (y0-y1)/t;
		r = (1-t)*((1-t)*x0+2.0*t*x1)+t*t*x2;                        /* Bx(t=P6) */
		t = (y0*y2-y1*y1)*t/(y0-y1);                        /* gradient dP6/dy=0 */
		x = floor(r+0.5); y = floor(t+0.5);
		r = (x1-x0)*(t-y0)/(y1-y0)+x0;                   /* intersect P6 | P0 P1 */
		bezier2s(image, x0,y0, floor(r+0.5),y, x,y, sx,sy, color);
		r = (x1-x2)*(t-y2)/(y1-y2)+x2;                   /* intersect P7 | P1 P2 */
		x0 = x; x1 = floor(r+0.5); y0 = y1 = y;              /* P0 = P6, P1 = P7 */
	}
	bezier2s(image, x0,y0, x1,y1, x2,y2, sx,sy, color);        /* remaining part */
}


bool TBitmapPainter::validate(int x, int y, int sx, int sy) {
	if (x < 0 || x >= sx || y < 0 || y >= sy)
		return false;
	return true;
}

void TBitmapPainter::draw(TRGBData& image, int x, int y, int sx, int sy, const TColor& color) {
	if (validate(x, y, sx, sy)) {
		plot(image, x, y, sx, sy, color);
	}
}

void TBitmapPainter::plot(TRGBData& image, int x, int y, int sx, int sy, const TColor& color) {
	TRGB *p = image.data() + 3 * (x + y * sx);
	ink(p, color);
}

void TBitmapPainter::ink(TRGB*& p, const TColor& color) {
	*(p++) = color.red;
	*(p++) = color.green;
	*(p++) = color.blue;
}


void TBitmapPainter::pixel555to888(const uint16_t *image16, TRGB *image24) {
	TColor color;
	TRGB *p = image24;
	color.blue  = (*image16 << 3) & 0xF8;
	color.green = (*image16 >> 2) & 0xF8;
	color.red   = (*image16 >> 7) & 0xF8;
    *(p++) = color.blue;
    *(p++) = color.green;
    *p = color.red;
}

void TBitmapPainter::pixel565to888(const uint16_t *image16, TRGB *image24) {
	TColor color;
	TRGB *p = image24;
	color.blue  = (*image16 & 0x001F);
	color.green = (*image16 & 0x07E0) >> 5;
	color.red   = (*image16 >> 11) & 0x1F;
    *(p++) = color.blue << 2;
    *(p++) = color.green << 1;
    *p = color.red << 2;
}

bool TBitmapPainter::convert555to888(const TRGBData& image16, TRGBData& image24) {
	if (!image16.empty()) {
		size_t size = image16.size() * 3 / 2;
		image24.resize(size, false);
		const TRGB *p16 = image16.data();
		TRGB *p24 = image24.data();
		for(size_t i=0; i<image16.size() / 2; ++i, p16 += 2, p24 += 3) {
			pixel555to888((uint16_t*)p16, p24);
		}
		return true;
	}
	image24.clear();
	return false;
}

bool TBitmapPainter::convert565to888(const TRGBData& image16, TRGBData& image24) {
	if (!image16.empty()) {
		size_t size = image16.size() / 2 * 3;
		image24.resize(size, false);
		const TRGB *p16 = image16.data();
		TRGB *p24 = image24.data();
		for(size_t i=0; i<image16.size() / 2; ++i, p16 += 2, p24 += 3) {
			pixel565to888((uint16_t*)p16, p24);
		}
		return true;
	}
	image24.clear();
	return false;
}




// Create color gradient from color to color based on normalized percent value
TColor TColorPicker::gradient(const double percent, const TColor& from, const TColor& to) {
	if (percent <= 0.0)
		return from;
	if (percent >= 100.0)
		return to;

	TColor color;
	double diff = 100.0 - percent;
	color.red   = (TRGB)std::min((int)floor((diff * from.red   + percent * to.red  ) / 100.0), 255);
	color.green = (TRGB)std::min((int)floor((diff * from.green + percent * to.green) / 100.0), 255);
	color.blue  = (TRGB)std::min((int)floor((diff * from.blue  + percent * to.blue ) / 100.0), 255);

	return color;
}

// Create natural "cold" to "hot" color gradient from blue to magenta based on normalized percent value
TColor TColorPicker::colors(const double percent) {
	double ratio = percent;
	if (percent < 0.0)
		ratio = 0.0;
	if (percent > 100.0)
		ratio = 100.0;

	// Normalize ratio to fit into 5 regions sized of 256 units each
	// --> 5.0 * 256.0 / 100.0 = 12.8
	int n = floor(ratio * 12.8);

	// Find the distance to the start of the closest region
	int x = n % 256, r = n / 256;

	TColor color = CL_WHITE;
	switch (r) {
		case 0: color.red = 0;   color.green = x;       color.blue = 255;     break; // Blue
		case 1: color.red = 0;   color.green = 255;     color.blue = 255 - x; break; // Cyan
		case 2: color.red = x;   color.green = 255;     color.blue = 0;       break; // Green
		case 3: color.red = 255; color.green = 255 - x; color.blue = 0;       break; // Yellow
		case 4: color.red = 255; color.green = 0;       color.blue = x;       break; // Red + Magenta
	}

	return color;
}

// Create rainbow color gradient from red to ultraviolet (magenta) based on normalized percent value
TColor TColorPicker::rainbow(const double percent) {
	double ratio = percent;
	if (percent < 0.0)
		ratio = 0.0;
	if (percent > 100.0)
		ratio = 100.0;

	// Normalize ratio to fit into 5 regions sized of 256 units each
	// --> 5.0 * 256.0 / 100.0 = 12.8
	int n = floor(ratio * 12.8);

	// Find the distance to the start of the closest region
	int x = n % 256, r = n / 256;

	TColor color = CL_WHITE;
	switch (r) {
		case 0: color.red = 255;     color.green = x;       color.blue = 0;       break; // Red
		case 1: color.red = 255 - x; color.green = 255;     color.blue = 0;       break; // Yellow
		case 2: color.red = 0;       color.green = 255;     color.blue = x;       break; // Green
		case 3: color.red = 0;       color.green = 255 - x; color.blue = 255;     break; // Cyan
		case 4: color.red = x;       color.green = 0;       color.blue = 255;     break; // Blue
		case 5: color.red = 255;     color.green = 0;       color.blue = 255 - x; break; // Magenta
	}

	return color;
}

TColor TColorPicker::circular(const double percent) {
	double ratio = percent;
	if (percent < 0.0)
		ratio = 0.0;
	if (percent > 100.0)
		ratio = 100.0;

	// Normalize ratio to fit into 6 regions sized of 256 units each
	// --> 6.0 * 256.0 / 100.0 = 12.8
	int n = floor(ratio * 15.36);

	// Find the distance to the start of the closest region
	int x = n % 256, r = n / 256;

	TColor color = CL_WHITE;
	switch (r) {
		case 0: color.red = 255;     color.green = x;       color.blue = 0;       break; // Red
		case 1: color.red = 255 - x; color.green = 255;     color.blue = 0;       break; // Yellow
		case 2: color.red = 0;       color.green = 255;     color.blue = x;       break; // Green
		case 3: color.red = 0;       color.green = 255 - x; color.blue = 255;     break; // Blue
		case 4: color.red = x;       color.green = 0;       color.blue = 255;     break; // Magenta
		case 5: color.red = 255;     color.green = 0;       color.blue = 255 - x; break; // Red
	}

	return color;
}


// Create gauge gradient from "red" = 0.0% to "green" = 100.0%
TColor TColorPicker::gauge(const double percent) {
	double ratio = percent;
	if (percent < 0.0)
		ratio = 0.0;
	if (percent > 100.0)
		ratio = 100.0;

	// Normalize ratio to fit into 2 regions sized of 256 units each
	// --> 2.0 * 256.0 / 100.0 = 5.12
	int n = floor(ratio * 5.12);

	// Find the distance to the start of the closest region
	int x = n % 256, r = n / 256;

	TColor color = CL_WHITE;
	switch (r) {
		case 0: color.red = 255;     color.green = x;   color.blue = 0; break; // Red
		case 1: color.red = 255 - x; color.green = 255; color.blue = 0; break; // Yellow to Green
	}

	return color;
}



// Create color gradient for physically correct color teperature based on normalized percent value
TColor TColorPicker::temperature(const double percent, const double from, const double to, const std::string& name) {
	const char* s = name.empty() ? nil : name.c_str();
	return temperature(percent, from, to, s);
}
TColor TColorPicker::temperature(const double percent, const double from, const double to, const char* name) {
	double r = percent;
	if (percent < 0.0)
		r = 0.0;
	if (percent > 100.0)
		r = 100.0;

	// Scale temperature by given percent value
	double t = from + (r * (to - from) / 100.0);

	// Get RGB value for given temperature
	return temperature(t, name);
}
TColor TColorPicker::temperature(const double percent, const double from, const double to, const CColorSystem *system) {
	double r = percent;
	if (percent < 0.0)
		r = 0.0;
	if (percent > 100.0)
		r = 100.0;

	// Scale temperature by given percent value
	double t = from + (r * (to - from) / 100.0);

	// Get RGB value for given temperature
	return temperature(t, system);
}

// Get color for give teperature in Kelvin for given named color system:
// "NTSC", "EBU", "SMPTE", "HDTV", "CIE", "REC709"
TColor TColorPicker::temperature(const double temperature, const std::string& system) {
	// Get RGB value for given temperature
	const char* name = system.empty() ? nil : system.c_str();
	return TColorPicker::temperature(temperature, name);
}

TColor TColorPicker::temperature(const double temperature, const char* system) {
	// Get RGB value for given temperature
	const CColorSystem *cs = getColorSystem(system);
	return TColorPicker::temperature(temperature, cs);
}

TColor TColorPicker::temperature(const double temperature, const CColorSystem *system) {
	double r, g, b;

	// Get RGB value for given temperature
	spectrum_to_rgb(system, &r, &g, &b, temperature);

	TColor color;
	color.red   = (TRGB)std::min((int)floor(r * 255.0), 255);
	color.green = (TRGB)std::min((int)floor(g * 255.0), 255);
	color.blue  = (TRGB)std::min((int)floor(b * 255.0), 255);

	return color;
}

// Get color system for given name: "NTSC", "EBU", "SMPTE", "HDTV", "CIE", "REC709"
const CColorSystem * TColorPicker::getColorSystem(const std::string& system) {
	const char* name = system.empty() ? nil : system.c_str();
	return getColorSystem(name);
}

const CColorSystem * TColorPicker::getColorSystem(const char* system) {
	return get_color_system_by_name(system);
}




TBitmapScaler::TBitmapScaler() {
}


PRGB TBitmapScaler::resizeColorAverage(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy) {
	TRGB *p, *q, *origin = src.data();
	int i, j, k, l, ya, yb;
	int sq, r, g, b;

	int sz = dx*dy*3;
	if(sz <= 0)	{
		dst = src;
		return dst.data();
	}

	dst.resize(sz, false);
	q = dst.data();

	int xa_v[dx];
	for (i=0; i<dx; i++)
		xa_v[i] = (int)(i*sx/dx);

	int xb_v[dx+1];
	for (i=0; i<dx; i++) {
		xb_v[i] = (int)((i+1)*sx/dx);
		if(xb_v[i] >= sx)
			xb_v[i] = sx-1;
	}

	for(j=0; j<dy; j++) {
		ya = (int)(j * sy / dy);
		yb = (int)((j + 1) * sy / dy);
		if (yb >= sy) yb = sy-1;
		for (i=0; i<dx; i++, q+=3) {
			for ( l=ya, r=0, g=0, b=0, sq=0; l<=yb; l++) {
				p = origin + ((l * sx + xa_v[i]) * 3);
				for (k = xa_v[i]; k <= xb_v[i]; k++, p += 3, sq++) {
					r += p[0]; g += p[1]; b += p[2];
				}
			}
			q[0] = r / sq; q[1] = g / sq; q[2] = b / sq;
		}
	}

	return dst.data();
}


PRGB TBitmapScaler::resizeSimple(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy) {
	TRGB *p, *q, *origin = src.data();
	int i, j, k, ip;

	size_t sz = dx*dy*3;
	if(sz <= 0) {
		dst = src;
		return dst.data();
	}

	dst.resize(sz, false);
	q = dst.data();

	for(j=0;j<dy;j++,q+=dx*3) {
		p=origin+(j*sy/dy*sx*3);
		for(i=0,k=0;i<dx;i++,k+=3) {
			ip=i*sx/dx*3;
			memcpy(q+k, p+ip, 3);
		}
	}

	return dst.data();
}


PRGB TBitmapScaler::resizeBilinear(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy) {
	int l, c;
	double t, u, d;
	double rx, ry;
	double d1, d2, d3, d4;
	TRGB *p1, *p2, *p3, *p4;
	TRGB *q, *origin = src.data();

	// Calculate destination buffer size
	size_t sz = dx*dy*3;
	if(sz <= 0) {
		dst = src;
		return dst.data();
	}

	// Resize destination buffer
	dst.resize(sz, false);
	q = dst.data();

	// Compression ration
	rx = (double)dx / (double)sx;
	ry = (double)dy / (double)sy;

	// Iterate through destination image buffer
	for (int y=0; y<dy; y++) {
		for (int x=0; x<dx; x++) {

			d = (double)y / ry;
			l = floor(d);
			if (l < 0) {
				l = 0;
			} else {
				if (l >= sy) {
					l = sy - 1;
				}
			}
			u = d - l;

			d = (double)x / rx;
			c = floor(d);
			if (c < 0) {
				c = 0;
			} else {
				if (c >= sx) {
					c = sx - 1;
				}
			}
			t = d - c;

			// Coefficients
			d1 = (1.0 - t) * (1.0 - u);
			d2 = t * (1.0 - u);
			d3 = (1.0 - t) * u;
			d4 = t * u;

			// Pointer to neighbor pixels
			l *= 3; c *= 3;
			p1 = origin + (l * sx) + c;
			p2 = p1 + 3;
			p3 = origin + ((l + 3) * sx) + c;
			p4 = p3 + 3;

			// Recalculate weighted color components
			// and store new RGB pixel in destination image buffer
			*q++ = *(p1++) * d1 + *(p2++) * d2 + *(p3++) * d3 + *(p4++) * d4;
			*q++ = *(p1++) * d1 + *(p2++) * d2 + *(p3++) * d3 + *(p4++) * d4;
			*q++ = *p1 * d1 + *p2 * d2 + *p3 * d3 + *p4 * d4;

		}
	}

	return dst.data();
}

PRGB TBitmapScaler::overlay(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy, const TColor& color) {
	TRGB *p, *q, *origin = src.data();
	int i, j, k = sx * 3;

	size_t sz = dx*dy*3;
	if(sz <= 0) {
		dst = src;
		return dst.data();
	}

	dst.resize(sz, false);
	q = dst.data();

	// Is destination higher than source?
	// --> Add lines at top and bottom position
	int header = 0;
	int footer = 0;
	if (dy > sy) {
		header = (dy - sy) / 2;
		if (dy > (header + sy))
			footer = dy - header - sy;
	}

	// Fill header RGB pixels
	if (header > 0) {
		int px = header * dx;
		for(i=0; i<px; i++) {
			ink(q, color);
		}
	}

	// Is destination wider than source?
	// --> Add RGB pixels at left and right position
	int left = 0;
	int right = 0;
	if (dx > sx) {
		left = (dx - sx) / 2;
		if (dy > (left + sx))
			right = dx - left - sx;
	}

	// Copy source to destination buffer
	for(j=0; j<sy; j++) {

		// Fill left position
		if (left > 0) {
			for(i=0; i<left; i++) {
				ink(q, color);
			}
		}

		// Copy row from source to destination buffer
		p = origin + (j * k);
		memcpy(q, p, k);
		q += k;

		// Fill right position
		if (right > 0) {
			for(i=0; i<right; i++) {
				ink(q, color);
			}
		}

	}

	// Fill footer RGB pixels
	if (footer > 0) {
		int px = footer * dx;
		for(i=0; i<px; i++) {
			ink(q, color);
		}
	}

	return dst.data();
}


PRGB TBitmapScaler::contrast(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor) {
	double z = factor;

	// Sharpening filter matrix: 4 * (0.2 + 0.8) = 4.0
	// See http://lodev.org/cgtutor/filtering.html for matrix examples
	TFilterMatrix matrix = {  -0.2 * z,   -0.8 * z,    -0.2 * z,
							  -0.8 * z, 1.0 + 4.0 * z, -0.8 * z,
							  -0.2 * z,   -0.8 * z,    -0.2 * z  };

	return filter(src, dst, sx, sy, matrix);
}


PRGB TBitmapScaler::blur(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor) {
	double z = factor;

	// Blur filter matrix
	TFilterMatrix matrix = { 0.0, z, 0.0,
							  z,  z,  z,
							 0.0, z, 0.0 };

	return filter(src, dst, sx, sy, matrix);
}


PRGB TBitmapScaler::edgy(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor) {
	double z = factor;

	// Artificial effect filter matrix
	TFilterMatrix matrix = {  -1.0 * z, -1.0 * z, -1.0 * z,
							  -1.0 * z,  8.0 * z, -1.0 * z,
							  -1.0 * z, -1.0 * z, -1.0 * z  };

	return filter(src, dst, sx, sy, matrix);
}


PRGB TBitmapScaler::filter(const TRGBData& src, TRGBData& dst, int sx, int sy, const TFilterMatrix& matrix) {
	TRGB *p, *q, *origin = src.data();
	double r, g, b, s, m;
	int xfs, xfe, yfs, yfe;
	int sxe = sx - 2;
	int sye = sy - 2;;
	int dx = 3*sx;

	// Check for 2D:3x3 = 1D:1x9 matrix
	if (matrix.size() != 9)
		throw util::app_error_fmt("TBitmapScaler::filter() Invalid matrix size (%)", matrix.size());

	// Calculate destination buffer size
	size_t sz = dx*sy;
	if(sz <= 0)	{
		dst = src;
		return dst.data();
	}

	// Resize destination buffer
	dst.resize(sz, false);
	q = dst.data();

	// Iterate through source image buffer
	for (int y=0; y<sy; y++) {
		for (int x=0; x<sx; x++) {

			// Reset values
			r = 0.0; g = 0.0; b = 0.0;
			s = 0.0;

			// Standard matrix limits
			yfs = 0; yfe = 2;
			xfs = 0; xfe = 2;

			// Calculate matrix limits to fit in image buffer at current position
			if (y < 1)   yfs = 1;
			if (y > sye) yfe = 1;
			if (x < 1)   xfs = 1;
			if (x > sxe) xfe = 1;

			// Accumulate weighted pixels values
			for(int yf=yfs; yf<=yfe; yf++) {
				for(int xf=xfs; xf<=xfe; xf++) {
					p = origin + dx*(y+yf-1) + 3*(x+xf-1);
					m = matrix[yf*3 + xf];
					r += m * (double)(*(p++));
					g += m * (double)(*(p++));
					b += m * (double)(*p);
					s += m;
				}
			}

			// Store final component values in resulting image
			normalizePixelValue(q++, r, s);
			normalizePixelValue(q++, g, s);
			normalizePixelValue(q++, b, s);
		}
	}

	return dst.data();
}


void TBitmapScaler::normalizePixelValue(TRGB *pixel, const double& component, const double& sum) {
	int v = floor(component);
	if (sum > 0.0) {
		// Normalize pixel value
		double w = component / sum;
		if (0 == std::isinf(w))
			v = floor(w);
	}
	*pixel = (TRGB)std::min(std::max(v, 0), 255);
}


PRGB TBitmapScaler::brightness(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor) {
	TRGB *p, *q;
	factor += 1.0;

	// Calculate destination buffer size
	size_t px = sx*sy;
	size_t sz = 3*px;
	if(sz <= 0)	{
		dst = src;
		return dst.data();
	}

	// Resize destination buffer
	dst.resize(sz, false);
	p = src.data();
	q = dst.data();

	// Iterate over all source image ixels
	for (size_t i=0; i<px; i++) {

		// Weight each RGB component by given factor
		*(q++) = (TRGB)std::min((int)floor((double)(*(p++)) * factor), 255);
		*(q++) = (TRGB)std::min((int)floor((double)(*(p++)) * factor), 255);
		*(q++) = (TRGB)std::min((int)floor((double)(*(p++)) * factor), 255);

	}

	return dst.data();
}


int TBitmapScaler::getColorRange(const TRGBData& src, int sx, int sy) {
	TRGB *p = src.data();

	// Calculate destination buffer size
	size_t px = sx*sy;
	double rms = 0.0;

	// Check ranges
	if (px == 0)
		return 0;

	// Iterate over all source image ixels
	for (size_t i=0; i<px; i++) {
		TRGB r = *(p++);
		TRGB g = *(p++);
		TRGB b = *(p++);

		rms += sqrt(
			(double)(r * r) * COLOR_WEIGHT_R +
			(double)(g * g) * COLOR_WEIGHT_G +
			(double)(b * b) * COLOR_WEIGHT_B
		);
	}

	int r = 255 - std::min((int)floor(rms / (double)px), 255);
	return r * 100 / 255;
}


/*
 * See: http://www.graficaobscura.com/matrix/index.html
 */
void applyMatrixOperation(TRGB& r, TRGB& g, TRGB& b, double matrix[4][4]) {
    int ir, ig, ib;

	ir = floor((double)r * matrix[0][0] + (double)g * matrix[1][0] + (double)b * matrix[2][0] + matrix[3][0]);
	ig = floor((double)r * matrix[0][1] + (double)g * matrix[1][1] + (double)b * matrix[2][1] + matrix[3][1]);
	ib = floor((double)r * matrix[0][2] + (double)g * matrix[1][2] + (double)b * matrix[2][2] + matrix[3][2]);

	r = (TRGB)std::min(std::max(ir, 0), 255);
	g = (TRGB)std::min(std::max(ig, 0), 255);
	b = (TRGB)std::min(std::max(ib, 0), 255);
}

PRGB TBitmapScaler::saturation(const TRGBData& src, TRGBData& dst, int sx, int sy, double factor) {
	TRGB *p, *q;
	TRGB r, g, b;
    int ir, ig, ib;

	// Calculate destination buffer size
	size_t px = sx*sy;
	size_t sz = 3*px;
	if(sz <= 0)	{
		dst = src;
		return dst.data();
	}

	// Create color saturation modifier matrix
    double c00, c01, c02, c10, c11, c12, c20, c21, c22;
	double s = factor;
	double z = 1.0 - s;

    c00 = z * COLOR_WEIGHT_R + s;
    c01 = z * COLOR_WEIGHT_R;
    c02 = z * COLOR_WEIGHT_R;

    c10 = z * COLOR_WEIGHT_G;
    c11 = z * COLOR_WEIGHT_G + s;
    c12 = z * COLOR_WEIGHT_G;

    c20 = z * COLOR_WEIGHT_B;
    c21 = z * COLOR_WEIGHT_B;
    c22 = z * COLOR_WEIGHT_B + s;

	// Resize destination buffer
	dst.resize(sz, false);
	p = src.data();
	q = dst.data();

	// Iterate over all source image ixels
	for (size_t i=0; i<px; i++) {
		r = *(p++); g = *(p++);	b = *(p++);

		// applyMatrixOperation(r, g, b, matrix);
		ir = floor((double)r * c00 + (double)g * c10 + (double)b * c20);
		ig = floor((double)r * c01 + (double)g * c11 + (double)b * c21);
		ib = floor((double)r * c02 + (double)g * c12 + (double)b * c22);

		*(q++) = (TRGB)std::min(std::max(ir, 0), 255);;
		*(q++) = (TRGB)std::min(std::max(ig, 0), 255);
		*(q++) = (TRGB)std::min(std::max(ib, 0), 255);
	}

	return dst.data();
}


PRGB TBitmapScaler::monochrome(const TRGBData& src, TRGBData& dst, int sx, int sy) {
	TRGB *p, *q;
	TRGB bw;
	double s;

	// Calculate destination buffer size
	size_t px = sx*sy;
	size_t sz = 3*px;
	if(sz <= 0)	{
		dst = src;
		return dst.data();
	}

	// Resize destination buffer
	dst.resize(sz, false);
	p = src.data();
	q = dst.data();

	// Iterate over all source image pixels
	for (size_t i=0; i<px; i++) {

		// Convert RGB to monochrome values by weighed average based on ITU Rec.709
		s  = (double)(*(p++)) * COLOR_WEIGHT_ITU709_R;
		s += (double)(*(p++)) * COLOR_WEIGHT_ITU709_G;
		s += (double)(*(p++)) * COLOR_WEIGHT_ITU709_B;
		bw = floor(s);
		*(q++) = bw;
		*(q++) = bw;
		*(q++) = bw;

	}

	return dst.data();
}


PRGB TBitmapScaler::negative(const TRGBData& src, TRGBData& dst, int sx, int sy) {
	TRGB *p, *q;

	// Calculate destination buffer size
	size_t px = sx*sy;
	size_t sz = 3*px;
	if(sz <= 0)	{
		dst = src;
		return dst.data();
	}

	// Resize destination buffer
	dst.resize(sz, false);
	p = src.data();
	q = dst.data();

	// Iterate over all source image pixels
	for (size_t i=0; i<px; i++) {

		// Invert RGB values
		*(q++) = 255 - (*(p++));
		*(q++) = 255 - (*(p++));
		*(q++) = 255 - (*(p++));

	}

	return dst.data();
}


//
// Convert 24 Bit to 16 Bit bitmap: RGB888 to RGB555
//
PRGB TBitmapScaler::convert888to555Simple(const TRGBData& src, TRGBData& dst, int sx, int sy) {
	TRGB *q, *origin = src.data();

	size_t sz = sx*sy; // *2 for 16 Bit, see for() loop...
	if(sz <= 0)
		return nil;

	dst.resize(sz*2, false);
	q = dst.data();

	for(size_t i=0; i<sz; i++) {
		q[i*2]   = ((origin[3*i]   >> 1) & 0x7C) | (origin[3*i+1] >> 6);
		q[i*2+1] = ((origin[3*i+1] << 2) & 0xE0) | (origin[3*i+2] >> 3);
	}

	return dst.data();
}

inline void TBitmapScaler::convert32to15(TRGB r, TRGB g , TRGB b , TRGB* d) {
	static TRGB r5;
	static TRGB g5;
	static TRGB b5;
	r5 = r >> 3;
	g5 = g >> 3;
	b5 = b >> 3;
	if (r < 0xF8 && ( r & 0x04 ) ) r5++;
	if (g < 0xF8 && ( g & 0x04 ) ) g5++;
	if (b < 0xF8 && ( b & 0x04 ) ) b5++;

	*d     = ((r5 << 2) | (g5 >> 3));
	*(d+1) = ((g5 << 5) | b5);
}

//
// This is an implementation of the Floyd Steinberg error diffusion algorithm
// adapted for 24bit to 15bit color reduction.
//
// For a description of the base algorithm see e.g.:
// http://www.informatik.fh-muenchen.de/~schieder/graphik-01-02/slide0264.html
//
#define FS_CALC_ERROR_COMMON(color, index) \
				component = p[index]; \
				if (component > 252)component=252 ; if (component < 4)component=4; \
				pixel = (component + (this_line_error_##color[ix]>>4)); \
				color = (pixel & 0xF8) | 0x4; \
				error = pixel - color; \

#define FS_CALC_ERROR_RIGHT(color, index) \
				FS_CALC_ERROR_COMMON(color,index) \
				this_line_error_##color[ix+1] += (error * 7); \
				next_line_error_##color[ix-1] += (error * 3); \
				next_line_error_##color[ix]   += (error * 5); \
				next_line_error_##color[ix+1] += error;

#define FS_CALC_ERROR_LEFT(color, index) \
				FS_CALC_ERROR_COMMON(color,index) \
				this_line_error_##color[ix-1] += (error * 7); \
				next_line_error_##color[ix+1] += (error * 3); \
				next_line_error_##color[ix]   += (error * 5); \
				next_line_error_##color[ix-1] += error;


//
// Convert 24 Bit to 16 Bit bitmap
// Dithered by Floyd Steinberg Error diffusion algorithm
//
PRGB TBitmapScaler::convert888to555Dithered(const TRGBData& src, TRGBData& dst, int sx, int sy) {
	bool odd = true;
	int ix, iy, error, pixel;
	TRGB r, g, b, component;
	TRGB *q, *p = src.data();
	size_t bs = sx + 2;
	size_t is = bs * sizeof(int);

	size_t sz = sx*sy*2;
	if(sz <= 0)
		return nil;

	dst.resize(sz, false);
	q = dst.data();

	int *this_line_error_r;
	int *this_line_error_g;
	int *this_line_error_b;
	int *next_line_error_r;
	int *next_line_error_g;
	int *next_line_error_b;
	int *save_error_r;
	int *save_error_g;
	int *save_error_b;

	// Using unique_ptr instead of deprecated malloc:
	// int *error1_r = (int*)malloc((x+2)*sizeof(int)) --> std::unique_ptr<int[]> error1_r(new int[bs])
	// --> Remember to replace the round brackets of malloc with square bracket to allocate an array
	std::unique_ptr<int[]> error1_r(new int[bs]);
	std::unique_ptr<int[]> error1_g(new int[bs]);
	std::unique_ptr<int[]> error1_b(new int[bs]);
	std::unique_ptr<int[]> error2_r(new int[bs]);
	std::unique_ptr<int[]> error2_g(new int[bs]);
	std::unique_ptr<int[]> error2_b(new int[bs]);

	this_line_error_r = error1_r.get();
	this_line_error_g = error1_g.get();
	this_line_error_b = error1_b.get();
	next_line_error_r = error2_r.get();
	next_line_error_g = error2_g.get();
	next_line_error_b = error2_b.get();

	memset(this_line_error_r, 0 , is);
	memset(this_line_error_g, 0 , is);
	memset(this_line_error_b, 0 , is);
	memset(next_line_error_r, 0 , is);
	memset(next_line_error_g, 0 , is);
	memset(next_line_error_b, 0 , is);

	for (iy=0; iy<sy; iy++) {
		save_error_r = this_line_error_r;
		this_line_error_r = next_line_error_r;
		next_line_error_r = save_error_r;
		save_error_g = this_line_error_g;
		this_line_error_g = next_line_error_g;
		next_line_error_g = save_error_g;
		save_error_b = this_line_error_b;
		this_line_error_b = next_line_error_b;
		next_line_error_b = save_error_b;

		memset(next_line_error_r, 0 , is);
		memset(next_line_error_g, 0 , is);
		memset(next_line_error_b, 0 , is);

		if (odd) {
			for(ix=1; ix<=sx; ix++) {
				FS_CALC_ERROR_RIGHT(r,0);
				FS_CALC_ERROR_RIGHT(g,1);
				FS_CALC_ERROR_RIGHT(b,2);
				convert32to15(r,g,b,q);
				p += 3;
				q += 2;
			}
			odd = false;
		} else {
			p += (sx-1)*3;
			q += (sx-1)*2;
			for(ix=sx; ix>=1; ix--) {
				FS_CALC_ERROR_LEFT(r,0);
				FS_CALC_ERROR_LEFT(g,1);
				FS_CALC_ERROR_LEFT(b,2);
				convert32to15(r,g,b,q);
				p -= 3;
				q -= 2;
			}
			p += sx*3;
			q += sx*2;
			odd = true;
		}
	}

	return dst.data();
}


bool TBitmapScaler::checkBuffer(const TRGBData& buffer, int width, int height, const std::string location) {
	size_t n = buffer.size();

	if (width <= 0 || height <= 0)
		throw util::app_error(location + " : Invalid dimension (" + std::to_string((size_s)width) + "," + std::to_string((size_s)height) + ")");

	if (n % BYTES_PER_PIXEL)
		throw util::app_error(location + " : Buffer not aligned to RGB 24 Bit.");

	if ((size_t)(width * height * BYTES_PER_PIXEL) > n)
		throw util::app_error(location + " : Buffer too small.");

	return true;
}


void TBitmapScaler::copy(TRGB* dst, const TRGB* src, size_t n) {
	memcpy(reinterpret_cast<void*>(dst), reinterpret_cast<const void*>(src), n);
}


void TBitmapScaler::reverse(const TRGBData& src, TRGBData& dst, int width, int height) {
	dst.resize(src.size(), false);
	const PRGB p = src.data();
	PRGB q = dst.data();
	int n = src.size();

	if (!checkBuffer(src, width, height, "TBitmapScaler::reverse()"))
		return;

	int xd = 0;
	int yd = 0;
	int yws;
	int ywd;
	int os;
	int od;

	for (int ys=height-1; ys>=0; ys--) {
		xd = 0;
		ywd = yd * width;
		yws = ys * width;
		for (int xs=0; xs<width; xs++) {
			od = (xd + ywd) * 3;
			os = (xs + yws) * 3 + 2;

			if ((os >= n) || (od + 2 >= n)) {
				std::cout << "xs  = " << xs << std::endl;
				std::cout << "ys  = " << ys << std::endl;
				std::cout << "xd  = " << xd << std::endl;
				std::cout << "yd  = " << yd << std::endl;
				std::cout << "n   = " << n << std::endl;
				std::cout << "os  = " << os << std::endl;
				std::cout << "od  = " << od << std::endl;
				std::cout << "ywd = " << ywd << std::endl;
				std::cout << "yws = " << yws << std::endl;
				throw util::app_error("TBitmapScaler::reverse() : Buffer overflow detected.");
			}

			*(q + od++) = *(p + os--);
			*(q + od++) = *(p + os--);
			*(q + od)   = *(p + os);
			xd++;
		}
		yd++;
	}

}


void TBitmapScaler::rgbToAlpha(const TRGBData& src, TRGBData& dst, int width, int height) {

	if (!checkBuffer(src, width, height, "TBitmapScaler::rgbToAlpha()"))
		return;

	// Resize to 4 bytes RGBA
	size_t size = (src.size() / 3) * 4;
	dst.resize(size, false);
	const PRGB p = src.data();
	PRGB q = dst.data();

	int ys;
	int os;
	int od;
	for (int y=0; y<height; y++) {
		ys = y * width;
		for (int x=0; x<width; x++) {
			os = (x + ys) * 3;
			od = (x + ys) * 4;

			*(q + od++) = 0;			// A (Alpha channel is empty)
			*(q + od++) = *(p + os++);	// R
			*(q + od++) = *(p + os++);	// G
			*(q + od)   = *(p + os);	// B

		}
	}
}


void TBitmapScaler::mirrorX(const TRGBData& src, TRGBData& dst, int width, int height) {
	dst.resize(src.size(), false);
	const PRGB p = src.data();
	PRGB q = dst.data();

	if (!checkBuffer(src, width, height, "TBitmapScaler::mirrorX()"))
		return;

	int yd = 0;
	int sx = width * 3;
	for (int ys = util::pred(height); ys >= 0; ys--) {
		copy(q, p + (ys * sx), sx);
		q += sx;
		yd++;
	}
}


void TBitmapScaler::mirrorY(const TRGBData& src, TRGBData& dst, int width, int height) {
	dst.resize(src.size(), false);
	const TRGB* p = src.data();
	TRGB* q = dst.data();

	if (!checkBuffer(src, width, height, "TBitmapScaler::mirrorY()"))
		return;

	int xd;
	int sx = width * 3;
	for (int ys = 0; ys < height; ys++) {
		xd = util::pred(width);
		for (int xs = 0; xs < width; xs++) {
			copy(q + xd * 3 + ys * sx, p + xs * 3, 3);
			xd--;
		}
		p += sx;
	}
}


// Remember that width and height is reversed afterwards!
void TBitmapScaler::rotate90(const TRGBData& src, TRGBData& dst, int width, int height) {
	dst.resize(src.size(), false);
	const TRGB* p = src.data();
	TRGB* q = dst.data();

	if (!checkBuffer(src, width, height, "TBitmapScaler::rotate90()"))
		return;

	int xd = util::pred(height);
	int yd;
	int sx = width * 3;
	int sy = height * 3;
	for (int ys = 0; ys < height; ys++) {
		yd = 0;
		for (int xs = 0; xs < width; xs++) {
			copy(q + xd * 3 + yd * sy, p + xs * 3, 3);
			yd++;
		}
		p += sx;
		xd--;
	}
}


void TBitmapScaler::rotate180(const TRGBData& src, TRGBData& dst, int width, int height) {
	dst.resize(src.size(), false);
	const TRGB* p = src.data();
	TRGB* q = dst.data();

	if (!checkBuffer(src, width, height, "TBitmapScaler::rotate180()"))
		return;

	int xd;
	int yd = util::pred(height);
	int sx = width * 3;
	for (int ys = 0; ys < height; ys++) {
		xd = util::pred(width);
		for (int xs = 0; xs < width; xs++) {
			copy(q + xd * 3 + yd * sx, p + xs * 3, 3);
			xd--;
		}
		p += sx;
		yd--;
	}
}


// Remember that width and height is reversed afterwards!
void TBitmapScaler::rotate270(const TRGBData& src, TRGBData& dst, int width, int height) {
	dst.resize(src.size(), false);
	const TRGB* p = src.data();
	TRGB* q = dst.data();

	if (!checkBuffer(src, width, height, "TBitmapScaler::rotate270()"))
		return;

	int xd = 0;
	int yd;
	int sx = width * 3;
	int sy = height * 3;
	for (int ys = 0; ys < height; ys++) {
		yd = util::pred(width);
		for (int xs = 0; xs < width; xs++) {
			copy(q + xd * 3 + yd * sy, p + xs * 3, 3);
			yd--;
		}
		p += sx;
		xd++;
	}
}



TPicture::TPicture() {
	prime();
}

TPicture::TPicture(const TPicture& o) {
	prime();
	assign(o);
}

TPicture::TPicture(TPicture&& o) {
	prime();
	move(o);
}

void TPicture::prime() {
	downsampling = false;
	scaling = ESM_DEFAULT;
	clear();
}

double TPicture::ratio(const TRGBSize width, const TRGBSize height) {
	if (height)
		return (double)width / (double)height;
	return 0.0;
}


TPicture& TPicture::operator = (const TPicture& picture) {
	assign(picture);
	return *this;
}

void TPicture::assign(const TPicture& picture) {
	clear();
	if (picture.hasImage()) {
		image = picture.getImage();
		imageWidth = picture.width();
		imageHeight = picture.height();
	}
}

void TPicture::move(TPicture& picture) {
	clear();
	if (picture.hasImage()) {
		image.move(picture.image);
		imageWidth = picture.imageWidth;
		imageHeight = picture.imageHeight;
		picture.imageWidth = 0;
		picture.imageHeight = 0;
	}
}

int TPicture::getColorRange() {
	return TBitmapScaler::getColorRange(image, imageWidth, imageHeight);
}

PRGB TPicture::resizer(const TRGBData& src, TRGBData& dst, int sx, int sy, int dx, int dy) {
	PRGB retVal = nil;
	switch (scaling) {
		case ESM_SIMPLE:
			retVal = resizeSimple(src, dst, sx, sy, dx, dy);
			break;

#ifdef TARGET_X64
		case ESM_DEFAULT:
#endif
		case ESM_BILINEAR:
			retVal = resizeBilinear(src, dst, sx, sy, dx, dy);
			break;

#ifndef TARGET_X64
		case ESM_DEFAULT:
#endif
		case ESM_DITHER:
		default:
			retVal = resizeColorAverage(src, dst, sx, sy, dx, dy);
			break;
	}
	return retVal;
}

bool TPicture::resizeX(const TRGBSize targetWidth) {
	if (imageWidth > 0 && imageHeight > 0 && targetWidth > 0) {
		double r = getRatio();
		if (r > 0.0) {
			TRGBSize targetHeight = (double)targetWidth / r;
			return resize(targetWidth, targetHeight);
		}
	}
	return false;
}

bool TPicture::resizeY(const TRGBSize targetHeight) {
	if (imageWidth > 0 && imageHeight > 0 && targetHeight > 0) {
		TRGBSize targetWidth = (double)targetHeight * getRatio();
		if (targetWidth > 0) {
			return resize(targetWidth, targetHeight);
		}
	}
	return false;
}

bool TPicture::resize(const TRGBSize targetWidth, const TRGBSize targetHeight) {
	TRGBData bitmap;
	PRGB retVal = nil;

	// Check prerequisites!
	if (image.size() <= 0 || targetWidth <= 0 || targetHeight <= 0)
		return false;

	// Resize needed?
	if (targetWidth != imageWidth || targetHeight != imageHeight) {
		retVal = resizer(image, bitmap, imageWidth, imageHeight, targetWidth, targetHeight);
	}

	if (downsampling && util::assigned(retVal) && !bitmap.empty()) {
		retVal = convert888to555Dithered(bitmap, image, targetWidth, targetHeight);
	} else {
		if (!bitmap.empty())
			image.move(bitmap);
	}

	if (util::assigned(retVal)) {
		imageWidth = targetWidth;
		imageHeight = targetHeight;
		return true;
	}

	return false;
}

bool TPicture::align(const TRGBSize dimension, const TColor& color) {
	TRGBData bitmap;

	// Check prerequisites!
	if (image.size() <= 0 || dimension <= 0)
		return false;

	// What has to be done?
	TRGBSize pictureWidth = dimension;
	TRGBSize pictureHeight = dimension;
	double ratio = getRatio();
	bool scaleX = false;
	bool scaleY = false;

	// #####
	// #   #
	// #   #
	// #####
	if (imageHeight > imageWidth) {
		pictureWidth = (double)dimension * ratio;
		scaleY = true;
	}

	// #######
	// #     #
	// #######
	if (imageWidth > imageHeight) {
		pictureHeight = (double)dimension / ratio;
		scaleX = true;
	}

	// Resize needed?
	if (scaleX || scaleY) {
		resizer(image, bitmap, imageWidth, imageHeight, pictureWidth, pictureHeight);
		if (!bitmap.empty()) {
			imageWidth = pictureWidth;
			imageHeight = pictureHeight;
		}
	} else {
		bitmap.move(image);
	}

	// Fill overlay pixel areas
	if (!bitmap.empty()) {
		overlay(bitmap, image, imageWidth, imageHeight, dimension, dimension, color);
		imageWidth = dimension;
		imageHeight = dimension;
		return true;
	}

	return false;
}

bool TPicture::scale(const TRGBSize dimension, const TColor& color) {
	if (dimension > 48) {
		int d = abs(imageWidth - imageHeight);
		int m = std::max(imageWidth, imageHeight);
		int p = d * 100 / m;
		if (p > 10) {
			return align(dimension, color);
		}
	}
	return resize(dimension, dimension);
}

void TPicture::mirrorX() {
	TRGBData bitmap;
	TBitmapScaler::mirrorX(image, bitmap, imageWidth, imageHeight);
	image.move(bitmap);
}

void TPicture::mirrorY() {
	TRGBData bitmap;
	TBitmapScaler::mirrorY(image, bitmap, imageWidth, imageHeight);
	image.move(bitmap);
}

void TPicture::rotate90() {
	TRGBData bitmap;
	TBitmapScaler::rotate90(image, bitmap, imageWidth, imageHeight);
	util::exchange(imageWidth, imageHeight);
	image.move(bitmap);
}

void TPicture::rotate180() {
	TRGBData bitmap;
	TBitmapScaler::rotate180(image, bitmap, imageWidth, imageHeight);
	image.move(bitmap);
}

void TPicture::rotate270() {
	TRGBData bitmap;
	TBitmapScaler::rotate270(image, bitmap, imageWidth, imageHeight);
	util::exchange(imageWidth, imageHeight);
	image.move(bitmap);
}

void TPicture::filter(const TFilterMatrix& matrix) {
	TRGBData bitmap;
	TBitmapScaler::filter(image, bitmap, imageWidth, imageHeight, matrix);
	image.move(bitmap);
}

void TPicture::contrast(double factor) {
	TRGBData bitmap;
	TBitmapScaler::contrast(image, bitmap, imageWidth, imageHeight, factor);
	image.move(bitmap);
}

void TPicture::blur(double factor) {
	TRGBData bitmap;
	TBitmapScaler::blur(image, bitmap, imageWidth, imageHeight, factor);
	image.move(bitmap);
}

void TPicture::edgy(double factor) {
	TRGBData bitmap;
	TBitmapScaler::edgy(image, bitmap, imageWidth, imageHeight, factor);
	image.move(bitmap);
}

void TPicture::brightness(double factor) {
	TRGBData bitmap;
	TBitmapScaler::brightness(image, bitmap, imageWidth, imageHeight, factor);
	image.move(bitmap);
}

void TPicture::saturation(double factor) {
	TRGBData bitmap;
	TBitmapScaler::saturation(image, bitmap, imageWidth, imageHeight, factor);
	image.move(bitmap);
}

void TPicture::monochrome() {
	TRGBData bitmap;
	TBitmapScaler::monochrome(image, bitmap, imageWidth, imageHeight);
	image.move(bitmap);
}

void TPicture::negative() {
	TRGBData bitmap;
	TBitmapScaler::negative(image, bitmap, imageWidth, imageHeight);
	image.move(bitmap);
}

size_t TPicture::encode(const std::string& fileName, const TRGBData& data, const TRGBSize width, const TRGBSize height) {
	throw util::app_error("TPicture::encode() not implemented.");
}

size_t TPicture::decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height) {
	throw util::app_error("TPicture::decode() not implemented.");
}



TPictureFile::TPictureFile() {
}

TPictureFile::TPictureFile(const std::string& file) : file(file) {
}


void TPictureFile::loadFromFile() {
	clear();
	if (validFile() && util::fileExists(file)) {
		if (decode(getFile(), image, imageWidth, imageHeight) <= 0) {
			TPicture::clear();
		}
	}
}

void TPictureFile::loadFromFile(const std::string& fileName) {
	setFile(fileName);
	loadFromFile();
}

size_t TPictureFile::saveToFile() {
	if (validFile())
		return saveToFile(file);
	return (size_t)0;
}

size_t TPictureFile::saveToFile(const std::string& fileName) {
	if (!fileName.empty() && hasImage())
		return encode(fileName, image, imageWidth, imageHeight);
	return (size_t)0;
}


void TPictureFile::fill(int width, int height, const TColor& color) {
	imageWidth = width;
	imageHeight = height;
	TBitmapPainter::fill(image, imageWidth, imageHeight, color);
}

void TPictureFile::line(int x0, int y0, int x1, int y1, const TColor& color) {
	TBitmapPainter::line(image, x0, y0, x1, y1, imageWidth, imageHeight, color);
}

void TPictureFile::line(int x0, int y0, int x1, int y1, int outline, const TColor& color) {
	TBitmapPainter::line(image, x0, y0, x1, y1, imageWidth, imageHeight, outline, color);
}

void TPictureFile::circle(int x0, int y0, int radius, const TColor& color) {
	TBitmapPainter::circle(image, x0, y0, radius, imageWidth, imageHeight, color);
}

void TPictureFile::rect(int x0, int y0, int width, int height, int outline, const TColor& color) {
	TBitmapPainter::rect(image, x0, y0, width, height, imageWidth, imageHeight, outline, color);
}

void TPictureFile::square(int x0, int y0, int width, int height, const TColor& color) {
	TBitmapPainter::square(image, x0, y0, width, height, imageWidth, imageHeight, color);
}

void TPictureFile::ellipse(int x0, int y0, int a, int b, const TColor& color) {
	TBitmapPainter::ellipse(image, x0, y0, a, b, imageWidth, imageHeight, color);
}

void TPictureFile::bezier2(int x0, int y0, int x1, int y1, int x2, int y2, const TColor& color) {
	TBitmapPainter::bezier2(image, x0, y0, x1, y1, x2, y2, imageWidth, imageHeight, color);
}

void TPictureFile::draw(int x, int y, const TColor& color) {
	TBitmapPainter::draw(image, x, y, imageWidth, imageHeight, color);
}




TJpegError::TJpegError() {
	memset(&error, 0, sizeof(error));
	manager = jpeg_std_error(&error.manager);
	error.manager.error_exit = jpegErrorCallback;
}

std::string TJpegError::getLastError() {
	size_t size = strnlen(error.message, JMSG_LENGTH_MAX);
	if (size > 0)
		return std::string(error.message, size);
	return "";
}



TJpegAction::TJpegAction() {
	created = false;
}



TJpegDecoder::TJpegDecoder() {
	memset(&decoder, 0, sizeof(decoder));
	decoder.client_data = util::asClass<TJpegAction>(this);
	decoder.err = getErrorManager();
	limit = MAX_BITMAP_SIZE;
	jpeg_create_decompress(&decoder);
	created = true;
}

TJpegDecoder::~TJpegDecoder() {
	clear();
}

void TJpegDecoder::clear() {
	if (created) {
		created = false;
		jpeg_destroy_decompress(&decoder);
	}
}

bool TJpegDecoder::isValid() {
	if (created) {
		if ((decoder.output_width > 0) && \
			(decoder.output_height > 0) && \
			(decoder.output_width < limit) && \
			(decoder.output_height < limit) && \
			(decoder.output_components == 3)) {
			return true;
		}
	}
	return false;
}


TJpegEncoder::TJpegEncoder() {
	memset(&encoder, 0, sizeof(encoder));
	encoder.client_data = util::asClass<TJpegAction>(this);
	encoder.err = getErrorManager();
	jpeg_create_compress(&encoder);
	created = true;
}

TJpegEncoder::~TJpegEncoder() {
	clear();
}

void TJpegEncoder::clear() {
	if (created) {
		created = false;
		jpeg_destroy_compress(&encoder);
	}
}




TJpeg::TJpeg() {
	prime();
}

TJpeg::TJpeg(const std::string& file) : TPictureFile(file) {
	prime();
};


void TJpeg::prime() {
	quality = 98;
}


bool TJpeg::isJPEG(const TRGBData& data) const {
	return isJPEG(data.data(), data.size());
}

bool TJpeg::isJPEG(const TRGB* data, size_t size) {
	if (util::assigned(data) && size > MIN_JPEG_SIZE) {
		if (((data[6]=='J' && data[7]=='F' && data[8]=='I' && data[9]=='F') ||
			(data[6]=='E' && data[7]=='x' && data[8]=='i' && data[9]=='f')) &&
				data[0]==0xFF && data[1]==0xD8 && data[2]==0xFF)
			return true;
	}
	return false;
}

bool TJpeg::isJFIF(const TRGB* data, size_t size) {
	if (util::assigned(data) && size > MIN_JPEG_SIZE) {
		if (data[6]=='J' && data[7]=='F' && data[8]=='I' && data[9]=='F' &&
			data[0]==0xFF && data[1]==0xD8 && data[2]==0xFF && data[3]==0xE0)
			return true;
	}
	return false;
}

bool TJpeg::isExif(const TRGB* data, size_t size) {
	if (util::assigned(data) && size > MIN_EXIF_SIZE) {
		if (data[6]=='E' && data[7]=='x' && data[8]=='i' && data[9]=='f' && data[10]==0x00 && data[11]==0x00 &&
			data[2]==0xFF && data[3]==0xE1)
			return true;
	}
	return false;
}


bool TJpeg::read(const util::TBaseFile& file, void *const data, const size_t size) const {
	try {
		return (ssize_t)size == file.read(data, size, 0, util::SO_FROM_START);
	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		throw util::app_error("TJpeg::read()::" + sExcept);
	} catch (...) {
		throw util::app_error("TJpeg::read() Unknown exception.");
	}
	return false;
}

bool TJpeg::seek(const util::TBaseFile& file, const size_t offset) const {
	if (offset != std::string::npos) {
		return ((ssize_t)offset == file.seek(offset, util::SO_FROM_START));
	}
	return false;
}


int TJpeg::readExifOrientation() const {
	return readExifOrientation(getFile());
}

/*
 * The Exif orientation value gives the orientation of the camera
 * relative to the scene when the image was captured.  The relation
 * of the '0th row' and '0th column' to visual position is shown as
 * below.
 *
 * Value | 0th Row     | 0th Column
 * ------+-------------+-----------
 *   1   | top         | left side
 *   2   | top         | right side
 *   3   | bottom      | right side
 *   4   | bottom      | left side
 *   5   | left side   | top
 *   6   | right side  | top
 *   7   | right side  | bottom
 *   8   | left side   | bottom
 *
 * For convenience, here is what the letter F would look like if it were
 * tagged correctly and displayed by a program that ignores the orientation
 * tag:
 *
 *   1        2       3      4         5            6           7          8
 *
 * 888888  888888      88  88      8888888888  88                  88  8888888888
 * 88          88      88  88      88  88      88  88          88  88      88  88
 * 8888      8888    8888  8888    88          8888888888  8888888888          88
 * 88          88      88  88
 * 88          88  888888  888888
 *
 */
int TJpeg::readExifOrientation(const std::string& fileName) const {

	// Check prerequisites...
	if (fileName.empty() || !util::fileExists(fileName))
		return -1;

	// Open file
	util::TBaseFile jpg;
	jpg.open(fileName, O_RDONLY);
	if (!jpg.isOpen())
		return -2;

	// Read EXIF file header
	size_t hsize = sizeof(TExifFileHeader);
	TExifFileHeader header;
	if (!read(jpg, &header, hsize))
		return -3;

	// Check for valid EXIF header
	size_t length = 0;
	if (!isExif((TRGB*)&header, hsize)) {

		// Check for JFIF header
		if (isJFIF((TRGB*)&header, hsize)) {

			// Skip size of JFIF Header and read following EXIF header
			length = (size_t)convertFromBigEndian16(header.exSize);
			size_t pos = length + 2;
			if (seek(jpg, pos)) {
				if (!read(jpg, &header, hsize))
					return -42;
				if (!isExif((TRGB*)&header, hsize))
					return -43;
			} else {
				return -41;
			}

		} else {
			return -4;
		}
	}

	// Length includes itself, so must be at least 2
	// Following EXIF data length must be at least 6
	length = (size_t)convertFromBigEndian16(header.exSize);
	if (length < 8)
		return -5;
	length -= 8;

	// Length of an IFD entry
	if (length < 12)
		return -6;

	// Limit length to some handsome value...
	if (length >= MAX_EXIF_HEADER_SIZE)
		length = MAX_EXIF_HEADER_SIZE;

	// Read complete EXIF data from file
	TRGBData exif(length);
	if (!read(jpg, exif(), length))
		return -7;

	// Discover byte order
	EEndianType endian = EE_UNKNOWN_ENDIAN;
	if (exif[0] == 0x49 && exif[1] == 0x49)
		endian = EE_LITTLE_ENDIAN;
	else if (exif[0] == 0x4D && exif[1] == 0x4D)
		endian = EE_BIG_ENDIAN;

	// Check for valid endian
	if (endian == EE_UNKNOWN_ENDIAN)
		return -8;

	// Check tag mark
	if (endian == EE_BIG_ENDIAN) {
		if (exif[2] != 0) return -9;
		if (exif[3] != 0x2A) return -9;
	} else {
		if (exif[3] != 0) return -9;
		if (exif[2] != 0x2A) return -9;
	}

	// Get first IFD offset (offset to IFD0)
	size_t offset = 0;
	if (endian == EE_BIG_ENDIAN) {
		if (exif[4] != 0) return -10;
		if (exif[5] != 0) return -10;
		offset = exif[6];
		offset <<= 8;
		offset += exif[7];
	} else {
		if (exif[7] != 0) return -10;
		if (exif[6] != 0) return -10;
		offset = exif[5];
		offset <<= 8;
		offset += exif[4];
	}

	// Check end of data segment
	if (offset > length - 2)
		return -11;

	// Get the number of directory entries contained in this IFD
	int count = 0;
	if (endian == EE_BIG_ENDIAN) {
		count = exif[offset];
		count <<= 8;
		count += exif[offset+1];
	} else {
		count = exif[offset+1];
		count <<= 8;
		count += exif[offset];
	}

	// Check for valid tag entries
	if (count <= 0)
		return -12;
	offset += 2;

	// Search for Orientation Tag in IFD0
	int tag = 0;
	for (;;) {
		// Check end of data segment
		if (offset > length - 12)
			return -13;

		// Get tag number
		if (endian == EE_BIG_ENDIAN) {
			tag = exif[offset];
			tag <<= 8;
			tag += exif[offset+1];
		} else {
			tag = exif[offset+1];
			tag <<= 8;
			tag += exif[offset];
		}

		// Found Orientation tag?
		if (tag == EXIF_TAG_ORIENTATION)
			break;

		// Check for end of tag list
		if (--count <= 0)
			return -14;

		offset += 12;
	}

	// Get the orientation value
	TRGB orientation = 0;
	if (endian == EE_BIG_ENDIAN) {
		if (exif[offset+8] != 0)
			return -15;
		orientation = exif[offset+9];
	} else {
		if (exif[offset+9] != 0)
			return -16;
		orientation = exif[offset+8];
	}

	// Valid orientation value found?
	if (orientation > 0 && orientation < 9)
		return (int)orientation;

	return (int)0;
}


bool TJpeg::realign(int& orientation) {
	orientation = 0;
	bool rotated = false;
	if (hasImage()) {
		// Rotate to fit correct aspect
		orientation = readExifOrientation();
		if (orientation == 8) {
			rotate270();
			rotated = true;
		} else {
			if (orientation == 6) {
				rotate90();
				rotated = true;
			}
		}
	}
	return rotated;
}


size_t TJpeg::decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height) {
	return decode(fileName, data, width, height, 0, 0);
}

size_t TJpeg::decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height, const TRGBSize targetWidth, const TRGBSize targetHeight) {
	size_t r;
	PRGB bp, lb;
	int px,py,c;
	TStdioFile file;
	TRGBData magic(10);
	data.clear();

	// Check prerequisites!
	if (!util::fileExists(fileName))
		return (size_t)0;

	file.open(fileName, "rb");
	if (!file.isOpen())
		throw util::sys_error("TJpeg::decode() : Open <" + fileName + "> failed.", errno);

	// Read magic header bytes
	r = fread(magic(), magic.ordinal(), magic.size(), file());
	if (r != magic.size()) {
		throw util::app_error("TJpeg::decode() : Can't read magic bytes on file <" + fileName + ">");
		return 0;
	}

	// Do magic number check on file header
	if (!isJPEG(magic)) {
		throw util::app_error("TJpeg::decode() : Magic number test for file <" + fileName + "> failed.");
		return 0;
	}

	// Seek back to beginning of file
	fseek(file(), 0, SEEK_SET);

	TJpegDecoder decoder;
	if (!decoder.setLongjump()) {
	    std::cout << app::red << "TJpeg::decode()::TJpegError() : \"" << decoder.getLastError() << "\"" << app::reset << std::endl;
	    std::cout << app::red << "TJpeg::decode()::TJpegError() : Processed file <" << fileName << ">" << app::reset << std::endl;

	    // Destructor should also do that...
	    decoder.clear();
		file.close();

		throw util::app_error("TJpeg::decode()::TJpegError() : Error \"" + decoder.getLastError() + "\" occurred in file <" + fileName + ">");
		return 0;
	}

	jpeg_stdio_src(decoder(), file());
	jpeg_read_header(decoder(), TRUE);

	// Deprecated...
	// ciptr->dct_method = JDCT_IFAST;
	// ciptr->out_color_space = JCS_RGB;
	// ciptr->do_fancy_upsampling = FALSE;
	// ciptr->do_block_smoothing = FALSE;

	decoder()->scale_num = 1;

	if (targetWidth > 0 && targetHeight > 0)
	{
		if(decoder()->image_width/8 >= targetWidth ||
				decoder()->image_height/8 >= targetHeight)
			decoder()->scale_denom=8;
		else if(decoder()->image_width/4 >= targetWidth ||
				decoder()->image_height/4 >= targetHeight)
			decoder()->scale_denom=4;
		else if(decoder()->image_width/2 >= targetWidth ||
				decoder()->image_height/2 >= targetHeight)
			decoder()->scale_denom=2;
		else
			decoder()->scale_denom=1;
	}
	else
		decoder()->scale_denom = 1;

	jpeg_start_decompress(decoder());

	px = decoder()->output_width;
	py = decoder()->output_height;
	c = decoder()->output_components;

	if (decoder.isValid()) {

		data.resize(px*py*c, false);
		bp = data.data();

		lb = (TRGB*)(*decoder()->mem->alloc_small)((j_common_ptr)decoder(), JPOOL_PERMANENT, c*px);

		while(decoder()->output_scanline < decoder()->output_height)
		{
			jpeg_read_scanlines(decoder(), &lb, 1);
			memcpy(bp, lb, px*c);
			bp += px*c;
		}

		width = px;
		height = py;

	} else
		data.clear();

	jpeg_finish_decompress(decoder());

	return px*py;
}


size_t TJpeg::decode(const TFile& file) {
	if (!file.empty() && file.isLoaded()) {
		return decode(file.getData(), file.getSize());
	}
	return (size_t)0;
}

size_t TJpeg::decode(const TBuffer& buffer) {
	if (!buffer.empty()) {
		return decode(buffer.data(), buffer.size());
	}
	return (size_t)0;
}

size_t TJpeg::decode(const char* buffer, const size_t size) {
	return decode(buffer, size, image, imageWidth, imageHeight, 0, 0);
}

size_t TJpeg::decode(const char* buffer, const size_t size, TRGBData& data, TRGBSize& width, TRGBSize& height, const TRGBSize targetWidth, const TRGBSize targetHeight) {
	PRGB bp, lb;
	int px,py,c;
	data.clear();

	// Check prerequisites!
	if (!util::assigned(buffer) && size > MIN_JPEG_SIZE)
		return (size_t)0;

	if (!isJPEG((const TRGB*)buffer, size)) {
		throw util::app_error("TJpeg::decode() : Magic number test failed.");
		return 0;
	}

	TJpegDecoder decoder;
	if (!decoder.setLongjump()) {
	    std::cout << app::red << "TJpeg::decode()::TJpegError() : \"" << decoder.getLastError() << "\"" << app::reset << std::endl;
	    std::cout << app::red << "TJpeg::decode()::TJpegError() : Processed input buffer" << app::reset << std::endl;

	    // Destructor should also do that...
	    decoder.clear();

		throw util::app_error("TJpeg::decode()::TJpegError() : Error \"" + decoder.getLastError() + "\" occurred.");
		return 0;
	}

	jpeg_mem_src(decoder(), (unsigned char*)buffer, size);
	jpeg_read_header(decoder(), TRUE);

	// Deprecated...
	// ciptr->dct_method = JDCT_IFAST;
	// ciptr->out_color_space = JCS_RGB;
	// ciptr->do_fancy_upsampling = FALSE;
	// ciptr->do_block_smoothing = FALSE;

	decoder()->scale_num = 1;

	if (targetWidth > 0 && targetHeight > 0)
	{
		if(decoder()->image_width/8 >= targetWidth ||
				decoder()->image_height/8 >= targetHeight)
			decoder()->scale_denom=8;
		else if(decoder()->image_width/4 >= targetWidth ||
				decoder()->image_height/4 >= targetHeight)
			decoder()->scale_denom=4;
		else if(decoder()->image_width/2 >= targetWidth ||
				decoder()->image_height/2 >= targetHeight)
			decoder()->scale_denom=2;
		else
			decoder()->scale_denom=1;
	}
	else
		decoder()->scale_denom = 1;

	jpeg_start_decompress(decoder());

	px = decoder()->output_width;
	py = decoder()->output_height;
	c = decoder()->output_components;

	if (decoder.isValid()) {

		data.resize(px*py*c, false);
		bp = data.data();

		lb = (TRGB*)(*decoder()->mem->alloc_small)((j_common_ptr)decoder(), JPOOL_PERMANENT, c*px);

		while(decoder()->output_scanline < decoder()->output_height)
		{
			jpeg_read_scanlines(decoder(), &lb, 1);
			memcpy(bp, lb, px*c);
			bp += px*c;
		}

		width = px;
		height = py;

	} else
		data.clear();

	jpeg_finish_decompress(decoder());

	return px*py;
}


size_t TJpeg::encode(const std::string& fileName, const TRGBData& data, const TRGBSize width, const TRGBSize height) {
	TStdioFile file;
	JSAMPROW rp[1];	/* pointer to JSAMPLE row[s] */
	int rs;			/* physical row width in image buffer */

	// Check prerequisites!
	if (fileName.empty() || data.size() <= 0 || quality <= 0 || quality > 100 || width <= 0 || height <= 0)
		return 0;

	file.open(fileName, "wb");
	if (!file.isOpen())
		throw util::sys_error("TJpeg::encode() : Open <" + fileName + "> failed.", errno);

	TJpegEncoder encoder;
	if (!encoder.setLongjump()) {
	    std::cout << app::red << "TJpeg::encode()::TJpegError() : \"" << encoder.getLastError() << "\"" << app::reset << std::endl;
	    std::cout << app::red << "TJpeg::encode()::TJpegError() : Processed file <" << fileName << ">" << app::reset << std::endl;

	    // Destructor should also do that...
	    encoder.clear();
		file.close();

		throw util::app_error("TJpeg::encode()::TJpegError() : Error \"" + encoder.getLastError() + "\" occurred in file <" + fileName + ">");
		return 0;
	}

	jpeg_stdio_dest(encoder(), file());
	encoder()->image_width = width;      /* image width and height, in pixels */
	encoder()->image_height = height;
	encoder()->input_components = 3;     /* # of color components per pixel */
	encoder()->in_color_space = JCS_RGB; /* colorspace of input image */

	/* Now use the library's routine to set default compression parameters.
	 * (You must set at least cinfo.in_color_space before calling this,
	 * since the defaults depend on the source color space.)
	 */
	jpeg_set_defaults(encoder());

	/* Now you can set any non-default parameters you wish to.
	 * Here we just illustrate the use of quality (quantization table) scaling:
	 */
	jpeg_set_quality(encoder(), quality, TRUE /* limit to baseline-JPEG values */);

	/* TRUE ensures that we will write a complete interchange-JPEG file.
	 * Pass TRUE unless you are very sure of what you're doing.
	 */
	jpeg_start_compress(encoder(), TRUE);


	/* Here we use the library's state variable cinfo.next_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 * To keep things simple, we pass one scanline per call; you can pass
	 * more if you wish, though.
	 */
	rs = width * 3;	/* JSAMPLEs per row in image_buffer */
	PRGB p = data();
	TRGBSize r;

	/* while (scan lines remain to be written) */
	/*     jpeg_write_scanlines(...);
	 */
	while (encoder()->next_scanline < encoder()->image_height) {
		/* jpeg_write_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could pass
		 * more than one scanline at a time if that's more convenient.
		 */
		rp[0] = p + (encoder()->next_scanline * rs);
		r += jpeg_write_scanlines(encoder(), rp, 1);
	}

	jpeg_finish_compress(encoder());

	return r;
}


size_t TJpeg::encode(char*& buffer, size_t& size, const TPicture& bitmap) {
	if (!bitmap.hasImage()) {
		return encode(buffer, size, bitmap.getImage(), bitmap.width(), bitmap.height());
	}
	return (size_t)0;
}

size_t TJpeg::encode(char*& buffer, size_t& size) {
	return encode(buffer, size, image, imageWidth, imageHeight);
}

size_t TJpeg::encode(char*& buffer, size_t& size, const TRGBData& data, const TRGBSize width, const TRGBSize height) {
	JSAMPROW rp[1];	/* pointer to JSAMPLE row[s] */
	int rs;			/* physical row width in image buffer */

	// Delete buffer to prevent memory leak
	if (util::assigned(buffer))
		delete[] buffer;
	buffer = nil;
	size = 0;

	// Check prerequisites!
	if (data.size() <= 0 || quality <= 0 || quality > 100 || width <= 0 || height <= 0)
		return 0;

	TJpegEncoder encoder;
	if (!encoder.setLongjump()) {
	    std::cout << app::red << "TJpeg::encode()::TJpegError() : \"" << encoder.getLastError() << "\"" << app::reset << std::endl;

	    // Destructor should also do that...
	    encoder.clear();
		if (util::assigned(buffer))
			delete[] buffer;
		buffer = nil;
		size = 0;

		throw util::app_error("TJpeg::encode()::TJpegError() : \"" + encoder.getLastError() + "\"");
		return 0;
	}

	jpeg_mem_dest(encoder(), (unsigned char**)&buffer, (long unsigned int*)&size);

	encoder()->image_width = width;      /* image width and height, in pixels */
	encoder()->image_height = height;
	encoder()->input_components = 3;     /* # of color components per pixel */
	encoder()->in_color_space = JCS_RGB; /* colorspace of input image */

	/* Now use the library's routine to set default compression parameters.
	 * (You must set at least cinfo.in_color_space before calling this,
	 * since the defaults depend on the source color space.)
	 */
	jpeg_set_defaults(encoder());

	/* Now you can set any non-default parameters you wish to.
	 * Here we just illustrate the use of quality (quantization table) scaling:
	 */
	jpeg_set_quality(encoder(), quality, TRUE /* limit to baseline-JPEG values */);

	/* TRUE ensures that we will write a complete interchange-JPEG file.
	 * Pass TRUE unless you are very sure of what you're doing.
	 */
	jpeg_start_compress(encoder(), TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */

	/* Here we use the library's state variable cinfo.next_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 * To keep things simple, we pass one scanline per call; you can pass
	 * more if you wish, though.
	 */
	rs = width * 3;	/* JSAMPLEs per row in image_buffer */
	PRGB p = data();
	TRGBSize r;

	while (encoder()->next_scanline < encoder()->image_height) {
		/* jpeg_write_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could pass
		 * more than one scanline at a time if that's more convenient.
		 */
		rp[0] = p + (encoder()->next_scanline * rs);
		r += jpeg_write_scanlines(encoder(), rp, 1);
	}

	jpeg_finish_compress(encoder());

	return (size_t)r;
}


void TJpeg::loadPrescaledFromFile(const std::string& fileName, const TRGBSize targetWidth, const TRGBSize targetHeight) {
	if (decode(fileName, image, imageWidth, imageHeight, targetWidth, targetHeight) <= 0) {
		clear();
	} else {
		setFile(fileName);
	}
}


void TJpeg::saveAsBitmap() {
	if (validFile())
		saveAsBitmap(util::fileReplaceExt(getFile(), "bmp"));
}

void TJpeg::saveAsBitmap(const std::string& fileName) {
	TBitmap dib;
	dib.encode(fileName, image, imageWidth, imageHeight);
}

void TJpeg::loadFromBitmap(const std::string& fileName) {
	TBitmap dib;
	dib.decode(fileName, image, imageWidth, imageHeight);
}



TBitmap::TBitmap() {
}

TBitmap::TBitmap(const std::string& file) : TPictureFile(file) {
};

bool TBitmap::isMSDIB(const BITMAPFILEHEADER& header) const {
	// Check for bitmap file header
	if (header.bfType == ('B' | 'M' << 8) &&
		header.bfOffBits == 54) {
		return true;
	}
	return false;
}

void TBitmap::setHeaderEndian(BITMAPINFOHEADER& header) const {
#ifdef TARGET_BIG_ENDIAN
	header.biSize = convertFromLittleEndian32(header.biSize);
	header.biWidth = convertFromLittleEndian32(header.biWidth);
	header.biHeight = convertFromLittleEndian32(header.biHeight);
	header.biPlanes = convertFromLittleEndian16(header.biPlanes);
	header.biBitCount = convertFromLittleEndian16(header.biBitCount);
	header.biCompression = convertFromLittleEndian32(header.biCompression);
	header.biSizeImage = convertFromLittleEndian32(header.biSizeImage);
	header.biXPelsPerMeter = convertFromLittleEndian32(header.biXPelsPerMeter);
	header.biYPelsPerMeter = convertFromLittleEndian32(header.biYPelsPerMeter);
	header.biClrUsed = convertFromLittleEndian32(header.biClrUsed);
	header.biClrImportant = convertFromLittleEndian32(header.biClrImportant);
#endif
}

size_t TBitmap::encode(const std::string& fileName, const TRGBData& data, const TRGBSize width, const TRGBSize height) {
	size_t hsize = sizeof(WORD) + 3*sizeof(DWORD);
	TRGB header[hsize];
	BITMAPINFOHEADER info;
	TRGBData bitmap;
	TStdioFile file;

	// Calculate header and data sizes
	size_t isize = sizeof(info);
	size_t bsize = data.size() * data.ordinal();
	size_t size  = hsize + isize + bsize;

	// Nothing to do?
	if (bsize <= 0 || fileName.empty() || width <= 0 || height <= 0)
		return 0;

	// JPEG uses RGB, Microsoft BMP/DIB has BGR byte order!
	// JPEG orders lines from top to bottom, Microsoft from bottom to top!
	reverse(data, bitmap, width, height);

	// Fill "magic" header bytes
	size_t idx = 0;			*(WORD  *)(header + idx) = 'B' | 'M' << 8;							// bfType 'BM'
	idx += sizeof(WORD);	*(DWORD *)(header + idx) = convertFromLittleEndian32((DWORD)size);	// bfSize
	idx += sizeof(DWORD);	*(DWORD *)(header + idx) = 0;										// bfReserved1, bfReserved2
	idx += sizeof(DWORD);	*(DWORD *)(header + idx) = convertFromLittleEndian32((DWORD)54);	// bfOffBits = 54

	// Fill BMP Header
	info.biSize          = (DWORD)isize;
	info.biWidth         = (DWORD)width;
	info.biHeight        = (DWORD)height;
	info.biPlanes        = (WORD)1;
	info.biBitCount      = (WORD)24;
	info.biCompression   = 0;
	info.biSizeImage     = (DWORD)bsize;
	info.biXPelsPerMeter = 0;
	info.biYPelsPerMeter = 0;
	info.biClrUsed       = 0;
	info.biClrImportant  = 0;

	// Set correct endianess
	setHeaderEndian(info);

	// Write content to file
	file.open(fileName, "wb");
	if (!file.isOpen())
		throw util::sys_error("TJpeg::saveAsBitmap() : Open <" + fileName + "> failed.", errno);

	size_t r = fwrite(header, hsize, 1, file());
	if (r != 1)
		throw util::sys_error("TJpeg::saveAsBitmap() : Writing header to <" + fileName + "> failed.", errno);

	r = fwrite(&info, isize, 1, file());
	if (r != 1)
		throw util::sys_error("TJpeg::saveAsBitmap() : Writing info structure to <" + fileName + "> failed.", errno);

	// Write buffer as one block
	r = fwrite(bitmap(), bitmap.size(), bitmap.ordinal(), file());
	if (r != bitmap.ordinal())
		throw util::sys_error("TJpeg::saveAsBitmap() : Writing bitmap data to <" + fileName + "> failed.", errno);

	return r + hsize + isize;
}


size_t TBitmap::encode(char*& buffer, size_t& size) {
	return encode(buffer, size, image, imageWidth, imageHeight);
}

size_t TBitmap::encode(char*& buffer, size_t& size, const TRGBData& data, const TRGBSize width, const TRGBSize height) {
	size_t hsize = sizeof(WORD) + 3 * sizeof(DWORD);
	TRGB header[hsize];
	BITMAPINFOHEADER info;
	TRGBData bitmap;
	TStdioFile file;

	// Calculate header and data sizes
	size_t isize = sizeof(info);
	size_t bsize = data.size() * data.ordinal();
	size  = hsize + isize + bsize;

	// Delete buffer to prevent memory leak
	if (util::assigned(buffer))
		delete[] buffer;

	// Nothing to do?
	if (bsize <= 0 || width <= 0 || height <= 0)
		return 0;

	// JPEG uses RGB, Microsoft BMP/DIB has BGR byte order!
	// JPEG orders lines from top to bottom, Microsoft from bottom to top!
	reverse(data, bitmap, width, height);

	// Fill "magic" header bytes
	size_t idx = 0;       *(WORD  *)(header + idx) = 'B' | 'M' << 8;                         // bfType 'BM'
	idx += sizeof(WORD);  *(DWORD *)(header + idx) = convertFromLittleEndian32((DWORD)size); // bfSize
	idx += sizeof(DWORD); *(DWORD *)(header + idx) = 0;                                      // bfReserved1, bfReserved2
	idx += sizeof(DWORD); *(DWORD *)(header + idx) = convertFromLittleEndian32((DWORD)54);   // bfOffBits = 54

	// Fill BMP Header
	info.biSize          = (DWORD)isize;
	info.biWidth         = (DWORD)width;
	info.biHeight        = (DWORD)height;
	info.biPlanes        = (WORD)1;
	info.biBitCount      = (WORD)24;
	info.biCompression   = 0;
	info.biSizeImage     = (DWORD)bsize;
	info.biXPelsPerMeter = 0;
	info.biYPelsPerMeter = 0;
	info.biClrUsed       = 0;
	info.biClrImportant  = 0;

	// Set correct endianess
	setHeaderEndian(info);

	// Allocate buffer
	buffer = new char[size];
	if (!util::assigned(buffer))
		throw util::app_error_fmt("TJpeg::saveAsBitmap() : Create buffer failed, size = %", size);

	// Copy bitmap header to buffer
	char* p = buffer;
	memcpy(p, header, hsize);

	// Copy bitmap info structure
	p += hsize;
	memcpy(p, &info, isize);

	// Copy whole bitmap buffer to destination
	p += isize;
	memcpy(p, bitmap(), bsize);

	return size;
}



size_t TBitmap::decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height) {
	BITMAPFILEHEADER header;
	BITMAPINFOHEADER info;
	size_t hsize = sizeof(header);
	size_t isize = sizeof(info);
	TStdioFile file;
	TRGBData bitmap;
	data.clear();

	// Nothing to do?
	if (!util::fileExists(fileName) || fileName.empty())
		return (size_t)0;

	// Read content from file
	file.open(fileName, "rb");
	if (!file.isOpen())
		throw util::sys_error("TJpeg::loadFromBitmap() : Open <" + fileName + "> failed.");

	// Read file header
	size_t r = fread(&header, 1, hsize, file());
	if (r != hsize)
		throw util::sys_error("TJpeg::loadFromBitmap() : Reading header from <" + fileName + "> failed.");

	// Check for bitmap file header
	if (!isMSDIB(header))
		throw util::app_error("TJpeg::loadFromBitmap() : Magic number test for file <" + fileName + "> failed.");

	// Read bitmap info
	r = fread(&info, 1, isize, file());
	if (r != isize)
		throw util::sys_error("TJpeg::loadFromBitmap() : Reading info structure from <" + fileName + "> failed.");

	// Set correct endianess
	setHeaderEndian(info);

	// Is bitmap RGB in 3 bytes?
	if (info.biBitCount != 24)
		throw util::app_error("TJpeg::loadFromBitmap() : Format not supported for file <" + fileName + ">, color size " + std::to_string((size_u)info.biBitCount) + " bit.");

	// Read data size
	size_t pos = ftell(file());
	if (pos != (hsize + isize))
		throw util::app_error("TJpeg::loadFromBitmap() : Invalid header position for <" + fileName + "> at " + std::to_string((size_u)pos) + ".");

	fseek(file(), 0, SEEK_END);
	size_t fsize = ftell(file());
	if (fsize <= pos)
		throw util::app_error("TJpeg::loadFromBitmap() : Invalid file size " + std::to_string((size_u)fsize) + " for <" + fileName + ">");

	// Read bitmap data from file
	size_t size = fsize - pos;
	bitmap.resize(size, false);
	fseek(file(), pos, SEEK_SET);
	r = fread(bitmap(), bitmap.ordinal(), bitmap.size(), file());
	if (r != bitmap.size())
		throw util::sys_error("TJpeg::loadFromBitmap() : Reading bitmap data from <" + fileName + "> failed.");

	// Reverse bitmap order
	width = info.biWidth;
	height = info.biHeight;
	reverse(bitmap, data, width, height);

	// Everything went fine!
	return size;
}



TPNGError::TPNGError() {
}

std::string TPNGError::getLastError() {
	return error.message;
}


TPNGAction::TPNGAction() {
	pimage = nil;
	pinfo = nil;
	created = false;
}



TPNGDecoder::TPNGDecoder() {
	pimage = png_create_read_struct(PNG_LIBPNG_VER_STRING, &error, pngErrorCallback, pngWarningCallback);
	if (!assigned(pimage))
		throw app_error("TPNGDecoder::TPNGDecoder() : png_create_read_struct() failed.");
	pinfo = png_create_info_struct(pimage);
	if (!assigned(pinfo))
		throw app_error("TPNGDecoder::TPNGDecoder() : png_create_info_struct() failed.");
	created = assigned(pimage) && assigned(pinfo);
}

TPNGDecoder::~TPNGDecoder() {
	clear();
}

void TPNGDecoder::clear() {
	if (util::assigned(pimage) || util::assigned(pinfo)) {
		created = false;
		png_destroy_read_struct(&pimage, &pinfo, NULL);
		pimage = nil;
		pinfo = nil;
	}
}



TPNGEncoder::TPNGEncoder() {
	pimage = png_create_write_struct(PNG_LIBPNG_VER_STRING, &error, pngErrorCallback, pngWarningCallback);
	if (!assigned(pimage))
		throw app_error("TPNGDecoder::TPNGDecoder() : png_create_write_struct() failed.");
	pinfo = png_create_info_struct(pimage);
	if (!assigned(pinfo))
		throw app_error("TPNGDecoder::TPNGDecoder() : png_create_info_struct() failed.");
	created = assigned(pimage) && assigned(pinfo);
}

TPNGEncoder::~TPNGEncoder() {
	clear();
}

void TPNGEncoder::clear() {
	if (util::assigned(pimage) || util::assigned(pinfo)) {
		created = false;
		png_destroy_write_struct(&pimage, &pinfo);
		pimage = nil;
		pinfo = nil;
	}
}



TPNGData::TPNGData() {
	prime();
}

TPNGData::TPNGData(const size_t rowSize, const size_t imageHeigth) {
	prime();
	create(rowSize, imageHeigth);
}

TPNGData::~TPNGData() {
	destroy();
}

void TPNGData::prime() {
	sx = 0;
	height = 0;
	ptr = nil;
}

void TPNGData::create(const size_t rowSize, const size_t imageHeigth) {
	if (assigned(ptr))
		destroy();
	sx = rowSize;
	height = imageHeigth;
	ptr = new png_bytep[height];
	for (size_t y=0; y<height; y++) {
		ptr[y] = new png_byte[sx];
	}
}

void TPNGData::destroy() {
	if (assigned(ptr)) {
		for (size_t y=0; y<height; y++) {
			png_bytep p = ptr[y];
			if (assigned(p))
				delete[] p;
			ptr[y] = nil;
		}
		delete[] ptr;
	}
	prime();
}

bool TPNGData::validIndex(const size_t index) const {
	return (index >= 0 && index < height);
}

png_bytep TPNGData::operator [] (const std::size_t index) const {
	if (validIndex(index)) return ptr[index];
	else return nil;
}

void TPNGData::resize(const size_t rowSize, const size_t imageHeigth) {
	destroy();
	create(rowSize, imageHeigth);
}

void TPNGData::clear() {
	destroy();
}

size_t TPNGData::size() const {
	return height * sx;
}



TPNG::TPNG() {
}

TPNG::TPNG(const std::string& file) : TPictureFile(file) {
};


bool TPNG::isPNG(const png_bytep data, const png_size_t size) {
	if (size > MIN_PNG_SIZE) {
		return (EXIT_SUCCESS == png_sig_cmp(data, 0, MIN_PNG_SIZE));
	}
	return false;
}

bool TPNG::isPNG(const TRGB* data, const size_t size) {
	return isPNG((png_bytep)data, (png_size_t)size);
}

bool TPNG::isPNG(const TRGBData& data) {
	return isPNG(data.data(), data.size());
}


void TPNG::rowcopy(png_bytep dst, const TRGB* src, size_t n) {
	memcpy(reinterpret_cast<void*>(dst), reinterpret_cast<const void*>(src), n);
}




void pngDataReader(png_structp png_ptr, png_bytep data, png_size_t bytesToRead) {
	// Nothing to do?
	if (bytesToRead <= 0) {
		png_error(png_ptr, "TPNG::pngDataReader() No data to read.");
		return;
	}

	// Get given reader from TPNG::decode()
	TPNGDataHandler* reader = (TPNGDataHandler*)png_get_io_ptr(png_ptr);
	if (!util::assigned(reader)) {
		png_error(png_ptr, "TPNG::pngDataReader() No reader assigned for callback.");
		return;
	}

	// Check if reader is standard IO file
	if (util::assigned(reader->file)) {
		ssize_t r = reader->file->read(data, bytesToRead);
		if (r < (ssize_t)0) {
			png_error(png_ptr, "TPNG::pngDataReader() Reading data from file failed.");
		}
		reader->read += (size_t)bytesToRead;
		return;
	}

	// Reader is be plain memory...
	if (util::assigned(reader->data) && (reader->size > 0) && (reader->read <= reader->size)) {
		memcpy(data, reader->data, (size_t)bytesToRead);
		reader->data += (size_t)bytesToRead / sizeof(TRGB);
		reader->read += (size_t)bytesToRead;
		return;
	}
}


size_t TPNG::decode(const std::string& fileName, TRGBData& data, TRGBSize& width, TRGBSize& height) {
	ssize_t r;
	size_t msize;
	int px,py;
	TStdioFile file;
	TPNGDataHandler reader;
	TRGBData magic(MIN_PNG_SIZE+1);
	TPNGData rows;

	// Prepare buffer
	data.clear();
	height = 0;
	width = 0;

	// Check prerequisites!
	if (!util::fileExists(fileName))
		return (size_t)0;

	file.open(fileName, "rb");
	if (!file.isOpen())
		throw util::sys_error("TPNG::decode() : Open <" + fileName + "> failed.");

	// Read magic header bytes
	msize = magic.ordinal() * magic.size();
	r = file.read(magic(), msize);
	if (r != (ssize_t)msize) {
		throw util::app_error("TPNG::decode() : Can't read magic bytes on file <" + fileName + ">");
		return 0;
	}

	// Do magic number check on file header
	if (!isPNG(magic)) {
		throw util::app_error("TPNG::decode() : Magic number test for file <" + fileName + "> failed.");
		return 0;
	}

	// Seek back to beginning of file
	file.seek(0);

	TPNGDecoder decoder;
	if (!decoder.setLongjump()) {
	    std::cout << app::red << "TPNG::decode()::TPNGError() : \"" << decoder.getLastError() << "\"" << app::reset << std::endl;
	    std::cout << app::red << "TPNG::decode()::TPNGError() : Processed file <" << fileName << ">" << app::reset << std::endl;
	    std::cout << app::red << "TPNG::decode()::TPNGError() : Version " << app::magenta << PNG_LIBPNG_VER_STRING << app::reset << std::endl;

	    // Destructor should also do that...
	    decoder.clear();
		file.close();
		rows.clear();

		throw util::app_error("TPNG::decode()::TPNGError() : Error \"" + decoder.getLastError() + "\" occurred in file <" + fileName + ">");
		return 0;
	}

	// Set reader file callback
	reader.file = &file;

	// Decode from file by callback function
	decode(decoder, reader, data, rows, px, py);

	// Set correct dimension
	if (px > 0 && py > 0) {
		height = py;
		width = px;
	}

	return height*width;
}

size_t TPNG::decode(const char* buffer, const size_t size, TRGBData& data, TRGBSize& width, TRGBSize& height) {
	int px,py;
	TPNGDataHandler reader;
	TPNGData rows;

	// Prepare buffer
	data.clear();
	height = 0;
	width = 0;

	// Do magic number check on file header
	if (!isPNG((png_bytep)buffer, size)) {
		throw util::app_error("TPNG::decode() : Magic number test failed.");
		return 0;
	}

	TPNGDecoder decoder;
	if (!decoder.setLongjump()) {
	    std::cout << app::red << "TPNG::decode()::TPNGError() : \"" << decoder.getLastError() << "\"" << app::reset << std::endl;
	    std::cout << app::red << "TPNG::decode()::TPNGError() : Version " << app::magenta << PNG_LIBPNG_VER_STRING << app::reset << std::endl;

	    // Destructor should also do that...
	    decoder.clear();
		rows.clear();

		throw util::app_error("TPNG::decode()::TPNGError() : Error \"" + decoder.getLastError() + "\" occurred.");
		return 0;
	}

	// Set reader data callback
	reader.data = (TRGB*)buffer;
	reader.size = size;

	// Decode from data buffer by callback function
	decode(decoder, reader, data, rows, px, py);

	// Set correct dimension on valid picture size
	if (px > 0 && py > 0) {
		height = py;
		width = px;
	}

	return height*width;
}

size_t TPNG::decode(const char* buffer, const size_t size) {
	return decode(buffer, size, image, imageWidth, imageHeight);
}

size_t TPNG::decode(const TBuffer& buffer) {
	if (!buffer.empty()) {
		return decode(buffer.data(), buffer.size());
	}
	return (size_t)0;
}

size_t TPNG::decode(const TFile& file) {
	if (!file.empty() && file.isLoaded()) {
		return decode(file.getData(), file.getSize());
	}
	return (size_t)0;
}


size_t TPNG::decode(TPNGDecoder& decoder, TPNGDataHandler& reader, TRGBData& data, TPNGData& rows, int& px, int& py) {
	data.clear();

	png_structp png = decoder.image();
	png_infop info = decoder.info();

	// Set PNG reader callback
	png_set_read_fn(decoder.image(), &reader, pngDataReader);

	// Read info structure from image
	png_read_info(png, info);

	// Get image geometry
	px = png_get_image_width(png, info);
	py = png_get_image_height(png, info);

	// Check for valid PNG picture type
	png_byte bit_depth = png_get_bit_depth(png, info);
	if (bit_depth != STD_PNG_BIT_DEPTH)
		throw util::app_error("TPNG::decode() : Invalid bit depth (" + std::to_string((size_s)bit_depth) + ")");

	// Expand paletted colors into true RGBA quartets
	png_byte color_type = png_get_color_type(png, info);
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png);
		if (png_get_valid(png, info, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png);
		png_set_filler(png, 0, PNG_FILLER_AFTER);
	}

	// Set the background color to draw transparent
	bool ok = false;
	png_color_16 background;
	background.red   = 0xFF;
	background.green = 0xFF;
	background.blue  = 0xFF;
	if (png_get_valid(png, info, PNG_INFO_bKGD)) {
		png_color_16p pbg = &background;
		if (png_get_bKGD(png, info, &pbg)) {
			png_set_background(png, pbg,
					PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
			ok = true;
		}
	}
	if (!ok) {
		png_set_background(png, &background,
				PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
	}

	// Enable gamma conversion handling by libpng
	ok = false;
	double screen_gamma = 2.2;
	if (png_get_valid(png, info, PNG_INFO_gAMA)) {
		double info_gamma = 0.0;
		if (png_get_gAMA(png, info, &info_gamma)) {
			if (info_gamma > 0.0) {
				png_set_gamma(png, screen_gamma, info_gamma);
				ok = true;
			}
		}
	}
	if (!ok) {
		png_set_gamma(png, screen_gamma, 0.45);
	}

	// Strip 16 bit depth files down to 8 bits
	if (bit_depth == 16)
		png_set_strip_16(png);

	// Pack pixels data into bytes
	if (bit_depth < 8)
		png_set_packing(png);

	// Update reader state (optional)
	png_start_read_image(png);

	// Calculate row sizes
	size_t sx = png_get_rowbytes(png,info);
	size_t bpx = sx / px;

	// Check for valid pixel size
	if (bpx != BYTES_PER_PIXEL)
		throw util::app_error("TPNG::decode() : Invalid bytes per pixel (" + std::to_string((size_s)bpx) + ")");

	// Read complete image to row buffer
	rows.resize(sx, py);
	png_read_image(png, rows());

	// Transfer row data to RGB buffer
	png_bytep p;
	data.reserve(px*py*bpx+sx, false);
	for (int y=0; y<py; y++) {
		p = rows[y];
		data.append((TRGB*)p, sx);
	}

	// Read the rest of the file:
	// - read any additional chunks to info
	// - do CRC file check (executes "longjmp" on error!)
	png_read_end(png, info);

	// Return pixel count
	return px*py;
}


size_t TPNG::encode(const std::string& fileName, const TRGBData& data, const TRGBSize width, const TRGBSize height) {
	TStdioFile file;
	TPNGDataHandler writer;
	TPNGData rows;

	// Check prerequisites!
	if (fileName.empty() || data.size() <= 0 || width <= 0 || height <= 0)
		return 0;

	util::deleteFile(fileName);
	file.open(fileName, "wb");
	if (!file.isOpen())
		throw util::sys_error("TPNG::encode() : Open <" + fileName + "> failed.");

	TPNGEncoder encoder;
	if (!encoder.setLongjump()) {
	    std::cout << app::red << "TPNG::encode()::TPNGError() : \"" << encoder.getLastError() << "\"" << app::reset << std::endl;
	    std::cout << app::red << "TPNG::encode()::TPNGError() : Processed file <" << fileName << ">" << app::reset << std::endl;
	    std::cout << app::red << "TPNG::decode()::TPNGError() : Version " << app::magenta << PNG_LIBPNG_VER_STRING << app::reset << std::endl;

	    // Destructor should also do that...
	    encoder.clear();
		file.close();
		rows.clear();

		throw util::app_error("TPNG::encode()::TPNGError() : Error \"" + encoder.getLastError() + "\" occurred in file <" + fileName + ">");
		return 0;
	}

	// Set writer file callback
	writer.file = &file;

	// Encode to file by callback function
	return encode(encoder, writer, data, rows, width, height);
}

size_t TPNG::encode(char*& buffer, size_t& size, const TPicture& bitmap) {
	if (!bitmap.hasImage()) {
		return encode(buffer, size, bitmap.getImage(), bitmap.width(), bitmap.height());
	}
	return (size_t)0;
}

size_t TPNG::encode(char*& buffer, size_t& size) {
	return encode(buffer, size, image, imageWidth, imageHeight);
}

size_t TPNG::encode(char*& buffer, size_t& size, const TRGBData& data, const TRGBSize width, const TRGBSize height) {
	TPNGData rows;
	TPNGDataHandler writer;
	size_t sx = BYTES_PER_PIXEL * width * height; // Worst case: PNG compression is 1:1 ...

	// Check prerequisites!
	if (data.size() <= 0 || width <= 0 || height <= 0)
		return 0;

	// Delete buffer to prevent memory leak
	if (util::assigned(buffer))
		delete[] buffer;
	buffer = nil;
	size = 0;

	// Set writer memory callback
	writer.file = nil;
	writer.size = sx;
	writer.data = new TRGB[sx];
	buffer = (char*)writer.data;

	// Catch encoder longjmp
	TPNGEncoder encoder;
	if (!encoder.setLongjump()) {
	    std::cout << app::red << "TPNG::encode()::TPNGError() : \"" << encoder.getLastError() << "\"" << app::reset << std::endl;
	    std::cout << app::red << "TPNG::decode()::TPNGError() : Version " << app::magenta << PNG_LIBPNG_VER_STRING << app::reset << std::endl;

	    // Destructor should also do that...
	    encoder.clear();
		if (util::assigned(buffer))
			delete[] buffer;
		buffer = nil;
		size = 0;

		throw util::app_error("TPNG::encode()::TPNGError() : Error \"" + encoder.getLastError() + "\" occurred.");
		return 0;
	}

	// Encode to memory by callback function
	size = encode(encoder, writer, data, rows, width, height);
	return size;
}



void pngDataWriter(png_structp png_ptr, png_bytep data, png_size_t bytesToWrite) {
	// Nothing to do?
	if (bytesToWrite <= 0) {
		png_error(png_ptr, "TPNG::pngDataWriter() No data to write.");
		return;
	}

	// Get given writer from TPNG::encode()
	TPNGDataHandler* writer = (TPNGDataHandler*)png_get_io_ptr(png_ptr);
	if (!util::assigned(writer)) {
		png_error(png_ptr, "TPNG::pngDataWriter() No writer assigned for callback.");
		return;
	}

	// Check if reader is standard IO file
	if (util::assigned(writer->file)) {
		ssize_t r = writer->file->write(data, bytesToWrite);
		if (r != (ssize_t)bytesToWrite) {
			png_error(png_ptr, "TPNG::pngDataWriter() Writing data to file failed.");
		}
		writer->written += (size_t)bytesToWrite;
		return;
	}

	// Writer is be plain memory...
	if (util::assigned(writer->data) && (writer->size > 0) && ((writer->written + bytesToWrite) <= writer->size)) {
		memcpy(writer->data, data, (size_t)bytesToWrite);
		writer->data += (size_t)bytesToWrite / sizeof(TRGB);
		writer->written += (size_t)bytesToWrite;
		return;
	}
}

void pngDataFlusher(png_structp png_ptr) {
	// Get given writer from TPNG::encode()
	TPNGDataHandler* writer = (TPNGDataHandler*)png_get_io_ptr(png_ptr);
	if (!util::assigned(writer)) {
		png_error(png_ptr, "TPNG::pngDataFlusher() No writer assigned for callback.");
		return;
	}

	// Check if reader is standard IO file
	if (util::assigned(writer->file)) {
		if (!writer->file->flush()) {
			png_error(png_ptr, "TPNG::pngDataFlusher() Flushing file failed.");
		}
	}
}



size_t TPNG::encode(TPNGEncoder& encoder, TPNGDataHandler& writer, const TRGBData& data, TPNGData& rows, const int px, const int py) {
	png_structp png = encoder.image();
	png_infop info = encoder.info();
	png_uint_32 width = px;
	png_uint_32 height = py;

	// Create row buffer
	size_t bpa = BYTES_PER_PIXEL + 1;
	size_t sx = width * bpa;
	rows.resize(sx, height);

	// Copy raw RGB buffer to PNG row buffer
	// Convert in 3 byte RGB to 4 byte ARGB
	png_bytep r;
	size_t ys, os, od;
	const TRGB* p = data();
	for (size_t y=0; y<height; y++)
	{
		r = rows[y];
		ys = y * width;
		for (size_t x=0; x<width; x++)
		{
			os = (x + ys) * BYTES_PER_PIXEL;
			od = x * bpa;

			*(r + od++) = 0;			// A (Empty Alpha channel)
			memcpy(r + od, p + os, 3);	// RGB
		}
	}

	// Assign PNG writer callback
	png_set_write_fn(encoder.image(), &writer, pngDataWriter, pngDataFlusher);

	// Set IHDR information chunk
	png_set_IHDR(png, info, width, height,
			STD_PNG_BIT_DEPTH, STD_PNG_COLOR_TYPE, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	// Set Gamma default value
	double gamma = 1.0;
	png_set_gAMA(png, info, gamma);

	// Set the background color to draw transparent
	png_color_16 background;
	background.red   = 0xFF;
	background.green = 0xFF;
	background.blue  = 0xFF;
	png_set_background(png, &background,
			PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);

	// No grayscale
	png_color_8 sig_bit;
	sig_bit.gray = 0;

	// Set color bit depth to 8 bits
	sig_bit.red   = 8;
	sig_bit.green = 8;
	sig_bit.blue  = 8;
	sig_bit.alpha = 8;

	// Set significant channel bits
	png_set_sBIT(png, info, &sig_bit);

	// Write info header to file
	png_write_info(png, info);

	// Pack pixels into bytes
	png_set_packing(png);

	// No filler bytes, pack RGB into 3 bytes.
	png_set_filler(png, 0, PNG_FILLER_BEFORE);

	// Write row buffer to file
	png_write_image(png, rows());
	png_write_end(png, info);
	png_write_flush(png);

	return writer.written;
}


void TPNG::saveAsBitmap() {
	if (validFile())
		saveAsBitmap(util::fileReplaceExt(getFile(), "bmp"));
}

void TPNG::saveAsBitmap(const std::string& fileName) {
	TBitmap dib;
	dib.encode(fileName, image, imageWidth, imageHeight);
}

void TPNG::loadFromBitmap(const std::string& fileName) {
	TBitmap dib;
	dib.decode(fileName, image, imageWidth, imageHeight);
}


} /* namespace util */
