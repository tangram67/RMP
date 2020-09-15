/*
 * main.h
 *
 *  Created on: 17.08.2014
 *      Author: Dirk Brinkmeier
 */
#ifndef MAIN_H_
#define MAIN_H_

#include "../inc/classes.h"
#include "../inc/webtoken.h"
#include "../inc/audiofile.h"
#include "../inc/inetstream.h"
#include "../inc/audiobuffer.h"
#include "../inc/stringutils.h"
#include "../inc/component.h"
#include "../inc/sockets.h"
#include "../inc/timeout.h"
#include "../inc/bitmap.h"
#include "../inc/image.h"
#include "../inc/json.h"
#include "../inc/alsa.h"
#include "../inc/udev.h"
#include "../inc/flac.h"
#include "../inc/mp3.h"
#include "../inc/ipc.h"
#include "controltypes.h"
#include "musicplayer.h"
#include "streamlist.h"
#include "library.h"
#include "control.h"
#include "sound.h"

namespace app {

STATIC_CONST char DEFAULT_HASH_TAG[]    = "--------------------------------";

STATIC_CONST char NOCOVER_REST_URL[]    = "/rest/thumbnails/NOCOVER--------------------------400.jpg";
STATIC_CONST char NOCOVER_PREVIEW_URL[] = "/rest/thumbnails/NOCOVER--------------------------600.jpg";
STATIC_CONST char SPECIAL_REST_URL[]    = "/rest/thumbnails/00000000000000000000000000000001-400.jpg";
STATIC_CONST char SPECIAL_PREVIEW_URL[] = "/rest/thumbnails/00000000000000000000000000000001-600.jpg";

STATIC_CONST char EXPORT_REST_URL[]     = "/rest/export.m3u";
STATIC_CONST char STATIONS_REST_URL[]   = "/rest/stations.m3u";

STATIC_CONST util::TTimePart FORCED_REOPEN_DELAY = 3600 * 3;
STATIC_CONST app::TTimerDelay DEFAULT_HOTPLUG_DELAY = 1000;
STATIC_CONST app::TTimerDelay DEFAULT_FILE_WATCH_DELAY = 750;
STATIC_CONST app::TTimerDelay DEFAULT_REMOTE_TIMEOUT = 1000;
STATIC_CONST app::TBaudRate REMOTE_BAUD_RATE = 4800;

STATIC_CONST char STREAM_PLAYLIST_NAME[] = "Radiostations";
STATIC_CONST size_t STREAM_TEXT_HISTORY_SIZE = 5;
STATIC_CONST size_t STREAM_TIMEOUT = 5;
STATIC_CONST size_t STREAM_BUFFER_SIZE = 1390; // Lower than PPPoE MTU 1492

STATIC_CONST size_t MP3_BUFFERING_THRESHOLD = 44100 * 2 * 2 * 3; // = 44100 Samples/sec * 2 channels * 2 Byte (16 Bit / 8 Bit) * 3 sec

#ifdef STL_HAS_TEMPLATE_ALIAS

using TFileCacheMap = std::map<std::string, util::PFile>;
using TFileCacheItem = std::pair<std::string, util::PFile>;
using TFileCacheInsert = std::pair<TFileCacheMap::iterator, bool>;
using TMediaNames = std::map<std::string, music::EMediaType>;
using TDomainNames = std::map<std::string, music::EFilterDomain>;
using TCommandBuffer = util::TDataBuffer<TCommandData>;
using TClientList = std::map<app::THandle, inet::TServerSocket*>;
using TDecoderBuffer = util::TDataBuffer<music::TSample>;

#else

typedef std::map<std::string, util::PFile> TFileCacheMap;
typedef std::pair<std::string, util::PFile> TFileCacheItem;
typedef std::pair<TFileCacheMap::iterator, bool> TFileCacheInsert;
typedef std::map<std::string, music::EMediaType> TMediaNames;
typedef std::map<std::string, music::EFilterDomain> TDomainNames;
typedef util::TCommandBuffer<TCommandData> TBuffer;
typedef std::map<app::THandle, inet::TServerSocket*> TClientList;
typedef util::TDataBuffer<music::TSample> TDecoderBuffer;

#endif

typedef struct CDecoderState {
	int state;
	int saved;
	music::PTrack track;
	music::PAudioStream stream;
	music::PAudioBuffer buffer;
	size_t hardwareIdx;
	size_t softwareIdx;
	size_t total;
	bool buffered;
	bool busy;
	std::string message;
	util::TDateTime time;
	app::PTimeout timeout;
	app::TTimerDelay period;
	app::TTimerDelay duration;

	void clear() {
		if (util::assigned(stream)) {
			stream->close();
		}
		state = 0;
		saved = 0;
		track = nil;
		stream = nil;
		buffer = nil;
		total = 0;
		busy = false;
		buffered = false;
	}

	void reset() {
		hardwareIdx = app::nsizet;
		softwareIdx = app::nsizet;
		clear();
	}

	void prime() {
		stream = nil;
		buffer = nil;
		timeout = nil;
		period = (app::TTimerDelay)0;
		duration = (app::TTimerDelay)0;
		clear();
	}

	CDecoderState() {
		prime();
	};

} TDecoderState;


typedef struct CRemoteState {
	int state;
	TCommandBuffer data;
	TCommandBuffer page0;
	TCommandBuffer page1;
	TCommandBuffer page2;
	TCommandBuffer page3;

	void reset0() {
		page0.fillchar(0);
	}

	void reset1() {
		page1.fillchar(0);
	}

	void reset2() {
		page2.fillchar(0);
	}

	void reset3() {
		page3.fillchar(0);
	}

	void reset() {
		reset0();
		reset1();
		reset2();
		reset3();
		data.clear();
	}

	void clear() {
		state = 0;
		reset();
	}

	void prime() {
		data.reserve(32, false);
		page0.resize(5, false);
		page1.resize(5, false);
		page2.resize(5, false);
		page3.resize(5, false);
		clear();
	}

	CRemoteState() {
		prime();
	};

} TRemoteState;


typedef struct CDeviceState {
	music::EPlayerState state;
	music::EPlayerCommand command;
	std::string device;
	music::PSong song;
	std::string playlist;
	app::PTimeout delay;
	app::PTimeout remote;
	util::TTimePart timeout;

	void prime() {
		state = music::EPS_CLOSED;
		command = music::EPP_DEFAULT;
		timeout = util::now();
		remote = nil;
		delay = nil;
		song = nil;
	}

	void clear() {
		prime();
		device.clear();
		playlist.clear();
	}

	CDeviceState() {
		prime();
	};

} TDeviceState;


typedef struct CPlayerState {
	int state;
	size_t index;
	TDeviceState hardware;
	TDeviceState software;
	TDeviceState playlist;
	bool busy;

	void prime() {
		state = 0;
		index = 0; //app::nsizet;
		busy = false;
	}

	void clear() {
		prime();
		hardware.clear();
		software.clear();
		playlist.clear();
	}

	CPlayerState() {
		prime();
	};

} TPlayerState;


typedef struct CCylcleState {
	app::PTimeout timeout;
	util::TDateTime cyclic;
	bool violation;
	bool warning;

	void clear() {
		warning = false;
		violation = false;
	}

	CCylcleState() {
		clear();
	};

} TCylcleState;


typedef struct CCommandState {
	PCommand command;
	std::string file;
	std::string album;
	std::string playlist;
	music::PSong song;

	void prime() {
		command = nil;
		song = nil;
	}
	void clear() {
		file.clear();
		album.clear();
		playlist.clear();
		prime();
	}

	CCommandState() {
		prime();
	};

} TCommandState;


typedef struct CPlayerMode {
	bool random;
	bool repeat;
	bool single;
	bool halt;
	bool disk;
	bool direct;

	void clear() {
		random = false;
		repeat = false;
		single = false;
		halt = false;
		disk = false;
		direct = false;
	}

	CPlayerMode() {
		clear();
	};

} TPlayerMode;


typedef struct CPlayerParameter {
	size_t maxBufferCount;
	size_t preBufferCount;
	size_t useBufferCount;

	void clear() {
		maxBufferCount = 0;
		preBufferCount = 0;
		useBufferCount = 0;
	}

	CPlayerParameter() {
		clear();
	};

} TPlayerParameter;


typedef struct CGlobalState {
	TDecoderState decoder;
	TPlayerState player;
	TCylcleState tasks;
	TCommandState interface;
	TRemoteState remote;
	TPlayerMode shuffle;
	TPlayerParameter params;

	void clear() {
		decoder.clear();
		player.clear();
		tasks.clear();
		interface.clear();
		remote.clear();
		params.clear();
	}

} TGlobalState;


typedef struct CStreamCounters {
	size_t text;
	size_t station;

	void prime() {
		text = 0;
		station = 0;
	}

	void clear() {
		prime();
	}

	void incText() {
		++text;
		if (text > 0xFFFF)
			text = 1;
	}

	void incStation() {
		++station;
		if (station > 0xFFFF)
			station = 1;
	}

	CStreamCounters() {
		prime();
	};
} TStreamCounters;



typedef struct CRadioStream {
	bool debug;
	bool error;
	bool started;
	bool running;
	bool streaming;
	bool playing;
	bool buffered;
	bool terminated;
	bool initialized;
	size_t written;
	size_t consumed;
	size_t threshhold;
	std::string hash;
	std::string last;
	std::string next;
	pthread_t thread;
	app::PTimeout limit;
	app::PTimeout refresh;
	app::PTimeout timeout;
	inet::TInetStream stream;
	music::TTrack* track;
	music::TMP3Song* mpeg;
	music::TFLACSong* flac;
	music::PAudioBuffer buffer;
	TDecoderBuffer block;
	util::TStringList text;
	std::string title;
	app::TNotifyEvent event;
	TStreamCounters counters;
	inet::TStreamInfo info;

	void prime() {
		debug = false;
		error = false;
		started = false;
		running = false;
		streaming = false;
		playing = false;
		buffered = false;
		terminated = false;
		initialized = false;
		threshhold = app::nsizet;
		consumed = 0;
		written = 0;
		thread = 0;
		track = nil;
		mpeg = nil;
		flac = nil;
		buffer = nil;
		limit = nil;
		refresh = nil;
		timeout = nil;
		counters.prime();
	}

	void clear() {
		prime();
		next.clear();
		last.clear();
		hash.clear();
		text.clear();
		title.clear();
		info.clear();
		block.clear();
	}

	CRadioStream() {
		timeout = nil;
		prime();
	};
} TRadioStream;


typedef TWorkerThread<util::TByteBuffer> TSerialThread;


class TPlayer : public TModule, private TThreadUtil {
private:
	TMediaNames mnames;
	TDomainNames fdnames;
	music::TPlaylists playlists;
	ECommandAction currentInput;
	ECommandAction currentFilter;
	ECommandAction currentPhase;
	TClientList clients;
	util::TTimePart reopenDelay;
	util::TStringList lircBlockingList;
	app::PTimeout toLircTimeout;
	ECommandAction lircAction;
	app::TNotifyEvent event;
	TSerialThread* remoteCommandThread;
	util::TByteBuffer remoteCommandData;
	bool remoteDeviceMessage;
	size_t songDisplayChanged;
	bool streamingAllowed;
	bool advancedSettingsAllowed;
	TRadioStream radio;
	TWebClient curl;

