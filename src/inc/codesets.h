/*
 * codepagetypes.h
 *
 *  Created on: 08.03.2021
 *      Author: dirk
 */

#ifndef SRC_INC_CODESETS_H_
#define SRC_INC_CODESETS_H_

#include "gcc.h"
#include "nullptr.h"
#include "../config.h"

namespace app {

#ifdef STL_HAS_ENUM_CLASS
enum class ECodepage {
#else
enum ECodepage {
#endif
	CP_NONE,
	ISO_8859_1,  ISO_8859_2,  ISO_8859_3,  ISO_8859_4,  ISO_8859_5,  ISO_8859_6,
	ISO_8859_7,	 ISO_8859_8,  ISO_8859_9,  ISO_8859_10, ISO_8859_11, ISO_8859_12,
	ISO_8859_13, ISO_8859_14, ISO_8859_15, ISO_8859_16,
	CP_874,  CP_932,  CP_936,  CP_949,  CP_950,
	CP_1250, CP_1251, CP_1252, CP_1253, CP_1254, CP_1255, CP_1256, CP_1257, CP_1258,
	CP_437, // IBM / PC DOS / MS DOS codepage
	CP_850, // DOS Latin 1 codepage
	UTF_8, 	// Universal Character Set (UCS) Transformation Format 8 Bit
	UTF_16,	// Universal Character Set (UCS) Transformation Format 16 Bit
	ANSI,	// Windows Character Set, Western European ASCII
	OEM,	// DOS OEM Codepage
	CP_DEFAULT = UTF_8
};

struct TCodesets {
	ECodepage codeset;
	const char* name;
	const char* region;
};

static const struct TCodesets codesets[] =
{
	{ ECodepage::ISO_8859_1,  "ISO-8859-1",  "Western European" 		},
	{ ECodepage::ISO_8859_2,  "ISO-8859-2",  "Central European" 		},
	{ ECodepage::ISO_8859_3,  "ISO-8859-3",  "South European" 			},
	{ ECodepage::ISO_8859_4,  "ISO-8859-4",  "North European" 			},
	{ ECodepage::ISO_8859_5,  "ISO-8859-5",  "Cyrillic"	 				},
	{ ECodepage::ISO_8859_6,  "ISO-8859-6",  "Arabic" 					},
	{ ECodepage::ISO_8859_7,  "ISO-8859-7",  "Greek" 					},
	{ ECodepage::ISO_8859_8,  "ISO-8859-8",  "Hebrew" 					},
	{ ECodepage::ISO_8859_9,  "ISO-8859-9",  "Turkish" 					},
	{ ECodepage::ISO_8859_10, "ISO-8859-10", "Nordic"	 				},
	{ ECodepage::ISO_8859_11, "ISO-8859-11", "Thai" 					},
	{ ECodepage::ISO_8859_12, "ISO-8859-12", "Devanagari" 				},
	{ ECodepage::ISO_8859_13, "ISO-8859-13", "Baltic Rim" 				},
	{ ECodepage::ISO_8859_14, "ISO-8859-14", "Celtic" 					},
	{ ECodepage::ISO_8859_15, "ISO-8859-15", "Western European"			},
	{ ECodepage::ISO_8859_16, "ISO-8859-16", "South-Eastern European"	},
	{ ECodepage::CP_850,      "CP870",       "Thai"						},
	{ ECodepage::CP_874,      "CP874",       "Thai"						},
	{ ECodepage::CP_932,      "CP932",       "Japanese"      			},
	{ ECodepage::CP_936,      "CP936",       "Simplified Chinese"		},
	{ ECodepage::CP_949,      "CP949",       "Korean"					},
	{ ECodepage::CP_950,      "CP950",       "Traditional Chinese"		},
	{ ECodepage::CP_1250,     "CP1250",      "Central European"			},
	{ ECodepage::CP_1251,     "CP1251",      "Cyrillic"					},
	{ ECodepage::CP_1252,     "CP1252",      "Western European"			},
	{ ECodepage::CP_1253,     "CP1253",      "Greek"					},
	{ ECodepage::CP_1254,     "CP1254",      "Turkish"					},
	{ ECodepage::CP_1255,     "CP1255",      "Hebrew"					},
	{ ECodepage::CP_1256,     "CP1256",      "Arabic"					},
	{ ECodepage::CP_1257,     "CP1257",      "Baltic Rim"				},
	{ ECodepage::CP_1258,     "CP1258",      "Vietnamese"				},
	{ ECodepage::CP_437,      "CP437",       "IBM Codepage"				},
	{ ECodepage::UTF_8,       "UTF-8",       "Universal Character Set"	},
	{ ECodepage::UTF_16,      "UTF-16",      "Universal Character Set"	},
	{ ECodepage::ANSI,        "ANSI",        "Western European ASCII"	},
	{ ECodepage::OEM,         "OEM",         "DOS OEM Codepage"			},
	{ ECodepage::CP_DEFAULT,   nil,           nil						}
};

} // namespace app

#endif /* SRC_INC_CODESETS_H_ */
