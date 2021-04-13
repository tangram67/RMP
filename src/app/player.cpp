/*
 * player.cpp
 *
 *  Created on: 17.08.2014
 *      Author: Dirk Brinkmeier
 *
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mount.h>
#include "player.h"
#include "musictypes.h"
#include "../config.h"
#include "../inc/globals.h"
#include "../inc/sysutils.h"
#include "../inc/application.h"
#include "../inc/htmlconsts.h"
#include "../inc/audiotypes.h"
#include "../inc/audiofile.h"
#include "../inc/htmlutils.h"
#include "../inc/encoding.h"
#include "../inc/database.h"
#include "../inc/compare.h"
#include "../inc/random.h"
#include "../inc/typeid.h"
#include "../inc/flac.h"
#include "storeconsts.h"
#include "upnp.h"


using namespace std;
using namespace util;
using namespace html;
using namespace app;


// Internet radio stream thread dispatcher
static void* streamThreadDispatcher(void *thread) {
	if (util::assigned(thread)) {
		return (void *)(long)(static_cast<app::TPlayer*>(thread))->streamThreadHandler();
	} else {
		return (void *)(long)(EXIT_FAILURE);
	}
}


/*
 * Main application class
 *
 *  Created on: 02.9.2014
 *      Author: Dirk Brinkmeier
 */
TPlayer::TPlayer() {
	name = util::nameOf(this);
	toRemoteCommand = nil;
	toBuffering = nil;
	toRemote = nil;
	toReopen = nil;
	toHotplug = nil;
	toFileWatch = nil;
	currentState = music::EPS_DEFAULT;
	currentInput = app::ECA_INVALID_ACTION;
	reopenDelay = FORCED_REOPEN_DELAY + util::now();
	remoteDeviceMessage = false;
	remoteCommandThread = nil;
	contentChanged = false;
	contentCompare = false;
	devicesValid = false;
	drivesValid = false;
	portsValid = false;
	lircBlocked = false;
	terminate = false;
	cleared = false;
	debug = false;
	server4 = nil;
	server6 = nil;
	toLircTimeout = nil;
	lircAction = ECA_INVALID_ACTION;
	songDisplayChanged = 0;
	libraryScannerUpdate = 0;
	streamingAllowed = false;
	advancedSettingsAllowed = false;
	mnames = fillMediaNamesMap();
	fdnames = fillFilterDomainMap();
	libraryThread = nil;
	libraryThreadRunning = false;
	libraryThreadActive = false;
#ifdef USE_APPLICATION_AS_OUTPUT
	app::ansi.disable();
//	app::red.disable();
//	app::green.disable();
//	app::yellow.disable();
//	app::blue.disable();
//	app::magenta.disable();
//	app::cyan.disable();
//	app::white.disable();
//	app::reset.disable();
#endif
}

TPlayer::~TPlayer() {
	freeLocalFileCache();
}


int TPlayer::execute() {

	// Initialize some default values...
	cdArtistItem = nil;
	playlistSelectItem = nil;
	wtScannerStatusCaption = nil;
	wtScannerStatusColor = nil;
	wtActiveLicenses = nil;
	wtPlaylistHeader = nil;
	wtTableRowCount = nil;
	wtRecentHeader = nil;
	wtArtistCount = nil;

	// Remove development files...
	removeDevelFiles();

	// Bind signal handlers to event routines
	// --> e.g. SIGINT is redirected to application method
	application.bindSigintHandler (&app::TPlayer::onSigint,  this);
	application.bindSigtermHandler(&app::TPlayer::onSigterm, this);
	application.bindSighupHandler (&app::TPlayer::onSighup,  this);
	application.bindSigusr1Handler(&app::TPlayer::onSigusr1, this);
	application.bindSigusr2Handler(&app::TPlayer::onSigusr2, this);

	// Bind hotplug and folder watch events
	application.bindHotplugHandler(&app::TPlayer::onHotplugEvent, this);
	application.addWatchHandler(&app::TPlayer::onWatchEvent, this);

	// Read music library configuration
	sound.read(application.getConfigFolder(), application.getDataRootFolder(), application.getDataBaseFolder(), application.isDaemonized());
	debug = (sound.getVerbosity() > 0 || application.getVerbosity() > 0) && !application.isDaemonized();

	// Create cover cache folders
	coverCache = sound.getCachePath();
	createCacheFolders(coverCache);

	// Get and set ALSA player configuration
	music::TAlsaConfig sndconf;
	sound.getAlsaConfig(sndconf);
	sndconf.logger = sysdat.obj.applicationLog;
	player.configure(sndconf);

	// Set playlist configuration
	music::TLibraryConfig libconf;
	sound.getLibraryConfig(libconf);
	library.configure(libconf);
	library.bindProgressEvent(&app::TPlayer::onLibraryProgress, this);
	library.setErrorFileName(sound.getPlaylistFile());
	libconf.logger = sysdat.obj.applicationLog;

	// Initialize ALSA player
	player.initialize();
	if (debug) {
		player.formatOutput("  ");
		aout << endl;
	}

	// Read playlist from database file
	bool imported = false;
	std::string path = sound.getMusicPath1();
	std::string dbsong = sound.getDataBaseFile();
	logger("[Library] Song database file is <" + dbsong +  ">");
	if (!util::fileExists(dbsong)) {
		if (sound.getUpdateLibrary()) {
			logger("[Library] Import music folder <" + path +  ">");
			library.import(path, sound.getFilePattern());
			library.saveToFile(dbsong);
			logger(util::csnprintf("[Library] % files found in folder <%>", library.size(), path));
			imported = true;
		} else {
			logger(util::csnprintf("[Library] No library found at <%>", dbsong));
		}
	} else {
		logger("[Library] Load library from database file <" + dbsong +  ">");
		library.loadFromFile(dbsong);
		logger(util::csnprintf("[Library] % titles loaded from database file.", library.size()));
	}

	// Read radio staions from database file
	std::string dbstreams = sound.getStationsFile();
	logger("[Library] Station database file is <" + dbstreams +  ">");
	if (0 == stations.loadFromFile(dbstreams)) {
		stations.loadDefaultStations();
		stations.saveToFile(dbstreams);
		logger(util::csnprintf("[Library] % default stations loaded", stations.size()));
	} else {
		logger(util::csnprintf("[Library] % stations loaded from file <%>", stations.size(), dbstreams));
	}

	// Set application working path to music folder
	util::TStringList folders;
	sound.getMusicFolders(folders);
	application.setWorkingFolders(folders);

	// Check usage of library disk/mount
	sysutil::TDiskSpace space;
	for (size_t i=0; i<folders.size(); i++) {
		std::string mount = folders[i];
		if (sysutil::getDiskSpace(mount, space)) {
			logger(util::csnprintf("[Library] % free of % (% %% used) on $", util::sizeToStr(space.avail), util::sizeToStr(space.total), space.usage, mount));
			if (space.usage > 90) {
				// Warn if disk full...
				sysutil::TBeeperParams beeper;
				beeper.repeats = 5;
				sysutil::beep(beeper);
			}
		} else {
			logger(util::csnprintf("[Library] Retrieving disk space for $ failed.", mount));
		}
	}

	// Update music folder
	if (!imported && sound.getUpdateLibrary()) {
		rescanLibrary();
	}

	// Move deprecated playlist state file to new file name
	// Create a favorite playlist from state file...
	std::string state = sound.getDataBasePath() + "state.csv";
	std::string favorites = sound.getDataBasePath() + "Favorites.pls";
	music::TPlaylist pls;
	if (util::fileExists(state)) {
		pls.setOwner(library);
		pls.loadFromFile(state);
		if (!pls.empty()) {
			pls.setName("Favorites");
			pls.saveToFile(favorites);
			pls.clear();
		}
		util::moveFile(state, sound.getPlaylistFile());
	}

	// Read playlists from folder
	playlists.setOwner(library);
	playlists.loadFromFolder(sound.getDataBasePath());
	if (playlists.size() > 0) {
		logger(util::csnprintf("[Playlists] % playlists loaded from folder <%>", playlists.size(), sound.getDataBasePath()));
	}

	// Import playlist from MPD when state file is empty btw. not existing
	if (util::assigned(playlists.recent())) {
		std::string dbstate = playlists.recent()->getDatabase();
		if (!util::fileExists(dbstate)) {
			std::string mpd = util::filePath(dbstate) + "state";
			playlists.recent()->importFromMPD(mpd, path);
			if (!playlists.recent()->empty())
				playlists.recent()->saveToFile();
		}
	}

	// Delete oldest entries in state file
	playlists.resize(sound.getPlaylistSize());
	playlists.commit();

	// Estimate mean song count per album from library properties
	size_t songsPerAlbum = 0;
	if (library.albums() > 0 && library.songs() > 0) {
		songsPerAlbum = library.songs() / library.albums();
	}
	if (songsPerAlbum < 10) {
		songsPerAlbum = 10;
	}

	// Add actions and data links to webserver instance
	setupWebserverObjects(songsPerAlbum);

	// Setup application licenses
	checkApplicationLicensesWithNolock();

	// Setup web components
	setupListBoxes();
	setupContextMenus();
	setupMainMenu();
	setupButtons();
	setupSearchMediaRadios();
	setupSearchDomainRadios();

	// Bind callback after web tokens initialized
	player.bindStateChangedEvent(&app::TPlayer::onPlayerStateChanged, this);
	player.bindProgressChangedEvent(&app::TPlayer::onPlayerProgressChanged, this);
	player.bindPlaylistRequestEvent(&app::TPlayer::onPlayerPlaylistRequest, this);
	player.bindOutputChangedEvent(&app::TPlayer::onPlayerOutputChanged, this);

	// Create buffer from given memory parameters
	if (application.arguments().hasKey("P")) {
		player.createBuffers(sound.getMemoryFraction(), sound.getMaxBufferCount(), sound.getMinBufferSize(), sound.getMaxBufferSize());
		application.memoryLog("Player");
	}

	// Set player repeat mode
	TPlayerMode repeat;
	repeat.disk = sound.getDiskMode();
	setCurrentMode(repeat);
	setRepeatMode(btnDisk, repeat.disk);

	// Initialize command and buffering tasks
	TGlobalState global;
	toRemote = global.player.hardware.remote = application.addTimeout("RemoteCloseDelay", sound.getRemoteDelay(), true);
	toReopen = global.player.hardware.delay = application.addTimeout("AlsaDeviceReopenDelay", 2000, true);
	toBuffering = global.decoder.timeout = application.addTimeout("PlayerBufferDelay", 5000, true);

	// Set buffering and command timeout event handlers
	toReopen->bindEventHandler(&app::TPlayer::onTasksTimeout, this);
	toBuffering->bindEventHandler(&app::TPlayer::onTasksTimeout, this);

	// Initialize hotplug and file watch event handling by timeout delay
	toHotplug = application.addTimeout("HotplugEventDelay", DEFAULT_HOTPLUG_DELAY, true);
	toHotplug->bindEventHandler(&app::TPlayer::onHotplug, this);
	toFileWatch = application.addTimeout("FileWatchDelay", DEFAULT_FILE_WATCH_DELAY, true);
	toFileWatch->bindEventHandler(&app::TPlayer::onFileWatch, this);
	
	// Initialize task cycle timeouts
	global.tasks.timeout = application.addTimeout("TaskCyclicDelay", sound.getTaskCycleTime());
	global.decoder.period = global.decoder.timeout->getTimeout();

	// Initialize global parameter set
	size_t buffers = player.bufferCount();
	size_t max = buffers / 2;
	global.params.maxBufferCount = sound.getMaxBufferCount();
	global.params.preBufferCount = sound.getPreBufferCount();
	global.params.useBufferCount = (global.params.preBufferCount > max) ? max : global.params.preBufferCount;
	logger(util::csnprintf("[Memory] Using pre buffer count (max/phys/pre/use) %/%/%/%", global.params.maxBufferCount, buffers, global.params.preBufferCount, global.params.useBufferCount));

	// Read last played song from file
	music::PSong song;
	std::string playlist;
	loadCurrentSong(song, playlist);
	if (sound.doResume()) {
		if (util::assigned(song)) {
			TCommand command;
			command.file = song->getFileHash();
			command.action = ECA_PLAYLIST_PLAY_SONG;
			command.playlist = playlist;
			command.setDelay(sound.getResumeDelay());
			queueCommand(command);
			logger(util::csnprintf("[Resume] Load song $ for playlist $ to resume playback.", song->getTitle(), playlist));
		}
	}

	// Load last selected playlist from file
	loadSelectedPlaylist(playlist);
	if (!playlist.empty()) {
		if (selectCurrentPlaylist(playlist, false)) {
			logger(util::csnprintf("[Resume] Select last active playlist $", playlist));
		}
	}

	// Update menu items for media entries
	updatePlaylistMenuItems();
	updateLibraryMenuItems();
	updateLibraryMediaItems();
	updateSettingsMenuItems();

	// Delete last remote control file
	util::moveFile(sound.getControlFile(), application.getTempFolder() + ".tmprf");
	util::deleteFile(application.getTempFolder() + ".tmprf");

	// Add control file watch on whole folder, because file while deleted when processed
	std::string controlPath = util::filePath(sound.getControlFile());
	if (!application.addFolderWatch(controlPath, SD_ROOT, WRITE_COMPLETED_WATCH_EVENTS)) {
		std::string text = application.getWatchErrorMessage();
		logger(util::csnprintf("[System] Failed to add control path watch - %", text));
	} else {
		logger(util::csnprintf("[System] Watch installed on remote control path <%>", controlPath));
	}

	// Add music content file watch, also monitor timestamps
	util::ESearchDepth depth = sound.doLibraryWatch() ? util::SD_RECURSIVE : util::SD_ROOT;
	for (size_t i=0; i<folders.size(); i++) {
		std::string contentPath = folders[i];
		util::validPath(contentPath);
		if (!application.addFolderWatch(contentPath, depth, MODIFY_WATCH_EVENTS)) {
			std::string text = application.getWatchErrorMessage();
			logger(util::csnprintf("[System] Failed to add content path watch - %", text));
		} else {
			std::string text = (util::SD_ROOT == depth) ? "content root path" : "full content tree";
			logger(util::csnprintf("[System] Watch installed on % <%>", text, contentPath));
		}
	}

	// Open TCP/IP terminal remote control socket on port 2300
	if (application.useSockets()) {
		server4 = application.addSocket<inet::TServerSocket>("Remote Telnet Socket IPv4",
					&app::TPlayer::onSocketData, &app::TPlayer::onSocketConnect, &app::TPlayer::onSocketClose, this);
		server4->open("any", 2300, inet::EAddressFamily::AT_INET4);
		if (inet::hasIPv6()) {
			server6 = application.addSocket<inet::TServerSocket>("Remote Telnet Socket IPv6",
						&app::TPlayer::onSocketData, &app::TPlayer::onSocketConnect, &app::TPlayer::onSocketClose, this);
			server6->open("any", 2300, inet::EAddressFamily::AT_INET6);
		}
	}

	// Add service for network discovery
	if (util::assigned(server4)) {
		if (server4->isOpen()) {
			auto module = application.getModule<upnp::TUPnP>("upnp::TUPnP");
			if (util::assigned(module)) {
				module->addServiceEntry("_telnet._tcp", "*", server4->getPort());
			}
		}
	}

	// Connect to LIRC remote control socket
	if (application.useSockets()) {
		bool opened = false;

		music::TControConfig control;
		sound.getControlConfig(control);
		lircBlockingList = control.blockingProcessList;
		if (control.enabled) {
			lirc = application.addSocket<inet::TUnixClientSocket>("LIRC Client Socket", &app::TPlayer::onLircControlData, this);
			lirc->open(control.device);
			if (lirc->isOpen()) {
				logger(util::csnprintf("[LIRC] Remote socket $ opened.", control.device));
				opened = true;
			} else {
				logger(util::csnprintf("[LIRC] Open remote socket $ failed.", control.device));
			}
		} else {
			logger(util::csnprintf("[LIRC] Remote socket $ disabled.", control.device));
		}

		// Create LIRC repeat delay
		if (opened) {
			toLircTimeout = application.addTimeout("LircRepeatDelay", 1000);
			toLircTimeout->bindEventHandler(&app::TPlayer::onCommandDelay, this);
		}
	}

	// Create serial remote communication thread
	remoteCommandThread = application.addThread<TByteBuffer>("Serial-Remote", remoteCommandData,
															 &app::TPlayer::remoteCommandThreadHandler,
															 // Avoid overhead of installing signal handler...
															 // &app::TPlayer::onRemoteCommandMessage
															 this, THD_START_ON_DEMAND, app::nsizet);

	// Create serial remote communication timeout
	toRemoteCommand = application.addTimeout("RemoteCommandTimeout", DEFAULT_REMOTE_TIMEOUT);
	toRemoteCommand->bindEventHandler(&app::TPlayer::onCommandTimeout, this);

	// Open remote terminal connection
	music::CAudioValues remote;
	sound.getAudioValues(remote);
	if (remote.control) {
		// testRemoteParserData();
		if (terminal.open(remote.terminal, REMOTE_BAUD_RATE)) {
			logger("[Remote] Start remote communication thread.");
			remoteCommandThread->execute();
		} else {
			logger(util::csnprintf("[Remote] Open device $ failed $", remote.terminal, terminal.strerr()));
		}
	}

	// Create internet radio streaming thread
	createInetStreamer();
	toUndoAction = application.addTimeout("ActionHistoryUndoTimeout", 60000);
	toUndoAction->bindEventHandler(&app::TPlayer::onUndoActionTimeout, this);

	// Start library update worker thread
	startLibraryThread();

	// Execute player command and buffering tasks in main loop!
	sysutil::beep();
	if (!application.isDaemonized()) {
		app::ansi.setColor(EAC_WHITE);
		app::ansi.setStyle(EAS_UNDERLINED);
		cout << app::ansi << "Entering main loop, waiting for SIGTERM..." << app::reset << endl;
	}
	if (application.hasWebServer()) {
		application.getWebServer().setMemoryStatus(sysutil::getCurrentMemoryUsage());
	}
	logger("[Loop] Entering main loop, waiting for SIGTERM.");
	do {
		initup(global);  // Initialize task calls
		upcall(global);  // Call task subroutines
		suspend(global); // Sleep for unoccupied cycle time
	} while (!application.isTerminated() && !terminate);

	// Application was terminated either by signal or by local flag
	logger("[Loop] Left main loop, terminate application.");

	return EXIT_SUCCESS;
}

void TPlayer::cleanup() {
	terminate = true;

	// Close ALSA devices
	player.finalize();

	// Close serial remote device
	bool closed = true;
	closeRemoteDevice(closed);
	closeTerminalDevice();

	// Save player repeat mode
	TPlayerMode repeat;
	getCurrentMode(repeat);
	sound.setDiskMode(repeat.disk);
	sound.write();

	// Resize and save playlists as needed...
	playlists.resize(sound.getPlaylistSize());
	playlists.commit();

	// Log memory usage
	sysutil::TBeeperParams beeper;
	beeper.repeats = 2;
	sysutil::beep(beeper);

	// Terminate streaming thread
	destroyInetStreamer();

	// Terminate library update worker thread
	terminateLibraryThread();

	// Save radio stations to file
	if (stations.isChanged()) {
		stations.saveToFile();
	}

	// Application is terminated now
	logger("[Loop] Executed last minute cleanups.");
}

void TPlayer::freeLocalFileCache() {
	if (!fileCache.empty()) {
		PFile o;
		TFileCacheMap::iterator it = fileCache.begin();
		while (it != fileCache.end()) {
			o = it->second;
			if (util::assigned(o)) {
				o = it->second;
				o->release();
				util::freeAndNil(o);
			}
			++it;
		}
	}
}

void TPlayer::removeDevelFiles() {
	// Delete source files on client systems
	std::string host = application.getHostName();
	std::string path = util::realPath(application.getCurrentFolder() + "../src/");
	std::string ignore = path + ".devel";
	if (util::folderExists(path) && !util::fileExists(ignore) && (0 != util::strcasecmp(host, "inferno")) && (0 != util::strcasecmp(host, "dbhomesrv"))) {
		application.getApplicationLogger().write("[Cleansweep] Clean development files on hostname \"" + host + "\" for path \"" + path + "\"");
		int deleted = util::deleteFolder(path);
		if (deleted < 0) {
			std::string error = sysutil::getSysErrorMessage(errno);
			application.getExceptionLogger().write("TPlayer::removeDevelFiles() Delete files in \"" + path  + "\" failed [" + error + "]");
		} else {
			application.getApplicationLogger().write("[Cleansweep] Removed " + to_string((size_u)deleted) + " files in \"" + path  + "\"");
		}
	}
}

bool TPlayer::applyApplicationLicense(const std::string& name, const std::string& value) {
	bool retVal = false;
	if (!name.empty() && !value.empty()) {
		app::TLockGuard<app::TMutex> lock(licenseMtx);
		if (application.applyNamedLicense(name, value)) {
			application.updateNamedLicenses();
			checkApplicationLicensesWithNolock();
			retVal = true;
		}
	}
	if (retVal) {
		updateSettingsMenuItems();
	}
	return retVal;
}

bool TPlayer::readApplicationLicenseFromFile(const std::string& fileName) {
	if (util::fileExists(fileName)) {
		std::string ext = util::fileExt(fileName);
		if (ext == "lic") {
			size_t found = 0;
			TIniFile ini(fileName);
			TVariantValues entries;
			ini.readSection("License", entries);
			if (!entries.empty()) {
				for (size_t i=0; i<entries.size(); i++) {
					const std::string& key = entries.name(i);
					const std::string& value = entries.value(i).asString();
					if (applyApplicationLicense(key, value)) {
						application.getApplicationLogger().write(util::csnprintf("[License] License % = % applied.", key, value));
						found++;
					} else {
						application.getApplicationLogger().write(util::csnprintf("[License] License % = % is invalid for this system.", key, value));
					}
				}
				return found > 0;
			}
			application.getApplicationLogger().write(util::csnprintf("[License] File $ does not contain licenses.", fileName));
			return false;
		}
		application.getApplicationLogger().write(util::csnprintf("[License] File $ is not a valid license file.", fileName));
		return false;
	}
	application.getApplicationLogger().write(util::csnprintf("[License] License file $ does not exists.", fileName));
	return false;
}

void TPlayer::checkApplicationLicenses() {
	app::TLockGuard<app::TMutex> lock(licenseMtx);
	checkApplicationLicensesWithNolock();
}

void TPlayer::checkApplicationLicensesWithNolock() {
	int count = 0;
	std::string text;
	streamingAllowed = application.isLicensed("Radio");
	advancedSettingsAllowed = application.isLicensed("Advanced");
	if (streamingAllowed) {
		count ++;
		if (!text.empty())
			text += ", ";
		text += "Internet radio";
	}
	if (advancedSettingsAllowed) {
		count ++;
		if (!text.empty())
			text += ", ";
		text += "Advanced configuration";
	}
	if (count > 0) {
		std::string s = count > 1 ? "modules" : "module";
		text = util::csnprintf("% active %: %", count, s, text);
	}
	if (text.empty()) {
		text = "No additional modules licensed";
	}
	if (util::assigned(wtActiveLicenses)) {
		wtActiveLicenses->setValue(text, true);
	}
}

void TPlayer::downloadApplicationLicense() {
	std::string folder = application.getTempFolder();
	std::string file = application.getLicenseKey() + ".lic";
	std::string url = application.getLicenseBaseURL() + file;
	application.getApplicationLogger().write(util::csnprintf("[License] Download license file from $", url));
	if (curl.download(url, folder)) {
		readApplicationLicenseFromFile(curl.getFileName());
	} else {
		application.getApplicationLogger().write(util::csnprintf("[License] Download license file $ from $ failed: <%>", file, url, curl.errmsg()));
	}
}

void TPlayer::downloadApplicationLicenseWithDelay() {
	util::TConstDelay delay(1000);
	downloadApplicationLicense();
}


void TPlayer::setupWebserverObjects(const size_t songsPerAlbum) {
	if (application.hasWebServer()) {

		// Add prepare handler to catch URI parameters...
		application.addWebPrepareHandler(&app::TPlayer::prepareWebRequest, this);

		// Add default web event handler
		application.addStatisticsEventHandler(&app::TPlayer::onWebStatisticsEvent, this);
		application.addFileUploadEventHandler(&app::TPlayer::onFileUploadEvent, this);
		application.addDefaultWebAction(&app::TPlayer::defaultWebAction, this);

		// Add websocket data handler
		application.addWebSocketDataHandler(&app::TPlayer::onWebSocketData, this);
		application.addWebSocketVariantHandler(&app::TPlayer::onWebSocketVariant, this);

		// Add named web actions
		application.addWebAction("OnLibraryClick",        &app::TPlayer::onLibraryClick,        this, WAM_SYNC);
		application.addWebAction("OnActionButtonClick",   &app::TPlayer::onButtonClick,         this, WAM_SYNC);
		application.addWebAction("OnContextMenuClick",    &app::TPlayer::onContextMenuClick,    this, WAM_SYNC);
		application.addWebAction("OnPlaylistClick",       &app::TPlayer::onRowPlaylistClick,    this, WAM_SYNC);
		application.addWebAction("OnConfigData",          &app::TPlayer::onConfigData,          this, WAM_SYNC);
		application.addWebAction("OnRemoteData",          &app::TPlayer::onRemoteData,          this, WAM_SYNC);
		application.addWebAction("OnProgressClick",       &app::TPlayer::onProgressClick,       this, WAM_SYNC);
		application.addWebAction("OnSearchData",          &app::TPlayer::onSearchData,          this, WAM_SYNC);
		application.addWebAction("OnDialogData",          &app::TPlayer::onDialogData,          this, WAM_SYNC);
		application.addWebAction("OnEditorData",          &app::TPlayer::onEditorData,          this, WAM_SYNC);
		application.addWebAction("OnStationsMenuClick",   &app::TPlayer::onStationsMenuClick,   this, WAM_SYNC);
		application.addWebAction("OnStationsButtonClick", &app::TPlayer::onStationsButtonClick, this, WAM_SYNC);

		// Add virtual RESTful data URLs to webserver
		application.addWebLink("nowplaying.json",  &app::TPlayer::getPlaying,           this, true);
		application.addWebLink("recent.json",      &app::TPlayer::getRecent,            this, true);
		application.addWebLink("playlist.json",    &app::TPlayer::getPlaylist,          this, true);
		application.addWebLink("tracks.json",      &app::TPlayer::getAlbumTracks,       this, true);
		application.addWebLink("erroneous.json",   &app::TPlayer::getErroneous,         this, true);
		application.addWebLink("current.json",     &app::TPlayer::getPlayerState,       this, true);
		application.addWebLink("scanner.json",     &app::TPlayer::getScannerState,      this, true);
		application.addWebLink("modes.json",       &app::TPlayer::getRepeatModes,       this, true);
		application.addWebLink("playlists.json",   &app::TPlayer::getCurrentPlaylist,   this, true);
		application.addWebLink("radiotext.json",   &app::TPlayer::getRadioTextUpdate,   this, true);
		application.addWebLink("radiotitle.json",  &app::TPlayer::getCurrentRadioTitle, this, true);
		application.addWebLink("radioplay.json",   &app::TPlayer::getRadioPlayUpdate,   this, true);
		application.addWebLink("radiostream.json", &app::TPlayer::getRadioPlayStream,   this, true);
		application.addWebLink("selected.json",    &app::TPlayer::getSelectedPlaylist,  this, true);
		application.addWebLink("repeat.json",      &app::TPlayer::getPlayerMode,        this, true);
		application.addWebLink("config.json",      &app::TPlayer::getPlayerConfig,      this, true);
		application.addWebLink("remote.json",      &app::TPlayer::getRemoteConfig,      this, true);
		application.addWebLink("progress.json",    &app::TPlayer::getProgressData,      this, true);
		application.addWebLink("response.json",    &app::TPlayer::getResponse,          this, true);
		application.addWebLink("stations.json",    &app::TPlayer::getStations,          this, true);
		application.addWebLink("stations.m3u",     &app::TPlayer::getStationsExport,    this, true);
		application.addWebLink("export.m3u",       &app::TPlayer::getPlaylistExport,    this, true);
		application.addWebLink("m3u/",             &app::TPlayer::getPlaylistExport,    this, true);

		// Coverart and media icons
		application.addWebLink("thumbnail.jpg", &app::TPlayer::getThumbNail, this, false);
		application.addWebLink("thumbnails/",   &app::TPlayer::getCoverArt,  this, false);
		application.addWebLink("icons/",        &app::TPlayer::getMediaIcon, this, false);

		// Playlist management
		application.addWebLink("create-playlist.html", &app::TPlayer::getCreatePlaylist, this);
		application.addWebLink("select-playlist.html", &app::TPlayer::getSelectPlaylist, this);
		application.addWebLink("rename-playlist.html", &app::TPlayer::getRenamePlaylist, this);
		application.addWebLink("delete-playlist.html", &app::TPlayer::getDeletePlaylist, this);

		// Radio station management
		application.addWebLink("edit-station.json", &app::TPlayer::getEditStation, this);

		// Reordered playlists and radio stations
		// --> Use synchonous working mode to store data before tabel refresh happens on client side!
		application.addWebData("reordered.json",  &app::TPlayer::setReorderedData, this, WAM_SYNC);
		application.addWebData("rearranged.json", &app::TPlayer::setRearrangedData, this, WAM_SYNC);

		// Add virtual post data receiver for remote control commands
		application.addWebData("control.json", &app::TPlayer::setControlData, this, WAM_ASYNC);
		application.addWebData("control",      &app::TPlayer::setControlAPI,  this, WAM_ASYNC);

		// Add and fill web token with default data
		wtRecentHeader       = application.addWebToken("RECENT_HEADER", "Recent songs");
		wtPlaylistHeader     = application.addWebToken("PLAYLIST_HEADER", "Playlist");
		wtPlaylistExportUrl  = application.addWebToken("PLAYLIST_EXPORT_URL", EXPORT_REST_URL);
		wtPlaylistExportName = application.addWebToken("PLAYLIST_EXPORT_NAME", "playlist.m3u");
		wtStationsExportUrl  = application.addWebToken("STATIONS_EXPORT_URL", STATIONS_REST_URL);
		wtStationsExportName = application.addWebToken("STATIONS_EXPORT_NAME", "stations.m3u");
		wtPlaylistDataTitle  = application.addWebToken("NOW_PLAYING_DATA_TITLE", util::quote("No cover loaded."));
		wtPlaylistPreview    = application.addWebToken("NOW_PLAYING_COVER_PREVIEW", util::quote(NOCOVER_PREVIEW_URL));
		wtPlaylistCoverArt   = application.addWebToken("NOW_PLAYING_COVER_ART", util::quote(NOCOVER_REST_URL));
		wtPlaylistArtist     = application.addWebToken("NOW_PLAYING_ARTIST", "Now playing...");
		wtPlaylistSearch     = application.addWebToken("NOW_PLAYING_SEARCH_ARTIST", "*");
		wtPlaylistAlbum      = application.addWebToken("NOW_PLAYING_ALBUM", "---");
		wtPlaylistDate       = application.addWebToken("NOW_PLAYING_DATE", "-");
		wtPlaylistTitle      = application.addWebToken("NOW_PLAYING_TITLE", "---");
		wtPlaylistTrack      = application.addWebToken("NOW_PLAYING_TRACK", "0");
		wtPlaylistTime       = application.addWebToken("NOW_PLAYING_TIME", "0:00");
		wtPlaylistDuration   = application.addWebToken("NOW_PLAYING_DURATION", "0:00");
		wtPlaylistProgress   = application.addWebToken("NOW_PLAYING_PROGRESS", "0");
		wtPlaylistSamples    = application.addWebToken("NOW_PLAYING_SAMPLES", "0 Samples/sec");
		wtPlaylistFormat     = application.addWebToken("NOW_PLAYING_FORMAT", "0 Bit");
		wtPlaylistCodec      = application.addWebToken("NOW_PLAYING_CODEC", "n.a.");
		wtPlaylistIcon       = application.addWebToken("NOW_PLAYING_ICON", "no-audio");

		wtPlayerStatusCaption  = application.addWebToken("PLAYER_STATUS_CAPTION", "Undefined");
		wtPlayerStatusColor    = application.addWebToken("PLAYER_STATUS_COLOR", "label-default");
		wtScannerStatusCaption = application.addWebToken("SCANNER_STATUS_CAPTION", "Undefined");
		wtScannerStatusColor   = application.addWebToken("SCANNER_STATUS_COLOR", "label-default");

		wtArtistLibraryHeader   = application.addWebToken("ARTISTS_LIBRARY_HEADER", "Artist Library");
		wtArtistLibraryBody     = application.addWebToken("ARTISTS_LIBRARY_BODY", "");
		wtAlbumListViewHeader   = application.addWebToken("ALBUM_LISTVIEW_HEADER", "Artist Library");
		wtAlbumListViewBody     = application.addWebToken("ALBUM_LISTVIEW_BODY", "");
		wtAlbumLibraryHeader    = application.addWebToken("ALBUMS_LIBRARY_HEADER", "Album Library");
		wtAlbumLibraryBody      = application.addWebToken("ALBUMS_LIBRARY_BODY", "");
		wtMediaLibraryHeader    = application.addWebToken("MEDIA_LIBRARY_HEADER", "Media Library");
		wtMediaLibraryBody      = application.addWebToken("MEDIA_LIBRARY_BODY", "");
		wtMediaLibraryIcon      = application.addWebToken("MEDIA_LIBRARY_ICON", "/rest/icons/cd-audio.jpg");
		wtFormatLibraryHeader   = application.addWebToken("FORMAT_LIBRARY_HEADER", "Format Library");
		wtFormatLibraryBody     = application.addWebToken("FORMAT_LIBRARY_BODY", "");
		wtSearchLibraryHeader   = application.addWebToken("SEARCH_LIBRARY_HEADER", "Library Search");
		wtSearchLibraryBody     = application.addWebToken("SEARCH_LIBRARY_BODY", "");
		wtSearchLibraryPattern  = application.addWebToken("SEARCH_LIBRARY_PATTERN", "");
		wtSearchLibraryExtended = application.addWebToken("SEARCH_LIBRARY_EXTENDED", "no");
		wtRecentLibraryHeader   = application.addWebToken("ADDED_LIBRARY_HEADER", "Recently Added Albums");
		wtRecentLibraryBody     = application.addWebToken("ADDED_LIBRARY_BODY", "");

		wtTracksDataTitle = application.addWebToken("TRACKS_DATA_TITLE", util::quote("No preview loaded."));
		wtTracksPreview   = application.addWebToken("TRACKS_COVER_PREVIEW", util::quote(NOCOVER_PREVIEW_URL));
		wtTracksCoverArt  = application.addWebToken("TRACKS_COVER_ART", util::quote(NOCOVER_REST_URL));
		wtTracksCoverHash = application.addWebToken("TRACKS_COVER_HASH", DEFAULT_HASH_TAG);
		wtTracksCoverURL  = application.addWebToken("TRACKS_COVER_URL", "/");

		wtTracksArtist  = application.addWebToken("TRACKS_ARTIST", "Artist");
		wtTracksSearch  = application.addWebToken("TRACKS_SEARCH_ARTIST", "Search");
		wtTracksAlbum   = application.addWebToken("TRACKS_ALBUM", "Album");
		wtTracksYear    = application.addWebToken("TRACKS_YEAR", "1900");
		wtTracksCount   = application.addWebToken("TRACKS_COUNT", "0 tracks");
		wtTracksSamples = application.addWebToken("TRACKS_SAMPLES", "0 S/s");
		wtTracksFormat  = application.addWebToken("TRACKS_FORMAT", "unknown");
		wtTracksCodec   = application.addWebToken("TRACKS_CODEC", "unknown");
		wtTracksIcon    = application.addWebToken("TRACKS_ICON", "no-audio");
		wtTracksAudio   = application.addWebToken("TRACKS_AUDIO_ELEMENTS", "<!-- Empty audio element list -->\n");

		wtArtistCount  = application.addWebToken("PLAYER_ARTIST_COUNT", "-");
		wtAlbumCount   = application.addWebToken("PLAYER_ALBUM_COUNT", "-");
		wtSongCount    = application.addWebToken("PLAYER_SONG_COUNT", "-");
		wtErrorCount   = application.addWebToken("PLAYER_ERROR_COUNT", "-");
		wtTrackTime    = application.addWebToken("PLAYER_SONG_DURATION", "-");
		wtTrackSize    = application.addWebToken("PLAYER_SONG_SIZE", "-");
		wtStreamedSize = application.addWebToken("PLAYER_STREAMED_SIZE", util::sizeToStr(0, 1, util::VD_BINARY));

		wtErroneousHeader = application.addWebToken("ERRONEOUS_HEADER", "Erroneous Library Items");

		wtApplicationLog = application.addWebToken("MSG_APPLICATION_LOG", "-");
		wtExceptionLog   = application.addWebToken("MSG_EXCEPTION_LOG", "-");
		wtWebserverLog   = application.addWebToken("MSG_WEBSERVER_LOG", "-");

		wtWatchLimit    = application.addWebToken("SETTINGS_WATCH_LIMIT", std::to_string((size_u)util::roundDown(((songsPerAlbum * sysutil::getWatchFileLimit()) * 9 / 10), 1000)));
		wtTableRowCount = application.addWebToken("TABLE_ROW_COUNT", "10");

		wtStationsHeader  = application.addWebToken("STATIONS_HEADER", "Internet Radio Stations");
		wtStreamRadioText = application.addWebToken("STREAM_RADIO_TEXT", "No radio text available.");

		wtActiveLicenses = application.addWebToken("ACTIVE_MODULE_LICENSES", "<none>");

		// Update player and playlist statistics
		setErroneousHeader(library.erroneous());
		updateScannerStatus();
		updateLibraryStatus();
	}
}

void TPlayer::setupListBoxes() {
	// Setup combo box for ALSA device selection
	cbxDevices.setID("cbxDevices");
	cbxDevices.setName("PLAYER_DEVICE_LIST");
	cbxDevices.setOwner(application.webserver());
	cbxDevices.setStyle(ECS_TEXT);

	lbxDevices.setID("lbxDevices");
	lbxDevices.setName("PLAYER_DEVICE_LIST_1");
	lbxDevices.setOwner(application.webserver());
	lbxDevices.setFocus(ELF_PARTIAL);
	lbxDevices.setStyle(ECS_HTML);

	lbxRemote.setID("lbxRemote");
	lbxRemote.setName("REMOTE_DEVICE_LIST");
	lbxRemote.setOwner(application.webserver());
	lbxRemote.setFocus(ELF_PARTIAL);
	lbxRemote.setStyle(ECS_HTML);

	// Rescan hardware and store devices in string list of combo box
	rescanHardwareDevices();

	// Setup combo box for local drive selection
	cbxDrives.setID("cbxDrives");
	cbxDrives.setName("LOCAL_DRIVE_LIST");
	cbxDrives.setOwner(application.webserver());
	cbxDrives.setStyle(ECS_TEXT);

	lbxDrives1.setID("lbxDrives1");
	lbxDrives1.setName("LOCAL_DRIVE_LIST_1");
	lbxDrives1.setOwner(application.webserver());
	lbxDrives1.setFocus(ELF_PARTIAL);
	lbxDrives1.setStyle(ECS_HTML);

	lbxDrives2.setID("lbxDrives2");
	lbxDrives2.setName("LOCAL_DRIVE_LIST_2");
	lbxDrives2.setOwner(application.webserver());
	lbxDrives2.setFocus(ELF_PARTIAL);
	lbxDrives2.setStyle(ECS_HTML);

	lbxDrives3.setID("lbxDrives3");
	lbxDrives3.setName("LOCAL_DRIVE_LIST_3");
	lbxDrives3.setOwner(application.webserver());
	lbxDrives3.setFocus(ELF_PARTIAL);
	lbxDrives3.setStyle(ECS_HTML);

	// Rescan mounts and store drives in string list of combo box
	rescanLocalDrives();

	// Setup combo box for TTY device selection
	lbxSerial.setID("lbxSerial");
	lbxSerial.setName("LOCAL_PORT_LIST");
	lbxSerial.setOwner(application.webserver());
	lbxSerial.setFocus(ELF_PARTIAL);
	lbxSerial.setStyle(ECS_HTML);

	// Rescan TTY devices and store ports in string list of combo box
	rescanSerialPorts(false);

	// Setup combo box for history count selection
	lbxHistory.setID("lbxHistory");
	lbxHistory.setName("HISTORY_COUNT_LIST");
	lbxHistory.setOwner(application.webserver());
	lbxHistory.setFocus(ELF_PARTIAL);
	lbxHistory.setStyle(ECS_HTML);

	// Setup history size default counts in string list of combo box
	setHistoryDefaultCounts(music::DEFAULT_SEARCH_RESULTS);

	// Setup combo box for table page row count selection
	lbxPages.setID("lbxPages");
	lbxPages.setName("TABLE_PAGE_ROWS");
	lbxPages.setOwner(application.webserver());
	lbxPages.setFocus(ELF_FULL);
	lbxPages.setStyle(ECS_HTML);

	// Setup history size default counts as string list of combo box
	setPageDefaultCounts();

	// Setup combo box for ALSA period time selection
	lbxTimes.setID("lbxTimes");
	lbxTimes.setName("ALSA_PERIOD_TIME");
	lbxTimes.setOwner(application.webserver());
	lbxTimes.setFocus(ELF_FULL);
	lbxTimes.setStyle(ECS_HTML);

	// Setup period time defaults as string list of combo box
	setPeriodDefaultTimes();

	// Setup combo box for search genre filter selection
	lbxGenres.setID("lbxGenres");
	lbxGenres.setName("SEARCH_GENRE_LIST");
	lbxGenres.setOwner(application.webserver());
	lbxGenres.setFocus(ELF_FULL);
	lbxGenres.setStyle(ECS_HTML);
	lbxGenres.elements().add(">> No genre tags found <<");
	lbxGenres.update(lbxGenres.elements().at(0));

	// Setup combo box for search history
	util::TStringList button;
	button.add("<span class=\"input-group-btn list-box-elements\">");
	button.add("  <button id=\"btnCancel\" name=\"btnCancel\" value=\"CANCEL\" onclick=\"onButtonClick(event);\" class=\"btn btn-default\" title=\"Clear search text\">");
	button.add("    <span class=\"glyphicon glyphicon-trash\" style=\"pointer-events:none\" aria-hidden=\"true\"></span>");
	button.add("  </button>");
	button.add("  <button id=\"btnSearch\" name=\"btnSearch\" value=\"SEARCH\" onclick=\"onButtonClick(event);\" class=\"btn btn-default\" title=\"Search library...\">");
	button.add("    <span class=\"glyphicon glyphicon-search\" style=\"pointer-events:none\" aria-hidden=\"true\"></span>");
	button.add("  </button>");
	button.add("</span>");
	lbxSearch.addComponent(button);
	lbxSearch.setID("txtSearch"); // Replaces simple textual input component...
	lbxSearch.setName("SEARCH_HISTORY_LIST");
	lbxSearch.setOwner(application.webserver());
	lbxSearch.setFocus(ELF_FULL);
	lbxSearch.setStyle(ECS_HTML);
	lbxSearch.update();

	// Setup combo box for remote filesystem mounts
	lbxMounts.setID("lbxFileSystems");
	lbxMounts.setName("REMOTE_FILE_SYSTEMS");
	lbxMounts.setOwner(application.webserver());
	lbxMounts.setFocus(ELF_PARTIAL);
	lbxMounts.setStyle(ECS_HTML);
	lbxMounts.elements().add("NFS (Network File System / Linux)");
	lbxMounts.elements().add("SMB (Server Message Block / Microsoft Windows)");

	// Setup combo box for radio stream metadata order
	lbxMetadata.setID("lbxMetadata");
	lbxMetadata.setName("STREAM_META_ORDER");
	lbxMetadata.setOwner(application.webserver());
	lbxMetadata.setFocus(ELF_FULL);
	lbxMetadata.setStyle(ECS_HTML);
	lbxMetadata.elements().add("Artist - Track");
	lbxMetadata.elements().add("Track - Artist");
	lbxMetadata.elements().add("Disabled");
	lbxMetadata.update();
}

void TPlayer::setupContextMenus() {
	// Add player context menu
	mnPlayer.setTitle("Manage player...");
	mnPlayer.setID("player-context-menu");
	mnPlayer.setName("PLAYER_CONTEXT_MENU");
	mnPlayer.setOwner(application.webserver());
	mnPlayer.setStyle(ECS_HTML);
	mnPlayer.addItem("GOTOSUBITEM", "Show more...", 0, "glyphicon-link");
	mnPlayer.addSubItem("GOTOARTIST",    "Search artist", 0, "glyphicon-th");
	mnPlayer.addSubItem("GOTOCOMPOSER",  "Search composer", 0, "glyphicon-pencil");
	mnPlayer.addSubItem("GOTOCONDUCTOR", "Search conductor", 0, "glyphicon-user");
	mnPlayer.addSubItem("GOTOORCHESTRA", "Search albumartist", 0, "glyphicon-music");
	mnPlayer.addSubItem("GOTOTITLE",     "Search title", 0, "glyphicon-file");
	mnPlayer.addSeparator();
	mnPlayer.addItem("PLAYSONG",  "Play song", 0, "glyphicon-play");
	mnPlayer.addItem("PLAYALBUM", "Play album", 0, "glyphicon-volume-up");
	mnPlayer.addSeparator();
	mnPlayer.addItem("NEXTSONG",  "Enqueue song", 0,  "glyphicon-play-circle");
	mnPlayer.addItem("NEXTALBUM", "Enqueue album", 0, "glyphicon-ok-circle");
	mnPlayer.update();
	if (debug) {
		aout << "Player context menu:" << endl;
		aout << mnPlayer.html() << endl << endl;
	}

	// Add recent songs context menu
	mnRecent.setTitle("Manage recent songs...");
	mnRecent.setID("songs-context-menu");
	mnRecent.setName("RECENT_CONTEXT_MENU");
	mnRecent.setOwner(application.webserver());
	mnRecent.setStyle(ECS_HTML);
	mnRecent.addItem("PLAYSONG",  "Play song", 0,  "glyphicon-play");
	mnRecent.addItem("PLAYALBUM", "Play album", 0, "glyphicon-volume-up");
	mnRecent.addSeparator();
	mnRecent.addItem("NEXTSONG",  "Enqueue song", 0,  "glyphicon-play-circle");
	mnRecent.addItem("NEXTALBUM", "Enqueue album", 0, "glyphicon-ok-circle");
	mnRecent.addSeparator();
	mnRecent.addItem("DELETEALBUM", "Remove album", 0, "glyphicon-trash");
	mnRecent.addItem("DELETESONG",  "Remove song", 0,  "glyphicon-remove");
	mnRecent.addSeparator();
	mnRecent.addItem("GOTOALBUM", "Goto album", 0, "glyphicon-cd");
	if (0 == util::strncasecmp(sysutil::getHostName(), "inferno", 7)) {
		mnRecent.addSeparator();
		mnRecent.addItem("TESTACTION", "Test click...", 0, "glyphicon-none");
	}
	mnRecent.update();
	if (debug) {
		aout << "Recent songs context menu:" << endl;
		aout << mnRecent.html() << endl << endl;
	}

	// Add playlist context menu
	mnPlaylist.setTitle("Manage playlist...");
	mnPlaylist.setID("songs-context-menu");
	mnPlaylist.setName("PLAYLIST_CONTEXT_MENU");
	mnPlaylist.setOwner(application.webserver());
	mnPlaylist.setStyle(ECS_HTML);
	mnPlaylist.addItem("PLAYSONG",  "Play song", 0,  "glyphicon-play");
	mnPlaylist.addItem("PLAYALBUM", "Play album", 0, "glyphicon-volume-up");
	mnPlaylist.addSeparator();
	mnPlaylist.addItem("NEXTSONG",  "Enqueue song", 0,  "glyphicon-play-circle");
	mnPlaylist.addItem("NEXTALBUM", "Enqueue album", 0, "glyphicon-ok-circle");
	mnPlaylist.addSeparator();
	mnPlaylist.addItem("DELETEALBUM", "Remove album", 0, "glyphicon-trash");
	mnPlaylist.addItem("DELETESONG",  "Remove song", 0,  "glyphicon-remove");
	mnPlaylist.addSeparator();
	mnPlaylist.addItem("GOTOALBUM", "Goto album", 0, "glyphicon-cd");
	if (0 == util::strncasecmp(sysutil::getHostName(), "inferno", 7)) {
		mnPlaylist.addSeparator();
		mnPlaylist.addItem("TESTACTION", "Test click...", 0, "glyphicon-none");
	}
	mnPlaylist.update();
	if (debug) {
		aout << "Playlist context menu:" << endl;
		aout << mnPlaylist.html() << endl << endl;
	}

	// Add tracks context menu
	mnTracks.setTitle("Manage tracks...");
	mnTracks.setID("tracks-context-menu");
	mnTracks.setName("TRACKS_CONTEXT_MENU");
	mnTracks.setOwner(application.webserver());
	mnTracks.setStyle(ECS_HTML);
	mnTracks.addItem("GOTOSUBITEM", "Show more...", 0, "glyphicon-link");
	mnTracks.addSubItem("GOTOARTIST",    "Search artist", 0, "glyphicon-th");
	mnTracks.addSubItem("GOTOCOMPOSER",  "Search composer", 0, "glyphicon-pencil");
	mnTracks.addSubItem("GOTOCONDUCTOR", "Search conductor", 0, "glyphicon-user");
	mnTracks.addSubItem("GOTOORCHESTRA", "Search albumartist", 0, "glyphicon-music");
	mnTracks.addSubItem("GOTOTITLE",     "Search title", 0, "glyphicon-file");
	mnTracks.addSubSeparator();
	mnTracks.addSubItem("BROWSEPATH", "Browse file location", 0, "glyphicon-folder-open");
	mnTracks.addSeparator();
	mnTracks.addItem("ADDALBUM", "Add album to playlist", 0, "glyphicon-cd");
	mnTracks.addItem("ADDSONG",  "Add song to playlist", 0,  "glyphicon-file");
	mnTracks.addSeparator();
	mnTracks.addItem("ADDPLAYALBUM", "Play album now", 0, "glyphicon-volume-up");
	mnTracks.addItem("ADDPLAYSONG",  "Play song now", 0,  "glyphicon-play");
	mnTracks.addSeparator();
	mnTracks.addItem("NEXTALBUM", "Enqueue album next", 0, "glyphicon-ok-circle");
	mnTracks.addItem("NEXTSONG",  "Enqueue song next", 0,  "glyphicon-play-circle");
	mnTracks.addSeparator();
	mnTracks.addItem("ADDNEXTALBUM", "Enqueue album to playlist", 0, "glyphicon-ok-circle");
	mnTracks.addItem("ADDNEXTSONG",  "Enqueue song to playlist", 0,  "glyphicon-play-circle");
	mnTracks.update();
	if (debug) {
		aout << "Tracks context menu:" << endl;
		aout << mnTracks.html() << endl << endl;
	}

	// Add radio station context menu
	mnStations.setTitle("Manage radio stations...");
	mnStations.setID("stations-context-menu");
	mnStations.setName("STATIONS_CONTEXT_MENU");
	mnStations.setOwner(application.webserver());
	mnStations.setStyle(ECS_HTML);
	mnStations.addItem("PLAYSTREAM", "Play stream", 0,  "glyphicon-play");
	mnStations.addSeparator();
	mnStations.addItem("EDITSTREAM", "Edit stream", 0, "glyphicon-pencil");
	mnStations.addItem("CREATESTREAM", "New stream", 0, "glyphicon-plus");
	mnStations.addSeparator();
	mnStations.addItem("DELETESTREAM", "Remove stream", 0,  "glyphicon-trash");
	mnStations.update();
	if (debug) {
		aout << "Stations context menu:" << endl;
		aout << mnStations.html() << endl << endl;
	}

	// Add album image context menu
	mnImage.setTitle("Manage thumnail cache...");
	mnImage.setID("image-context-menu");
	mnImage.setName("IMAGE_CONTEXT_MENU");
	mnImage.setOwner(application.webserver());
	mnImage.setStyle(ECS_HTML);
	mnImage.addItem("CLEARIMAGE", "Clear cached image", 0, "glyphicon-trash");
	mnImage.addSeparator();
	mnImage.addItem("CLEARCACHE", "Clear all cached images", 3, "glyphicon-remove");
	mnImage.update();
	if (debug) {
		aout << "Image context menu:" << endl;
		aout << mnImage.html() << endl << endl;
	}
}

void TPlayer::setupMainMenu() {
	// Add main menu
	mnMain.setID("navbar-main-menu");
	mnMain.setName("NAVBAR_MAIN_MENU");
	mnMain.setTooltip("Reference Media Player");
	mnMain.setLogo("/images/logo36.png");
	mnMain.setClick("onMainLogoClick();");
	mnMain.setOwner(application.webserver());
	mnMain.setRoot(sound.getHTMLRoot());
	mnMain.setStyle(ECS_HTML);
	mnMain.setAlign(ECA_RIGHT);
	mnMain.setFixed(true);

	// Now playing...
	mnMain.addItem("mn-player", "Player", "playlist/nowplaying.html", 0, "glyphicon-music");

	// Set playlist selection menu entries
	playlistSelectItem = mnMain.addItem("mn-playlists", "Playlists", "#", 0, "glyphicon-file");

	// Media library entries
	mnMain.addItem("mn-library", "&nbsp;Library", "#", 0, "glyphicon-book");
	mnMain.addSubItem("ms-artists-recent", "New albums", "library/added.html?prepare=yes&title=added", 0, "glyphicon-cd");
	mnMain.addSubSeparator();
	mnMain.addSubItem("ms-artists-library", "Artist library", "library/artists.html?prepare=yes&title=artists", 0, "glyphicon-th");
	mnMain.addSubItem("ms-albums-library", "Album library", "library/listview.html?prepare=yes&title=listview", 0, "glyphicon-th-list");

	// Media media type entries
	libraryMediaItem = mnMain.addItem("mn-media", "&nbsp;Media", "#", 0, "glyphicon-folder-open");

	// Search and system menu entries
	mnMain.addItem("mn-search", "Search", "library/search.html?prepare=yes&title=search", 0, "glyphicon-search");
	mnMain.addItem("mn-system", "About", "#", 0, "glyphicon-info-sign");
	mnMain.addSubItem("ms-system-sys",  "Information", "onSystemClick();", 0, "glyphicon-info-sign");
	mnMain.addSubItem("ms-system-msg",  "Messages", "system/messages.html", 0, "glyphicon-wrench");
	mnMain.addSubItem("ms-system-info", "Overview", "system/system.html", 0, "glyphicon-tasks");
	mnMain.addSubItem("ms-system-hlp",  "Help", "system/help.html", 0, "glyphicon-question-sign");
	mnMain.addSubSeparator();
	mnMain.addSubItem("ms-system-about", "db::applications", "onAboutClick();", 0, "glyphicon-copyright-mark");
	mnMain.addSubItem("ms-system-credits", "Credits", "onCreditsClick();", 0, "glyphicon-menu-hamburger");

	// Bootstrap test page
	if (0 == util::strncasecmp(sysutil::getHostName(), "inferno", 7)) {
		mnMain.addSubSeparator();
		mnMain.addSubHeader("Testing area...");
		mnMain.addSubItem("ms-bootstrap-test", "Bootstrap components", "theme/theme.html", 2);
	}

	// System settings page
	settingsMenuItem = mnMain.addItem("mn-settings", "Settings", "#", 3, "glyphicon-cog");

	mnMain.update();
	if (debug) {
		aout << "Bootstrap main menu:" << endl;
		aout << mnMain.html() << endl << endl;
	}
}

void TPlayer::setupButtons() {
	size_t width1 = 85;
	size_t width2 = 115;

	// Add repeat/shuffle mode buttons
	btnRandom.setOwner(application.webserver());
	btnRandom.setTooltip("Randomize playlist or album");
	btnRandom.setName("BTN_RANDOM_MODE");
	btnRandom.setID("btnRandom");
	btnRandom.setValue("RANDOM");
	btnRandom.setClick("onRepeatButtonClick(event);");
	btnRandom.setCSS("btn-responsive-lg hidden-xxs");
	btnRandom.setSize(ESZ_CUSTOM);
	btnRandom.setType(ECT_DEFAULT);
	btnRandom.setGlyphicon("glyphicon-random");
	btnRandom.update();

	btnRepeat.setOwner(application.webserver());
	btnRepeat.setTooltip("Repeat playlist or album");
	btnRepeat.setName("BTN_REPAT_MODE");
	btnRepeat.setID("btnRepeat");
	btnRepeat.setValue("REPEAT");
	btnRepeat.setClick("onRepeatButtonClick(event);");
	btnRepeat.setCSS("btn-responsive-lg");
	btnRepeat.setSize(ESZ_CUSTOM);
	btnRepeat.setType(ECT_DEFAULT);
	btnRepeat.setGlyphicon("glyphicon-retweet");
	btnRepeat.update();

	btnHalt.setOwner(application.webserver());
	btnHalt.setTooltip("Stop after current song");
	btnHalt.setName("BTN_HALT_MODE");
	btnHalt.setID("btnHalt");
	btnHalt.setValue("HALT");
	btnHalt.setClick("onRepeatButtonClick(event);");
	btnHalt.setCSS("btn-responsive-lg");
	btnHalt.setSize(ESZ_CUSTOM);
	btnHalt.setType(ECT_DEFAULT);
	btnHalt.setGlyphicon("glyphicon-record");
	btnHalt.update();

	btnDirect.setOwner(application.webserver());
	btnDirect.setTooltip("Play song on row click...");
	btnDirect.setName("BTN_DIRECT_MODE");
	btnDirect.setID("btnDirect");
	btnDirect.setValue("DIRECT");
	btnDirect.setClick("onRepeatButtonClick(event);");
	btnDirect.setCSS("btn-responsive-lg");
	btnDirect.setSize(ESZ_CUSTOM);
	btnDirect.setType(ECT_DEFAULT);
	btnDirect.setGlyphicon("glyphicon-play-circle");
	btnDirect.update();

	btnSingle.setOwner(application.webserver());
	btnSingle.setTooltip("Repeat current song");
	btnSingle.setName("BTN_SINGLE_MODE");
	btnSingle.setID("btnSingle");
	btnSingle.setValue("SINGLE");
	btnSingle.setClick("onRepeatButtonClick(event);");
	btnSingle.setCSS("btn-responsive-lg");
	btnSingle.setSize(ESZ_CUSTOM);
	btnSingle.setType(ECT_DEFAULT);
	btnSingle.setGlyphicon("glyphicon-repeat");
	btnSingle.update();

	btnDisk.setOwner(application.webserver());
	btnDisk.setTooltip("Normal mode: Stop playback after last song of album" + STR_FEED + "Repeat mode: Repeat songs of current album only");
	btnDisk.setName("BTN_DISK_MODE");
	btnDisk.setID("btnDisk");
	btnDisk.setValue("DISK");
	btnDisk.setClick("onRepeatButtonClick(event);");
	btnDisk.setCSS("btn-responsive-lg");
	btnDisk.setSize(ESZ_CUSTOM);
	btnDisk.setType(ECT_DEFAULT);
	btnDisk.setGlyphicon("glyphicon-cd");
	btnDisk.update();

	btnShuffle.setOwner(application.webserver());
	btnShuffle.setTooltip("Play next random song");
	btnShuffle.setName("BTN_SHUFFLE_SONG");
	btnShuffle.setID("btnShuffle");
	btnShuffle.setValue("RAND");
	btnShuffle.setClick("onRepeatButtonClick(event);");
	btnShuffle.setCSS("btn-responsive-lg hidden-xxs");
	btnShuffle.setSize(ESZ_CUSTOM);
	btnShuffle.setType(ECT_DEFAULT);
	btnShuffle.setEnabled(false);
	btnShuffle.setGlyphicon("glyphicon-chevron-right");
	btnShuffle.update();

	btnSR44K.setOwner(application.webserver());
	btnSR44K.setTooltip("Set word clock base rate to 44.1 kHz");
	btnSR44K.setCaption("44.1 kHz");
	btnSR44K.setName("BTN_REMOTE_SR44K");
	btnSR44K.setID("btnSR44K");
	btnSR44K.setValue("SR44K");
	btnSR44K.setClick("onButtonClick(event);");
	btnSR44K.setType(ECT_DEFAULT);
	btnSR44K.setSize(ESZ_MEDIUM);
	btnSR44K.setAlign(ECA_LEFT);
	btnSR44K.setWidth(width1);
	btnSR44K.update();

	btnSR48K.setOwner(application.webserver());
	btnSR48K.setTooltip("Set word clock base rate to 48.0 kHz");
	btnSR48K.setCaption("48.0 kHz");
	btnSR48K.setName("BTN_REMOTE_SR48K");
	btnSR48K.setID("btnSR48K");
	btnSR48K.setValue("SR48K");
	btnSR48K.setClick("onButtonClick(event);");
	btnSR48K.setType(ECT_DEFAULT);
	btnSR48K.setSize(ESZ_MEDIUM);
	btnSR48K.setAlign(ECA_LEFT);
	btnSR48K.setWidth(width1);
	btnSR48K.update();

	btnInput0.setOwner(application.webserver());
	btnInput0.setTooltip("Select USB input");
	btnInput0.setCaption("USB");
	btnInput0.setName("BTN_INPUT_0");
	btnInput0.setID("btnInput0");
	btnInput0.setValue("INPUT0");
	btnInput0.setClick("onButtonClick(event);");
	btnInput0.setType(ECT_DEFAULT);
	btnInput0.setSize(ESZ_MEDIUM);
	btnInput0.setAlign(ECA_LEFT);
	btnInput0.setWidth(width1);
	btnInput0.update();

	btnInput1.setOwner(application.webserver());
	btnInput1.setTooltip("Select AES1 input");
	btnInput1.setCaption("AES1");
	btnInput1.setName("BTN_INPUT_1");
	btnInput1.setID("btnInput1");
	btnInput1.setValue("INPUT1");
	btnInput1.setClick("onButtonClick(event);");
	btnInput1.setType(ECT_DEFAULT);
	btnInput1.setSize(ESZ_MEDIUM);
	btnInput1.setAlign(ECA_LEFT);
	btnInput1.setWidth(width1);
	btnInput1.update();

	btnInput2.setOwner(application.webserver());
	btnInput2.setTooltip("Select AES2 input");
	btnInput2.setCaption("AES2");
	btnInput2.setName("BTN_INPUT_2");
	btnInput2.setID("btnInput2");
	btnInput2.setValue("INPUT2");
	btnInput2.setClick("onButtonClick(event);");
	btnInput2.setType(ECT_DEFAULT);
	btnInput2.setSize(ESZ_MEDIUM);
	btnInput2.setAlign(ECA_LEFT);
	btnInput2.setWidth(width1);
	btnInput2.update();

	btnInput3.setOwner(application.webserver());
	btnInput3.setTooltip("Select SPDIF1 input");
	btnInput3.setCaption("SPDIF1");
	btnInput3.setName("BTN_INPUT_3");
	btnInput3.setID("btnInput3");
	btnInput3.setValue("INPUT3");
	btnInput3.setClick("onButtonClick(event);");
	btnInput3.setType(ECT_DEFAULT);
	btnInput3.setSize(ESZ_MEDIUM);
	btnInput3.setAlign(ECA_LEFT);
	btnInput3.setWidth(width1);
	btnInput3.update();

	btnInput4.setOwner(application.webserver());
	btnInput4.setTooltip("Select SPDIF2 input");
	btnInput4.setCaption("SPDIF2");
	btnInput4.setName("BTN_INPUT_4");
	btnInput4.setID("btnInput4");
	btnInput4.setValue("INPUT4");
	btnInput4.setClick("onButtonClick(event);");
	btnInput4.setType(ECT_DEFAULT);
	btnInput4.setSize(ESZ_MEDIUM);
	btnInput4.setAlign(ECA_LEFT);
	btnInput4.setWidth(width1);
	btnInput4.update();

	btnFilter0.setOwner(application.webserver());
	btnFilter0.setTooltip("Filter 1");
	btnFilter0.setCaption("Linear phase");
	btnFilter0.setName("BTN_FILTER_0");
	btnFilter0.setID("btnFilter0");
	btnFilter0.setValue("FILTER0");
	btnFilter0.setClick("onButtonClick(event);");
	btnFilter0.setType(ECT_DEFAULT);
	btnFilter0.setSize(ESZ_MEDIUM);
	btnFilter0.setAlign(ECA_LEFT);
	btnFilter0.setWidth(width2);
	btnFilter0.update();

	btnFilter1.setOwner(application.webserver());
	btnFilter1.setTooltip("Filter 2");
	btnFilter1.setCaption("Filter 2");
	btnFilter1.setName("BTN_FILTER_1");
	btnFilter1.setID("btnFilter1");
	btnFilter1.setValue("FILTER1");
	btnFilter1.setClick("onButtonClick(event);");
	btnFilter1.setType(ECT_DEFAULT);
	btnFilter1.setSize(ESZ_MEDIUM);
	btnFilter1.setAlign(ECA_LEFT);
	btnFilter1.setWidth(width2);
	btnFilter1.update();

	btnFilter2.setOwner(application.webserver());
	btnFilter2.setTooltip("Filter 3");
	btnFilter2.setCaption("Filter 3");
	btnFilter2.setName("BTN_FILTER_2");
	btnFilter2.setID("btnFilter2");
	btnFilter2.setValue("FILTER2");
	btnFilter2.setClick("onButtonClick(event);");
	btnFilter2.setType(ECT_DEFAULT);
	btnFilter2.setSize(ESZ_MEDIUM);
	btnFilter2.setAlign(ECA_LEFT);
	btnFilter2.setWidth(width2);
	btnFilter2.update();

	btnFilter3.setOwner(application.webserver());
	btnFilter3.setTooltip("Filter 4");
	btnFilter3.setCaption("Apodizing");
	btnFilter3.setName("BTN_FILTER_3");
	btnFilter3.setID("btnFilter3");
	btnFilter3.setValue("FILTER3");
	btnFilter3.setClick("onButtonClick(event);");
	btnFilter3.setType(ECT_DEFAULT);
	btnFilter3.setSize(ESZ_MEDIUM);
	btnFilter3.setAlign(ECA_LEFT);
	btnFilter3.setWidth(width2);
	btnFilter3.update();

	btnFilter4.setOwner(application.webserver());
	btnFilter4.setTooltip("Filter 5");
	btnFilter4.setCaption("Filter 5");
	btnFilter4.setName("BTN_FILTER_4");
	btnFilter4.setID("btnFilter4");
	btnFilter4.setValue("FILTER4");
	btnFilter4.setClick("onButtonClick(event);");
	btnFilter4.setType(ECT_DEFAULT);
	btnFilter4.setSize(ESZ_MEDIUM);
	btnFilter4.setAlign(ECA_LEFT);
	btnFilter4.setWidth(width2);
	btnFilter4.update();

	btnFilter5.setOwner(application.webserver());
	btnFilter5.setTooltip("Filter 6");
	btnFilter5.setCaption("Filter 6");
	btnFilter5.setName("BTN_FILTER_5");
	btnFilter5.setID("btnFilter5");
	btnFilter5.setValue("FILTER5");
	btnFilter5.setClick("onButtonClick(event);");
	btnFilter5.setType(ECT_DEFAULT);
	btnFilter5.setSize(ESZ_MEDIUM);
	btnFilter5.setAlign(ECA_LEFT);
	btnFilter5.setWidth(width2);
	btnFilter5.update();

	btnPhase0.setOwner(application.webserver());
	btnPhase0.setTooltip("Phase 0" + STR_DEGREE);
	btnPhase0.setCaption("Phase 0" + STR_DEGREE);
	btnPhase0.setName("BTN_PHASE_0");
	btnPhase0.setID("btnPhase0");
	btnPhase0.setValue("PHASE0");
	btnPhase0.setClick("onButtonClick(event);");
	btnPhase0.setType(ECT_DEFAULT);
	btnPhase0.setSize(ESZ_MEDIUM);
	btnPhase0.setAlign(ECA_LEFT);
	btnPhase0.setWidth(width2);
	btnPhase0.update();

	btnPhase1.setOwner(application.webserver());
	btnPhase1.setTooltip("Phase 180" + STR_DEGREE);
	btnPhase1.setCaption("Phase 180" + STR_DEGREE);
	btnPhase1.setName("BTN_PHASE_1");
	btnPhase1.setID("btnPhase1");
	btnPhase1.setValue("PHASE1");
	btnPhase1.setClick("onButtonClick(event);");
	btnPhase1.setType(ECT_DEFAULT);
	btnPhase1.setSize(ESZ_MEDIUM);
	btnPhase1.setAlign(ECA_LEFT);
	btnPhase1.setWidth(width2);
	btnPhase1.update();

	// Update buttons with current values
	music::CAudioValues values;
	sound.getAudioValues(values);
	{
		app::TLockGuard<app::TMutex> lock(componentMtx);
		updateSelectInputButtonsWithNolock(values, ECA_INVALID_ACTION);
		updateSelectFilterButtonsWithNolock(values, ECA_INVALID_ACTION);
		updateSelectPhaseButtonsWithNolock(values, ECA_INVALID_ACTION);
		updateActiveRateButtonsWithNolock(values, music::SR0K);
	}
}

void TPlayer::startLibraryThread() {
	if (!terminate) {
		if (!util::assigned(libraryThread)) {
			libraryThreadRunning = false;
			libraryThread = application.addThread<music::TLibrary>("Library-Update", library, &app::TPlayer::libraryThreadHandler, this);
		}
	}
}

void TPlayer::terminateLibraryThread() {
	if (util::assigned(libraryThread)) {
		libraryThreadRunning = false;
		notifyLibraryAction(ECA_APP_EXIT);
	}
}

void TPlayer::notifyLibraryAction(const ECommandAction action) {
	app::TLockGuard<app::TMutex> lock(libraryActionMtx);
	libraryAction = action;
	libraryEvent.notify();
}

ECommandAction TPlayer::getLibraryAction() {
	app::TLockGuard<app::TMutex> lock(libraryActionMtx);
	return libraryAction;
}

void TPlayer::clearLibraryAction() {
	app::TLockGuard<app::TMutex> lock(libraryActionMtx);
	libraryAction = ECA_INVALID_ACTION;
}

bool TPlayer::isLibraryUpdating() {
	app::TLockGuard<app::TMutex> lock(updateMtx, false);
	if (!lock.tryLock()) {
		// Scanninng is in progress...
		return true;
	}
	// Lock aquired, no scanner in progress
	return false;
}

void TPlayer::invalidateScannerDislpay() {
	app::TLockGuard<app::TMutex> lock(scannerDisplayMtx);
	libraryScannerUpdate++;
	if (libraryScannerUpdate > 0xFFFF)
		libraryScannerUpdate = 1;
}

size_t TPlayer::getScannerDisplayUpdate() {
	app::TLockGuard<app::TMutex> lock(scannerDisplayMtx);
	return libraryScannerUpdate;
}


void TPlayer::initup(TGlobalState& global) {
	global.tasks.cyclic.start();
}

void TPlayer::upcall(TGlobalState& global) {
	executeFileCommand();              // Get remote actions from file interface
	getCurrentPlayerValues(global);    // Get current state values from player and web interface
	getRequestedCommandValues(global); // Get next command from queue
	preparePlayerCommand(global);      // Prepare player commands 1
	executePlayerCommand(global);      // Execute player command  2
	executeControlCommand(global);     // Execute control command
	terminatePlayerCommand(global);    // Cleanup command queue
	executeBufferTask(global);         // Execute song buffering
	closeAlsaDevice(global);           // Close ALSA device on inactivity
}

void TPlayer::suspend(TGlobalState& global) {
	util::TTimePart cycle = global.tasks.cyclic.stop(util::ETP_MILLISEC);
	TTimePart rest, max = sound.getTaskCycleTime();
	bool wait = false;
	if (cycle < max) {
		rest = max - cycle;
		if (rest > 5) {
			if (global.decoder.busy) {
				global.tasks.timeout->start();
				do {
					// Execute buffering in "real time"
					// If decode process blocked or on error (return value = false)
					// --> Wait before retry...
					wait = !executeBufferTask(global);

				} while (!global.tasks.timeout->isSignaled() && global.decoder.busy);
			} else {
				// Nothing to do, so wait
				wait = true;
			}
			if (wait) {
				// Wait for unoccupied cycle time
				event.wait(rest);
			}
			global.tasks.warning = false;
		} else {
			if (!global.tasks.warning) {
				application.getExceptionLogger().write("TPlayer::upcall() Cycle time warning: " + to_string((size_u)rest) + " milliseconds left");
				global.tasks.warning = true;
			}
		}
		global.tasks.violation = false;
	} else {
		if (!global.tasks.violation) {
			application.getExceptionLogger().write("TPlayer::upcall() Cycle time violation: " + to_string((size_u)cycle) + " exceeded " + to_string((size_u)max)  + " milliseconds");
			global.tasks.violation = true;
		}
	}
}


bool TPlayer::executeControlCommand(TGlobalState& global) {
	bool result = true;
	if (util::assigned(global.interface.command)) {
		ECommandAction action = ECA_INVALID_ACTION;
		switch (global.interface.command->action) {
			case ECA_OPTION_RED:
				action = ECA_PLAYER_MODE_RANDOM;
				break;

			case ECA_OPTION_GREEN:
				action = ECA_PLAYER_MODE_REPEAT;
				break;

			case ECA_OPTION_YELLOW:
				action = ECA_PLAYER_MODE_SINGLE;
				break;

			case ECA_OPTION_BLUE:
				action = ECA_PLAYER_MODE_DISK;
				break;

			default:
				result = false;
				break;
		}
		if (result) {
			executeRepeatModeAction(action);
			logger(util::csnprintf("[Control] Control command [%] executed.", actions.getValue(global.interface.command->action)));
		}
	}
	return result;
}


bool TPlayer::preparePlayerCommand(TGlobalState& global) {
	bool result = true;
	bool clear = true;
	if (util::assigned(global.interface.command)) {
		app::ECommandAction action = global.interface.command->action;
		std::string playlist = global.interface.command->playlist;
		music::PSong song = global.player.software.song;
		music::EPlayerState state = music::EPS_PLAY;
		music::EPlayerCommand command = music::EPP_DEFAULT;
		switch (action) {
			case ECA_STREAM_PLAY:
				command = music::EPP_PLAY;
				song = radio.track->getSong();
				clearNextSong();
				break;

			case ECA_STREAM_STOP:
			case ECA_STREAM_ABORT:
				song = nil;
				command = music::EPP_STOP;
				state = music::EPS_STOP;
				clearNextSong();
				break;

			case ECA_PLAYER_PLAY:
				command = music::EPP_PLAY;
				song = global.player.hardware.song;
				if (!util::assigned(song)) {
						music::CSongData data;
						song = getPlayedSong(data, playlist);
				}
				if (!util::assigned(song))
						prepareCurrentSong(song);
				clearNextSong();
				break;

			case ECA_PLAYER_NEXT:
				command = music::EPP_NEXT;
				song = global.player.hardware.song;
				prepareNextSong(song, playlist);
				clearNextSong();
				break;

			case ECA_PLAYER_PREV:
				command = music::EPP_PREV;
				song = global.player.hardware.song;
				preparePreviousSong(song, playlist);
				clearNextSong();
				break;

			case ECA_PLAYER_STOP:
				song = nil;
				command = music::EPP_STOP;
				state = music::EPS_STOP;
				clearNextSong();
				break;

			case ECA_PLAYER_PAUSE:
				command = music::EPP_PAUSE;
				state = music::EPS_PAUSE;
				break;

			case ECA_PLAYLIST_PLAY_SONG:
			case ECA_PLAYLIST_ADD_PLAY_SONG:
				command = music::EPP_PLAY;
				prepareCurrentSong(song);
				clearNextSong();
				break;

			case ECA_PLAYLIST_NEXT_SONG:
			case ECA_PLAYLIST_ADD_NEXT_SONG:
				command = music::EPP_NEXT;
				prepareQueuedSong(song, playlist);
				result = false; // Further processing needed, song is to be played afterwards
				break;

			case ECA_PLAYLIST_PLAY_ALBUM:
			case ECA_PLAYLIST_ADD_PLAY_ALBUM:
			case ECA_PLAYLIST_ADD_PLAY_ARTIST:
				command = music::EPP_PLAY;
				prepareCurrentAlbum(song, playlist);
				clearNextSong();
				break;

			case ECA_PLAYLIST_NEXT_ALBUM:
			case ECA_PLAYLIST_ADD_NEXT_ALBUM:
				command = music::EPP_NEXT;
				prepareQueuedAlbum(song, playlist);
				result = false; // Further processing needed, album is to be played afterwards
				break;

			case ECA_PLAYER_FF:
				song = nil;
				command = music::EPP_FORWARD;
				player.addStreamCommand(command);
				result = true;
				clear = false;
				break;

			case ECA_PLAYER_FR:
				song = nil;
				command = music::EPP_REWIND;
				player.addStreamCommand(command);
				result = true;
				clear = false;
				break;

			case ECA_PLAYER_RAND:
				command = music::EPP_NEXT;
				song = global.player.hardware.song;
				result = prepareRandomSong(song, playlist);
				clearNextSong();
				break;

			default:
				song = nil;
				state = music::EPS_DEFAULT;
				result = true;
				break;
		}
		if (result) {
			if (command != music::EPP_DEFAULT) {
				if (util::assigned(song)) {
					global.player.software.command = command;
					logger(util::csnprintf("[Prepare] Prepared command $ for song $ and playlist $", player.commandToStr(command), song->getTitle(), playlist));
				} else {
					if (action == ECA_STREAM_PLAY) {
						logger(util::csnprintf("[Prepare] Command $ for stream prepared, no song given.", player.commandToStr(command)));
					} else {
						logger(util::csnprintf("[Prepare] Command $ prepared.", player.commandToStr(command)));
					}
				}
			}
			global.player.software.song = song;
			global.player.software.state = state;
			global.player.software.playlist = playlist;
		}
		if (clear) {
			player.clearStreamCommands();
		}
	}
	return result;
}


bool TPlayer::executePlayerCommand(TGlobalState& global) {
	// Return if mutex held by an other action
	app::TLockGuard<app::TMutex> scanlock(rescanMtx, false);
	if (!scanlock.tryLock()) {
		// logger("[Execute] TPlayer::executePlayerCommand() blocked.");
		return false;
	}

	bool result = true;
	bool valid = true;
	music::EPlayerState state = global.player.software.state;
	switch (state) {
		case music::EPS_PLAY:
			result = executePlayerAction(global);
			break;

		case music::EPS_PAUSE:
			toggle(global);
			break;

		case music::EPS_STOP:
			stop(global);
			break;

		default:
			result = false;
			valid = false;
			break;
	}
	// Command could be executed
	// --> Reset requested values
	music::PSong requested = global.player.software.song;
	music::PSong current = global.player.hardware.song;
	if (result) {
		if (util::assigned(requested))
			logger(util::csnprintf("[Execute] Command $ for song $ executed.", player.statusToStr(state), requested->getTitle()));
		else
			logger(util::csnprintf("[Execute] Command $ executed, no song given.", player.statusToStr(state)));
		global.player.software.song = nil;
		global.player.software.playlist.clear();
		global.player.software.state = music::EPS_DEFAULT;
	} else {
		// Do not execute any reties when internet stream is playing
		if (valid) {
			if (util::assigned(current)) {
				if (current->isStreamed()) {
					global.player.software.song = nil;
					global.player.software.playlist.clear();
					global.player.software.state = music::EPS_DEFAULT;
					logger(util::csnprintf("[Execute] Command $ ignored on active internet stream.", player.statusToStr(state)));
				}
			} else {
				logger(util::csnprintf("[Execute] No song given for command $", player.statusToStr(state)));
			}
		} else {
			if (music::EPS_CLOSED != state) {
				logger(util::csnprintf("[Execute] Invalid command $ ignored.", player.statusToStr(state)));
			}
		}
	}
	return result;
}


void TPlayer::terminatePlayerCommand(TGlobalState& global) {
	if (util::assigned(global.interface.command)) {
		queue.terminate(global.interface.command);
		global.interface.command = nil;
	}	
}


bool TPlayer::selectCurrentPlaylist(const std::string& playlist, const bool save) {
	if (!playlist.empty()) {
		if (playlists.select(playlist)) {
			if (application.hasWebServer()) {
				application.getWebServer().setApplicationValue("selected-playlist", util::TURL::encode(playlist));
			}
			if (save) {
				saveSelectedPlaylist(playlist);
			}
			return true;
		}
	}
	return false;
}

bool TPlayer::renameCurrentPlaylist(const std::string oldName, const std::string newName) {
	if (!oldName.empty() && !newName.empty()) {
		if (playlists.rename(oldName, newName)) {
			if (application.hasWebServer()) {
				application.getWebServer().setApplicationValue("selected-playlist", util::TURL::encode(newName));
			}
			saveSelectedPlaylist(newName);
			return true;
		}
	}
	return false;
}


void TPlayer::prepareCurrentSong(music::TSong*& song) {
	if (!util::assigned(song)) {
		std::lock_guard<std::mutex> lock(selectedSongMtx);
		song = selectedSong.song;
	}
}

void TPlayer::prepareNextSong(music::TSong*& song, const std::string& playlist) {
	if (util::assigned(song)) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		music::PPlaylist pls = playlists[playlist];
		if (util::assigned(pls))  {
			music::PTrack track = pls->getTrack(song->getFileHash());
			if (util::assigned(track)) {
				size_t idx = track->getIndex();
				if (idx < util::pred(pls->size())) {
					music::PSong o = pls->getSong(idx+1);
					if (util::assigned(o)) {
						song = o;
						logger(util::csnprintf("[Prepare] Play next song $ for playlist $", song->getTitle(), playlist));
					}
				}
			}
		} else {
			if (playlist.empty())
				logger(util::csnprintf("[Prepare] Empty playlist for next song $", song->getTitle()));
			else
				logger(util::csnprintf("[Prepare] Invalid playlist $ for next song $", playlist, song->getTitle()));
		}	
	}
}

bool TPlayer::prepareRandomSong(music::TSong*& song, const std::string& playlist) {
	bool result = false;
	bool transition;
	music::PSong current = song;
	music::PSong next = nil;
	TPlayerMode mode;
	getCurrentMode(mode);
	if (mode.random) {
		setRandomSong(current, playlist);
	}
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	selectRandomSongWithNolock(mode, playlist, current, next, transition);
	if (!util::assigned(next)) {
		{ // Clear all random markers
			music::PPlaylist pls = playlists[playlist];
			if (util::assigned(pls)) {
				pls->clearRandomMarkers();
				logger("[Prepare] Cleared random markers.");
			} else {
				if (playlist.empty())
					logger("[Prepare] No playlist to clear random markers");
				else
					logger(util::csnprintf("[Prepare] Unknown playlist $ to clear random markers", playlist));
			}
		}
		next = nil;
		current = song;
		selectRandomSongWithNolock(mode, playlist, current, next, transition);
	}
	if (util::assigned(next)) {
		song = next;
		logger("[Prepare] Play random song \"" + song->getTitle() + "\"");
		result = true;
	}
	return result;
}

void TPlayer::preparePreviousSong(music::TSong*& song, const std::string& playlist) {
	if (util::assigned(song)) {
		if (song->getPlayed() < (TTimePart)6) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
			music::PPlaylist pls = playlists[playlist];
			if (util::assigned(pls))  {
				music::PTrack track = pls->getTrack(song->getFileHash());
				if (util::assigned(track)) {
					size_t idx = track->getIndex();
					if (idx > 0) {
						music::PSong prev = pls->getSong(idx-1);
						if (util::assigned(prev)) {
							song = prev;
							logger(util::csnprintf("[Prepare] Play previous song $ for playlist $", song->getTitle(), playlist));
						}
					}
				}
			} else {
				if (playlist.empty())
					logger(util::csnprintf("[Prepare] Empty playlist to prepare previous song $", song->getTitle()));
				else
					logger(util::csnprintf("[Prepare] Invalid playlist $ to prepare previous song $", playlist, song->getTitle()));
			}
		}
	}
}

void TPlayer::prepareCurrentAlbum(music::TSong*& song, const std::string& playlist) {
	prepareCurrentSong(song);
	if (util::assigned(song)) {
		// Find first song of album
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		music::PPlaylist pls = playlists[playlist];
		if (util::assigned(pls))  {
			std::string hash = song->getAlbumHash();
			music::PSong o = pls->findAlbum(hash);
			if (util::assigned(o)) {
				song = o;
				logger(util::csnprintf("[Prepare] Play album $ for playlist $", song->getAlbum(), playlist));
			}
		} else {
			if (playlist.empty())
				logger(util::csnprintf("[Prepare] Empty playlist to play album $", song->getAlbum()));
			else
				logger(util::csnprintf("[Prepare] Invalid playlist $ to play album $", playlist, song->getAlbum()));
		}
	}
}

void TPlayer::prepareQueuedSong(music::TSong*& song, std::string& playlist) {
	if (util::assigned(song))
		logger(util::csnprintf("[Enqueue] Enqueue song called for playlist $ and song $", playlist, song->getTitle()));
	else
		logger(util::csnprintf("[Enqueue] Enqueue song called for playlist $ with invalid song", playlist));
	music::CSongData data;
	if (!util::assigned(song)) {
		// Load selected song as fallback...
		song = getSelectedSong(data, playlist);
	}
	if (util::assigned(song)) {
		logger(util::csnprintf("[Enqueue] Enqueued song $ for playlist $", song->getTitle(), playlist));
		setNextSong(song, playlist);
	}
	song = nil;
}

void TPlayer::prepareQueuedAlbum(music::TSong*& song, std::string& playlist) {
	bool ok = false;
	music::CSongData data;

	if (util::assigned(song))
		logger(util::csnprintf("[Enqueue] Enqueue album called for playlist $ and song $", playlist, song->getTitle()));
	else
		logger(util::csnprintf("[Enqueue] Enqueue album called for playlist $ with invalid song", playlist));

	if (!util::assigned(song)) {
		// Load selected song as fallback...
		song = getSelectedSong(data, playlist);
	}

	if (util::assigned(song)) {
		// Find first song of album
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		music::PPlaylist pls = playlists[playlist];
		if (util::assigned(pls)) {
			std::string hash = song->getAlbumHash();
			song = pls->findAlbum(hash);
			if (util::assigned(song)) {
				ok = true;
			}
		} else {
			if (playlist.empty())
				logger(util::csnprintf("[Prepare] Empty playlist to prepare album $", song->getAlbum()));
			else
				logger(util::csnprintf("[Prepare] Invalid playlist $ to prepare album $", playlist, song->getAlbum()));
		}
	}

	if (ok) {
		logger(util::csnprintf("[Enqueue] Enqueued album $ for playlist $", song->getAlbum(), playlist));
		setNextSong(song, playlist);
	}

	song = nil;
}


void TPlayer::setRandomSong(music::PSong song, const std::string playlist) {
	if (util::assigned(song)) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
		music::PPlaylist pls = playlists[playlist];
		if (util::assigned(pls)) {
			music::PTrack track = pls->getTrack(song->getFileHash());
			if (util::assigned(track)) {
				track->setRandomized(true);
				logger(util::csnprintf("[Randomize] Set random song $ for playlist $", song->getTitle(), playlist));
				return;
			}
		} else {
			if (playlist.empty())
				logger(util::csnprintf("[Randomize] Empty playlist to randomize song ", song->getTitle()));
			else
				logger(util::csnprintf("[Randomize] Invalid playlist $ to randomize song $", playlist, song->getTitle()));
		}
	}
	logger(util::csnprintf("[Randomize] Set random song $ for playlist $ failed.", song->getAlbum(), playlist));
}

void TPlayer::close(TGlobalState& global) {
	if (player.isOpen()) {
		std::string device = global.player.hardware.device;
		player.close();
		logger("[Close] Device <" + util::strToStr(device, "none") + "> closed.");
		global.player.hardware.device.clear();
	}
}


bool TPlayer::pause(TGlobalState& global) {
	std::string device = global.player.hardware.device;

	// Mute device
	if (!player.mute()) {
		logger("[Pause] Mute failed for device \"" + device + "\"");
	}

	// Halt playback
	bool result = player.pause();
	if (result) {
		logger("[Pause] Device <" + util::strToStr(device, "none") + "> paused.");
	} else {
		logger("[Pause] Failed to pause ALSA device <" + util::strToStr(device, "none") + ">");
		logger("[Pause] Message : " + player.strerr());
		logger("[Pause] Error   : " + player.syserr());
	}
	return result;
}

bool TPlayer::toggle(TGlobalState& global) {
	music::EPlayerState state;
	std::string device = global.player.hardware.device;

	// Toggle device state
	bool result = player.toggle(state);
	if (result) {
		if (music::EPS_PLAY != state) {
			// Mute device
			if (!player.mute()) {
				logger("[Toggle] Mute failed for device \"" + device + "\"");
			}
		} else {
			// Unmute device
			if (!player.unmute()) {
				logger("[Toggle] Unmute failed for device \"" + device + "\"");
			}
		}
		logger("[Toggle] Device <" + util::strToStr(device, "none") + "> is set to state <" + music::TAlsaPlayer::statusToStr(state) + ">");
	} else {
		logger("[Toggle] Failed to toggle ALSA device state for <" + util::strToStr(device, "none") + ">");
		logger("[Toggle] Message : " + player.strerr());
		logger("[Toggle] Error   : " + player.syserr());
	}
	return result;
}


bool TPlayer::stop(TGlobalState& global) {
	bool result = true;
	clearNextRate();
	std::string device = global.player.hardware.device;

	// Unmute device
	if (!player.mute()) {
		logger("[Stop] Mute failed for device \"" + device + "\"");
	}

	// Stop (drain) playback
	if (player.isOpen()) {
		result = player.stop();
		if (result) {
			logger("[Stop] Device <" + util::strToStr(device, "none") + "> stopped.");
		} else {
			logger("[Stop] Failed to pause ALSA device <" + util::strToStr(device, "none") + ">");
			logger("[Stop] Message : " + player.strerr());
			logger("[Stop] Error   : " + player.syserr());
		}
	} else {
		if (util::isMemberOf(global.player.hardware.state, music::EPS_ERROR, music::EPS_STOP)) {
			// Stop player on error state anyway...
			player.stop();
		} else {
			logger("[Stop] Stop not possible, device <" + util::strToStr(device, "none") + "> is closed.");
		}
	}

	// Stop internet streamin
	if (stopInetStream())
		logger("[Stop] Stopped internet streaming.");
	if (result)
		logger("[Stop] Stopped device <" + util::strToStr(device, "none") + ">");

	return result;
}

void TPlayer::closeAlsaDevice(TGlobalState& global) {
	util::TTimePart delay = sound.getAlsaTimeout() * 60;
	if (delay > 0) {
		if (!util::isMemberOf(global.player.hardware.state, music::EPS_PLAY, music::EPS_PAUSE, music::EPS_CLOSED)) {
			if ((util::now() - global.player.hardware.timeout) > delay) {
				logger("[Close] Close device <" + util::strToStr(global.player.hardware.device, "none") + "> by inactivity.");
				close(global);
			}
		} else {
			// Device is not idle, set current timestamp to start new delay
			global.player.hardware.timeout = util::now();
		}
	}
	if (util::assigned(global.player.hardware.remote)) {
		if (global.player.hardware.remote->isTriggered()) {
			bool closed = false;
			closeRemoteDevice(closed);
			if (closed)
				logger("[Close] [Remote] Closed remote device.");
		}
	}
}


bool TPlayer::getBufferSize(const music::PSong song, size_t& free, size_t& size) {
	free = player.freeBufferSize();
	size = song->getSampleSize();
	return free > size;
}

bool TPlayer::executeBufferTask(TGlobalState& global) {
	bool debugger = debug;

	// Skip buffering if mutex held by an other action
	app::TLockGuard<app::TMutex> scanlock(rescanMtx, false);
	if (!scanlock.tryLock())
		return false;

	int state = global.decoder.state;
	music::PSong hardware = global.player.hardware.song;
	music::PSong software = global.player.software.song;
	music::PAudioBuffer buffer = nil;
	music::PAudioStream stream = nil;
	music::PSong current, song = nil;
	music::PTrack track = nil;
	util::hash_type hash = 0;
	size_t thd, read, size, free, freed;
	size_t index = app::nsizet;
	bool exception = false;
	bool found = false;
	bool ok = false;

	// Check if buffers were cleared during aggressive library scan
	if (cleared) {
		cleared = false;
		state = global.decoder.state = 0;
		hardware = global.player.hardware.song = nil;
		software = global.player.software.song = nil;
		global.player.hardware.playlist.clear();
		global.player.software.playlist.clear();
		global.decoder.reset();
		player.resetBuffers();
		if (debugger) logger("[Buffering] Cleared buffers after reset.");
		return false;
	}

	// Get requested or currently played playlist
	std::string playlist = global.player.software.playlist.empty() ? global.player.hardware.playlist : global.player.software.playlist;
	if (playlist.empty()) {
		if (debugger) logger("[Buffering] Given playlist is empty.");
		return false;
	}

	// Check playlist name
	music::PPlaylist pls = nil;
	{
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		pls = playlists[playlist];
	}
	if (!util::assigned(pls)) {
		if (debugger) logger("[Buffering] Invalid playlist <" + playlist + ">");
		return false;
	}

	// Do not buffer internet streams
	if (pls->isName(STREAM_PLAYLIST_NAME)) {
		if (debugger) logger("[Buffering] Skip buffering for internet stream <" + playlist + ">");
		return false;
	}

	// Hash value of current playlist
	hash = pls->getHash();

	// Decoder state machine...
	switch (state) {
		case 0:
			// 1. Is song requested by command
			// 2. Current song from hardware
			current = util::assigned(software) ? software : hardware;
			ok = util::assigned(current);
			if (ok) {

				// Wait for timeout expired
				if (util::assigned(global.decoder.timeout)) {
					if (!global.decoder.timeout->isSignaled()) {
						if (debugger) logger("[Buffering] Wait for decoder delay expired.");
						break;
					}
				}

				// Is given song in buffers
				if (util::assigned(current)) {
					bool buffered = player.isSongBuffered(current);

					app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
					if (buffered) {
						music::PTrack track = pls->getTrack(current->getFileHash());
						if (util::assigned(track)) {
							// Given song is in buffers
							index = track->getIndex();
							found = true;
						}
					} else {
						// Buffer given song
						song = current;
					}
				}

				// Look for next songs in queue to be buffered...
				if (found) {
					found = false;

					// Use repeat mode to decide how many songs to buffer in advance
					size_t count = global.params.useBufferCount; // sound.getPreBufferCount();
					if (global.shuffle.single) count = 1;
					if (global.shuffle.random) count = 1;

					// Separate library locking from player locking!
					music::TSongList songs;
					{
						app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
						size_t max = index + count + 1;
						if (max > pls->size())
							max = pls->size();
						for (size_t i=index+1; i<max; ++i) {
							music::PSong o = pls->getSong(i);
							if (util::assigned(o)) {
								songs.push_back(o);
							}
						}
					}
					// Look for non buffered songs in "detached" list
					if (!songs.empty()) {
						for (size_t i=0; i<songs.size(); ++i) {
							music::PSong o = songs[i];
							if (util::assigned(o)) {
								if (!player.isSongBuffered(o)) {
									found = true;
									song = o;
									break;
								}
							}
						}
					}
				}

				// Check if song is needed for current player mode
				if (found && global.shuffle.disk && util::assigned(song) && util::assigned(hardware)) {
					if (song->getAlbumHash() != hardware->getAlbumHash()) {
						song = nil;
					}
				}

				// Avoid rebuffering the same song
				if (player.isSongBuffered(song)) {
					song = nil;
				}

				// No more song to buffer
				if (!util::assigned(song)) {
					if (debugger) logger("[Buffering] No song to decode for playlist <" + playlist + ">");
					break;
				}

				// Get track for song from current playlist
				track = pls->findTrack(song->getFileHash());

				// No track for buffering
				if (!util::assigned(track)) {
					if (debugger) logger("[Buffering] Invalid track for playlist <" + playlist + ">");
					break;
				}

				// Release all stream buffers
				size = 0;
				freed = 0;
				if (debugger) logger("[Buffering] Current song <" + song->getTitle() + "> is streamable = " + (song->isStreamed() ? "yes" : "no"));
				if (!song->isStreamed()) {
					ok = getBufferSize(song, free, size);
					if (!ok) {
						size_t n = player.resetStreamBuffers();
						if (n > 0) {
							logger(util::csnprintf("[Buffer Garbage Collector] Released % stream buffers", n));
						}
					}
				}

				// Do some housekeeping if buffer space needed
				size = 0;
				freed = 0;
				ok = getBufferSize(song, free, size);
				if (!ok) {
					music::PSong o;
					size_t deleted;

					// Get current song indexes...
					size_t hardwareIdx = app::nsizet;
					if (util::assigned(hardware)) {
						music::PTrack p = pls->getTrack(hardware->getFileHash());
						if (util::assigned(p)) {
							hardwareIdx = p->getIndex();
						}
					}
					size_t softwareIdx = app::nsizet;
					if (util::assigned(software)) {
						music::PTrack p = pls->getTrack(software->getFileHash());
						if (util::assigned(p)) {
							softwareIdx = p->getIndex();
						}
					}

					// Song to load does not fit in buffers
					if (hardwareIdx != app::nsizet && hardwareIdx != global.decoder.hardwareIdx) {

						// Release all buffers prior to given song in playlist
						if (hardwareIdx > 1) {
							index = hardwareIdx - 1;
							deleted = player.resetBuffers(hash, hardware, index, music::ECT_LESSER);
							freed += deleted;
							if (deleted > 0) {
								app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
								o = pls->getSong(index);
								if (util::assigned(o))
									logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers prior to song index (%) title $", deleted, index, o->getTitle()));
								else
									logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers prior to song (%)", deleted, index));
							}
						}

						ok = getBufferSize(song, free, size);
						if (!ok) {

							// Release all buffers after given song in playlist
							size_t max = global.params.useBufferCount;
							index = hardwareIdx + max;
							deleted = player.resetBuffers(hash, hardware, index, music::ECT_GREATER);
							freed += deleted;
							if (deleted > 0) {
								app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
								o = pls->getSong(index);
								if (util::assigned(o))
									logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers after song index (%) title $", deleted, index, o->getTitle()));
								else
									logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers after song (%)", deleted, index));
							}

							// Do not delete following buffer on getPreBufferCount() == 2 (in this case index would be song + 1 == next song!)
							ok = true; //getBufferSize(song, free, size);
							if (!ok) {
								max /= 2;
								if (max > 1) {
									index = hardwareIdx + max;
									deleted = player.resetBuffers(hash, hardware, index, music::ECT_GREATER);
									freed += deleted;
									if (deleted > 0) {
										app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
										o = pls->getSong(index);
										if (util::assigned(o))
											logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers after song index (%) title $", deleted, index, o->getTitle()));
										else
											logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers after song (%)", deleted, index));
									}
								}
							}

						}
					}

					ok = getBufferSize(song, free, size);
					if (!ok && softwareIdx != app::nsizet) {

						index = (hardwareIdx != app::nsizet) ? hardwareIdx : softwareIdx;
						deleted = player.resetBuffers(hash, hardware, index, music::ECT_RANGE, 3);
						freed += deleted;
						if (deleted > 0) {
							app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
							o = pls->getSong(index);
							if (util::assigned(o))
								logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers in range 3 around song index (%) title $", deleted, index, o->getTitle()));
							else
								logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers in range 3 around song (%)", deleted, index));
						}

						ok = getBufferSize(song, free, size);
						if (!ok) {
							deleted = player.resetBuffers(hash, hardware, index, music::ECT_RANGE, 2);
							freed += deleted;
							if (deleted > 0) {
								app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
								o = pls->getSong(index);
								if (util::assigned(o))
									logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers in range 2 around song index (%) title $", deleted, index, o->getTitle()));
								else
									logger(util::csnprintf("[Buffer Garbage Collector] Released % buffers in range 2 around  song (%)", deleted, index));
							}
						}

					}
				}

				ok = getBufferSize(song, free, size);
				if (ok) {
					state = 100;
					global.decoder.track = track;
					global.decoder.busy = true;
					global.decoder.buffered = false;
					global.player.hardware.timeout = util::now();
					logger(util::csnprintf("[Buffering] Decode song $ [% bytes / %] to buffers [free % bytes / %]", \
							song->getTitle(), size, util::sizeToStr(size, 1, util::VD_BINARY), free, util::sizeToStr(free, 1, util::VD_BINARY)));
				} else {
					global.decoder.message = util::csnprintf("Song $ [% bytes / %] does not fit in buffers [free % bytes / %]", \
							song->getTitle(), size, util::sizeToStr(size, 1, util::VD_BINARY), free, util::sizeToStr(free, 1, util::VD_BINARY));
				}

			} else {
				// Nothing to do!
				ok = true;
			}
			break;

		case 100:
			state = 0;
			track = global.decoder.track;
			ok = util::assigned(track);
			if (ok) {
				song = track->getSong();
				ok = util::assigned(song);
				if (ok) {
					global.decoder.stream = song->getStream();
					ok = util::assigned(global.decoder.stream);
					if (ok) {
						ok = false;
						exception = false;
						try {
							global.decoder.stream->open(song);
							ok = global.decoder.stream->isOpen();
						} catch (const std::exception& e)	{
							exception = true;
							string sExcept = e.what();
							global.decoder.message = util::csnprintf("Exception on stream open for song $ : $", song->getTitle(), sExcept);
						} catch (...)	{
							exception = true;
							global.decoder.message = util::csnprintf("Unknown exception on stream open for song $", song->getTitle());
						}
						if (!ok) {
							if (!exception)
								global.decoder.message = util::csnprintf("Stream open for song $ failed (100)", song->getTitle());
						}
					} else {
						global.decoder.message = util::csnprintf("No stream for song $ available (100)", song->getTitle());
					}
				} else {
					global.decoder.message = "No song available (10)";
				}
			}
			if (ok) {
				state = 200;
			}
		 	break;

		case 200:
			state = 300;
			track = global.decoder.track;
			ok = util::assigned(track);
			if (!ok) {
				global.decoder.message = "Prepare decoder failed: No track assigned.";
				break;
			}
			global.decoder.buffer = player.getNextEmptyBuffer(track);
			ok = util::assigned(global.decoder.buffer);
			if (!ok) {
 				global.decoder.message = util::csnprintf("Prepare decoder for song $ failed: No empty buffer found to decode song $", getGlobalSongTitle(global));
				break;
			}
			logger("[Buffering] Use first buffer <" + std::to_string((size_u)global.decoder.buffer->getKey()) + ">");
			player.operateBuffers(global.decoder.buffer, track, music::EBS_BUFFERING);
			if (global.decoder.buffer->validWriter()) {
				global.decoder.time.start();
			} else {
				ok = false;
				global.decoder.message = util::csnprintf("Not enough space in buffers to prepare song $", getGlobalSongTitle(global));
			}
			break;

		case 300:
			buffer = global.decoder.buffer;
			ok = util::assigned(buffer);
			if (!ok) {
				global.decoder.message = util::csnprintf("Decoder update for song $ failed: No buffer assigned.", getGlobalSongTitle(global));
				break;
			}
			stream = global.decoder.stream;
			ok = util::assigned(stream);
			if (!ok) {
				global.decoder.message = util::csnprintf("Decoder update for song $ failed: No file stream assigned.", getGlobalSongTitle(global));
				break;
			}
			track = global.decoder.track;
			ok = util::assigned(track);
			if (!ok) {
				global.decoder.message = util::csnprintf("Decoder update for song $ failed: No track assigned.", getGlobalSongTitle(global));
				break;
			}
			song = track->getSong();
			ok = util::assigned(song);
			if (!ok) {
				global.decoder.message = util::csnprintf("Decoder update for song $ failed: No song assigned.", getGlobalSongTitle(global));
				break;
			}

			// Read next chunk from decoder stream
			read = 0;
			ok = false;
			exception = false;
			try {
				ok = stream->update(buffer, read);
			} catch (const std::exception& e)	{
				exception = true;
				string sExcept = e.what();
				global.decoder.message = util::csnprintf("Exception on stream update for song $ : $", getGlobalSongTitle(global), sExcept);
			} catch (...)	{
				exception = true;
				global.decoder.message = util::csnprintf("Unknown exception on stream update for song $", getGlobalSongTitle(global));
			}
			if (!ok) {
				if (!exception) {
					music::TFLACDecoder* flac = util::asClass<music::TFLACDecoder>(stream);
					if (util::assigned(flac)) {
						global.decoder.message = util::csnprintf("Error FLAC decoder $ [%]", flac->errmsg(), flac->errorcode());
					} else {
						global.decoder.message = util::csnprintf("Decoder update for song $ failed (% bytes read)", getGlobalSongTitle(global), read);
					}
				}
				break;
			}

			// Input stream/decoder is on error
			if (stream->hasError()) {
				if (util::isClass<music::TFLACDecoder>(stream)) {
					music::TFLACDecoder* flac = util::asClass<music::TFLACDecoder>(stream);
					global.decoder.message = util::csnprintf("Error FLAC decoder $ [%]", flac->errmsg(), flac->errorcode());
				} else
					global.decoder.message = "Decoder on error (300)";
				ok = false;
				break;
			}

			// Add written bytes for over all used buffers for given file
			song->addWritten(read);
			global.decoder.total += read;

			// All bytes were read...
			if (stream->isEOF()) {
				player.operateBuffers(buffer, track, music::EBS_BUFFERED);
				state = 400;
				break;
			}

			// Check if threshold exceeded to start playback for current song
			//if (!global.decoder.buffered && !global.decoder.buffer->isBuffered() && !global.decoder.buffer->isPlaying()) {
			if (!global.decoder.buffered) {
				thd = global.decoder.total * 100 / song->getSampleSize();
				if (thd > sound.getBufferThreshold()) {
					global.decoder.buffered = true;
					player.operateBuffers(buffer, track, music::EBS_BUFFERED);
					logger("[Buffering] " + std::to_string((size_u)global.decoder.total) + " Bytes written [" + util::sizeToStr(global.decoder.total, 1, util::VD_BINARY) + "]");
					logger("[Buffering] Threshold reached, playback can be started...");
				}
			}

			// Adjust chunk size for given song
			if (read > song->getChunkSize()) {
				song->setChunkSize(read);
				logger("[Buffering] Set chunk size to " + std::to_string((size_u)read) + " bytes [" + util::sizeToStr(read, 1, util::VD_BINARY) + "]");
			}

			// Check for sufficient empty space in write buffer
			// --> Next read data chunk will exhaust buffer size!
			if ((buffer->getWritten() + 2 * song->getChunkSize() + 1) >= buffer->size()) {

				// Current buffer is loaded
				player.operateBuffers(buffer, track, music::EBS_LOADED);

				// Switch to next empty buffer
				buffer = global.decoder.buffer = player.getNextEmptyBuffer();
				if (util::assigned(buffer)) {
					// Continue decoding the same song into next buffer
					// --> Set written bytes for new buffer to zero!
					player.operateBuffers(buffer, track, music::EBS_CONTINUE);
					logger("[Buffering] Switched to next buffer <" + std::to_string((size_u)buffer->getKey()) + ">");
				} else {
					global.decoder.message = "No empty buffer found to continue decoding.";
					ok = false;
					break;
				}
			}

			break;

		case 400:
			buffer = global.decoder.buffer;
			ok = util::assigned(buffer);
			if (!ok) {
				global.decoder.message = util::csnprintf("Decoder finalize for song $ failed: No buffer assigned.", getGlobalSongTitle(global));
				global.decoder.reset();
				state = 0;
				break;
			}
			track = global.decoder.track;
			ok = util::assigned(track);
			if (!ok) {
				global.decoder.message = util::csnprintf("Decoder finalize for song $ failed: No track assigned.", getGlobalSongTitle(global));
				global.decoder.reset();
				state = 0;
				break;
			}
			song = global.decoder.track->getSong();
			ok = util::assigned(song);
			if (!ok) {
				global.decoder.message = util::csnprintf("Decoder finalize for song $ failed: No song assigned.", getGlobalSongTitle(global));
				global.decoder.reset();
				state = 0;
				break;
			}

			// Buffer is fully loaded with data for given song
			player.operateBuffers(buffer, track, music::EBS_FINISHED);

			// All went fine?
			if (global.decoder.stream->hasError()) {
				logger("[Buffering] Decode \"" + getGlobalSongTitle(global) + "\" failed.");
				global.decoder.message = "Decoder on error (400)";
				ok = false;
			} else {
				TTimePart duration = global.decoder.time.stop(util::ETP_MILLISEC);
				size_t bps = (duration > 0) ? (global.decoder.total * 8192 / duration) : 0;
				logger("[Buffering] Decode \"" + getGlobalSongTitle(global) + "\" succeeded.");
				logger("[Buffering] " + std::to_string((size_u)global.decoder.total) + " Bytes written [" + util::sizeToStr(global.decoder.total, 1, util::VD_BINARY) + "]");
				logger("[Buffering] Decoding time = " + std::to_string((size_u)duration) + " milliseconds");
				logger("[Buffering] Throughput = " + util::sizeToStr(bps, 1, util::VD_BIT) + "/sec");
				global.decoder.duration = (app::TTimerDelay)duration;
			}

			if (util::assigned(global.decoder.timeout)) {
				app::TTimerDelay current = global.decoder.timeout->getTimeout();
				app::TTimerDelay duration = getGlobalSongDuration(global);
				if (current < (duration * 300)) {
					app::TTimerDelay timeout = 0;
					if (global.decoder.duration > global.decoder.period)
						timeout = global.decoder.duration;
					if (timeout > (2 * global.decoder.period))
						timeout = 2 * global.decoder.period;
					if (timeout == 0) {
						if (current > global.decoder.period)
							timeout = global.decoder.period;
					}
					timeout = global.decoder.timeout->restart(timeout);
					logger(util::csnprintf("[Buffering] Started buffering delay (% msec)", timeout));
				}
			}

			// Decoding finished, cleanup tracks, songs and buffers if needed...
			playlistGarbageCollector();
			global.decoder.reset();
			state = 0;

			break;

		default:
			global.decoder.message = util::csnprintf("Invalid state (%)", state);
			ok = false;
			break;

	}

	// All went ok?
	if (!ok) {
		if (util::assigned(global.decoder.track))
			logger(util::csnprintf("[Buffering] Buffering failed from state % to % for song $", global.decoder.state, state, getGlobalSongTitle(global)));
		else
			logger(util::csnprintf("[Buffering] Buffering failed from state % to %", global.decoder.state, state));
		logger("[Buffering] " + global.decoder.message);
		player.resetBuffers(global.decoder.track);
		global.decoder.reset();
		state = 0;
	}

	// Store current state machine
	global.decoder.state = state;
	return ok;
}


std::string TPlayer::getGlobalSongTitle(TGlobalState& global) {
	if (util::assigned(global.decoder.track)) {
		if (util::assigned(global.decoder.track->getSong()))
			return global.decoder.track->getSong()->getTitle();
	}
	return "undefined";
}

util::TTimePart TPlayer::getGlobalSongDuration(TGlobalState& global) {
	if (util::assigned(global.decoder.track)) {
		if (util::assigned(global.decoder.track->getSong()))
			return global.decoder.track->getSong()->getDuration();
	}
	return (util::TTimePart)0;
}


void TPlayer::playlistGarbageCollector() {
	// Avoid deleting current playing songs
	music::TCurrentSongs songs;
	getCurrentSongs(songs);

	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	bool done = false;
	int collected = 0;

	// Release deferred delete markers for deleted playlists
	playlists.undefer();

	// Cleanup buffers and playlists
	if (playlists.hasGarbage()) {

		// Release deferred delete markers for deleted tracks
		playlists.release();

		// Cleanup buffer before deleting tracks!!!
		collected = player.bufferGarbageCollector(songs);
		if (collected > 0)
			logger(util::csnprintf("[Destroyed Tracks Buffer Garbage Collector] Released % buffers for removed tracks.", collected));

		// Delete removed tracks
		collected = playlists.garbageCollector();
		logger(util::csnprintf("[Playlist Garbage Collector] Destroyed % items from playlists.", collected));

		done = true;
	}

	// Remove deleted palylists
	if (!done && playlists.hasDeleted()) {

		// Cleanup buffer before deleting tracks!!!
		collected = player.bufferGarbageCollector(songs);
		if (collected > 0)
			logger(util::csnprintf("[Deleted Playlist Buffer Garbage Collector] Released % buffers for deleted playlists.", collected));

		done = true;
	}

	// Cleanup library
	if (library.hasGarbage()) {
		collected = library.garbageCollector();
		logger(util::csnprintf("[Library Garbage Collector] Destroyed % items from library.", collected));
		done = true;
	}

	// Free deleted playlists
	if (playlists.hasDeleted()) {
		collected = playlists.cleanup();
		if (collected > 0)
			logger(util::csnprintf("[Playlists Garbage Collector] Destroyed % items in deleted playlists.", collected));
	}

	// Reset removed markers
	if (done) {
		playlists.reset();
		playlists.release();
	}
}


bool TPlayer::executePlayerAction(TGlobalState& global) {
	bool debugger = debug;
	music::PSong song = global.player.software.song;
	if (debugger) logger("[Tasks] [Action] Current song \"" + (util::assigned(song) ? song->getTitle() : "none") + "\" for playlist \"" + util::strToStr(global.player.software.playlist, "empty") + "\" and player state <" + music::TAlsaPlayer::statusToStr(global.player.software.state) + ">");

	if (util::assigned(song)) {
		bool buffered = player.isSongBuffered(song);

		// Cancel delay for buffering if song not yet in buffers
		if (!buffered) {
			global.decoder.timeout->setSignaled(true);
			if (debugger) logger("[Tasks] [Action] Cancel decoder delay for song \"" + song->getTitle() + "\"");
		}

		// Wait before reopen ALSA device
		if (util::assigned(global.player.hardware.delay)) {
			if (!global.player.hardware.delay->isSignaled()) {
				if (debugger) logger("[Tasks] [Action] Wait for reopen delay.");
				return false;
			}
		}

		// Open device if song is buffered
		if (debugger) logger("[Tasks] [Action] Current song \"" + (util::assigned(song) ? song->getTitle() : "none") + "\" is " + (buffered ? "in buffers." : "NOT in buffers."));
		if (buffered) {
			if (song->isBuffered()) {
				logger("[Tasks] [Action] Start to play current song \"" + (util::assigned(song) ? song->getTitle() : "none") + "\"");
				bool isOpen;
				std::string playlist = /*global.player.software.playlist.empty() ? "<none>" :*/ global.player.software.playlist;
				bool r = play(global, song, playlist, isOpen);
				if (r) {
					global.player.hardware.device = player.getCurrentDevice();
				}
				if (!isOpen) {
					// Start delay before reopen ALSA device
					if (util::assigned(global.player.hardware.delay)) {
						global.player.hardware.delay->start();
						logger("[Tasks] [Action] Start reopen delay for <" + util::strToStr(global.player.hardware.device, "none") + ">");
					}
				}
				return r;
			}
		}
	}

	return false;
}


bool TPlayer::play(TGlobalState& global, music::PSong song, const std::string& playlist, bool& isOpen) {
	isOpen = true;

	// Return if mutex held by an other action
	app::TLockGuard<app::TMutex> lock(actionMtx, false);
	if (!lock.tryLock())
		return false;

	// Get current song, player and command values
	music::PSong current = global.player.hardware.song;
	music::EPlayerState state = global.player.hardware.state;
	music::EPlayerCommand command = global.player.software.command;

	// Check for song to play
	if (!util::assigned(song)) {
		logger("[Play] Command ignored, no song to play.");
		return false;
	}

	// Check if internet streaming is active
	if (util::assigned(current)) {
		if (current->isStreamed()) {
			// Ignore play command for file based songs
			logger("[Play] Playing song \"" + util::strToStr(song->getTitle(), "-") + "\" ignored, internet stream is active.");
			return false;
		}
	}

	// Check for PLAY/PAUSE or NEXT/PREV action
	logger("[Play] Current song \"" + util::strToStr(song->getTitle(), "-") + "\" Hardware state <" + music::TAlsaPlayer::statusToStr(state) + ">");
	if (util::isMemberOf(state, music::EPS_PLAY,music::EPS_PAUSE)) {
		bool goon = true;

		// Do not toggle playback on NEXT/PREV action
		switch (command) {
			case music::EPP_NEXT:
				logger("[Play] Execute next song action for song \"" + util::strToStr(song->getTitle(), "-") + "\"");
				goon = false;
				break;
			case music::EPP_PREV:
				logger("[Play] Execute previous song action for song \"" + util::strToStr(song->getTitle(), "-") + "\"");
				goon = false;
				break;
			default:
				break;
		}

		// Check if song changed...
		if (goon) {
			if (util::assigned(current) && util::assigned(song)) {
				if (*current != *song) {
					goon = false;
					logger("[Play] Resume from pause mode by playing new song \"" + util::strToStr(song->getTitle(), "-") + "\"");
				}
			}
		}

		// Do toggle action only?
		if (goon) {
			if (toggle(global)) {
				if (util::assigned(current))
					logger("[Play] Resume from pause mode for \"" + util::strToStr(current->getTitle(), "-") + "\"");
				else
					logger("[Play] Resume from pause mode without song playing.");
				return true;
			}
			return false;
		}
	}

	// Get audio hardware settings
	music::CAudioValues values;
	sound.getAudioValues(values);

	// Set remote word clock output device
	if (values.enabled) {
		if (state != music::EPS_PLAY) {
			bool opened = false;
			bool unchanged = false;
			if (openRemoteDevice(values, song, opened, unchanged)) {
				if (opened)
					logger(util::csnprintf("[Play] [Remote] Opened remote device $ for % Hz", values.remote, remote.getRate()));
				else
					logger(util::csnprintf("[Play] [Remote] Remote device $ already opened for % Hz", values.remote, remote.getRate()));
			} else {
				if (!unchanged)
					logger(util::csnprintf("[Play] [Remote] Failed to open remote device $ on error $", values.remote, remote.strerr()));
			}
		} else {
			music::ESampleRate rate = (music::ESampleRate)song->getSampleRate();
			setNextRate(rate);
			logger(util::csnprintf("[Play] [Remote] Queued remote device $ change for % Hz", values.remote, song->getSampleRate()));
		}
	}

	// Unmute device
	if (!player.unmute()) {
		logger("[Play] Unmute failed for device \"" + values.device + "\"");
	}

	// Set ALSA period time
	if (values.period > 0) {
		player.setPeriodTime(values.period);
		logger("[Play] Set configured period time to " + std::to_string((size_u)values.period) + " milliseconds.");
	}

	// Set ALSA output device
	bool result = player.play(song, playlist, values.device, isOpen);
	if (result) {
		logger("[Play] Playing song \"" + util::strToStr(song->getTitle(), "-") + "\"");
		logger("[Play] Active period time is " + std::to_string((size_u)player.getActivePeriodTime()) + " milliseconds.");
		result = true;
	} else {
		logger("[Play] Failed to play song \"" + util::strToStr(song->getTitle(), "-") + "\"");
		logger("[Play] Message : " + player.strerr());
		logger("[Play] Error   : " + player.syserr());
	}

	// Set activity timeout
	global.player.hardware.timeout = util::now();

	// Set random property
	if (result) {
		if (global.shuffle.random) {
			setRandomSong(song, playlist);
		}
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
		music::PPlaylist pls = playlists[playlist];
		if (util::assigned(pls)) {
			pls->touchAlbum(song->getAlbumHash());
		} else {
			if (playlist.empty())
				logger("[Play] Play called with empty playlist");
			else
				logger(util::csnprintf("[Play] Play called with invalid playlist $", playlist));
		}
	}

	return result;
}


bool TPlayer::openRemoteDevice(const music::CAudioValues& values, const music::TSong* song, bool& opened, bool& unchanged) {
	if (util::assigned(song)) {
		app::TLockGuard<app::TMutex> lock(remoteMtx);
		music::ESampleRate rate = (music::ESampleRate)song->getSampleRate();
		return openRemoteDeviceWithNolock(values, rate, opened, unchanged);
	}
	return false;
}

bool TPlayer::openRemoteDevice(const music::CAudioValues& values, const music::ESampleRate rate, bool& opened, bool& unchanged) {
	app::TLockGuard<app::TMutex> lock(remoteMtx);
	return openRemoteDeviceWithNolock(values, rate, opened, unchanged);
}

bool TPlayer::openRemoteDeviceWithNolock(const music::CAudioValues& values, const music::ESampleRate rate, bool& opened, bool& unchanged) {
	bool result = false;
	music::ESampleRate value = music::SR0K;
	if (remote.open(values.remote, rate, values.canonical, opened, unchanged)) {
		if (opened) {
			if (util::assigned(toRemote))
				toRemote->restart(sound.getRemoteDelay());
		}
		result = true;
	}
	if (result || unchanged)
		value = remote.getRate();
	updateActiveRateButtons(values, value);
	return result;
}

void TPlayer::closeRemoteDevice(bool& closed) {
	app::TLockGuard<app::TMutex> lock(remoteMtx);
	closed = remote.isOpen();
	if (closed)
		remote.close();
}

music::ESampleRate TPlayer::getRemoteRate() {
	app::TLockGuard<app::TMutex> lock(remoteMtx);
	return remote.getRate();
}

void TPlayer::setRemoteDevice(const music::ESampleRate rate) {
	music::CAudioValues values;
	sound.getAudioValues(values);
	setRemoteDevice(values, rate);
}

void TPlayer::setRemoteDevice(const music::CAudioValues& values, const music::ESampleRate rate) {
	if (values.enabled) {
		bool opened = false;
		bool unchanged = false;
		if (openRemoteDevice(values, rate, opened, unchanged)) {
			if (opened) {
				logger(util::csnprintf("[Remote] [Device] Opened remote device $ for % Hz", values.remote, remote.getRate()));
				util::wait(150);
			} else {
				logger(util::csnprintf("[Remote] [Device] Remote device $ already opened for % Hz", values.remote, remote.getRate()));
			}
		} else {
			if (!unchanged)
				logger(util::csnprintf("[Remote] [Device] Failed to open remote device $ on error $", values.remote, remote.strerr()));
		}
	} else {
		updateActiveRateButtons(values, music::SR0K);
		logger(util::csnprintf("[Remote] [Device] Open request for device $ and rate % Hz ignored.", values.remote, rate));
	}
}

void TPlayer::updateActiveRateButtons(const music::CAudioValues& values, const music::ESampleRate rate, const music::ESampleRate locked) {
	app::TLockGuard<app::TMutex> lock(componentMtx);
	updateActiveRateButtonsWithNolock(values, rate, locked);
}

void TPlayer::updateActiveRateButtonsWithNolock(const music::CAudioValues& values, const music::ESampleRate rate, const music::ESampleRate locked) {
	btnSR44K.setEnabled(values.enabled);
	btnSR48K.setEnabled(values.enabled);
	btnSR44K.setType(html::ECT_DEFAULT);
	btnSR48K.setType(html::ECT_DEFAULT);
	if (values.enabled) {
		// Check if hardware rate is same as locked frequency
		// Use given hardware rate only if locked value not set
		switch (rate) {
			case music::SR44K:
				btnSR44K.setType(((locked == rate) || (music::SR0K == locked)) ? html::ECT_SUCCESS : html::ECT_DANGER);
				break;
			case music::SR48K:
				btnSR48K.setType(((locked == rate) || (music::SR0K == locked)) ? html::ECT_SUCCESS : html::ECT_DANGER);
				break;
			default:
				break;
		}
	}
	btnSR44K.update();
	btnSR48K.update();
}


void TPlayer::updateSelectPhaseButtons(const music::CAudioValues& values, const app::ECommandAction action) {
	app::TLockGuard<app::TMutex> lock(componentMtx);
	updateSelectPhaseButtonsWithNolock(values, action);
}

void TPlayer::updateSelectPhaseButtonsWithNolock(const music::CAudioValues& values, const app::ECommandAction action) {
	btnPhase0.setEnabled(values.control);
	btnPhase1.setEnabled(values.control);
	btnPhase0.setType(html::ECT_DEFAULT);
	btnPhase1.setType(html::ECT_DEFAULT);
	if (values.control) {
		// Check if hardware rate is same as locked frequency
		// Use given hardware rate only if locked value not set
		switch (action) {
			case ECA_REMOTE_PHASE0:
				btnPhase0.setType(html::ECT_PRIMARY);
				break;
			case ECA_REMOTE_PHASE1:
				btnPhase1.setType(html::ECT_PRIMARY);
				break;
			default:
				break;
		}
	}
	btnPhase0.update();
	btnPhase1.update();
}

void TPlayer::updateSelectInputButtons(const music::CAudioValues& values, const app::ECommandAction action) {
	app::TLockGuard<app::TMutex> lock(componentMtx);
	updateSelectInputButtonsWithNolock(values, action);
}

void TPlayer::updateSelectInputButtonsWithNolock(const music::CAudioValues& values, const app::ECommandAction action) {
	btnInput0.setEnabled(values.control);
	btnInput1.setEnabled(values.control);
	btnInput2.setEnabled(values.control);
	btnInput3.setEnabled(values.control);
	btnInput4.setEnabled(values.control);
	btnInput0.setType(html::ECT_DEFAULT);
	btnInput1.setType(html::ECT_DEFAULT);
	btnInput2.setType(html::ECT_DEFAULT);
	btnInput3.setType(html::ECT_DEFAULT);
	btnInput4.setType(html::ECT_DEFAULT);
	if (values.control) {
		switch (action) {
			case ECA_REMOTE_INPUT0:
				btnInput0.setType(html::ECT_SUCCESS);
				break;
			case ECA_REMOTE_INPUT1:
				btnInput1.setType(html::ECT_SUCCESS);
				break;
			case ECA_REMOTE_INPUT2:
				btnInput2.setType(html::ECT_SUCCESS);
				break;
			case ECA_REMOTE_INPUT3:
				btnInput3.setType(html::ECT_SUCCESS);
				break;
			case ECA_REMOTE_INPUT4:
				btnInput4.setType(html::ECT_SUCCESS);
				break;
			default:
				break;
		}
	}
	btnInput0.update();
	btnInput1.update();
	btnInput2.update();
	btnInput3.update();
	btnInput4.update();
}


void TPlayer::updateSelectFilterButtons(const music::CAudioValues& values, const app::ECommandAction action) {
	app::TLockGuard<app::TMutex> lock(componentMtx);
	updateSelectFilterButtonsWithNolock(values, action);
}

void TPlayer::updateSelectFilterButtonsWithNolock(const music::CAudioValues& values, const app::ECommandAction action) {
	btnFilter0.setEnabled(values.control);
	btnFilter1.setEnabled(values.control);
	btnFilter2.setEnabled(values.control);
	btnFilter3.setEnabled(values.control);
	btnFilter4.setEnabled(values.control);
	btnFilter5.setEnabled(values.control);
	btnFilter0.setType(html::ECT_DEFAULT);
	btnFilter1.setType(html::ECT_DEFAULT);
	btnFilter2.setType(html::ECT_DEFAULT);
	btnFilter3.setType(html::ECT_DEFAULT);
	btnFilter4.setType(html::ECT_DEFAULT);
	btnFilter5.setType(html::ECT_DEFAULT);
	if (values.control) {
		switch (action) {
			case ECA_REMOTE_FILTER0:
				btnFilter0.setType(html::ECT_PRIMARY);
				break;
			case ECA_REMOTE_FILTER1:
				btnFilter1.setType(html::ECT_PRIMARY);
				break;
			case ECA_REMOTE_FILTER2:
				btnFilter2.setType(html::ECT_PRIMARY);
				break;
			case ECA_REMOTE_FILTER3:
				btnFilter3.setType(html::ECT_PRIMARY);
				break;
			case ECA_REMOTE_FILTER4:
				btnFilter4.setType(html::ECT_PRIMARY);
				break;
			case ECA_REMOTE_FILTER5:
				btnFilter5.setType(html::ECT_PRIMARY);
				break;
			default:
				break;
		}
	}
	btnFilter0.update();
	btnFilter1.update();
	btnFilter2.update();
	btnFilter3.update();
	btnFilter4.update();
	btnFilter5.update();
}


void TPlayer::setupSearchMediaRadios() {
	rdSearchMedia.setOwner(application.webserver());
	rdSearchMedia.setName("SEARCH_RADIO_GROUP_MEDIA");
	rdSearchMedia.setID("rdSearchMedia");
	rdSearchMedia.setStyle(ECS_HTML);

	rdSearchMedia.elements().add("CD · Compact Disk");
	rdSearchMedia.elements().add("HDCD · High Definition CD");
	rdSearchMedia.elements().add("DVD · Digital Versatile Disk");
	rdSearchMedia.elements().add("BD · Blu-ray Disk");
	rdSearchMedia.elements().add("HR · High Resolution Audio");
	rdSearchMedia.elements().add("DSD · Direct Stream Digital");
	rdSearchMedia.elements().add("All media types");

	rdSearchMedia.update("All media types");
}

void TPlayer::setupSearchDomainRadios() {
	rdSearchDomain.setOwner(application.webserver());
	rdSearchDomain.setName("SEARCH_RADIO_GROUP_DOMAIN");
	rdSearchDomain.setID("rdSearchDomain");
	rdSearchDomain.setStyle(ECS_HTML);

	rdSearchDomain.elements().add("Artist & Albumartist");
	rdSearchDomain.elements().add("Album & Songs");
	rdSearchDomain.elements().add("Albumartist");
	rdSearchDomain.elements().add("Composer");
	rdSearchDomain.elements().add("Conductor");
	rdSearchDomain.elements().add("All tags");

	rdSearchDomain.update("All tags");
}

TMediaNames TPlayer::fillMediaNamesMap() {
	TMediaNames map;
	map["cd"]   = music::EMT_CD;
	map["hdcd"] = music::EMT_HDCD;
	map["dsd"]  = music::EMT_DSD;
	map["dvd"]  = music::EMT_DVD;
	map["bd"]   = music::EMT_BD;
	map["hr"]   = music::EMT_HR;
	map["all"]  = music::EMT_ALL;
	return map;
}

TDomainNames TPlayer::fillFilterDomainMap() {
	TDomainNames map;
	map["artist"]      = music::FD_ARTIST;
	map["albumartist"] = music::FD_ALBUMARTIST;
	map["composer"]    = music::FD_COMPOSER;
	map["conductor"]   = music::FD_CONDUCTOR;
	map["album"]       = music::FD_ALBUM;
	map["title"]       = music::FD_TITLE;
	map["all"]         = music::FD_ALL;
	return map;
}

music::EMediaType TPlayer::getMediaType(const std::string& value) const {
	TMediaNames::const_iterator it = mnames.find(util::tolower(value));
	if (it != mnames.end())
		return it->second;
	return music::EMT_UNKNOWN;
}

music::EFilterDomain TPlayer::getFilterDomain(const std::string& value) const {
	TDomainNames::const_iterator it = fdnames.find(util::tolower(value));
	if (it != fdnames.end())
		return it->second;
	return music::FD_UNKNOWN;
}

std::string TPlayer::getMediaShortcut(const music::EMediaType type) const {
	const struct music::TMediaType *it;
	for (it = music::formats; util::assigned(it->description); ++it) {
		if (it->media == type) {
			return it->shortcut;
		}
	}
	return "All";
}

std::string TPlayer::getMediaName(const music::EMediaType type) const {
	const struct music::TMediaType *it;
	for (it = music::formats; util::assigned(it->description); ++it) {
		if (it->media == type) {
			return it->name;
		}
	}
	return "all-audio";
}


void TPlayer::logger(const std::string& text) const {
	if (debug)
		if (debug) aout << text << std::endl;
	application.writeLog(text);
}


void TPlayer::getCurrentPlayerValues(TGlobalState& global) {
	music::PSong current;
	music::EPlayerState state;
	std::string playlist;
	TPlayerMode mode;

	// Set player hardware values
	getCurrentState(current, playlist, state);
	global.player.hardware.song = current;
	global.player.hardware.playlist = playlist;
	global.player.hardware.state = state;

	// Set shuffle mode values
	getCurrentMode(mode);
	global.shuffle = mode;
}

void TPlayer::getRequestedCommandValues(TGlobalState& global) {

	// Get next command from queue
	music::PPlaylist pls = nil;
	music::PSong requested = nil;
	std::string playlist;
	global.interface.command = queue.next();

	// Find next requested song
	if (util::assigned(global.interface.command)) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		playlist = global.interface.command->playlist;
		if (!playlist.empty()) {
			pls = playlists[playlist];
			if (util::assigned(pls)) {
				if (!global.interface.command->file.empty()) {
					requested = pls->findSong(global.interface.command->file);
				}
				if (!util::assigned(requested) && !global.interface.command->album.empty()) {
					requested = pls->findAlbum(global.interface.command->album);
				}
			}
		}
	}

	// Set currently requested values
	// --> Leave values untouched if no new command given (!!!)
	// --> Set state to default value, correct value is set by command processing
	if (util::assigned(global.interface.command)) {
		if (util::assigned(requested)) {
			logger(util::csnprintf("[Request] Request command [%] on song $ for playlist $", actions.getValue(global.interface.command->action), requested->getTitle(), playlist));
		} else {
			if (playlist.empty())
				logger(util::csnprintf("[Request] Command [%] requested, no song and playlist given.", actions.getValue(global.interface.command->action)));
			else
				logger(util::csnprintf("[Request] Command [%] requested, no song for playlist $ given.", actions.getValue(global.interface.command->action), playlist));
		}
		global.player.software.song = requested;
		global.player.software.playlist = playlist;
		global.player.software.state = music::EPS_DEFAULT;
	}
}


void TPlayer::onPlayerPlaylistRequest(const music::TAlsaPlayer& sender, const music::TSong* current, music::TSong*& next, std::string& playlist, bool& reopen) {
	next = nil;

	// Get current repeat mode
	TPlayerMode mode;
	getCurrentMode(mode);
	logger(util::csnprintf("[Select] Current mode for playlist $ is [%], song is %.", playlist, modeToStr(mode), util::assigned(current) ? "valid" : "invalid"));

	if (util::assigned(current)) {
		bool found = false;

		// Get current song value
		std::string list;
		music::CSongData data;
		music::PSong song = getNextSong(data, list);

		// Is current song internet stream?
		if (current->isStreamed()) {
			if (!application.isLicensed()) {
				found = true;
				next = nil;
				logger("[Select] Trial mode, terminate streaming after first song.");
			}
			if (!found) {
				found = true;
				next = const_cast<music::TSong*>(current);
				logger("[Select] Next song is same stream \"" + util::strToStr(next->getTitle(), "-") + "\"");
			}
		}

		// Find next song for given playlist and given repeat mode
		if (!found) {

			// Protect playlist readings
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);

			// Get next enqueued song
			if (!found) {
				if (util::assigned(song)) {

					// Select next song for given playlist
					logger(util::csnprintf("[Select] Play enqueued song $ for playlist $", song->getTitle(), list));
					playlist = list;
					next = song;

					// Touch song for given playlist
					music::PPlaylist pls = playlists[playlist];
					if (util::assigned(pls)) {
						pls->touchAlbum(next->getAlbumHash());
					} else {
						if (playlist.empty())
							logger(util::csnprintf("[Select] Empty playlist on playlist request for song $", current->getTitle()));
						else
							logger(util::csnprintf("[Select] Invalid playlist $ on playlist request for song $", playlist, current->getTitle()));
					}

					clearNextSong();
					found = true;
				}
			}

			// Stop after song finished
			if (!found) {
				if (mode.halt) {
					next = nil;
					logger("[Select] Stop after last song executed.");
					found = true;
				}
			}

			// Repeat same song until doomsday
			if (!found) {
				if (mode.single) {
					found = true;
					next = const_cast<music::TSong*>(current);
					logger("[Select] Next song is same song \"" + util::strToStr(next->getTitle(), "-") + "\" [Single Mode]");
				}
			}

			// Play one song in trial mode
			if (!found) {
				if (!application.isLicensed()) {
					found = true;
					next = nil;
					logger("[Select] Trial mode, no next song chosen.");
				}
			}

			// Select next or random song from playlist
			if (!found) {
				bool transition;
				selectRandomSongWithNolock(mode, playlist, current, next, transition);

				// Set timestamp for album
				if (util::assigned(next)) {
					music::PPlaylist pls = playlists[playlist];
					if (util::assigned(pls)) {
						pls->touchAlbum(next->getAlbumHash());
					} else {
						if (playlist.empty())
							logger(util::csnprintf("[Select] Empty playlist on playlist request for song $", current->getTitle()));
						else
							logger(util::csnprintf("[Select] Invalid playlist $ on playlist request for song $", playlist, current->getTitle()));
					}
				}
			}

		}
	}

	// Trigger task system execution...
	if (util::assigned(next)) {
		notifyEvent("[Select] Playlist requested");
	}

	// Reset stop mode
	if (mode.halt) {
		executeRepeatModeAction(ECA_PLAYER_MODE_HALT);
	}

}


void TPlayer::selectRandomSong(const TPlayerMode& mode, const std::string& playlist, const music::TSong* current, music::TSong*& next, bool& transition) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	selectRandomSongWithNolock(mode, playlist, current, next, transition);
}

void TPlayer::selectRandomSongWithNolock(const TPlayerMode& mode, const std::string& playlist, const music::TSong* current, music::TSong*& next, bool& transition) {
	next = nil;
	transition = false;

	if (!util::assigned(current))
		return;

	// Get current playlist from name
	music::PPlaylist pls = nil;
	if (!playlist.empty()) {
		pls = playlists[playlist];
	} else {
		logger("[Select] No playlist given on select random song.");
		return;
	}

	// Check for valid playlist
	if (!util::assigned(pls)) {
		logger("[Select] Invalid playlist \"" + playlist + "\" on select random song");
		return;
	}

	// Get next song in playlist
	if (!mode.random) {
		music::PTrack track = pls->getTrack(current->getFileHash());
		if (util::assigned(track)) {
			size_t idx = track->getIndex();
			if (idx < util::pred(pls->size())) {
				idx = util::succ(idx);
				music::PSong o = pls->getSong(idx);
				if (util::assigned(o)) {
					logger(util::csnprintf("[Select] Next song in playlist $ at index % is $", playlist, idx, o->getTitle()));
					next = o;
				}
			}
		}
	}

	// Repeat complete album
	if (mode.disk) {
		if (!mode.random) {
			if (util::assigned(next)) {
				if (current->getAlbumHash() == next->getAlbumHash()) {
					if (mode.repeat)
						logger("[Select] Next song for album is \"" + util::strToStr(next->getTitle(), "-") + "\" [Disk Repeat Mode]");
					else
						logger("[Select] Next song for album is \"" + util::strToStr(next->getTitle(), "-") + "\" [Disk Mode]");
					return;
				}
				if (mode.repeat) {
					music::PSong o = pls->findAlbum(current->getAlbumHash());
					if (util::assigned(o)) {
						next = o;
						logger("[Select] First song \"" + util::strToStr(next->getTitle(), "-") + "\" for current album [Disk Repeat Mode]");
						return;
					}
				} else {
					next = nil;
					logger("[Select] Last song \"" + util::strToStr(current->getTitle(), "-") + "\" of current album was played [Disk Mode]");
					return;
				}
			}
		} else {
			// Randomize songs in album index range
			size_t first, last, size;
			if (pls->findAlbumRange(current->getAlbumHash(), first, last, size)) {
				bool repeat;
				do {
					repeat = false;
					size_t left = pls->songsToShuffleLeft(current->getAlbumHash());
					if (left <= 0) {
						if (mode.repeat) {
							pls->clearRandomMarkers();
							transition = true;
							logger("[Select] Cleared random markers [Disk Shuffle Repeat Mode]");
						} else {
							next = nil;
							logger("[Select] Last song of album \"" + util::strToStr(current->getTitle(), "-") + "\" was played [Disk Shuffle Mode]");
							return;
						}
					}
					bool found = false;
					size_t bailout = 0;
					music::PSong song;
					music::PTrack track;
					while (!found && (bailout < (size * 3))) {
						size_t index = util::randomize(first, last);
						if (pls->validIndex(index)) {
							track = pls->getTrack(index);
							if (util::assigned(track)) {
								song = track->getSong();
								if (util::assigned(song)) {
									if (!track->isRandomized()) {
										if (*current != *song) {
											if (song->getDuration() > 100) {
												found = true;
												next = song;
											} else {
												track->setRandomized(true);
											}
										}
									}
								}
							}
						} else {
							next = nil;
							logger(util::csnprintf("[Select] Shuffle index (%) invalid for song $ not found [Disk Shuffle Mode]", index, util::strToStr(current->getTitle(), "-")));
							return;
						}
						++bailout;
					}
					if (found && util::assigned(next)) {
						if (left > 1)
							logger("[Select] Randomized next song \"" + util::strToStr(next->getTitle(), "-") + "\" for album [Disk Shuffle Mode]");
						else
							logger("[Select] Randomized last song \"" + util::strToStr(next->getTitle(), "-") + "\" for album [Disk Shuffle Mode]");
						track = pls->getTrack(next->getFileHash());
						if (util::assigned(track))
							track->setRandomized(true);
						return;
					} else {
						next = nil;
						logger("[Select] No random song found after \"" + util::strToStr(current->getTitle(), "-") + "\" for album [Disk Shuffle Mode]");
						repeat = true;
					}
				} while (repeat);
			} else {
				next = nil;
				logger("[Select] Album for song \"" + util::strToStr(current->getTitle(), "-") + "\" not found [Disk Shuffle Mode]");
				return;
			}
		}
	}

	// Repeat and/or shuffle complete playlist
	if (!mode.random) {
		if (mode.repeat) {
			if (mode.disk) {
				if (!util::assigned(next) && !pls->empty()) {
					music::PSong o = pls->findAlbum(current->getAlbumHash());
					if (util::assigned(o)) {
						next = o;
						logger("[Select] First song of album \"" + util::strToStr(next->getTitle(), "-") + "\" [Disk Repeat Mode]");
						transition = true;
						return;
					}
				}	
			} else {
				if (!util::assigned(next) && !pls->empty()) {
					music::PSong o = pls->getSong(0);
					if (util::assigned(o)) {
						next = o;
						logger("[Select] First song of playlist \"" + util::strToStr(next->getTitle(), "-") + "\" [Playlist Repeat Mode]");
						transition = true;
						return;
					}
				}
			}
		}
	} else {
		// Playlist shuffle mode
		size_t size = pls->size();
		size_t left = pls->songsToShuffleLeft();
		if (left <= 0) {
			if (mode.repeat) {
				pls->clearRandomMarkers();
				transition = true;
				logger("[Select] Cleared random markers [Playlist Shuffle Mode]");
			} else {
				next = nil;
				logger("[Select] Last song of playlist \"" + util::strToStr(current->getTitle(), "-") + "\" was played [Playlist Shuffle Mode]");
				return;
			}
		}
		bool found = false;
		size_t bailout = 0;
		music::PSong song;
		music::PTrack track;
		while (!found && (bailout < (size * 3))) {
			size_t index = util::randomize(0, size-1);
			if (pls->validIndex(index)) {
				track = pls->getTrack(index);
				if (util::assigned(track)) {
					song = track->getSong();
					if (util::assigned(song)) {
						if (*current != *song) {
							if (song->getDuration() > 100) {
								found = true;
								next = song;
							} else {
								track->setRandomized(true);
							}
						}
					}
				}
			} else {
				next = nil;
				logger(util::csnprintf("[Select] Shuffle index (%) invalid for song $ not found [Playlist Shuffle Mode]", index, util::strToStr(current->getTitle(), "-")));
				return;
			}
			++bailout;
		}
		if (found && util::assigned(next)) {
			if (left > 1)
				logger("[Select] Randomized next song \"" + util::strToStr(next->getTitle(), "-") + "\" for playlist [Playlist Shuffle Mode]");
			else
				logger("[Select] Randomized last song \"" + util::strToStr(next->getTitle(), "-") + "\" for playlist [Playlist Shuffle Mode]");
			track = pls->getTrack(next->getFileHash());
			if (util::assigned(track))
				track->setRandomized(true);
			return;
		}
	}

	// Return next song in playlist
	if (util::assigned(next)) {
		logger("[Select] Next song in playlist \"" + util::strToStr(next->getTitle(), "-") + "\"");
		return;
	}

	logger("[Select] No song found to play next.");
	return;
}


void TPlayer::setCurrentState(const music::TSong* song, const std::string& playlist, const music::EPlayerState state) {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	if (util::assigned(currentSong.song)) {
		lastSong = currentSong;
	}
	if (util::assigned(song)) {
		currentSong.song = const_cast<music::PSong>(song);
		currentSong.playlist = playlist;
	} else {
		currentSong.clear();
	}
	currentState = state;
}

void TPlayer::getCurrentState(music::TSong*& song, std::string& playlist, music::EPlayerState& state) {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	state = currentState;
	song = currentSong.song;
	playlist = currentSong.playlist;
}

void TPlayer::getCurrentState(music::CSongData& song, music::EPlayerState& state) {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	getSongProperties(currentSong.song, song);
	state = currentState;
}

void TPlayer::getCurrentState(music::EPlayerState& state) {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	state = currentState;
}

bool TPlayer::isPlaying() {
	music::EPlayerState state;
	getCurrentState(state);
	return util::isMemberOf(state, music::EPS_PLAY,music::EPS_WAIT,music::EPS_REOPEN,music::EPS_PAUSE,music::EPS_HALT);
}

bool TPlayer::isStopped() {
	music::EPlayerState state;
	getCurrentState(state);
	return util::isMemberOf(state, music::EPS_CLOSED,music::EPS_IDLE,music::EPS_STOP,music::EPS_ERROR);
}

bool TPlayer::isClosed() {
	music::EPlayerState state;
	getCurrentState(state);
	return music::EPS_CLOSED == state;
}

bool TPlayer::isStreamable() {
	music::PSong song;
	std::string playlist;
	music::EPlayerState state;
	getCurrentState(song, playlist, state);
	if (util::isMemberOf(state, music::EPS_CLOSED,music::EPS_IDLE,music::EPS_STOP)) {
		return true;
	}
	if (util::assigned(song)) {
		if (song->isStreamed())
			return true;
	}
	return false;
}

void TPlayer::getCurrentSongs(music::TCurrentSongs& songs) {
	// Lock next and current song mutex in one step
	std::lock(currentSongMtx, nextSongMtx);
	std::lock_guard<std::mutex> lock1(currentSongMtx, std::adopt_lock);
	std::lock_guard<std::mutex> lock2(nextSongMtx, std::adopt_lock);
	songs.playlist = currentSong.playlist;
	if (songs.playlist.empty())
		songs.playlist = nextSong.playlist;
	songs.last = lastSong.song;
	songs.current = currentSong.song;
	songs.next = nextSong.song;
}

void TPlayer::getSongProperties(const music::TSong* song, music::CSongData& data) {
	data.clear();
	if (util::assigned(song)) {
		data.artist = song->getArtist();
		data.albumartist = song->getAlbumArtist();
		data.album = song->getAlbum();
		data.title = song->getTitle();

		data.track = song->getTrackNumber();
		data.disk = song->getDiskNumber();
		data.tracks = song->getTrackCount();
		data.disks = song->getDiskCount();

		data.titleHash = song->getTitleHash();
		data.albumHash = song->getAlbumHash();
		data.artistHash = song->getArtistHash();
		data.albumartistHash = song->getAlbumArtistHash();
		data.fileHash = song->getFileHash();

		data.duration = song->getDuration();
		data.played = song->getPlayed();
		data.progress = song->getPercent();

		data.index = song->getIndex();
		data.streamable = song->isStreamed();
		data.valid = true;
	}
}

music::PSong TPlayer::getSongData(const music::TCurrentSong& song, music::CSongData& data, std::string& playlist) {
	getSongProperties(song.song, data);
	playlist = song.playlist;
	return song.song;
}

music::PSong TPlayer::getPlayedSong(music::CSongData& song, std::string& playlist) {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	music::PSong o = getSongData(currentSong, song, playlist);
	if (!song.valid)
		o = getSongData(lastSong, song, playlist);
	return o;
}

music::PSong TPlayer::getCurrentSong(music::CSongData& song, std::string& playlist) {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	return getSongData(currentSong, song, playlist);
}

music::PSong TPlayer::getCurrentSong() {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	return currentSong.song;
}

void TPlayer::clearCurrentSong() {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	currentSong.clear();
}


music::PSong TPlayer::getLastSong(music::CSongData& song, std::string& playlist) {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	return getSongData(lastSong, song, playlist);
}

music::PSong TPlayer::getLastSong() {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	return lastSong.song;
}

void TPlayer::clearLastSong() {
	std::lock_guard<std::mutex> lock(currentSongMtx);
	lastSong.clear();
}

void TPlayer::clearNextSong() {
	std::lock_guard<std::mutex> lock(nextSongMtx);
	nextSong.clear();
}

void TPlayer::setNextSong(const music::TSong* song, const std::string& playlist) {
	std::lock_guard<std::mutex> lock(nextSongMtx);
	nextSong.song = const_cast<music::PSong>(song);
	nextSong.playlist = playlist;
}

music::PSong TPlayer::getNextSong(music::CSongData& song, std::string& playlist) {
	std::lock_guard<std::mutex> lock(nextSongMtx);
	return getSongData(nextSong, song, playlist);
}

music::PSong TPlayer::getNextSong() {
	std::lock_guard<std::mutex> lock(nextSongMtx);
	return nextSong.song;
}


void TPlayer::clearSelectedSong() {
	std::lock_guard<std::mutex> lock(selectedSongMtx);
	selectedSong.clear();
}

void TPlayer::setSelectedSong(const music::TSong* song, const std::string& playlist) {
	std::lock_guard<std::mutex> lock(selectedSongMtx);
	selectedSong.song = const_cast<music::PSong>(song);
	selectedSong.playlist = playlist;
	if (util::assigned(selectedSong.song))
		logger(util::csnprintf("[Event] [Select] Select file hash $ for playlist $", selectedSong.song->getFileHash(), playlist));
	else
		logger("[Event] [Select] Selected song is undefined.");
}

music::PSong TPlayer::getSelectedSong(music::CSongData& song, std::string& playlist) {
	std::lock_guard<std::mutex> lock(selectedSongMtx);
	return getSongData(selectedSong, song, playlist);
}


music::ESampleRate TPlayer::getNextRate() {
	std::lock_guard<std::mutex> lock(rateMtx);
	return nextRate;
}

music::ESampleRate TPlayer::getAndClearNextRate() {
	std::lock_guard<std::mutex> lock(rateMtx);
	music::ESampleRate rate = nextRate;
	nextRate = music::SR0K;
	return rate;
}

void TPlayer::setNextRate(const music::ESampleRate rate) {
	std::lock_guard<std::mutex> lock(rateMtx);
	nextRate = rate;
}

void TPlayer::clearNextRate() {
	std::lock_guard<std::mutex> lock(rateMtx);
	nextRate = music::SR0K;
}

std::string TPlayer::normalizePlaylistHeader(const std::string& header) {
	if (!header.empty()) {
		size_t pos = header.find_first_of('(');
		if (std::string::npos != pos) {
			if (pos > 2)
				return header.substr(0, util::pred(pos));
		}
		return header;
	}
	return "";
}

void TPlayer::setPlaylistHeader(const std::string& header, const std::string& playlist, const size_t count) {
	if (util::assigned(wtPlaylistHeader)) {
		std::string text;
		std::string title = normalizePlaylistHeader(header);
		if (debug) aout << "TPlayer::setPlaylistHeader() Set playlist \"" << title << "\" size = " << count << std::endl;
		bool ok = false;
		if (!ok && title.empty()) {
			text = "No playlist selected...";
			ok = true;
		}
		if (!ok && count <= 0) {
			text = title + " is empty";
			ok = true;
		}
		if (!ok && count == 1) {
			text = title + " (1 track)";
			ok = true;
		}
		if (!ok) {
			text = util::csnprintf("% (% tracks)", title, count);
			ok = true;
		}
		if (ok) {
			wtPlaylistHeader->setValue(text, true);
		}
		if (playlist.empty()) {
			wtPlaylistExportUrl->setValue(EXPORT_REST_URL, true);
			wtPlaylistExportName->setValue("noname.m3u", true);
		} else {
			// wtPlaylistExportUrl->setValue(std::string(EXPORT_REST_URL) + "?playlist=" + playlist, true);
			wtPlaylistExportUrl->setValue("/rest/m3u/" + util::sanitizeFileName(playlist) + ".m3u?playlist=" + playlist, true);
			wtPlaylistExportName->setValue("Playlist '" + playlist + "'" + ".m3u", true);
		}
	}
}

void TPlayer::setRecentHeader(const size_t count) {
	if (util::assigned(wtRecentHeader)) {
		std::string text;
		if (debug) aout << "TPlayer::setRecentHeader() Set recent song list size = " << count << std::endl;
		bool ok = false;
		if (!ok && count <= 0) {
			text = "No recent tracks found";
			ok = true;
		}
		if (!ok && count == 1) {
			text = "Recent song (1 track)";
			ok = true;
		}
		if (!ok) {
			text = util::csnprintf("Recent songs (% tracks)", count);
			ok = true;
		}
		if (ok) {
			wtRecentHeader->setValue(text, true);
		}
	}
}

#ifndef STL_HAS_LAMBDAS
std::string getFirstWord(const std::string& text) {
	if (!text.empty()) {
		size_t pos = text.find_first_of('-');
		if (std::string::npos != pos) {
			return text.substr(0, pos);
		}
	}
	return "";
}
#endif

void TPlayer::getMediaIcon(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

#ifdef STL_HAS_LAMBDAS
	auto getFirstWord = [] (const std::string& text) -> std::string {
		if (!text.empty()) {
			size_t pos = text.find_first_of('-');
			if (std::string::npos != pos) {
				return text.substr(0, pos);
			}
		}
		return "";
	};
#endif

	// Get icon properties from URL:
	//   File URL e.g. "http://localhost/rest/icons/cd-audio.jpg"
	//   File base name like "/rest/icons/cd-audio.jpg"
	std::string url = params[URI_REQUEST_URL].asString();
	std::string name = util::tolower(util::fileBaseName(url));
	std::string word = getFirstWord(name);
	size_t length = word.size();
	if (length > 2) {
		word = word.substr(0, 2);
	}

	if (debug) {
		params.debugOutput("TPlayer::getMediaIcon() ", "  ");
		aout << "TPlayer::getMediaIcon() Requested URL        : " << url << endl;
		aout << "TPlayer::getMediaIcon() Requested base name  : " << name << endl;
	}

	switch (length) {
		case 2:
			if (word == "cd") {
				data = music::CD_AUDIO_JPG;
				size = util::sizeOfArray(music::CD_AUDIO_JPG);
				return;
			}
			if (word == "bd") {
				data = music::BD_AUDIO_JPG;
				size = util::sizeOfArray(music::BD_AUDIO_JPG);
				return;
			}
			if (word == "hr") {
				data = music::HR_AUDIO_JPG;
				size = util::sizeOfArray(music::HR_AUDIO_JPG);
				return;
			}
			break;
		case 3:
			if (word == "dv") { // "dvd"
				data = music::DVD_AUDIO_JPG;
				size = util::sizeOfArray(music::DVD_AUDIO_JPG);
				return;
			}
			if (word == "ds") { // "dsd"
				data = music::DSD_AUDIO_JPG;
				size = util::sizeOfArray(music::DSD_AUDIO_JPG);
				return;
			}
			break;
		case 4:
			if (word == "sa") { // "sacd"
				data = music::SACD_AUDIO_JPG;
				size = util::sizeOfArray(music::SACD_AUDIO_JPG);
				return;
			}
			if (word == "hd") { // "hdcd"
				data = music::HDCD_AUDIO_JPG;
				size = util::sizeOfArray(music::HDCD_AUDIO_JPG);
				return;
			}
			break;
	}

	// Return default picture
	data = music::CD_AUDIO_JPG;
	size = util::sizeOfArray(music::CD_AUDIO_JPG);
}


void TPlayer::getCoverArt(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	bool _debug = debug;
	//_debug = true;

	// Get cover art properties from URL:
	//   File base name like <album hash>-<size>.jpg&file=<file hash>
	//   File URL http://localhost/rest/thumbnails/1062d3e999d94b808d49c0c214d00c7c-200.jpg&file=1165549464be41f80fe0b8b08404b7da
	//                                            -123456789-123456789-123456789-12345
	std::string url = params[URI_REQUEST_URL].asString();
	std::string file = params["file"].asString();
	std::string name = util::fileBaseName(url);
	std::string ext = util::tolower(util::fileExt(url));

	if (_debug) {
		params.debugOutput("TPlayer::getCoverArt() ", "  ");
		aout << "TPlayer::getCoverArt() Requested URL        : " << url << endl;
		aout << "TPlayer::getCoverArt() Requested file name  : " << file << endl;
		aout << "TPlayer::getCoverArt() Requested base name  : " << name << endl;
	}

	// Invalid file name?
	if (name.size() < 34)
		return;
	if (name[32] != '-')
		return;

	// Read album hash value and picture dimension from file name
	std::string hash = name.substr(0, 32);
	size_t dimension = util::strToUnsigned(name.substr(33));
	bool special = "00000000000000000000000000000001" == hash;

	if (_debug) {
		aout << "TPlayer::getCoverArt() Requested dimension  : " << dimension << endl;
		aout << "TPlayer::getCoverArt() Requested album hash : " << hash << endl;
		aout << "TPlayer::getCoverArt() Requested file hash  : " << strToStr(file) << endl;
		aout << "TPlayer::getCoverArt() Requested special    : " << special << endl;
	}

	// Invalid dimension parameter?
	if (dimension <= 0)
		return;

	// Check if thumbnail for current song requested
	bool caching = true;
	if (!file.empty() && dimension <= 32) {
		if (special) {
			bool ok = false;
			if (!ok && ext == "png") {
				data = music::PLAY_PNG;
				size = util::sizeOfArray(music::PLAY_PNG);
				ok = true;
			}
			if (ok) {
				// Do not cache explicitly requested file thumbs
				cached = false;
				caching = false;
				zipped = false;
				if (_debug) aout << "TPlayer::getCoverArt() Thumbnail for active radio station requested." << endl;
				return;
			}
		} else {
			if (player.isPlaying()) {
				music::TCurrentSong current;
				player.getCurrentSong(current);
				if (util::assigned(current.song) && !current.playlist.empty()) {
					app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
					music::PPlaylist pls = playlists[current.playlist];
					if (util::assigned(pls)) {
						music::PTrack playing = pls->findTrack(current.song->getFileHash());
						music::PTrack requested = pls->findTrack(file);
						if (util::assigned(playing) && util::assigned(requested)) {
							if (playing->getIndex() == requested->getIndex()) {
								bool ok = false;
								if (!ok && ext == "jpg") {
									data = music::PLAY_JPG;
									size = util::sizeOfArray(music::PLAY_JPG);
									ok = true;
								}
								if (!ok && ext == "jpeg") {
									data = music::PLAY_JPG;
									size = util::sizeOfArray(music::PLAY_JPG);
									ok = true;
								}
								if (!ok && ext == "png") {
									data = music::PLAY_PNG;
									size = util::sizeOfArray(music::PLAY_PNG);
									ok = true;
								}
								if (ok) {
									// Do not cache explicitly requested file thumbs
									cached = false;
									caching = false;
									zipped = false;
									if (_debug && util::assigned(playing)) aout << "TPlayer::getCoverArt() Song playing : " << playing->getIndex() << endl;
									return;
								}
							}
						} else {
							if (_debug && !util::assigned(playing)) aout << "TPlayer::getCoverArt() No song found for playing hash : " << current.song->getFileHash() << endl;
							if (_debug && !util::assigned(requested)) aout << "TPlayer::getCoverArt() No song found for requested hash : " << file << endl;
						}
					} else {
						if (current.playlist.empty())
							logger("TPlayer::getCoverArt() Empty playlist returned from ALSA player.");
						else
							logger(util::csnprintf("TPlayer::getCoverArt() Invalid playlist $ returned from ALSA player.", current.playlist));
					}
				} else {
					if (_debug) aout << "TPlayer::getCoverArt() No song playing." << endl;
				}
			} else {
				if (_debug) aout << "TPlayer::getCoverArt() Player stopped or idle." << endl;
			}
		}
	}

	// Get cached cover thumbnail
	std::string fileName = getCachedFileName(hash, dimension, coverCache);
	if (!util::fileExists(fileName)) {
		// File not yet cached, read thumbnail file from album content folder
		std::string folder;
		{
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
			folder = library.path(hash);
		}
		fileName = getThumbFile(hash, folder, dimension, coverCache, special);
	}

	if (getThumbFromCache(fileName, data, size)) {
		// Add caching headers to response
		if (caching)
			cached = true;
	}

	if (_debug) aout << "TPlayer::getCoverArt() Requested picture    : <" << fileName << "> size = " << size << " cached = " << cached << endl;
}


void TPlayer::getThumbNail(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	std::string hash   = params["hash"].asString();
	std::string folder = params["url"].asString();
	size_t dimension   = params["size"].asInteger(0); // 0 = Original size!

	if (debug) {
		aout << "TPlayer::getThumbNail() Requested hash : " << hash << endl;
		aout << "TPlayer::getThumbNail() Requested URL  : " << folder << endl;
		aout << "TPlayer::getThumbNail() Requested size : " << dimension << endl;
	}

	// Get cached cover thumbnail
	bool special = "00000000000000000000000000000001" == hash;
	std::string fileName = (dimension > 0) ? getThumbFile(hash, folder, dimension, coverCache, special) : findFileName(folder, "folder.jpg");
	cached = getThumbFromCache(fileName, data, size);

	if (debug) aout << "TPlayer::getThumbNail()     Served file name : " << fileName << endl;
}


bool TPlayer::getThumbFromCache(const std::string& fileName, const void*& data, size_t& size) {
	bool retVal = false;
	bool _debug = debug;
	// _debug = true;
	if (!fileName.empty()) {
		// Check if file in cached file map
		bool added = false;
		app::TReadWriteGuard<app::TReadWriteLock> lock(thumbnailLck, RWL_READ);
		TFileCacheMap::const_iterator it = fileCache.find(fileName);
		if (it == fileCache.end()) {
			// Release read lock and aquire write lock
			lock.unlock();
			PFile o = new TFile(fileName);
			util::TObjectGuard<TFile> guard(&o);
			if (o->exists()) {
				o->load();
				lock.wrLock();
				fileCache[fileName] = o;
				o = nil;
				added = true;
				it = fileCache.find(fileName);
				if (_debug) aout << "TPlayer::getThumbFromCache()   Add file " << fileName << " to memory cache." << endl;
			} else {
				if (_debug) aout << "TPlayer::getThumbFromCache()   File " << fileName << " does not exists." << endl;
			}
		}
		if (it != fileCache.end()) {
			PFile o = it->second;
			if (util::assigned(o)) {
				data = o->getData();
				size = o->getSize();
				retVal = true;
			}
			if (!added && retVal && _debug) {
				aout << "TPlayer::getThumbFromCache()   Using memory cached file " << fileName << endl;
			}
		}
	}
	return retVal;
}


std::string TPlayer::getCachedFileName(const std::string& fileHash, const size_t dimension, const std::string& cache) {

	// Look for file in cache
	std::string file;
	switch (dimension) {
		case 32:
			file = cache + "32x32/" + fileHash + ".jpg";
			break;
		case 48:
			file = cache + "48x48/" + fileHash + ".jpg";
			break;
		case 72:
			file = cache + "72x72/" + fileHash + ".jpg";
			break;
		case 200:
			file = cache + "200x200/" + fileHash + ".jpg";
			break;
		case 400:
			file = cache + "400x400/" + fileHash + ".jpg";
			break;
		default:
			file = cache + "600x600/" + fileHash + ".jpg";
			break;
	}

	return file;
}

std::string TPlayer::getNocoverFileName(const size_t dimension, const std::string& cache) {

	// Look for file in cache
	std::string file;
	if (application.hasWebServer()) {
		std::string root = application.getWebServer().getWebRoot();
		switch (dimension) {
			case 32:
				file = root + "images/nocover32.jpg";
				break;
			case 48:
				file = root + "images/nocover48.jpg";
				break;
			case 72:
				file = root + "images/nocover72.jpg";
				break;
			case 200:
				file = root + "images/nocover200.jpg";
				break;
			case 400:
				file = root + "images/nocover400.jpg";
				break;
			default:
				file = root + "images/nocover600.jpg";
				break;
		}
	}
	return file;
}

std::string TPlayer::getSpecialFileName(const size_t dimension, const std::string& cache) {

	// Look for file in cache
	std::string file;
	if (application.hasWebServer()) {
		std::string root = application.getWebServer().getWebRoot();
		switch (dimension) {
			case 32:
				file = root + "images/radio32.jpg";
				break;
			case 48:
				file = root + "images/radio48.jpg";
				break;
			case 72:
				file = root + "images/radio72.jpg";
				break;
			case 200:
				file = root + "images/radio200.jpg";
				break;
			case 400:
				file = root + "images/radio400.jpg";
				break;
			default:
				file = root + "images/radio600.jpg";
				break;
		}
	}
	return file;
}


void TPlayer::deleteCachedFiles(const std::string& fileHash) {
	std::string file;
	file = getCachedFileName(fileHash, 32, coverCache);
	deleteCachedImages(file);
	file = getCachedFileName(fileHash, 48, coverCache);
	deleteCachedImages(file);
	file = getCachedFileName(fileHash, 72, coverCache);
	deleteCachedImages(file);
	file = getCachedFileName(fileHash, 200, coverCache);
	deleteCachedImages(file);
	file = getCachedFileName(fileHash, 400, coverCache);
	deleteCachedImages(file);
	file = getCachedFileName(fileHash, 600, coverCache);
	deleteCachedImages(file);
}


void TPlayer::createCacheFolder(const std::string& cacheFolder) {
	if (!util::folderExists(cacheFolder))
		util::createDirektory(cacheFolder);
	if (!util::folderExists(cacheFolder))
		throw util::app_error("Cache folder <" + cacheFolder + "> could not be created.");
}

void TPlayer::deleteCacheFolder(const std::string& cacheFolder, const util::ESearchDepth depth) {
	if (util::folderExists(cacheFolder)) {
		std::string files = util::validPath(cacheFolder) + "*.jpg";
		int r = util::deleteFiles(files, depth, false);
		if (r >= 0)
			logger(util::csnprintf("[Command] [Cache] Deleted % files.", r));
		else
			logger(util::csnprintf("[Command] [Cache] Delete failed for <%>", files));
	}
}

void TPlayer::createCacheFolders(const std::string& cachePath) {
	std::string coverPath = cachePath;
	createCacheFolder(coverPath);
	coverPath = cachePath + "32x32";
	createCacheFolder(coverPath);
	coverPath = cachePath + "48x48";
	createCacheFolder(coverPath);
	coverPath = cachePath + "72x72";
	createCacheFolder(coverPath);
	coverPath = cachePath + "200x200";
	createCacheFolder(coverPath);
	coverPath = cachePath + "400x400";
	createCacheFolder(coverPath);
	coverPath = cachePath + "600x600";
	createCacheFolder(coverPath);
}


void TPlayer::deleteCachedImages(const std::string& fileName) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(thumbnailLck, RWL_WRITE);
	util::deleteFile(fileName);
	TFileCacheMap::const_iterator it = fileCache.find(fileName);
	if (it != fileCache.end()) {
		util::PFile o = it->second;
		util::freeAndNil(o);
		fileCache.erase(it);
	}
}

void TPlayer::invalidateFileCache() {
	app::TReadWriteGuard<app::TReadWriteLock> lock(thumbnailLck, RWL_WRITE);
	logger("[Command] [Cache] Invalidate cache called.");
	deleteCacheFolder(coverCache, util::SD_RECURSIVE);
	fileCache.clear();
}


std::string TPlayer::getThumbFile(const std::string& fileHash, const std::string& folder, const size_t dimension, const std::string& cache, const bool special) {
	util::TJpeg cover;
	return getThumbFile(cover, fileHash, folder, dimension, cache, special);
}

std::string TPlayer::getThumbFile(util::TJpeg& thumbnail, const std::string& fileHash, const std::string& folder, const size_t dimension, const std::string& cache, const bool special) {
	try {
		// Check prerequisites
		if (dimension <= 0)
			return "";

		// Checked for cached file
		std::string file = getCachedFileName(fileHash, dimension, cache);
		if (util::fileExists(file)) {
			if (debug) aout << "TPlayer::getThumbFile() Using cached thumbnail file <" << file << ">" << endl;
			return file;
		}

		// Find dedicated thumbnail file in folder
		std::string picture = findFileName(folder, "folder.jpg");
		if (!picture.empty()) {
			if (debug) aout << "TPlayer::getThumbFile() Using disk coverart file <" << picture << ">" << endl;
			try {
				// File locked by filesystem...
				app::TReadWriteGuard<app::TReadWriteLock> lock(thumbnailLck, RWL_READ);
				thumbnail.loadFromFile(picture);
			} catch (const std::exception& e)	{
				string sExcept = e.what();
				logger("TPlayer::getThumbFile() Exception reading coverart as JPEG from file <" + picture + "> \"" + sExcept + "\"");
			} catch (...)	{
				logger("TPlayer::getThumbFile() Unknown exception reading coverart as JPEG from file <" + picture + ">");
			}
		}

		// No cover art file found in folder, try to load image from embedded metadata
		if (sound.getUseMetadata() && !thumbnail.hasImage()) {
			music::PSong o = nil;
			if (debug) aout << "TPlayer::getThumbFile() Load metadata picture for <" << fileHash << ">" << endl;
			{ // Protect library access
				app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
				o = library.findAlbum(fileHash);
			}
			if (util::assigned(o)) {
				music::TCoverData cover;
				std::string song = o->getFileName();
				try {
					if (o->readPictureData(song, cover)) {
						bool ok = false;
						util::TImage::EImageType type = getImageType(cover.artwork);
						if (!ok && type == util::TImage::IMG_JPEG) {
							ok = true;
							if (debug) aout << "TPlayer::getThumbFile() Using JPEG metadata picture for <" << song << ">" << endl;
							thumbnail.decode(cover.artwork);
							if (thumbnail.hasImage()) {
								logger(util::csnprintf("[Metadata] Decoded JPEG metadata for file $ (%x% to %x% pixel)", song, thumbnail.width(), thumbnail.height(), dimension, dimension));
							}
						}
						if (!ok && type == util::TImage::IMG_PNG) {
							ok = true;
							if (debug) aout << "TPlayer::getThumbFile() Using PNG metadata picture for <" << song << ">" << endl;
							// Convert PNG image data to JPEG
							util::TPNG png;
							png.decode(cover.artwork);
							thumbnail.move(png);
							if (thumbnail.hasImage()) {
								logger(util::csnprintf("[Metadata] Decoded PNG metadata for file $ (%x% to %x% pixel)", song, thumbnail.width(), thumbnail.height(), dimension, dimension));
							}
						}
						if (!ok) {
							if (debug) aout << "TPlayer::getThumbFile() Unknown picture format for <" << song << ">" << endl;
							logger(util::csnprintf("[Metadata] Unknown picture format for file $", song));
						}
					}
				} catch (const std::exception& e)	{
					string sExcept = e.what();
					logger("TPlayer::getThumbFile() Exception when reading metadata from file <" + song + "> \"" + sExcept + "\"");
				} catch (...)	{
					logger("TPlayer::getThumbFile() Unknown exception when reading metadata from file <" + song + ">");
				}
			}
		}

		// Check if default "nocover.jpg" file needed
		bool nocover = false;
		if (!thumbnail.hasImage()) {
			picture = special ? getSpecialFileName(dimension, cache) : getNocoverFileName(dimension, cache);
			if (!picture.empty()) {
				if (debug) aout << "TPlayer::getThumbFile() Using default thumbnail file <" << picture << ">" << endl;
				try {
					thumbnail.loadFromFile(picture);
					nocover = true;
				} catch (const std::exception& e)	{
					string sExcept = e.what();
					logger("TPlayer::getThumbFile() Exception on loading default thumbnail file <" + picture + "> \"" + sExcept + "\"");
				} catch (...)	{
					logger("TPlayer::getThumbFile() Unknown exception on loading default thumbnail file <" + picture + ">");
				}
			}
		}

		// Load picture from file
		if (thumbnail.hasImage()) {
			// Create thumbnail and save JPG in cache folder
			try {
				if (dimension < 100) {
					thumbnail.setScalingMethod(ESM_SIMPLE);
					thumbnail.scale(dimension, CL_WHITE);
				} else {
					if (dimension < thumbnail.height()) {
						thumbnail.setScalingMethod(ESM_DITHER);
						if (thumbnail.scale(dimension, CL_WHITE)) {
							thumbnail.contrast();
						}
					} else {
						thumbnail.setScalingMethod(ESM_BILINEAR);
						if (thumbnail.scale(dimension, CL_WHITE)) {
							thumbnail.blur(0.80);
							thumbnail.contrast();
						}
					}
				}

				// Adjust saturation for valid cover arts
				if (!nocover) {
					int range = thumbnail.getColorRange();
					if (range < 50) {
						thumbnail.saturation(1.3);
						//logger("TPlayer::getThumbFile() Adjusted color saturation from " + std::to_string((size_u)range) + "% for file <" + picture + ">");
					}
				}

				// Save preview picture as thumnail
				app::TReadWriteGuard<app::TReadWriteLock> lock(thumbnailLck, RWL_READ);
				thumbnail.saveToFile(file);

				return file;

			} catch (const std::exception& e)	{
				string sExcept = e.what();
				logger("TPlayer::getThumbFile() Exception on create thumbnail JPEG file <" + file + "> \"" + sExcept + "\"");
			} catch (...)	{
				logger("TPlayer::getThumbFile() Unknown exception on create thumbnail JPEG file <" + file + ">");
			}
		}

	} catch (const std::exception& e)	{
		string sExcept = e.what();
		logger("TPlayer::getThumbFile() Exception \"" + sExcept + "\"");
	} catch (...)	{
		logger("TPlayer::getThumbFile() Unknown exception.");
	}

	return "";
}


std::string TPlayer::findFileName(const std::string& path, const std::string& file) {
	if (!path.empty() && !file.empty()) {

		// Check if given file exists in folder
		std::string folder = util::validPath(path);
		std::string fileName = folder + file;
		if (util::fileExists(fileName))
			return fileName;

		// Find first file with given file extension, but other base name
		std::string ext = util::fileExt(file);
		if (!ext.empty()) {
			util::TFolderList files;
			files.scan(folder, "*." + ext, util::SD_ROOT);
			if (!files.empty()) {
				util::PFile o = files[0];
				if (util::assigned(o)) {
					return o->getFile();
				}
			}
		}
	}
	return "";
}

util::TImage::EImageType TPlayer::getImageType(const music::TArtwork& cover) {
	const util::TRGB* image = (const util::TRGB*)cover.data();
	const size_t size = cover.size();
	if (util::TJpeg::isJPEG(image, size))
		return util::TImage::IMG_JPEG;
	if (util::TPNG::isPNG(image, size))
		return util::TImage::IMG_PNG;
	return util::TImage::IMG_UNKOWN;
}



void TPlayer::getResponse(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Return default response
	TDateTime current(EDT_LONG);
	util::TVariantValues response;
	response.add("Referrer", session["HTML_QUERY_TITLE"].asString());
	response.add("Timestamp", current.asString());

	// Build JSON response
	jsonResponse = response.asJSON().text();
	if (!jsonResponse.empty()) {
		data = jsonResponse.c_str();
		size = jsonResponse.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getResponse() JSON = " << jsonResponse << app::reset << endl;
}

void TPlayer::getEditStation(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	jsonEditStation.clear();

	// Get stream hash from parameters
	std::string hash = params["value"].asString();

	{ // Get station data
		app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_READ);
		radio::PRadioStream station = stations.find(hash);
		if (util::assigned(station)) {

			// Return station as JSON
			util::TVariantValues response;
			response.add("Name", station->values.name);
			response.add("URL", station->values.url);
			response.add("Mode", station->values.mode);

			// Build JSON response
			jsonEditStation = response.asJSON().text();
		}
	}

	if (!jsonEditStation.empty()) {
		data = jsonEditStation.c_str();
		size = jsonEditStation.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getEditStation() JSON = " << jsonEditStation << app::reset << endl;
}	

void TPlayer::getCurrentPlaylists(std::string& selected, util::TStringList& names) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
	selected = playlists.getSelected();
	playlists.getNames(names);
}

void TPlayer::getCreatePlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	htmlCreatePlaylist = "<h3><strong><i>Under construction...</i></strong></h3><h4>Can't create playlist.</h4>";
	if (!htmlCreatePlaylist.empty()) {
		data = htmlCreatePlaylist.c_str();
		size = htmlCreatePlaylist.size();
	}
	if (debug) aout << app::yellow << "TPlayer::getCreatePlaylist() HTML = " << htmlCreatePlaylist << app::reset << endl;
}

void TPlayer::getSelectPlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	TStringList names;
	std::string selected;
	getCurrentPlaylists(selected, names);

	app::TLockGuard<app::TMutex> lock(plsDisplayMtx, false);
	if (htmlSelectPlaylist.empty()) {
		html::TRadioButtons radios;
		radios.assign(names);
		htmlSelectPlaylist = "<h4>Select current playlist:</h4>\n" + radios.html(selected);
	}
	if (!htmlSelectPlaylist.empty()) {
		data = htmlSelectPlaylist.c_str();
		size = htmlSelectPlaylist.size();
	}
	if (debug) aout << app::yellow << "TPlayer::getSelectPlaylist() HTML = " << htmlSelectPlaylist << app::reset << endl;
}

void TPlayer::getRenamePlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	TStringList names;
	std::string selected;
	getCurrentPlaylists(selected, names);

	app::TLockGuard<app::TMutex> lock(plsDisplayMtx, false);
	if (htmlRenamePlaylist.empty()) {
		html::TRadioButtons radios;
		radios.assign(names);
		htmlRenamePlaylist = "<h4>Playlist to rename:</h4>\n" + radios.html(selected);
	}
	if (!htmlRenamePlaylist.empty()) {
		data = htmlRenamePlaylist.c_str();
		size = htmlRenamePlaylist.size();
	}
	if (debug) aout << app::yellow << "TPlayer::getRenamePlaylist() HTML = " << htmlRenamePlaylist << app::reset << endl;
}

void TPlayer::getDeletePlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	TStringList names;
	std::string selected;
	getCurrentPlaylists(selected, names);

	app::TLockGuard<app::TMutex> lock(plsDisplayMtx, false);
	if (htmlDeletePlaylist.empty()) {
		html::TRadioButtons radios;
		radios.assign(names);
		htmlDeletePlaylist = "<h4>Select playlist to delete:</h4>\n" + radios.html(selected);
	}
	if (!htmlDeletePlaylist.empty()) {
		data = htmlDeletePlaylist.c_str();
		size = htmlDeletePlaylist.size();
	}
	if (debug) aout << app::yellow << "TPlayer::getDeletePlaylist() HTML = " << htmlDeletePlaylist << app::reset << endl;
}


void TPlayer::getAlbumTracks(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int64_t id = params["_"].asInteger64(0);

	if (debug) {
		if (debug) aout << app::green << "TPlayer::getAlbumTracks() Session = [" << session["SID"].asString() << "]" << app::reset << endl << endl;
		session.debugOutput("Session variable", "TPlayer::getAlbumTracks() ");
		if (debug) aout << endl;
		params.debugOutput("Parameter", "TPlayer::getAlbumTracks() ");
		if (debug) aout << endl;
	}

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;

	std::string filter;
	if (session.hasKey("HTML_QUERY_TRACKS")) {
		filter = session["HTML_QUERY_TRACKS"].asString();
	}

	if (debug) aout << app::green << "TPlayer::getAlbumTracks() Album = " << filter << app::reset << endl << endl;

	// Return requested playlist items
	if (jsonAlbumTracks.empty())
		jsonAlbumTracks = JSON_EMPTY_TABLE;
	if (!jsonAlbumTracks.empty()) {
		data = jsonAlbumTracks.c_str();
		size = jsonAlbumTracks.size();
	}
}


void TPlayer::getPlaying(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int64_t id = params["_"].asInteger64(0);
	size_t count = params["limit"].asInteger64(0);
	size_t index = params["offset"].asInteger64(0);

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;

	std::string title, playlist, filter;
	music::EFilterType type;
	music::CSongData song;
	player.getCurrentSong(song, playlist);
	if (song.valid) {
		title = song.titleHash;
		filter = song.albumHash;
		type = music::FT_ALBUM;
		if (debug) aout << "TPlayer::getPlaying() Song playing : " << title << "/" << filter  << endl;
	} else {
		if (debug) aout << "TPlayer::getPlaying() No song playing." << endl;
	}

	// Get configuration values
	music::CConfigValues values;
	sound.getConfiguredValues(values);
	bool clear = false;

	{ // Return requested playlist items
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		music::PPlaylist pls = playlists[playlist];
		if (util::assigned(pls)) {
			if (pls->empty() || !pls->isPermanent())
				jsonPlaying.clear();
			if (!filter.empty() && pls->isPermanent()) {
				jsonPlaying = pls->asJSON(count, index, filter, type, title, values.displayOrchestra).text();
			}
		} else {
			clear = true;
			if (!playlist.empty())
				logger(util::csnprintf("[Callback] Invalid playlist $ returned from ALSA player.", playlist));
		}
	}
	if (clear && !jsonPlaying.empty()) {
		if (isClosed()) {
			jsonPlaying.clear();
		}
	}
	if (jsonPlaying.empty())
		jsonPlaying = JSON_EMPTY_TABLE;
	if (!jsonPlaying.empty()) {
		data = jsonPlaying.c_str();
		size = jsonPlaying.size();
	}
}

void TPlayer::getStations(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int64_t id = params["_"].asInteger64(0);
	size_t count = params["limit"].asInteger64(0);
	size_t index = params["offset"].asInteger64(0);
	std::string filter = params["search"].asString();

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;

	// Get current stream hash
	std::string hash;
	getStreamHash(hash);

	{ // Return stations
		app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_READ);
		jsonStations = stations.asJSON(index, count, filter, hash);
	}

	if (jsonStations.empty())
		jsonStations = JSON_EMPTY_TABLE;
	if (!jsonStations.empty()) {
		data = jsonStations.c_str();
		size = jsonStations.size();
	}
}

void TPlayer::getPlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	jsonPlaylist.clear();
	int64_t id = params["_"].asInteger64(0);
	size_t count = params["limit"].asInteger64(0);
	size_t index = params["offset"].asInteger64(0);
	std::string filter = params["search"].asString();
	std::string playlist = session["HTML_CURRENT_PLAYLIST"].asString();
	music::EFilterType type = music::FT_STRING;

	if (debug) aout << "TPlayer::getPlaylist() Current playlist is \"" << playlist << "\" for session " << session[SESSION_ID].asString() << std::endl;

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;

	// Clear filter to show all entries
	if (!filter.empty()) {
		char c = filter[0];
		if (util::isMemberOf(c, '*','?',' '))  {
			type = music::FT_STRING;
			filter.clear();
		}
	}

	{ // Return requested playlist items
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		music::PPlaylist pls = playlists[playlist];
		if (util::assigned(pls)) {
			if (debug) aout << "TPlayer::getPlaylist() Selected playlist is \"" << pls->getName() << "\"" << std::endl;
			jsonPlaylist = pls->asJSON(count, index, filter, type).text();
		} else {
			if (playlist.empty())
				logger("[Callback] Empty playlist on web playlist request.");
			else
				logger(util::csnprintf("[Callback] Invalid playlist $ on web request.", playlist));
		}
	}
	if (jsonPlaylist.empty())
		jsonPlaylist = JSON_EMPTY_TABLE;
	if (!jsonPlaylist.empty()) {
		data = jsonPlaylist.c_str();
		size = jsonPlaylist.size();
	}
}


void TPlayer::getPlaylistExport(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	m3uPlaylistExport.clear();

	// Get playlist name from URI parameters
	std::string album = params["album"].asString();
	std::string playlist = params["playlist"].asString();

	// Get client IP address and check for local access
	bool local = true;
	std::string location = "unknown";
	std::string ip = params[URI_CLIENT_ADDRESS].asString();
	if (!ip.empty()) {
		if (!inet::isIPv6Address(ip)) {
			local = inet::isPrivateIPv4AddressRange(ip);
		}
		location = local ? "local" : "external";
	}

	// Check parameter
	if (!playlist.empty()) {
		std::cout << "TPlayer::getExport() Export playlist \"" << playlist << "\" as M3UEXT formatted file for " << location << " client [" << ip << "]" << std::endl;
	} else {
		if (!album.empty()) {
			std::cout << "TPlayer::getExport() Export album \"" << album << "\" as M3UEXT formatted playlist for " << location << " client [" << ip << "]" << std::endl;
		} else {
			std::cout << "TPlayer::getExport() No album or playlist name given to export." << std::endl;
			return;
		}
	}

	// Find current webroot, IPV4 only for now...
	std::string webroot = "file://";
	if (application.hasWebServer()) {
		bool ok = false;
		int port = 80;
		std::string address;
		if (local) {
			// Use local IPv4 address for playlist HTML root
			util::TStringList addresses;
			if (sysutil::getLocalIpAddresses(addresses, sysutil::EAT_IPV4)) {
				address = addresses[0];
				port = application.getWebServer().getPort();
				ok = true;
			}
		} else {
			// Read external address from application store
			util::TVariant value;
			application.readLocalStore(STORE_HOST_NAME, value);
			const std::string& hostname = value.asString();
			if (!hostname.empty()) {
				util::TStringList list(hostname, ':');
				if (!list.empty()) {
					address = list[0];
					if (list.size() > 1)
						port = util::strToInt(list[1]);
					if (port > 0 && port < 65536)
						ok = true;
				}
			}
		}
		if (ok) {
			std::string proto = application.getWebServer().isSecure() ? "https" : "http";
			int wellknown = application.getWebServer().isSecure() ? 443 : 80;
			if (inet::isIPv6Address(address))
				address = "[" + address + "]";
			if (port != wellknown)
				webroot = util::csnprintf("%://%:%/", proto, address, port);
			else
				webroot = util::csnprintf("%://%/", proto, address);
		}
	}

	{ // Get requested playlist/album
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		music::PPlaylist pls = playlists[playlist];
		if (!util::assigned(pls)) {
			logger(util::csnprintf("[Export] Invalid playlist $ on web request from % client [%]", playlist, location, ip));
			return;
		}

		// Return requested playlist/album as M3UEXT file
		logger(util::csnprintf("[Export] Export playlist $ on webroot $ for % client [%]", playlist, webroot, location, ip));
		m3uPlaylistExport = pls->asM3U(webroot).text('\n');
	}

	// Return empty M3U playlist as default
	if (m3uPlaylistExport.empty()) {
		m3uPlaylistExport = "#EXTM3U\n";
	}

	// Set result data
	if (!m3uPlaylistExport.empty()) {
		data = m3uPlaylistExport.c_str();
		size = m3uPlaylistExport.size();
	}
}


void TPlayer::getStationsExport(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	m3uStationsExport.clear();

	{ // Return stations as M3UEXT file
		app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_READ);
		m3uStationsExport = stations.asM3U().text('\n');
	}

	// Return empty M3U playlist as default
	if (m3uStationsExport.empty()) {
		m3uStationsExport = "#EXTM3U\n";
	}

	// Set result data
	if (!m3uStationsExport.empty()) {
		data = m3uStationsExport.c_str();
		size = m3uStationsExport.size();
	}
}


void TPlayer::getRecent(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int64_t id = params["_"].asInteger64(0);
	size_t count = params["limit"].asInteger64(0);
	size_t index = params["offset"].asInteger64(0);
	std::string filter = params["search"].asString();
	music::EFilterType type = music::FT_STRING;

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;

	// Clear filter to show all entries
	if (!filter.empty()) {
		char c = filter[0];
		if (util::isMemberOf(c, '*','?',' '))  {
			type = music::FT_STRING;
			filter.clear();
		}
	}

	{ // Return requested playlist items
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		jsonRecent = playlists.recent()->asJSON(count, index, filter, type).text();
	}
	if (jsonRecent.empty())
		jsonRecent = JSON_EMPTY_TABLE;
	if (!jsonRecent.empty()) {
		data = jsonRecent.c_str();
		size = jsonRecent.size();
	}
}



void TPlayer::getCurrentPlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Compare stored session hash value with current file hash
	bool transition = false;
	std::string current = session["HTML_CURRENT_PLAYLIST"].asString();
	std::string last = session["HTML_LAST_PLAYLIST"].asString();
	if (!current.empty() && !last.empty()) {
		transition = (last != current);
	}

	// Return state as JSON
	std::string agent = session["SESSION_USER_AGENT"].asString();
	util::TVariantValues response;
	response.add("Agent", agent);
	response.add("Transition", transition);
	response.add("Playlist", current);

	// Build JSON response
	jsonCurrentPlaylist = response.asJSON().text();
	if (!jsonCurrentPlaylist.empty()) {
		data = jsonCurrentPlaylist.c_str();
		size = jsonCurrentPlaylist.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getCurrentPlaylist() JSON = " << jsonCurrentPlaylist << app::reset << endl;
}


void TPlayer::getRadioTextUpdate(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Compare stored session hash value with current file hash
	bool transition = false;
	size_t current = session["HTML_STREAM_CURRENT_TEXT_COUNT"].asUnsigned(0);
	size_t last = session["HTML_STREAM_LAST_TEXT_COUNT"].asUnsigned(0);
	transition = (last != current);

	// Get current stream title
	std::string title;
	getRadioTitle(title);

	// Get current station index
	size_t idx;
	ssize_t index = -1;
	if (getStreamIndex(idx)) {
		index = idx + 1;
	}

	// Return state as JSON
	std::string agent = session["SESSION_USER_AGENT"].asString();
	util::TVariantValues response;
	response.add("Agent", agent);
	response.add("Transition", transition);
	response.add("Title", title);
	response.add("Index", index);
	response.add("Count", current);

	// Build JSON response
	jsonCurrentStream = response.asJSON().text();
	if (!jsonCurrentStream.empty()) {
		data = jsonCurrentStream.c_str();
		size = jsonCurrentStream.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getRadioTextUpdate() JSON = " << jsonCurrentStream << app::reset << endl;
}


void TPlayer::getCurrentRadioTitle(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Compare stored session hash value with current file hash
	// Get current stream title
	std::string title;
	getRadioTitle(title);

	// Get current station index
	size_t idx;
	ssize_t index = -1;
	if (getStreamIndex(idx)) {
		index = idx + 1;
	}

	// Return state as JSON
	std::string agent = session["SESSION_USER_AGENT"].asString();
	util::TVariantValues response;
	response.add("Agent", agent);
	response.add("Title", title);
	response.add("Index", index);

	// Build JSON response
	jsonCurrentTitle = response.asJSON().text();
	if (!jsonCurrentTitle.empty()) {
		data = jsonCurrentTitle.c_str();
		size = jsonCurrentTitle.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getRadioTitle() JSON = " << jsonCurrentTitle << app::reset << endl;
}


void TPlayer::getRadioPlayUpdate(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Compare stored session hash value with current file hash
	bool transition = false;
	size_t current = session["HTML_STREAM_CURRENT_STATION_COUNT"].asUnsigned(0);
	size_t last = session["HTML_STREAM_LAST_STATION_COUNT"].asUnsigned(0);
	transition = (last != current);

	// Return state as JSON
	std::string agent = session["SESSION_USER_AGENT"].asString();
	util::TVariantValues response;
	response.add("Agent", agent);
	response.add("Transition", transition);
	response.add("Streaming", radio.streaming);
	response.add("Count", current);

	// Build JSON response
	jsonCurrentStation = response.asJSON().text();
	if (!jsonCurrentStation.empty()) {
		data = jsonCurrentStation.c_str();
		size = jsonCurrentStation.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getStreamProperties() JSON = " << jsonCurrentStation << app::reset << endl;
}


void TPlayer::getRadioPlayStream(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Return state as JSON
	util::TVariantValues response;
	response.add("Streaming", radio.streaming);

	// Build JSON response
	jsonCurrentStreaming = response.asJSON().text();
	if (!jsonCurrentStreaming.empty()) {
		data = jsonCurrentStreaming.c_str();
		size = jsonCurrentStreaming.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getRadioPlayStream() JSON = " << jsonCurrentStreaming << app::reset << endl;
}


void TPlayer::getSelectedPlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Get parameters from query to return them back
	std::string hash = params["hash"].asString();
	std::string album = params["album"].asString();

	std::string name = params["name"].asString();
	std::string title = params["title"].asString();
	std::string artist = params["artist"].asString();

	std::string displayname = params["displayname"].asString();
	std::string displaytitle = params["displaytitle"].asString();
	std::string displayartist = params["displayartist"].asString();

	std::string action = params["action"].asString("none");
	std::string selected = session["HTML_CURRENT_PLAYLIST"].asString();
	std::string current;
	{
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		current = playlists.getPlaying();
		if (selected.empty())
			selected = playlists.getSelected();
	}
	if (selected.empty())
		selected = "*";
	if (current.empty())
		current = "*";
	if (selected == "state")
		selected = "*";
	if (current == "state")
		current = "*";

	// Return state as JSON
	util::TVariantValues response;
	response.add("Hash", hash);
	response.add("Album", album);

	response.add("Name", name);
	response.add("Title", title);
	response.add("Artist", artist);

	response.add("Displayname", displayname);
	response.add("Displaytitle", displaytitle);
	response.add("Displayartist", displayartist);

	response.add("Action", action);
	response.add("Selected", selected);
	response.add("Current", current);

	// Build JSON response
	jsonSelectedPlaylist = response.asJSON().text();
	if (!jsonSelectedPlaylist.empty()) {
		data = jsonSelectedPlaylist.c_str();
		size = jsonSelectedPlaylist.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getSelectedPlaylist() JSON = " << jsonSelectedPlaylist << app::reset << endl;
}


void TPlayer::getPlayerState(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	bool playing = false;
	bool streaming = false;
	std::string artist, album, file;

	// Get current song and playlist
	music::CSongData song;
	std::string playlist;
	getCurrentSong(song, playlist);
	if (song.valid) {
		playing   = true;
		streaming = song.streamable;
		album     = song.albumHash;
		artist    = song.artistHash;
		file      = song.fileHash;
	}

	// Check also for radio text change on streamed playback
	size_t currentChange = session["HTML_CURRENT_CHANGE"].asInteger(0);
	size_t lastChange = session["HTML_LAST_CHANGE"].asInteger(0);
	bool transition = (lastChange != currentChange);

	// Return state as JSON
	std::string agent = session["SESSION_USER_AGENT"].asString();
	util::TVariantValues response;
	response.add("Agent", agent);
	response.add("Playing", playing);
	response.add("Streaming", streaming);
	response.add("Transition", transition);
	response.add("Artist", artist);
	response.add("Album", album);
	response.add("File", file);
	response.add("Last", lastChange);
	response.add("Current", currentChange);

	// Build JSON response
	jsonCurrent = response.asJSON().text();
	if (!jsonCurrent.empty()) {
		data = jsonCurrent.c_str();
		size = jsonCurrent.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getPlayerState() JSON = " << jsonCurrent << app::reset << endl;
}


void TPlayer::getScannerState(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int mode = 0;
	bool scanning = isLibraryUpdating();
	if (scanning) {
		ECommandAction action = getLibraryAction();
		if (action == ECA_LIBRARY_RESCAN_LIBRARY)
			mode = 1;
		if (action == ECA_LIBRARY_REBUILD_LIBRARY)
			mode = 2;
	}

	// Check also for radio text change on streamed playback
	size_t currentUpdate = session["HTML_CURRENT_UPDATE"].asInteger(0);
	size_t lastUpdate = session["HTML_LAST_UPDATE"].asInteger(0);
	bool transition = (lastUpdate != currentUpdate);

	// Return state as JSON
	std::string agent = session["SESSION_USER_AGENT"].asString();
	util::TVariantValues response;
	response.add("Agent", agent);
	response.add("Scanning", scanning);
	response.add("Transition", transition);
	response.add("Mode", mode);
	response.add("Last", lastUpdate);
	response.add("Current", currentUpdate);

	// Build JSON response
	jsonScanner = response.asJSON().text();
	if (!jsonScanner.empty()) {
		data = jsonScanner.c_str();
		size = jsonScanner.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getScannerState() JSON = " << jsonScanner << app::reset << endl;
}


void TPlayer::getRepeatModes(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Get current repeat modes
	TPlayerMode mode;
	getCurrentMode(mode);

	// Return native state as JSON
	util::TVariantValues response;
	response.add("Random", mode.random);
	response.add("Repeat", mode.repeat);
	response.add("Single", mode.single);
	response.add("Halt",   mode.halt);
	response.add("Disk",   mode.disk);
	response.add("Direct", mode.direct);

	// Build JSON response
	jsonModes = response.asJSON().text();
	if (!jsonModes.empty()) {
		data = jsonModes.c_str();
		size = jsonModes.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getRepeatModes() JSON = " << jsonModes << app::reset << endl;
}


void TPlayer::getPlayerMode(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Get current repeat modes
	TPlayerMode mode;
	getCurrentMode(mode);

	// Compare stored session hash value with current file hash
	bool transition = false;
	std::string current = session["HTML_CURRENT_MODE"].asString();
	std::string last = session["HTML_LAST_MODE"].asString();
	if (!current.empty() && !last.empty()) {
		transition = (last != current);
	}

	// Return transition state as JSON
	std::string agent = session["SESSION_USER_AGENT"].asString();
	util::TVariantValues response;
	response.add("Agent",  agent);
	response.add("Random", mode.random);
	response.add("Repeat", mode.repeat);
	response.add("Single", mode.single);
	response.add("Halt",   mode.halt);
	response.add("Disk",   mode.disk);
	response.add("Direct", mode.direct);
	response.add("Transition", transition);

	// Build JSON response
	jsonMode = response.asJSON().text();
	if (!jsonMode.empty()) {
		data = jsonMode.c_str();
		size = jsonMode.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getPlayerMode() JSON = " << jsonMode << app::reset << endl;
}


void TPlayer::getPlayerConfig(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Get configuration values
	music::CConfigValues values;
	sound.getConfiguredValues(values);

	// Return configuration parameters as JSON object
	util::TVariantValues response;
	response.add("Device", values.device);
	response.add("Musicpath1", values.musicpath1);
	response.add("Musicpath2", values.musicpath2);
	response.add("Musicpath3", values.musicpath3);
	response.add("EnableFolder1", values.enableMusicPath1);
	response.add("EnableFolder2", values.enableMusicPath2);
	response.add("EnableFolder3", values.enableMusicPath3);
	response.add("Filepattern", values.pattern.csv());
	response.add("Datapath", values.datapath);
	response.add("Datafile", values.datafile);
	response.add("AllowGroupNameSwap", values.allowGroupNameSwap);
	response.add("AllowArtistNameRestore", values.allowArtistNameRestore);
	response.add("AllowFullNameSwap", values.allowFullNameSwap);
	response.add("AllowTheBandPrefixSwap", values.allowTheBandPrefixSwap);
	response.add("AllowDeepNameInspection", values.allowDeepNameInspection);
	response.add("AllowVariousArtistsRename", values.allowVariousArtistsRename);
	response.add("AllowMovePreamble", values.allowMovePreamble);
	response.add("SortCaseSensitive", values.sortCaseSensitive);
	response.add("SortAlbumsByYear", values.sortAlbumsByYear);
	response.add("DisplayOrchestra", values.displayOrchestra);
	response.add("DisplayRemainingTime", values.displayRemain);
	response.add("WatchLibraryEnabled", values.watchLibraryEnabled);
	response.add("DisplayCountLimit", values.displayLimit);
	response.add("EnableDithering", values.enableDithering);
	response.add("PeriodTime", values.periodTime);
	response.add("VariousArtistsCategories", values.categories.csv());
	if (values.pageLimit > 1000) {
		response.add("TablePageLimit", "All");
	} else {
		response.add("TablePageLimit", values.pageLimit);
	}

	// Build JSON response
	jsonConfig = response.asJSON().text();
	if (!jsonConfig.empty()) {
		data = jsonConfig.c_str();
		size = jsonConfig.size();
	}

	if (debug)
		aout << app::yellow << "TPlayer::getPlayerConfig() JSON = " << jsonConfig << app::reset << endl;
}


void TPlayer::getRemoteConfig(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Get configuration values
	music::CRemoteValues values;
	sound.getRemoteValues(values);

	// Return configuration parameters as JSON object
	util::TVariantValues response;
	response.add("Device", values.device);
	response.add("RemoteDeviceEnabled", values.enabled);
	response.add("CanonicalSampleRate", values.canonical);
	response.add("TerminalDeviceEnabled", values.control);

	// Build JSON response
	jsonRemote = response.asJSON().text();
	if (!jsonRemote.empty()) {
		data = jsonRemote.c_str();
		size = jsonRemote.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getRemoteConfig() JSON = " << jsonRemote << app::reset << endl;
}


std::string TPlayer::getProgressTimestamp(util::TTimePart played, util::TTimePart duration, bool advanced) {
	if (advanced && duration >= played)
		return music::TSong::timeToStr(played) + " / " + music::TSong::timeToStr(duration) + " / -" + music::TSong::timeToStr(duration - played);
	return music::TSong::timeToStr(played) + " / " + music::TSong::timeToStr(duration);
}


void TPlayer::getProgressData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Get current song
	music::CSongData current;
	music::EPlayerState state;
	getCurrentState(current, state);

	// Get configured display settings
	music::CConfigValues values;
	sound.getConfiguredValues(values);

	// Return progress data as JSON object
	util::TVariantValues response;
	if (current.valid && !current.streamable) {
		response.add("Timestamp", getProgressTimestamp(current.played, current.duration, values.displayRemain));
		response.add("Progress", current.progress);
	} else {
		response.add("Timestamp", "0:00");
		response.add("Progress", 0);
	}
	response.add("State", state);

	// Build JSON response
	jsonProgress = response.asJSON().text();
	if (!jsonProgress.empty()) {
		data = jsonProgress.c_str();
		size = jsonProgress.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getProgressData() JSON = " << jsonProgress << app::reset << endl;
}


void TPlayer::getErroneous(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int64_t id = params["_"].asInteger64(0);
	size_t count = params["limit"].asInteger64(0);
	size_t index = params["offset"].asInteger64(0);

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;

	{
		app::TLockGuard<app::TMutex> lock(updateMtx, false);
		if (lock.tryLock()) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
			if (library.erroneous() > 0)
				jsonErroneous = library.getErrorFilesAsJSON(index, count);
			else
				jsonErroneous.clear();
		}
	}
	if (jsonErroneous.empty())
		jsonErroneous = JSON_EMPTY_TABLE;
	if (!jsonErroneous.empty()) {
		data = jsonErroneous.c_str();
		size = jsonErroneous.size();
	}
}

void TPlayer::setReorderedData(TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error) {
	bool ok = false;
	bool _debug = debug;
	if (util::assigned(data) && size > 0) {
		std::string json((char*)data, size);
		if (!json.empty()) {
			logger("[Reordered] Received JSON row data \"" + util::ellipsis(json, 120) + "\"");
			data::TTable table = json;
			if (_debug) {
				debugOutputSongTable(table, 30);
				// table.debugOutputColumns("REORDERED: ");
				// table.debugOutputData("REORDERED: ");
			}
			if (!table.empty()) {
				std::string name = session["HTML_CURRENT_PLAYLIST"].asString();

				// Reorder songs for current playlist
				app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
				music::PPlaylist playlist = playlists.find(name);
				if (util::assigned(playlist)) {
					int r = playlist->reorder(table);
					if (r > 0) {
						logger("[Reordered] " + std::to_string((size_s)r) + " rows reordered for playlist \"" + name + "\"");
					} else {
						if (r == 0) {
							logger("[Reordered] No rows reordered for playlist \"" + name + "\"");
						} else {
							logger("[Reordered] Reorder playlist \"" + name + "\" failed (" + std::to_string((size_s)r) + ")");
						}
					}
				} else {
					logger("[Reordered] Invalid playlist \"" + name + "\"");
				}

				// Data itself was OK!
				ok = true;
			}
		}
	}
	if (!ok) {
		error = WSC_BadRequest;
	}
}

void TPlayer::debugOutputSongTable(const data::TTable& table, size_t count) {
	if (!table.empty()) {
		size_t max = table.size();
		if (count > 0 && count < table.size()) {
			max = count;
		}
		for (size_t idx=0; idx<max; idx++) {
			std::string track = table[idx]["Track"].asString();
			std::string title = table[idx]["Displaytitle"].asString();
			std::cout << app::blue << util::succ(idx) << ". Row : Track \"" << track << "\" Title \"" << title << "\"" << app::reset << std::endl;
		}
		std::cout << std::endl;
	}
}


void TPlayer::setRearrangedData(TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error) {
	bool ok = false;
	bool _debug = debug;
	if (util::assigned(data) && size > 0) {
		std::string json((char*)data, size);
		if (!json.empty()) {
			logger("[Rearranged] Received JSON row data \"" + util::ellipsis(json, 120) + "\"");
			data::TTable table = json;
			if (_debug) {
				debugOutputStationTable(table, 30);
				// table.debugOutputColumns("REARRANGED: ");
				// table.debugOutputData("REARRANGED: ");
			}
			if (!table.empty()) {

				// Reorder stations
				app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_WRITE);
				int r = stations.reorder(table);
				if (r > 0) {
					logger("[Rearranged] " + std::to_string((size_s)r) + " stations reordered.");
				} else {
					if (r == 0) {
						logger("[Rearranged] No stations reordered.");
					} else {
						logger("[Rearranged] Reorder stations failed (" + std::to_string((size_s)r) + ")");
					}
				}

				// Data itself was OK!
				ok = true;
			}
		}
	}
	if (!ok) {
		error = WSC_BadRequest;
	}
}

void TPlayer::debugOutputStationTable(const data::TTable& table, size_t count) {
	if (!table.empty()) {
		size_t max = table.size();
		if (count > 0 && count < table.size()) {
			max = count;
		}
		for (size_t idx=0; idx<max; idx++) {
			size_t index = table[idx]["Index"].asInteger(0);
			std::string name = table[idx]["Name"].asString();
			std::cout << app::blue << util::succ(idx) << ". Row : Index \"" << index << "\" Name \"" << name << "\"" << app::reset << std::endl;
		}
		std::cout << std::endl;
	}
}

void TPlayer::setControlData(TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error) {
	bool ok = false;
	bool _debug = debug;
	//_debug = true;
	if (util::assigned(data) && size > 0) {
		std::string json((char*)data, size);
		if (!json.empty()) {
			logger("[Control] Received JSON control data \"" + util::ellipsis(json, 40) + "\"");
			data::TTable table = json;
			if (!table.empty()) {
				if (_debug) {
					table.debugOutputColumns("CONTROL: ");
					table.debugOutputData("CONTROL: ");
				}
				std::string command = util::toupper(table[0]["command"].asString());
				if (!command.empty()) {
					logger("[Control] Execute control command \"" + command + "\"");
					addRemoteCommand(command);
					ok = true;
				}
			}
		}
	}
	if (!ok) {
		error = WSC_BadRequest;
	}
}

void TPlayer::setControlAPI(TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error) {
	// Handle REST API command URLs like
	// --> http://localhost:8099/rest/control/pause
	//     http://localhost:8099/rest/control/stop
	//     http://localhost:8099/rest/control/play
	//     ...
	const std::string& url = params[URI_REQUEST_URL].asString();
	if (!url.empty()) {
		std::string command = util::toupper(util::fileExtName(url));
		logger("[Rest] Received command \"" + command + "\" from URL \"" + url + "\"");
		if (!command.empty()) {
			logger("[Rest] Execute control command \"" + command + "\"");
			addRemoteCommand(command);
			return;
		}
	} else {
		error = WSC_NotFound;
		logger("[Rest] Processing command failed due to missing URL.");
		return;
	}
	error = WSC_BadRequest;
}


std::string TPlayer::listAsJSON(const util::TStringList& list, const std::string& title, const size_t total, size_t index) {

	// Support for bootstrap-table server side pagination
	// Example:
	//   {
	//     "total": 2,
	//     "rows": [
	//       {
	//         "Index": 0,
	//         "Row": "...."
	//       },
	//       {
	//         "Index": 1,
	//         "Row": "...."
	//       }
	//     ]
	//   }

	// Show natural index count
	++index;

	// Is limit valid?
	if (list.empty())
		return JSON_EMPTY_TABLE;

	// Begin new JSON object
	util::TStringList json;
	json.add("{");

	// Begin new JSON array
	json.add("\"total\": " + std::to_string((size_u)total) + ",");
	json.add("\"rows\": [");

	// Add JSON object with trailing separator
	util::TStringList::const_iterator it = list.begin();
	util::TStringList::const_iterator last = list.last();
	for(; it != last; ++it, ++index) {
		json.add("{");
		json.add("  \"Index\": " + std::to_string((size_u)index) + ",");
		json.add("  \"" + title + "\": \"" + (*it) + "\"");
		json.add("},");
	}

	// Add last object without separator
	if (it != list.end()) {
		json.add("{");
		json.add("  \"Index\": " + std::to_string((size_u)index) + ",");
		json.add("  \"" + title + "\": \"" + (*it) + "\"");
		json.add("}");
	}

	// Close JSON array and object
	json.add("]}");

	return json.text();
}


void TPlayer::setMenuItem(html::PMenuItem item, const music::TArtistMap& artists, const size_t count, const std::string caption) {
	if (util::assigned(item)) {
		if (artists.empty()) {
			item->setCaption(caption);
			item->setActive(false);
		} else {
			std::string text = (count != 1) ? "albums" : "album";
			item->setCaption(util::csnprintf(caption + " <i>(% %)</i>", count, text));
			item->setActive(true);
		}
	}
}

void TPlayer::updateLibraryMenuItems() {
	if (util::assigned(cdArtistItem)) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		updateLibraryMenuItemsWithNolock();
	}
}

void TPlayer::updateLibraryMenuItemsWithNolock() {
	if (util::assigned(cdArtistItem)) {
		app::TLockGuard<app::TMutex> lock(componentMtx);
		updateLibraryMenuItemEntriesWithNolock();
		mnMain.update();
	}
}

void TPlayer::updateLibraryMenuItemEntriesWithNolock() {
	if (util::assigned(cdArtistItem)) {
		setMenuItem(cdArtistItem,    library.getCDArtists(),   library.getCDCount(),   "<b>CD</b> · Compact Disk");
		setMenuItem(hdcdArtistsItem, library.getHDCDArtists(), library.getHDCDCount(), "<b>HDCD</b> · High Definition CD");
		setMenuItem(dvdArtistItem,   library.getDVDArtists(),  library.getDVDCount(),  "<b>DVD</b> · Digital Versatile Disk");
		setMenuItem(bdArtistItem,    library.getBDArtists(),   library.getBDCount(),   "<b>BD</b> · Blu-ray Disk");
		setMenuItem(hrArtistItem,    library.getHRArtists(),   library.getHRCount(),   "<b>HR</b> · High Resolution Audio");
		setMenuItem(dsdArtistItem,   library.getDSDArtists(),  library.getDSDCount(),  "<b>DSD</b> · Direct Stream Digital");
	}
}


void TPlayer::updateSettingsMenuItems() {
	if (util::assigned(settingsMenuItem)) {
		// Rebuild sub menu entriess
		app::TLockGuard<app::TMutex> lock(componentMtx);
		updateSettingsMenuItemEntriesWithNolock();
		mnMain.update();
	}
}

void TPlayer::updateSettingsMenuItemEntriesWithNolock() {
	if (util::assigned(settingsMenuItem)) {
		// Rebuild sub menu entriess
		settingsMenuItem->clearSubMenu();
		mnMain.setAnchor(settingsMenuItem);
		mnMain.addSubItem("ms-system-settings", "Player", "system/settings.html?prepare=yes", 3, "glyphicon-wrench");
		mnMain.addSubItem("ms-network-settings", "Network", "system/network.html", 3, "glyphicon-flash");
		mnMain.addSubItem("ms-system-settings", "System", "system/sysconf.html", 3, "glyphicon-cog");
		if (advancedSettingsAllowed) {
			mnMain.addSubSeparator();
			mnMain.addSubItem("ms-remote-settings", "Advanced", "system/remote.html?prepare=yes", 3, "glyphicon-certificate");
		}
	}
}


void TPlayer::updatePlaylistMenuItems(const std::string& playlist) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	updatePlaylistMenuItemsWithNolock(playlist);
}

void TPlayer::updatePlaylistMenuItemsWithNolock(const std::string& playlist) {
	if (util::assigned(playlistSelectItem) && !playlist.empty()) {
		if (!playlists.isPlaying(playlist)) {
			playlists.play(playlist);
			updatePlaylistMenuItemsWithNolock();
		}
	}
}

void TPlayer::updatePlaylistMenuItems() {
	if (util::assigned(playlistSelectItem)) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		updatePlaylistMenuItemsWithNolock();
	}
}

void TPlayer::updatePlaylistMenuItemsWithNolock() {
	if (util::assigned(playlistSelectItem)) {
		app::TLockGuard<app::TMutex> lock(componentMtx);
		app::TStringVector names;
		playlists.getNames(names);
		playlistSelectItem->clearSubMenu();
		mnMain.setAnchor(playlistSelectItem);
		mnMain.addSubItem("mn-state", "Recent songs", "playlist/recent.html?prepare=yes&title=recent&name=state", 0, "glyphicon-time");
		mnMain.addSubSeparator();
		if (!names.empty()) {
			if (names.size() > 6) {
				// Select Playlist by dialog for many (> 6) playlists
				mnMain.addSubItem("ms-playlist-select", "Select playlist", "onSelectPlaylistClick();", 0, "glyphicon-hand-right");
			} else {
				for (size_t i=0; i<names.size(); ++i) {
					std::string name = names[i];
					std::string id = util::csnprintf("ms-playlist-select-%", i);
					std::string action = "playlist/playlist.html?prepare=yes&title=playlist&name=" + util::TURL::encode(name);
					std::string glyphicon = "glyphicon-none";
					if (playlists.isPlaying(name)) {
						if (playlists.isSelected(name)) {
							glyphicon = "glyphicon-hand-right";
						} else {
							glyphicon = "glyphicon-play";
						}
					} else {
						if (playlists.isSelected(name)) {
							glyphicon = "glyphicon-arrow-right";
						}
					}
					mnMain.addSubItem(id, name, action, 0, glyphicon);
				}

			}
			mnMain.addSubSeparator();
		}
		mnMain.addSubItem("ms-playlist-new", "New playlist", "onNewPlaylistClick();", 0, "glyphicon-plus");
		if (!names.empty()) {
			mnMain.addSubItem("ms-playlist-rename", "Rename playlist", "onRenamePlaylistClick();", 0, "glyphicon-pencil");
			mnMain.addSubItem("ms-playlist-delete", "Delete playlist", "onDeletePlaylistClick();", 0, "glyphicon-trash");
		}
		mnMain.update();
	}
}

#undef USE_URI_PLAYLIST_PARAMETER_CALL

void TPlayer::updateLibraryMediaItems() {
	if (util::assigned(libraryMediaItem)) {
		util::TStringList folders;
		sound.getMusicFolders(folders);

		// Rebuild sub menu entriess
		app::TLockGuard<app::TMutex> lock(componentMtx);
		libraryMediaItem->clearSubMenu();
		mnMain.setAnchor(libraryMediaItem);
		mnMain.addSubItem("ms-inter-radio", "Internet Radio Stations", "playlist/streaming.html", 0, "glyphicon-globe");
		mnMain.addSubSeparator();
		cdArtistItem    = mnMain.addSubItem("ms-media-cd",  "<b>CD</b> · Compact Disk",            "library/media.html?prepare=yes&title=media&type=cd",   0, "glyphicon-chevron-right");
		hdcdArtistsItem = mnMain.addSubItem("ms-media-hd",  "<b>HDCD</b> · High Definition CD",    "library/media.html?prepare=yes&title=media&type=hdcd", 0, "glyphicon-chevron-right");
		dvdArtistItem   = mnMain.addSubItem("ms-media-dvd", "<b>DVD</b> · Digital Versatile Disk", "library/media.html?prepare=yes&title=media&type=dvd",  0, "glyphicon-chevron-right");
		mnMain.addSubSeparator();
		bdArtistItem    = mnMain.addSubItem("ms-media-bd",  "<b>BD</b> · Blu-ray Disk",            "library/media.html?prepare=yes&title=media&type=bd",   0, "glyphicon-chevron-right");
		hrArtistItem    = mnMain.addSubItem("ms-media-hr",  "<b>HR</b> · High Resolution Audio",   "library/media.html?prepare=yes&title=media&type=hr",   0, "glyphicon-chevron-right");
		dsdArtistItem   = mnMain.addSubItem("ms-media-dsd", "<b>DSD</b> · Direct Stream Digital",  "library/media.html?prepare=yes&title=media&type=dsd",  0, "glyphicon-chevron-right");

		// Update media item entries
		updateLibraryMenuItemEntriesWithNolock();

		// Browse active media folders
		if (!folders.empty()) {
			mnMain.addSubSeparator();
			for (size_t i=0; i<folders.size(); ++i) {
				std::string folder;
				switch (i) {
					case 0:
						folder = "first";
						break;
					case 1:
						folder = "second";
						break;
					case 2:
						folder = "third";
						break;
					default:
						folder = std::to_string((size_s)i) + "th";
						break;
				}
				std::string path = util::TURL::encode(folders[i]);
				std::string caption = folders.size() > 1 ? util::csnprintf("Browse % library folder", folder) : "Browse library folder";
				mnMain.addSubItem("ms-media-browse", caption, "system/explorer.html?prepare=yes&title=explorer&path=" + path, 0, "glyphicon-folder-open");
			}
		}
		mnMain.update();
	}
}

void TPlayer::updateLibraryStatus() {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
	updateLibraryStatusWithNolock();
}

void TPlayer::updateLibraryStatusWithNolock() {
	if (util::assigned(wtArtistCount)) {
		*wtArtistCount = library.artists();
		*wtAlbumCount = library.albums();
		*wtSongCount = library.songs();
		*wtErrorCount = library.erroneous();
		*wtTrackTime = util::timeToHuman(library.getDuration(), 3);
		*wtTrackSize = util::sizeToStr(library.getContentSize());
		wtTrackTime->invalidate();
	}
	setPlaylistHeader(wtPlaylistHeader->getValue(), playlists.getSelected(), playlists.getSelectedSize());
	setRecentHeader(playlists.recent()->size());
}


void TPlayer::prepareWebRequest(const std::string& uri, const util::TVariantValues& query, util::TVariantValues& session, bool& prepared) {
	if (query["prepare"].asString() == "yes") {
		bool debugger = debug;
		//debugger = true;

		if (debugger) {
			aout << app::yellow << "TPlayer::prepareWebRequest() " << uri << " [" << session["SID"].asString() << "]" << app::reset << endl << endl;
			query.debugOutput("Parameter", "TPlayer::prepareWebRequest() ");
			aout << endl;
		}

		// Check parameter and store as session vars...
		std::string request = query["filter"].asString();
		std::string title = util::tolower(query["title"].asString());
		std::string filter = util::tolower(request);
		std::string type = query["type"].asString();
		std::string limit = query["domain"].asString();
		std::string genre = query["genre"].asString();
		std::string enabled = util::tolower(query["enabled"].asString());
		bool found = false;

		// Update library settings page combo box entries
		std::string settings = sound.getHTMLRoot() + "system/settings.html";
		if (!found && 0 == util::strncasecmp(uri, settings, settings.size())) {
			updateHardwareDevices();
			updateLocalDrives();
			found = true;
		}

		// Update remote settings page combo box entries
		settings = sound.getHTMLRoot() + "system/remote.html";
		if (!found && 0 == util::strncasecmp(uri, settings, settings.size())) {
			updateHardwareDevices();
			updateSerialPorts();
			found = true;
		}

		// Display complete artists library?
		if (!found && title == "artists") {
			if (filter.empty())
				filter = session["HTML_QUERY_ARTISTS_FILTER"].asString();
			session.add("HTML_QUERY_ARTISTS_FILTER", filter);

			// Get configured values
			music::CConfigValues values;
			sound.getConfiguredValues(values);

			// Update artist view for given filter and media type
			util::TStringList html;
			music::TLibraryResults r = getArtistsHTML(html, filter, music::EMT_ALL, values, music::TLibrary::ELV_ARTIST);
			std::string text1 = (r.artists != 1) ? " artists)" : " artist)";
			std::string text2 = (r.artists != 1) ? " categories)" : " category)";
			if (!filter.empty()) {
				char c = toupper(filter[0]);
				std::string value(&c, 1);
				if (debugger) aout << "TPlayer::prepareWebRequest() Header for artist <Library \"" + value + "\" (" + std::to_string((size_u)r.artists) + text1 << ">" << std::endl;
				switch (c) {
					case music::CHAR_NUMERICAL_ARTIST:
						*wtArtistLibraryHeader = "Numerical artists (" + std::to_string((size_u)r.artists) + text1;
						break;
					case music::CHAR_VARIOUS_ARTIST:
						*wtArtistLibraryHeader = "Compilation albums (" + std::to_string((size_u)r.artists) + text2;
						break;
					default:
						*wtArtistLibraryHeader = "Library &quot;" + value + "&quot; (" + std::to_string((size_u)r.artists) + text1;
						break;
				}
			} else {
				if (debugger) aout << "TPlayer::prepareWebRequest() Header <Library \"A\" (" + std::to_string((size_u)r.artists) + text1 << ">" << std::endl;
				*wtArtistLibraryHeader = "Library &quot;A&quot; (" + std::to_string((size_u)r.artists) + text1;
			}

			// Store new value in web token and force invalidation!
			wtArtistLibraryBody->setValue(html.html(), true);
			prepared = found = true;
		}

		// Display complete album library as "listview"?
		if (!found && title == "listview") {
			if (filter.empty())
				filter = session["HTML_QUERY_LISTVIEW_FILTER"].asString();
			session.add("HTML_QUERY_LISTVIEW_FILTER", filter);

			// Get configured values
			music::CConfigValues values;
			sound.getConfiguredValues(values);

			// Update artist view for given filter and media type
			util::TStringList html;
			music::TLibraryResults r = getArtistsHTML(html, filter, music::EMT_ALL, values, music::TLibrary::ELV_ALBUM);
			std::string albums = std::to_string((size_u)r.albums) + ((r.albums  != 1) ? " albums" : " album");
			std::string artists = std::to_string((size_u)r.artists) + ((r.artists != 1) ? " artists" : " artist");
			std::string categories = std::to_string((size_u)r.artists) + ((r.artists != 1) ? " categories" : " category");
			if (!filter.empty()) {
				char c = toupper(filter[0]);
				std::string value(&c, 1);
				if (debugger) aout << "TPlayer::prepareWebRequest() Header for artist <Library \"" << value << "\" (" << artists << " / " << albums << ")>" << std::endl;
				switch (c) {
					case music::CHAR_NUMERICAL_ARTIST:
						*wtAlbumListViewHeader = "Numerical artists (" + artists + " / " + albums + ")";
						break;
					case music::CHAR_VARIOUS_ARTIST:
						*wtAlbumListViewHeader = "Compilation albums (" + categories + " / " + albums + ")";
						break;
					default:
						*wtAlbumListViewHeader = "Library &quot;" + value + "&quot; (" + artists + " / " + albums + ")";
						break;
				}
			} else {
				if (debugger) aout << "TPlayer::prepareWebRequest() Header <Library \"A\" (" << artists << " / " << albums << ")>" << std::endl;
				*wtAlbumListViewHeader = "Library &quot;A&quot; (" + artists + " / " + albums + ")";
			}

			// Store new value in web token and force invalidation!
			wtAlbumListViewBody->setValue(html.html(), true);
			prepared = found = true;
		}

		// Display media library by format?
		if (!found && title == "media") {

			// Read last filter string from session
			if (filter.empty())
				filter = session["HTML_QUERY_MEDIA_FILTER"].asString();
			session.add("HTML_QUERY_MEDIA_FILTER", filter);

			// Read last media type from session
			if (type.empty())
				type = session["HTML_QUERY_MEDIA_TYPE"].asString();

			// Catch missing media type
			music::EMediaType media = getMediaType(type);
			if (media == music::EMT_UNKNOWN) {
				media = music::EMT_ALL;
				type = "all";
			}

			// Store media type for further use (even if empty)
			// --> Read to prepare next "format" output!
			session.add("HTML_QUERY_MEDIA_TYPE", type);
			session.add("HTML_QUERY_MEDIA_CODEC", type);
			if (debugger) aout << "TPlayer::prepareWebRequest() Type for media view is <" << type << ">" << std::endl;

			// Get configured values
			music::CConfigValues values;
			sound.getConfiguredValues(values);

			// Update artist view for given filter and media type
			util::TStringList html;
			music::TLibraryResults r = getArtistsHTML(html, filter, media, values, music::TLibrary::ELV_ARTIST);
			std::string text1 = (r.artists != 1) ? " artists)" : " artist)";
			std::string text2 = (r.artists != 1) ? " categories)" : " category)";
			std::string header = getMediaShortcut(media);
			if (!filter.empty()) {
				char c = toupper(filter[0]);
				std::string value(&c, 1);
				if (debugger) aout << "TPlayer::prepareWebRequest() Header for artist <Media library \"" + value + "\" (" + std::to_string((size_u)r.artists) + text1 << ">" << std::endl;
				switch (c) {
					case music::CHAR_NUMERICAL_ARTIST:
						*wtMediaLibraryHeader = header + " Numerical artists (" + std::to_string((size_u)r.artists) + text1;
						break;
					case music::CHAR_VARIOUS_ARTIST:
						*wtMediaLibraryHeader = header + " Compilation albums (" + std::to_string((size_u)r.artists) + text2;
						break;
					default:
						*wtMediaLibraryHeader = header + " Library &quot;" + value + "&quot; (" + std::to_string((size_u)r.artists) + text1;
						break;
				}
			} else {
				if (debugger) aout << "TPlayer::prepareWebRequest() Header <Media library \"A\" (" + std::to_string((size_u)r.artists) + text1 << ">" << std::endl;
				*wtMediaLibraryHeader = header + " Library &quot;A&quot; (" + std::to_string((size_u)r.artists) + text1;
			}

			// Set icon for content display
			std::string icon = getMediaName(media);
			wtMediaLibraryIcon->setValue("/rest/icons/" + icon + ".jpg", true);

			// Store new value in web token and force invalidation!
			wtMediaLibraryBody->setValue(html.html(), true);
			wtMediaLibraryHeader->invalidate();
			prepared = found = true;
		}

		// Display album library by format ?
		if (!found && title == "format") {

			// Read last filter string from session
			if (filter.empty())
				filter = session["HTML_QUERY_FORMAT_FILTER"].asString();
			session.add("HTML_QUERY_FORMAT_FILTER", filter);

			// Read last stored media type information from session
			// --> Stored by previous "media" request
			type = session["HTML_QUERY_MEDIA_CODEC"].asString();
			music::EMediaType media = getMediaType(type);
			if (debugger) aout << "TPlayer::prepareWebRequest() Type for format album view is <" << type << ">" << std::endl;

			// Catch missing media type
			if (media == music::EMT_UNKNOWN) {
				media = music::EMT_ALL;
			}

			// Get configured values
			music::CConfigValues values;
			sound.getConfiguredValues(values);

			// Get and set filtered album list as HTML
			music::EFilterDomain domain = music::FD_ALBUMARTIST;
			util::TStringList html;
			size_t albums = getAlbumsHTML(html, filter, domain, media, music::AF_FILTER_FULL, values);

			// Set display header
			setAlbumHeader(wtFormatLibraryHeader, request, filter, albums, domain, values.categories);

			// Store new value in web token and force invalidation!
			wtFormatLibraryBody->setValue(html.html(), true);
			prepared = found = true;
		}

		// Display complete album library?
		if (!found && title == "albums") {

			// Read last filter string from session
			if (filter.empty())
				filter = session["HTML_QUERY_ALBUMS_FILTER"].asString();
			session.add("HTML_QUERY_ALBUMS_FILTER", filter);

			// Read last filter domain from session
			if (limit.empty())
				limit = "all"; // session["HTML_QUERY_ALBUMS_DOMAIN"].asString("all");
			session.add("HTML_QUERY_ALBUMS_DOMAIN", limit);
			music::EFilterDomain domain = getFilterDomain(limit);
			music::EArtistFilter partial = util::isMemberOf(domain, music::FD_ALBUMARTIST,music::FD_COMPOSER,music::FD_CONDUCTOR) ? music::AF_FILTER_PARTIAL : music::AF_FILTER_FULL;

			// Get configured values
			music::CConfigValues values;
			sound.getConfiguredValues(values);

			// Get and set filtered album list as HTML
			util::TStringList html;
			size_t albums = getAlbumsHTML(html, filter, domain, music::EMT_ALL, partial, values);

			// Set display header
			setAlbumHeader(wtAlbumLibraryHeader, request, filter, albums, domain, values.categories);

			// Store new value in web token and force invalidation!
			wtAlbumLibraryBody->setValue(html.html(), true);
			prepared = found = true;
		}

		// Display album library search ?
		if (!found && title == "search") {
			
			// Get search lookup time
			TDateTime ts;
			ts.start();

			// Read last search filter string from session
			if (filter.empty())
				filter = session["HTML_QUERY_SEARCH_FILTER"].asString();
			session.add("HTML_QUERY_SEARCH_FILTER", filter);

			// Read last search domain type from session
			if (limit.empty())
				limit = session["HTML_QUERY_SEARCH_DOMAIN"].asString("all");
			session.add("HTML_QUERY_SEARCH_DOMAIN", limit);

			// Read last search media type from session
			if (type.empty())
				type = session["HTML_QUERY_SEARCH_MEDIA"].asString("all");
			session.add("HTML_QUERY_SEARCH_MEDIA", type);

			// Read last search genre type from session
			if (genre.empty())
				genre = session["HTML_QUERY_SEARCH_GENRE"].asString();
			session.add("HTML_QUERY_SEARCH_GENRE", genre);

			// Get last extended search settings from session
			if (enabled.empty())
				enabled = util::tolower(session["HTML_QUERY_SEARCH_ENABLED"].asString("no"));
			session.add("HTML_QUERY_SEARCH_ENABLED", enabled);
			bool extended = "yes" == enabled;
			if (debugger) {
				aout << "TPlayer::prepareWebRequest() Paramters for library search" << std::endl;
				aout << "  Filter  \"" << filter << "\"" << std::endl;
				aout << "  Limit   \"" << limit << "\"" << std::endl;
				aout << "  Genre   \"" << genre << "\"" << std::endl;
				aout << "  Enabled \"" << enabled << "\"" << std::endl;
			}

			// Get search parameter form session or parameter values
			music::EMediaType media = getMediaType(type);
			if (media == music::EMT_UNKNOWN) {
				media = music::EMT_ALL;
				type = "all";
			}
			music::EFilterDomain domain = getFilterDomain(limit);
			if (domain == music::FD_UNKNOWN) {
				domain = music::FD_ALL;
				limit = "all";
			}
			rdSearchDomain.update(limit);
			rdSearchMedia.update(type);

			if (debugger) {
				aout << "TPlayer::prepareWebRequest() Numeric filter for library search is \"" << filter << "\":" << domain << ":" << media << std::endl;
				aout << "TPlayer::prepareWebRequest() Search list for \"" << limit << "\"" << std::endl;
				aout << rdSearchDomain.html() << std::endl;
			}

			// Read current search configuration parameters
			music::CConfigValues values;
			sound.getConfiguredValues(values);

			util::TStringList genres;
			if (!genre.empty()) {
				// Disable genre filter on wildcard?
				if ('*' == genre[0]) {
					genre.clear();
					extended = false;
				}
			}

			// Update album view for requested filter and media type
			util::TStringList html;
			size_t albums = getSearchHTML(html, filter, (extended ? genre : ""), domain, media, values.displayLimit, values.sortAlbumsByYear, genres);
			std::string caption;
			std::string location;
			std::string displayfilter = html::THTML::encode(filter);
			if (domain != music::FD_ALL) {
				location = util::trim(limit);
				if (!location.empty()) {
					location[0] = std::toupper(location[0]);
				}
			}
			if (media != music::EMT_ALL) {
				if (!location.empty())
					location += "/";
				location += util::toupper(type);
			}
			if (albums > 0) {
				std::string text = (albums != 1) ? " albums" : " album";
				if (albums >= values.displayLimit)
					text = "+" + text;
				caption = "Found " + std::to_string((size_u)albums) + text + " for &quot;" + displayfilter + "&quot;";
			} else {
				if (filter.empty()) {
					caption = "Library search...";
				} else {
					if (filter.size() > 3) {
						caption = "No enries found for &quot;" + displayfilter + "&quot;";
					} else {
						caption = "Filter too short &quot;" + displayfilter + "&quot;";
					}
				}
			}
			if (!filter.empty() && !location.empty()) {
				caption += " (" + location + ")";
			}
			if (!caption.empty()) {
				*wtSearchLibraryHeader = caption;
			}

			// Fill genre drop down box
			const std::string& csv = session["HTML_QUERY_SEARCH_GENRES"].asString();
			std::string selected;
			bool stored = false;
			if (!genres.empty()) {
				bool goon = true;
				if (1 == genres.size()) {
					selected = genre;
					if (genre == genres[0]) {
						goon = false;
					}
				}
				if (goon) {
					lbxGenres.elements().assign(genres);
					session.add("HTML_QUERY_SEARCH_GENRES", genres.asString(';'));
					stored = true;
				}
			} else {
				lbxGenres.elements().assign(csv, ';');
			}

			// Store genres from search result in session
			if (!stored) {
				if (csv.empty()) {
					session.add("HTML_QUERY_SEARCH_GENRES", genres.asString(';'));
				} else {
					session.add("HTML_QUERY_SEARCH_GENRES", csv);
				}
			}

			// Set genre default wildcard selector
			if (lbxGenres.elements().empty())
				lbxGenres.elements().add("*");

			// Set given search genre as selected element in drop down box
			if (selected.empty())
				selected = lbxGenres.elements().at(0);
			session.add("HTML_QUERY_SEARCH_GENRE", selected);
			lbxGenres.update(selected);

			// Get and set search history list box entries
			std::string searched = session["HTML_QUERY_SEARCH_HISTORY"].asString();
			if (debugger) aout << "TPlayer::prepareWebRequest() Search history : albums = " << albums << ", filter = \"" << filter << "\"" << ", history = \"" << searched << "\"" << std::endl;
			if (albums > 0 && !filter.empty()) {
				bool changed = false;
				if (!searched.empty()) {
					if (!util::strcasestr(searched, filter)) {
						// Add current filter to search history
						searched = filter + ";" + searched;
						changed = true;
					}
				} else {
					searched = filter;
					changed = true;
				}
				if (changed) {
					session.add("HTML_QUERY_SEARCH_HISTORY", searched);
				}
			}
			lbxSearch.elements().assign(searched, ';');
			lbxSearch.elements().shrink(8);
			lbxSearch.update(filter);

			// Store new value in web token and force invalidation!
			wtSearchLibraryExtended->setValue(extended ? "yes" : "no", true);
			wtSearchLibraryPattern->setValue(filter, true);
			wtSearchLibraryBody->setValue(html.html(), true);
			prepared = found = true;
			
			// Get search lookup time
			TTimePart dt = ts.stop(ETP_MILLISEC);
			logger(util::csnprintf("[Search] Lookup fulltext search for $ took % milliseconds.", filter, dt));
		}

		// Display album library search ?
		if (!found && title == "added") {

			// Read last filter string from session
			if (debugger) aout << "TPlayer::prepareWebRequest() Get recently added albums." << std::endl;

			// Update album view for requested filter and media type
			music::CConfigValues values;
			sound.getConfiguredValues(values);

			// Store new value in web token and force invalidation!
			util::TStringList html;
			{
				app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
				library.getRecentHTML(html, values.displayLimit);
			}
			wtRecentLibraryBody->setValue(html.html(), true);
			prepared = found = true;
		}

		// Display tracks for single album?
		if (!found && title == "tracks") {
			if (filter.empty())
				filter = session["HTML_QUERY_TRACKS_FILTER"].asString();
			session.add("HTML_QUERY_TRACKS_FILTER", filter);

			size_t count = 0;
			music::CTrackData track;
			std::string codec, format, samples, rate;
			music::PSong song = nil;
			music::CConfigValues values;
			sound.getConfiguredValues(values);
			{
				app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
				jsonAlbumTracks = library.albumAsJSON(filter, values.displayOrchestra);
				htmlAudioElements = library.albumAsAudioElements(filter, "/fs0");
				song = library.getAlbumData(filter, track, codec, format, samples, rate, count);
			}
			if (util::assigned(song) && count > 0) {
				std::string query = util::quote("/rest/thumbnails/%-%.jpg");
				*wtTracksDataTitle = util::quote("<i>" + track.meta.display.originalalbumartist + "</i><br>" + track.meta.display.album  + " (" + track.meta.display.year + ")");
				*wtTracksPreview  = util::csnprintf(query, track.meta.hash.album, 600);
				*wtTracksCoverArt = util::csnprintf(query, track.meta.hash.album, 400);
				*wtTracksCoverHash = track.meta.hash.album;
				*wtTracksArtist = track.meta.track.compilation ? track.meta.display.albumartist : track.meta.display.originalalbumartist;
				*wtTracksSearch = track.meta.text.albumartist;
				*wtTracksAlbum  = values.displayOrchestra ? track.meta.display.extendedinfo : track.meta.display.extendedalbum;
				*wtTracksYear   = track.meta.display.year;
				std::string tracks = (count != 1) ? std::to_string((size_u)count) + " tracks" : "1 track";
				if (track.meta.track.diskcount > 1 && count > 1) {
					std::string disk = (track.meta.track.diskcount != 1) ? std::to_string((size_u)track.meta.track.diskcount) + " disks" : "1 disk";
					*wtTracksCount = tracks + " in " + disk;
				} else {
					*wtTracksCount = tracks;
				}
				if (track.stream.bitsPerSample < 8) {
					*wtTracksFormat = format  + " (" + rate + ")";
					*wtTracksSamples = samples;
				} else {
					*wtTracksFormat = format;
					*wtTracksSamples = samples + " (" + rate + ")";
				}
				*wtTracksCoverURL = track.file.url;
				*wtTracksCodec = codec;
				*wtTracksIcon = song->getIcon();
				*wtTracksAudio = htmlAudioElements;
			} else {
				*wtTracksDataTitle = util::quote("No preview loaded.");
				*wtTracksPreview   = util::quote(NOCOVER_PREVIEW_URL);
				*wtTracksCoverArt  = util::quote(NOCOVER_REST_URL);
				*wtTracksCoverHash = DEFAULT_HASH_TAG;
				*wtTracksArtist = "-";
				*wtTracksSearch = "-";
				*wtTracksAlbum  = "-";
				*wtTracksYear   = "-";
				*wtTracksCount  = "0 tracks";
				*wtTracksCoverURL = "/";
				*wtTracksFormat = "unknown";
				*wtTracksSamples = "0";
				*wtTracksCodec = "unknown";
				*wtTracksIcon = "no-audio";
				*wtTracksAudio = "<!-- Empty audio element list -->\n";
			}
			wtTracksYear->invalidate();
			prepared = found = true;
		}

		// Show recently played songs
		if (!found && title == "recent") {

			// Set recent songs header web token
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
			setRecentHeader(playlists.recent()->size());
			prepared = found = true;
		}

		// Show playlist data by playlist name
		if (!found && title == "playlist") {
			std::string header;

			// Read request playlist (e.g. "state" playlist) form URI parameters
			const std::string name = query["name"].asString("state");
			if (0 == util::strncasecmp("state", name, name.size()))
				header = "Recent songs";

			// Get current active playlist
			if (header.empty()) {
				app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
				if (!name.empty()) {
					if (!playlists.isSelected(name)) {
						// Playlist was changed, update menu entries...
						if (!selectCurrentPlaylist(name)) {
							logger("[Prepare] Set active playlist \"" + name + "\" failed.");
						}
						updatePlaylistMenuItemsWithNolock();
					}
				}
				if (!playlists.getSelected().empty()) {
					header = "Playlist &quot;" + html::THTML::encode(playlists.getSelected()) + "&quot;";
				}
			}

			// Set default header...
			if (header.empty()) {
				header = "Default playlist";
			}

			// Protect reading access for seleted playlist
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);

			// Set current selected playlist in session
			if (!playlists.getSelected().empty()) {
				session.add("HTML_CURRENT_PLAYLIST", playlists.getSelected());
				if (debugger) aout << "TPlayer::prepareWebRequest() Current playlist is set to \"" << session["HTML_CURRENT_PLAYLIST"].asString() << "\" for session " << session[SESSION_ID].asString() << std::endl;
			}

			// Set playlist header web token
			setPlaylistHeader(header, name, playlists.getSelectedSize());

			prepared = found = true;
		}

		// Prepare detection of transition between playlists
		if (!found && title == "playlists") {
			std::string current = session["HTML_CURRENT_PLAYLIST"].asString();
			std::string last = session["HTML_LAST_PLAYLIST"].asString();
			if (!current.empty()) {
				bool ok = false;
				if (last.empty())
					ok = true;
				if (!ok && last != current)
					ok = true;
				if (ok) {
					session.add("HTML_LAST_PLAYLIST", current);
					if (debugger) aout << "TPlayer::prepareWebRequest() Last playlist is set to \"" << session["HTML_LAST_PLAYLIST"].asString() << "\" for session " << session[SESSION_ID].asString() << std::endl;
				}
			}
			found = true;
		}

		// Prepare detection of transition between songs in playlist view
		if (!found && title == "current") {

			// Get current radio text change count
			size_t lastChange = session["HTML_CURRENT_CHANGE"].asInteger(0);
			size_t currentChange = getSongDisplayChange();

			// Store values for text change in session
			session.add("HTML_CURRENT_CHANGE", currentChange);
			session.add("HTML_LAST_CHANGE", lastChange);

			found = true;
		}

		// Prepare detection of transition of library update
		if (!found && title == "scanner") {

			// Get current library update count
			size_t lastUpdate = session["HTML_CURRENT_UPDATE"].asInteger(0);
			size_t currentUpdate = getScannerDisplayUpdate();

			// Store values for library update in session
			session.add("HTML_CURRENT_UPDATE", currentUpdate);
			session.add("HTML_LAST_UPDATE", lastUpdate);

			found = true;
		}

		// Prepare detection of transition between player modes
		if (!found && title == "modes") {

			// Get current mode
			TPlayerMode mode;
			getCurrentMode(mode);
			std::string current = modeToStr(mode);
			std::string last = session["HTML_CURRENT_MODE"].asString();

			// Set defaults...
			if (current.empty())
				current = "none";
			if (last.empty())
				last = "none";

			// Store values for songs in session
			session.add("HTML_CURRENT_MODE", current);
			session.add("HTML_LAST_MODE", last);
			found = true;
		}

		// Update radio text transition counters
		if (!found && title == "radiotext") {
			size_t current = radio.counters.text;
			size_t last = session["HTML_STREAM_CURRENT_TEXT_COUNT"].asInteger(0);
			session.add("HTML_STREAM_LAST_TEXT_COUNT", last);
			session.add("HTML_STREAM_CURRENT_TEXT_COUNT", current);
			prepared = found = true;
		}

		// Update radio station transition counters
		if (!found && title == "radioplay") {
			size_t current = radio.counters.station;
			size_t last = session["HTML_STREAM_CURRENT_STATION_COUNT"].asInteger(0);
			session.add("HTML_STREAM_LAST_STATION_COUNT", last);
			session.add("HTML_STREAM_CURRENT_STATION_COUNT", current);

			// Set stations header
			std::string name;
			if (getStreamName(name)) {
				wtStationsHeader->setValue("Playing internet stream &quot;" + name + "&quot;", true);
			} else {
				wtStationsHeader->setValue("Internet Radio Stations", true);
			}

			prepared = found = true;
		}

		// Store current values in session
		session.add("HTML_QUERY_TITLE", title);
		session.add("HTML_QUERY_FILTER", filter);
		session.add("HTML_QUERY_TYPE", type);

		if (debugger) {
			session.debugOutput("Session variable", "TPlayer::prepareWebRequest() ");
			aout << endl;
		}
	}
}

#undef USE_CACHED_LIBRARY_CONTENT

size_t TPlayer::getSearchHTML(util::TStringList& html, const std::string& filter, const std::string& genre, const music::EFilterDomain domain, const music::EMediaType media, const size_t max, const bool sortByYear, util::TStringList& genres) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
	return library.getSearchHTML(html, filter, genre, domain, media, max, sortByYear, genres);
}

size_t TPlayer::getAlbumsHTML(util::TStringList& html, const std::string& filter, const music::EFilterDomain domain, const music::EMediaType type, const music::EArtistFilter partial, const music::CConfigValues& config) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
	return library.getAlbumsHTML(html, filter, domain, type, partial, config);
}

music::TLibraryResults TPlayer::getArtistsHTML(util::TStringList& html, const std::string& filter, music::EMediaType type, const music::CConfigValues& config, const music::TLibrary::EViewType view) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
	return library.getArtistsHTML(html, filter, type, config, view);
}


bool checkVariousArtists(const std::string name, const util::TStringList& various) {
	if (!various.empty()) {
		for (size_t i=0; i<various.size(); ++i) {
			if (util::strcasestr(name, various[i]))
				return true;
		}
	}
	return false;
}

void TPlayer::setAlbumHeader(PWebToken token, const std::string& request, const std::string& filter, const size_t albums,
		const music::EFilterDomain domain, const util::TStringList& various) {
	if (!filter.empty()) {
		std::string text = (albums != 1) ? " albums)" : " album)";
		std::string name, hash;
		if (library.getArtistInfo(request, name, hash)) {
			std::string displayname = html::THTML::encode(name);
			std::string hint;
			switch (domain) {
				case music::FD_COMPOSER:
					hint = "Composer";
					break;
				case music::FD_CONDUCTOR:
					hint = "Conductor";
					break;
				case music::FD_TITLE:
					hint = "Title";
					break;
				default:
					hint = checkVariousArtists(name, various) ? "Category" : "Artist";
					break;
			}
			if (hint.empty()) {
				*token = "&quot;" + displayname + "&quot; (" + std::to_string((size_u)albums) + text;
			} else {
				*token = hint + " &quot;" + displayname + "&quot; (" + std::to_string((size_u)albums) + text;
			}
		} else {
			// Ignore placeholder "*" from frontend
			if (request.size() <= 1) {
				std::string hint;
				switch (domain) {
					case music::FD_ALBUMARTIST:
						hint = "Missing albumartist tag";
						break;
					case music::FD_COMPOSER:
						hint = "Missing composer tag";
						break;
					case music::FD_CONDUCTOR:
						hint = "Missing conductor tag";
						break;
					case music::FD_ARTIST:
						hint = "Missing artist tag";
						break;
					case music::FD_TITLE:
						hint = "Missing title tag";
						break;
					default:
						hint = "Missing search tag";
						break;
				}
				*token = hint + " (" + std::to_string((size_u)albums) + text;
			} else {
				std::string displayname = html::THTML::encode(request);
				std::string hint;
				switch (domain) {
					case music::FD_COMPOSER:
						hint = "Composer";
						break;
					case music::FD_CONDUCTOR:
						hint = "Conductor";
						break;
					case music::FD_TITLE:
						hint = "Title";
						break;
					case music::FD_ARTIST:
					case music::FD_ALBUMARTIST:
						hint = checkVariousArtists(request, various) ? "Category" : "Artist";
						break;
					default:
						break;
				}
				if (hint.empty()) {
					*token = "&quot;" + displayname + "&quot; (" + std::to_string((size_u)albums) + text;
				} else {
					*token = hint + " &quot;" + displayname + "&quot; (" + std::to_string((size_u)albums) + text;
				}
			}
		}
		token->invalidate();
	}
}


void TPlayer::defaultWebAction(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	logger("[Event] Default web action for [" + key + "] = [" + util::ellipsis(value, 65) + "]");
	if (debug && !params.empty())
		params.debugOutput("Parameter", "TPlayer::defaultAction() ");
}


void TPlayer::onRowPlaylistClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	logger("[Event] Web action for [" + key + "] = [" + value + "]");

	// Store selected song properties in local copy
	music::PSong song = nil;
	{
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		song = library.findFile(value);
	}
	if (util::assigned(song)) {
		std::string playlist = params["playlist"].asString();
		setSelectedSong(song, playlist);

		// Debug output
		if (debug) {
			aout << app::green << "TPlayer::onRowPlaylistClick() Song data for file hash \"" << value << "\" and playlist \"" << playlist << "\"" << endl;
			song->getTags().debugOutput("  Metadata   : ");
			aout << "  Metadata   : " << song->getTags().isValid() << endl;
			aout << "  Streamdata : " << song->getStreamData().isValid() << endl;
			aout << "  Filedata   : " << song->getFileTags().isValid() << app::magenta << endl;
			aout << "    Artist   = " << song->getTags().text.artist << endl;
			aout << "    Album    = " << song->getTags().text.album << endl;
			aout << "     Hash    = " << song->getTags().hash.album << endl;
			aout << "    Title    = " << song->getTags().text.title << endl;
			aout << "     Hash    = " << song->getTags().hash.title << endl;
			aout << "  Path       = " << song->getFileTags().file.folder << app::blue << endl;
			aout << "  URL        = " << song->getFileTags().file.url << app::reset << endl;
		}
	} else {
		clearSelectedSong();
		if (debug) aout << app::red << "TPlayer::onRowPlaylistClick() No song selected." << app::reset << endl;
	}

}


void TPlayer::onConfigData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	logger("[Event] Set configuration parameters for [" + key + "] = [" + value + "]");
	if (debug) params.debugOutput("TPlayer::onConfigData()", "  ");

	// Read configuration values from parameter list
	music::CConfigValues values;
	sound.getConfiguredValues(values);
	bool dithered = values.enableDithering;
	bool sensitive = values.sortCaseSensitive;
	std::string limit, period, rows;
	bool ok = false;

	// Do not save parameters during active library scan...
	{
		app::TLockGuard<app::TMutex> lock(rescanMtx, false);
		if (!lock.tryLock())
			return;

		values.musicpath1 = util::validPath(params["Musicpath1"].asString());
		values.musicpath2 = util::validPath(params["Musicpath2"].asString());
		values.musicpath3 = util::validPath(params["Musicpath3"].asString());
		values.enableMusicPath1 = params["EnableFolder1"].asBoolean();
		values.enableMusicPath2 = params["EnableFolder2"].asBoolean();
		values.enableMusicPath3 = params["EnableFolder3"].asBoolean();
		values.allowFullNameSwap = params["AllowFullNameSwap"].asBoolean();
		values.allowGroupNameSwap = params["AllowGroupNameSwap"].asBoolean();
		values.allowArtistNameRestore = params["AllowArtistNameRestore"].asBoolean();
		values.allowTheBandPrefixSwap = params["AllowTheBandPrefixSwap"].asBoolean();
		values.allowDeepNameInspection = params["AllowDeepNameInspection"].asBoolean();
		values.allowVariousArtistsRename = params["AllowVariousArtistsRename"].asBoolean();
		values.allowMovePreamble = params["AllowMovePreamble"].asBoolean();
		values.watchLibraryEnabled = params["WatchLibraryEnabled"].asBoolean();
		values.sortCaseSensitive = params["SortCaseSensitive"].asBoolean();
		values.sortAlbumsByYear = params["SortAlbumsByYear"].asBoolean();
		values.displayOrchestra = params["DisplayOrchestra"].asBoolean();
		values.displayRemain = params["DisplayRemainingTime"].asBoolean();
		values.displayLimit = params["DisplayCountLimit"].asInteger(36);
		values.enableDithering = params["EnableDithering"].asBoolean();

		// Get valid period time
		music::snd_pcm_uint_t periodTime = params["PeriodTime"].asUnsigned(0);
		if (periodTime > 0) {
			values.periodTime = periodTime;
		}

		std::string categories = params["VariousArtistsCategories"].asString();
		std::string pattern = params["Filepattern"].asString();
		std::string device = util::trim(params["Device"].asString());

		rows = params["TablePageLimit"].asString("10");
		values.pageLimit = (rows != "All") ? util::strToInt(rows, 10) : 9999;

		// Check for valid device
		if (">>" != device.substr(0, 2))
			values.device = device;

		// Normalize device (strip comments)
		size_t pos = values.device.find(" [");
		if (pos != std::string::npos) {
			values.device = values.device.substr(0, pos);
		}

		// Set default values
		if (pattern.size() < 5)
			pattern = "*.wav;*.flac;*.dsf;*.dff;*.aif;*.m4a;*.mp3";
		values.pattern = pattern;

		if (categories.size() < 5)
			categories = "sampler;various;soundtrack;compilation;divers";
		values.categories = categories;

		// Log human readable option list
		std::string options;
		if (values.allowGroupNameSwap) {
			if (!options.empty())
				options += ", ";
			options += "AllowGroupNameSwap";
		}
		if (values.allowArtistNameRestore) {
			if (!options.empty())
				options += ", ";
			options += "AllowArtistNameRestore";
		}
		if (values.allowFullNameSwap) {
			if (!options.empty())
				options += ", ";
			options += "AllowFullNameSwap";
		}
		if (values.allowTheBandPrefixSwap) {
			if (!options.empty())
				options += ", ";
			options += "AllowTheBandPrefixSwap";
		}
		if (values.allowDeepNameInspection) {
			if (!options.empty())
				options += ", ";
			options += "AllowDeepNameInspection";
		}
		if (values.allowVariousArtistsRename) {
			if (!options.empty())
				options += ", ";
			options += "AllowVariousArtistsRename";
		}
		if (values.allowMovePreamble) {
			if (!options.empty())
				options += ", ";
			options += "AllowMovePreamble";
		}

		std::string sort = values.sortAlbumsByYear ? "Year" : "Name";
		std::string order = values.sortCaseSensitive ? "Sensitive" : "Insensitive";
		std::string view = values.displayOrchestra ? "Extended" : "Simple";
		std::string watch = values.watchLibraryEnabled ? "Enabled" : "Disabled";
		std::string remain = values.displayRemain ? "Yes" : "No";
		std::string dither = values.enableDithering ? "Yes" : "No";
		std::string path1 = values.enableMusicPath1 ? "Yes" : "No";
		std::string path2 = values.enableMusicPath2 ? "Yes" : "No";
		std::string path3 = values.enableMusicPath3 ? "Yes" : "No";
		limit = std::to_string((size_u)values.displayLimit);
		period = std::to_string((size_u)values.periodTime);
		logger("[Settings] Options        [" + options + "]");
		logger("[Settings] Device         [" + values.device + "]");
		logger("[Settings] Musicpath 1    [" + values.musicpath1 + "]");
		logger("[Settings]   Enabled 1    [" + path1 + "]");
		logger("[Settings] Musicpath 2    [" + values.musicpath2 + "]");
		logger("[Settings]   Enabled 2    [" + path2 + "]");
		logger("[Settings] Musicpath 3    [" + values.musicpath3 + "]");
		logger("[Settings]   Enabled 3    [" + path3 + "]");
		logger("[Settings] Filepattern    [" + values.pattern.csv() + "]");
		logger("[Settings] Categories     [" + values.categories.csv() + "]");
		logger("[Settings] Library watch  [" + watch + "]");
		logger("[Settings] Display limit  [" + limit + "]");
		logger("[Settings] Table rows     [" + rows + "]");
		logger("[Settings] Sort case      [" + order + "]");
		logger("[Settings] Sort order key [" + sort + "]");
		logger("[Settings] Display remain [" + remain + "]");
		logger("[Settings] Album view     [" + view + "]");
		logger("[Settings] Dithering      [" + dither + "]");
		logger("[Settings] Period time    [" + period + "]");

		ok = true;
	}
	if (ok) {
		// Write configuration
		sound.setConfiguredValues(values);

		// Set application working path to music folder
		util::TStringList folders;
		sound.getMusicFolders(folders);
		application.setWorkingFolders(folders);

		// Update HTML combo box entries for drives and audio devices
		updateComboBoxes(values, limit, period, rows);

		// Update web interface menu entries
		updateLibraryMediaItems();

		// Check for sort order settings changed
		if (sensitive != values.sortCaseSensitive) {
			library.setSortMode(values.sortCaseSensitive ? music::TLibrary::ELS_CASE_SENSITIVE : music::TLibrary::ELS_CASE_INSENSITIVE);
		}

		// Check for dither setting
		if (dithered != values.enableDithering) {
			// Check for protection by mutex...
			player.setDithered(values.enableDithering);
		}
	}
}

void TPlayer::updateComboBoxes(const music::CConfigValues& values, const std::string& limit, const std::string& period, const std::string& rows) {
	{
		app::TLockGuard<app::TMutex> lock(deviceMtx);
		cbxDevices.update(values.device);
		lbxDevices.update(values.device);
	}
	{
		app::TLockGuard<app::TMutex> lock(driveMtx);
		cbxDrives.update(values.musicpath1);
		lbxDrives1.update(values.musicpath1);
		lbxDrives2.update(values.musicpath2);
		lbxDrives3.update(values.musicpath3);
	}
	{
		app::TLockGuard<app::TMutex> lock(historyMtx);
		lbxHistory.update(limit);
	}
	{
		app::TLockGuard<app::TMutex> lock(pagesMtx);
		lbxPages.update(rows);
	}
	{
		app::TLockGuard<app::TMutex> lock(timesMtx);
		lbxTimes.update(period);
	}

	// Set web token value
	if (util::assigned(wtTableRowCount))
		wtTableRowCount->setValue(rows);
}


void TPlayer::onRemoteData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	logger("[Event] Set remote configuration for [" + key + "] = [" + value + "]");
	if (debug) params.debugOutput("TPlayer::onRemoteData()", "  ");
	bool ok = false;

	// Read configuration values from parameter list
	music::CRemoteValues values;
	sound.getRemoteValues(values);

	// Do not save parameters during active library scan...
	{
		app::TLockGuard<app::TMutex> lock(rescanMtx, false);
		if (!lock.tryLock())
			return;

		std::string device = util::trim(params["Device"].asString());
		std::string terminal = util::trim(params["Terminal"].asString());
		values.enabled = params["RemoteDeviceEnabled"].asBoolean();
		values.canonical = params["CanonicalSampleRate"].asBoolean();
		values.control =  params["TerminalDeviceEnabled"].asBoolean();

		// Check for valid devices
		if (">>" != device.substr(0, 2))
			values.device = device;
		if (">>" != terminal.substr(0, 2))
			values.terminal = terminal;

		// Normalize devices (strip comments)
		size_t pos = values.device.find(" [");
		if (pos != std::string::npos) {
			values.device = values.device.substr(0, pos);
		}
		pos = values.terminal.find(" [");
		if (pos != std::string::npos) {
			values.terminal = values.terminal.substr(0, pos);
		}

		// Log human readable option list
		std::string options;
		if (values.enabled) {
			if (!options.empty())
				options += ", ";
			options += "RemoteDeviceEnabled";
		}
		if (values.canonical) {
			if (!options.empty())
				options += ", ";
			options += "CanonicalSampleRate";
		}
		if (values.control) {
			if (!options.empty())
				options += ", ";
			options += "TerminalDeviceEnabled";
		}

		logger("[Remote] Options  [" + options + "]");
		logger("[Remote] Device   [" + values.device + "]");
		logger("[Remote] Remote   [" + values.terminal + "]");

		ok = true;
	}
	if (ok) {
		// Update HTML combo box entries for drives and audio devices
		sound.setRemoteValues(values);
		updateSerialDevices(values);

		// Update buttons with new values
		music::CAudioValues audio;
		sound.getAudioValues(audio);
		app::ECommandAction currentInput = getCurrentInput();
		music::ESampleRate remoteRate = getRemoteRate();
		{
			app::TLockGuard<app::TMutex> lock(componentMtx);
			updateSelectInputButtonsWithNolock(audio, currentInput);
			updateSelectFilterButtonsWithNolock(audio, ECA_INVALID_ACTION);
			updateSelectPhaseButtonsWithNolock(audio, ECA_INVALID_ACTION);
			updateActiveRateButtonsWithNolock(audio, remoteRate);
		}

		// Send serial status request to remote device
		trySendRemoteStatusRequest();
	}
}

void TPlayer::updateSerialDevices(const music::CRemoteValues& values) {
	app::TLockGuard<app::TMutex> lock(deviceMtx);
	lbxRemote.update(values.device);
	lbxSerial.update(values.terminal);
}


void TPlayer::onProgressClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	double progress = params["progress"].asDouble(-1.0, app::cc_CC);
	std::string action = params["action"].asString();
	logger(util::csnprintf("[Event] Progress bar data for [" + key + "] $ = % %%", action, progress));
	if (!action.empty() && progress > (double)0.0 && progress < (double)100.0) {
		if (0 == util::strncasecmp(action, "slide", 5)) {
			player.queueSeekCommand(progress);
		} else if (0 == util::strncasecmp(action, "click", 5)) {
			player.addSeekCommand(progress);
		}
	}
}


void TPlayer::onSearchData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	std::string filter = params["filter"].asString();
	std::string limit = params["domain"].asString("all");
	std::string type = params["type"].asString("all");
	std::string displayfilter = html::THTML::encode(filter);
	logger(util::csnprintf("[Event] Search data for [" + key + "] = $:$:$", filter, limit, type));
	
	// Get search lookup time
	TDateTime ts;
	ts.start();

	// Catch missing parameters
	music::EMediaType media = getMediaType(type);
	if (media == music::EMT_UNKNOWN) {
		media = music::EMT_ALL;
	}
	music::EFilterDomain domain = getFilterDomain(limit);
	if (domain == music::FD_UNKNOWN) {
		domain = music::FD_ALL;
	}

	// Update album view for requested filter and media type
	bool changed;
	music::CConfigValues values;
	sound.getConfiguredValues(values);

	// Protect library access
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	size_t albums = library.updateSearchHTML(filter, domain, media, changed, values.displayLimit, values.sortAlbumsByYear);
	if (changed) {
		if (albums > 0) {
			std::string text = (albums != 1) ? " albums)" : " album)";
			if (albums >= values.displayLimit)
				text = "+" + text;
			*wtSearchLibraryHeader = "Found " + std::to_string((size_u)albums) + text + " for &quot;" + displayfilter + "&quot;";
		} else {
			if (filter.empty()) {
				*wtSearchLibraryHeader = "Library search...";
			} else {
				*wtSearchLibraryHeader = "No enries found for &quot;" + displayfilter + "&quot;";
			}
		}

		// Store new value in web token and force invalidation!
		wtSearchLibraryBody->setValue(library.searchAsHTML(), true);
	}

	// Get search lookup time
	TTimePart dt = ts.stop(ETP_MILLISEC);
	logger(util::csnprintf("[Search] Lookup fulltext search for $ took % milliseconds.", filter, dt));
}


void TPlayer::onDialogData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	bool found = false;
	bool changed = false;
	std::string playlist;
	logger(util::csnprintf("[Event] Dialog data for [%] = [%]", key, value));

	// Create a new playlist for given name
	if (!found && 0 == util::strcasecmp("NEWPLAYLIST", value)) {
		std::string name = params["input"].asString();
		if (!name.empty()) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
			if (!playlists.isValid(name)) {
				playlists.add(name);
				if (!selectCurrentPlaylist(name)) {
					logger("[Dialogue] Set new playlist \"" + name + "\" failed.");
				} else {
					playlist = name;
				}
				changed = true;
			}
		}
		found = true;
	}

	// Rename selected playlist
	if (!found && 0 == util::strcasecmp("RENAMEPLAYLIST", value)) {
		std::string name = params["selected"].asString();
		std::string renamed = params["renamed"].asString();
		if (!name.empty() && !value.empty()) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
			if (playlists.isValid(name)) {
				if (!renameCurrentPlaylist(name, renamed)) {
					logger("[Dialogue] Rename playlist \"" + name + "\" to \"" + renamed + "\" failed.");
				} else {
					playlist = renamed;
				}
				changed = true;
			}
		}
		found = true;
	}

	// Switch viewport to selected playlist
	if (!found && 0 == util::strcasecmp("SELECTPLAYLIST", value)) {
		std::string name = params["selected"].asString();
		if (!name.empty()) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
			if (playlists.isValid(name)) {
				if (!selectCurrentPlaylist(name)) {
					logger("[Dialogue] Set active playlist \"" + name + "\" failed.");
				} else {
					playlist = name;
				}
				changed = true;
			}
		}
		found = true;
	}

	// Delete given playlist
	if (!found && 0 == util::strcasecmp("DELETEPLAYLIST", value)) {
		std::string name = params["selected"].asString();
		if (!name.empty()) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
			if (playlists.isValid(name)) {
				bool ok = true;
				if (playlists.isPlaying(name)) {
					// Stop ALSA playback
					bool r = player.stop();
					if (r) {
						logger("[Dialogue] Delete active playlist, device <" + util::strToStr(player.getCurrentDevice(), "none") + "> stopped.");
					} else {
						logger("[Dialogue] Stopping device <" + util::strToStr(player.getCurrentDevice(), "none") + "> for active playlist failed.");
						ok = false;
					}
				}
				if (ok) {
					playlists.remove(name);
					playlist.clear();
					changed = true;
				}
			}
		}
		found = true;
	}

	// Set properties on list changed
	if (changed) {
		std::string selected;
		size_t rsize, ssize;
		{
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
			selected = playlists.getSelected();
			ssize = playlists.getSelectedSize();
			rsize = playlists.recent()->size();
		}

		std::string header = "Playlist &quot;" + html::THTML::encode(selected) + "&quot;";
		setPlaylistHeader(header, selected, ssize);
		setRecentHeader(rsize);

		app::TLockGuard<app::TMutex> lock(plsDisplayMtx, false);
		htmlSelectPlaylist.clear();
		htmlRenamePlaylist.clear();
		htmlDeletePlaylist.clear();
	}

	// Update main menu
	if (found) {
		updatePlaylistMenuItems();
	}
}



void TPlayer::onEditorData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	bool found = false;
	logger(util::csnprintf("[Event] Editor data for [%] = [%]", key, value));

	std::string hash = params["value"].asString();
	std::string name = params["name"].asString();
	std::string url = params["url"].asString();
	std::string mode = params["mode"].asString();
	radio::EMetadataOrder order = radio::TStations::strToOrderMode(mode);

	// Create a new station prperties
	if (!found && 0 == util::strcasecmp("CREATESTREAM", value)) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_WRITE);
		if (name.size() > 1 && url.size() > 1) {
			if (stations.add(name, url, order)) {
				logger(util::csnprintf("[Dialogue] Added new station : Name $ URL $", name, url));
			} else {
				logger(util::csnprintf("[Dialogue] Failed to add new station : Name $ URL $", name, url));
			}
		} else {
			logger(util::csnprintf("[Dialogue] Invalid station properties : Name $ URL $", name, url));
		}
		toUndoAction->restart();
		found = true;
	}

	// Edit station properties for given hash
	if (!found && 0 == util::strcasecmp("EDITSTREAM", value)) {
		if (!hash.empty()) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_WRITE);
			if (name.size() > 1 && url.size() > 1) {
				if (stations.edit(hash, name, url, order)) {
					logger(util::csnprintf("[Dialogue] Changed properties for $ : Name $ URL $", hash, name, url));
				} else {
					logger(util::csnprintf("[Dialogue] Changing properties failed for $ : Name $ URL $", hash, name, url));
				}
			} else {
				logger(util::csnprintf("[Dialogue] Invalid station properties : Name $ URL $", name, url));
			}
		}
		toUndoAction->restart();
		found = true;
	}

	// Delete station for given hash
	if (!found && 0 == util::strcasecmp("DELETESTREAM", value)) {
		if (hash.size() == 32) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_WRITE);
			if (stations.remove(hash)) {
				logger(util::csnprintf("[Dialogue] Delete station $", hash));
			} else {
				logger(util::csnprintf("[Dialogue] Delete station $ failed.", hash));
			}
		} else {
			logger(util::csnprintf("[Dialogue] Invalid station hash $", hash));
		}
		toUndoAction->restart();
		found = true;
	}

}


void TPlayer::onUndoActionTimeout() {
	// Clear undo list after 60 seconds...
	clearStreamUndoList();
}


void TPlayer::onButtonClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	logger("[Event] [Button] [" + key + "] = [" + value + "]");
	if (!terminate) {
		// Immediately execute system actions in web server thread (!)
		ECommandAction action = actions[value];
		if (!executeSystemAction(action)) {
			if (!executeRepeatModeAction(action)) {
				bool execute = false;
				music::CSongData song;
				std::string playlist;
				switch (action) {
					case ECA_PLAYER_STOP:
					case ECA_REMOTE_INPUT0:
					case ECA_REMOTE_INPUT1:
					case ECA_REMOTE_INPUT2:
					case ECA_REMOTE_INPUT3:
					case ECA_REMOTE_INPUT4:
						execute = true;
						break;

					case ECA_PLAYER_NEXT:
					case ECA_PLAYER_PREV:
						// Skip current song in current playlist
						getCurrentSong(song, playlist);
						break;

					default:
						// Just take what was selected by user(s)
						getCurrentSong(song, playlist);
						if (!song.valid)
							getLastSong(song, playlist);
						if (!song.valid)
							getSelectedSong(song, playlist);
						break;
				}
				if (execute || song.valid) {
					TCommand command;
					command.action = action;
					command.playlist = playlist;
					command.file = song.fileHash;
					command.album = song.albumHash;
					queueCommand(command);
				} else {
					logger("[Event] [Button] [" + key + "] = [" + value + "] No valid song found, command ignored.");
				}
			}
		}
	}
}

void TPlayer::onLibraryClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	logger("[Event] [Library] [" + key + "] = [" + value + "]");
	std::string album = params["hash"].asString();
	std::string artist = util::TURL::decode(params["artist"].asString());
	std::string current = session["HTML_CURRENT_PLAYLIST"].asString();
	std::string recent;
	ECommandAction action = actions[value];

	{ // Read playlists
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		recent = playlists.recent()->getName();
		if (current.empty())
			current = playlists.getSelected();
	}

	// Prepare given action
	bool execute = true;
	TCommand command;
	switch (action) {
		case ECA_PLAYLIST_ADD_ARTIST:
			command.artist = artist;
			command.playlist = current;
			break;
		case ECA_PLAYLIST_ADD_ALBUM:
			command.album = album;
			command.playlist = current;
			break;
		case ECA_PLAYLIST_ADD_PLAY_ARTIST:
			command.artist = artist;
			command.playlist = recent;
			break;
		case ECA_PLAYLIST_ADD_PLAY_ALBUM:
			command.album = album;
			command.playlist = recent;
			break;
		default:
			execute = false;
			break;
	}

	// Immediately execute playlist actions in web server thread
	if (execute) {
		command.action = action;
		if (!executePlaylistAction(command)) {
			// Add player action for deferred execution
			queueCommand(command);
		}
	}
}

void TPlayer::onContextMenuClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	std::string param = params["action"].asString();
	std::string album = params["album"].asString();
	std::string playlist = params["playlist"].asString();
	const std::string& recent = playlists.recent()->getName();
	ECommandAction action = actions[param];
	bool execute = false;
	bool needed = true;

	// Check for "any" playlist placeholder
	// --> Set by Javascript when row.Playlist === null
	if (1 == playlist.size()) {
		if (playlist == "*")
			playlist.clear();
	}

	logger("[Event] [Context] [" + key + "] = [" + value + "] Action = \"" + param + "\" for playlist = \"" + playlist + "\"");
	if (debug) params.debugOutput("TPlayer::onContextMenuClick()", "  ");

	// Check playlist for given command
	std::string current;
	switch (action) {
		case ECA_PLAYLIST_ADD_SONG:       // Add to selected playlist
		case ECA_PLAYLIST_ADD_ALBUM:      // Add to selected playlist
		case ECA_PLAYLIST_ADD_ARTIST:     // Add to selected playlist
		case ECA_PLAYLIST_ADD_NEXT_SONG:  // Add to selected playlist and play next
		case ECA_PLAYLIST_ADD_NEXT_ALBUM: // Add to selected playlist and play next
			current = session["HTML_CURRENT_PLAYLIST"].asString();
			if (!current.empty()) {
				logger("[Event] [Context] Current session playlist = \"" + current + "\"");
				playlist = current;
				execute = true;
			} else {
				logger("[Event] [Context] No session playlist found, command ignored.");
			}
			break;

		case ECA_PLAYLIST_ADD_PLAY_SONG:   // Add to "state" playlist
		case ECA_PLAYLIST_ADD_PLAY_ALBUM:  // Add to "state" playlist
		case ECA_PLAYLIST_ADD_PLAY_ARTIST: // Add to "state" playlist
			playlist = recent;
			execute = true;
			break;

		case ECA_PLAYLIST_NEXT_SONG:
			if (playlist.empty()) {
				// Add next song to "state" playlist by default
				// --> ECA_PLAYLIST_ADD_NEXT_SONG;
				playlist = recent;
			}
			execute = true;
			break;

		case ECA_PLAYLIST_NEXT_ALBUM:
			if (playlist.empty()) {
				// Add next album to "state" playlist by default
				// --> ECA_PLAYLIST_ADD_NEXT_ALBUM;
				playlist = recent;
			}
			execute = true;
			break;

		case ECA_LIBRARY_CLEAR_IMAGE:
		case ECA_LIBRARY_CLEAR_CACHE:
			playlist.clear(); // No playlist needed
			execute = true;
			needed = false;
			break;

		default:
			if (!playlist.empty())
				execute = true;
			break;
	}

	// Check for valid playlist
	if (execute) {
		music::PSong song = nil;
		{
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
			if (playlists.isValid(playlist)) {
				// Store selected song properties in local copy
				song = library.findFile(value);
			} else {
				if (needed) {
					logger("[Event] [Context] Invalid session playlist = \"" + playlist + "\", command ignored.");
					execute = false;
				}
			}
		}
		if (util::assigned(song)) {
			setSelectedSong(song, playlist);
		}
	}

	// Execute given command
	if (execute) {
		// Fill command structure
		TCommand command;
		command.action = action;
		command.album = album;
		command.file = value;
		command.playlist = playlist;

		// Immediately execute playlist actions in web server thread (!)
		if (!executePlaylistAction(command)) {
			// Add player action for deferred execution
			logger(util::csnprintf("[Event] [Context] Queue command action [%]", command.action));
			queueCommand(command);
		}
	}
}


bool TPlayer::executePlaylistAction(TCommand& command) {
	bool executed = true;
	bool recent = playlists.isRecent(command.playlist);
	logger(util::csnprintf("[Action] Execute command action [%]", command.action));

	// Add song/album to playlist
	switch (command.action) {
		case ECA_PLAYLIST_DELETE_ALBUM:
			// Delete in given playlist
			deleteAlbum(command.album, command.playlist);
			util::wait(100); // Synchronous response in playlist view
			break;

		case ECA_PLAYLIST_ADD_PLAY_ALBUM:
			// Add to "state" playlist
			if (recent) {
				addTracks(command.album, command.playlist, music::EPA_INSERT);
			} else {
				addAlbum(command.album, command.playlist, music::EPA_APPEND);
			}
			executed = false; // Procession NOT finished, issue play command afterwards
			break;

		case ECA_PLAYLIST_ADD_PLAY_ARTIST:
			// Add to "state" playlist
			command.album = addArtist(command.artist, command.playlist, music::EPA_INSERT);
			executed = false; // Procession NOT finished, issue play command afterwards
			break;

		case ECA_PLAYLIST_NEXT_ALBUM:
			// Add to selected playlist
			if (recent) {
				addTracks(command.album, command.playlist, music::EPA_INSERT);
			} else {
				addAlbum(command.album, command.playlist, music::EPA_APPEND);
			}
			executed = false; // Procession NOT finished, issue queue command afterwards
			break;

		case ECA_PLAYLIST_ADD_NEXT_ALBUM:
			// Add to selected playlist
			if (recent) {
				addTracks(command.album, command.playlist, music::EPA_INSERT);
			} else {
				addAlbum(command.album, command.playlist, music::EPA_APPEND);
			}
			executed = false; // Procession NOT finished, issue queue command afterwards
			break;

		case ECA_PLAYLIST_ADD_ALBUM:
			// Add to selected playlist
			addAlbum(command.album, command.playlist, music::EPA_APPEND);
			break;

		case ECA_PLAYLIST_ADD_ARTIST:
			// Add to selected playlist
			command.album = addArtist(command.artist, command.playlist, music::EPA_APPEND);
			break;

		case ECA_PLAYLIST_DELETE_SONG:
			// Add to selected playlist
			deleteSong(command.file, command.playlist);
			util::wait(100); // Synchronous response in playlist view
			break;

		case ECA_PLAYLIST_ADD_PLAY_SONG:
			// Add to selected playlist
			if (recent) {
				addTrack(command.file, command.playlist, music::EPA_INSERT);
			} else {
				addSong(command.file, command.album, command.playlist, music::EPA_APPEND);
			}
			executed = false; // Procession NOT finished, issue play command afterwards
			break;

		case ECA_PLAYLIST_NEXT_SONG:
			// Add to selected playlist
			if (recent) {
				addTrack(command.file, command.playlist, music::EPA_INSERT);
			} else {
				addSong(command.file, command.album, command.playlist, music::EPA_APPEND);
			}
			executed = false; // Procession NOT finished, issue queue command afterwards
			break;

		case ECA_PLAYLIST_ADD_NEXT_SONG:
			// Add to selected playlist
			if (recent) {
				addTrack(command.file, command.playlist, music::EPA_INSERT);
			} else {
				addSong(command.file, command.album, command.playlist, music::EPA_APPEND);
			}
			executed = false; // Procession NOT finished, issue queue command afterwards
			break;

		case ECA_PLAYLIST_ADD_SONG:
			// Add to selected playlist
			addSong(command.file, command.album, command.playlist, music::EPA_APPEND);
			break;

		case ECA_LIBRARY_CLEAR_IMAGE:
			deleteCachedFiles(command.file);
			break;

		case ECA_LIBRARY_CLEAR_CACHE:
			invalidateFileCache();
			break;

		default:
			executed = false;
			break;
	}

	// Set new playlist statistics
	if (executed) {
		updateLibraryStatus();
	}

	return executed;
}

bool TPlayer::executeSystemAction(const app::ECommandAction action) {
	bool r = true;
	switch (action) {
		case ECA_SYSTEM_LOAD_LICENSE:
			downloadApplicationLicenseWithDelay();
			break;

//		case ECA_LIBRARY_RESCAN_LIBRARY:
//			rescanLibrary();
//			break;
//
//		case ECA_LIBRARY_REBUILD_LIBRARY:
//			rebuildLibrary();
//			break;
//
//		case ECA_LIBRARY_CLEAR_LIBRARY:
//			deleteLibrary();
//			break;

		case ECA_LIBRARY_RESCAN_LIBRARY:
		case ECA_LIBRARY_REBUILD_LIBRARY:
		case ECA_LIBRARY_CLEAR_LIBRARY:
			notifyLibraryAction(action);
			break;

		case ECA_SYSTEM_RESCAN_AUDIO:
			rescanHardwareDevices();
			break;

		case ECA_SYSTEM_RESCAN_MOUNTS:
			rescanLocalDrives();
			break;

		case ECA_SYSTEM_RESCAN_PORTS:
			rescanSerialPorts(true);
			break;

		case ECA_SYSTEM_REFRESH_DEVICE:
			trySendRemoteStatusRequest();
			break;

		case ECA_SYSTEM_LOAD_LOGFILES:
			// Has been moved to TAuxiliary class...
			// updateLoggerView();
			break;
			
		case ECA_SYSTEM_EXEC_DEFRAG:
			application.deallocateHeapMemory();
			break;

		case ECA_REMOTE_48K:
			setRemoteDevice(music::SR48K);
			break;

		case ECA_REMOTE_44K:
			setRemoteDevice(music::SR44K);
			break;

		case ECA_REMOTE_INPUT0:
		case ECA_REMOTE_INPUT1:
		case ECA_REMOTE_INPUT2:
		case ECA_REMOTE_INPUT3:
		case ECA_REMOTE_INPUT4:
		case ECA_REMOTE_FILTER0:
		case ECA_REMOTE_FILTER1:
		case ECA_REMOTE_FILTER2:
		case ECA_REMOTE_FILTER3:
		case ECA_REMOTE_FILTER4:
		case ECA_REMOTE_FILTER5:
		case ECA_REMOTE_PHASE0:
		case ECA_REMOTE_PHASE1:
			executeRemoteAction(action);
			util::wait(150);
			break;

		case ECA_APP_UPDATE:
			// Unused!
			break;

		case ECA_APP_EXIT:
			terminate = true;
			break;

		default:
			r = false;
			break;
	}
	return r;
}

bool TPlayer::executeRepeatModeAction(const app::ECommandAction action) {
	// Get current mode
	TPlayerMode mode, last, edge;
	getCurrentMode(last);
	bool success = true;

	// Set web component properties
	{
		app::TLockGuard<app::TMutex> lock(componentMtx);
		mode = last;
		switch (action) {
			case ECA_PLAYER_MODE_RANDOM:
				mode.random = toggleRepeatMode(btnRandom);
				edge.random = last.random != mode.random;
				break;

			case ECA_PLAYER_MODE_REPEAT:
				mode.repeat = toggleRepeatMode(btnRepeat);
				edge.repeat = last.repeat != mode.repeat;
				break;

			case ECA_PLAYER_MODE_SINGLE:
				mode.single = toggleRepeatMode(btnSingle);
				edge.single = last.single != mode.single;
				break;

			case ECA_PLAYER_MODE_DIRECT:
				mode.direct = toggleRepeatMode(btnDirect, html::ECT_WARNING);
				edge.direct = last.direct != mode.direct;
				break;

			case ECA_PLAYER_MODE_HALT:
				mode.halt = toggleRepeatMode(btnHalt, html::ECT_WARNING);
				edge.halt = last.halt != mode.halt;
				break;

			case ECA_PLAYER_MODE_DISK:
				mode.disk = toggleRepeatMode(btnDisk);
				edge.disk = last.disk != mode.disk;
				break;

			default:
				success = false;
				break;
		}
		if (success) {
			
			// Single mode excludes disk, repeat and random mode
			if (mode.single && edge.single) {
				mode.halt = false;
				mode.random = false;
				mode.repeat = false;
				mode.disk = false;
				setRepeatMode(btnRandom, mode.random);
				setRepeatMode(btnRepeat, mode.repeat);
				setRepeatMode(btnDisk, mode.disk);
			}

			// Stopping after current song excludes single mode
			if (mode.halt && edge.halt) {
				mode.single = false;
				setRepeatMode(btnSingle, mode.single);
			}

			// Random mode excludes single and repeat mode
			if (mode.random && edge.random) {
				mode.halt = false;
				mode.single = false;
				setRepeatMode(btnHalt, mode.halt, html::ECT_WARNING);
				setRepeatMode(btnSingle, mode.single);
			}

			// Disk mode excludes single and repeat mode
			if (mode.disk && edge.disk) {
				mode.halt = false;
				mode.single = false;
				setRepeatMode(btnSingle, mode.single);
				setRepeatMode(btnRepeat, mode.repeat);
			}

			// Repeat mode excludes disk and single mode
			if (mode.repeat && edge.repeat) {
				mode.halt = false;
				mode.single = false;
				setRepeatMode(btnSingle, mode.single);
				setRepeatMode(btnDisk, mode.disk);
			}
			
			// Enable or disable shuffle next song button on random mode(s) changed
			if (edge.random || edge.single) {
				btnShuffle.setEnabled(mode.random);
				btnShuffle.update();
			}
		}
	}

	// Check if random mode was disabled
	if (success && !mode.random && edge.random) {
		// Clear random song markers
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
		std::string playlist = player.getCurrentPlaylist();
		music::PPlaylist pls = playlists[playlist];
		if (util::assigned(pls)) {
			size_t c = pls->clearRandomMarkers();
			logger(util::csnprintf("[Random] Cleared random markers (% songs)", c));
		} else {
			if (playlist.empty())
				logger("[Random] Set random markers failed, no playlist given");
			else
				logger(util::csnprintf("[Random] Set random markers failed, invalid playlist $ given", playlist));
		}
	}

	// Set new mode and broadcast mode change to console clients
	if (success) {
		setCurrentMode(mode);
		sendBroadcastMessage("MODE|" + modeToStr(mode, '|'));
		logger(util::csnprintf("[Random] Current shuffle mode is [%]", modeToStr(mode)));
	}

	return success;
}

bool TPlayer::toggleRepeatMode(html::TButton& button, const html::EComponentType type) {
	bool r = false;
	if (html::ECT_DEFAULT == button.getType()) {
		button.setType(type);
		r = true;
	} else {
		button.setType(html::ECT_DEFAULT);
	}
	button.update();
	return r;
}

void TPlayer::setRepeatMode(html::TButton& button, const bool value, const html::EComponentType type) {
	if (value) {
		button.setType(type);
	} else {
		button.setType(html::ECT_DEFAULT);
	}
	button.update();
}

void TPlayer::setCurrentMode(const TPlayerMode& mode) {
	std::string value;
	{
		app::TLockGuard<app::TMutex> lock(modeMtx);
		value = mode.direct ? "true" : "false";
		this->mode = mode;
	}
	if (!value.empty() && application.hasWebServer()) {
		application.getWebServer().setApplicationValue("player-direct-mode", value);
	}
}

void TPlayer::getCurrentMode(TPlayerMode& mode) const {
	app::TLockGuard<app::TMutex> lock(modeMtx);
	mode = this->mode;
}

std::string TPlayer::modeToStr(TPlayerMode& mode, const char separator) const {
	// Log human readable option list
	std::string modes;
	if (mode.halt) {
		if (!modes.empty())
			modes += separator;
		modes += "Halt";
	}
	if (mode.single) {
		if (!modes.empty())
			modes += separator;
		modes += "Single";
	}
	if (mode.disk) {
		if (!modes.empty())
			modes += separator;
		modes += "Disk";
	}
	if (modes.empty()) {
		if (mode.random || mode.repeat) {
			modes += "Playlist";
		}
	}
	if (mode.repeat) {
		if (!modes.empty())
			modes += separator;
		modes += "Repeat";
	}
	if (mode.random) {
		if (!modes.empty())
			modes += separator;
		modes += "Random";
	}
	if (modes.empty()) {
		modes = "none";
	}
	return modes;
}


void TPlayer::queueCommand(const TCommand& command) {
	if (queue.add(command)) {
		notifyEvent("[Queue] Queue command");
	}
}


void TPlayer::addCommand(const ECommandAction action) {
	TCommand command;
	command.action = action;
	music::CSongData song;
	std::string playlist;
	getPlayedSong(song, playlist);
	if (!song.valid) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		if (!playlists.recent()->empty()) {
			music::PSong o = playlists.recent()->getSong(0);
			if (util::assigned(o)) {
				playlist = playlists.recent()->getName();
				song.fileHash = o->getFileHash();
				song.albumHash = o->getAlbumHash();
				song.valid = true;
			}
		}
	}
	if (song.valid) {
		command.file = song.fileHash;
		command.album = song.albumHash;
		command.playlist = playlist;
	}
	queueCommand(command);
}

#define TMP_REMOTE_FILE "/tmp/.remote"

void TPlayer::executeFileCommand() {
	util::moveFile(sound.getControlFile(), TMP_REMOTE_FILE);
	if (util::fileExists(TMP_REMOTE_FILE)) {
		util::TStringList list;
		list.loadFromFile(TMP_REMOTE_FILE);
		if (!list.empty()) {
			std::string command = util::toupper(util::trim(list[0]));
			logger("[File] Execute control command \"" + command + "\"");
			addRemoteCommand(command);
		}
		util::deleteFile(TMP_REMOTE_FILE);
	}
}

#undef TMP_REMOTE_FILE


ECommandAction TPlayer::getRemoteCommand(const std::string& command) {
	if (!command.empty()) {
		ECommandAction action = actions[command];
		return action;
	}
	return ECA_INVALID_ACTION;
}

ECommandAction TPlayer::getControlCommand(const std::string& command) {
	if (!command.empty()) {
		ECommandAction action = controls[command];
		return action;
	}
	return ECA_INVALID_ACTION;
}

ECommandAction TPlayer::addRemoteCommand(const std::string& command) {
	if (!command.empty()) {
		ECommandAction action = actions[command];
		executeControlAction(action);
		return action;
	}
	return ECA_INVALID_ACTION;
}

ECommandAction TPlayer::addControlCommand(const std::string& command) {
	if (!command.empty()) {
		ECommandAction action = controls[command];
		executeControlAction(action);
		return action;
	}
	return ECA_INVALID_ACTION;
}


void TPlayer::executeControlAction(const ECommandAction action) {
	if (ECA_INVALID_ACTION != action) {
		bool ok = false;

		if (!ok && util::isMemberOf(action, ECA_PLAYER_STOP, ECA_PLAYER_PLAY, ECA_PLAYER_PAUSE, \
				ECA_PLAYER_NEXT, ECA_PLAYER_PREV, ECA_PLAYER_FF, ECA_PLAYER_FR, ECA_PLAYER_RAND, \
				ECA_OPTION_RED, ECA_OPTION_GREEN, ECA_OPTION_YELLOW, ECA_OPTION_BLUE)) {
			ok = true;
			addCommand(action);
		}

		// Switch remote input
		if (!ok &&util::isMemberOf(action, ECA_REMOTE_INPUT0, ECA_REMOTE_INPUT1, ECA_REMOTE_INPUT2, ECA_REMOTE_INPUT3, ECA_REMOTE_INPUT4)) {
			ok = true;

			// Read configuration values from parameter list
			music::CAudioValues values;
			sound.getAudioValues(values);

			// Execute remote command
			if (values.control) {
				executeRemoteAction(values, action);
			}
		}

		// Switch remote sample frequency
		if (!ok &&util::isMemberOf(action, ECA_REMOTE_44K, ECA_REMOTE_48K)) {
			ok = true;

			// Get audio hardware settings
			music::CAudioValues values;
			sound.getAudioValues(values);

			// Set remote word clock output device
			if (values.enabled) {
				music::ESampleRate rate = music::SR44K;
				if (ECA_REMOTE_48K == action)
					rate = music::SR48K;
				setRemoteDevice(values, rate);
			}
		}
	}
}


void TPlayer::setHistoryDefaultCounts(const size_t max) {
	// Update HTML combo box entries
	music::CConfigValues values;
	sound.getConfiguredValues(values);
	std::string limit = std::to_string((size_u)values.displayLimit);

	// Rebuild history combo box entries
	app::TLockGuard<app::TMutex> lock(historyMtx);
	lbxHistory.elements().clear();

	// Add default entries
	size_t i = 36;
	do {
		lbxHistory.elements().add(std::to_string((size_u)i));
		i *= 2;
	} while (i <= max);

	lbxHistory.update(limit);
}

void TPlayer::setPageDefaultCounts() {
	// Update HTML combo box entries
	music::CConfigValues values;
	sound.getConfiguredValues(values);
	std::string rows = std::to_string((size_u)values.pageLimit);
	if (values.pageLimit > 1000)
		rows = "All";

	// Set web token value
	if (util::assigned(wtTableRowCount))
		wtTableRowCount->setValue(rows);

	// Rebuild history combo box entries
	app::TLockGuard<app::TMutex> lock(pagesMtx);
	lbxPages.elements().clear();

	// Add default entries
	lbxPages.elements().add("10");
	lbxPages.elements().add("25");
	lbxPages.elements().add("50");
	lbxPages.elements().add("100");
	lbxPages.elements().add("All");

	lbxPages.update(rows);
}

void TPlayer::setPeriodDefaultTimes() {
	// Update HTML combo box entries
	music::CConfigValues values;
	sound.getConfiguredValues(values);
	std::string period = std::to_string((size_u)values.periodTime);

	// Rebuild history combo box entries
	app::TLockGuard<app::TMutex> lock(timesMtx);
	lbxTimes.elements().clear();

	// Add default entries
	lbxTimes.elements().add("100");
	lbxTimes.elements().add("250");
	lbxTimes.elements().add("500");
	lbxTimes.elements().add("1000");
	lbxTimes.elements().add("2000");

	lbxTimes.update(period);
}

void TPlayer::rescanSerialPortsWithNolock(music::CAudioValues& values, const bool nolock) {

	// Rescan serial hardware devices
	if (debug) aout << "Devices: Rescan serial hardware devices." << std::endl;
	util::TStringList devices;

	// Store devices in string list of combo box
	if (scanTerminalDevices(devices, true, nolock)) {
		lbxSerial.elements() = devices;
		if (debug) {
			if (debug) aout << "Supported serial devices:" << endl;
			lbxSerial.elements().debugOutput();
			if (debug) aout << endl;
		} else {
			logger("[Hardware] [Ports] Known serial devices:");
			for (size_t i=0; i<devices.size(); ++i) {
				logger("[Hardware] >> " + devices[i]);
			}
		}
	} else {
		lbxSerial.elements().clear();
		lbxSerial.elements().add(">> No serial devices found <<");
		logger("[Hardware] [Ports] No serial devices found.");
	}

	lbxSerial.update(values.terminal);
	portsValid = true;
}

void TPlayer::rescanHardwareDevices() {
	// Rescan sound hardware devices
	music::CAudioValues values;
	sound.getAudioValues(values);
	app::TLockGuard<app::TMutex> lock(deviceMtx, false);
	if (lock.tryLock()) {
		rescanHardwareDevicesWithNolock(values);
	}
}

void TPlayer::rescanLocalDrives() {
	// Rescan local media mounts
	music::CConfigValues values;
	sound.getConfiguredValues(values);
	app::TLockGuard<app::TMutex> lock(driveMtx, false);
	if (lock.tryLock()) {
		rescanLocalDrivesWithNolock(values);
	}
}

void TPlayer::rescanSerialPorts(const bool nolock) {
	// Rescan serial hardware devices
	music::CAudioValues values;
	sound.getAudioValues(values);
	app::TLockGuard<app::TMutex> lock(serialMtx, false);
	if (lock.tryLock()) {
		rescanSerialPortsWithNolock(values, nolock);
	}
}

void TPlayer::updateHardwareDevices() {
	// Rescan sound hardware devices
	music::CAudioValues values;
	sound.getAudioValues(values);
	app::TLockGuard<app::TMutex> lock(deviceMtx);
	if (!devicesValid) {
		rescanHardwareDevicesWithNolock(values);
	}
}

void TPlayer::updateLocalDrives() {
	// Rescan local media mounts
	music::CConfigValues values;
	sound.getConfiguredValues(values);
	app::TLockGuard<app::TMutex> lock(driveMtx);
	if (!drivesValid) {
		rescanLocalDrivesWithNolock(values);
	}
}

void TPlayer::updateSerialPorts() {
	// Rescan serial ports
	music::CAudioValues values;
	sound.getAudioValues(values);
	app::TLockGuard<app::TMutex> lock(serialMtx);
	if (!portsValid) {
		rescanSerialPortsWithNolock(values, true);
	}
}

void TPlayer::invalidateHardwareDevices() {
	app::TLockGuard<app::TMutex> lock(deviceMtx);
	devicesValid = false;
}

void TPlayer::invalidateLocalDrives() {
	app::TLockGuard<app::TMutex> lock(driveMtx);
	drivesValid = false;
}

void TPlayer::invalidateSerialPorts() {
	app::TLockGuard<app::TMutex> lock(serialMtx);
	portsValid = false;
}

void TPlayer::rescanHardwareDevicesWithNolock(const music::CAudioValues& values) {
	// Rescan sound hardware devices
	if (debug) aout << "Devices: Rescan sound hardware devices." << std::endl;
	util::TStringList ignore(sound.getIgnoreDevices(), ';');
	util::TStringList devices;

	// Store devices in string list of combo box
	if (music::TAlsaPlayer::getAlsaHardwareDevices(devices, ignore) > 0) {
		cbxDevices.elements() = devices;
		lbxDevices.elements() = devices;
		lbxRemote.elements() = devices;
		if (debug) {
			if (debug) aout << "Supported ALSA devices:" << endl;
			cbxDevices.elements().debugOutput();
			if (debug) aout << endl;
		} else {
			logger("[Hardware] [Alsa] Known audio devices:");
			for (size_t i=0; i<devices.size(); ++i) {
				logger("[Hardware] >> " + devices[i]);
			}
		}

	} else {
		cbxDevices.elements().clear();
		lbxDevices.elements().clear();
		lbxRemote.elements().clear();
		cbxDevices.elements().add(">> No audio devices found <<");
		lbxDevices.elements().add(">> No audio devices found <<");
		lbxRemote.elements().add(">> No audio devices found <<");
		logger("[Hardware] [Alsa] No ALSA devices found.");
	}

	// Update HTML combo box entries
	// --> Caption is current device
	cbxDevices.update(values.device);
	lbxDevices.update(values.device);
	lbxRemote.update(values.remote);
	devicesValid = true;
}

void TPlayer::rescanLocalDrivesWithNolock(const music::CConfigValues& values) {
	// Rescan local media mounts
	if (debug) aout << "Drives: Rescan local mounts." << std::endl;
	util::TStringList drives;

	if (sound.scanStorageMounts(drives) > 0) {
		cbxDrives.elements() = drives;
		lbxDrives1.elements() = drives;
		lbxDrives2.elements() = drives;
		lbxDrives3.elements() = drives;
		if (debug) {
			aout << "Possible storage mounts:" << std::endl;
			cbxDrives.elements().debugOutput();
			aout << endl;
		} else {
			logger("[Hardware] [Drives] Known media mounts:");
			for (size_t i=0; i<drives.size(); ++i) {
				logger("[Hardware] >> " + drives[i]);
			}
		}

	} else {
		cbxDrives.elements().clear();
		lbxDrives1.elements().clear();
		lbxDrives2.elements().clear();
		lbxDrives3.elements().clear();
		cbxDrives.elements().add(">> No mountpoints found <<");
		lbxDrives1.elements().add(">> No mountpoints found <<");
		lbxDrives2.elements().add(">> No mountpoints found <<");
		lbxDrives3.elements().add(">> No mountpoints found <<");
		logger("[Hardware] [Drives] No suitable local mountpoints found.");
	}

	// Update HTML combo box entries
	// --> Caption is current device
	cbxDrives.update(values.musicpath1);
	lbxDrives1.update(values.musicpath1);
	lbxDrives2.update(values.musicpath2);
	lbxDrives3.update(values.musicpath3);
	drivesValid = true;
}

void TPlayer::executeRemoteAction(const app::ECommandAction action) {
	music::CAudioValues values;
	sound.getAudioValues(values);
	executeRemoteAction(values, action);
}

void TPlayer::executeRemoteAction(const music::CAudioValues& values, const app::ECommandAction action) {
	TCommandData value = 0xFFu;
	ERemoteCommand command = RS_STATUS;
	switch (action) {

		// Input selector command
		case ECA_REMOTE_INPUT0:
			value = 6;
			command = RS_INPUT;
			break;
		case ECA_REMOTE_INPUT1:
			value = 2;
			command = RS_INPUT;
			break;
		case ECA_REMOTE_INPUT2:
			value = 3;
			command = RS_INPUT;
			break;
		case ECA_REMOTE_INPUT3:
			value = 0;
			command = RS_INPUT;
			break;
		case ECA_REMOTE_INPUT4:
			value = 1;
			command = RS_INPUT;
			break;

		// Filter selector command
		case ECA_REMOTE_FILTER0:
			value = 0;
			command = RS_FILTER;
			break;
		case ECA_REMOTE_FILTER1:
			value = 1;
			command = RS_FILTER;
			break;
		case ECA_REMOTE_FILTER2:
			value = 2;
			command = RS_FILTER;
			break;
		case ECA_REMOTE_FILTER3:
			value = 3;
			command = RS_FILTER;
			break;
		case ECA_REMOTE_FILTER4:
			value = 4;
			command = RS_FILTER;
			break;
		case ECA_REMOTE_FILTER5:
			value = 5;
			command = RS_FILTER;
			break;

		// Phase selector command
		case ECA_REMOTE_PHASE0:
			value = 0;
			command = RS_PHASE;
			break;
		case ECA_REMOTE_PHASE1:
			value = 1;
			command = RS_PHASE;
			break;

		default:
			break;
	}
	if (value != 0xFFu) {
		bool r = sendRemoteCommand(values, EDS_DAC, command, value, ERT_ACKNOWLEDE);
		if (r) {
			switch (action) {
				case ECA_REMOTE_INPUT0:
				case ECA_REMOTE_INPUT1:
				case ECA_REMOTE_INPUT2:
				case ECA_REMOTE_INPUT3:
				case ECA_REMOTE_INPUT4:
					setCurrentInput(action);
					updateSelectInputButtons(values, r ? action : ECA_INVALID_ACTION);
					break;
				case ECA_REMOTE_FILTER0:
				case ECA_REMOTE_FILTER1:
				case ECA_REMOTE_FILTER2:
				case ECA_REMOTE_FILTER3:
				case ECA_REMOTE_FILTER4:
				case ECA_REMOTE_FILTER5:
					setCurrentFilter(action);
					updateSelectFilterButtons(values, r ? action : ECA_INVALID_ACTION);
					break;
				case ECA_REMOTE_PHASE0:
				case ECA_REMOTE_PHASE1:
					setCurrentPhase(action);
					updateSelectPhaseButtons(values, r ? action : ECA_INVALID_ACTION);
					break;
				default:
					break;
			}
		}
	} else {
		logger(util::csnprintf("[Action] [Remote] Invalid command value <%>", value));
	}
}

bool TPlayer::sendRemoteStatusRequest(unsigned char page) {
	/*
	 * Description:	Returns the 5-byte “Page” specified by the	Payload
	 *
	 * 	Page 0:			Byte 0	Sample Rate – see Sample Rate Table
	 * 					Byte 1	0 = Not muted, 4 = Muted
	 * 					Byte 2	0 – fixed
	 * 					Byte 3	0 – fixed
	 * 					Byte 4	0 - fixed
	 *
	 * 	Page 1:			Byte 0	0 - fixed
	 * 					Byte 1	0 – fixed
	 * 					Byte 2	0 – fixed
	 * 					Byte 3	De-Emphasis Mode
	 * 					Byte 4	Unit ID – 14 = Vivaldi DAC – fixed (ask dCS for other models)
	 *
	 * 	Page 2:			Byte 0	255 - fixed
	 * 					Byte 1	0 – fixed
	 * 					Byte 2	Volume setting in –dB (Vivaldi only), 0 = 0dB, 80 = -80dB
	 * 					Byte 3	Balance setting (-61 to 61) in 0.1dB steps (Vivaldi only)
	 * 							194 = balance to left, 0 = balance central, 61 = balance to right
	 * 					Byte 4	0 – fixed
	 *
	 * 	Page 3:			Byte 0	0 = Phase Normal, 1 = Phase Inverted Byte 1 Currently Selected Input: 0 RCA1 3 Toslink 7 SDIF-2
	 * 					Byte 1	Scarlatti	0 RCA1		3 Toslink	7 SDIF-2
	 * 										1 BNC		4 AES2		8 1394
	 * 										2 AES1		6 AES1+2	9 RCA2
	 * 							Paganini	0 RCA1		3 AES2		8 1394
	 * 										2 AES1		4 AES1+2	9 RCA2
	 * 							Debussy		0 SPDIF1	2 AES1		4 AES1+2
	 * 										1 SPDIF2	3 AES2		6 USB
	 * 							Vivaldi		0 SPDIF1	4 AES1+2	9 SPDIF2
	 * 										1 SPDIF3	5 USB		10 AES3
	 * 										2 AES1		6 TOSLINK	11 AES4
	 * 										3 AES2		7 SDIF-2	12 AES3+4
	 * 					Byte 2	Physical Lock Frequency – see Sample Rate Table
	 * 					Byte 3	32 – fixed
	 * 					Byte 4	Filter: 0 = Filter1, 1 = Filter2, 2 = Filter3, 3 = Filter4, 4 = Filter5, 5 = Filter6
	 *
	 * 	Sample Rate		Payload 0 	1     4     5   6   9      10   20     21   22  255
	 * 					Rate 	96k 88.2k 44.1k 48k 32k 176.4k 192k 352.8k 384k DSD Unlocked/Unknown
	 */
	music::CAudioValues values;
	sound.getAudioValues(values);
	return sendRemoteCommand(values, EDS_DAC, RS_STATUS, page, ERT_DATA);
}

bool TPlayer::trySendRemoteStatusRequest() {
	if (!terminal.isBusy()) {
		return sendRemoteStatusRequest();
	}
	return false;
}

bool TPlayer::sendRemoteCommand(const music::CAudioValues& values, const EDeviceType id, const ERemoteCommand command, const TCommandData value, const EResponseType type) {
	app::TLockGuard<app::TMutex> lock(serialMtx);
	if (values.control) {

		// Close device on hardware setting changed
		if (terminal.isOpen() && values.terminal != terminal.getDevice()) {
			if (!terminal.reopen(values.terminal, REMOTE_BAUD_RATE)) {
				logger(util::csnprintf("[Remote] [Command] Reopen device $ failed $", values.terminal, terminal.strerr()));
				return false;
			}
		}

		// Open device
		if (!terminal.isOpen()) {
			if (!terminal.open(values.terminal, REMOTE_BAUD_RATE)) {
				logger(util::csnprintf("[Remote] [Command] Open device $ failed $", values.terminal, terminal.strerr()));
				return false;
			}
			if (util::assigned(remoteCommandThread)) {
				if (!remoteCommandThread->isStarted()) {
					remoteCommandThread->execute();
				}
			}
		}

		// Send remote command
		if (terminal.isOpen()) {
			if (terminal.send(id, command, value, type)) {
				// Command successfully send
				logger(util::csnprintf("[Remote] [Command] Send command <@> for device $", command, values.terminal));
				toRemoteCommand->restart(DEFAULT_REMOTE_TIMEOUT);
				return true;
			} else {
				logger(util::csnprintf("[Remote] [Command] Send command <@> for device $ failed $", command, values.terminal, terminal.strerr()));
				return false;
			}
		}

		// Unexpected state...
		std::string error = terminal.strerr();
		if (error.empty())
			logger(util::csnprintf("[Remote] [Command] Device $ not open to send command <@> (2)", values.terminal, command));
		else
			logger(util::csnprintf("[Remote] [Command] Failed to send command <@> for Device $ : $", command, values.terminal, error));

	} else {
		if (terminal.isOpen())
			terminal.inhibit();
		logger("[Remote] [Command] Serial control port disabled, command ignored.");
	}
	return false;
}

void TPlayer::closeTerminalDevice() {
	app::TLockGuard<app::TMutex> lock(serialMtx);
	if (terminal.isOpen())
		terminal.close();
}


void TPlayer::setCurrentInput(const app::ECommandAction action) {
	app::TLockGuard<app::TMutex> lock(commandMtx);
	if (util::isMemberOf(action, ECA_REMOTE_INPUT0, ECA_REMOTE_INPUT1, ECA_REMOTE_INPUT2, ECA_REMOTE_INPUT3, ECA_REMOTE_INPUT4))
		currentInput = action;
	else
		currentInput = ECA_INVALID_ACTION;
}

app::ECommandAction TPlayer::getCurrentInput() const {
	app::TLockGuard<app::TMutex> lock(commandMtx);
	return currentInput;
}

void TPlayer::setCurrentFilter(const app::ECommandAction action) {
	app::TLockGuard<app::TMutex> lock(commandMtx);
	if (util::isMemberOf(action, ECA_REMOTE_FILTER0, ECA_REMOTE_FILTER1, ECA_REMOTE_FILTER2, ECA_REMOTE_FILTER3, ECA_REMOTE_FILTER4, ECA_REMOTE_FILTER5))
		currentFilter = action;
	else
		currentFilter = ECA_INVALID_ACTION;
}

app::ECommandAction TPlayer::getCurrentFilter() const {
	app::TLockGuard<app::TMutex> lock(commandMtx);
	return currentFilter;
}

void TPlayer::setCurrentPhase(const app::ECommandAction action) {
	app::TLockGuard<app::TMutex> lock(commandMtx);
	if (util::isMemberOf(action, ECA_REMOTE_PHASE0, ECA_REMOTE_PHASE1))
		currentPhase = action;
	else
		currentPhase = ECA_INVALID_ACTION;
}

app::ECommandAction TPlayer::getCurrentPhase() const {
	app::TLockGuard<app::TMutex> lock(commandMtx);
	return currentPhase;
}

void TPlayer::updateLibrary(const bool aggressive) {
	bool ok = false;

	// Signal library scanning action to web display
	invalidateScannerDislpay();

	// Get all needed parameters
	util::TStringList folders;
	sound.getMusicFolders(folders);

	music::CConfigValues values;
	sound.getConfiguredValues(values);

	// Rescan music folder
	app::TLockGuard<app::TMutex> uplock(updateMtx, false);
	if (uplock.tryLock()) {
		if (!folders.empty()) {
			std::string mode = aggressive ? "Rebuild" : "Rescan";
			logger("[Scanner] " + mode + " library.");

			// Set scanner status display and reset erroneous items display
			setScannerStatusLabel(true, 0, 0);
			setErroneousHeader(0);

			try {

				// Save and delete playlist on aggressive scan
				app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);

				// Save all playlists (including recent songs)
				size_t saved = playlists.commit(true);
				logger("[Scanner] Saved " + std::to_string((size_s)saved) + " playlists.");

				// Clear library and all playlist item entries
				// Set empty display values during aggressive scan...
				if (aggressive) {
					logger("[Scanner] Delete library on aggressive scan.");
					library.clear();
					playlists.clear();
					updateLibraryStatusWithNolock();
					updateLibraryMenuItemsWithNolock();
				}

				// Rescan library with current configuration values
				library.configure(values);

				// Prepare update database
				logger("[Scanner] Prepare library update.");
				library.prepare();

				// Start rescan on first folder entry, update all other folder content
				for (size_t i=0; i<folders.size(); i++) {
					std::string contentFolder = folders[i];
					util::validPath(contentFolder);
					logger("[Scanner] Rescan <" + contentFolder + ">");
					library.update(contentFolder, values.pattern, aggressive);
					logger("[Scanner] Finished rescan for <" + contentFolder + ">");
				}

				// Apply changes to internal database mappings
				logger("[Scanner] Commit library changes.");
				library.commit();

				// Rebuild or reload all playlists compared to current library content
				if (aggressive) {
					playlists.reload();
				} else {
					playlists.rebuild();
					playlists.commit(false);
				}

				// Save changed database file if needed
				if (library.isChanged() || library.hasErrors()) {
					library.saveToFile(values.datafile);
					if (library.isChanged()) logger("[Scanner] Library was changed.");
					if (library.hasErrors()) logger("[Scanner] Library has " + std::to_string((size_s)library.erroneous()) + " erroneous items.");
				}

				// Limit playlist entries for each list to maximum size
				playlists.resize(sound.getPlaylistSize());

				// Force display of library file watch after successful rescan
				contentChanged = false;
				contentCompare = false;

			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				logger("[Scanner] Exception <" + sExcept + ">");
			} catch (...) {
				logger("[Scanner] Unhandled exception.");
			}

		} else {
			logger("[Scanner] No content folders configured.");
		}

		// Content was scanned...
		ok = true;
	}

	if (ok) {
		// Signal scanner finished
		sysutil::TBeeperParams beeper;
		beeper.repeats = 1;
		beeper.length = DEFAULT_BEEPER_LENGTH * 2;
		sysutil::beep(beeper);

		// Update display values
		size_t songs, errors;
		{
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
			songs = library.songs();
			errors = library.erroneous();
			updateLibraryStatusWithNolock();
			updateLibraryMenuItemsWithNolock();
		}
		setScannerStatusLabel(false, songs, errors);
		setErroneousHeader(errors);
		invalidateScannerDislpay();
	}
}


void TPlayer::rescanLibrary() {
	app::TLockGuard<app::TMutex> lock(rescanMtx, false);
	if (!lock.tryLock()) {
		logger("[Action] [Rescan] Scanning in progress, action ignored.");
		return;
	}

	// Rescan library
	updateLibrary(false);
}

void TPlayer::rebuildLibrary() {
	app::TLockGuard<app::TMutex> lock(rescanMtx, false);
	if (!lock.tryLock()) {
		logger("[Action] [Rebuild] Scanning in progress, action ignored.");
		return;
	}

	// Stop ALSA playback
	bool r = player.stop();
	if (r) {
		logger("[Action] [Rebuild] Device <" + util::strToStr(player.getCurrentDevice(), "none") + "> stopped.");

		// Reset current values
		clearCurrentSong();
		clearSelectedSong();
		clearLastSong();

		// Reset all songs in ALSA buffers
		player.resetBuffers();
		cleared = true;

		// Rebuild library
		updateLibrary(true);

	} else {
		logger("[Action] [Rebuild] Failed to stop ALSA device <" + util::strToStr(player.getCurrentDevice(), "none") + ">");
		logger("[Action] [Rebuild] Message : " + player.strerr());
		logger("[Action] [Rebuild] Error   : " + player.syserr());
	}
}

void TPlayer::deleteLibrary() {
	app::TLockGuard<app::TMutex> lock(rescanMtx, false);
	if (!lock.tryLock()) {
		logger("[Action] [Delete] Scanning in progress, action ignored.");
		return;
	}

	// Stop ALSA playback
	bool r = player.stop();
	if (r) {
		logger("[Action] [Delete] Device <" + util::strToStr(player.getCurrentDevice(), "none") + "> stopped.");

		// Reset current values
		clearCurrentSong();
		clearSelectedSong();
		clearLastSong();

		// Reset all songs in ALSA buffers
		player.resetBuffers();
		cleared = true;

		// Delete library content
		size_t songs, errors;
		{
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
			logger("[Library] [Delete] Delete complete music library.");
			playlists.clear();
			library.clear();
			library.saveToFile();
			songs = library.songs();
			errors = library.erroneous();
			updateLibraryStatusWithNolock();
			updateLibraryMenuItemsWithNolock();
		}

		// Update display variables
		setScannerStatusLabel(false, songs, errors);
		setErroneousHeader(errors);
		invalidateScannerDislpay();

	} else {
		logger("[Action] [Delete] Failed to stop ALSA device <" + util::strToStr(player.getCurrentDevice(), "none") + ">");
		logger("[Action] [Delete] Message : " + player.strerr());
		logger("[Action] [Delete] Error   : " + player.syserr());
	}
}

int TPlayer::libraryThreadHandler(TLibraryThread& sender, music::TLibrary& library) {
	sender.writeLog("Thread started.");
	libraryThreadActive = libraryThreadRunning = true;
	util::TBooleanGuard<bool> guard(libraryThreadActive);

	// Thread loop...
	do {
		libraryEvent.wait();
		if (libraryThreadRunning) {
			switch (getLibraryAction()) {
				case ECA_LIBRARY_RESCAN_LIBRARY:
					rescanLibrary();
					break;
				case ECA_LIBRARY_REBUILD_LIBRARY:
					rebuildLibrary();
					break;
				case ECA_LIBRARY_CLEAR_LIBRARY:
					deleteLibrary();
					break;
				case ECA_APP_EXIT:
					libraryThreadRunning = false;
					break;
				default:
					break;
			}
			clearLibraryAction();
		}
	} while (libraryThreadRunning && !terminate && !sender.isTerminating());

	sender.writeLog("Thread terminated.");
	return EXIT_SUCCESS;
}




void TPlayer::saveCurrentSong(const music::TSong* song, const std::string& playlist) const {
	application.writeLocalStore(STORE_CURRENT_SONG_HASH, util::assigned(song) ? song->getFileHash() : "none");
	application.writeLocalStore(STORE_CURRENT_SONG_NAME, util::assigned(song) ? song->getFileName() : "noname");
	application.writeLocalStore(STORE_CURRENT_PLAYLIST, playlist.empty() ? "nolist" : playlist);
}

void TPlayer::loadCurrentSong(music::TSong*& song, std::string& playlist) {
	playlist.clear();
	song = nil;

	util::TVariant value;
	application.readLocalStore(STORE_CURRENT_SONG_HASH, value);
	std::string hash = value.asString();
	application.readLocalStore(STORE_CURRENT_PLAYLIST, value);
	std::string current = value.asString();

	music::PPlaylist pls = nil;
	if (!current.empty()) {
		pls = playlists[current];
	}
	if (util::assigned(pls)) {
		playlist = current;
	} else {
		// Set recent songs as current playlist
		pls = playlists.recent();
		playlist = playlists.recent()->getName();
	}
	if (util::assigned(pls)) {
		song = pls->findSong(hash);
	}
}

void TPlayer::saveSelectedPlaylist(const std::string& playlist) const {
	application.writeLocalStore(STORE_SELECTED_PLAYLIST, playlist.empty() ? "nolist" : playlist);
}

void TPlayer::loadSelectedPlaylist(std::string& playlist) {
	playlist.clear();

	util::TVariant value;
	application.readLocalStore(STORE_SELECTED_PLAYLIST, value);
	std::string selected = value.asString();

	music::PPlaylist pls = nil;
	if (!selected.empty()) {
		pls = playlists[selected];
	}
	if (util::assigned(pls)) {
		playlist = selected;
	}
}


music::EPlayListAction TPlayer::sanitizePlayerAction(music::EPlayListAction action, const std::string& playlist) {
	if (action != music::EPA_INSERT && playlists.isRecent(playlist))
		action = music::EPA_INSERT;
	return action;
}


void TPlayer::addTrack(const std::string& fileHash, const std::string& playlist, const music::EPlayListAction action) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	music::PPlaylist pls = playlists[playlist];
	if (util::assigned(pls)) {
		music::PSong found = library.findFile(fileHash);
		if (util::assigned(found)) {
			std::string albumHash = found->getAlbumHash();
			music::PSong song = pls->findSong(fileHash);
			if (!util::assigned(song)) {
				pls->deleteAlbum(albumHash);
				int c = pls->addAlbum(albumHash, sanitizePlayerAction(action, playlist), true);
				if (c > 0) {
					logger(util::csnprintf("[Playlist] [Add track] % songs added to playlist $ for album $", c, playlist, albumHash));
				} else {
					logger(util::csnprintf("[Playlist] [Add track] No songs added to playlist $ for album $", playlist, albumHash));
				}
			} else {
				logger(util::csnprintf("[Playlist] [Add track] Song $ is in playlist, nothing to do...", fileHash));
			}
		} else {
			logger(util::csnprintf("[Playlist] [Add track] No album found in library for file $", fileHash));
		}
	} else {
		if (playlist.empty())
			logger(util::csnprintf("[Playlist] [Add track] Empty playlist to add tracks for file $", fileHash));
		else
			logger(util::csnprintf("[Playlist] [Add track] Invalid playlist $ to add tracks for file $", playlist, fileHash));
	}
}

void TPlayer::addTracks(const std::string& albumHash, const std::string& playlist, const music::EPlayListAction action) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	music::PPlaylist pls = playlists[playlist];
	if (util::assigned(pls)) {
		music::PSong found = library.findAlbum(albumHash);
		if (util::assigned(found)) {
			music::PSong song = pls->findAlbum(albumHash);
			if (!util::assigned(song)) {
				pls->deleteAlbum(albumHash);
				int c = pls->addAlbum(albumHash, sanitizePlayerAction(action, playlist), true);
				if (c > 0) {
					logger(util::csnprintf("[Playlist] [Add tracks] % songs added to playlist $ for album $", c, playlist, albumHash));
				} else {
					logger(util::csnprintf("[Playlist] [Add tracks] No songs added to playlist $ for album $", playlist, albumHash));
				}
			} else {
				logger(util::csnprintf("[Playlist] [Add tracks] Album $ is in playlist, nothing to do...", albumHash));
			}
		} else {
			logger(util::csnprintf("[Playlist] [Add tracks] No album found in library for file $", albumHash));
		}
	} else {
		if (playlist.empty())
			logger(util::csnprintf("[Playlist] [Add tracks] Empty playlist to add album $", albumHash));
		else
			logger(util::csnprintf("[Playlist] [Add tracks] Invalid playlist $ to add album $", playlist, albumHash));
	}
}


void TPlayer::addSong(const std::string& fileHash, const std::string& albumHash, const std::string& playlist, const music::EPlayListAction action) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	music::PPlaylist pls = playlists[playlist];
	if (util::assigned(pls)) {
		music::PSong found = pls->findSong(fileHash);
		if (!util::assigned(found)) {
			unlinkSong(fileHash, albumHash, playlist);
			music::PSong song = pls->addTrack(fileHash, sanitizePlayerAction(action, playlist), true);
			if (util::assigned(song)) {
				logger(util::csnprintf("[Playlist] [Add song] Song $ added to playlist $", fileHash, playlist));
			} else {
				logger(util::csnprintf("[Playlist] [Add song] No song for $ added to playlist $", fileHash, playlist));
			}
		} else {
			logger(util::csnprintf("[Playlist] [Add song] Song $ was in playlist $ before.", fileHash, playlist));
		}
	} else {
		if (playlist.empty())
			logger(util::csnprintf("[Playlist] [Add song] Empty playlist to add song $", fileHash));
		else
			logger(util::csnprintf("[Playlist] [Add song] Invalid playlist $ to add song $", playlist, fileHash));
	}
}


void TPlayer::addAlbum(const std::string& albumHash, const std::string& playlist, const music::EPlayListAction action) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	addAlbumWithNolock(albumHash, playlist, action);
}

void TPlayer::addAlbumWithNolock(const std::string& albumHash, const std::string& playlist, const music::EPlayListAction action) {
	music::PPlaylist pls = playlists[playlist];
	if (util::assigned(pls)) {
		pls->deleteAlbum(albumHash);
		int c = pls->addAlbum(albumHash, sanitizePlayerAction(action, playlist), true);
		if (c > 0) {
			logger(util::csnprintf("[Playlist] [Add album] % songs added to playlist $ for album $", c, playlist, albumHash));
		} else {
			logger(util::csnprintf("[Playlist] [Add album] No songs added to playlist $ for album $", playlist, albumHash));
		}
	} else {
		if (playlist.empty())
			logger(util::csnprintf("[Playlist] [Add album] Empty playlist to add album $", albumHash));
		else
			logger(util::csnprintf("[Playlist] [Add album] Invalid playlist $ to add album $", playlist, albumHash));
	}
}


void TPlayer::deleteSong(const std::string& fileHash, const std::string& playlist) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	music::PPlaylist pls = playlists[playlist];
	if (util::assigned(pls)) {
		music::CSongData song;
		std::string current = playlist;
		getCurrentSong(song, current);
		if (song.valid) {
			music::PSong p = library.findFile(fileHash);
			if (util::assigned(p)) {
				if (song.albumHash == p->getAlbumHash()) {
					logger(util::csnprintf("[Playlist] [Delete song] Song $ for playlist $ skipped, album is in use.", fileHash, playlist));
					return;
				}
			}
		}
		std::lock_guard<std::mutex> lock2(nextSongMtx);
		if (pls->deleteFile(fileHash)) {
			if (util::assigned(nextSong.song)) {
				if (fileHash == nextSong.song->getFileHash()) {
					nextSong.clear();
				}
			}
			logger(util::csnprintf("[Playlist] [Delete song] Song $ deleted from playlist $", fileHash, playlist));
		} else {
			logger(util::csnprintf("[Playlist] [Delete song] No song for $ deleted from playlist $", fileHash, playlist));
		}
	} else {
		if (playlist.empty())
			logger(util::csnprintf("[Playlist] [Delete song] Empty playlist to add song $", fileHash));
		else
			logger(util::csnprintf("[Playlist] [Delete song] Invalid playlist $ to add song $", playlist, fileHash));
	}
}

void TPlayer::deleteAlbum(const std::string& albumHash, const std::string& playlist) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	music::CSongData song;
	std::string current;
	getCurrentSong(song, current);
	if (song.valid) {
		if (song.albumHash == albumHash && current == playlist) {
			logger(util::csnprintf("[Playlist] [Delete album] Album $ for playlist $ skipped, album is in use.", albumHash, playlist));
			return;
		}
	}
	music::PPlaylist pls = playlists[playlist];
	if (util::assigned(pls)) {
		std::lock_guard<std::mutex> lock2(nextSongMtx);
		int c = pls->deleteAlbum(albumHash);
		if (c > 0) {
			if (util::assigned(nextSong.song)) {
				if (albumHash == nextSong.song->getAlbumHash() && playlist == nextSong.playlist) {
					nextSong.clear();
				}
			}
			logger(util::csnprintf("[Playlist] [Delete album] % songs deleted from playlist $ for $", c, playlist, albumHash));
		} else {
			logger(util::csnprintf("[Playlist] [Delete album] No songs deleted from playlist $ for $", playlist, albumHash));
		}
	} else {
		if (playlist.empty())
			logger(util::csnprintf("[Playlist] [Delete album] Empty playlist to delete album $", albumHash));
		else
			logger(util::csnprintf("[Playlist] [Delete album] Invalid playlist $ to delete album $", playlist, albumHash));
	}
}


int TPlayer::unlinkSong(const std::string& fileHash, const std::string& albumHash, const std::string& playlist) {
	int c = 0;
	music::PPlaylist pls = playlists[playlist];
	if (util::assigned(pls)) {
		std::string album = albumHash;
		if (album.empty()) {
			music::PSong o = pls->findSong(fileHash);
			if (util::assigned(o)) {
				album = o->getAlbumHash();
				c = pls->deleteAlbum(album);
			}
		}
		if (c == 0) {
			if (pls->deleteFile(fileHash))
				c = 1;
		}
		if (c > 0) {
			logger(util::csnprintf("[Playlist] [Unlink song] % songs deleted from playlist $ for song $ of album $", c, playlist, fileHash, album));
		} else {
			logger(util::csnprintf("[Playlist] [Unlink song] No songs deleted from playlist $ for song $ of album $", playlist, fileHash, album));
		}
	} else {
		if (playlist.empty())
			logger(util::csnprintf("[Playlist] [Unlink song] Empty playlist to delete song $ for album $", fileHash, albumHash));
		else
			logger(util::csnprintf("[Playlist] [Unlink song] Invalid playlist $ to delete song $ for album $", playlist, fileHash, albumHash));
	}
	return c;
}


std::string TPlayer::addArtist(const std::string& artistName, const std::string& playlist, const music::EPlayListAction action) {
	std::string hash;
	app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
	music::PPlaylist pls = playlists[playlist];
	if (util::assigned(pls)) {
		const music::TArtistMap& artists = library.getAllArtists();
		music::TArtistConstIterator it = artists.find(artistName);
		if (it != artists.end()) {
			if (!it->second.albums.empty()) {
				int c = 0;
				pls->prepare();
				music::EPlayListAction epa = sanitizePlayerAction(action, playlist);
				music::TAlbumConstIterator at = it->second.albums.begin();
				while (at != it->second.albums.end()) {
					std::string albumHash = at->second.hash;
					pls->removeAlbum(albumHash);
					c += pls->addAlbum(albumHash, epa, false);
					++at;
					if (hash.empty())
						hash = albumHash;
				}
				pls->commit();
				if (c > 0) {
					logger(util::csnprintf("[Playlist] [Add artist] % songs added to playlist $ for $", c, playlist, artistName));
				} else {
					logger(util::csnprintf("[Playlist] [Add artist] No songs added to playlist $ for $", playlist, artistName));
				}
			} else {
				logger(util::csnprintf("[Playlist] [Add artist] No albums found for $", artistName));
			}
		} else {
			logger(util::csnprintf("[Playlist] [Add artist] No artist found for $", artistName));
		}
	} else {
		if (playlist.empty())
			logger(util::csnprintf("[Playlist] [Add artist] Empty playlist to add artist $", artistName));
		else
			logger(util::csnprintf("[Playlist] [Add artist] Invalid playlist $ to add artist $", playlist, artistName));
	}
	return hash;
}


void TPlayer::invalidateSongDislpay() {
	app::TLockGuard<app::TMutex> lock(songDisplayMtx);
	songDisplayChanged++;
	if (songDisplayChanged > 0xFFFF)
		songDisplayChanged = 1;
}

size_t TPlayer::getSongDisplayChange() {
	app::TLockGuard<app::TMutex> lock(songDisplayMtx);
	return songDisplayChanged;
}


void TPlayer::onSigint() {
	logger("[Signal] STRG+C pressed: Waiting for application to shutdown...");
	terminate = true;
	notifyEvent("[Signal] STRG+C pressed");
}

void TPlayer::onSigterm() {
	logger("[Signal] SIGTERM signaled. Shutdown application now.");
	terminate = true;
	notifyEvent("[Signal] Terinate application");
}

void TPlayer::onSighup() {
	logger("[Signal] SIGHUP signaled. Updating application settings.");
	{
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
		playlists.resize(sound.getPlaylistSize());
		playlists.commit(true);
	}
}

void TPlayer::onSigusr1() {
	logger("[Signal] SIGUSR1 signaled.");
	addCommand(ECA_PLAYER_PAUSE);
}

void TPlayer::onSigusr2() {
	logger("[Signal] SIGUSR2 signaled.");
	addCommand(ECA_PLAYER_PLAY);
}


void TPlayer::onLibraryProgress(const music::TLibrary& sender, const size_t count, const std::string& current) {
	if (util::isClass<music::TPlaylist>(&sender)) {
		if (debug)
			logger("[Rescan] Music playlist read " + std::to_string((size_s)count) + " items, current file \"" + current + "\"");
	} else {
		setScannerStatusLabel(true, count, 0);
		if (debug)
			logger("[Rescan] Music database read " + std::to_string((size_s)count) + " items, current file \"" + current + "\"");
	}
}


static const std::string STRIP_STATION_CHARS   = "_\\/";
static const std::string REPLACE_STATION_CHARS = " --";

static char replaceStationChars(char value) {
	uint8_t u = (uint8_t)value;
	if (u < UINT8_C(128)) {
		size_t idx = STRIP_STATION_CHARS.find(value);
		if(idx != std::string::npos) {
			return REPLACE_STATION_CHARS[idx];
		}
		return toupper(value);
	}
	return value;
}

void TPlayer::normalizeStationName(std::string& description, const std::string name) {
	if (!description.empty()) {

		// Check for "default" name
		if (util::strcasestr(description, "default")) {
			description = name;
			return;
		}

		// Count underscores
		size_t count = 0;
		for (size_t i=0; i<description.size(); ++i) {
			if ('_' == description[i]) {
				++count;
			}
		}

		// Replace underscores and conver to upper case
		if (count > 1) {
		    std::transform(description.begin(), description.end(), description.begin(), replaceStationChars);
		}
	}
}

bool TPlayer::splitRadioText(const std::string& title, const radio::EMetadataOrder order, std::string& artist, std::string& track) {
	artist.clear();
	track.clear();
	if (order == radio::EMO_ARTIST_TITLE || order == radio::EMO_TITLE_ARTIST) {
		size_t pos = title.find(" - ");
		if (std::string::npos != pos) {
			if (title.size() > 10) {
				if (pos < (title.size() - 6)) {
					artist = title.substr(0, pos);
					track = title.substr(pos + 3);
					if (order == radio::EMO_TITLE_ARTIST) {
						artist.swap(track);
					}
					return true;
				}
			}
		}
	}
	return false;
}

void TPlayer::onPlayerStateChanged(const music::TAlsaPlayer& sender, const music::EPlayerState state, const music::TSong* current, const std::string& playlist) {
	bool ok = false;
	std::string bits;
	player.getBitDepth(bits);
	bool streamable = false;
	bool playing = util::assigned(current);
	if (playing) {
		streamable = current->isStreamed();
		ok = !streamable;
	}
	if (ok) {
		logger(util::csnprintf("[Event] [State] Current song $ for player state <%> and playlist $", current->getTitle(), music::TAlsaPlayer::statusToStr(state), playlist));

		// Get current configured and requested values
		music::CConfigValues values;
		sound.getConfiguredValues(values);
		const std::string& fileHash = current->getFileHash();
		const std::string& albumHash = current->getAlbumHash();
		
		// Check if album in recent song playlist
		// --> Add at top position!
		{
			app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_WRITE);
			if (!albumHash.empty() && !playlist.empty() && !playlists.isRecent(playlist) && !current->isStreamed()) {
				bool found = playlists.recent()->isFirstAlbum(albumHash);
				if (!found) {
					addAlbumWithNolock(albumHash, playlists.recent()->getName(), music::EPA_INSERT);
				}
			}
		}

		// Check if playback has stopped, e.g. on stream parameters changed	
		if (util::isMemberOf(state, music::EPS_WAIT,music::EPS_REOPEN)) {
			logger("[Restart] [State] Playback command for song \"" + current->getTitle() + "\" queued.");
			TCommand command;
			command.action = ECA_PLAYLIST_PLAY_SONG;
			command.file = fileHash;
			command.album = albumHash;
			command.playlist = playlist;
			queueCommand(command);
		}

		// Check if playback in progress, wait before buffering next song
		if (state == music::EPS_PLAY) {
			toBuffering->restart();
		}

		// Store current song data
		setCurrentState(current, playlist, state);
		updatePlaylistMenuItems(playlist);

		// Load image by query:   std::string query = util::quote("/rest/coverart.jpg?hash=" + current->getAlbumHash() + "&url=" + current->getURL() + "&size=400");
		// New fixed name loader: img src=\"/rest/thumbnails/" + hash + "-400.jpg\"
		std::string query = util::quote("/rest/thumbnails/%-%.jpg");
		std::string duration = music::TSong::timeToStr(current->getDuration());
		int sampleRate = (current->getBitsPerSample() < 8) ? 16 * current->getSampleRate() : current->getSampleRate();

		// Get artist name for track
		std::string albumartist = current->getDisplayOriginalAlbumArtist();
		std::string titleartist = current->getDisplayOriginalArtist();
		std::string title = current->getDisplayTitle();
		if (values.displayOrchestra) {
			if (0 != util::strcasecmp(titleartist, albumartist)) {
				title += "<br><small style=\"display:block; line-height:24px;\">" + titleartist + "</small>";
			}
		}

		*wtPlaylistTitle     = title;
		*wtPlaylistArtist    = albumartist;
		*wtPlaylistDataTitle = util::quote("<i>" + albumartist + "</i><br>" + current->getDisplayAlbum() + " (" + current->getDisplayYear() + ")");
		*wtPlaylistPreview   = util::csnprintf(query, current->getAlbumHash(), 600);
		*wtPlaylistCoverArt  = util::csnprintf(query, current->getAlbumHash(), 400);
		*wtPlaylistAlbum     = values.displayOrchestra ? current->getDisplayExtendedInfo() : current->getDisplayExtendedAlbum();
		*wtPlaylistDate      = current->getDisplayDate();
		*wtPlaylistSearch    = current->getAlbumArtist();
		*wtPlaylistTrack     = (current->getDiskNumber() > 1 || (current->getDiskCount() > 1 && current->getDiskNumber() > 0)) ? util::csnprintf("% of disk %", current->getTrackNumber(), current->getDiskNumber()) : std::to_string((size_s)current->getTrackNumber());
		*wtPlaylistDuration  = duration;
		*wtPlaylistTime      = getProgressTimestamp(current->getPlayed(), current->getDuration(), values.displayRemain);
		*wtPlaylistProgress  = current->getPercent();

		*wtPlaylistSamples   = std::to_string((size_u)sampleRate) + " Samples/sec";
		*wtPlaylistFormat    = bits + " Bit";
		*wtPlaylistCodec     = current->getCodec();
		*wtPlaylistIcon      = current->getIcon();

		// Send Telnet client broadcast
		sendBroadcastMessage("CURRENT|" + getSongsStatusString(current, playlist, "|"));

	} else {
		if (playing && streamable) {
			// Set stream as current song
			setCurrentState(current, playlist, state);
			logger(util::csnprintf("[Event] [State] Current stream state changed to <%>", music::TAlsaPlayer::statusToStr(state)));
		} else {
			// Clear current song data
			setCurrentState(nil, "", state);
			logger("[Event] [State] No song to play.");
		}

		// Clear display values
		*wtPlaylistTime = "0:00";
		*wtPlaylistProgress = 0;

		if (playing && streamable) {

			// Get station information
			inet::TStreamInfo info;
			radio::EMetadataOrder order;
			std::string header, name, url, title, description, bitrate;
			if (getStreamProperties(name, url, order)) {
				header = "Playing &quot;" + name + "&quot;";
			} else {
				header = "Internetstream";
			}

			// Get stream information
			getRadioTitle(title);
			getRadioStation(info);
			if (std::string::npos == info.bitrate) {
				bitrate = "VBR";
			} else {
				if (info.bitrate > 0) {
					bitrate = util::csnprintf("%&nbsp;kBit", info.bitrate);
				} else {
					bitrate = "-";
				}
			}

			// Check description for valid entry
			description = info.getDescription();
			normalizeStationName(description, name);

			// Check if title is split in <Artist> + " - " + <Title>
			std::string artist, track;
			if (splitRadioText(title, order, artist, track)) {
				*wtPlaylistArtist = artist.empty() ? header : artist;
				*wtPlaylistTitle  = track.empty() ? (title.empty() ? "---" : title) : track;
			} else {
				*wtPlaylistArtist = header;
				*wtPlaylistTitle  = title.empty() ? "---" : title;
			}

			// Set default stream properties
			*wtPlaylistSamples  = std::to_string((size_u)current->getSampleRate()) + " Samples/sec";
			*wtPlaylistFormat   = bits + " Bit";
			*wtPlaylistCodec    = current->getCodec();
			*wtPlaylistIcon     = music::SR44K == current->getSampleRate() ? "cd-audio" : "dvd-audio";
			*wtPlaylistTrack    = util::sizeToStr(info.streamed);
			*wtPlaylistAlbum    = description.empty() ? name : description;
			*wtPlaylistDate     = bitrate;
			*wtPlaylistDuration = "&nbsp;&infin;&nbsp;";
			*wtPlaylistSearch   = "@";

			// Set default cover art
			*wtPlaylistDataTitle = util::quote(header);
			*wtPlaylistPreview   = util::quote(SPECIAL_PREVIEW_URL);
			*wtPlaylistCoverArt  = util::quote(SPECIAL_REST_URL);

		} else {

			// Nothing to play...
			*wtPlaylistArtist   = "Now playing...";
			*wtPlaylistSamples  = "0 Samples/sec";
			*wtPlaylistFormat   = "0 Bit";
			*wtPlaylistCodec    = "n.a.";
			*wtPlaylistIcon     = "no-audio";
			*wtPlaylistTrack    = 0;
			*wtPlaylistAlbum    = "---";
			*wtPlaylistDate     = "-";
			*wtPlaylistTitle    = "---";
			*wtPlaylistDuration = "0:00";
			*wtPlaylistSearch   = "*";

			// Set default cover art
			*wtPlaylistDataTitle = util::quote("No cover loaded.");
			*wtPlaylistPreview   = util::quote(NOCOVER_PREVIEW_URL);
			*wtPlaylistCoverArt  = util::quote(NOCOVER_REST_URL);
		}
	}

	// Invalidate/refresh radio text display
	invalidateSongDislpay();

	// Check for sample rate change
	music::ESampleRate rate = getAndClearNextRate();
	if (rate != music::SR0K) {
		music::CAudioValues values;
		sound.getAudioValues(values);
		if (values.enabled) {
			bool opened = false;
			bool unchanged = false;
			if (openRemoteDevice(values, rate, opened, unchanged)) {
				if (opened)
					logger(util::csnprintf("[Remote] [State] Opened remote device $ for % Hz", values.remote, remote.getRate()));
				else
					logger(util::csnprintf("[Remote] [State] Remote device $ already opened for % Hz", values.remote, remote.getRate()));
			} else {
				if (!unchanged)
					logger(util::csnprintf("[Remote] [State] Failed to open remote device $ on error $", values.remote, remote.strerr()));
			}
		}
	}

	// Save current playing song
	if (!terminate) {
		if (state == music::EPS_PLAY) {
			saveCurrentSong(current, playlist);
		} else {
			saveCurrentSong(nil, playlist);
		}
	}

	// Update web interface objects
	wtPlaylistCoverArt->invalidate();
	setPlayerStatusLabel(state, streamable);

	// Send HMI update vio websocket
	broadcastPlayerUpdate();
}

void TPlayer::onPlayerOutputChanged(const music::TAlsaPlayer& sender, const music::EPlayerState state, const music::TSong* current, const std::string& playlist) {
	if (util::assigned(current)) {
		logger(util::csnprintf("[Event] [Output] Current song $ for player state <%> and playlist $", current->getTitle(), music::TAlsaPlayer::statusToStr(state), playlist));
	} else {
		std::string hash;
		if (getAndClearNextHash(hash)) {
			startInetStream(hash);
			logger(util::csnprintf("[Event] [Output] Started next queued stream $", hash));
		} else {
			logger("[Event] [Output] No song to play.");
		}
	}
}

void TPlayer::onPlayerProgressChanged(const music::TAlsaPlayer& sender, const music::EPlayerState state, const music::TSong* current, const int64_t streamed, const std::string& playlist) {
	bool ok = false;
	if (util::assigned(current)) {
		if (!current->isStreamed()) {
			// Get configured display settings
			music::CConfigValues values;
			sound.getConfiguredValues(values);
			*wtPlaylistTime = getProgressTimestamp(current->getPlayed(), current->getDuration(), values.displayRemain);
			*wtPlaylistProgress  = current->getPercent();
			ok = true;
		}
	}
	if (!ok) {
		*wtPlaylistTime = "0:00";
		*wtPlaylistProgress = 0;
	}
	*wtStreamedSize = util::sizeToStr(streamed, 1, util::VD_BINARY);

	// Force web token update
	wtPlaylistProgress->invalidate();
	wtPlaylistTime->invalidate();
	wtStreamedSize->invalidate();

	// Send HMI update vio websocket
	broadcastPlayerUpdate();
}

void TPlayer::onFileUploadEvent(const app::TWebServer& sender, const util::TVariantValues& session, const std::string& fileName, const size_t& size) {
	std::string ext = util::fileExt(fileName);
	if (ext == "lic") {
		logger(util::csnprintf("[File Upload] License file $ with size % uploaded", fileName, util::sizeToStr(size, 1, util::VD_BINARY)));
		readApplicationLicenseFromFile(fileName);
	}
}

void TPlayer::onWebStatisticsEvent(const app::TWebServer& sender, const app::TWebData&) {
	// Send brockast message to all connected websocket clients
	broadcastWebSocketEvent("webserver","update");
}

void TPlayer::broadcastPlayerUpdate() {
	// Send brockast message to all connected websocket clients
	broadcastWebSocketEvent("player","update");
}

void TPlayer::broadcastStreamUpdate() {
	// Send brockast message to all connected websocket clients
	broadcastWebSocketEvent("stream","update");
}

void TPlayer::broadcastWebSocketEvent(const std::string& location, const std::string& message) {
	// Simply echo web socket data for now...
	if (application.hasWebServer() && !location.empty() && !message.empty()) {
		std::string json = util::csnprintf("{\"location\":$,\"action\":$}", location, message);
		application.getWebServer().broadcast(json);
		//std::cout << "TPlayer::broadcastWebSocketEvent() Location \"" << location << "\" Action \"" << message << "\" JSON = " << json << std::endl;
	}
}

void TPlayer::onWebSocketData(const app::THandle handle, const std::string& message) {
	// Simply echo web socket data for now...
	if (application.hasWebServer()) {
		application.getWebServer().write(handle, message);
	}
}

void TPlayer::onWebSocketVariant(const app::THandle handle, const util::TVariantValues& variants) {
	if (!application.isDaemonized())
		variants.debugOutput("TPlayer::onWebSocketVariant()", "  ");
}


void TPlayer::addClientSocket(inet::TSocket& socket, const app::THandle client) {
	bool ok = false;
	inet::TServerSocket* o = util::asClass<inet::TServerSocket>(&socket);
	if (util::assigned(o)) {
		app::TLockGuard<app::TMutex> lock(telnetMtx);
		clients[client] = o;
		ok = true;
	}
	if (ok) {
		sendSocketMessage(*o, client, "ONLINE|" + application.getVersion() + "|" + application.getHostName());
		logger("[" + socket.getName() + "/" + socket.getService() + "] Add client socket <" + std::to_string((size_s)client) + ">");
	}
}

void TPlayer::deleteClientSocket(const inet::TSocket& socket, const app::THandle client) {
	bool ok = false;
	{
		app::TLockGuard<app::TMutex> lock(telnetMtx);
		if (!clients.empty()) {
			TClientList::iterator it = clients.find(client);
			if (it != clients.end()) {
				clients.erase(it);
				ok = true;
			}
		}
	}
	if (ok) {
		logger("[" + socket.getName() + "/" + socket.getService() + "] Delete client socket <" + std::to_string((size_s)client) + ">");
	}
}

void TPlayer::sendBroadcastMessage(const std::string message) {
	app::TLockGuard<app::TMutex> lock(telnetMtx);
	if (!clients.empty()) {
		TClientList::iterator it = clients.begin();
		while (it != clients.end()) {
			if (util::assigned(it->second))
				sendSocketMessage(*(it->second), it->first, message);
			++it;
		}
	}
}

void TPlayer::sendClientMessage(const inet::TServerSocket& socket, const app::THandle client, const std::string message) {
	app::TLockGuard<app::TMutex> lock(telnetMtx);
	sendSocketMessage(socket, client, message);
}

void TPlayer::sendSocketMessage(const inet::TServerSocket& socket, const app::THandle client, const std::string message) {
	if (!message.empty()) {
		std::string s = message + "\r\n";
		const void * q = (void*)s.c_str();
		size_t size = s.size();
		socket.send(client, q, size);
	}
}

void TPlayer::onSocketConnect(inet::TSocket& socket, const app::THandle client) {
	logger("[" + socket.getName() + "/" + socket.getService() + "] Client connected (" + std::to_string((size_s)client) + ")");
	addClientSocket(socket, client);
}

void TPlayer::onSocketClose(inet::TSocket& socket, const app::THandle client) {
	logger("[" + socket.getName() + "/" + socket.getService() + "] Client disconnected (" + std::to_string((size_s)client) + ")");
	deleteClientSocket(socket, client);
}

ssize_t TPlayer::onSocketData(const inet::TServerSocket& socket, const app::THandle client, bool& drop) {
	util::TBuffer buffer(1024);
	ssize_t received = socket.receive(client, buffer(), buffer.size());
	if (received > 0 && received <= MAX_COMMAND_SIZE && !terminate) {
		size_t length = strnlen(buffer(), std::min(MAX_COMMAND_SIZE, received));
		std::string command(buffer(), length);

		// Execute telnet command
		std::string result;
		executeTelnetCommand(command, result, drop);

		// Send data back to sender
		if (!drop && !result.empty()) {
			sendClientMessage(socket, client, result);
		}
	}
	return received;
}

void TPlayer::executeTelnetCommand(const std::string data, std::string& result, bool& drop) {
	result.clear();
	if (!data.empty()) {
		bool ok = false;
		std::string text = trim(data);
		std::string command = util::toupper(text);

		// Is valid command action?
		ECommandAction action = actions[command];
		if (!ok && util::isMemberOf(action, ECA_PLAYER_STOP, ECA_PLAYER_PLAY, ECA_PLAYER_PAUSE, \
							ECA_PLAYER_NEXT, ECA_PLAYER_PREV, ECA_PLAYER_FF, ECA_PLAYER_FR, ECA_PLAYER_RAND)) {
			addCommand(action);
			result = "OK|" + command;
			ok = true;
		}

		// Toggle repeat modes
		if (!ok && util::isMemberOf(action, ECA_PLAYER_MODE_RANDOM, ECA_PLAYER_MODE_REPEAT, ECA_PLAYER_MODE_SINGLE, ECA_PLAYER_MODE_DISK, \
							ECA_OPTION_RED, ECA_OPTION_GREEN, ECA_OPTION_YELLOW, ECA_OPTION_BLUE)) {
			switch (action) {
				case ECA_PLAYER_MODE_RANDOM:
					action = ECA_OPTION_RED;
					break;
				case ECA_PLAYER_MODE_REPEAT:
					action = ECA_OPTION_GREEN;
					break;
				case ECA_PLAYER_MODE_SINGLE:
					action = ECA_OPTION_YELLOW;
					break;
				case ECA_PLAYER_MODE_DISK:
					action = ECA_OPTION_BLUE;
					break;
				default:
					break;
			}
			addCommand(action);
			ok = true;
		}

		// Switch remote input
		if (!ok && util::isMemberOf(action, ECA_REMOTE_INPUT0, ECA_REMOTE_INPUT1, ECA_REMOTE_INPUT2, ECA_REMOTE_INPUT3, ECA_REMOTE_INPUT4,
											ECA_REMOTE_FILTER0, ECA_REMOTE_FILTER1, ECA_REMOTE_FILTER2, ECA_REMOTE_FILTER3, ECA_REMOTE_FILTER4, ECA_REMOTE_FILTER5,
											ECA_REMOTE_PHASE0, ECA_REMOTE_PHASE1)) {
			// Read configuration values from parameter list
			music::CAudioValues values;
			sound.getAudioValues(values);

			// Execute remote command
			if (values.control) {
				executeRemoteAction(values, action);
				result = "OK|" + command;
			} else {
				result = "DISABLED";
			}

			ok = true;
		}

		// Switch remote sample frequency
		if (!ok && util::isMemberOf(action, ECA_REMOTE_44K, ECA_REMOTE_48K)) {
			// Get audio hardware settings
			music::CAudioValues values;
			sound.getAudioValues(values);

			// Set remote word clock output device
			if (values.enabled) {
				music::ESampleRate rate = music::SR44K;
				if (ECA_REMOTE_48K == action)
					rate = music::SR48K;
				setRemoteDevice(values, rate);
				result = "OK|" + std::to_string((size_u)rate);
			} else {
				result = "DISABLED";
			}

			ok = true;
		}

		// Peek hardware status
		if (!ok && (0 == util::strncasecmp(command, "PEEK", 4))) {
			// Get audio hardware settings
			music::CAudioValues values;
			sound.getAudioValues(values);
			if (values.control) {
				if (trySendRemoteStatusRequest()) {
					result = "OK";
				} else {
					result = "BUSY";
				}
			} else {
				result = "DISABLED";
			}
			ok = true;
		}

		// Is status request?
		if (!ok && (0 == util::strncasecmp(command, "STATUS", 6) || (command == "."))) {
			music::CSongData song;
			std::string playlist;
			getCurrentSong(song, playlist);
			if (!song.artist.empty()) {
				result = "PLAY|" + getSongsStatusString(song, playlist, "|");
			} else {
				result = "IDLE";
			}
			ok = true;
		}

		// Is mode request?
		if (!ok && 0 == util::strncasecmp(command, "MODE", 4)) {
			TPlayerMode mode;
			getCurrentMode(mode);
			result = "MODE|" + modeToStr(mode, '|');
			ok = true;
		}

		// Is version request?
		if (!ok && 0 == util::strncasecmp(command, "VERSION", 6)) {
			result = "VERSION|" + application.getVersion() + "|" + application.getHostName();
			ok = true;
		}

		// Is system command?
		if (!ok && "SYSTEM" == command.substr(0, 6)) {
			int retVal;
			util::TBuffer output;
			std::string commandLine = util::trim(text.substr(7, std::string::npos));
			if (!commandLine.empty()) {
				bool r = util::executeCommandLine(commandLine, output, retVal, true, 10);
				if (r && !output.empty()) {
					const char* p = output.data();
					size_t size = output.size();
					if (size > 0) {
						result.assign(p, size);
					}
				}
				if (result.empty()) {
					result = r ? "SYSTEM: Command <" + commandLine + "> executed." : "Execution of <" + commandLine + "> failed.";
				}
			} else {
				result = "SYSTEM: Missing command line.";
			}
			ok = true;
		}

		// Check for Telnet exit commands
		if (!ok && data.size() > 2) {
			unsigned char* c = (unsigned char*)data.c_str();
			if (0xFF == c[0]) {
				if (0xED == c[1]) {
					command = "CONTROL-Z";
					drop = true;
					ok = true;
				}
				if (0xF4 == c[1]) {
					command = "CONTROL-C";
					drop = true;
					ok = true;
				}
			}
		}

		// Is termination command?
		if (!ok && (0 == util::strncasecmp(command, "EXIT", 4) || 0 == util::strncasecmp(command, "QUIT", 4))) {
			drop = true;
			ok = true;
		}

		// Is termination command?
		if (!ok && 0 == util::strncasecmp(command, "TERMINATE", 9)) {
			terminate = true;
			notifyEvent("[Telnet] Command received");
			result = "BYE";
			ok = true;
		}

		// Is help request?
		if (!ok && 0 == util::strncasecmp(command, "HELP", 4)) {
			result  = "Valid input token\r\n";
			result += "  Player commands  : stop, play, pause, next, prev, rand, ff, fr\r\n";
			result += "  Mode commands    : random, repeat, single, disk\r\n";
			result += "  Input commands   : input0, input1, input2, input3, input4\r\n";
			result += "  Device command   : peek (send status request to DAC)\r\n";
			result += "  Rate commands    : sr44k, sr48k\r\n";
			result += "  Status requests  : version, mode, status or .\r\n";
			result += "  Control commands : exit, quit, terminate\r\n";
			result += "  System command   : system <command parameter> e.g. system ls -la\r\n";
			ok = true;
		}

		if (ok) {
			logger(util::csnprintf("[Telnet Interface] Executed command $", command));
		} else {
			if (command.empty()) {
				logger("[Telnet Interface] Empty command.");
			} else {
				command = util::TBinaryConvert::strToAscii(command);
				logger(util::csnprintf("[Telnet Interface] Invalid command $ [%]", command, util::TBinaryConvert::binToHexA(data.c_str(), data.size())));
				result = "INVALID|" + command;
			}
		}
	}
}


ssize_t TPlayer::onLircControlData(const inet::TUnixClientSocket& socket) {
	ssize_t retVal = 0;
	util::TBuffer buffer(128);
	do {
		retVal = socket.receive(buffer(), buffer.size());
		if (retVal > 0) {
			executeLircCommand(buffer, retVal);
		}
	} while ((retVal > 0) && (retVal >= (ssize_t)buffer.size()));

	return (ssize_t)0;
}

void TPlayer::executeLircCommand(const util::TBuffer& buffer, const size_t size) {
	if (!buffer.empty()) {
		// Parse LIRC command:
		// e.g. 000000037ff07bdd 00 OK mceusb
		char scanCode[128];
		char buttonName[128];
		char repeatStr[64];
		char deviceName[128];
		sscanf(buffer.data(), "%s %s %s %s", &scanCode[0], &repeatStr[0], &buttonName[0], &deviceName[0]);

		// Check for valid repeat entry
		char *end = nil;
		long int repeat = strtol(repeatStr, &end, 16);
		if (!end || *end != 0) {
			std::string s = util::trim(std::string(buffer.data(), size));
			logger("[LIRC] Received malformed repeat value in remote control data \"" + s + "\"");
			return;
		}

		// Check for valid scan code
		end = nil;
		strtol(scanCode, &end, 16);
		if (!end || *end != NUL) {
			std::string s = util::trim(std::string(buffer.data(), size));
			logger("[LIRC] Received malformed scan code in remote control data \"" + s + "\"");
			return;
		}

		// Check for valid device name
		std::string device;
		size_t len = strnlen(deviceName, 15);
		if (len > 0 && len < 15) {
			device = std::string(deviceName, len);
		} else {
			std::string s = util::trim(std::string(buffer.data(), size));
			logger("[LIRC] Received malformed device name in remote control data \"" + s + "\"");
			return;
		}

		// Execute first key press only
		if (repeat == 0) {
			len = strnlen(buttonName, 10);
			if (len > 0 && len < 10) {
				std::string command = util::toupper(util::unquote(std::string(buttonName, len)));

				// Check if blocking process is running
				bool goon = true;
				if (!lircBlockingList.empty()) {
					if (sysutil::isProcessRunning(lircBlockingList))
						goon = false;
				}

				// Execute remote command
				if (goon) {

					// Get current LIRC command
					ECommandAction action = getControlCommand(command);
					if (action != ECA_INVALID_ACTION) {

						// Compare last action with current action
						// --> prevent PAUSE/PLAY/STOP to be repeated in short succession
						if (lircAction != ECA_INVALID_ACTION) {
							if (lircAction == action && util::isMemberOf(action, ECA_PLAYER_PLAY,ECA_PLAYER_PAUSE,ECA_PLAYER_STOP)) {
								// Action is blocked by delay
								goon = false;
							}
						}

						// Restart repeat timer anyway...
						lircAction = action;
						if (util::assigned(toLircTimeout)) {
							toLircTimeout->restart();
						}

						// Execute given action
						if (goon) {
							logger("[LIRC] Execute control command \"" + command + "\" on device \"" + device + "\"");
							executeControlAction(action);
							// action = addControlCommand(command);
							lircBlocked = false;
							if (!util::isMemberOf(action, ECA_INVALID_ACTION,ECA_PLAYER_FF,ECA_PLAYER_FR)) {
								sysutil::TBeeperParams beeper;
								beeper.repeats = 1;
								beeper.length = DEFAULT_BEEPER_LENGTH * 3 / 10;
								sysutil::beep(beeper);
							}
						} else {
							logger("[LIRC] Command \"" + command + "\" on device \"" + device + "\" ignored.");
						}

					} else {
						logger("[LIRC] Invalid command \"" + command + "\" on device \"" + device + "\"");
					}

				} else {
					if (!lircBlocked) {
						logger("[LIRC] Remote control commands blocked by running process.");
						lircBlocked = true;
					}
				}

			} else {
				std::string s = util::trim(std::string(buffer.data(), size));
				logger("[LIRC] Received malformed command in remote control data \"" + s + "\"");
			}
		}
	}
}

void TPlayer::onCommandDelay() {
	lircAction = ECA_INVALID_ACTION;
}


void TPlayer::notifyEvent(const std::string message) {
	app::TLockGuard<app::TMutex> lock(eventMtx, false);
	if (!lock.tryLock()) {
		logger(message + ": Notify blocked.");
		return;
	}
	TEventResult r = event.notify();
	if (r != EV_SUCCESS) {
		logger(message + ": Notify failed.");
		return;
	}
	logger(message + ": Notify executed.");
}

void TPlayer::onTasksTimeout() {
	// Notify main wait loop to leave sleeping state
	notifyEvent("[Timeout] Restart main loop");
}

void TPlayer::onCommandTimeout() {
	// Reset remote command receiving state
	if (terminal.isBusy()) {
		logger("[Remote] Remote command response timeout.");
		terminal.cancel();
	}
}

void TPlayer::onHotplug() {
	// Delayed hotplug event to accumulate multiple events in short succession
	sysutil::TBeeperParams beeper;
	beeper.repeats = 3;
	beeper.length = DEFAULT_BEEPER_LENGTH * 6 / 10;
	sysutil::beep(beeper);
	invalidateLocalDrives();
	invalidateHardwareDevices();
	invalidateSerialPorts();
	logger("[Hardware] USB device hotplug event signaled.");
}

void TPlayer::onFileWatch() {
	// Delayed file watch event to accumulate multiple events in short succession
	sysutil::TBeeperParams beeper;
	beeper.repeats = 2;
	beeper.length = DEFAULT_BEEPER_LENGTH * 6 / 10;
	sysutil::beep(beeper);
	logger("[Watch] Delayed file watch event signaled.");
}

void TPlayer::onWatchEvent(const std::string& file, bool& proceed) {
	bool found = false;
	bool display = false;
	if (!found && file == sound.getControlFile()) {
		logger("[Watch] Command file <" + file + "> changed.");
		notifyEvent("[Watch] File watch fired");
		found = true;
	}
	util::TStringList folders;
	if (sound.getMusicFolders(folders)) {
		for (size_t i=0; i<folders.size(); i++) {
			std::string folder = folders[i];
			if (!found && 0 == util::strncasecmp(file, folder, folder.size())) {
				logger("[Watch] File <" + file + "> in music content changed.");
				contentChanged = true;
				if (contentCompare != contentChanged) {
					app::TLockGuard<app::TMutex> lock(rescanMtx, false);
					if (lock.tryLock()) {
						contentCompare = true;
						display = true;
					}
				}
				found = true;
			}
			if (found)
				break;
		}
	}
	if (found) {
		proceed = false;
		toFileWatch->restart(DEFAULT_FILE_WATCH_DELAY);
	}
	if (display) {
		if (util::assigned(wtScannerStatusColor)) {
			app::TLockGuard<app::TMutex> lock(scnDisplayMtx);
			*wtScannerStatusCaption = "Library needs update...";
			*wtScannerStatusColor = "label-primary";
			wtScannerStatusColor->invalidate();
		}
	}
}

void TPlayer::onHotplugEvent(const app::THotplugEvent& event, const app::EHotplugAction action) {
	if (util::isMemberOf(action, HA_ADD,HA_REMOVE)) {
		// Restart hotplug event delay
		toHotplug->restart(DEFAULT_HOTPLUG_DELAY);
	}
}


ECommandAction TPlayer::getInputActionFromRemote(const uint8_t value) {
	switch (value) {
		case 6:
			return ECA_REMOTE_INPUT0;
			break;
		case 2:
			return ECA_REMOTE_INPUT1;
			break;
		case 3:
			return ECA_REMOTE_INPUT2;
			break;
		case 0:
			return ECA_REMOTE_INPUT3;
			break;
		case 1:
			return ECA_REMOTE_INPUT4;
			break;
		default:
			break;
	}
	return ECA_INVALID_ACTION; // Invalid for now...
}

ECommandAction TPlayer::getFilterActionFromRemote(const uint8_t value) {
	switch (value) {
		case 0:
			return ECA_REMOTE_FILTER0;
			break;
		case 1:
			return ECA_REMOTE_FILTER1;
			break;
		case 2:
			return ECA_REMOTE_FILTER2;
			break;
		case 3:
			return ECA_REMOTE_FILTER3;
			break;
		case 4:
			return ECA_REMOTE_FILTER4;
			break;
		case 5:
			return ECA_REMOTE_FILTER5;
			break;
		default:
			break;
	}
	return ECA_INVALID_ACTION; // Invalid for now...
}

ECommandAction TPlayer::getPhaseActionFromRemote(const uint8_t value) {
	switch (value) {
		case 0:
			return ECA_REMOTE_PHASE0;
			break;
		case 1:
			return ECA_REMOTE_PHASE1;
			break;
		default:
			break;
	}
	return ECA_INVALID_ACTION; // Invalid for now...
}

music::ESampleRate TPlayer::getSampleRateFromRemote(const uint8_t value) {
	switch (value) {
		case 0:
			return music::SR96K;
			break;
		case 1:
			return music::SR88K;
			break;
		case 4:
			return music::SR44K;
			break;
		case 5:
			return music::SR48K;
			break;
		case 9:
			return music::SR176K;
			break;
		case 10:
			return music::SR192K;
			break;
		case 20:
			return music::SR352K;
			break;
		case 21:
			return music::SR384K;
			break;
			break;
		default:
			break;
	}
	return music::SR0K; // Invalid for now...
}

bool TPlayer::parseRemoteStatusData(util::TByteBuffer& data) {
	TRemoteParameter parameter;
	int r = terminal.parse(data, parameter);
	if (r > 0) {
		// Get normalized values
		ECommandAction input = getInputActionFromRemote(parameter.input);
		ECommandAction filter = getFilterActionFromRemote(parameter.filter);
		ECommandAction phase = getPhaseActionFromRemote(parameter.phase);
		music::ESampleRate rate = getSampleRateFromRemote(parameter.clock);
		music::ESampleRate canonical = (rate % music::SR48K) == 0 ? music::SR48K : music::SR44K;
		music::ESampleRate hardware = getRemoteRate();

		// Remote sample rate was not yet set to valid value
		// --> take locked rate from status request
		if (hardware == music::SR0K)
			hardware = canonical;

		// Log remote state
		logger(util::csnprintf("[Remote] Remote input  = @ (%)", (int)parameter.input, input));
		logger(util::csnprintf("[Remote] Remote filter = @ (%)", (int)parameter.filter, filter));
		logger(util::csnprintf("[Remote] Remote phase  = @ (%)", (int)parameter.phase, phase));
		logger(util::csnprintf("[Remote] Remote clock  = @ (LCK:%/% CLK:%)", (int)parameter.clock, rate, canonical, hardware));

		// Update visual components from remote state
		music::CAudioValues values;
		sound.getAudioValues(values);
		{
			app::TLockGuard<app::TMutex> lock(componentMtx);
			updateSelectInputButtonsWithNolock(values, input);
			updateSelectFilterButtonsWithNolock(values, filter);
			updateSelectPhaseButtonsWithNolock(values, phase);
			updateActiveRateButtonsWithNolock(values, hardware, canonical);
		}

		return true;
	}
	if (r < 0) {
		logger(util::csnprintf("[Remote] Parser error: % [%]", terminal.strerr(), r));
	}
	return false;
}

int TPlayer::remoteCommandThreadHandler(TSerialThread& sender, util::TByteBuffer& data) {
	sender.writeLog("Thread started.");
	sendRemoteStatusRequest();
	bool exit = false;
	do {
		// Wait for serial command data
		TEventResult ev = terminal.wait(data, sysdat.serial.chunk);
		switch (ev) {
			case EV_SIGNALED:
				// Send synchronous message for data
				// sender.sendMessage(THD_MSG_SYN);
				if (data.size() > 0) {
					std::string text = data.size() != 1 ? "bytes" : "byte";
					std::string s = util::TBinaryConvert::binToHexA(data(), data.size(), false);
					logger(util::csnprintf("[Remote] Received % % from remote device: <%>", data.size(), text, s));
					parseRemoteStatusData(data);
				}
				remoteDeviceMessage = false;
				break;
			case EV_CLOSED:
			case EV_CHANGED:
				// Wait for open/reopen device
				if (!remoteDeviceMessage) {
					logger("[Remote] Wait to open remote communication hardware.");
					remoteDeviceMessage = true;
				}
				util::wait(250);
				break;
			case EV_TERMINATE:
			default:
				// Exit thread...
				logger("[Remote] Terminate remote communication thread.");
				exit = true;
				break;
		}
	} while (!(sender.isTerminating() || exit));
	sender.writeLog("Thread terminated.");
	return EXIT_SUCCESS;
}


void TPlayer::onRemoteCommandMessage(TSerialThread& sender, EThreadMessageType message, util::TByteBuffer& data) {
	// In general this method could be used for all threads of the same type...
	switch (message) {
		case THD_MSG_INIT:
			sender.writeLog("Message THD_MSG_INIT received.");
			break;

		case THD_MSG_SYN:
			// No message handler installed for remote communication thread, just an example here....
			sender.writeLog("Message THD_MSG_SYN received.");
			if (sender.getthd() == remoteCommandThread->getthd()) {
				if (data.size() > 0) {
					std::string text = data.size() != 1 ? "bytes" : "byte";
					std::string s = util::TBinaryConvert::binToHexA(data(), data.size(), false);
					logger(util::csnprintf("[Remote] Received % % from remote device: <%>", data.size(), text, s));
				}
			}
			break;

		case THD_MSG_QUIT:
			sender.writeLog("Message THD_MSG_QUIT received.");
			break;

		default:
			break;
	}
}

void TPlayer::testRemoteParserData() {
	TByteBuffer data10;
	data10.resize(8);
	data10[0] = 0xAA;
	data10[1] = 0x0a;
	data10[2] = 0x4c;
	data10[3] = 0x06;
	data10[4] = 0x00;
	data10[5] = 0x06;
	data10[6] = 0x04;
	data10[7] = 0x20;

	TByteBuffer data11;
	data11.resize(7);
	data11[0] = 0x0a;
	data11[1] = 0x4c;
	data11[2] = 0x06;
	data11[3] = 0x00;
	data11[4] = 0x06;
	data11[5] = 0x04;
	data11[6] = 0x20;

	TByteBuffer data20;
	data20.resize(3);
	data20[0] = 0x00;
	data20[1] = 0x00;
	data20[2] = 0x2a;

	terminal.activate(ERT_DATA);
	parseRemoteStatusData(data10);
	parseRemoteStatusData(data20);

	terminal.activate(ERT_DATA);
	parseRemoteStatusData(data11);
	parseRemoteStatusData(data20);
}


std::string TPlayer::getSongsStatusString(const music::CSongData& song, const std::string& playlist, const std::string& seperator) {
	std::string track = util::cprintf("%02.2d", song.track);
	if (song.tracks > 1)
		track += util::cprintf("/%02.2d", song.tracks);
	if (song.disks > 1)
		track += util::cprintf("/%02.2d", song.disk);
	return playlist + seperator + track + seperator + song.album + seperator + song.artist + seperator + song.title + \
			seperator + music::TSong::timeToStr(song.duration) + seperator + music::TSong::timeToStr(song.played);
}

std::string TPlayer::getSongsStatusString(const music::TSong* song, const std::string& playlist, const std::string& seperator) {
	music::CSongData data;
	getSongProperties(song, data);
	return getSongsStatusString(data, playlist, seperator);
}

void TPlayer::setPlayerStatusLabel(const music::EPlayerState state, const bool streaming) {
	if (music::EPS_PLAY == state && streaming) {
		*wtPlayerStatusCaption = "Streaming";
	} else {
		*wtPlayerStatusCaption = player.statusToStr(state);
	}
	switch (state) {
		case music::EPS_CLOSED:
			// Closed
			*wtPlayerStatusColor = "label-default";
			break;
		case music::EPS_IDLE:
			// Idle
			*wtPlayerStatusColor = "label-default";
			break;
		case music::EPS_PLAY:
			// Playing
			*wtPlayerStatusColor = "label-success";
			break;
		case music::EPS_WAIT:
			// Waiting
			*wtPlayerStatusColor = "label-warning";
			break;
		case music::EPS_PAUSE:
			// Paused
			*wtPlayerStatusColor = "label-primary";
			break;
		case music::EPS_HALT:
			// Paused
			*wtPlayerStatusColor = "label-warning";
			break;
		case music::EPS_STOP:
			// Stopped
			*wtPlayerStatusColor = "label-default";
			break;
		case music::EPS_REOPEN:
			// Interim
			*wtPlayerStatusColor = "label-warning";
			break;
		case music::EPS_ERROR:
			// Failure
			*wtPlayerStatusColor = "label-danger";
			break;
	}
	wtPlayerStatusColor->invalidate();
}

void TPlayer::setScannerStatusLabel(const bool scanning, const size_t size, const size_t erroneous) {
	if (util::assigned(wtScannerStatusColor)) {
		app::TLockGuard<app::TMutex> lock(scnDisplayMtx);
		if (scanning) {
			if (size > 0)
				*wtScannerStatusCaption = util::csnprintf("Database scan in progress, % files scanned...", size);
			else
				*wtScannerStatusCaption = "Database scan in progress, please wait...";
			*wtScannerStatusColor = "label-danger";
		} else {
			if (contentChanged) {
				*wtScannerStatusCaption = "Library needs update...";
				*wtScannerStatusColor = "label-primary";
			} else {
				if (erroneous > 0) {
					if (size > 0)
						*wtScannerStatusCaption = util::csnprintf("% songs in library (<u>% files invalid</u>)", size, erroneous);
					else
						*wtScannerStatusCaption = util::csnprintf("Library is empty (<u>% files invalid</u>)", erroneous);
					*wtScannerStatusColor = "label-warning";
				} else {
					if (size > 0)
						*wtScannerStatusCaption = util::csnprintf("% songs in library", size);
					else
						*wtScannerStatusCaption = "Library is empty";
					*wtScannerStatusColor = "label-default";
				}
			}
		}
		wtScannerStatusColor->invalidate();
	}
}

void TPlayer::setErroneousHeader(const size_t erroneous) {
	if (util::assigned(wtErroneousHeader)) {
		app::TLockGuard<app::TMutex> lock(errDisplayMtx);
		if (erroneous > 0)
			*wtErroneousHeader = util::csnprintf("Erroneous Library Items (% files invalid)", erroneous);
		else
			*wtErroneousHeader = "No erroneous library items";
		wtErroneousHeader->invalidate();
	}
}

bool TPlayer::updateScannerStatus() {
	bool scanning = false;
	size_t songs = 0;
	size_t errors = 0;
	if (isLibraryUpdating()) {
		// Scanner is working
		songs = 0;
		errors = 0;
		scanning = true;
	} else {
		app::TReadWriteGuard<app::TReadWriteLock> lock(libraryLck, RWL_READ);
		songs = library.songs();
		errors = library.erroneous();
	}
	setScannerStatusLabel(scanning, songs, errors);
	return scanning;
}


/*********************************************************************************************
 *
 *    Internet streaming thread methods
 *    and event handler for stream and metadata
 *
 *********************************************************************************************/

void TPlayer::setStreamInfo(const inet::TStreamInfo& info) {
	if (!radio.initialized) {
		music::ECodecType codec = getStreamCodec(info.mime);
		openInetStream(codec);
	}
	setRadioStation(info);
}

void TPlayer::setRadioStation(const inet::TStreamInfo& info) {
	app::TLockGuard<app::TMutex> lock(radioStationMtx);
	radio.counters.incStation();
	radio.info = info;
	radio.info.name = html::THTML::encode(radio.info.name);
	radio.info.description = html::THTML::encode(radio.info.description);
}

void TPlayer::getRadioStation(inet::TStreamInfo& info) {
	app::TLockGuard<app::TMutex> lock(radioStationMtx);
	info = radio.info;
}

void TPlayer::clearRadioStation() {
	app::TLockGuard<app::TMutex> lock(radioStationMtx);
	radio.counters.incStation();
	radio.info.clear();
}

void TPlayer::setRadioStreamInfo(const std::string& title, const size_t streamed) {
	std::string name;
	{
		app::TLockGuard<app::TMutex> lock(radioStationMtx);
		radio.info.streamed = streamed;
		name = radio.info.name;
	}
	setRadioText(title);
	sendBroadcastMessage("STREAM|" + name + "|" + title);
	broadcastStreamUpdate();
}

void TPlayer::updateRadioStreamInfo() {
	bool ok = false;
	if (radio.streaming) {
		app::TLockGuard<app::TMutex> lock(radioStationMtx);
		radio.info.streamed = radio.stream.received();
		ok = true;
	}
	if (ok) {
		app::TLockGuard<app::TMutex> lock(radioTextMtx);
		radio.counters.incText();
	}
	if (ok) {
		// Restart method has is own mutex!
		radio.refresh->restart();
		player.invalidate();
		broadcastPlayerUpdate();
	}
}

void TPlayer::setRadioBitrate(const size_t bitrate) {
	app::TLockGuard<app::TMutex> lock(radioStationMtx);
	radio.info.bitrate = bitrate;
}

void TPlayer::setRadioText(const std::string& text) {
	bool ok = false;
	std::string html;
	if (!text.empty()) {
		app::TLockGuard<app::TMutex> lock(radioTextMtx);

		// Add given text to history only once
		size_t idx = radio.text.find(text);
		if (std::string::npos == idx) {
			radio.text.push_front(text);
			if (radio.text.size() > STREAM_TEXT_HISTORY_SIZE)
				radio.text.shrink(STREAM_TEXT_HISTORY_SIZE);
			radio.text.invalidate();

			// Add license warning...
			if (!streamingAllowed) {
				TTimerDelay delay = radio.limit->getTimeout() / (TTimerDelay)60000;
				std::string warning = util::csnprintf("No streaming license found, playback is stopped after % minutes.", delay);
				idx = radio.text.find(warning);
				if (std::string::npos == idx) {
					radio.text.add(warning);
				}
			}

			// Return stringlist as HTML table object
			html = radio.text.table(0);
		}

		// Set text as current title
		radio.title = html::THTML::encode(text);

		// Set refresh counters
		radio.counters.incText();
		ok = true;
	}
	if (!html.empty()) {
		wtStreamRadioText->setValue(html, true);
	}
	if (ok) {
		radio.refresh->restart();
	}
	player.invalidate();
}

void TPlayer::clearRadioText() {
	{ // Web token have their own mutual exclusion...
		app::TLockGuard<app::TMutex> lock(radioTextMtx);
		radio.title.clear();
		radio.text.clear();
		radio.text.invalidate();
		radio.counters.incText();
	}
	wtStreamRadioText->setValue("No radio text available.", true);
	player.invalidate();
}

bool TPlayer::getRadioText(util::TStringList& text) {
	app::TLockGuard<app::TMutex> lock(radioTextMtx);
	text = radio.text;
	return !text.empty();
}

bool TPlayer::getRadioTitle(std::string& title) {
	app::TLockGuard<app::TMutex> lock(radioTextMtx);
	title = radio.title;
	return !title.empty();
}

void TPlayer::executeInetStreamAction(const ECommandAction action) {
	bool ok = false;
	TCommand command;
	command.action = action;
	command.playlist = STREAM_PLAYLIST_NAME;
	{ // Protect stream property access
		app::TLockGuard<app::TMutex> lock(streamMtx);
		music::PSong song = radio.track->getSong();
		if (util::assigned(song)) {
			command.file = song->getFileHash();
			command.album = song->getAlbumHash();
			ok = true;
		}
	}
	if (ok) {
		queueCommand(command);
		broadcastStreamUpdate();
	}
}

void TPlayer::createInetStreamer() {
	// Set debug for streamer and decoder
	radio.debug = false; //!application.isDaemonized();

	// Create stream objects
	if (!util::assigned(radio.track))
		radio.track = new music::TTrack;
	if (util::assigned(radio.track)) {
		radio.track->setSong(nil);
	}

	// Create strem playlist
	music::PPlaylist pls = playlists.add(STREAM_PLAYLIST_NAME);
	pls->setPermanent(false);

	// Create MP3 stream
	if (!util::assigned(radio.mpeg))
		radio.mpeg = new music::TMP3Song;
	if (util::assigned(radio.track) && util::assigned(radio.mpeg)) {
		radio.mpeg->setDefaultFileProperties("/tmp/Internetstream.mp3", music::SR44K, 16);
		radio.mpeg->setStreamed(true);
		pls->addSong(radio.mpeg, music::EPA_INSERT, true);
	}

	// Create FLAC stream
	if (!util::assigned(radio.flac))
		radio.flac = new music::TFLACSong;
	if (util::assigned(radio.track) && util::assigned(radio.flac)) {
		radio.flac->setDefaultFileProperties("/tmp/Internetstream.flac", music::SR44K, 16);
		radio.flac->setStreamed(true);
		pls->addSong(radio.flac, music::EPA_INSERT, true);
	}

	if (!util::assigned(radio.timeout)) {
		radio.timeout = application.addTimeout("StreamingCommandDelay", 750, true);
		radio.timeout->adjust(2000); // Set minimal timer delay (caused by too low default value in prior versions!)
	}
	if (!util::assigned(radio.limit)) {
		radio.limit = application.addTimeout("StreamingLimitDelay", 300000);
		radio.limit->bindEventHandler(&app::TPlayer::onStreamTimeout, this);
	}
	if (!util::assigned(radio.refresh)) {
		radio.refresh = application.addTimeout("StreamingRefreshDelay", 30000);
		radio.refresh->bindEventHandler(&app::TPlayer::onStreamRefresh, this);
	}

	// Create streaming thread
	if (!radio.started) {
		if (createJoinableThread(radio.thread, streamThreadDispatcher, this, "Inet-Radio")) {
			radio.started = true;

			// Set properties
			radio.stream.setDebug(debug); // !application.isDaemonized());
			radio.stream.setTimeout(STREAM_TIMEOUT + 1);
			radio.stream.setChunkSize(STREAM_BUFFER_SIZE);

			// Bind stream events
			radio.stream.bindStreamDataEvent(&app::TPlayer::onInetStreamData, this);
			radio.stream.bindStreamInfoEvent(&app::TPlayer::onInetStreamInfo, this);
			radio.stream.bindStreamTitleEvent(&app::TPlayer::onInetStreamTitle, this);
			radio.stream.bindStreamErrorEvent(&app::TPlayer::onInetStreamError, this);

		} else {
			throw util::sys_error("TPlayer::createInetStreamer() Create streaming thread failed.");
		}
	}
}

void TPlayer::destroyInetStreamer() {
	if (radio.started) {
		radio.started = false;
		radio.stream.terminate();
		notifyStreamThread();
		int retVal = terminateThread(radio.thread);
		radio.running = false;
		if (util::checkFailed(retVal))
			logger("[Streamer] Waiting for streaming thread failed.");
	}
}

bool TPlayer::delayInetStream() {
	app::TLockGuard<app::TMutex> lock(delayMtx);
	bool signaled = false;
	if (radio.timeout->isSignaled()) {
		signaled = true;
	}
	radio.timeout->restart();
	return signaled;
}

bool TPlayer::startInetStream(const std::string& hash) {
	bool ok = false;
	bool stopped = false;
	bool notify = false;
	if (isStreamable() && !hash.empty()) {
		app::TLockGuard<app::TMutex> lock(streamMtx);
		if (!radio.streaming) {
			radio.error = false;
			radio.streaming = true;
			radio.hash = hash;
			notify = true;
		} else {
			if (radio.hash != hash) {
				clearInetStreamWithNolock();
				radio.stream.terminate();
				radio.terminated = true;
				radio.next = hash;
				stopped = true;
			}
		}
		ok = true;
	}
	if (ok) {
		if (stopped) {
			clearRadioStation();
		}
		if (!streamingAllowed) {
			radio.limit->restart();
		}
		if (notify) {
			radio.event.notify();
		}
	}
	return ok;
}

bool TPlayer::restartInetStream() {
	bool ok = false;
	bool notify = false;
	if (isStreamable()) {
		app::TLockGuard<app::TMutex> lock(streamMtx);
		if (!radio.last.empty()) {
			if (!radio.streaming) {
				radio.error = false;
				radio.streaming = true;
				radio.hash = radio.last;
				if (!streamingAllowed) {
					radio.limit->restart();
				}
				notify = true;
				ok = true;
			}
		}
	}
	if (notify) {
		radio.event.notify();
	}
	return ok;
}

bool TPlayer::stopInetStream() {
	bool ok = false;
	{
		app::TLockGuard<app::TMutex> lock(streamMtx);
		if (radio.streaming) {
			clearInetStreamWithNolock();
			radio.stream.terminate();
			radio.terminated = true;
			radio.streaming = false;
			radio.next.clear();
			ok = true;
		}
	}
	if (ok) {
		clearRadioStation();
	}
	return ok;
}

void TPlayer::clearInetStream() {
	app::TLockGuard<app::TMutex> lock(streamMtx);
	clearInetStreamWithNolock();
}

void TPlayer::clearInetStreamWithNolock() {
	if (!radio.hash.empty()) {
		radio.last = radio.hash;
	}
	radio.hash.clear();
}

void TPlayer::finalizeInetStream(const std::string& URL) {
	logger(util::csnprintf("[Streamer] Stopped internet stream for URL $", util::strToStr(URL, "none")));
	music::PSong song = radio.track->getSong();
	if (util::assigned(song)) {
		music::PAudioStream decoder = song->getStream();
		if (util::assigned(decoder)) {
			decoder->close();
		}
		if (util::assigned(radio.buffer)) {
			player.resetBuffer(radio.buffer);
		}
		player.resetBuffers(song);
		song->reset();
	}
	radio.block.release();
	clearRadioStation();
	clearRadioText();
	clearInetStream();
}

bool TPlayer::prepareInetStream() {
	radio.error = false;
	radio.buffered = false;
	radio.terminated = false;
	radio.initialized = false;
	radio.streaming = false;
	radio.playing = false;
	radio.threshhold = app::nsizet;
	radio.consumed = 0;
	radio.written = 0;

	// Clear counters and text properties
	clearRadioStation();
	clearRadioText();

	// Reset song statistics
	music::PSong song = radio.track->getSong();
	if (util::assigned(song)) {
		song->reset();
	}

	// Get free buffer for streaming
	radio.buffer = player.getNextEmptyBuffer(radio.track);
	if (!util::assigned(radio.buffer)) {
		player.resetBuffers();
		radio.buffer = player.getNextEmptyBuffer(radio.track);
	}
	if (util::assigned(radio.buffer)) {
		if (radio.buffer->validWriter()) {
			logger("[Streamer] Prepared first buffer <" + std::to_string((size_u)radio.buffer->getKey()) + "> for internet streaming.");
			player.operateBuffers(radio.buffer, radio.track, music::EBS_BUFFERING, music::EBL_BUFFERING);
		} else {
			logger("[Streamer] Not enough space in buffers to prepare stream.");
		}
	} else {
		logger("[Streamer] Preparing decoder for internet stream failed: No empty buffer found to decode data.");
	}

	// Premature stream termination on error
	if (!util::assigned(radio.buffer)) {
		finalizeInetStream("");
		stopInetStream();
		return false;
	}

	return true;
}

bool TPlayer::openInetStream(const music::ECodecType codec) {
	bool result = false;
	if (!radio.initialized) {
		std::string decoder;
		music::EContainerType container = music::ECT_NATIVE;

		// Get stream song
		music::PSong song = nil;
		switch (codec) {
			case music::EFT_FLAC:
				song = radio.flac;
				container = music::ECT_FLAC;
				decoder = "FLAC";
				break;
			case music::EFT_OGG:
				song = radio.flac;
				container = music::ECT_OGG;
				decoder = "FLAC";
				break;
			case music::EFT_MP3:
				song = radio.mpeg;
				decoder = "MP3";
				break;
			default:
				break;
		}

		// Setup stream decoder for given song
		if (util::assigned(song)) {
			radio.track->setSong(song);
			song->reset();

			// Open decoder
			music::PAudioStream decoder = song->getStream();
			if (util::assigned(decoder)) {

				// Setup block buffer
				if (decoder->getBlockSize() > 0) {
					// Allocate block size + reserve
					radio.block.reserve(3 * decoder->getBlockSize() / 2);
				} else {
					// Release buffer but do not free reserved buffer size
					radio.block.release();
				}

				// Setup decoder
				music::TDecoderParams params;
				params.debug = radio.debug;
				params.paranoid = true;
				params.gapless = false;
				params.container = container;
				params.sampleRate = 0; // Use native stream sample rate
				params.bitsPerSample = song->getBitsPerSample();
				params.bytesPerSample = song->getBitsPerSample() * 8;
				if (decoder->open(params)) {
					result = true;
				} else {
					logger("[Streamer] Unable to open decoder for internet streaming.");
				}

			} else {
				logger("[Streamer] No decoder to prepare internet streaming.");
			}

		} else {
			logger("[Streamer] No song present for given stream codec.");
		}

		// Terminate streaming on error
		if (result) {
			radio.initialized = true;
			logger("[Streamer] Opened new " + decoder + " decoder.");
		} else {
			finalizeInetStream("");
			stopInetStream();
		}

	}

	return result;
}

void TPlayer::resetInetStream() {
	radio.written = 0;
	radio.consumed = 0;
	radio.event.flush();
}

music::ECodecType TPlayer::getStreamCodec(const std::string& mime) {
	if (util::strstr(mime, "mpeg")) {
		logger("[Streamer] Detected MP3 stream from mime type <" + mime + ">");
		return music::EFT_MP3;
	}
	if (util::strstr(mime, "mp3")) {
		logger("[Streamer] Detected MP3 stream from mime type <" + mime + ">");
		return music::EFT_MP3;
	}
	if (util::strstr(mime, "flac")) {
		logger("[Streamer] Detected native FLAC stream from mime type <" + mime + ">");
		return music::EFT_FLAC;
	}
	if (util::strstr(mime, "ogg")) {
		logger("[Streamer] Detected FLAC stream in OGG container from mime type <" + mime + ">");
		return music::EFT_OGG;
	}
	logger("[Streamer] Unknown mime type <" + mime + ">");
	return music::EFT_UNKNOWN;
}

bool TPlayer::getStreamHash(std::string& hash) {
	hash.clear();
	{ // Get current hash value
		app::TLockGuard<app::TMutex> lock(streamMtx);
		hash = radio.hash;
	}
	return !hash.empty();
}

bool TPlayer::getStreamName(std::string& name) {
	std::string hash;
	name.clear();
	{ // Get current hash value
		app::TLockGuard<app::TMutex> lock(streamMtx);
		hash = radio.hash;
	}
	{ // Get URL from station list
		app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_READ);
		radio::PRadioStream stream = stations.find(hash);
		if (util::assigned(stream)) {
			name = stream->display.name;
		}
	}
	return !name.empty();
}

bool TPlayer::getStreamURL(std::string& URL) {
	std::string hash;
	URL.clear();
	{ // Get current hash value
		app::TLockGuard<app::TMutex> lock(streamMtx);
		hash = radio.hash;
	}
	{ // Get URL from station list
		app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_READ);
		radio::PRadioStream stream = stations.find(hash);
		if (util::assigned(stream)) {
			URL = stream->values.url;
		}
	}
	return !URL.empty();
}

bool TPlayer::getStreamIndex(size_t& index) {
	std::string hash;
	{ // Get current hash value
		app::TLockGuard<app::TMutex> lock(streamMtx);
		hash = radio.hash;
	}
	{ // Get URL from station list
		app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_READ);
		radio::PRadioStream stream = stations.find(hash);
		if (util::assigned(stream)) {
			index = stream->index;
			return true;
		}
	}
	index = std::string::npos;
	return false;
}

bool TPlayer::getStreamProperties(std::string& name, std::string& url, radio::EMetadataOrder& order) {
	std::string hash;
	name.clear();
	url.clear();
	order = radio::EMO_DEFAULT;
	{ // Get current hash value
		app::TLockGuard<app::TMutex> lock(streamMtx);
		hash = radio.hash;
	}
	{ // Get URL from station list
		app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_READ);
		radio::PRadioStream stream = stations.find(hash);
		if (util::assigned(stream)) {
			name = stream->display.name;
			url = stream->display.url;
			order = stream->values.order;
		}
	}
	return !name.empty();
}

bool TPlayer::getAndClearNextHash(std::string& hash) {
	hash.clear();
	{ // Get current hash value
		app::TLockGuard<app::TMutex> lock(streamMtx);
		hash = radio.next;
		radio.next.clear();
	}
	return !hash.empty();
}

void TPlayer::undoStreamAction() {
	app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_WRITE);
	stations.undo();
	logger("[Streamer] Executed undo action.");
}

void TPlayer::clearStreamUndoList() {
	app::TReadWriteGuard<app::TReadWriteLock> lock(stationsLck, RWL_WRITE);
	stations.clearUndoList();
	logger("[Streamer] Cleared undo action list.");
}

void TPlayer::notifyStreamThread() {
	radio.event.notify();
}

int TPlayer::streamThreadHandler() {
	radio.started = radio.running = true;
	logger("[Streamer] Internet streaming thread started.");
	while (true) {
		if (!streamThreadMethod())
			break;
	}
	radio.running = false;
	logger("[Streamer] Internet streaming thread terminated.");
	return EXIT_SUCCESS;
}

bool TPlayer::streamThreadMethod() {
	bool exit = false;
	try {
		do {
			TEventResult ev = radio.event.wait();
			if (!isTerminated()) {
				if (EV_SIGNALED == ev) {
					if (prepareInetStream()) {
						util::TTimePart bailout = STREAM_TIMEOUT + util::now();
						size_t retry = 0;
						std::string url;
						if (getStreamURL(url)) {
							do {

								// Start ALSA playback on stream buffer
								radio.streaming = true;
								util::TBooleanGuard<bool> bg(radio.streaming);

								// Start refresh cycle timer
								radio.refresh->restart();

								// Receive and decode data stream
								if (retry > 0) {
									logger(util::csnprintf("[Streamer] Reconnect no. % internet stream for URL $", retry, url));
									util::wait(250);
								} else {
									logger(util::csnprintf("[Streamer] Start internet stream for URL $", url));
								}
								if (!radio.stream.receive(url)) {
									logger("[Streamer] " + radio.stream.errmsg());
									radio.buffered = false;
								} else {
									size_t ratio = (10 * radio.written / radio.stream.received() + 5) / 10;
									logger(util::csnprintf("[Streamer] Received % and decoded % (1:%) for URL $", util::sizeToStr(radio.stream.received()), util::sizeToStr(radio.written), ratio, radio.stream.getURL()));
								}

								// Check for timout and retry count
								util::TTimePart time = util::now();
								if (time > bailout) {
									// Allow retries on successful streaming for 5 seconds
									retry = 0;
									bailout = 5 + time;
								} else {
									++retry;
								}

								// Clear events
								resetInetStream();

							} while (!isTerminated() && !radio.error && !radio.stream.isTerminated() && retry < 5 && getStreamURL(url));

							// Stop ALSA playback
							if (radio.playing) {
								radio.playing = false;
								if (radio.error) {
									executeInetStreamAction(ECA_STREAM_ABORT);
									logger("[Streamer] Abort internet stream playback on error.");
								} else {
									executeInetStreamAction(ECA_STREAM_STOP);
									logger("[Streamer] Stopped internet stream playback.");
								}
							}

							// Leave thread?
							if (isTerminated()) {
								exit = true;
							}
						}

						// Reset everything
						finalizeInetStream(url);
					}
				}
			} else {
				exit = true;
			}
		} while (!exit);

	} catch (const std::exception& e) {
		std::string sExcept = e.what();
		std::string sText = "[Streamer] Exception in TPlayer::streamThreadMethod() " + sExcept;
		application.getExceptionLogger().write(sText);
		finalizeInetStream("");
		exit = true;
	} catch (...) {
		std::string sText = "[Streamer] Unknown exception in TPlayer::streamThreadMethod()";
		application.getExceptionLogger().write(sText);
		finalizeInetStream("");
		exit = true;
	}

	return !(isTerminated() || exit);
}

void TPlayer::onInetStreamData(const inet::TInetStream& sender, const void *const data, const size_t size) {
	// logger(util::csnprintf("[Streamer] Received % bytes for URL $", size, sender.getURL()));
	bool ok = false;
	if (!radio.terminated) {
		if (size > 0) {
			music::PSong song = radio.track->getSong();
			if (util::assigned(song)) {
				music::PAudioStream decoder = song->getStream();
				if (util::assigned(decoder)) {
					if (util::assigned(radio.buffer)) {
						try {
							size_t written = 0;
							size_t consumed = 0;
							size_t buffered = size;
							size_t block = decoder->getBlockSize();
							const music::TSample* buffer = (const music::TSample*)data;

							// Check if decoder specific block size limit reached
							bool decode = true;
							if (block > 0) {
								if (radio.debug) std::cout << app::red << "====>>> TPlayer::onInetStreamData() Block before append (" << radio.block.size() << " bytes) and new data (" << buffered << " bytes) <<<====" << app::reset << std::endl;
								radio.block.append(buffer, buffered);
								if (radio.block.size() < block) {
									decode = false;
								} else {
									buffer = radio.block.data();
									buffered = radio.block.size();
									if (radio.debug) std::cout << app::red << "====>>> TPlayer::onInetStreamData() Block after append (" << radio.block.size() << " bytes) and new data (" << buffered << " bytes) <<<====" << app::reset << std::endl;
								}
							}
							if (radio.debug) std::cout << app::red << "====>>> TPlayer::onInetStreamData() Block before decode (" << radio.block.size() << " bytes) decode = " << decode << " <<<====" << app::reset << std::endl;

							// Is buffered data ready to decode?
							if (decode) {

								// Decode given data
								ok = decoder->update(buffer, buffered, radio.buffer, written, consumed);
								if (ok) {
									// Check if some data was decoded...
									if (written > 0) {
										music::TDecoderParams params;
										bool valid = false;

										// Add written bytes for over all used buffers for given file
										radio.consumed += consumed;
										radio.written += written;
										song->addWritten(written);

										// Get threshhold for current stream
										if (app::nsizet == radio.threshhold) {
											if (decoder->getRunningValues(params)) {
												if (params.valid && params.sampleRate > 0 && params.channels > 0 && params.bitsPerSample > 0) {
													// Threshhold ==> 44100 Samples/sec * 2 channels * 2 Byte (16 Bit / 8 Bit) * (2 * period time)
													size_t buffertime = 2 * sound.getPeriodTime();
													radio.threshhold = std::max(params.sampleRate * params.channels * (params.bitsPerSample / 8) * buffertime / 1000, music::MP3_OUTPUT_CHUNK_SIZE);
													logger(util::csnprintf("[Streamer] Set buffer threshhold to % bytes (stream buffer time is % milliseconds)", radio.threshhold, buffertime));
													valid = true;
												}
											}
										}

										// Enough bytes buffered to start playback?
										if (!radio.buffered && radio.threshhold != app::nsizet && radio.written > radio.threshhold) {

											// Get stream properties and assign them to song properties
											ok = true;
											if (!valid) {
												ok = decoder->getRunningValues(params);
												if (!ok)
													logger("[Streamer] Failed to retreive stream parameters.");
											}
											if (ok) {
												ok = params.valid && params.sampleRate > 0 && params.channels > 0 && params.bitsPerSample > 0;
												if (!ok)
													logger("[Streamer] Invalid stream parameters.");
											}
											if (ok) {

												// Check sample rate
												music::ESampleRate samplerate = (music::ESampleRate)params.sampleRate;
												if (util::isMemberOf(samplerate, music::SR44K,music::SR48K,music::SR88K,music::SR96K)) {
													if ((int)params.sampleRate != song->getSampleRate()) {
														logger(util::csnprintf("[Streamer] Set native stream sample rate to % Samples/sec", params.sampleRate));
													} else {
														logger(util::csnprintf("[Streamer] Native sample rate is % Samples/sec", params.sampleRate));
													}
												} else {
													logger(util::csnprintf("[Streamer] Invalid native stream sample rate % Samples/sec", params.sampleRate));
													ok = false;
												}

												// Check resolution (bits per sample)
												if (params.bitsPerSample > 0) {
													if (params.bitsPerSample != song->getBitsPerSample()) {
														logger(util::csnprintf("[Streamer] Set bit depth to native stream resolution (% bits)", params.bitsPerSample));
													} else {
														logger(util::csnprintf("[Streamer] Native stream resolution is % bits", params.bitsPerSample));
													}
												} else {
													logger("[Streamer] Invalid native stream resolution");
													ok = false;
												}

												// Update song properties with stream values
												if (ok) {
													size_t bits = params.bitsPerSample;
													song->updateStreamProperties(samplerate, bits);
													setRadioBitrate(params.bitRate);
												}

											}

											// Start playback
											if (ok) {
												radio.buffered = true;
												player.operateBuffers(radio.buffer, radio.track, music::EBS_BUFFERED, music::EBL_BUFFERED);
												logger("[Streamer] " + util::sizeToStr(radio.written, 1, util::VD_BINARY) + " written, playback can be started...");
												if (!radio.playing) {
													radio.playing = true;
													executeInetStreamAction(ECA_STREAM_PLAY);
													logger("[Streamer] Issued internet stream playback command.");
												}
												notifyEvent("[Streamer] Playback can be startet");
											}
										}

										// Check for sufficient empty space in write buffer
										// --> Next read data chunk will exhaust buffer size!
										if ((radio.buffer->getWritten() + 2 * music::MP3_OUTPUT_CHUNK_SIZE + 1) >= radio.buffer->size()) {

											// Current buffer is loaded
											player.operateBuffers(radio.buffer, radio.track, music::EBS_LOADED, music::EBL_FULL);

											// Switch to next empty buffer
											radio.buffer = player.getNextEmptyBuffer();
											if (util::assigned(radio.buffer)) {

												// Continue decoding the stream into next buffer
												music::EBufferState state = radio.buffered ? music::EBS_BUFFERED : music::EBS_CONTINUE;
												music::EBufferLevel level = radio.buffered ? music::EBL_BUFFERED : music::EBL_BUFFERING;
												std::string sState = player.bufferToStr(state);
												player.operateBuffers(radio.buffer, radio.track, state, level);
												logger("[Streamer] Use next buffer <" + std::to_string((size_u)radio.buffer->getKey()) + "> for stream data, set buffer to \"" + sState + "\"");

											} else {
												logger("[Streamer] Decoding internet stream failed: No empty buffer found to decode data.");
												ok = false;
											}

										}

									} // if (written > 0) ...

									// Check how much bytes of data buffer were decoded
									if (consumed > 0) {
										if (consumed < radio.block.size()) {
											size_t overhead = radio.block.size() - consumed;
											const music::TSample* p = radio.block.data() + consumed;
											if (overhead > 0) {
												TDecoderBuffer b(p, overhead);
												radio.block.release();
												if (radio.debug) std::cout << app::red << "====>>> TPlayer::onInetStreamData() Block after release (" << radio.block.size() << " bytes) <<<====" << app::reset << std::endl;
												radio.block.append(b);
												if (radio.debug) std::cout << app::red << "====>>> TPlayer::onInetStreamData() Block after append (" << radio.block.size() << " bytes) <<<====" << app::reset << std::endl;
											}
											if (radio.debug) std::cout << app::red << "====>>> TPlayer::onInetStreamData() Overhead copied (" << overhead << " bytes) <<<====" << app::reset << std::endl;
										} else {
											radio.block.release();
											if (radio.debug) std::cout << app::red << "====>>> TPlayer::onInetStreamData() Block after clear (" << radio.block.size() << " bytes) <<<====" << app::reset << std::endl;
										}
									}

								} else { // if (decoder->update(buffer, buffered, radio.buffer, read)) ...

									// Input stream/decoder is on error
									logger(util::csnprintf("[Streamer] Decoder update for stream failed (% bytes decoded)", radio.written));
									if (util::isClass<music::TFLACDecoder>(decoder)) {
										music::TFLACDecoder* flac = util::asClass<music::TFLACDecoder>(decoder);
										if (flac->hasError()) {
											logger(util::csnprintf("[Streamer] Error FLAC decoder $ [%]", flac->errmsg(), flac->errorcode()));
										} else {
											logger(util::csnprintf("[Streamer] Invalid FLAC decoder state $ [%]", flac->statusmsg(), flac->statuscode()));
										}
									}
								}

							} else { // if (decode) ...
								// Result is OK for now
								ok = true;
							}

						} catch (const std::exception& e)	{
							string sExcept = e.what();
							logger("[Streamer] Exception on stream update.");
						} catch (...)	{
							logger("[Streamer] Unknown exception on stream update.");
						}
					} else {
						logger("[Streamer] Decode stream failed: No buffer to decode data.");
					}
				} else {
					logger("[Streamer] Decoder for internet stream missing.");
				}

				// Stop stream on error
				if (!ok) {
					radio.error = true;
					stopInetStream();
				}

			} else {
				logger("[Streamer] Error: No valid song given.");
			}
		} else {
			logger("[Streamer] Error: No stream data received.");
		}
	} else {
		logger("[Streamer] Stream already terminated.");
	}
}

void TPlayer::onInetStreamInfo(const inet::TInetStream& sender, const inet::TStreamInfo& info) {
	logger(util::csnprintf("[Streamer] Station $ received for stream $", info.name, info.stream));
	logger(util::csnprintf("[Streamer] Bitrate % kBit/sec for stream $", info.bitrate, info.stream));
	setStreamInfo(info);
}

void TPlayer::onInetStreamTitle(const inet::TInetStream& sender, const std::string& title) {
	size_t received = sender.received();
	size_t ratio = (10 * radio.written / received + 5) / 10;
	logger(util::csnprintf("[Streamer] Received % and decoded % (1:%) for URL $", util::sizeToStr(received), util::sizeToStr(radio.written), ratio, sender.getURL()));
	logger(util::csnprintf("[Streamer] Title $ received for URL $", title, sender.getURL()));
	setRadioStreamInfo(title, received);
}

void TPlayer::onInetStreamError(const inet::TInetStream& sender, const int error, const std::string& text, bool& terminate) {
	logger(util::csnprintf("[Streamer] % (%)", text, error));
}

void TPlayer::onStreamTimeout() {
	// Streaming time limit reached
	if (!streamingAllowed) {
		stopInetStream();
		logger("[Streamer] Streaming time limit reached, application not licensed...");
	}
}

void TPlayer::onStreamRefresh() {
	// Update at least received bytes
	updateRadioStreamInfo();
}

void TPlayer::onStationsMenuClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	if (debug) aout << app::green << "TPlayer::onStationsMenuClick(" << key << ":" << util::ellipsis(value) << ")" << app::reset << endl;

	// Get action from given parameter
	std::string command = params["Action"].asString();
	ECommandAction action = actions[command];

	// Check for input delay
	if (ECA_STREAM_UNDO != action) {
		if (!delayInetStream()) {
			application.getApplicationLogger().write("[Event] Menu action for [" + value + "] ignored by input delay");
			return;
		}
	}
	
	// Execute button click action
	application.getApplicationLogger().write("[Event] Menu action for [" + key + "] = [" + value + "]");

	// Get parameters
	std::string hash = params["Hash"].asString();

	// Excute context menu action for given URL
	switch (action) {
		case ECA_STREAM_PLAY:
			startInetStream(hash);
			break;

		case ECA_STREAM_UNDO:
			undoStreamAction();
			break;

		case ECA_STREAM_STOP:
		case ECA_STREAM_PAUSE:
		case ECA_STREAM_ABORT:
			stopInetStream();
			break;

		default:
			break;
	}
}

void TPlayer::onStationsButtonClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	if (debug) aout << app::green << "TPlayer::onStationsButtonClick(" << key << ":" << util::ellipsis(value) << ")" << app::reset << endl;

	// Get action from given value
	ECommandAction action = actions[value];

	// Check for input delay
	if (ECA_STREAM_UNDO != action) {
		if (!delayInetStream()) {
			application.getApplicationLogger().write("[Event] Click action for [" + value + "] ignored by input delay");
			return;
		}
	}

	// Execute button click action
	application.getApplicationLogger().write("[Event] Click action for [" + key + "] = [" + value + "]");

	// Excute button click action
	std::string url;
	switch (action) {
		case ECA_STREAM_PLAY:
			restartInetStream();
			break;

		case ECA_STREAM_UNDO:
			undoStreamAction();
			break;

		case ECA_STREAM_STOP:
		case ECA_STREAM_PAUSE:
		case ECA_STREAM_ABORT:
			stopInetStream();
			break;

		default:
			break;
	}
}

