/*
 * locales.h
 *
 *  Created on: 24.10.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef LOCALIZATIONS_H_
#define LOCALIZATIONS_H_

/**********************************************************************************
#/bin/bash

#
# Extract locales from glibc
# Execute in folder localedata/locales
#
# LC_IDENTIFICATION
# title        "Akan locale for Ghana"
# source       "Sugar Labs / OLPC"
# address      ""
# contact      "sugarlabs.org"
# email        "libc-alpha@sourceware.org"
# tel          ""
# fax          ""
# language     "Akan"
# territory    "Ghana"
# revision     "1.0"
# date         "2013-08-24"
#

SAVEIFS=$IFS
GREP=/bin/grep
NAWK=/usr/bin/awk

IFS=$(echo -en "\n\b")

FILES="??_??"
OUT="locales.csv"
HEADER="locales.h"
LOCALES="locales.txt"

[ -f $OUT ] && rm $OUT
[ -f $HEADER ] && rm $HEADER
[ -f $LOCALES ] && rm $LOCALES

for f in $FILES
do
  FILE=$f
  TITLE=`$GREP -m1 "title" $FILE | $NAWK 'BEGIN{FS="\""}{print $2}'`
  LANGUAGE=`$GREP -m1 "language" $FILE | $NAWK 'BEGIN{FS="\""}{print $2}'`
  COUNTRY=`$GREP -m1 "territory" $FILE | $NAWK 'BEGIN{FS="\""}{print $2}'`
  CHARSET=`$GREP -i -m1 "charset" $FILE | $NAWK '{print $3}'`

  [ ".$TITLE" = "." ] && TITLE="Undefined"
  [ ".$LANGUAGE" = "." ] && LANGUAGE="Undefined"
  [ ".$COUNTRY" = "." ] && COUNTRY="Undefined"
  [ ".$CHARSET" = "." ] && CHARSET="UTF-8"

  if [ ".$CHARSET" != ".UTF-8" ] && [ ".$CHARSET" != "." ] && \
     [ ".$CHARSET" != ".ISO-8859-1" ] && [ ".$CHARSET" != ".ISO-8859-2" ] && \
     [ ".$CHARSET" != ".ISO-8859-3" ] && [ ".$CHARSET" != ".ISO-8859-4" ] && \
     [ ".$CHARSET" != ".ISO-8859-5" ] && [ ".$CHARSET" != ".ISO-8859-6" ] && \
     [ ".$CHARSET" != ".ISO-8859-7" ] && [ ".$CHARSET" != ".ISO-8859-8" ] && \
     [ ".$CHARSET" != ".ISO-8859-9" ] && [ ".$CHARSET" != ".ISO-8859-10" ] && \
     [ ".$CHARSET" != ".ISO-8859-11" ] && [ ".$CHARSET" != ".ISO-8859-12" ] && \
     [ ".$CHARSET" != ".ISO-8859-13" ] && [ ".$CHARSET" != ".ISO-8859-14" ] && \
     [ ".$CHARSET" != ".ISO-8859-15" ] && [ ".$CHARSET" != ".ISO-8859-16" ] && \
     [ ".$CHARSET" != ".CP874" ] && [ ".$CHARSET" != ".CP932" ] && \
     [ ".$CHARSET" != ".CP936" ] && [ ".$CHARSET" != ".CP949" ] && \
     [ ".$CHARSET" != ".CP950" ] && [ ".$CHARSET" != ".CP1250" ] && \
     [ ".$CHARSET" != ".CP1251" ] && [ ".$CHARSET" != ".CP1252" ] && \
     [ ".$CHARSET" != ".CP1253" ] && [ ".$CHARSET" != ".CP1254" ] && \
     [ ".$CHARSET" != ".CP1255" ] && [ ".$CHARSET" != ".CP1256" ] && \
     [ ".$CHARSET" != ".CP1257" ] && [ ".$CHARSET" != ".CP1258" ] ; then
    CHARSET="UTF-8"
  fi

  LOCALE=$FILE
  LINE="\""$FILE"\";\""$TITLE"\";\""$LANGUAGE"\";\""$COUNTRY"\";\""$CHARSET"\""

  echo $LOCALE";"$LINE
  echo $LINE >> $OUT

  REGION=${FILE:0:2}

  LINE="\""$FILE"\", \""$REGION"\", \""$TITLE"\", \""$LANGUAGE"\", \""$COUNTRY"\", \""$CHARSET"\""
  echo -e "\t{ "ELocale::$LOCALE, $LINE" }," >> $HEADER

  echo -n " "$LOCALE, >> $LOCALES
done

# restore $IFS
IFS=$SAVEIFS
***********************************************************************************/

/**********************************************************************************

# List installed locales:
locale -a

# List supported locales
less /usr/share/i18n/SUPPORTED

# Install needed locales (not sufficient, see instructions below!)
sudo locale-gen en_US
sudo locale-gen de_DE
sudo locale-gen es_ES
sudo locale-gen fr_FR
sudo locale-gen it_IT
sudo update-locale

# Under Ubuntu Desktop
sudo apt-get install language-pack-de language-pack-en language-pack-es language-pack-fr

sudo mcedit /var/lib/locales/supported.d/local
de_DE.UTF-8 UTF-8
en_US.UTF-8 UTF-8
fr_FR.ISO-8859-1 ISO-8859-1
it_IT.ISO-8859-1 ISO-8859-1
es_ES.ISO-8859-1 ISO-8859-1

sudo locale-gen
###sudo dpkg-reconfigure locales

# Under Debian Server
sudo mcedit /etc/locale.gen
sudo locale-gen

***********************************************************************************/