	std::mutex nextSongMtx;
	std::mutex currentSongMtx;
	std::mutex selectedSongMtx;
	std::mutex rateMtx;
	app::TMutex actionMtx;
	app::TMutex licenseMtx;
	app::TMutex componentMtx;
	app::TMutex updateMtx;
	app::TMutex rescanMtx;
	app::TMutex remoteMtx;
	app::TMutex deviceMtx;
	app::TMutex serialMtx;
	app::TMutex driveMtx;
	app::TMutex telnetMtx;
	app::TMutex historyMtx;
	app::TMutex pagesMtx;
	app::TMutex timesMtx;
	app::TMutex mountMtx;
	app::TMutex eventMtx;
	app::TMutex streamMtx;
	app::TMutex delayMtx;
	app::TMutex mntStateMtx;
	app::TMutex scnDisplayMtx;
	app::TMutex errDisplayMtx;
	app::TMutex mntDisplayMtx;
	app::TMutex plsDisplayMtx;
	app::TMutex radioTextMtx;
	app::TMutex radioStationMtx;
	app::TMutex songDisplayMtx;
	mutable app::TMutex modeMtx;
	mutable app::TMutex commandMtx;
	app::TReadWriteLock libraryLck;
	app::TReadWriteLock stationsLck;
	app::TReadWriteLock thumbnailLck;
	
