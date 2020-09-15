/*
 * charconsts.h
 *
 *  Created on: 10.10.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef CHARCONSTS_H_
#define CHARCONSTS_H_

#include "gcc.h"
#include <string>
#include <cwchar>

/**************************************************************************
 *   ASCII - TABELLE mit Steuerkommandos
 *   Kunde     : Allgemein
 *   Auftr.Nr. :
 *
 *   Edition History :
 *   No.   Date       Change                                            By
 *   ---   --------   ------------------------------------------------  ---
 *   1.0   11/11/94   Erstversion für Intel ASM51                       Bri
 *   1.1   05/07/99   Version für Delphi 4.0
 *   1.2   18/01/15   Version für C++ (Linux GNU CC)
 *   1.3   15/05/15   Support for hex buffer convert
 *   1.4   19/06/15   Support for Ansi/Oem/UTF-8 convert
 *   1.5   10/10/16   Moved ASCII constants to separate header file
 *
 **************************************************************************/

namespace util {

#ifdef STL_HAS_CONSTEXPR

static constexpr char NUL = '\x00';
static constexpr char SOH = '\x01';
static constexpr char STX = '\x02';
static constexpr char ETX = '\x03';
static constexpr char EOT = '\x04';
static constexpr char ENQ = '\x05';
static constexpr char ACK = '\x06';
static constexpr char BEL = '\x07';
static constexpr char BS  = '\x08';
static constexpr char HT  = '\x09';
static constexpr char LF  = '\x0A';
static constexpr char VT  = '\x0B';
static constexpr char FF  = '\x0C';
static constexpr char CR  = '\x0D';
static constexpr char SO  = '\x0E';
static constexpr char SI  = '\x0F';
static constexpr char DLE = '\x10';
static constexpr char DC1 = '\x11';
static constexpr char DC2 = '\x12';
static constexpr char DC3 = '\x13';
static constexpr char DC4 = '\x14';
static constexpr char NAK = '\x15';
static constexpr char SYN = '\x16';
static constexpr char ETB = '\x17';
static constexpr char CAN = '\x18';
static constexpr char EM  = '\x19';
static constexpr char SUB = '\x1A';
static constexpr char ESC = '\x1B';
static constexpr char FS  = '\x1C';
static constexpr char GS  = '\x1D';
static constexpr char RS  = '\x1E';
static constexpr char US  = '\x1F';
static constexpr char SPC = '\x20';

static constexpr wchar_t WNUL = L'\x00';
static constexpr wchar_t WSOH = L'\x01';
static constexpr wchar_t WSTX = L'\x02';
static constexpr wchar_t WETX = L'\x03';
static constexpr wchar_t WEOT = L'\x04';
static constexpr wchar_t WENQ = L'\x05';
static constexpr wchar_t WACK = L'\x06';
static constexpr wchar_t WBEL = L'\x07';
static constexpr wchar_t WBS  = L'\x08';
static constexpr wchar_t WHT  = L'\x09';
static constexpr wchar_t WLF  = L'\x0A';
static constexpr wchar_t WVT  = L'\x0B';
static constexpr wchar_t WFF  = L'\x0C';
static constexpr wchar_t WCR  = L'\x0D';
static constexpr wchar_t WSO  = L'\x0E';
static constexpr wchar_t WSI  = L'\x0F';
static constexpr wchar_t WDLE = L'\x10';
static constexpr wchar_t WDC1 = L'\x11';
static constexpr wchar_t WDC2 = L'\x12';
static constexpr wchar_t WDC3 = L'\x13';
static constexpr wchar_t WDC4 = L'\x14';
static constexpr wchar_t WNAK = L'\x15';
static constexpr wchar_t WSYN = L'\x16';
static constexpr wchar_t WETB = L'\x17';
static constexpr wchar_t WCAN = L'\x18';
static constexpr wchar_t WEM  = L'\x19';
static constexpr wchar_t WSUB = L'\x1A';
static constexpr wchar_t WESC = L'\x1B';
static constexpr wchar_t WFS  = L'\x1C';
static constexpr wchar_t WGS  = L'\x1D';
static constexpr wchar_t WRS  = L'\x1E';
static constexpr wchar_t WUS  = L'\x1F';
static constexpr wchar_t WSPC = L'\x20';

#else

#define NUL '\x00'
#define SOH '\x01'
#define STX '\x02'
#define ETX '\x03'
#define EOT '\x04'
#define ENQ '\x05'
#define ACK '\x06'
#define BEL '\x07'
#define BS  '\x08'
#define HT  '\x09'
#define LF  '\x0A'
#define VT  '\x0B'
#define FF  '\x0C'
#define CR  '\x0D'
#define SO  '\x0E'
#define SI  '\x0F'
#define DLE '\x10'
#define DC1 '\x11'
#define DC2 '\x12'
#define DC3 '\x13'
#define DC4 '\x14'
#define NAK '\x15'
#define SYN '\x16'
#define ETB '\x17'
#define CAN '\x18'
#define EM  '\x19'
#define SUB '\x1A'
#define ESC '\x1B'
#define FS  '\x1C'
#define GS  '\x1D'
#define RS  '\x1E'
#define US  '\x1F'
#define SPC '\x20'

#define WNUL L'\x00'
#define WSOH L'\x01'
#define WSTX L'\x02'
#define WETX L'\x03'
#define WEOT L'\x04'
#define WENQ L'\x05'
#define WACK L'\x06'
#define WBEL L'\x07'
#define WBS  L'\x08'
#define WHT  L'\x09'
#define WLF  L'\x0A'
#define WVT  L'\x0B'
#define WFF  L'\x0C'
#define WCR  L'\x0D'
#define WSO  L'\x0E'
#define WSI  L'\x0F'
#define WDLE L'\x10'
#define WDC1 L'\x11'
#define WDC2 L'\x12'
#define WDC3 L'\x13'
#define WDC4 L'\x14'
#define WNAK L'\x15'
#define WSYN L'\x16'
#define WETB L'\x17'
#define WCAN L'\x18'
#define WEM  L'\x19'
#define WSUB L'\x1A'
#define WESC L'\x1B'
#define WFS  L'\x1C'
#define WGS  L'\x1D'
#define WRS  L'\x1E'
#define WUS  L'\x1F'
#define WSPC L'\x20'

#endif

#define USPC 0x20u
#define ULF  0x0Au
#define UCR  0x0Du
#define UHT  0x09u

} /* namespace util */

#endif /* CHARCONSTS_H_ */
