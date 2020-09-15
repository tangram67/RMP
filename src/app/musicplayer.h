/*
 * musicplayer.h
 *
 *  Created on: 11.09.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef MUSICPLAYER_H_
#define MUSICPLAYER_H_

#include <string>
#include <mutex>
#include "../inc/inifile.h"
#include "../inc/datetime.h"
#include "../inc/alsatypes.h"
#include "../inc/audiotypes.h"
#include "../inc/stringutils.h"
#include "librarytypes.h"
#include "musictypes.h"

namespace music {

class TPlayerConfig {
private:
	std::mutex configMtx;
	app::TIniFile config;

	CConfigValues running;
	CConfigValues configured;

	std::string m_device;
	std::string m_path;
	std::string m_path1;
	std::string m_path2;
	std::string m_path3;
	std::string m_database;
	std::string m_stations;
	std::string m_covercache;
	std::string m_playlist;
	std::string m_selected;
	std::string m_ignoredevices;
	size_t m_playlistsize;
	bool m_dithered;
	bool m_debug;
	bool m_scandebug;
	int m_verbosity;
	bool m_updatelibrary;
	bool m_ignoremixer;
	bool m_usemetadata;
	bool m_path_enabled1;
	bool m_path_enabled2;
	bool m_path_enabled3;
	util::TTimePart m_resumedelay;
	size_t m_maxbuffersize;
	size_t m_minbuffersize;
	size_t m_maxbuffercount;
	size_t m_bufferthreshold;
	size_t m_rebufferthreshold;
	size_t m_fraction;
	size_t m_prebuffer;
	size_t m_displaylimit;
	size_t m_pagelimit;
	util::TStringList m_filepattern;
	util::TTimePart m_alsatimeout;
	util::TTimePart m_skipframe;
	snd_pcm_uint_t m_periodtime;

	bool m_allowGroupNameSwap;
	bool m_allowArtistNameRestore;
	bool m_allowFullNameSwap;
	bool m_allowTheBandPrefixSwap;
	bool m_allowDeepNameInspection;
	bool m_allowVariousArtistsRename;
	bool m_allowMovePreamble;
	bool m_watchLibraryEnabled;
	bool m_sortCaseSensitive;
	bool m_sortAlbumsByYear;
	bool m_displayOrchestra;
	bool m_displayRemain;
	util::TStringList m_variousArtistsCategories;

	std::string m_controlfile;
	std::string m_htmlroot;
	util::TTimePart m_taskcycletime;

	std::string m_remotedevice;
	std::string m_remoteterminal;
	std::string m_lircdevice;
	std::string m_blockinglist;
	util::TTimePart m_remotedelay;
	bool m_useremotedevice;
	bool m_userecontrolport;
	bool m_usercanonrate;
	bool m_uselircdevice;

	bool m_diskmode;
	size_t m_playsong;

	std::string c_path;
	std::string c_database;
	std::string c_appdata;
	std::string c_file;
	bool c_damonized;

	void prime();

	void readConfig();
	void writeConfig();
	void reWriteConfig();

	void getPlayerValues(CConfigValues& values);
	void setPlayerValues(const CConfigValues& values);

	std::string guessDeviceName() const;
	std::string guessMusicPath() const;

public:
	void read(const std::string& configPath, const std::string& dataRootPath, const std::string& dataBasePath, const bool isDamonized);
	void write() { writeConfig(); };

	const std::string& getDeviceName() const { return m_device; };
	const std::string& getIgnoreDevices() const { return m_ignoredevices; };

	bool getMusicFolders(util::TStringList& folders);
	const std::string& getMusicPath1() const { return m_path1; };
	const std::string& getMusicPath2() const { return m_path2; };
	const std::string& getMusicPath3() const { return m_path3; };
	const std::string& getCachePath() const { return m_covercache; };
	const std::string& getDataRootPath() const { return c_appdata; };
	const std::string& getDataBasePath() const { return c_database; };
	const std::string& getDataBaseFile() const { return m_database; };
	const std::string& getStationsFile() const { return m_stations; };
	const std::string& getPlaylistFile() const { return m_playlist; };
	const std::string& getSelectedFile() const { return m_selected; };
	util::TTimePart getResumeDelay() const { return m_resumedelay; };
	bool doResume() const { return m_resumedelay > (util::TTimePart)0; };

	const std::string& getRemoteDevice() const { return m_remotedevice; };
	util::TTimePart getRemoteDelay() const { return m_remotedelay; };
	bool useRemoteDevice() const { return m_useremotedevice; };

	size_t getMaxBufferSize() const { return m_maxbuffersize; };
	size_t getMinBufferSize() const { return m_minbuffersize; };
	size_t getMaxBufferCount() const { return m_maxbuffercount; };
	size_t getMemoryFraction() const { return m_fraction; };
	size_t getBufferThreshold() const { return m_bufferthreshold; };
	size_t getRebufferThreshold() const { return m_rebufferthreshold; };
	size_t getPreBufferCount() const { return m_prebuffer; };
	util::TTimePart getAlsaTimeout() const { return m_alsatimeout; };
	bool getDithered() const { return m_dithered; };

	snd_pcm_uint_t getPeriodTime() const { return m_periodtime; };
	util::TTimePart getTaskCycleTime() const { return m_taskcycletime; };
	bool getIgnoreMixer() const { return m_ignoremixer; };

	bool doDebug(const int level) const;
	int getVerbosity() const { return m_verbosity; };
	bool getDebug() const { return c_damonized ? false : m_debug; };
	bool getUpdateLibrary() const { return m_updatelibrary; };
	bool getUseMetadata() const { return m_usemetadata; };
	size_t getPlaylistSize() const { return m_playlistsize; };

	bool getScannerDebug() const { return m_scandebug; };
	bool getGroupNameSwap() const { return m_allowGroupNameSwap; };
	bool getArtistNameRestore() const { return m_allowArtistNameRestore; };
	bool getFullNameSwap() const { return m_allowFullNameSwap; };
	bool getTheBandPrefixSwap() const { return m_allowTheBandPrefixSwap; };
	bool getDeepNameInspection() const { return m_allowDeepNameInspection; };
	bool getVariousArtistsRename() const { return m_allowVariousArtistsRename; };
	bool getMovePreamble() const { return m_allowMovePreamble; };

	bool doLibraryWatch() const { return m_watchLibraryEnabled; };
	bool doSortCaseSensitive() const { return m_sortCaseSensitive; };
	bool doSortByYear() const { return m_sortAlbumsByYear; };

	bool getDiskMode() const { return m_diskmode; };
	void setDiskMode(const bool value) { m_diskmode = value; };
	size_t getPlaySong() const { return m_playsong; };
	void setPlaySong(const size_t value) { m_playsong = value; };

	const std::string& getControlFile() const { return m_controlfile; };
	const std::string& getHTMLRoot() const { return m_htmlroot; };
	const util::TStringList& getFilePattern() const { return m_filepattern; };
	util::TTimePart getSkipFrame() const { return m_skipframe; };

	bool hasOptionsChanged();
	void getRunningValues(CConfigValues& values);
	void getConfiguredValues(CConfigValues& values);
	void setConfiguredValues(const CConfigValues& values);
	void getRemoteValues(CRemoteValues& values);
	void setRemoteValues(const CRemoteValues& values);
	void getAudioValues(CAudioValues& values);
	void commitConfiguration();

	void getControlConfig(TControConfig& config);
	void getAlsaConfig(TAlsaConfig& config);
	void getLibraryConfig(TLibraryConfig& config);

	size_t scanStorageMounts_A(util::TStringList& mounts);
	size_t scanStorageMounts(util::TStringList& mounts);

	TPlayerConfig();
	TPlayerConfig(const std::string& configPath, const std::string& dataRootPath, const std::string& dataBasePath, const bool isDamonized);
	virtual ~TPlayerConfig();
};

} /* namespace music */

#endif /* MUSICPLAYER_H_ */