#include "gcc.h"
#include "nullptr.h"
#include "../config.h"

namespace app {


#define WIDE_STR(str) WIDE_STR2(str)
#define WIDE_STR2(str) L##str

STATIC_CONST char BOOLEAN_TRUE_NAME_A[] = DEFAULT_BOOLEAN_TRUE_NAME;
STATIC_CONST char BOOLEAN_FALSE_NAME_A[] = DEFAULT_BOOLEAN_FALSE_NAME;
STATIC_CONST wchar_t BOOLEAN_TRUE_NAME_W[] = WIDE_STR(DEFAULT_BOOLEAN_TRUE_NAME);
STATIC_CONST wchar_t BOOLEAN_FALSE_NAME_W[] = WIDE_STR(DEFAULT_BOOLEAN_FALSE_NAME);

STATIC_CONST char DEFAULT_LOCALE_TIME_FORMAT_A[] = "%c";
STATIC_CONST wchar_t DEFAULT_LOCALE_TIME_FORMAT_W[] = L"%c";

STATIC_CONST char NO_GROUPING_VALUE[] = "\000";

#undef WIDE_STR
#undef WIDE_STR2


// For GCC compatibility every identifier still has to be unique!
#ifdef STL_HAS_ENUM_CLASS
enum class ELocale {
#else
enum ELocale {
#endif
	aa_DJ, aa_ER, aa_ET, af_ZA, ak_GH, am_ET, an_ES, ar_AE,
	ar_BH, ar_DZ, ar_EG, ar_IN, ar_IQ, ar_JO, ar_KW, ar_LB,
	ar_LY, ar_MA, ar_OM, ar_QA, ar_SA, ar_SD, ar_SS, ar_SY,
	ar_TN, ar_YE, as_IN, az_AZ, be_BY, bg_BG, bn_BD, bn_IN,
	bo_CN, bo_IN, br_FR, bs_BA, ca_AD, ca_ES, ca_FR, ca_IT,
	ce_RU, cs_CZ, cv_RU, cy_GB, da_DK, de_AT, de_BE, de_CH,
	de_DE, de_LU, dv_MV, dz_BT, el_CY, el_GR, en_AG, en_AU,
	en_BW, en_CA, en_DK, en_GB, en_HK, en_IE, en_IN, en_NG,
	en_NZ, en_PH, en_SG, en_US, en_ZA, en_ZM, en_ZW, es_AR,
	es_BO, es_CL, es_CO, es_CR, es_CU, es_DO, es_EC, es_ES,
	es_GT, es_HN, es_MX, es_NI, es_PA, es_PE, es_PR, es_PY,
	es_SV, es_US, es_UY, es_VE, et_EE, eu_ES, fa_IR, ff_SN,
	fi_FI, fo_FO, fr_BE, fr_CA, fr_CH, fr_FR, fr_LU, fy_DE,
	fy_NL, ga_IE, gd_GB, gl_ES, gu_IN, gv_GB, ha_NG, he_IL,
	hi_IN, hr_HR, ht_HT, hu_HU, hy_AM, ia_FR, id_ID, ig_NG,
	ik_CA, is_IS, it_CH, it_IT, iu_CA, iw_IL, ja_JP, ka_GE,
	kk_KZ, kl_GL, km_KH, kn_IN, ko_KR, ks_IN, ku_TR, kw_GB,
	ky_KG, lb_LU, lg_UG, li_BE, li_NL, lo_LA, lt_LT, lv_LV,
	mg_MG, mi_NZ, mk_MK, ml_IN, mn_MN, mr_IN, ms_MY, mt_MT,
	my_MM, nb_NO, ne_NP, nl_AW, nl_BE, nl_NL, nn_NO, nr_ZA,
	oc_FR, om_ET, om_KE, or_IN, os_RU, pa_IN, pa_PK, pl_PL,
	ps_AF, pt_BR, pt_PT, ro_RO, ru_RU, ru_UA, rw_RW, sa_IN,
	sc_IT, sd_IN, se_NO, si_LK, sk_SK, sl_SI, so_DJ, so_ET,
	so_KE, so_SO, sq_AL, sq_MK, sr_ME, sr_RS, ss_ZA, st_ZA,
	sv_FI, sv_SE, sw_KE, sw_TZ, ta_IN, ta_LK, te_IN, tg_TJ,
	th_TH, ti_ER, ti_ET, tk_TM, tl_PH, tn_ZA, tr_CY, tr_TR,
	ts_ZA, tt_RU, ug_CN, uk_UA, ur_IN, ur_PK, uz_UZ, ve_ZA,
	vi_VN, wa_BE, wo_SN, xh_ZA, yi_US, yo_NG, zh_CN, zh_HK,
	zh_SG, zh_TW, zu_ZA, // End of real world locales
	cloc,	// Standard C locale
	ploc,	// POSIX locale
	siloc,	// SI scientific locale
	sysloc,	// System standard locale
	nloc	// No locale
};

#ifdef STL_HAS_ENUM_CLASS
enum class ERegion {
#else
enum ERegion {
#endif
	nreg = 0,
	de = 1L << 1,
	en = 1L << 2,
	it = 1L << 3,
	fr = 1L << 4,
	es = 1L << 5,
	sireg = 1L << 6,
	creg =  1L << 7,
	allreg = (de | en | it | fr | es)
};

struct TRegion {
	ERegion region;
	const char* name;
	const char* mainland;
};

static const struct TRegion regions[] =
{
	{ ERegion::de,    "de", "Western Europe" },
	{ ERegion::en,    "en", "Western Europe" },
	{ ERegion::it,    "it", "South Europe"   },
	{ ERegion::fr,    "fr", "Western Europe" },
	{ ERegion::es,    "es", "South Europe"   },
	{ ERegion::sireg, "SI", "Scientific"     },
	{ ERegion::creg,  "CC", "System"         },
	{ ERegion::nreg,   nil,  nil             }
};

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


struct TLanguage {
	ELocale locale;
	const char* name;
	const char* region;
	const char* description;
	const char* language;
	const char* country;
	const char* codeset;
	const char* datetimeA;
	const wchar_t* datetimeW;
	const char* booleanTrueNameA;
	const char* booleanFalseNameA;
	const wchar_t* booleanTrueNameW;
	const wchar_t* booleanFalseNameW;
};

//
// For date format by country see:
//   https://en.wikipedia.org/wiki/Date_format_by_country
//   https://www.microsoft.com/resources/msdn/goglobal/default.mspx?OS=Windows%207
//   ftp://ftp.software.ibm.com/software/globalization/documents/
//
static const struct TLanguage locales[] =
{
	{ ELocale::aa_DJ, "aa_DJ", "aa", "Afar language locale for Djibouti (Cadu/Laaqo Dialects).", "Afar", "Djibouti", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::aa_ER, "aa_ER", "aa", "Afar language locale for Eritrea (Cadu/Laaqo Dialects).", "Afar", "Eritrea", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::aa_ET, "aa_ET", "aa", "Afar language locale for Ethiopia (Cadu/Carra Dialects).", "Afar", "Ethiopia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::af_ZA, "af_ZA", "af", "Afrikaans locale for South Africa", "Afrikaans", "South Africa", "ISO-8859-1", "%Y/%m/%d %H:%M:%S", L"%Y/%m/%d %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ak_GH, "ak_GH", "ak", "Akan locale for Ghana", "Akan", "Ghana", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::am_ET, "am_ET", "am", "Amharic language locale for Ethiopia.", "Amharic", "Ethiopia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::an_ES, "an_ES", "an", "Aragonese locale for Spain", "Aragonese", "Spain", "ISO-8859-15", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_AE, "ar_AE", "ar", "Arabic language locale for United Arab Emirates", "Arabic", "United Arab Emirates", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_BH, "ar_BH", "ar", "Arabic language locale for Bahrain", "Arabic", "Bahrain", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_DZ, "ar_DZ", "ar", "Arabic language locale for Algeria", "Arabic", "Algeria", "UTF-8", "%d-%m-%Y %H:%M:%S", L"%d-%m-%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_EG, "ar_EG", "ar", "Arabic language locale for Egypt", "Arabic", "Egypt", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_IN, "ar_IN", "ar", "Arabic language locale for India", "Arabic", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_IQ, "ar_IQ", "ar", "Arabic language locale for Iraq", "Arabic", "Iraq", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_JO, "ar_JO", "ar", "Arabic language locale for Jordan", "Arabic", "Jordan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_KW, "ar_KW", "ar", "Arabic language locale for Kuwait", "Arabic", "Kuwait", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_LB, "ar_LB", "ar", "Arabic language locale for Lebanon", "Arabic", "Lebanon", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_LY, "ar_LY", "ar", "Arabic language locale for Libyan Arab Jamahiriya", "Arabic", "Libyan Arab Jamahiriya", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_MA, "ar_MA", "ar", "Arabic language locale for Morocco", "Arabic", "Morocco", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_OM, "ar_OM", "ar", "Arabic language locale for Oman", "Arabic", "Oman", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_QA, "ar_QA", "ar", "Arabic language locale for Qatar", "Arabic", "Qatar", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_SA, "ar_SA", "ar", "Arabic locale for Saudi Arabia", "Arabic", "Saudi Arabia", "ISO-8859-6", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_SD, "ar_SD", "ar", "Arabic language locale for Sudan", "Arabic", "Sudan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_SS, "ar_SS", "ar", "Arabic language locale for South Sudan", "Arabic", "South Sudan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_SY, "ar_SY", "ar", "Arabic language locale for Syrian Arab Republic", "Arabic", "Syrian Arab Republic", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_TN, "ar_TN", "ar", "Arabic language locale for Tunisia", "Arabic", "Tunisia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ar_YE, "ar_YE", "ar", "Arabic language locale for Yemen", "Arabic", "Yemen", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::as_IN, "as_IN", "as", "Assamese language locale for India", "Assamese", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::az_AZ, "az_AZ", "az", "Azeri language locale for Azerbaijan (latin)", "Azeri", "Azerbaijan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::be_BY, "be_BY", "be", "Belarusian locale for Belarus", "Belarusian", "Belarus", "CP1251", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::bg_BG, "bg_BG", "bg", "Bulgarian locale for Bulgaria", "Bulgarian", "Bulgaria", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::bn_BD, "bn_BD", "bn", "Bengali/Bangla language locale for Bangladesh", "Bengali/Bangla", "Bangladesh", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::bn_IN, "bn_IN", "bn", "Bengali language locale for India", "Bengali language locale for India", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::bo_CN, "bo_CN", "bo", "Tibetan language locale for P.R. of China", "Tibetan", "P.R. of China", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::bo_IN, "bo_IN", "bo", "Tibetan language locale for India", "Tibetan", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::br_FR, "br_FR", "br", "Breton language locale for France", "Breton", "France", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::bs_BA, "bs_BA", "bs", "Bosnian language locale for Bosnia and Herzegowina", "Bosnian", "Bosnia and Herzegowina", "ISO-8859-2", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ca_AD, "ca_AD", "ca", "Catalan locale for Andorra ", "Catalan", "Andorra", "ISO-8859-15", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ca_ES, "ca_ES", "ca", "Catalan locale for Spain", "Catalan", "Spain", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ca_FR, "ca_FR", "ca", "Catalan locale for France ", "Catalan", "France", "ISO-8859-15", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ca_IT, "ca_IT", "ca", "Catalan locale for Italy (L'Alguer) ", "Catalan", "Italy (L'Alguer)", "ISO-8859-15", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ce_RU, "ce_RU", "ce", "Chechen locale for Russian Federation", "Chechen", "Russian Federation", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::cs_CZ, "cs_CZ", "cs", "Czech locale for the Czech Republic", "Czech", "Czech Republic", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::cv_RU, "cv_RU", "cv", "Chuvash locale for Russia", "Chuvash", "Russia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::cy_GB, "cy_GB", "cy", "Welsh language locale for Great Britain", "Welsh language locale for Great Britain", "Great Britain", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::da_DK, "da_DK", "da", "Danish locale for Denmark", "Danish", "Denmark", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::de_AT, "de_AT", "de", "German locale for Austria", "German", "Austria", "ISO-8859-1", "%d.%m.%Y %H:%M:%S", L"%d.%m.%Y %H:%M:%S", "Ja", "Nein", L"Ja", L"Nein"},
	{ ELocale::de_BE, "de_BE", "de", "German locale for Belgium", "German", "Belgium", "ISO-8859-1", "%d.%m.%Y %H:%M:%S", L"%d.%m.%Y %H:%M:%S", "Ja", "Nein", L"Ja", L"Nein"},
	{ ELocale::de_CH, "de_CH", "de", "German locale for Switzerland", "German", "Switzerland", "UTF-8", "%d.%m.%Y %H:%M:%S", L"%d.%m.%Y %H:%M:%S", "Ja", "Nein", L"Ja", L"Nein"},
	{ ELocale::de_DE, "de_DE", "de", "German locale for Germany", "German", "Germany", "UTF-8", "%d.%m.%Y %H:%M:%S", L"%d.%m.%Y %H:%M:%S", "Ja", "Nein", L"Ja", L"Nein"},
	{ ELocale::de_LU, "de_LU", "de", "German locale for Luxemburg", "German", "Luxemburg", "ISO-8859-1", "%d.%m.%Y %H:%M:%S", L"%d.%m.%Y %H:%M:%S", "Ja", "Nein", L"Ja", L"Nein"},
	{ ELocale::dv_MV, "dv_MV", "dv", "Dhivehi Language Locale for Maldives", "Divehi", "Maldives", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::dz_BT, "dz_BT", "dz", "Dzongkha language locale for Bhutan", "Dzongkha language locale for Bhutan", "Bhutan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::el_CY, "el_CY", "el", "Greek locale for Cyprus", "Greek", "Cyprus", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::el_GR, "el_GR", "el", "Greek locale for Greece", "Greek", "Greece", "ISO-8859-7", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::en_AG, "en_AG", "en", "English language locale for Antigua and Barbuda", "English", "Antigua and Barbuda", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_AU, "en_AU", "en", "English locale for Australia", "English", "Australia", "ISO-8859-1", "%d/%m/%Y %I:%M:%S %p", L"%d/%m/%Y %I:%M:%S %p", "yes", "no", L"yes", L"no"},
	{ ELocale::en_BW, "en_BW", "en", "English locale for Botswana", "English", "Botswana", "ISO-8859-1", "%d/%m/%Y %I:%M:%S %p", L"%d/%m/%Y %I:%M:%S %p", "yes", "no", L"yes", L"no"},
	{ ELocale::en_CA, "en_CA", "en", "English locale for Canada", "English", "Canada", "ISO-8859-1", "%d/%m/%Y %I:%M:%S %p", L"%d/%m/%Y %I:%M:%S %p", "yes", "no", L"yes", L"no"},
	{ ELocale::en_DK, "en_DK", "en", "English locale for Denmark", "English", "Denmark", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_GB, "en_GB", "en", "English locale for Britain", "English", "Great Britain", "UTF-8", "%d/%m/%Y %I:%M:%S %P", L"%d/%m/%Y %I:%M:%S %P", "yes", "no", L"yes", L"no"},
	{ ELocale::en_HK, "en_HK", "en", "English locale for Hong Kong", "English", "Hong Kong", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_IE, "en_IE", "en", "English locale for Ireland", "English", "Ireland", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_IN, "en_IN", "en", "English language locale for India", "English", "India", "UTF-8", "%d-%m-%Y %H:%M:%S", L"%d-%m-%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_NG, "en_NG", "en", "English locale for Nigeria", "English", "Nigeria", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_NZ, "en_NZ", "en", "English locale for New Zealand", "English", "New Zealand", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_PH, "en_PH", "en", "English language locale for Philippines", "English", "Philippines", "UTF-8", "%m/%d/%Y %H:%M:%S", L"%m/%d/%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_SG, "en_SG", "en", "English language locale for Singapore", "English", "Singapore", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_US, "en_US", "en", "English locale for the USA", "English", "USA", "UTF-8", "%m/%d/%Y %I:%M:%S %p", L"%m/%d/%Y %I:%M:%S %p", "yes", "no", L"yes", L"no"},
	{ ELocale::en_ZA, "en_ZA", "en", "English locale for South Africa", "English", "South Africa", "ISO-8859-1", "%Y/%m/%d %I:%M:%S %p", L"%Y/%m/%d %I:%M:%S %p", "yes", "no", L"yes", L"no"},
	{ ELocale::en_ZM, "en_ZM", "en", "English locale for Zambia", "English", "Zambia", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "yes", "no", L"yes", L"no"},
	{ ELocale::en_ZW, "en_ZW", "en", "English locale for Zimbabwe", "English", "Zimbabwe", "ISO-8859-1", "%d/%m/%Y %I:%M:%S %p", L"%d/%m/%Y %I:%M:%S %p", "yes", "no", L"yes", L"no"},
	{ ELocale::es_AR, "es_AR", "es", "Spanish locale for Argentina", "Spanish", "Argentina", "ISO-8859-1", "%d/%m/%Y %I:%M:%S %p", L"%d/%m/%Y %I:%M:%S %p", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_BO, "es_BO", "es", "Spanish locale for Bolivia", "Spanish", "Bolivia", "ISO-8859-1", "%d/%m/%Y %I:%M:%S %p", L"%d/%m/%Y %I:%M:%S %p", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_CL, "es_CL", "es", "Spanish locale for Chile", "Spanish", "Chile", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_CO, "es_CO", "es", "Spanish locale for Colombia", "Spanish", "Colombia", "ISO-8859-1", "%d/%m/%Y %I:%M:%S %p", L"%d/%m/%Y %I:%M:%S %p", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_CR, "es_CR", "es", "Spanish locale for Costa Rica", "Spanish", "Costa Rica", "ISO-8859-1", "%d/%m/%Y %I:%M:%S %p", L"%d/%m/%Y %I:%M:%S %p", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_CU, "es_CU", "es", "Spanish locale for Cuba", "Spanish", "Cuba", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_DO, "es_DO", "es", "Spanish locale for Dominican Republic", "Spanish", "Dominican Republic", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_EC, "es_EC", "es", "Spanish locale for Ecuador", "Spanish", "Ecuador", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_ES, "es_ES", "es", "Spanish locale for Spain", "Spanish", "Spain", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_GT, "es_GT", "es", "Spanish locale for Guatemala", "Spanish", "Guatemala", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_HN, "es_HN", "es", "Spanish locale for Honduras", "Spanish", "Honduras", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_MX, "es_MX", "es", "Spanish locale for Mexico", "Spanish", "Mexico", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_NI, "es_NI", "es", "Spanish locale for Nicaragua", "Spanish", "Nicaragua", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_PA, "es_PA", "es", "Spanish locale for Panama", "Spanish", "Panama", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_PE, "es_PE", "es", "Spanish locale for Peru", "Spanish", "Peru", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_PR, "es_PR", "es", "Spanish locale for Puerto Rico", "Spanish", "Puerto Rico", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_PY, "es_PY", "es", "Spanish locale for Paraguay", "Spanish", "Paraguay", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_SV, "es_SV", "es", "Spanish locale for El Salvador", "Spanish", "El Salvador", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_US, "es_US", "es", "Spanish locale for the USA", "Spanish", "USA", "ISO-8859-1", "%d/%m/%Y %I:%M:%S %p", L"%d/%m/%Y %I:%M:%S %p", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_UY, "es_UY", "es", "Spanish locale for Uruguay", "Spanish", "Uruguay", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::es_VE, "es_VE", "es", "Spanish locale for Venezuela", "Spanish", "Venezuela", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::et_EE, "et_EE", "et", "Estonian locale for Estonia", "Estonian", "Estonia", "ISO-8859-15", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::eu_ES, "eu_ES", "eu", "Basque locale for Spain", "Basque", "Spain", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::fa_IR, "fa_IR", "fa", "Persian locale for Iran", "Persian", "Iran", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ff_SN, "ff_SN", "ff", "Fulah locale for Senegal", "ff", "Senegal", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::fi_FI, "fi_FI", "fi", "Finnish locale for Finland", "Undefined", "Finland", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::fo_FO, "fo_FO", "fo", "Faroese locale for Faroe Islands", "Faroese", "Faroe Islands", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::fr_BE, "fr_BE", "fr", "French locale for Belgium", "French", "Belgium", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "oui", "non", L"oui", L"non"},
	{ ELocale::fr_CA, "fr_CA", "fr", "French locale for Canada", "French", "Canada", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "oui", "non", L"oui", L"non"},
	{ ELocale::fr_CH, "fr_CH", "fr", "French locale for Switzerland", "French", "Switzerland", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "oui", "non", L"oui", L"non"},
	{ ELocale::fr_FR, "fr_FR", "fr", "French locale for France", "French", "France", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "oui", "non", L"oui", L"non"},
	{ ELocale::fr_LU, "fr_LU", "fr", "French locale for Luxemburg", "French", "Luxemburg", "ISO-8859-1", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", "oui", "non", L"oui", L"non"},
	{ ELocale::fy_DE, "fy_DE", "fy", "Sater Frisian and North Frisian Locale for Germany", "Frisian", "Germany", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::fy_NL, "fy_NL", "fy", "Frisian locale for the Netherlands", "Frisian", "Netherlands", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ga_IE, "ga_IE", "ga", "Irish locale for Ireland", "Irish", "Ireland", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::gd_GB, "gd_GB", "gd", "Scots Gaelic language locale for Great Britain", "Scots Gaelic", "Great Britain", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::gl_ES, "gl_ES", "gl", "Galician locale for Spain", "Galician", "Spain", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::gu_IN, "gu_IN", "gu", "Gujarati Language Locale For India", "Gujarati", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::gv_GB, "gv_GB", "gv", "Manx Gaelic locale for Britain", "Manx Gaelic", "Britain", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ha_NG, "ha_NG", "ha", "Hausa locale for Nigeria", "Hausa", "Nigeria", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::he_IL, "he_IL", "he", "Hebrew locale for Israel", "Hebrew", "Israel", "ISO-8859-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::hi_IN, "hi_IN", "hi", "Hindi language locale for India", "Hindi", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::hr_HR, "hr_HR", "hr", "Croatian locale for Croatia", "Croatian", "Croatia", "ISO-8859-2", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ht_HT, "ht_HT", "ht", "Kreyol locale for Haiti", "Kreyol", "Haiti", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::hu_HU, "hu_HU", "hu", "Hungarian locale for Hungary", "Hungarian", "Hungary", "ISO-8859-2", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::hy_AM, "hy_AM", "hy", "Armenian language locale for Armenia", "Armenian language locale for Armenia", "Armenia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ia_FR, "ia_FR", "ia", "Interlingua locale for France", "Interlingua", "France", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::id_ID, "id_ID", "id", "Indonesian locale for Indonesia", "Indonesian", "Indonesia", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ig_NG, "ig_NG", "ig", "Igbo locale for Nigeria", "Igbo", "Nigeria", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ik_CA, "ik_CA", "ik", "Inupiaq locale for Canada", "Inupiaq", "Canada", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::is_IS, "is_IS", "is", "Icelandic locale for Iceland", "Icelandic", "Iceland", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::it_CH, "it_CH", "it", "Italian locale for Switzerland", "Italian", "Switzerland", "ISO-8859-1", "%d.%m.%Y %H:%M:%S", L"%d.%m.%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::it_IT, "it_IT", "it", "Italian locale for Italy", "Italian", "Italy", "UTF-8", "%d/%m/%Y %H:%M:%S", L"%d/%m/%Y %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::iu_CA, "iu_CA", "iu", "Inuktitut language locale for Nunavut, Canada", "Inuktitut language locale for Nunavut, Canada", "Canada", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::iw_IL, "iw_IL", "iw", "Hebrew locale for Israel", "Hebrew", "Israel", "ISO-8859-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ja_JP, "ja_JP", "ja", "Japanese language locale for Japan", "Japanese language locale for Japan", "Japan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ka_GE, "ka_GE", "ka", "Georgian language locale for Georgia", "Georgian language locale for Georgia", "Georgia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::kk_KZ, "kk_KZ", "kk", "Kazakh locale for Kazakhstan", "Kazakh", "Kazakhstan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::kl_GL, "kl_GL", "kl", "Greenlandic locale for Greenland", "Greenlandic", "Greenland", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::km_KH, "km_KH", "km", "Khmer locale for Cambodia", "Khmer", "Cambodia", "UTF-8", "%Y-%m-%d %H:%M:%S", L"%Y-%m-%d %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::kn_IN, "kn_IN", "kn", "Kannada language locale for India", "Kannada", "India", "UTF-8", "%Y-%m-%d %H:%M:%S", L"%Y-%m-%d %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ko_KR, "ko_KR", "ko", "Korean locale for Republic of Korea", "Korean", "Undefined", "UTF-8", "%Y-%m-%d %H:%M:%S", L"%Y-%m-%d %H:%M:%S", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ks_IN, "ks_IN", "ks", "Kashmiri language locale for India", "Kashmiri", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ku_TR, "ku_TR", "ku", "Kurdish (latin) locale for Turkey", "Kurdish", "Turkey", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::kw_GB, "kw_GB", "kw", "Cornish locale for Britain", "Cornish", "Britain", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ky_KG, "ky_KG", "ky", "Kyrgyz Language Locale for Kyrgyzstan", "Kyrgyz", "Kyrgyzstan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::lb_LU, "lb_LU", "lb", "Luxembourgish locale for Luxembourg", "Luxembourgish", "Luxembourg", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::lg_UG, "lg_UG", "lg", "Luganda locale for Uganda", "Luganda", "Uganda", "ISO-8859-10", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::li_BE, "li_BE", "li", "Limburgish Language Locale for Belgium", "Limburgish", "Belgium", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::li_NL, "li_NL", "li", "Limburgish Language Locale for the Netherlands", "Limburgish", "Netherlands", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::lo_LA, "lo_LA", "lo", "Lao locale for Laos", "Lao", "Laos", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::lt_LT, "lt_LT", "lt", "Lithuanian locale for Lithuania", "Lithuanian", "Lithuania", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::lv_LV, "lv_LV", "lv", "Latvian locale for Latvia", "Latvian", "Latvia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::mg_MG, "mg_MG", "mg", "Malagasy locale for Madagascar", "Malagasy", "Madagascar", "ISO-8859-15", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::mi_NZ, "mi_NZ", "mi", "Maori language locale for New Zealand", "Maori", "New Zealand", "ISO-8859-13", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::mk_MK, "mk_MK", "mk", "Macedonian locale for Macedonia", "Macedonian", "Macedonia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ml_IN, "ml_IN", "ml", "Malayalam language locale for India", "Malayalam", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::mn_MN, "mn_MN", "mn", "Mongolian locale for Mongolia", "Mongolian", "Mongolia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::mr_IN, "mr_IN", "mr", "Marathi language locale for India", "Marathi", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ms_MY, "ms_MY", "ms", "Malay language locale for Malaysia", "Malay", "Malaysia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::mt_MT, "mt_MT", "mt", "Maltese language locale for Malta", "Maltese", "malta", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::my_MM, "my_MM", "my", "Burmese language locale for Myanmar", "Burmese language locale for Myanmar", "Myanmar", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::nb_NO, "nb_NO", "nb", "Norwegian (Bokmal) locale for Norway", "Norwegian", "Norway", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ne_NP, "ne_NP", "ne", "Nepali language locale for Nepal", "Nepali", "Nepal", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::nl_AW, "nl_AW", "nl", "Dutch language locale for Aruba", "Dutch", "Aruba", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::nl_BE, "nl_BE", "nl", "Dutch locale for Belgium", "Dutch", "Belgium", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::nl_NL, "nl_NL", "nl", "Dutch locale for the Netherlands", "Dutch", "Netherlands", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::nn_NO, "nn_NO", "nn", "Nynorsk language locale for Norway", "Nynorsk", "Norway", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::nr_ZA, "nr_ZA", "nr", "Southern Ndebele locale for South Africa", "Southern Ndebele", "South Africa", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::oc_FR, "oc_FR", "oc", "Occitan Language Locale for France", "Occitan", "France", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::om_ET, "om_ET", "om", "Oromo language locale for Ethiopia.", "Oromo", "Ethiopia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::om_KE, "om_KE", "om", "Oromo language locale for Kenya.", "Oromo", "Kenya", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::or_IN, "or_IN", "or", "Odia language locale for India", "Odia language locale for India", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::os_RU, "os_RU", "os", "Ossetian locale for Russia", "Ossetian", "Russia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::pa_IN, "pa_IN", "pa", "Punjabi language locale for Indian Punjabi(Gurmukhi)", "Undefined", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::pa_PK, "pa_PK", "pa", "Punjabi (Shahmukhi) Language Locale for Pakistan", "Punjabi (Shahmukhi)", "Pakistan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::pl_PL, "pl_PL", "pl", "Polish locale for Poland", "Polish", "Poland", "ISO-8859-2", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ps_AF, "ps_AF", "ps", "Pashto locale for Afghanistan", "Pashto", "Afghanistan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::pt_BR, "pt_BR", "pt", "Portuguese locale for Brasil", "Portuguese", "Brasil", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::pt_PT, "pt_PT", "pt", "Portuguese locale for Portugal", "Portuguese", "Portugal", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ro_RO, "ro_RO", "ro", "Romanian locale for Romania", "Romanian", "Romania", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ru_RU, "ru_RU", "ru", "Russian locale for Russia", "Russian", "Russia", "ISO-8859-5", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ru_UA, "ru_UA", "ru", "Russian locale for Ukraine", "Russian", "Ukraine", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::rw_RW, "rw_RW", "rw", "Kinyarwanda language locale for Rwanda", "Kinyarwanda language locale for Rwanda", "Rwanda", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sa_IN, "sa_IN", "sa", "Sanskrit language locale for India", "Sanskrit", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sc_IT, "sc_IT", "sc", "Sardinian locale for Italy", "Sardinian", "Italy", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sd_IN, "sd_IN", "sd", "Sindhi language locale for India", "Sindhi", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::se_NO, "se_NO", "se", "Northern Saami language locale for Norway", "Northern Saami language locale for Norway", "Norway", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::si_LK, "si_LK", "si", "Sinhala language locale for Sri Lanka", "Sinhala", "Sri Lanka", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sk_SK, "sk_SK", "sk", "Slovak locale for Slovak", "Slovak", "Slovak", "ISO-8859-2", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sl_SI, "sl_SI", "sl", "Slovenian locale for Slovenia", "Slovenian", "Slovenia", "ISO-8859-2", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::so_DJ, "so_DJ", "so", "Somali language locale for Djibouti.", "Somali", "Djibouti", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::so_ET, "so_ET", "so", "Somali language locale for Ethiopia", "Somali", "Ethiopia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::so_KE, "so_KE", "so", "Somali language locale for Kenya", "Somali", "Kenya", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::so_SO, "so_SO", "so", "Somali language locale for Somalia", "Somali", "Somalia", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sq_AL, "sq_AL", "sq", "Albanian language locale for Albania", "Albanian", "Albania", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sq_MK, "sq_MK", "sq", "Albanian language locale for Macedonia", "Albanian", "Macedonia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sr_ME, "sr_ME", "sr", "Serbian locale for Montenegro", "Serbian", "Montenegro", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sr_RS, "sr_RS", "sr", "Serbian locale for Serbia", "Serbian", "Serbia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ss_ZA, "ss_ZA", "ss", "Swati locale for South Africa", "Swati", "South Africa", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::st_ZA, "st_ZA", "st", "Sotho locale for South Africa", "Sotho", "South Africa", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sv_FI, "sv_FI", "sv", "Swedish locale for Finland", "Swedish", "Finland", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sv_SE, "sv_SE", "sv", "Swedish locale for Sweden", "Swedish", "Sweden", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sw_KE, "sw_KE", "sw", "Swahili locale for Kenya", "Swahili", "Kenya", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::sw_TZ, "sw_TZ", "sw", "Swahili locale for Tanzania", "Swahili", "Tanzania", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ta_IN, "ta_IN", "ta", "Tamil language locale for India", "Tamil", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ta_LK, "ta_LK", "ta", "Tamil language locale for Sri Lanka", "Tamil", "Sri Lanka", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::te_IN, "te_IN", "te", "Telugu language locale for India", "Telugu", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::tg_TJ, "tg_TJ", "tg", "Tajik language locale for Tajikistan", "Tajik language locale for Tajikistan", "Tajikistan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::th_TH, "th_TH", "th", "Thai locale for Thailand", "Thai", "Thailand", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ti_ER, "ti_ER", "ti", "Tigrigna language locale for Eritrea.", "Tigrigna", "Eritrea", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ti_ET, "ti_ET", "ti", "Tigrigna language locale for Ethiopia.", "Tigrigna", "Ethiopia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::tk_TM, "tk_TM", "tk", "Turkmen locale for Turkmenistan", "Turkmen", "Turkmenistan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::tl_PH, "tl_PH", "tl", "Tagalog language locale for Philippines", "Tagalog", "Philippines", "ISO-8859-1", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::tn_ZA, "tn_ZA", "tn", "Tswana locale for South Africa", "Tswana", "South Africa", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::tr_CY, "tr_CY", "tr", "Turkish language locale for Cyprus", "Turkish language locale for Cyprus", "Cyprus", "ISO-8859-9", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::tr_TR, "tr_TR", "tr", "Turkish locale for Turkey", "Turkish", "Turkey", "ISO-8859-9", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ts_ZA, "ts_ZA", "ts", "Tsonga locale for South Africa", "Tsonga", "South Africa", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::tt_RU, "tt_RU", "tt", "Tatar language locale for Russia", "Tatar language locale for Russia", "Russia", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ug_CN, "ug_CN", "ug", "Uyghur locale for China", "Uyghur", "China", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::uk_UA, "uk_UA", "uk", "Ukrainian Language Locale for Ukraine", "Ukrainian", "Ukraine", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ur_IN, "ur_IN", "ur", "Urdu language locale for India", "Urdu", "India", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ur_PK, "ur_PK", "ur", "Urdu Language Locale for Pakistan", "Urdu", "Pakistan", "CP1256", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::uz_UZ, "uz_UZ", "uz", "Uzbek (latin) locale for Uzbekistan", "Uzbek", "Uzbekistan", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ve_ZA, "ve_ZA", "ve", "Venda locale for South Africa", "Venda", "South Africa", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::vi_VN, "vi_VN", "vi", "Vietnamese language locale for Vietnam", "Vietnamese", "Vietnam", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::wa_BE, "wa_BE", "wa", "Walloon Language Locale for Belgium", "Walloon", "Belgium", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::wo_SN, "wo_SN", "wo", "Wolof locale for Senegal", "Wolof", "Senegal", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::xh_ZA, "xh_ZA", "xh", "Xhosa locale for South Africa", "Xhosa", "South Africa", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::yi_US, "yi_US", "yi", "Yiddish Language locale for the USA", "Yiddish", "USA", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::yo_NG, "yo_NG", "yo", "Yoruba locale for Nigeria", "Yoruba", "Nigeria", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::zh_CN, "zh_CN", "zh", "Chinese locale for Peoples Republic of China", "Chinese", "P.R. of China", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::zh_HK, "zh_HK", "zh", "Chinese language locale for Hong Kong", "Chinese", "Hong Kong", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::zh_SG, "zh_SG", "zh", "Chinese language locale for Singapore", "Chinese", "Singapore", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::zh_TW, "zh_TW", "zh", "Chinese locale for Taiwan R.O.C.", "Chinese", "Taiwan R.O.C.", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::zu_ZA, "zu_ZA", "zu", "Zulu locale for South Africa", "Zulu", "South Africa", "UTF-8", "%Y/%m/%d %I:%M:%S %p", L"%Y/%m/%d %I:%M:%S %p", BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},

	// Locales for internal use
	{ ELocale::cloc,   "C",     "C",     "System default language locale", "GLibC",  "Internal",  "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::ploc,   "POSIX", "POSIX", "System default language locale", "POSIX",  "Internal",  "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},
	{ ELocale::siloc,  "SI",    "SI",    "Scientific language locale", "Scientific", "Universal", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, "1", "0", L"1", L"0"},
	{ ELocale::sysloc, "SYS",   "SYS",   "System default locale", "System", "Internal", "UTF-8", DEFAULT_LOCALE_TIME_FORMAT_A, DEFAULT_LOCALE_TIME_FORMAT_W, BOOLEAN_TRUE_NAME_A, BOOLEAN_FALSE_NAME_A, BOOLEAN_TRUE_NAME_W, BOOLEAN_FALSE_NAME_W},

	// End Of Table
	{ ELocale::nloc, nil, nil, nil, nil, nil, nil, nil, nil, nil }
};


} // namespace app

#endif /* LOCALIZATIONS_H_ */