	bool terminate;
	bool cleared;
	bool debug;
	bool lircBlocked;
	bool contentChanged;
	bool contentCompare;
	bool devicesValid;
	bool drivesValid;
	bool portsValid;

	PWebToken wtCoverArtQuery;
	PWebToken wtStationsHeader;
	PWebToken wtRecentHeader;
	PWebToken wtPlaylistHeader;
	PWebToken wtPlaylistExportUrl;
	PWebToken wtPlaylistExportName;
	PWebToken wtPlaylistDataTitle;
	PWebToken wtPlaylistPreview;
	PWebToken wtPlaylistCoverArt;
	PWebToken wtPlaylistAlbum;
	PWebToken wtPlaylistArtist;
	PWebToken wtPlaylistSearch;
	PWebToken wtPlaylistTitle;
	PWebToken wtPlaylistTrack;
	PWebToken wtPlaylistTime;
	PWebToken wtPlaylistDate;
	PWebToken wtPlaylistDuration;
	PWebToken wtPlaylistProgress;
	PWebToken wtPlaylistFormat;
	PWebToken wtPlaylistCodec;
	PWebToken wtPlaylistSamples;
	PWebToken wtPlaylistIcon;
	PWebToken wtStationsExportUrl;
	PWebToken wtStationsExportName;
	PWebToken wtArtistLibraryHeader;
	PWebToken wtArtistLibraryBody;
	PWebToken wtAlbumListViewHeader;
	PWebToken wtAlbumListViewBody;
	PWebToken wtAlbumLibraryHeader;
	PWebToken wtAlbumLibraryBody;
	PWebToken wtMediaLibraryHeader;
	PWebToken wtMediaLibraryBody;
	PWebToken wtMediaLibraryIcon;
	PWebToken wtFormatLibraryHeader;
	PWebToken wtFormatLibraryBody;
	PWebToken wtSearchLibraryHeader;
	PWebToken wtSearchLibraryBody;
	PWebToken wtSearchLibraryPattern;
	PWebToken wtSearchLibraryExtended;
	PWebToken wtRecentLibraryHeader;
	PWebToken wtRecentLibraryBody;
	PWebToken wtTracksArtist;
	PWebToken wtTracksSearch;
	PWebToken wtTracksAlbum;
	PWebToken wtTracksYear;
	PWebToken wtTracksCount;
	PWebToken wtTracksFormat;
	PWebToken wtTracksCodec;
	PWebToken wtTracksSamples;
	PWebToken wtTracksCoverHArt;
	PWebToken wtTracksCoverURL;
	PWebToken wtTracksDataTitle;
	PWebToken wtTracksPreview;
	PWebToken wtTracksCoverArt;
	PWebToken wtTracksCoverHash;
	PWebToken wtTracksIcon;
	PWebToken wtTracksAudio;
	PWebToken wtPlayerStatusCaption;
	PWebToken wtPlayerStatusColor;
	PWebToken wtScannerStatusCaption;
	PWebToken wtScannerStatusColor;
	PWebToken wtArtistCount;
	PWebToken wtAlbumCount;
	PWebToken wtSongCount;
	PWebToken wtErrorCount;
	PWebToken wtTrackTime;
	PWebToken wtTrackSize;
	PWebToken wtStreamedSize;
	PWebToken wtApplicationLog;
	PWebToken wtExceptionLog;
	PWebToken wtWebserverLog;
	PWebToken wtErroneousHeader;
	PWebToken wtTableRowCount;
	PWebToken wtStreamRadioText;
	PWebToken wtActiveLicenses;
	PWebToken wtWatchLimit;

