/*
 * ASCII.cpp
 *
 *  Created on: 19.01.2015
 *      Author: Dirk Brinkmeier
 */
#include <algorithm>
#include "ASCII.h"
#include "codepages.h"
#include "asciitables.h"
#include "stringutils.h"
#include "templates.h"
#include "functors.h"
#include "memory.h"

using namespace util;

/******************************************************************************
Aufgabe        : Convert char/wchar to ASCII
Beschreibung   :
Autor          : Dipl.-Ing. Dirk Brinkmeier
entwickelt am  : 05.07.1999 Delphi 4.0
geändert am    : 18.01.2015 C++ (Linux GCC)
*******************************************************************************/
std::string TBinaryConvert::binToAscii(const char value) {
	int p = static_cast<int>(value);
	if (p < 0) {
		std::string s = "<XX>";
		unsigned char c = (unsigned char)p;
		s[1] = nibbleLookupTableUA[(c >> 4) & 0x0F];
		s[2] = nibbleLookupTableUA[c & 0x0F];
		return s;
	}
	if ((p >= 0) && (p < sizeASCIITableA))
		return ASCIITableA[p];
	return std::string(&value, 1);
}

std::wstring TBinaryConvert::binToAscii(const wchar_t value) {
	int p = static_cast<int>(value);
	if ((p >= 0) && (p < sizeASCIITableW))
		return ASCIITableW[p];
	return std::wstring(&value, 1);
}


std::string TBinaryConvert::strToAscii(const std::string& value) {
	std::string s;
	for (size_t i=0; i<value.size(); i++)
		s += binToAscii(value[i]);
	return s;
}

std::wstring TBinaryConvert::strToAscii(const std::wstring& value) {
	std::wstring s;
	for (size_t i=0; i<value.size(); i++)
		s += binToAscii(value[i]);
	return s;
}


std::string TBinaryConvert::binToHexA(const void *const buffer, size_t size, const bool upper, const char separator) {
	if (!util::assigned(buffer) || size <= 0)
		return std::string();

	char c;
	int nNibble;
	size_t nLength = size * 3;
	std::string s;
	s.reserve(nLength);
    unsigned char const *p = (unsigned char const *)buffer;
    size_t last = util::pred(size);

	for (size_t i=0; i<size; i++) {
		s.append("0x");

		nNibble = (p[i] >> 4) & 0x0F;
		c = upper ? nibbleLookupTableUA[nNibble] : nibbleLookupTableLA[nNibble];
		s.push_back(c);

		nNibble = p[i] & 0x0F;
		c = upper ? nibbleLookupTableUA[nNibble] : nibbleLookupTableLA[nNibble];
		s.push_back(c);

		if (i < last)
			s.push_back(separator);
	}

	return s;
}


std::wstring TBinaryConvert::binToHexW(const void *const buffer, size_t size, const bool upper, const wchar_t separator) {
	if (!util::assigned(buffer) || size <= 0)
		return std::wstring();

    wchar_t w;
	int nNibble;
	size_t nLength = size * 3;
	std::wstring s;
	s.reserve(nLength);
    unsigned char const *p = (unsigned char const *)buffer;
    size_t last = util::pred(size);

	for (size_t i=0; i<size; i++) {
		s.append(L"0x");

		nNibble = (p[i] >> 4) & 0x0F;
		w = upper ? nibbleLookupTableUW[nNibble] : nibbleLookupTableLW[nNibble];
		s.push_back(w);

		nNibble = p[i] & 0x0F;
		w = upper ? nibbleLookupTableUW[nNibble] : nibbleLookupTableLW[nNibble];
		s.push_back(w);

		if (i < last)
			s.push_back(separator);
	}

	return s;
}


