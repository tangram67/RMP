/*
 * asciitables.h
 *
 *  Created on: 16.11.2019
 *      Author: dirk
 */

#ifndef INC_ASCIITABLES_H_
#define INC_ASCIITABLES_H_

#include "templates.h"

using namespace util;

namespace util {

std::string ASCIITableA[] = {	"<NUL>","<SOH>","<STX>","<ETX>",
								"<EOT>","<ENQ>","<ACK>","<BEL>",
								"<BS>" ,"<HT>" ,"<LF>" ,"<VT>" ,
								"<FF>" ,"<CR>" ,"<SO>" ,"<SI>" ,
								"<DLE>","<DC1>","<DC2>","<DC3>",
								"<DC4>","<NAK>","<SYN>","<ETB>",
								"<CAN>","<EM>" ,"<SUB>","<ESC>",
								"<FS>" ,"<GS>" ,"<RS>" ,"<US>" };

std::wstring ASCIITableW[] = {	L"<NUL>",L"<SOH>",L"<STX>",L"<ETX>",
								L"<EOT>",L"<ENQ>",L"<ACK>",L"<BEL>",
								L"<BS>" ,L"<HT>" ,L"<LF>" ,L"<VT>" ,
								L"<FF>" ,L"<CR>" ,L"<SO>" ,L"<SI>" ,
								L"<DLE>",L"<DC1>",L"<DC2>",L"<DC3>",
								L"<DC4>",L"<NAK>",L"<SYN>",L"<ETB>",
								L"<CAN>",L"<EM>" ,L"<SUB>",L"<ESC>",
								L"<FS>" ,L"<GS>" ,L"<RS>" ,L"<US>" };

const int sizeASCIITableA = sizeOfArray(ASCIITableA);
const int sizeASCIITableW = sizeOfArray(ASCIITableW);

static const unsigned char OemToAnsiTable[] = {
		0,   1,   2,   3,   4,   5,   6,   7,   // #   0 - 7
		8,   9,   10,  11,  12,  13,  14,  15,  // #   8 - 15
		16,  17,  18,  19,  182, 186, 22,  23,  // #  16 - 23
		24,  25,  26,  27,  28,  29,  30,  31,  // #  24 - 31
		32,  33,  34,  35,  36,  37,  38,  39,  // #  32 - 39
		40,  41,  42,  43,  44,  45,  46,  47,  // #  40 - 47
		48,  49,  50,  51,  52,  53,  54,  55,  // #  48 - 55
		56,  57,  58,  59,  60,  61,  62,  63,  // #  56 - 63
		64,  65,  66,  67,  68,  69,  70,  71,  // #  64 - 71
		72,  73,  74,  75,  76,  77,  78,  79,  // #  72 - 79
		80,  81,  82,  83,  84,  85,  86,  87,  // #  80 - 87
		88,  89,  90,  91,  92,  93,  94,  95,  // #  88 - 95
		96,  97,  98,  99,  100, 101, 102, 103, // #  96 - 103
		104, 105, 106, 107, 108, 109, 110, 111, // # 104 - 111
		112, 113, 114, 115, 116, 117, 118, 119, // # 112 - 119
		120, 121, 122, 123, 124, 125, 126, 127, // # 120 - 127
		199, 252, 233, 226, 228, 224, 229, 231, // # 128 - 135
		234, 235, 232, 239, 238, 236, 196, 197, // # 136 - 143
		201, 181, 198, 244, 247, 242, 251, 249, // # 144 - 151
		223, 214, 220, 243, 183, 209, 158, 159, // # 152 - 159
		255, 173, 155, 156, 177, 157, 188, 21,  // # 160 - 167
		191, 169, 166, 174, 170, 237, 189, 187, // # 168 - 175
		248, 241, 253, 179, 180, 230, 20,  250, // # 176 - 183
		184, 185, 167, 175, 172, 171, 190, 168, // # 184 - 191
		192, 193, 194, 195, 142, 143, 146, 128, // # 192 - 199
		200, 144, 202, 203, 204, 205, 206, 207, // # 200 - 207
		208, 165, 210, 211, 212, 213, 153, 215, // # 208 - 215
		216, 217, 218, 219, 154, 221, 222, 225, // # 216 - 223
		133, 160, 131, 227, 132, 134, 145, 135, // # 224 - 231
		138, 130, 136, 137, 141, 161, 140, 139, // # 232 - 239
		240, 164, 149, 162, 147, 245, 148, 246, // # 240 - 247
		176, 151, 163, 150, 129, 178, 254, 152  // # 248 - 255
};

const char* nibbleLookupTableLA = { "0123456789abcdef" };
const char* nibbleLookupTableUA = { "0123456789ABCDEF" };
const wchar_t* nibbleLookupTableLW = { L"0123456789abcdef" };
const wchar_t* nibbleLookupTableUW = { L"0123456789ABCDEF" };

} // namespace util

#endif /* INC_ASCIITABLES_H_ */