	html::PMainMenuItem playlistSelectItem;
	html::PMainMenuItem libraryMediaItem;
	html::PMainMenuItem settingsMenuItem;
	html::PMenuItem cdArtistItem;
	html::PMenuItem hdcdArtistsItem;
	html::PMenuItem dsdArtistItem;
	html::PMenuItem dvdArtistItem;
	html::PMenuItem bdArtistItem;
	html::PMenuItem hrArtistItem;

	PTimeout toBuffering;
	PTimeout toRemote;
	PTimeout toReopen;
	PTimeout toHotplug;
	PTimeout toFileWatch;
	PTimeout toRemoteCommand;
	PTimeout toUndoAction;

	music::TLibrary library;
	music::TAlsaPlayer player;
	radio::TStations stations;
	app::TRemoteAlsaDevice remote;
	app::TRemoteSerialDevice terminal;
	music::TPlayerConfig sound;
	util::TFile thumb;
	util::TFile cover;
	TPlayerMode mode;

	std::string jsonCurrentTitle;
	std::string jsonCurrentStream;
	std::string jsonCurrentStation;
	std::string jsonCurrentStreaming;
	std::string jsonCurrentPlaylist;
	std::string jsonSelectedPlaylist;
	std::string jsonRecent;
	std::string jsonPlaylist;
	std::string jsonPlaying;
	std::string jsonStations;
	std::string jsonCurrent;
	std::string jsonModes;
	std::string jsonMode;
	std::string jsonConfig;
	std::string jsonRemote;
	std::string jsonNetwork;
	std::string jsonProgress;
	std::string jsonResponse;
	std::string jsonAlbumTracks;
	std::string jsonErroneous;
	std::string jsonEditStation;
	std::string jsonCreateStation;
	std::string htmlCreatePlaylist;
	std::string htmlSelectPlaylist;
	std::string htmlRenamePlaylist;
	std::string htmlDeletePlaylist;
	std::string htmlAudioElements;
	std::string m3uPlaylistExport;
	std::string m3uStationsExport;
	
	music::TCurrentSong selectedSong;
	music::TCurrentSong currentSong;
	music::TCurrentSong nextSong;
	music::TCurrentSong lastSong;
	music::ESampleRate nextRate;
	music::EPlayerState currentState;
	app::TActionMap actions;
	app::TControlMap controls;
	app::TCommandQueue queue;
	TFileCacheMap fileCache;
	std::string coverCache;

	html::TMainMenu mnMain;
	html::TContextMenu mnRecent;
	html::TContextMenu mnPlaylist;
	html::TContextMenu mnStations;
	html::TContextMenu mnPlayer;
	html::TContextMenu mnTracks;
	html::TContextMenu mnImage;
	html::TComboBox cbxDevices;
	html::TComboBox cbxDrives;
	html::TListBox lbxDevices;
	html::TListBox lbxDrives1;
	html::TListBox lbxDrives2;
	html::TListBox lbxDrives3;
	html::TListBox lbxMounts;
	html::TListBox lbxRemote;
	html::TListBox lbxSerial;
	html::TListBox lbxHistory;
	html::TListBox lbxPages;
	html::TListBox lbxTimes;
	html::TListBox lbxGenres;
	html::TListBox lbxSearch;
	html::TListBox lbxMetadata;
	html::TButton btnRandom;
	html::TButton btnRepeat;
	html::TButton btnSingle;
	html::TButton btnDisk;
	html::TButton btnHalt;
	html::TButton btnDirect;
	html::TButton btnShuffle;
	html::TButton btnSR44K;
	html::TButton btnSR48K;
	html::TButton btnInput0;
	html::TButton btnInput1;
	html::TButton btnInput2;
	html::TButton btnInput3;
	html::TButton btnInput4;
	html::TButton btnFilter0;
	html::TButton btnFilter1;
	html::TButton btnFilter2;
	html::TButton btnFilter3;
	html::TButton btnFilter4;
	html::TButton btnFilter5;
	html::TButton btnPhase0;
	html::TButton btnPhase1;
	html::TRadioGroup rdSearchDomain;
	html::TRadioGroup rdSearchMedia;

	inet::PServerSocket server4;
	inet::PServerSocket server6;
	inet::PUnixClientSocket lirc;