std::string TBinaryConvert::binToText(const void *const buffer, size_t size, const EBinaryTextMode mode, const bool upper) {
	if (!util::assigned(buffer) || size <= 0)
		return std::string();

    char c;
    bool space;
	int nNibble;
	size_t nLength = size * 4;
	std::string s;
	s.reserve(nLength);
    unsigned char const *p = (unsigned char const *)buffer;

	for (size_t i=0; i<size; i++) {
		unsigned char v = p[i];
		space = (mode == EBT_TEXT) ? v < USPC : v <= USPC;
		if (space || v > 0x7F) {
			s.push_back('<');

			nNibble = (v >> 4) & 0x0F;
			c = upper ? nibbleLookupTableUA[nNibble] : nibbleLookupTableLA[nNibble];
			s.push_back(c);

			nNibble = v & 0x0F;
			c = upper ? nibbleLookupTableUA[nNibble] : nibbleLookupTableLA[nNibble];
			s.push_back(c);

			s.push_back('>');
		} else {
			s.push_back(v);
		}
	}

	return s;
}


std::string TBinaryConvert::binToAsciiA(const void *const buffer, size_t size, const bool upper) {
	if (!util::assigned(buffer) || size <= 0)
		return std::string();

	size_t nLength = size * 3;
	std::string s;
	s.reserve(nLength);
    char const *p = (char const *)buffer;

	for (size_t i=0; i<size; ++i, ++p)
		s += binToAscii(*p);

	return s;
}

std::wstring TBinaryConvert::binToAsciiW(const void *const buffer, size_t size, const bool upper) {
	if (!util::assigned(buffer) || size <= 0)
		return std::wstring();

	return TStringConvert::MultiByteToWideString(binToAsciiA(buffer, size, upper));
}

std::string TBinaryConvert::binToHex(const void *const buffer, size_t size, const bool upper) {
	if (!util::assigned(buffer) || size <= 0)
		return std::string();

	char c;
	int nNibble;
	size_t nLength = size * 3;
	std::string s;
	s.reserve(nLength);
    unsigned char const *p = (unsigned char const *)buffer;

	for (size_t i=0; i<size; i++) {
		nNibble = (p[i] >> 4) & 0x0F;
		c = upper ? nibbleLookupTableUA[nNibble] : nibbleLookupTableLA[nNibble];
		s.push_back(c);

		nNibble = p[i] & 0x0F;
		c = upper ? nibbleLookupTableUA[nNibble] : nibbleLookupTableLA[nNibble];
		s.push_back(c);
	}

	return s;
}

bool TBinaryConvert::hexToBin(const std::string& hex, void *const buffer, size_t size) {
	if (!util::assigned(buffer) || size <= 0)
		return false;

	// Check prerequisites
	if (0 != (hex.size() % 2))
		return false;
	if (size < (hex.size() / 2))
		return false;

	// Convert hex buffer to binary
	char* w;
	unsigned char* p = (unsigned char *)buffer;
	for (size_t i=0, j=0; i<hex.size(); i+=2, ++j) {
		if (j >= size) {
			// Buffer too small!
			return false;
		}
		const char* v = hex.substr(i, 2).c_str();
		errno = EXIT_SUCCESS;
		long int l = strtol(v, &w, 16);
		if (EXIT_SUCCESS != errno || ((v + 2) != w) || l < 0 || l > 255) {
			return false;
		}
		*(p++) = (unsigned char)l;
	}

	// All went OK...
	return true;
}


