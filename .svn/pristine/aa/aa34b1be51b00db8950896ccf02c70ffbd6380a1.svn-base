/*
 * librarytypes.h
 *
 *  Created on: 29.04.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef LIBRARYTYPES_H_
#define LIBRARYTYPES_H_

#include "../inc/logger.h"

namespace music {

typedef struct CLibraryConfig {
	app::PLogFile logger;
	bool debug;
	std::string documentRoot;
	bool allowGroupNameSwap;
	bool allowArtistNameRestore;
	bool allowFullNameSwap;
	bool allowTheBandPrefixSwap;
	bool allowDeepNameInspection;
	bool allowVariousArtistsRename;
	bool allowMovePreamble;
	bool sortCaseSensitive;

	void clear() {
		logger = nil;
		if (!documentRoot.empty()) documentRoot.clear();
		allowGroupNameSwap = false;
		allowArtistNameRestore = false;
		allowFullNameSwap = false;
		allowTheBandPrefixSwap = false;
		allowDeepNameInspection = false;
		allowVariousArtistsRename = false;
		allowMovePreamble = false;
		sortCaseSensitive = true;
	}

	CLibraryConfig() {
		clear();
	}
} TLibraryConfig;

} /* namespace music */




#endif /* LIBRARYTYPES_H_ */
