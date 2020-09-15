/*
 * alsatypes.h
 *
 *  Created on: 29.04.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef ALSATYPES_H_
#define ALSATYPES_H_

#include "audiotypes.h"
#include "datetime.h"
#include "logger.h"

namespace music {

typedef struct CAlsaConfig {
	app::PLogFile logger;
	snd_pcm_uint_t periodtime;
	util::TTimePart skipframe;
	int verbosity;
	bool debug;
	bool dithered;
	bool ignoremixer;

	void clear() {
		logger = nil;
		periodtime= 0;
		skipframe = 8; // 8 seconds timebase for fast forward/rewind
		verbosity = 0;
		debug = false;
		dithered = false;
		ignoremixer = false;
	}

	CAlsaConfig() {
		clear();
	}
} TAlsaConfig;

} /* namespace music */

#endif /* ALSATYPES_H_ */