std::string TUUID::convertBinaryToUUID(const char* buffer, size_t size, bool upper) {
	if (size >= 16) {
		// 16 byte UUID "6418F001-0E15-F66A-718F-C8600089F30B"
		unsigned char const *p = (unsigned char const *)buffer;
		char retVal[40]; // 32 Byte + 4 times '-' + reserve
		char *q = retVal;
		int off = upper ? 55 : 87;
		int hi, lo, i;

		/* Data 1 - 8 characters.*/
		for(i = 0; i < 4; i++) {
			hi = (*p >> 4) & 0x0F;
			((*q = (hi % 16)) < 10) ? *q += 48 : *q += off;
			q++;
			lo = *p++ & 0x0F;
			((*q = (lo % 16)) < 10) ? *q += 48 : *q += off;
			q++;
		}

		/* Data 2 - 4 characters.*/
		*q++ = '-';
		for(i = 0; i < 2; i++) {
			hi = (*p >> 4) & 0x0F;
			((*q = (hi % 16)) < 10) ? *q += 48 : *q += off;
			q++;
			lo = *p++ & 0x0F;
			((*q = (lo % 16)) < 10) ? *q += 48 : *q += off;
			q++;
		}

		/* Data 3 - 4 characters.*/
		*q++ = '-';
		for(i = 0; i < 2; i++) {
			hi = (*p >> 4) & 0x0F;
			((*q = (hi % 16)) < 10) ? *q += 48 : *q += off;
			q++;
			lo = *p++ & 0x0F;
			((*q = (lo % 16)) < 10) ? *q += 48 : *q += off;
			q++;
		}

		/* Data 4 - 4 characters.*/
		*q++ = '-';
		for(i = 0; i < 2; i++) {
			hi = (*p >> 4) & 0x0F;
			((*q = (hi % 16)) < 10) ? *q += 48 : *q += off;
			q++;
			lo = *p++ & 0x0F;
			((*q = (lo % 16)) < 10) ? *q += 48 : *q += off;
			q++;
		}

		/* Data 5 - 12 characters.*/
		*q++ = '-';
		for(i = 0; i < 6; i++) {
			hi = (*p >> 4) & 0x0F;
			((*q = (hi % 16)) < 10) ? *q += 48 : *q += off;
			q++;
			lo = *p++ & 0x0F;
			((*q = (lo % 16)) < 10) ? *q += 48 : *q += off;
			q++;
		}
		*q = '\0';

		return std::string(retVal, 36);
	}
	return std::string();
}

std::string TUUID::convertBinaryToMAC(const char* buffer, size_t size, bool upper) {
	// 6 Byte MAC address "XX:XX:XX:XX:XX:XX"
	// 8 Byte MAC address "XX:XX:XX:XX:XX:XX:XX:XX"
	unsigned char const *p = (unsigned char const *)buffer;
	size_t len = 3 * size;
	char retVal[len + 2];
	char *q = retVal;
	int n, off = upper ? 55 : 87;

	/* Data 1 - size characters.*/
	for(size_t i = 0; i < size; i++) {
		n = (*p >> 4) & 0x0F;
		((*q = (n % 16)) < 10) ? *q += 48 : *q += off;
		q++;
		n = *p++ & 0x0F;
		((*q = (n % 16)) < 10) ? *q += 48 : *q += off;
		q++;
		*q++ = ':';
	}

	return std::string(retVal, len);
}


int convertOemToAnsi(char c) {
	unsigned char u = static_cast<unsigned char>(c);
	if (u > 0x7Fu)
		return (int)OemToAnsiTable[u];
	return (int)u;
}

void TOEM::OemToAnsi(std::string& value) {
	std::transform(value.begin(), value.end(), value.begin(), convertOemToAnsi);
}

// Convert Windows-1252 to Ansi Single Byte String
void TOEM::OemToAnsi(void *const buffer, size_t size) {
	unsigned char* p = (unsigned char*)buffer;
	for (size_t i=0; i<size; ++i) {
		if (*p > 0x7Fu)
			*p = OemToAnsiTable[*p];
		++p;
	}
}



// Convert Windows-1252 to UTF-8 Multi Byte String
std::string TASCII::OemToMultiByteStr(const std::string& value) {
	return OemToMultiByteStr(value.c_str(), value.size());
}

std::string TASCII::OemToMultiByteStr(const void *const buffer, size_t size) {
	if (size > 0) {
		TBuffer p(size*2+1);
		const char* in = (const char*)buffer;
		char* out = p.data();
		size_t length = 0;
		unsigned char u, c;

		for (size_t i=0; i<size; i++) {
			u = static_cast<unsigned char>(*in++);
			c = OemToAnsiTable[u];
			if (0 == isControlCharA(c) /* c > 0x20 */) {
				if (c < 0x80) {
					*out++ = c;
					++length;
				} else {
					*out++ = 0xC0 | ((c >> 6) & 0x1F); /* 2+1+5 bits */
					*out++ = 0x80 | (c & 0x3F);        /* 1+1+6 bits */
					length += 2;
				}
			}
		}

		if (length)
			return std::string(p.data(), length);
	}
	return std::string();
}

// Convert ISO-8859-1 to UTF-8 Multi Byte String
std::string TASCII::AnsiToMultiByteStr(const std::string& value) {
	return AnsiToMultiByteStr(value.c_str(), value.size());
}

