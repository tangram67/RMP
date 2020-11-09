/*
 * alsa.h
 *
 *  Created on: 25.08.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef ALSA_H_
#define ALSA_H_

#include <alsa/asoundlib.h>
#include "alsatypes.h"
#include "audiotypes.h"
#include "audiobuffer.h"
#include "audiofile.h"
#include "tagtypes.h"
#include "semaphores.h"
#include "threadqueue.h"
#include "threads.h"

#define _TRACERT_ traceline=__LINE__

namespace music {

class TAlsaPlayer;

enum EPlayerState {
	EPS_CLOSED  = 0,
	EPS_IDLE    = 1,
	EPS_PLAY    = 2,
	EPS_WAIT    = 4,
	EPS_REOPEN  = 8,
	EPS_PAUSE   = 16,
	EPS_HALT    = 32,
	EPS_STOP    = 64,
	EPS_ERROR   = 128,
	EPS_DEFAULT = EPS_CLOSED
};


struct TPlayerStateName {
	EPlayerState state;
	const char* name;
};

static const struct TPlayerStateName playerstates[] =
{
	{ EPS_CLOSED,  "Closed"  },
	{ EPS_IDLE,    "Open"    },
	{ EPS_PLAY,    "Playing" },
	{ EPS_WAIT,    "Waiting" },
	{ EPS_REOPEN,  "Reopen"  },
	{ EPS_PAUSE,   "Paused"  },
	{ EPS_HALT,    "Halted"  },
	{ EPS_STOP,    "Stopped" },
	{ EPS_ERROR,   "Failure" },
	{ EPS_DEFAULT,  nil      }
};


enum EPlayerCommand {
	EPP_NONE,
	EPP_FORWARD,
	EPP_REWIND,
	EPP_NEXT,
	EPP_PREV,
	EPP_PAUSE,
	EPP_PLAY,
	EPP_STOP,
	EPP_POSITION,
	EPP_DEFAULT = EPP_NONE
};

struct TPlayerCommandName {
	EPlayerCommand command;
	const char* name;
};

static const struct TPlayerCommandName commands[] =
{
	{ EPP_NONE,    "None"     },
	{ EPP_FORWARD, "Forward"  },
	{ EPP_REWIND,  "Rewind"   },
	{ EPP_NEXT,    "Next"     },
	{ EPP_PREV,    "Previous" },
	{ EPP_PAUSE,   "Pause"    },
	{ EPP_PLAY,    "Play"     },
	{ EPP_STOP,    "Stop"     },
	{ EPP_DEFAULT,  nil       }
};

typedef struct CPlayerState {
	TCurrentSong current;
	EPlayerState state;
	bool invalidated;
	bool progressed;
	bool changed;

	void prime() {
		state = EPS_DEFAULT;
		invalidated = false;
		progressed = false;
		changed = false;;
	}

	CPlayerState() {
		prime();
	}
} TPlayerState;


#ifdef STL_HAS_TEMPLATE_ALIAS

using TPlayerStateCallback = std::function<void(const TAlsaPlayer& sender, const EPlayerState state, const TSong* current, const std::string& playlist)>;
using TPlayerProgressCallback = std::function<void(const TAlsaPlayer& sender, const EPlayerState state, const TSong* current, const int64_t streamed, const std::string& playlist)>;
using TPlayerPlaylistRequest = std::function<void(const TAlsaPlayer& sender, const TSong* current, TSong*& next, std::string& playlist, bool& reopen)>;

#else

typedef std::function<void(const TAlsaPlayer& sender, const EPlayerState state, const TSong* current, const std::string& playlist)> TPlayerStateCallback;
typedef std::function<void(const TAlsaPlayer& sender, const EPlayerState state, const TSong* current, const int64_t streamed, const std::string& playlist)> TPlayerProgressCallback;
typedef std::function<void(const TAlsaPlayer& sender, const TSong* current, TSong*& next, const std::string& playlist, bool& reopen)> TPlayerPlaylistRequest;

#endif


struct TPlayerCommand {
	EPlayerCommand command;
	double value;
	int state;

	TPlayerCommand() {
		command = EPP_NONE;
		value = 0.0;
		state = 0;
	}
};


class TByteConverter {
public:
	// Converter for different word sizes (NO endianess!)
	void convert8to8Bit(const int8_t *src, int8_t *dst, size_t size);
	void convert8to16Bit(const int8_t *src, int16_t *dst, size_t size);
	void convert8to24Bit(const int8_t *src, int32_t *dst, size_t size);
	void convert8to32Bit(const int8_t *src, int32_t *dst, size_t size);

	void convert16to16Bit(const int16_t *src, int16_t *dst, size_t size);
	void convert16to24Bit(const int16_t *src, int32_t *dst, size_t size);
	void convert16to32Bit(const int16_t *src, int32_t *dst, size_t size);

	void convert24to24Bit(const int32_t *src, int32_t *dst, size_t size);
	void convert24to32Bit(const int32_t *src, int32_t *dst, size_t size);

	// Writer from PCM sample buffer to different word sizes (NO endianess!)
	void write16to16Bit(const TSample *src, int16_t *dst, size_t& read, snd_pcm_uframes_t frames);
	void write16to24Bit(const TSample *src, int32_t *dst, size_t& read, snd_pcm_uframes_t frames);
	void write16to32Bit(const TSample *src, int32_t *dst, size_t& read, snd_pcm_uframes_t frames);

	void write24to24Bit(const TSample *src, int32_t *dst, size_t& read, snd_pcm_uframes_t frames);
	void write24to32Bit(const TSample *src, int32_t *dst, size_t& read, snd_pcm_uframes_t frames);

	void write16to16Bit(const TSample *& src, int16_t *& dst, size_t& read);
	void write16to24Bit(const TSample *& src, int32_t *& dst, size_t& read);
	void write16to32Bit(const TSample *& src, int32_t *& dst, size_t& read);

	void write24to24Bit(const TSample *& src, int32_t *& dst, size_t& read);
	void write24to32Bit(const TSample *& src, int32_t *& dst, size_t& read);

	TByteConverter();
	virtual ~TByteConverter();
};



class TPCMConverter {
protected:
	TSample getRandomDither();

public:
	// Writer from PCM sample buffer to byte aligned destination buffer
	void write16to16Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian);
	void write16to24Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian, const TSample dither = 0);
	void write16to32Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian, const TSample dither = 0);

	void write24to24Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian);
	void write24to32Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian, const TSample dither = 0);

	// Writer from PCM sample buffer to byte aligned destination buffer for count of frames
	void write16to16Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian);
	void write16to24Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian, const bool dithered = false);
	void write16to32Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian, const bool dithered = false);

	void write24to24Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian);
	void write24to32Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian, const bool dithered = false);

	// Writer from PCM sample buffer to byte aligned destination buffer an increment destination by step width in bytes
	void write16to16Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian);
	void write16to24Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian, const TSample dither = 0);
	void write16to32Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian, const TSample dither = 0);

	void write24to24Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian);
	void write24to32Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian, const TSample dither = 0);

	TPCMConverter();
	virtual ~TPCMConverter();
};


class TDSDMarker {
private:
	TSample marker;
	TSample marker1;
	TSample marker2;

public:
	TSample getMarker() {
		return marker;
	}
	TSample getNextMarker() {
		marker = (marker == marker1) ? marker2 : marker1;
		return marker;
	}
	void setMarker(const TSample value) {
		marker = value;
	}
	void setLastMarker(const TSample value) {
		marker = value;
		getNextMarker();
	}

	TDSDMarker() {
		marker1 = 0x05;
		marker2 = 0xFA;
		marker = marker1;
	};
	virtual ~TDSDMarker() = default;
};


class TAlsaCallbackHandler {
public:
	virtual void alsaCallbackHandler() = 0;

	virtual ~TAlsaCallbackHandler() = default;
};


class TAlsaMixer {
private:
	snd_mixer_t *smx_handle;

public:
	bool isValid() const { return util::assigned(smx_handle); };
	snd_mixer_t * handle() const { return smx_handle; };
	snd_mixer_t * operator () () { return handle(); };

	void open();
	void close();

	TAlsaMixer();
	virtual ~TAlsaMixer();
};


class TAlsaElementId {
private:
	snd_mixer_selem_id_t * sid;
	void init();

public:
	bool isValid() const { return util::assigned(sid); };
	snd_mixer_selem_id_t * value() const { return sid; };
	snd_mixer_selem_id_t * operator () () { return value(); };
	void clear();

	TAlsaElementId();
	virtual ~TAlsaElementId();
};



class TAlsaPlayer : private TPCMConverter, private TDSDMarker, private app::TThreadUtil, private app::TThreadAffinity {
private:
	mutable app::TSemaphore sema;
	mutable app::TMutex alsaMtx;
	app::TMutex eventMtx;
	pthread_t thread;
	bool invalidated;
	bool progressed;
	bool changed;
	bool dithered;
	bool terminate;
	bool running;
	bool started;
	int64_t streamed;
	util::TStringList cards;

	app::TThreadQueue<TPlayerCommand> queue;

	snd_pcm_t* snd_handle;
	snd_pcm_hw_params_t* hw_params;
	snd_pcm_sw_params_t* sw_params;
    snd_async_handler_t* handler;

	CStreamData stream;
	std::string device;
	std::string card;
	long volume;
	bool muted;

	int errval;
	int e_count;
	std::string errmsg;
	EPlayerState m_state;
	EPlayerState p_state;
	EPlayerState e_state;
	TAudioBufferList buffers;
	PAudioBuffer buffer;
	TCurrentSong lastSong;
	TCurrentSong currentSong;

	int verbosity;
	bool debug;
	bool enabled;
	bool periodEvent;
	bool ignoreMixer;
	bool forceReopen;
	bool dop;

	snd_pcm_format_t snd_format;
	snd_pcm_uint_t snd_channels;
	snd_pcm_uint_t snd_samplerate;
	snd_pcm_uint_t snd_buffertime;
	snd_pcm_uframes_t snd_buffersize;
	snd_pcm_uint_t snd_periodtime;
	snd_pcm_uint_t snd_activeperiodtime;
	snd_pcm_uframes_t snd_periodsize;
	snd_pcm_uframes_t snd_framesize;
	snd_pcm_int_t snd_physicalwidth;
	snd_pcm_int_t snd_datawidth;
	snd_pcm_int_t snd_dataframesize;

	snd_pcm_uint_t m_periodtime;
	snd_pcm_uint_t m_buffertime;
	snd_pcm_int_t  m_physicalwidth;
	util::TTimePart m_skipframe;
	util::EEndianType m_endian;

	TPlayerStateCallback onOutputStateChanged;
	TPlayerStateCallback onPlaybackStateChanged;
	TPlayerProgressCallback onPlaybackProgressChanged;
	TPlayerPlaylistRequest onPlaybackPlaylistRequest;
	size_t lastSongProgress;
	util::TTimePart lastSongPlayed;
	int64_t lastSongStreamed;
	app::PLogFile logfile;

	void prime();
	void clear();
	void reset(const EPlayerState state);
	bool success() const { return errval == EXIT_SUCCESS; };

	void createAlsaThread();
	void terminateAlsaThread();
	void alsaThreadMethod();

	bool setAlsaParams(const CStreamData& stream, const util::EEndianType endian);
	bool setHardwareParams(snd_pcm_access_t access);
	bool setSoftwareParams();
	bool validStreamParams();

	snd_pcm_format_t getHardwareFormat();
	bool setHardwareFormat(const snd_pcm_format_t format);
	util::EEndianType getEndianFromFormat(const snd_pcm_format_t format);
	std::string formatToStr(const snd_pcm_format_t format) const;
	std::string getHardwareCard() const ;
	void getSongProperties(const music::TSong* song, music::CSongData& data) const;

	bool writeSilence();
	int writeDSDSilence(TSample *dst[], snd_pcm_uframes_t& frames);
	void write24BitDSDSilence(TSample *& dst, snd_pcm_uframes_t& frames);
	void write32BitDSDSilence(TSample *& dst, snd_pcm_uframes_t& frames);

	bool isLE() const { return m_endian == util::EE_LITTLE_ENDIAN; };
	bool isBE() const { return m_endian == util::EE_BIG_ENDIAN; };

	bool writePeriodData();
	int writeFrameData(const TSample * src, TSample *dst[], size_t& read,
			const snd_pcm_channel_area_t *areas, const snd_pcm_int_t steps[], snd_pcm_uframes_t& frames);
	int writeStereoData(const TSample * src, TSample *dst[], size_t& read, snd_pcm_uframes_t& frames);
	void write24BitDSDData(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames);
	void write32BitDSDData(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames);

	bool doSeek(TAudioBuffer*& buffer, PSong song, double position);
	bool doFastForward(TAudioBuffer*& buffer, PSong song);
	bool doFastRewind(TAudioBuffer*& buffer, PSong song);
	util::TTimePart getSkipFrame(const TSong* song);
	size_t getSkipBytes(const TSong* song, const util::TTimePart frame);

	int recoverUnderrun(int error, int location);

	bool getOpen() const { return util::assigned(snd_handle); };
	bool getPlaying() const { return m_state == EPS_PLAY; };
	bool getPaused() const { return util::isMemberOf(m_state, EPS_PAUSE,EPS_WAIT,EPS_REOPEN); };
	bool getStopped() const { return m_state == EPS_STOP; };
	bool getHalted() const { return m_state == EPS_HALT; };
	bool getDoP() const { return dop; };
	EPlayerState getState() const { return m_state; };
	const std::string& getDevice() const { return device; };

	bool openDevice(const std::string& device, const CStreamData& stream);
	void closeDevice();
	bool dropDevice();
	bool stopDevice();
	bool pauseDevice();
	bool resumeDevice();
	bool haltDevice();
	bool restartDevice();
	bool toggleDevice(EPlayerState& state);

	void setErrorState();
	void setErrorStateWithNolock();
	void setPlayerState(const EPlayerState value);
	void setPlayerStateWithNolock(const EPlayerState value);
	void getPlayerState(TPlayerState& state);
	void getPlayerStateWithNolock(TPlayerState& state);
	void getAndResetPlayerState(TPlayerState& state);
	void getAndResetPlayerStateWithNolock(TPlayerState& state);
	void updatePlayerState(TPlayerState& state, const EPlayerState value);
	void updatePlayerStateWithNolock(TPlayerState& state, const EPlayerState value);
	void detectOutputChange(const EPlayerState& value);
	void detectOutputChangeWithNolock(const EPlayerState& value);
	void processStateChangeEvents(TPlayerState& player, const std::string& action);

	void executeStatusChangeWithNolock(const EPlayerState value, const TCurrentSong& current);
	void executeStatusChange();
	void executeOutputChangeWithNolock(const EPlayerState value, const TCurrentSong& current);
	void executeOutputChange();
	void executeProgressChangeWithNolock(const TCurrentSong& current);
	void executeProgressChange();
	void onProgressChangedCallback(const TCurrentSong& current);
	void onStateChangedCallback(const EPlayerState state, const TCurrentSong& current);
	void onOutputChangedCallback(const EPlayerState state, const TCurrentSong& current);
	void onPlaylistRequest(const TCurrentSong& current, TSong*& next, std::string& playlist, bool& reopen);

public:
	int alsaThreadHandler();
	void alsaCallbackHandler();

	bool isOpen() const;
	bool isPlaying() const;
	bool isPaused() const;
	bool isHalted() const;
	bool isStopped() const;
	bool isDoP() const;
	bool isDithered() const;

	void getBitDepth(std::string& bits) const;
	EPlayerState getCurrentState() const;
	const std::string& getCurrentDevice() const;
	const std::string& getCurrentPlaylist() const;
	void getLastSong(CSongData& song) const;
	void getLastSong(CSongData& song, std::string& playlist) const;
	void getLastSong(CCurrentSong& song) const;
	void getCurrentSong(CSongData& song) const;
	void getCurrentSong(CSongData& song, std::string& playlist) const;
	void getCurrentSong(CCurrentSong& song) const;
	int64_t getStreamed() const;

	void initialize();
	void finalize();
	void invalidate();

	bool open(const CStreamData& stream);
	bool open(const std::string& device, const CStreamData& stream);
	void close();

	void setDithered(const bool value) { dithered = value; };
	void setVerbosity(const int value) { verbosity = value; };
	void setDebug(const bool value) { debug = value; };
	void setIgnoreMixer(const bool value) { ignoreMixer = value; };
	void setLogFile(const app::PLogFile logger) { logfile = logger; };

	static std::string statusToStr(const EPlayerState value);
	static std::string bufferToStr(const EBufferState value);
	static std::string levelToStr(const EBufferLevel value);
	static std::string commandToStr(const EPlayerCommand value);

	bool play(PSong song, const std::string& playlist, const std::string& device, bool& isOpen);
	bool stop();
	bool pause();
	bool resume();
	bool halt();
	bool restart();
	bool toggle(EPlayerState& state);

	bool setMasterVolume(long volume);
	bool unmute();
	bool mute();

	void addStreamCommand(const EPlayerCommand command);
	void addSeekCommand(const double position);
	void queueSeekCommand(const double position);
	void clearStreamCommands();

	void setPeriodTime(snd_pcm_uint_t periodTime) { m_periodtime = periodTime * 1000; };
	snd_pcm_uint_t getPeriodTime() { return m_periodtime / 1000; };
	snd_pcm_uint_t getActivePeriodTime() { return snd_activeperiodtime / 1000; };
	void setBufferTime(snd_pcm_uint_t bufferTime) { m_buffertime = bufferTime * 1000; };
	snd_pcm_uint_t getBufferTime() { return m_buffertime / 1000; };
	void setSkipFrame(util::TTimePart skipFrame) { m_skipframe = skipFrame; };
	util::TTimePart getSkipFrame() { return m_skipframe; };

	void configure(const TAlsaConfig& config);
	bool getDebug() const { return debug; };
	int getVerbosity() const { return verbosity; };

	snd_pcm_t* handle() const { return snd_handle; };
	int error() const { return errval; };
	std::string syserr() const { return util::csnprintf("% [%]", snd_strerror(errval), errval); };
	std::string strerr() const { return errmsg; };

	void logger(const std::string& text);
	void logger(int level, const std::string& text);

	void formatOutput(const std::string& preamble = "") const;
	void parameterOutput(const std::string& preamble = "") const;
	static int getAlsaHardwareDevices(util::TStringList& devices, util::TStringList& ignore);

	void createBuffers(const size_t fraction, const size_t maxBufferCount = MAX_BUFFER_COUNT,
			const size_t minBufferSize = MIN_BUFFER_SIZE, const size_t maxBufferSize = MAX_BUFFER_SIZE);
	size_t bufferCount() const;
	size_t freeBufferSize() const;

	size_t resetStreamBuffers();
	size_t bufferGarbageCollector(const music::TCurrentSongs& songs);
	void debugOutputBuffers(const std::string& preamble = "") const;

	bool isSongBuffered(const TSong* song) const;
	bool isFileBuffered(const std::string& fileHash) const;
	bool isAlbumBuffered(const std::string& albumHash) const;

	size_t resetBuffers(const util::hash_type hash, const PSong current, const size_t index, const ECompareType type = ECT_LESSER, size_t range = 0);
	void resetBuffers(PSong song);
	void resetBuffers(PTrack track);
	void resetBuffers();
	void resetBuffer(PAudioBuffer buffer);

	void operateBuffers(PAudioBuffer buffer, const PTrack track, const EBufferState state, const EBufferLevel level);
	void operateBuffers(PAudioBuffer buffer, const PTrack track, const EBufferState state);
	void operateBuffers(PAudioBuffer buffer, const EBufferState state, const bool force = false);
	void setBufferLevel(PAudioBuffer buffer, const EBufferLevel level);

	PAudioBuffer getNextEmptyBuffer(const TTrack* track);
	PAudioBuffer getNextEmptyBuffer(const TSong* song);
	PAudioBuffer getNextEmptyBuffer();

	template<typename reader_t, typename class_t>
		inline void bindStateChangedEvent(reader_t &&onPlaybackState, class_t &&owner) {
			onPlaybackStateChanged = std::bind(onPlaybackState, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		}

	template<typename reader_t, typename class_t>
		inline void bindOutputChangedEvent(reader_t &&onMuteState, class_t &&owner) {
			onOutputStateChanged = std::bind(onMuteState, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		}

	template<typename reader_t, typename class_t>
		inline void bindProgressChangedEvent(reader_t &&onProgressChanged, class_t &&owner) {
			onPlaybackProgressChanged = std::bind(onProgressChanged, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
		}

	template<typename reader_t, typename class_t>
		inline void bindPlaylistRequestEvent(reader_t &&onPlaylistRequest, class_t &&owner) {
			onPlaybackPlaylistRequest = std::bind(onPlaylistRequest, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
		}

	TAlsaPlayer();
	virtual ~TAlsaPlayer();
};


template<typename T>
class TMixerGuard {
private:
	typedef T mixer_t;
	mixer_t& instance;

public:
	void open() {
		if (!instance.isValid()) {
			instance.open();
		}
	}
	void close() {
		if (instance.isValid()) {
			instance.close();
		}
	}

	TMixerGuard& operator=(const TMixerGuard&) = delete;
	TMixerGuard(const TMixerGuard&) = delete;

	explicit TMixerGuard(mixer_t& F) : instance(F) {}
	~TMixerGuard() { instance.close(); }
};

} /* namespace music */

#endif /* ALSA_H_ */
