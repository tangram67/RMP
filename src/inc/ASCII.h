/*
 * ASCII.h
 *
 *  Created on: 18.01.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef ASCII_H_
#define ASCII_H_

#include <string>
#include <cwchar>
#include "charconsts.h"
#include "codesets.h"

namespace util {

enum EBOMType {
	BOM_NONE,
	BOM_LE,
	BOM_BE
};


class TBinaryConvert {
public:
	enum EBinaryTextMode { EBT_TEXT, EBT_HEX, EBT_DEFAULT = EBT_TEXT };

	static std::string binToAscii(const char value);
	static std::wstring binToAscii(const wchar_t value);

	static std::string strToAscii(const std::string& value);
	static std::wstring strToAscii(const std::wstring& value);

	static std::string binToHexA(const void *const buffer, size_t size, const bool upper = true, const char separator = ' ');
	static std::wstring binToHexW(const void *const buffer, size_t size, const bool upper = true, const wchar_t separator = L' ');

	static std::string binToAsciiA(const void *const buffer, size_t size, const bool upper = true);
	static std::wstring binToAsciiW(const void *const buffer, size_t size, const bool upper = true);

	static std::string binToHex(const void *const buffer, size_t size, const bool upper = true);
	static bool hexToBin(const std::string& hex, void *const buffer, size_t size);

	static std::string binToText(const void *const buffer, size_t size, const EBinaryTextMode mode = EBT_DEFAULT, const bool upper = true);
};


class TStringConvert {
public:
	static std::wstring MultiByteToWideString(const std::string& str);
	static std::wstring MultiByteToWideString(const char* s, const size_t size);
	static std::string WideToMultiByteString(const std::wstring& str);
	static std::string WideToMultiByteString(const wchar_t* ws, const size_t size);

	static std::string SingleByteToMultiByteString(const void *const buffer, size_t size, const app::ECodepage codepage);
	static std::wstring SingleByteToWideString(const void *const buffer, size_t size, const app::ECodepage codepage);
	static std::string SingleByteToMultiByteString(const std::string& str, const app::ECodepage codepage);
	static std::wstring SingleByteToWideString(const std::string& str, const app::ECodepage codepage);
};


class TUUID {
public:
	static std::string convertBinaryToUUID(const char* buffer, size_t size, bool upper = true);
	static std::string convertBinaryToMAC(const char* buffer, size_t size, bool upper = true);
};


class TOEM {
public:
	static void OemToAnsi(void *const buffer, size_t size);
	static void OemToAnsi(std::string& value);
};


class TASCII {
private:
	static std::string convertToMultiByteStr(const void *const buffer, size_t size, const unsigned char* table, const size_t* offsets);
	static void showMultiByteCodepage(const wchar_t* codepage, const size_t size);

protected:
	static EBOMType hasBOM(const unsigned char* buffer, size_t size);
	static EBOMType removeBOM(const unsigned char** buffer, size_t& size);

public:
	static void trim(const char* buffer, size_t& size);

	static bool isBOM(const void* buffer, size_t size);
	static bool isValidUTF8MultiByteStr(const void *const buffer, size_t size);

	static void showCP437MultiByteCodepage();
	static void showCP850MultiByteCodepage();
	static void showCP1252MultiByteCodepage();

	static std::string OemToMultiByteStr(const void *const buffer, size_t size);
	static std::string OemToMultiByteStr(const std::string& value);
	static std::string AnsiToMultiByteStr(const void *const buffer, size_t size);
	static std::string AnsiToMultiByteStr(const std::string& value);
	static std::string CP1252ToMultiByteStr(const std::string& value);
	static std::string CP1252ToMultiByteStr(const void *const buffer, size_t size);
	static std::string CP437ToMultiByteStr(const std::string& value);
	static std::string CP437ToMultiByteStr(const void *const buffer, size_t size);
	static std::string CP850ToMultiByteStr(const std::string& value);
	static std::string CP850ToMultiByteStr(const void *const buffer, size_t size);
	static std::string UTF16ToMultiByteStr(const void *const buffer, size_t size);
	static std::string UTF16ToMultiByteStr(const std::string& value);
	static std::string UTF8ToMultiByteStr(const void *const buffer, size_t size);
	static std::string UTF8ToMultiByteStr(const std::string& value);
};

} // namespace util

#endif /* ASCII_H_ */