std::string TASCII::AnsiToMultiByteStr(const void *const buffer, size_t size) {
	if (size > 0) {
		TBuffer p(2*size+1);
		const char* in = (const char*)buffer;
		char* out = p.data();
		size_t length = 0;
		unsigned char c;

		for (size_t i=0; i<size; i++) {
			c = static_cast<unsigned char>(*in++);
			if (0 == isControlCharA(c)) {
				if (c < 0x80) {
					*out++ = c;
					++length;
				} else {
					*out++ = 0xC0 | ((c >> 6) & 0x1F); /* 2+1+5 bits */
					*out++ = 0x80 | (c & 0x3F);        /* 1+1+6 bits */
					length += 2;
				}
			}
		}

		if (length)
			return std::string(p.data(), length);
	}
	return std::string();
}


// Convert CP1252 to UTF-8 Multi Byte String
std::string TASCII::CP1252ToMultiByteStr(const std::string& value) {
	return CP1252ToMultiByteStr(value.c_str(), value.size());
}

std::string TASCII::CP1252ToMultiByteStr(const void *const buffer, size_t size) {
	return convertToMultiByteStr(buffer, size, CP1252ToUTF8Table, CP1252ToUTF8Offsets);
}


// Convert CP437 to UTF-8 Multi Byte String
std::string TASCII::CP437ToMultiByteStr(const std::string& value) {
	return CP437ToMultiByteStr(value.c_str(), value.size());
}

std::string TASCII::CP437ToMultiByteStr(const void *const buffer, size_t size) {
	return convertToMultiByteStr(buffer, size, CP437ToUTF8Table, CP437ToUTF8Offsets);
}


// Convert CP870 to UTF-8 Multi Byte String
std::string TASCII::CP850ToMultiByteStr(const std::string& value) {
	return CP850ToMultiByteStr(value.c_str(), value.size());
}

std::string TASCII::CP850ToMultiByteStr(const void *const buffer, size_t size) {
	return convertToMultiByteStr(buffer, size, CP850ToUTF8Table, CP850ToUTF8Offsets);
}


std::string TASCII::convertToMultiByteStr(const void *const buffer, size_t size, const unsigned char* codepage, const size_t* offsets) {
	if (size > 0) {
		TBuffer p(3*size+1);
		const char* in = (const char*)buffer;
		char* out = p.data();
		size_t length = 0;
		unsigned char c;

		for (size_t i=0; i<size; i++) {
			c = static_cast<unsigned char>(*in++);
			if (0 == isControlCharA(c)) {
				if (c < 0x80) {
					*out++ = c;
					++length;
				} else {
					// Codepage dependant multibyte replacement
					size_t k1 = offsets[c];
					size_t k2 = offsets[c + 1];
					while (k1 < k2) {
						*out++ = codepage[k1++];
						++length;
					}
				}
			}
		}

		if (length)
			return std::string(p.data(), length);
	}
	return std::string();
}


void TASCII::showMultiByteCodepage(const wchar_t* codepage, const size_t size) {
	char buffer[MB_CUR_MAX];
	size_t offsets[size+1];
	size_t offs = 0;
	size_t i, last = pred(size);
	for (i=0; i<size; ++i) {
		wchar_t w = codepage[i];
		int length = ::wctomb(buffer, w);
		std::cout << "/* " << util::cprintf("0x%02X", i) << " @ " << offs << " */ ";
		if (length > 0)
			std::cout << TBinaryConvert::binToHexA(buffer, length, true, ',');
		if (i < last)
			std::cout << "," << std::endl;
		else
			std::cout << std::endl;
		offsets[i] = offs;
		offs += length;
	}
	std::cout << std::endl;
	last = size;
	offsets[i] = offs;
	for (i=0; i<=size; ++i) {
		if (i < last)
			std::cout << offsets[i] << ", ";
		else
			std::cout << offsets[i];
		if (0 == (succ(i) % 8))
			std::cout << std::endl;
	}
	std::cout << std::endl << std::endl;
}

void TASCII::showCP437MultiByteCodepage() {
	showMultiByteCodepage((const wchar_t*)CP437ToUTF8TableW, util::sizeOfArray(CP437ToUTF8TableW));
}

