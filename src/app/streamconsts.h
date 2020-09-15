/*
 * streamconsts.h
 *
 *  Created on: 01.12.2019
 *      Author: dirk
 */

#ifndef APP_STREAMCONSTS_H_
#define APP_STREAMCONSTS_H_

#include "../inc/nullptr.h"

namespace radio {

typedef struct CRadioStreamNames {
	const char* name;
	const char* url;
} TRadioStreamNames;

static const struct CRadioStreamNames radiostreams[] =
{
		{ "1LIVE", "http://wdr-1live-live.icecast.wdr.de/wdr/1live/live/mp3/128/stream.mp3" },
		{ "WDR 2 - Ostwestfalen Lippe", "http://wdr-wdr2-ostwestfalenlippe.icecast.wdr.de/wdr/wdr2/ostwestfalenlippe/mp3/128/stream.mp3" },
		{ "WDR 3 (256k)", "http://www.wdr.de/wdrlive/media/wdr3_hq.m3u" },
		{ "WDR 3", "http://wdr-wdr3-live.icecast.wdr.de/wdr/wdr3/live/mp3/128/stream.mp3" },
		{ "WDR 4", "http://wdr-wdr4-live.icecast.wdr.de/wdr/wdr4/live/mp3/128/stream.mp3" },
		{ "WDR 5", "http://wdr-wdr5-live.icecast.wdr.de/wdr/wdr5/live/mp3/128/stream.mp3" },
		{ "NDR 1", "http://www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u" },
		{ "NDR 2", "http://www.ndr.de/resources/metadaten/audio/m3u/ndr2.m3u" },
		{ "SWR 3 (256k)", "http://mp3-live.swr.de/swr2_m.m3u" },
		{ "MDR Klassik", "http://avw.mdr.de/streams/284350-0_mp3_high.m3u" },
		{ "Deutschlandfunk", "http://www.dradio.de/streaming/dlf.m3u" },
		{ "Deutschlandradio Wissen", "http://www.dradio.de/streaming/dradiowissen.m3u" },
		{ "Klassik Radio (192k)", "http://klassikr.streamabc.net/klassikradio-simulcast-mp3-hq" },
		{ "Linn Classical (320k)", "http://89.16.185.174:8004/autodj.m3u" },
		{ "Linn Jazz (320k)", "http://89.16.185.174:8000/autodj.m3u" },
		{ "Schwarzwaldradio", "http://str31.creacast.com:80/hitradio_ohr_thema1" },
		{ nil, nil }
};

} /* namespace radio */

#endif /* APP_STREAMCONSTS_H_ */
