/*
 * upnpconsts.h
 *
 *  Created on: 04.02.2019
 *      Author: dirk
 */

#ifndef APP_UPNPCONSTS_H_
#define APP_UPNPCONSTS_H_

#include "../inc/gcc.h"
#include "../inc/timer.h"

namespace upnp {

STATIC_CONST app::TTimerDelay MESSAGE_EXPIRE_TIME = 1800; // UPnP Expire time in seconds
STATIC_CONST app::TTimerDelay BROADCAST_DELAY = MESSAGE_EXPIRE_TIME * 1000 / 3; // Broadcast delay in milliseconds
STATIC_CONST app::TTimerDelay SEND_DELAY = 150; // Send delay in milliseconds

#define DLNA_FLAG_DLNA_V1_5      0x00100000
#define DLNA_FLAG_HTTP_STALLING  0x00200000
#define DLNA_FLAG_TM_B           0x00400000
#define DLNA_FLAG_TM_I           0x00800000
#define DLNA_FLAG_TM_S           0x01000000
#define DLNA_FLAG_LOP_BYTES      0x20000000
#define DLNA_FLAG_LOP_NPT        0x40000000

#define UPNP_EVENTED (1 << 7)

#define UPNP_RESOURCE_PROTOCOL_BASIC_VALUES \
		"http-get:*:image/png:*," \
		"http-get:*:image/jpeg:*," \
		"http-get:*:audio/mp4:*," \
		"http-get:*:audio/x-wav:*," \
		"http-get:*:audio/x-flac:*," \
		"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3"

#define UPNP_RESOURCE_PROTOCOL_MULTIMEDIA_VALUES \
		"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_TN," \
		"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_SM," \
		"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_MED," \
		"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_LRG," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_50_AC3_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_60_AC3_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HP_HD_AC3_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_HD_AAC_MULT5_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_HD_AC3_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_HD_MPEG1_L3_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_SD_AAC_MULT5_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_SD_AC3_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_SD_MPEG1_L3_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_NTSC," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_PAL," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_HD_NA_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_SD_NA_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_SD_EU_ISO," \
		"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG1," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_SD_AAC_MULT5," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_SD_AC3," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_CIF15_AAC_520," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_CIF30_AAC_940," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_L31_HD_AAC," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_L32_HD_AAC," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_L3L_SD_AAC," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_HP_HD_AAC," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_HD_1080i_AAC," \
		"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_HD_720p_AAC," \
		"http-get:*:video/mp4:DLNA.ORG_PN=MPEG4_P2_MP4_ASP_AAC," \
		"http-get:*:video/mp4:DLNA.ORG_PN=MPEG4_P2_MP4_SP_VGA_AAC," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_50_AC3," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_50_AC3_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_60_AC3," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_60_AC3_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HP_HD_AC3_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_AAC_MULT5," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_AAC_MULT5_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_AC3," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_AC3_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_MPEG1_L3," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_MPEG1_L3_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_AAC_MULT5," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_AAC_MULT5_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_AC3," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_AC3_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_MPEG1_L3," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_MPEG1_L3_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_HD_NA," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_HD_NA_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_EU," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_EU_T," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_NA," \
		"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_NA_T," \
		"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVSPLL_BASE," \
		"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVSPML_BASE," \
		"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVSPML_MP3," \
		"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVMED_BASE," \
		"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVMED_FULL," \
		"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVMED_PRO," \
		"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVHIGH_FULL," \
		"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVHIGH_PRO," \
		"http-get:*:video/3gpp:DLNA.ORG_PN=MPEG4_P2_3GPP_SP_L0B_AAC," \
		"http-get:*:video/3gpp:DLNA.ORG_PN=MPEG4_P2_3GPP_SP_L0B_AMR," \
		"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMABASE," \
		"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMAFULL," \
		"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMAPRO," \
		"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMALSL," \
		"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMALSL_MULT5," \
		"http-get:*:audio/mp4:DLNA.ORG_PN=AAC_ISO_320," \
		"http-get:*:audio/3gpp:DLNA.ORG_PN=AAC_ISO_320," \
		"http-get:*:audio/mp4:DLNA.ORG_PN=AAC_ISO," \
		"http-get:*:audio/mp4:DLNA.ORG_PN=AAC_MULT5_ISO," \
		"http-get:*:audio/L16;rate=44100;channels=2:DLNA.ORG_PN=LPCM," \
		"http-get:*:video/avi:*," \
		"http-get:*:video/divx:*," \
		"http-get:*:video/x-matroska:*," \
		"http-get:*:video/mpeg:*," \
		"http-get:*:video/mp4:*," \
		"http-get:*:video/x-ms-wmv:*," \
		"http-get:*:video/x-msvideo:*," \
		"http-get:*:video/x-flv:*," \
		"http-get:*:video/x-tivo-mpeg:*," \
		"http-get:*:video/quicktime:*," \
		"http-get:*:application/ogg:*" \
		UPNP_RESOURCE_PROTOCOL_BASIC_VALUES

// No multimedia file types supported for now
#define UPNP_RESOURCE_PROTOCOL_INFO_VALUES UPNP_RESOURCE_PROTOCOL_BASIC_VALUES

}

#endif /* APP_UPNPCONSTS_H_ */