void TASCII::showCP850MultiByteCodepage() {
	showMultiByteCodepage((const wchar_t*)CP850ToUTF8TableW, util::sizeOfArray(CP850ToUTF8TableW));
}

void TASCII::showCP1252MultiByteCodepage() {
	showMultiByteCodepage((const wchar_t*)CP1252ToUTF8TableW, util::sizeOfArray(CP1252ToUTF8TableW));
}

// Convert UTF-8 Multi Byte byte character buffer to UTF-8 String
// and remove control characters
std::string TASCII::UTF8ToMultiByteStr(const std::string& value) {
	return UTF8ToMultiByteStr(value.c_str(), value.size());
}

//
// See https://de.wikipedia.org/wiki/UTF-8
//
// 00000000–01111111 	00–7F 	0–127 		Ein-Byte lange Zeichen, deckungsgleich mit US-ASCII.
// 10000000–10111111 	80–BF 	128–191 	Zweites, drittes oder viertes Byte einer Bytesequenz.
// 11000000–11000001 	C0–C1 	192–193 	Start einer 2 Byte langen Sequenz, welche den Codebereich aus 0 bis 127 abbildet, unzulässig
// 11000010–11011111 	C2–DF 	194–223 	Start einer 2 Byte langen Sequenz (U+0080 … U+07FF)
// 11100000–11101111 	E0–EF 	224–239 	Start einer 3 Byte langen Sequenz (U+0800 … U+FFFF)
// 11110000–11110100 	F0–F4 	240–244 	Start einer 4 Byte langen Sequenz (Inklusive der ungültigen Codebereiche von 110000 bis 13FFFF)
// 11110101–11110111 	F5–F7 	245–247 	Ungültig nach RFC 3629: Start einer 4 Byte langen Sequenz für Codebereich über 140000
// 11111000–11111011 	F8–FB 	248–251 	Ungültig nach RFC 3629: Start einer 5 Byte langen Sequenz
// 11111100–11111101 	FC–FD 	252–253 	Ungültig nach RFC 3629: Start einer 6 Byte langen Sequenz
// 11111110–11111111 	FE–FF 	254–255 	Ungültig. In der ursprünglichen UTF-8-Spezifikation nicht definiert.
//
bool TASCII::isValidUTF8MultiByteStr(const void *const buffer, size_t size) {
	if (size > 0) {
		int bytes = 0, state = 0;
		unsigned char c;
		const unsigned char* p = (const unsigned char*)buffer;
		for (size_t i=0; i<size; ++i) {
			c = *p;
			switch (state) {
				case 0:
					// Test for valid start byte
					if (c >= 0x80) {
						switch (c & 0xF0) {
							case 0xF0:
								// Start of 4 Byte sequence: 11110000
								bytes = 4;
								break;
							case 0xE0:
								// Start of 3 Byte sequence: 11100000
								bytes = 3;
								break;
							case 0xC0:
								// Start of 2 Byte sequence: 11000000
								bytes = 2;
								break;
						}
						// Check for valid start byte
						if (bytes > 0)
							state = 10;
						else
							return false;
					}
					break;
				case 10:
					if (c < 0x80)
						return false;
					// Valid second, third or fourth byte...
					if (bytes <= 1)
						state = 0;
					break;
				default:
					break;
			}
			++p;
			if (bytes > 0)
				--bytes;
		}
		return true;
	}
	return false;
}


