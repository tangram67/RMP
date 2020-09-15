/*
 * musicplayer.cpp
 *
 *  Created on: 11.09.2016
 *      Author: Dirk Brinkmeier
 */

#include "../inc/exception.h"
#include "../inc/fileutils.h"
#include "../inc/sysutils.h"
#include "../inc/alsa.h"
#include "../inc/process.h"
#include "../inc/compare.h"
#include "../inc/audioconsts.h"
#include "musicplayer.h"

namespace music {

TPlayerConfig::TPlayerConfig() {
	prime();
}

TPlayerConfig::TPlayerConfig(const std::string& configPath, const std::string& dataRootPath, const std::string& dataBasePath, const bool isDamonized) {
	read(configPath, dataRootPath, dataBasePath, isDamonized);
}

TPlayerConfig::~TPlayerConfig() {
}

void TPlayerConfig::prime() {
	m_debug = false;
	m_verbosity = 0;
	m_maxbuffersize = MAX_BUFFER_SIZE;
	m_minbuffersize = MIN_BUFFER_SIZE;
	m_maxbuffercount = MAX_BUFFER_COUNT;
	m_bufferthreshold = BUFFER_THRESHOLD_PERCENT;
	m_rebufferthreshold = REBUFFER_THRESHOLD_PERCENT;
	m_playlistsize = 3000;
	m_taskcycletime = 400;
	m_periodtime = 1000;
	m_skipframe = 8;
	m_fraction = 50;
	m_prebuffer = 6;
	m_alsatimeout = 240;
	m_ignoremixer = false;
	m_updatelibrary = false;
	m_usemetadata = true;
	c_damonized = false;
	m_allowGroupNameSwap = true;
	m_allowFullNameSwap = false;
	m_allowArtistNameRestore = true;
	m_allowTheBandPrefixSwap = true;
	m_allowDeepNameInspection = false;
	m_allowVariousArtistsRename = false;
	m_allowMovePreamble = false;
	m_watchLibraryEnabled = false;
	m_sortCaseSensitive = true;
	m_sortAlbumsByYear = false;
	m_displayOrchestra = true;
	m_displayRemain = false;
	m_diskmode = true;
	m_playsong = 0;
	m_resumedelay = 4;
	m_remotedelay = 30000;
	m_useremotedevice = false;
	m_userecontrolport = false;
	m_usercanonrate = true;
	m_displaylimit = 36;
	m_pagelimit = 10;
	m_dithered = false;
	m_path_enabled1 = true;
	m_path_enabled2 = false;
	m_path_enabled3 = false;
}

bool TPlayerConfig::doDebug(const int level) const {
	return (level >= m_verbosity) && m_debug && !c_damonized;
}

void TPlayerConfig::read(const std::string& configPath, const std::string& dataRootPath, const std::string& dataBasePath, const bool isDamonized) {
	c_damonized = isDamonized;
	c_path = util::validPath(configPath);
	c_database = util::validPath(dataBasePath);
	c_appdata = util::validPath(dataRootPath);
	c_file = c_path + "player.conf";
	config.open(c_file);
	reWriteConfig();
	getPlayerValues(running);
	getPlayerValues(configured);
}

void TPlayerConfig::readConfig() {
	config.setSection("Player");

	m_device = config.readString("Device", "?");
	m_ignoredevices = config.readString("IgnoreDeviceList", "CARD=PCH;CARD=NVidia;CARD=SB");
	if (m_device == "?")
		m_device = guessDeviceName();
	m_periodtime = config.readInteger("PeriodTime", m_periodtime);

	m_path = config.readString("MusicFolder", "");
	util::validPath(m_path);
	m_filepattern = config.readString("FileExtensions", "*.wav;*.flac;*.dsf;*.dff;*.aif;*.m4a;*.mp3");

	m_database = config.readString("DatabaseFileName", "");
	if (m_database.empty())
		m_database = c_database + "library.csv";

	m_stations = config.readString("StationFileName", "");
	if (m_stations.empty())
		m_stations = c_database + "stations.csv";

	// Set fixed state playlist name
	m_playlist = c_database + "state.pls";

	m_selected = config.readString("PlaylistFileName", "");
	if (m_selected.empty())
		m_selected = c_database + "selected.csv";

	m_updatelibrary = config.readBool("UpdateLibraryOnStartup", m_updatelibrary);
	m_usemetadata = config.readBool("UsePictureMetadata", m_usemetadata);
	m_ignoremixer = config.readBool("IgnoreMixer", m_ignoremixer);

	m_skipframe = config.readInteger("FastForwardFrameDuration", m_skipframe);
	m_dithered = config.readBool("UseDithering", true); // Default ist dithered upscaling output

	// Delete deprecated path entry
	config.deleteKey("MusicFolder");

	// Read libryrary path
	config.setSection("Library");
	m_path1 = config.readString("MusicFolder1", m_path);
	m_path_enabled1 = config.readBool("MusicFolderEnabled1", m_path_enabled1);
	m_path2 = config.readString("MusicFolder2", "");
	m_path_enabled2 = config.readBool("MusicFolderEnabled2", m_path_enabled2);
	m_path3 = config.readString("MusicFolder3", "");
	m_path_enabled3 = config.readBool("MusicFolderEnabled3", m_path_enabled3);

	// Geuess music path for first entry
	if (m_path1.empty())
		m_path1 = guessMusicPath();
	m_path = m_path1;

	// Validate path names
	util::validPath(m_path1);
	util::validPath(m_path2);
	util::validPath(m_path3);

	config.setSection("Buffering");
	m_fraction = config.readInteger("RelativeMemoryUsage", m_fraction);
	m_maxbuffersize = config.readSize("MaxBufferSize", m_maxbuffersize);
	m_minbuffersize = config.readSize("MinBufferSize", m_minbuffersize);
	m_maxbuffercount = config.readInteger("MaxBufferCount", m_maxbuffercount);
	m_bufferthreshold = config.readInteger("BufferPlayingThreshold", m_bufferthreshold);
	m_rebufferthreshold = config.readInteger("NextBufferThreshold", m_rebufferthreshold);
	m_alsatimeout = config.readInteger("DeviceShutdownDelay", m_alsatimeout);
	m_prebuffer = config.readInteger("PreloadBufferCount", m_prebuffer);
	if (m_prebuffer < 2)
		m_prebuffer = 2;
	if (m_alsatimeout != 0 && m_alsatimeout < 10)
		m_alsatimeout = 10;

	config.setSection("Logging");
	m_debug = config.readBool("Debug", false);
	m_verbosity = config.readInteger("Verbosity", 0);
	m_scandebug = config.readBool("ScannerCallback", false);

	config.setSection("Tagging");
	m_allowGroupNameSwap = config.readBool("AllowGroupNameSwap", m_allowGroupNameSwap);
	m_allowArtistNameRestore = config.readBool("AllowArtistNameRestore", m_allowArtistNameRestore);
	m_allowFullNameSwap = config.readBool("AllowFullNameSwap", m_allowFullNameSwap);
	m_allowTheBandPrefixSwap = config.readBool("AllowTheBandPrefixSwap", m_allowTheBandPrefixSwap);
	m_allowDeepNameInspection = config.readBool("AllowDeepNameInspection", m_allowDeepNameInspection);
	m_allowVariousArtistsRename = config.readBool("AllowVariousArtistsRename", m_allowVariousArtistsRename);
	m_allowMovePreamble = config.readBool("AllowMovePreamble", m_allowMovePreamble);
	m_variousArtistsCategories = config.readString("VariousArtistsCategories", "sampler;various;soundtrack;compilation;divers");

	config.setSection("Display");
	m_watchLibraryEnabled = config.readBool("WatchLibraryEnabled", m_watchLibraryEnabled);
	m_sortCaseSensitive = config.readBool("SortCaseSensitive", m_sortCaseSensitive);
	m_sortAlbumsByYear = config.readBool("SortAlbumsByYear", m_sortAlbumsByYear);
	m_displayOrchestra = config.readBool("DisplayOrchestra", m_displayOrchestra);
	m_displayRemain = config.readBool("DisplayRemainingTime", m_displayRemain);
	m_displaylimit = config.readInteger("DisplayCountLimit", m_displaylimit);
	m_pagelimit = config.readInteger("TablePageLimit", m_pagelimit);

	config.setSection("Control");
	m_controlfile = config.readString("ControlFile", c_appdata + ".remote");
	m_htmlroot = util::validPath(config.readString("HTMLRoot", "/"));
	m_playlistsize = config.readInteger("MaxPlaylistSize", m_playlistsize);
	m_resumedelay = config.readInteger("ResumeDelay", m_resumedelay);
	m_lircdevice = config.readString("LircDevice", "/run/lirc/lircd");
	m_uselircdevice = config.readBool("UseLircDevice", m_uselircdevice);
	m_blockinglist = config.readString("LircProcessBlockingList", "kodi;kodi.bin;xbmc;xbmc.bin;vlc");

	config.setSection("Remote");
	m_remotedevice = config.readString("Device", "?");
	m_remoteterminal = config.readString("Terminal", "/dev/null");
	m_userecontrolport = config.readBool("UseTerminalDevice", m_userecontrolport);
	m_useremotedevice = config.readBool("UseRemoteDevice", m_useremotedevice);
	m_usercanonrate = config.readBool("UseCanonicalSampleRate", m_usercanonrate);
	m_remotedelay = config.readInteger("DeviceShutdownDelay", m_remotedelay);

	config.setSection("Tasks");
	m_taskcycletime = config.readInteger("TaskCycleTime", m_taskcycletime);
	if (m_taskcycletime == 250)
		m_taskcycletime = 400;

	config.setSection("State");
	m_diskmode = config.readBool("DiskRepeatMode", m_diskmode);
	m_playsong = config.readInteger("CurrentPlayerSong", m_playsong);

	config.setSection("Coverart");
	m_covercache = config.readPath("CacheFolder", c_appdata + "cache/");
}

void TPlayerConfig::writeConfig() {
	config.setSection("Player");
	config.writeString("Device", m_device);
	config.writeString("IgnoreDeviceList", m_ignoredevices);
	config.writeInteger("PeriodTime", m_periodtime);
	config.writeString("FileExtensions", m_filepattern.asString(';', util::ET_EXPORT_TRIMMED));
	config.writeString("DatabaseFileName", m_database);
	config.writeString("StationFileName", m_stations);
	config.writeString("PlaylistFileName", m_selected);

	config.deleteKey("UpdateLibraryOnStartup1");
	config.writeBool("UpdateLibraryOnStartup", m_updatelibrary, app::INI_BLYES);
	config.writeBool("UsePictureMetadata", m_usemetadata, app::INI_BLYES);
	config.writeBool("IgnoreMixer", m_ignoremixer, app::INI_BLYES);
	config.writeInteger("FastForwardFrameDuration", m_skipframe);
	config.writeBool("UseDithering", m_dithered, app::INI_BLYES);

	config.deleteKey("StateFileName");

	// Read libryrary path
	config.setSection("Library");
	config.writePath("MusicFolder1", m_path1);
	config.writeBool("MusicFolderEnabled1", m_path_enabled1, app::INI_BLYES);
	config.writePath("MusicFolder2", m_path2);
	config.writeBool("MusicFolderEnabled2", m_path_enabled2, app::INI_BLYES);
	config.writePath("MusicFolder3", m_path3);
	config.writeBool("MusicFolderEnabled3", m_path_enabled3, app::INI_BLYES);

	config.setSection("Buffering");
	config.writeInteger("RelativeMemoryUsage", m_fraction);
	config.writeSize("MaxBufferSize", m_maxbuffersize);
	config.writeSize("MinBufferSize", m_minbuffersize);
	config.writeInteger("MaxBufferCount", m_maxbuffercount);
	config.writeInteger("BufferPlayingThreshold", m_bufferthreshold);
	config.writeInteger("NextBufferThreshold", m_rebufferthreshold);
	config.writeInteger("PreloadBufferCount", m_prebuffer);
	config.writeInteger("DeviceShutdownDelay", m_alsatimeout);
	config.deleteKey("ReplayBuffers");
	config.deleteKey("MinBufferCount");

	config.setSection("Logging");
	config.writeBool("Debug", m_debug, app::INI_BLYES);
	config.writeInteger("Verbosity", m_verbosity);
	config.writeBool("ScannerCallback", m_scandebug, app::INI_BLYES);

	config.setSection("Tagging");
	config.deleteKey("AllowArtistNameSwap");
	config.writeBool("AllowGroupNameSwap", m_allowGroupNameSwap, app::INI_BLYES);
	config.writeBool("AllowArtistNameRestore", m_allowArtistNameRestore, app::INI_BLYES);
	config.writeBool("AllowFullNameSwap", m_allowFullNameSwap, app::INI_BLYES);
	config.writeBool("AllowTheBandPrefixSwap", m_allowTheBandPrefixSwap, app::INI_BLYES);
	config.writeBool("AllowDeepNameInspection", m_allowDeepNameInspection, app::INI_BLYES);
	config.writeBool("AllowVariousArtistsRename", m_allowVariousArtistsRename, app::INI_BLYES);
	config.writeBool("AllowMovePreamble", m_allowMovePreamble, app::INI_BLYES);
	config.writeString("VariousArtistsCategories", m_variousArtistsCategories.asString(';', util::ET_EXPORT_TRIMMED));
	config.deleteKey("SortAlbumsByYear");

	config.setSection("Display");
	config.writeBool("WatchLibraryEnabled", m_watchLibraryEnabled, app::INI_BLYES);
	config.writeBool("SortCaseSensitive", m_sortCaseSensitive, app::INI_BLYES);
	config.writeBool("SortAlbumsByYear", m_sortAlbumsByYear, app::INI_BLYES);
	config.writeBool("DisplayOrchestra", m_displayOrchestra, app::INI_BLYES);
	config.writeBool("DisplayRemainingTime", m_displayRemain, app::INI_BLYES);
	config.writeInteger("DisplayCountLimit", m_displaylimit);
	config.writeInteger("TablePageLimit", m_pagelimit);

	config.setSection("Control");
	config.writeString("ControlFile", m_controlfile);
	config.writeString("HTMLRoot", m_htmlroot);
	config.writeInteger("MaxPlaylistSize", m_playlistsize);
	config.writeInteger("ResumeDelay", m_resumedelay);
	config.deleteKey("ResumeOnStartup");
	config.writeString("LircDevice", m_lircdevice);
	config.writeBool("UseLircDevice", m_uselircdevice, app::INI_BLYES);
	config.writeString("LircProcessBlockingList", m_blockinglist);
	config.deleteKey("UsePortraitOrientation");

	config.setSection("Remote");
	config.writeString("Device", m_remotedevice);
	config.writeString("Terminal", m_remoteterminal);
	config.writeBool("UseTerminalDevice", m_userecontrolport, app::INI_BLYES);
	config.writeBool("UseRemoteDevice", m_useremotedevice, app::INI_BLYES);
	config.writeBool("UseCanonicalSampleRate", m_usercanonrate, app::INI_BLYES);
	config.writeInteger("DeviceShutdownDelay", m_remotedelay);
	config.deleteKey("UseAdvancedConfig");

	config.setSection("Tasks");
	config.writeInteger("TaskCycleTime", m_taskcycletime);

	config.setSection("State");
	config.writeBool("DiskRepeatMode", m_diskmode, app::INI_BLON);
	config.writeInteger("CurrentPlayerSong", m_playsong);

	config.setSection("Coverart");
	config.writePath("CacheFolder", m_covercache);

	config.deleteSection("Network");

	// Save changes to disk
	config.flush();
}

void TPlayerConfig::reWriteConfig() {
	readConfig();
	writeConfig();
}


std::string TPlayerConfig::guessDeviceName() const {
	util::TStringList devices, ignore;
	ignore = m_ignoredevices;
	music::TAlsaPlayer::getAlsaHardwareDevices(devices, ignore);
	if (devices.empty()) {
		if (sysutil::getHostName(false) == "Inferno")
			return "hw:CARD=USBASIO,DEV=0";
		else
			return "hw:CARD=d2,DEV=0";
	} else {
		return devices[0];
	}
}

std::string TPlayerConfig::guessMusicPath() const {
	std::string path = "/mnt/DBHOMESRV/nfs/Musik/files/";
	if (!util::folderExists(path))
		path = "/media/musik/";
	if (!util::folderExists(path))
		path = "/data/musik/files/";
	if (!util::folderExists(path))
		path = "/mnt/Musik/files/";
	if (!util::folderExists(path))
		path = "/mnt/musik/files/";
	if (!util::folderExists(path))
		path = "/data/musik/";
	if (!util::folderExists(path))
		path = "~/Musik";
	if (!util::folderExists(path))
		path = "~/music";
	return path;
}

size_t TPlayerConfig::scanStorageMounts_A(util::TStringList& mounts) {
	mounts.clear();
	int result = 0;
	std::string output;
	std::string commandLine = "df -hT";
	bool r = util::executeCommandLine(commandLine, output, result);
	if (r && !output.empty()) {
		bool ok;
		std::string row, col;
		util::TStringList rows, cols;
		rows.assign(output, '\n');
		if (rows.size() > 1) {
			for (size_t i=1; i<rows.size(); ++i) {
				row = rows[i];
				util::split(row, cols);
				if (cols.size() >= 6) {
					col = cols[6];
					if (!col.empty()) {
						ok = col.size() > 1; // Ignore root "/" folder
						if (ok) ok = col[0] == '/'; // Is valid path?
						if (ok) ok = 0 != util::strncasecmp(col, "/dev", 4);
						if (ok) ok = 0 != util::strncasecmp(col, "/run", 4);
						if (ok) ok = 0 != util::strncasecmp(col, "/tmp", 4);
						if (ok) ok = !util::strstr(col, "/system");
						if (ok) ok = !util::strstr(col, "/cgroup");
						if (ok) ok = !util::strstr(col, "/efi");
						if (ok) ok = !util::strstr(col, "/shm");
						if (ok) ok = !util::strstr(col, "/aufs");
						if (ok) mounts.add(col);
					}
				}
			}
		}
	}
	if (!mounts.empty()) mounts.sort();
	return mounts.size();
}


size_t TPlayerConfig::scanStorageMounts(util::TStringList& mounts) {
	mounts.clear();
	util::getMountPoints(mounts, util::MT_DISK);
	if (!mounts.empty()) mounts.sort();
	return mounts.size();
}


bool TPlayerConfig::getMusicFolders(util::TStringList& folders) {
	std::lock_guard<std::mutex> lock(configMtx);
	folders.clear();
	if (m_path_enabled1 && !m_path1.empty())
		folders.add(m_path1);
	if (m_path_enabled2 && !m_path2.empty())
		folders.add(m_path2);
	if (m_path_enabled3 && !m_path3.empty())
		folders.add(m_path3);
	return !folders.empty();
}


void TPlayerConfig::getRunningValues(CConfigValues& values) {
	std::lock_guard<std::mutex> lock(configMtx);
	if (!running.valid) {
		getPlayerValues(values);
		running = values;
	} else {
		values = running;
	}
}

void TPlayerConfig::getConfiguredValues(CConfigValues& values) {
	std::lock_guard<std::mutex> lock(configMtx);
	if (!configured.valid) {
		getPlayerValues(values);
		configured = values;
	} else {
		values = configured;
	}
}

void TPlayerConfig::setConfiguredValues(const CConfigValues& values) {
	std::lock_guard<std::mutex> lock(configMtx);
	setPlayerValues(values);
	configured = values;
	configured.valid = true;
	configured.altered = running.altered = ((configured.device != running.device) || (configured.enableDithering != running.enableDithering));
	writeConfig();
}

void TPlayerConfig::commitConfiguration() {
	std::lock_guard<std::mutex> lock(configMtx);
	getPlayerValues(running);
	running.altered = false;
	configured.altered = false;
}


void TPlayerConfig::getPlayerValues(CConfigValues& values) {
	values.device = m_device;
	values.datapath = c_database;
	values.datafile = m_database;
	values.playlist = m_playlist;

	values.musicpath1 = m_path1;
	values.musicpath2 = m_path2;
	values.musicpath3 = m_path3;

	values.enableMusicPath1 = m_path_enabled1;
	values.enableMusicPath2 = m_path_enabled2;
	values.enableMusicPath3 = m_path_enabled3;

	values.pattern = m_filepattern;
	values.categories = m_variousArtistsCategories;

	values.allowGroupNameSwap = m_allowGroupNameSwap;
	values.allowArtistNameRestore = m_allowArtistNameRestore;
	values.allowFullNameSwap = m_allowFullNameSwap;
	values.allowTheBandPrefixSwap = m_allowTheBandPrefixSwap;
	values.allowDeepNameInspection = m_allowDeepNameInspection;
	values.allowVariousArtistsRename = m_allowVariousArtistsRename;
	values.allowMovePreamble = m_allowMovePreamble;

	values.watchLibraryEnabled = m_watchLibraryEnabled;
	values.sortCaseSensitive = m_sortCaseSensitive;
	values.sortAlbumsByYear = m_sortAlbumsByYear;
	values.displayOrchestra = m_displayOrchestra;
	values.displayRemain = m_displayRemain;
	values.displayLimit = m_displaylimit;
	values.pageLimit = m_pagelimit;
	values.enableDithering = m_dithered;
	values.periodTime = m_periodtime;

	values.valid = true;
}

void TPlayerConfig::setPlayerValues(const CConfigValues& values) {
	m_device = values.device;
	c_database = values.datapath;
	m_database = values.datafile;
	m_playlist = values.playlist;

	m_path1 = values.musicpath1;
	m_path2 = values.musicpath2;
	m_path3 = values.musicpath3;

	m_path_enabled1 = values.enableMusicPath1;
	m_path_enabled2 = values.enableMusicPath2;
	m_path_enabled3 = values.enableMusicPath3;

	m_filepattern = values.pattern;
	m_variousArtistsCategories = values.categories;

	m_allowGroupNameSwap = values.allowGroupNameSwap;
	m_allowArtistNameRestore = values.allowArtistNameRestore;
	m_allowFullNameSwap = values.allowFullNameSwap;
	m_allowTheBandPrefixSwap = values.allowTheBandPrefixSwap;
	m_allowDeepNameInspection = values.allowDeepNameInspection;
	m_allowVariousArtistsRename = values.allowVariousArtistsRename;
	m_allowMovePreamble = values.allowMovePreamble;

	m_watchLibraryEnabled = values.watchLibraryEnabled;
	m_sortCaseSensitive = values.sortCaseSensitive;
	m_sortAlbumsByYear = values.sortAlbumsByYear;
	m_displayOrchestra = values.displayOrchestra;
	m_displayRemain = values.displayRemain;
	m_displaylimit = values.displayLimit;
	m_pagelimit = values.pageLimit;
	m_dithered = values.enableDithering;
	m_periodtime = values.periodTime;
}


void TPlayerConfig::getRemoteValues(CRemoteValues& values) {
	std::lock_guard<std::mutex> lock(configMtx);
	values.device = m_remotedevice;
	values.terminal = m_remoteterminal;
	values.delay = m_remotedelay;
	values.canonical = m_usercanonrate;
	values.enabled = m_useremotedevice;
	values.control = m_userecontrolport;
}

void TPlayerConfig::setRemoteValues(const CRemoteValues& values) {
	std::lock_guard<std::mutex> lock(configMtx);
	m_remotedevice = values.device;
	m_remoteterminal = values.terminal;
	m_remotedelay = values.delay;
	m_usercanonrate = values.canonical;
	m_useremotedevice = values.enabled;
	m_userecontrolport = values.control;
}

void TPlayerConfig::getAudioValues(CAudioValues& values) {
	std::lock_guard<std::mutex> lock(configMtx);
	values.device = m_device;
	values.remote = m_remotedevice;
	values.terminal = m_remoteterminal;
	values.canonical = m_usercanonrate;
	values.enabled = m_useremotedevice;
	values.control = m_userecontrolport;
	values.period = m_periodtime;
}

bool TPlayerConfig::hasOptionsChanged() {
	std::lock_guard<std::mutex> lock(configMtx);
	return ((configured.allowGroupNameSwap != running.allowGroupNameSwap) ||
			(configured.allowArtistNameRestore != running.allowArtistNameRestore) ||
			(configured.allowFullNameSwap != running.allowFullNameSwap) ||
			(configured.allowTheBandPrefixSwap != running.allowTheBandPrefixSwap) ||
			(configured.allowDeepNameInspection != running.allowDeepNameInspection) ||
			(configured.allowVariousArtistsRename != running.allowVariousArtistsRename));
}

void TPlayerConfig::getControlConfig(TControConfig& config) {
	std::lock_guard<std::mutex> lock(configMtx);
	config.device = m_lircdevice;
	config.enabled = m_uselircdevice;
	config.blockingProcessList = m_blockinglist;
}

void TPlayerConfig::getAlsaConfig(TAlsaConfig& config) {
	std::lock_guard<std::mutex> lock(configMtx);
	config.periodtime = getPeriodTime();
	config.skipframe = getSkipFrame();
	config.verbosity = getVerbosity();
	config.dithered = getDithered();
	config.debug = getDebug();
	config.ignoremixer = getIgnoreMixer();
	config.logger = nil;
}

void TPlayerConfig::getLibraryConfig(TLibraryConfig& config) {
	std::lock_guard<std::mutex> lock(configMtx);
	config.debug = getScannerDebug();
	config.documentRoot = getHTMLRoot();
	config.allowFullNameSwap = getFullNameSwap();
	config.allowGroupNameSwap = getGroupNameSwap();
	config.allowArtistNameRestore = getArtistNameRestore();
	config.allowTheBandPrefixSwap = getTheBandPrefixSwap();
	config.allowDeepNameInspection = getDeepNameInspection();
	config.allowVariousArtistsRename = getVariousArtistsRename();
	config.allowMovePreamble = getMovePreamble();
	config.sortCaseSensitive = doSortCaseSensitive();
	config.logger = nil;
}


} /* namespace music */