	int remoteCommandThreadHandler(TSerialThread& sender, util::TByteBuffer& data);
	void onRemoteCommandMessage(TSerialThread& sender, EThreadMessageType message, util::TByteBuffer& data);
	ECommandAction getInputActionFromRemote(const uint8_t value);
	ECommandAction getFilterActionFromRemote(const uint8_t value);
	ECommandAction getPhaseActionFromRemote(const uint8_t value);
	music::ESampleRate getSampleRateFromRemote(const uint8_t value);
	bool parseRemoteStatusData(util::TByteBuffer& data);
	void testRemoteParserData();
	void removeDevelFiles();
	void checkApplicationLicenses();
	void checkApplicationLicensesWithNolock();
	bool applyApplicationLicense(const std::string& name, const std::string& value);
	bool readApplicationLicenseFromFile(const std::string& fileName);
	void downloadApplicationLicenseWithDelay();
	void downloadApplicationLicense();

	void setupSearchMediaRadios();
	void setupSearchDomainRadios();
	TMediaNames fillMediaNamesMap();
	TDomainNames fillFilterDomainMap();
	music::EMediaType getMediaType(const std::string& value) const;
	music::EFilterDomain getFilterDomain(const std::string& value) const;
	std::string getMediaShortcut(const music::EMediaType type) const;
	std::string getMediaName(const music::EMediaType type) const;
	void createCacheFolder(const std::string& cacheFolder);
	void deleteCacheFolder(const std::string& coverPath, const util::ESearchDepth depth);
	std::string normalizePlaylistHeader(const std::string& header);
	void setPlaylistHeader(const std::string& header, const std::string& playlist, const size_t count);
	void setRecentHeader(const size_t count);

	void updateLibraryStatus();
	void updateLibraryMenuItems();
	void updatePlaylistMenuItems(const std::string& playlist);
	void updatePlaylistMenuItems();
	void updateLibraryStatusWithNolock();
	void updateLibraryMenuItemsWithNolock();
	void updateLibraryMenuItemEntriesWithNolock();
	void updatePlaylistMenuItemsWithNolock(const std::string& playlist);
	void updatePlaylistMenuItemsWithNolock();
	void updateSettingsMenuItems();
	void updateSettingsMenuItemEntriesWithNolock();

	void updateComboBoxes(const music::CConfigValues& values, const std::string& limit, const std::string& period, const std::string& rows);
	void updateSerialDevices(const music::CRemoteValues& values);
	void updateLibraryMediaItems();
	void setMenuItem(html::PMenuItem item, const music::TArtistMap& artists, const size_t count, const std::string caption);
	bool updateScannerStatus();
	void suspend(TGlobalState& global);
	void upcall(TGlobalState& global);
	void initup(TGlobalState& global);
	void logger(const std::string& text) const;

	bool getBufferSize(const music::PSong song, size_t& free, size_t& size);
	int unlinkSong(const std::string& fileHash, const std::string& albumHash, const std::string& playlist);
	void saveCurrentSong(const music::TSong* song, const std::string& playlist) const;
	void loadCurrentSong(music::TSong*& song, std::string& playlist);
	void saveSelectedPlaylist(const std::string& playlist) const;
	void loadSelectedPlaylist(std::string& playlist);
	util::TImage::EImageType getImageType(const music::TArtwork& cover);
	std::string listAsJSON(const util::TStringList& list, const std::string& title, const size_t total, size_t index);
	music::EPlayListAction sanitizePlayerAction(music::EPlayListAction action, const std::string& playlist);
	music::PSong getSongData(const music::TCurrentSong& song, music::CSongData& data, std::string& playlist);
	void getSongProperties(const music::TSong* song, music::CSongData& data);
	std::string getGlobalSongTitle(TGlobalState& global);
	util::TTimePart getGlobalSongDuration(TGlobalState& global);
	void playlistGarbageCollector();

	void setupWebserverObjects(const size_t songsPerAlbum);
	void setupListBoxes();
	void setupContextMenus();
	void setupMainMenu();
	void setupButtons();

	void updateActiveRateButtons(const music::CAudioValues& values, const music::ESampleRate rate, const music::ESampleRate locked = music::SR0K);
	void updateSelectPhaseButtons(const music::CAudioValues& values, const app::ECommandAction action);
	void updateSelectInputButtons(const music::CAudioValues& values, const app::ECommandAction action);
	void updateSelectFilterButtons(const music::CAudioValues& values, const app::ECommandAction action);
	void updateActiveRateButtonsWithNolock(const music::CAudioValues& values, const music::ESampleRate rate, const music::ESampleRate locked = music::SR0K);
	void updateSelectPhaseButtonsWithNolock(const music::CAudioValues& values, const app::ECommandAction action);
	void updateSelectInputButtonsWithNolock(const music::CAudioValues& values, const app::ECommandAction action);
	void updateSelectFilterButtonsWithNolock(const music::CAudioValues& values, const app::ECommandAction action);