std::string TASCII::UTF8ToMultiByteStr(const void *const buffer, size_t size) {
	if (size > 0) {
		size_t bytes = 0, index = 0, state = 0;
		unsigned char c;
		unsigned char utf8[4];
		TBuffer p(3*size+1);
		const char* in = (const char*)buffer;
		char* out = p.data();
		size_t length = 0;

		for (size_t i=0; i<size; ++i) {
			c = static_cast<unsigned char>(*in);
			switch (state) {
				case 0:
					// Ignore any control chars apart from LF
					if (0 == isControlCharA(c)) {

						// Test for valid start byte
						if (c >= 0x80) {

							// Check for valid start byte
							switch (c & 0xF0) {
								case 0xF0:
									// Start of 4 Byte sequence: 11110000
									bytes = 4;
									break;
								case 0xE0:
									// Start of 3 Byte sequence: 11100000
									bytes = 3;
									break;
								case 0xC0:
									// Start of 2 Byte sequence: 11000000
									bytes = 2;
									break;
							}

							if (bytes > 0) {
								// Start byte found
								state = 10;
								utf8[0] = c;
								index = 1;
							} else {
								// U+FFFD REPLACEMENT CHARACTER
								// UTF-8 	EF BF BD
								// UTF-16 	FF FD
								*out++ = 0xEF;
								*out++ = 0xBF;
								*out++ = 0xBD;
								length += 3;
							}

						} else {
							// Single ASCII char found
							*out++ = c;
							++length;
						}
					}
					break;

				case 10:
					if (index > 0 && index < 4) {
						if (c >= 0x80) {
							// Valid second, third or fourth byte...
							utf8[index] = c;
						} else {
							// U+FFFD REPLACEMENT CHARACTER
							// UTF-8 	EF BF BD
							// UTF-16 	FF FD
							if (index > 1) {
								for (size_t i=0; i<index; ++i) {
									*out++ = 0xEF;
									*out++ = 0xBF;
									*out++ = 0xBD;
									length += 3;
								}
							} else {
								*out++ = 0xEF;
								*out++ = 0xBF;
								*out++ = 0xBD;
								length += 3;
							}

							// Add current ASCII character
							*out++ = c;
							++length;

							// Skip further conversions
							index = 0;
							bytes = 0;
							state = 0;
						}
					}
					if (bytes <= 1) {
						state = 0;
						if (index > 0 && index < 4) {
							for (size_t i=0; i<=index; ++i) {
								*out++ = utf8[i];
							}
							length += succ(index);
						}
					}
					if (index > 0)
						++index;
					break;

				default:
					break;
			}

			// Walk through input buffer and UTF codepoints
			++in;
			if (bytes > 0)
				--bytes;

			// Check for sufficient buffer storage
			if (length > (p.size() / 3 * 2))
				p.resize(p.size() + length, true);

		}

		trim(p.data(), length);
		if (length > 0) {
			return std::string(p.data(), length);
		}

	}
	return std::string();
}


/*
 * Code taken and modified from the following source:
 * copyright 2006-2013 by the mpg123 project - free software under the terms of the LGPL 2.1
 * see COPYING and AUTHORS files in distribution or http://mpg123.org
 * initially written by Thomas Orgis
 */
EBOMType TASCII::hasBOM(const unsigned char* buffer, size_t size) {
	if(size < 2)
		return BOM_NONE;

	/* Little endian */
	if(buffer[0] == 0xff && buffer[1] == 0xfe)
		return BOM_LE;

	/* Big endian */
	if(buffer[0] == 0xfe && buffer[1] == 0xff)
		return BOM_BE;

	return BOM_NONE;
}

bool TASCII::isBOM(const void* buffer, size_t size) {
	const unsigned char* p = (unsigned char*)buffer;
	return BOM_NONE != hasBOM(p, size);
}

EBOMType TASCII::removeBOM(const unsigned char** buffer, size_t& size) {
	if(size < 2)
		return BOM_NONE;

	EBOMType next = BOM_NONE;
	EBOMType bom = hasBOM(*buffer, size);
	if(BOM_NONE == bom)
		return BOM_NONE;

	/* Skip the detected BOM. */
	*buffer += 2;
	size -= 2;

	/* Check for following BOMs. The last one wins! */
	next = removeBOM(buffer, size);
	if (next == BOM_NONE)
		return bom; /* End of the recursion. */
	else
		return next;
}

void TASCII::trim(const char* buffer, size_t& size) {
	while (buffer[size - 1] == '\0' && size > 0) {
		--size;
	}
}

/* Remember: There's a limit at 0x1ffff. */
#define FULLPOINT(f,s) ( (((f)&0x3FF)<<10) + ((s)&0x3FF) + 0x10000 )
#define UTF8LEN(x) ( (x)<0x80 ? 1 : ((x)<0x800 ? 2 : ((x)<0x10000 ? 3 : 4)))

std::string TASCII::UTF16ToMultiByteStr(const std::string& value) {
	return UTF16ToMultiByteStr(value.c_str(), value.size());
}

