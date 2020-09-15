/*
 * musictypes.h
 *
 *  Created on: 13.04.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef MUSICTYPES_H_
#define MUSICTYPES_H_

#include "../inc/stringutils.h"

namespace music {

struct CConfigValues {
	size_t pageLimit;
	size_t displayLimit;
	snd_pcm_uint_t periodTime;

	std::string device;
	std::string datapath;
	std::string datafile;
	std::string playlist;

	std::string musicpath1;
	std::string musicpath2;
	std::string musicpath3;

	bool enableMusicPath1;
	bool enableMusicPath2;
	bool enableMusicPath3;

	util::TStringList pattern;
	util::TStringList categories;

	bool allowGroupNameSwap;
	bool allowArtistNameRestore;
	bool allowFullNameSwap;
	bool allowTheBandPrefixSwap;
	bool allowDeepNameInspection;
	bool allowVariousArtistsRename;
	bool allowMovePreamble;
	bool watchLibraryEnabled;
	bool sortCaseSensitive;
	bool sortAlbumsByYear;
	bool displayOrchestra;
	bool displayRemain;
	bool enableDithering;

	bool valid;
	bool altered;

	void prime() {
		valid = false;
		altered = false;
		enableMusicPath1 = true;
		enableMusicPath2 = false;
		enableMusicPath3 = false;
		allowGroupNameSwap = false;
		allowArtistNameRestore = false;
		allowFullNameSwap = false;
		allowTheBandPrefixSwap = false;
		allowDeepNameInspection = false;
		allowVariousArtistsRename = false;
		allowMovePreamble = false;
		watchLibraryEnabled = false;
		sortCaseSensitive = true;
		sortAlbumsByYear = false;
		displayOrchestra = true;
		displayRemain = false;
		enableDithering = false;
		periodTime = 1000;
		displayLimit = 36;
		pageLimit = 10;
	}

	void clear() {
		prime();
		device.clear();
		musicpath1.clear();
		musicpath2.clear();
		musicpath3.clear();
		datapath.clear();
		datafile.clear();
		playlist.clear();
		pattern.clear();
		categories.clear();
	}

	CConfigValues& operator = (const CConfigValues &value) {
		valid = value.valid;
		altered = value.altered;
		allowGroupNameSwap = value.allowGroupNameSwap;
		allowArtistNameRestore = value.allowArtistNameRestore;
		allowFullNameSwap = value.allowFullNameSwap;
		allowTheBandPrefixSwap = value.allowTheBandPrefixSwap;
		allowDeepNameInspection = value.allowDeepNameInspection;
		allowVariousArtistsRename = value.allowVariousArtistsRename;
		allowMovePreamble = value.allowMovePreamble;
		watchLibraryEnabled = value.watchLibraryEnabled;
		sortCaseSensitive = value.sortCaseSensitive;
		sortAlbumsByYear = value.sortAlbumsByYear;
		displayOrchestra = value.displayOrchestra;
		displayRemain = value.displayRemain;
		displayLimit = value.displayLimit;
		pageLimit = value.pageLimit;
		periodTime = value.periodTime;
		enableDithering = value.enableDithering;
		device = value.device;
		musicpath1 = value.musicpath1;
		musicpath2 = value.musicpath2;
		musicpath3 = value.musicpath3;
		enableMusicPath1 = value.enableMusicPath1;
		enableMusicPath2 = value.enableMusicPath2;
		enableMusicPath3 = value.enableMusicPath3;
		datapath = value.datapath;
		datafile = value.datafile;
		playlist = value.playlist;
		pattern = value.pattern;
		categories = value.categories;
		return *this;
	}

	CConfigValues() {
		prime();
	}
};

struct CRemoteValues {
	std::string device;
	std::string terminal;
	util::TTimePart delay;
	bool canonical;
	bool enabled;
	bool control;

	void prime() {
		delay = (util::TTimePart)0;
		canonical = true;
		enabled = false;
		control = false;
	}

	void clear() {
		prime();
		device.clear();
		terminal.clear();
	}

	CRemoteValues& operator = (const CRemoteValues &value) {
		device = value.device;
		terminal = value.terminal;
		delay = value.delay;
		canonical = value.canonical;
		enabled = value.enabled;
		control = value.control;
		return *this;
	}

	CRemoteValues() {
		prime();
	}
};

struct CAudioValues {
	std::string device;
	std::string remote;
	std::string terminal;
	snd_pcm_uint_t period;
	bool canonical;
	bool enabled;
	bool control;

	void prime() {
		canonical = true;
		enabled = false;
		control = false;
		period  = 1000;
	}

	void clear() {
		prime();
		device.clear();
		remote.clear();
		terminal.clear();
	}

	CAudioValues& operator = (const CAudioValues &value) {
		period = value.period;
		device = value.device;
		remote = value.remote;
		terminal = value.terminal;
		canonical = value.canonical;
		enabled = value.enabled;
		control = value.control;
		return *this;
	}

	CAudioValues() {
		prime();
	}
};

typedef struct CControConfig {
	std::string device;
	bool enabled;
	std::string blockingProcessList;

	void prime() {
		enabled = false;
	}

	void clear() {
		prime();
		device.clear();
		blockingProcessList.clear();
	}

	CControConfig& operator = (const CControConfig &value) {
		device = value.device;
		enabled = value.enabled;
		blockingProcessList = value.blockingProcessList;
		return *this;
	}

	CControConfig() {
		prime();
	}
} TControConfig;

} /* namespace music */

#endif /* MUSICTYPES_H_ */