	void setRemoteDevice(const music::ESampleRate rate);
	void setRemoteDevice(const music::CAudioValues& values, const music::ESampleRate rate);
	bool openRemoteDevice(const music::CAudioValues& values, const music::ESampleRate rate, bool& opened, bool& unchanged);
	bool openRemoteDevice(const music::CAudioValues& values, const music::TSong* song, bool&, bool& unchanged);
	bool openRemoteDeviceWithNolock(const music::CAudioValues& values, const music::ESampleRate rate, bool&, bool& unchanged);
	void closeRemoteDevice(bool& closed);
	music::ESampleRate getRemoteRate();
	bool sendRemoteCommand(const music::CAudioValues& values, const EDeviceType id, const ERemoteCommand command,
			const TCommandData value, const EResponseType type);
	bool sendRemoteStatusRequest(unsigned char page = 3);
	bool trySendRemoteStatusRequest();
	void executeRemoteAction(const app::ECommandAction action);
	void executeRemoteAction(const music::CAudioValues& values, const app::ECommandAction action);

	void executeLircCommand(const util::TBuffer& buffer, const size_t size);
	void executeTelnetCommand(const std::string command, std::string& result, bool& drop);

	void closeTerminalDevice();
	void setCurrentInput(const app::ECommandAction action);
	app::ECommandAction getCurrentInput() const;
	void setCurrentFilter(const app::ECommandAction action);
	app::ECommandAction getCurrentFilter() const;
	void setCurrentPhase(const app::ECommandAction action);
	app::ECommandAction getCurrentPhase() const;
	void addClientSocket(inet::TSocket& socket, const app::THandle client);
	void deleteClientSocket(const inet::TSocket& socket, const app::THandle client);
	void sendSocketMessage(const inet::TServerSocket& socket, const app::THandle client, const std::string message);
	void sendClientMessage(const inet::TServerSocket& socket, const app::THandle client, const std::string message);
	void sendBroadcastMessage(const std::string message);
	void notifyEvent(const std::string message);
	bool selectCurrentPlaylist(const std::string& playlist, const bool save = true);
	bool renameCurrentPlaylist(const std::string oldName, const std::string newName);

	size_t getSearchHTML(util::TStringList& html, const std::string& filter, const std::string& genre, const music::EFilterDomain domain, const music::EMediaType media, const size_t max, const bool sortByYear, util::TStringList& genres);
	size_t getAlbumsHTML(util::TStringList& html, const std::string& filter, const music::EFilterDomain domain, const music::EMediaType type, const music::EArtistFilter partial, const music::CConfigValues& config);
	music::TLibraryResults getArtistsHTML(util::TStringList& html, const std::string& filter, music::EMediaType type, const music::CConfigValues& config, const music::TLibrary::EViewType view);

	void notifyStreamThread();
	bool streamThreadMethod();
	void createInetStreamer();
	void destroyInetStreamer();
	bool prepareInetStream();
	bool openInetStream(const music::ECodecType codec);
	void resetInetStream();
	bool delayInetStream();
	void finalizeInetStream(const std::string& URL);
	void executeInetStreamAction(const ECommandAction action);

	void clearInetStreamWithNolock();
	void setStreamInfo(const inet::TStreamInfo& info);
	void setRadioStation(const inet::TStreamInfo& info);
	void getRadioStation(inet::TStreamInfo& info);
	void clearRadioStation();
	void updateRadioStreamInfo();
	void setRadioStreamInfo(const std::string& title, const size_t streamed);
	void setRadioBitrate(const size_t bitrate);
	void setRadioText(const std::string& text);
	bool getRadioText(util::TStringList& text);
	bool getRadioTitle(std::string& title);
	void clearRadioText();

	void invalidateSongDislpay();
	size_t getSongDisplayChange();

	void normalizeStationName(std::string& description, const std::string name);
	bool splitRadioText(const std::string& title, const radio::EMetadataOrder order, std::string& artist, std::string& track);

	void onInetStreamData(const inet::TInetStream& sender, const void *const data, const size_t size);
	void onInetStreamInfo(const inet::TInetStream& sender, const inet::TStreamInfo& info);
	void onInetStreamTitle(const inet::TInetStream& sender, const std::string& title);
	void onInetStreamError(const inet::TInetStream& sender, const int error, const std::string& text, bool& terminate);

	void broadcastPlayerUpdate();
	void broadcastStreamUpdate();
	void broadcastWebSocketEvent(const std::string& location, const std::string& message);
	void onWebSocketData(const app::THandle handle, const std::string& message);
	void onWebSocketVariant(const app::THandle handle, const util::TVariantValues& variants);