std::string TASCII::UTF16ToMultiByteStr(const void *const buffer, size_t size) {
	size_t i;
	size_t n; /* number bytes that make up full pairs */
	const unsigned char *s = (const unsigned char *)buffer;
	unsigned char *p;
	size_t length = 0; /* the resulting UTF-8 length */
	/* Determine real length... extreme case can be more than utf-16 length. */
	size_t high = 0; /* BE: The first byte is the high byte. */
	size_t low  = 1; /* BE: The second byte is the low byte. */
	EBOMType bom;

	bom = removeBOM(&s, size);
	if(bom == BOM_LE) { /* little-endian */
		high = 1; /* LE: The second byte is the high byte. */
		low  = 0; /* LE: The first byte is the low byte. */
	}

	if (size < 2)
		return "";

	n = (size / 2) * 2; /* number bytes that make up full pairs */

	/* first: get length, check for errors -- stop at first one */
	for(i=0; i<n; i+=2) {
		unsigned long point = ((unsigned long) s[i+high]<<8) + s[i+low];
		if((point & 0xFC00) == 0xD800) { /* lead surrogate */
			unsigned short second = (i+3 < size) ? (s[i+2+high]<<8) + s[i+2+low] : 0;
			if((second & 0xFC00) == 0xDC00) { /* good... */
				point = FULLPOINT(point,second);
				length += UTF8LEN(point); /* possibly 4 bytes */
				i+=2; /* We overstepped one word. */
			} else { /* if no valid pair, break here */
				// if(noquiet) error2("Invalid UTF16 surrogate pair at %li (0x%04lx).", (unsigned long)i, point);
				n = i; /* Forget the half pair, END! */
				break;
			}
		} else {
			length += UTF8LEN(point); /* 1,2 or 3 bytes */
		}
	}

	// Set data buffer to calculated length
	TDataBuffer<char> sb(length + 1);

	/* Now really convert, skip checks as these have been done just before. */
	p = (unsigned char*) sb.data(); /* Signedness doesn't matter but it shows I thought about the non-issue */
	length = 0;
	for(i=0; i < n; i+=2) {
		unsigned long codepoint = ((unsigned long) s[i+high]<<8) + s[i+low];
		if((codepoint & 0xFC00) == 0xD800) { /* lead surrogate */
			unsigned short second = (s[i+2+high]<<8) + s[i+2+low];
			codepoint = FULLPOINT(codepoint,second);
			i+=2; /* We overstepped one word. */
		}
		if (codepoint >= USPC && codepoint != ULF) {
			if (codepoint < 0x80) {
				*p++ = (unsigned char) codepoint;
				++length;
			} else if(codepoint < 0x800) {
				*p++ = (unsigned char) (0xC0 | (codepoint >> 6));
				*p++ = (unsigned char) (0x80 | (codepoint & 0x3F));
				length += 2;
			} else if(codepoint < 0x10000) {
				*p++ = (unsigned char) (0xE0 | (codepoint >> 12));
				*p++ = 0x80 | ((codepoint >> 6) & 0x3F);
				*p++ = 0x80 | (codepoint & 0x3F);
				length += 3;
			} else if (codepoint < 0x200000) {
				*p++ = (unsigned char) (0xF0 | codepoint >> 18);
				*p++ = (unsigned char) (0x80 | ((codepoint >> 12) & 0x3F));
				*p++ = (unsigned char) (0x80 | ((codepoint >> 6) & 0x3F));
				*p++ = (unsigned char) (0x80 | (codepoint & 0x3F));
				length += 4;
			} /* ignore bigger ones (that are not possible here anyway) */
		}
	}

	// Return trimmed string length
	trim(sb.data(), length);
	return (length > 0) ? std::string(sb.data(), length) : "";
}

#undef UTF8LEN
#undef FULLPOINT


std::wstring TStringConvert::MultiByteToWideString(const std::string& str) {
	std::wstring ws;
	if (!str.empty()) {
		ws.reserve(2 * str.size());
		::mbtowc(NULL, 0, 0); // Reset the conversion state
		const char* it = str.data();
		const char* end = it + str.size();
		int r;
		for (wchar_t wc; (r = ::mbtowc(&wc, it, end-it)) > 0; it += r) {
			ws.append(&wc, 1);
		}
	}
	return ws;
}

std::wstring TStringConvert::MultiByteToWideString(const char* s, const size_t size) {
	std::wstring ws;
	if (util::assigned(s) && size > 0) {
		ws.reserve(2 * size);
		::mbtowc(NULL, 0, 0); // Reset the conversion state
		const char* it = s;
		const char* end = it + size;
		int r;
		for (wchar_t wc; (r = ::mbtowc(&wc, it, end-it)) > 0; it += r) {
			ws.append(&wc, 1);
		}
	}
	return ws;
}

std::string TStringConvert::WideToMultiByteString(const std::wstring& str) {
	std::string s;
	if (!str.empty()) {
		s.reserve(2 * str.size());
		const wchar_t* it = str.data();
		char buffer[MB_CUR_MAX];
		int i,length;
		char c;
		while (*it) {
			length = ::wctomb(buffer,*it);
			if (length<1) break;
			for (i=0; i<length; ++i) {
				c = buffer[i];
				s.append(&c, 1);
			}
			++it;
		}
	}
	return s;
}

std::string TStringConvert::WideToMultiByteString(const wchar_t* ws, const size_t size) {
	std::string s;
	if (util::assigned(ws) && size > 0) {
		s.reserve(2 * size);
		const wchar_t* it = ws;
		char buffer[MB_CUR_MAX];
		int i,length;
		char c;
		while (*it) {
			length = ::wctomb(buffer,*it);
			if (length<1) break;
			for (i=0; i<length; ++i) {
				c = buffer[i];
				s.append(&c, 1);
			}
			++it;
		}
	}
	return s;
}

std::string TStringConvert::SingleByteToMultiByteString(const std::string& str, const app::ECodepage codepage) {
	if (!str.empty()) {
		switch (codepage) {
			case app::ECodepage::OEM:
				return util::TASCII::OemToMultiByteStr(str);
				break;
			case app::ECodepage::ANSI:
				return util::TASCII::AnsiToMultiByteStr(str);
				break;
			case app::ECodepage::CP_1252:
				return util::TASCII::CP1252ToMultiByteStr(str);
				break;
			case app::ECodepage::CP_437:
				return util::TASCII::CP437ToMultiByteStr(str);
				break;
			case app::ECodepage::CP_850:
				return util::TASCII::CP850ToMultiByteStr(str);
				break;
			case app::ECodepage::UTF_16:
				return util::TASCII::UTF16ToMultiByteStr(str);
				break;
			case app::ECodepage::UTF_8:
				return util::TASCII::UTF8ToMultiByteStr(str);
				break;
			default:
				break;
		}
	}
	return str;
}

std::wstring TStringConvert::SingleByteToWideString(const std::string& str, const app::ECodepage codepage) {
	return MultiByteToWideString(SingleByteToMultiByteString(str, codepage));
}

std::string TStringConvert::SingleByteToMultiByteString(const void *const buffer, size_t size, const app::ECodepage codepage) {
	if (size > 0) {
		switch (codepage) {
			case app::ECodepage::ANSI:
				return util::TASCII::AnsiToMultiByteStr(buffer, size);
				break;
			case app::ECodepage::OEM:
				return util::TASCII::OemToMultiByteStr(buffer, size);
				break;
			case app::ECodepage::CP_1252:
				return util::TASCII::CP1252ToMultiByteStr(buffer, size);
				break;
			case app::ECodepage::CP_437:
				return util::TASCII::CP437ToMultiByteStr(buffer, size);
				break;
			case app::ECodepage::CP_850:
				return util::TASCII::CP850ToMultiByteStr(buffer, size);
				break;
			case app::ECodepage::UTF_16:
				return util::TASCII::UTF16ToMultiByteStr(buffer, size);
				break;
			case app::ECodepage::UTF_8:
				return util::TASCII::UTF8ToMultiByteStr(buffer, size);
				break;
			default:
				break;
		}
	}
	return std::string(static_cast<const char *const>(buffer), size);
}

std::wstring TStringConvert::SingleByteToWideString(const void *const buffer, size_t size, const app::ECodepage codepage) {
	return MultiByteToWideString(SingleByteToMultiByteString(buffer, size, codepage));
}