	void prepareWebRequest(const std::string& uri, const util::TVariantValues& query, util::TVariantValues& session, bool& prepared);
	void defaultWebAction(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onLibraryClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onButtonClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onContextMenuClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onRowPlaylistClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onStationsMenuClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onStationsButtonClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onProgressClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onSearchData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onDialogData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onEditorData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onConfigData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onRemoteData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onFileUploadEvent(const app::TWebServer& sender, const util::TVariantValues& session, const std::string& fileName, const size_t& size);
	void onWebStatisticsEvent(const app::TWebServer& sender, const app::TWebData&);
	ssize_t onLircControlData(const inet::TUnixClientSocket& socket);

	bool play(TGlobalState& global, music::PSong song, const std::string& playlist, bool& isOpen);
	bool stop(TGlobalState& global);
	bool pause(TGlobalState& global);
	bool toggle(TGlobalState& global);
	void close(TGlobalState& global);

	int getSerialPorts(util::TStringList& devices);
	void rescanSerialPorts(const bool nolock);
	void rescanSerialPortsWithNolock(music::CAudioValues& values, const bool nolock);
	void setHistoryDefaultCounts(const size_t max);
	void setPageDefaultCounts();
	void setPeriodDefaultTimes();
	void rescanHardwareDevicesWithNolock(const music::CAudioValues& values);
	void rescanLocalDrivesWithNolock(const music::CConfigValues& values);
	void rescanHardwareDevices();
	void rescanLocalDrives();
	void updateHardwareDevices();
	void updateLocalDrives();
	void updateSerialPorts();
	void invalidateHardwareDevices();
	void invalidateLocalDrives();
	void invalidateSerialPorts();

	void executeFileCommand();
	void queueCommand(const TCommand& command);
	void addCommand(const ECommandAction action);
	ECommandAction getRemoteCommand(const std::string& command);
	ECommandAction getControlCommand(const std::string& command);
	ECommandAction addRemoteCommand(const std::string& command);
	ECommandAction addControlCommand(const std::string& command);
	void executeControlAction(const ECommandAction action);
	bool preparePlayerCommand(TGlobalState& global);
	void prepareCurrentSong(music::TSong*& song);
	void prepareCurrentAlbum(music::TSong*& song, const std::string& playlist);
	void prepareNextSong(music::TSong*& song, const std::string& playlist);
	void preparePreviousSong(music::TSong*& song, const std::string& playlist);
	bool prepareRandomSong(music::TSong*& song, const std::string& playlist);
	void prepareQueuedSong(music::TSong*& song, std::string& playlist);
	void prepareQueuedAlbum(music::TSong*& song, std::string& playlist);
	bool executePlayerCommand(TGlobalState& global);
	bool executeControlCommand(TGlobalState& global);
	bool executeSystemAction(const app::ECommandAction action);
	bool executeRepeatModeAction(const app::ECommandAction action);
	bool executePlaylistAction(TCommand& command);
	void terminatePlayerCommand(TGlobalState& global);
	bool executeBufferTask(TGlobalState& global);
	bool executePlayerAction(TGlobalState& global);
	void closeAlsaDevice(TGlobalState& global);

	bool toggleRepeatMode(html::TButton& button, const html::EComponentType type = html::ECT_PRIMARY);
	void setRepeatMode(html::TButton& button, const bool value, const html::EComponentType type = html::ECT_PRIMARY);
	void setCurrentMode(const TPlayerMode& mode);
	void getCurrentMode(TPlayerMode& mode) const;
	std::string modeToStr(TPlayerMode& mode, const char separator = ' ') const;

	void updateLibrary(const bool aggressive);
	void rescanLibrary();
	void rebuildLibrary();
	void deleteLibrary();
	void addSong(const std::string& fileHash, const std::string& albumHash, const std::string& playlist, const music::EPlayListAction action);
	void deleteSong(const std::string& fileHash, const std::string& playlist);
	std::string addArtist(const std::string& artistName, const std::string& playlist, const music::EPlayListAction action);
	void addAlbum(const std::string& albumHash, const std::string& playlist, const music::EPlayListAction action);
	void addAlbumWithNolock(const std::string& albumHash, const std::string& playlist, const music::EPlayListAction action);
	void deleteAlbum(const std::string& albumHash, const std::string& playlist);
	void addTrack(const std::string& fileHash, const std::string& playlist, const music::EPlayListAction action);
	void addTracks(const std::string& albumHash, const std::string& playlist, const music::EPlayListAction action);

	void onLibraryProgress(const music::TLibrary& sender, const size_t count, const std::string& current);
	void onPlayerPlaylistRequest(const music::TAlsaPlayer& sender, const music::TSong* current, music::TSong*& next, std::string& playlist, bool& reopen);
	void onPlayerStateChanged(const music::TAlsaPlayer& sender, const music::EPlayerState state, const music::TSong* current, const std::string& playlist);
	void onPlayerOutputChanged(const music::TAlsaPlayer& sender, const music::EPlayerState state, const music::TSong* current, const std::string& playlist);
	void onPlayerProgressChanged(const music::TAlsaPlayer& sender, const music::EPlayerState state, const music::TSong* current, const int64_t streamed, const std::string& playlist);
	void onSocketConnect(inet::TSocket& socket, const app::THandle client);
	void onSocketClose(inet::TSocket& socket, const app::THandle client);
	ssize_t onSocketData(const inet::TServerSocket& socket, const app::THandle client, bool& drop);
	void onWatchEvent(const std::string& file, bool& proceed);
	void onHotplugEvent(const app::THotplugEvent& event, const app::EHotplugAction action);
	void onUndoActionTimeout();
	void onCommandTimeout();
	void onTasksTimeout();
	void onStreamTimeout();
	void onStreamRefresh();
	void onHotplug();
	void onFileWatch();
	void onCommandDelay();

	void selectRandomSong(const TPlayerMode& mode, const std::string& playlist, const music::TSong* current, music::TSong*& next, bool& transition);
	void selectRandomSongWithNolock(const TPlayerMode& mode, const std::string& playlist, const music::TSong* current, music::TSong*& next, bool& transition);
	void setPlayerStatusLabel(const music::EPlayerState state, const bool streaming);
	void setScannerStatusLabel(const bool scanning, const size_t size, const size_t erroneous);
	void setErroneousHeader(const size_t erroneous);
	void setAlbumHeader(PWebToken token, const std::string& request, const std::string& filter, const size_t albums, const music::EFilterDomain domain, const util::TStringList& various);
	std::string getSongsStatusString(const music::CSongData& song, const std::string& playlist, const std::string& seperator);
	std::string getSongsStatusString(const music::TSong* song, const std::string& playlist, const std::string& seperator);

	void getCurrentSongs(music::TCurrentSongs& songs);
	music::PSong getCurrentSong(music::CSongData& song, std::string& playlist);
	music::PSong getCurrentSong();
	void clearCurrentSong();
	void setRandomSong(music::PSong song, const std::string playlist);

	music::PSong getSelectedSong(music::CSongData& song, std::string& playlist);
	void setSelectedSong(const music::TSong* song, const std::string& playlist);
	void clearSelectedSong();

	music::PSong getLastSong(music::CSongData& song, std::string& playlist);
	music::PSong getLastSong();
	void clearLastSong();

	music::PSong getPlayedSong(music::CSongData& song, std::string& playlist);
	music::PSong getNextSong(music::CSongData& song, std::string& playlist);
	music::PSong getNextSong();
	void setNextSong(const music::TSong* song, const std::string& playlist);
	void clearNextSong();

	music::ESampleRate getNextRate();
	music::ESampleRate getAndClearNextRate();
	void setNextRate(const music::ESampleRate rate);
	void clearNextRate();

	void setCurrentState(const music::TSong* song, const std::string& playlist, const music::EPlayerState state);
	void getCurrentState(music::TSong*& song, std::string& playlist, music::EPlayerState& state);
	void getCurrentState(music::CSongData& song, music::EPlayerState& state);
	void getCurrentState(music::EPlayerState& state);

	bool isPlaying();
	bool isStopped();
	bool isClosed();
	bool isStreamable();

	void getCurrentPlayerValues(TGlobalState& global);
	void getRequestedCommandValues(TGlobalState& global);
	std::string getProgressTimestamp(util::TTimePart played, util::TTimePart duration, bool advanced);

	void getResponse(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getCoverArt(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getMediaIcon(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getThumbNail(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getPlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getRecent(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getPlaylistExport(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getStationsExport(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getErroneous(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getPlaying(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getStations(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getPlayerState(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getRepeatModes(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getPlayerMode(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getPlayerConfig(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getRemoteConfig(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getCurrentPlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getRadioTextUpdate(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getCurrentRadioTitle(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getRadioPlayUpdate(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getRadioPlayStream(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getSelectedPlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getProgressData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getAlbumTracks(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getCreatePlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getSelectPlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getRenamePlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getDeletePlaylist(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getCurrentPlaylists(std::string& selected, util::TStringList& names);
	void getDeleteStation(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getEditStation(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);

	void setReorderedData(TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error);
	void setRearrangedData(TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error);

	void debugOutputSongTable(const data::TTable& table, size_t count = 0);
	void debugOutputStationTable(const data::TTable& table, size_t count = 0);

	void setControlData(TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error);
	void setControlAPI(TThreadData& sender, void const * const data, const size_t size, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error);

	std::string getNocoverFileName(const size_t dimension, const std::string& cache);
	std::string getSpecialFileName(const size_t dimension, const std::string& cache);
	std::string getCachedFileName(const std::string& fileHash, const size_t dimension, const std::string& cache);
	bool getThumbFromCache(const std::string& fileName, const void*& data, size_t& size);
	std::string getThumbFile(const std::string& fileHash, const std::string& folder, const size_t dimension, const std::string& cache, const bool special);
	std::string getThumbFile(util::TJpeg& jpg, const std::string& fileHash, const std::string& folder, const size_t dimension, const std::string& cache, const bool special);
	std::string findFileName(const std::string& path, const std::string& file);
	void createCacheFolders(const std::string& cachePath);
	void deleteCachedFiles(const std::string& fileHash);
	void deleteCachedImages(const std::string& fileName);
	void invalidateFileCache();

	void undoStreamAction();
	void clearStreamUndoList();
	bool getStreamURL(std::string& URL);
	bool getStreamName(std::string& name);
	bool getStreamHash(std::string& hash);
	bool getStreamIndex(size_t& index);
	bool getStreamProperties(std::string& name, std::string& url, radio::EMetadataOrder& order);
	music::ECodecType getStreamCodec(const std::string& mime);
	bool getAndClearNextHash(std::string& hash);
	bool startInetStream(const std::string& hash);
	bool restartInetStream();
	bool stopInetStream();
	void clearInetStream();

	void onSigint();
	void onSigterm();
	void onSighup();
	void onSigusr1();
	void onSigusr2();

public:
	int streamThreadHandler();

	int execute();
	void cleanup();
	bool isTerminated() const { return terminate; };

	TPlayer();
	virtual ~TPlayer();
};
typedef TPlayer* PMain;


} /* namespace app */

#endif /* MAIN_H_ */
