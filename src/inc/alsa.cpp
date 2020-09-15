/*
 * alsa.cpp
 *
 *  Created on: 25.08.2016
 *      Author: Dirk Brinkmeier
 */

#include <iostream>
#include <string>
#include "alsa.h"
#include "bits.h"
#include "threads.h"
#include "random.h"
#include "convert.h"
#include "compare.h"
#include "process.h"
#include "datetime.h"
#include "templates.h"
#include "semaphores.h"
#include "endianutils.h"
#include "audioconsts.h"
#include "audiofile.h"


static void alsaAsyncCallback(snd_async_handler_t *handler) {
    void * ctx = snd_async_handler_get_callback_private(handler);
	if (util::assigned(ctx))
		static_cast<music::TAlsaPlayer*>(ctx)->alsaCallbackHandler();
}

static void* alsaThreadDispatcher(void *thread) {
	if (util::assigned(thread))
		return (void *)(long)(static_cast<music::TAlsaPlayer*>(thread))->alsaThreadHandler();
	return (void *)(long)(EXIT_FAILURE);
}


namespace music {


TByteConverter::TByteConverter() {
}

TByteConverter::~TByteConverter() {
}


void TByteConverter::convert8to8Bit(const int8_t *src, int8_t *dst, size_t size) {
	for (size_t i=0; i<size; ++i)
		dst[i] = src[i];
}

void TByteConverter::convert8to16Bit(const int8_t *src, int16_t *dst, size_t size) {
	for (size_t i=0; i<size; ++i)
		dst[i] = src[i] << 8;
}

void TByteConverter::convert8to24Bit(const int8_t *src, int32_t *dst, size_t size) {
	for (size_t i=0; i<size; ++i)
		dst[i] = src[i] << 16;
}

void TByteConverter::convert8to32Bit(const int8_t *src, int32_t *dst, size_t size) {
	for (size_t i=0; i<size; ++i)
		dst[i] = src[i] << 24;
}

void TByteConverter::convert16to16Bit(const int16_t *src, int16_t *dst, size_t size) {
	for (size_t i=0; i<size; ++i)
		dst[i] = src[i];
}

void TByteConverter::convert16to24Bit(const int16_t *src, int32_t *dst, size_t size) {
	for (size_t i=0; i<size; ++i)
		dst[i] = src[i] << 8;
}

void TByteConverter::convert16to32Bit(const int16_t *src, int32_t *dst, size_t size) {
	for (size_t i=0; i<size; ++i)
		dst[i] = src[i] << 16;
}

void TByteConverter::convert24to24Bit(const int32_t *src, int32_t *dst, size_t size) {
	for (size_t i=0; i<size; ++i)
		dst[i] = src[i];
}

void TByteConverter::convert24to32Bit(const int32_t *src, int32_t *dst, size_t size) {
	for (size_t i=0; i<size; ++i)
		dst[i] = src[i] << 8;
}


void TByteConverter::write16to16Bit(const TSample *src, int16_t *dst, size_t& read, snd_pcm_uframes_t frames) {
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {
		write16to16Bit(src, dst, read);
	}
}

void TByteConverter::write16to24Bit(const TSample *src, int32_t *dst, size_t& read, snd_pcm_uframes_t frames) {
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {
		write16to24Bit(src, dst, read);
	}
}

void TByteConverter::write16to32Bit(const TSample *src, int32_t *dst, size_t& read, snd_pcm_uframes_t frames) {
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {
		write16to32Bit(src, dst, read);
	}
}


void TByteConverter::write24to24Bit(const TSample *src, int32_t *dst, size_t& read, snd_pcm_uframes_t frames) {
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {
		write24to24Bit(src, dst, read);
	}
}

void TByteConverter::write24to32Bit(const TSample *src, int32_t *dst, size_t& read, snd_pcm_uframes_t frames) {
	for (size_t i=0; i<frames; ++i) {
		write24to32Bit(src, dst, read);
	}
}


void TByteConverter::write16to16Bit(const TSample *& src, int16_t *& dst, size_t& read) {
	*(dst++) = *(src++);
	*(dst++) = *(src++) << 8;
	read += 2;
}

void TByteConverter::write16to24Bit(const TSample *& src, int32_t *& dst, size_t& read) {
	*(dst++) = *(src++) << 8;
	*(dst++) += *(src++) << 16;
	read += 2;
}

void TByteConverter::write16to32Bit(const TSample *& src, int32_t *& dst, size_t& read) {
	*(dst++) = *(src++) << 16;
	*(dst++) += *(src++) << 24;
	read += 2;
}


void TByteConverter::write24to24Bit(const TSample *& src, int32_t *& dst, size_t& read) {
	*(dst++) = *(src++);
	*(dst++) += *(src++) << 8;
	*(dst++) += *(src++) << 16;
	read += 3;
}

void TByteConverter::write24to32Bit(const TSample *& src, int32_t *& dst, size_t& read) {
	*(dst++) = *(src++) << 8;
	*(dst++) += *(src++) << 16;
	*(dst++) += *(src++) << 24;
	read += 3;
}



TPCMConverter::TPCMConverter() {
}

TPCMConverter::~TPCMConverter() {
}

TSample TPCMConverter::getRandomDither() {
	return (TSample)util::randomize(0, 255);
}


void TPCMConverter::write16to16Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian) {
	bool isLittleEndian = endian == util::EE_LITTLE_ENDIAN;
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {
		write16to16Bit(src, dst, read, isLittleEndian); // Left channel
		write16to16Bit(src, dst, read, isLittleEndian); // Right channel
	}
}

void TPCMConverter::write16to24Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian, const bool dithered) {
	bool isLittleEndian = endian == util::EE_LITTLE_ENDIAN;
	TSample dither = 0;
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {
		if (dithered) {
			dither = getRandomDither();
		}
		write16to24Bit(src, dst, read, isLittleEndian, dither); // Left channel
		if (dithered) {
			dither = getRandomDither();
		}
		write16to24Bit(src, dst, read, isLittleEndian, dither); // Right channel
	}
}

void TPCMConverter::write16to32Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian, const bool dithered) {
	bool isLittleEndian = endian == util::EE_LITTLE_ENDIAN;
	TSample dither = 0;
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {
		if (dithered) {
			dither = getRandomDither();
		}
		write16to32Bit(src, dst, read, isLittleEndian, dither); // Left channel
		if (dithered) {
			dither = getRandomDither();
		}
		write16to32Bit(src, dst, read, isLittleEndian, dither); // Right channel
	}
}


void TPCMConverter::write24to24Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian) {
	bool isLittleEndian = endian == util::EE_LITTLE_ENDIAN;
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {
		write24to24Bit(src, dst, read, isLittleEndian); // Left channel
		write24to24Bit(src, dst, read, isLittleEndian); // Right channel
	}
}

void TPCMConverter::write24to32Bit(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames, const util::EEndianType endian, const bool dithered) {
	bool isLittleEndian = endian == util::EE_LITTLE_ENDIAN;
	TSample dither = 0;
	for (size_t i=0; i<frames; ++i) {
		if (dithered) {
			dither = getRandomDither();
		}
		write24to32Bit(src, dst, read, isLittleEndian, dither); // Left channel
		if (dithered) {
			dither = getRandomDither();
		}
		write24to32Bit(src, dst, read, isLittleEndian, dither); // Right channel
	}
}


void TPCMConverter::write16to16Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian) {
	if (isLittleEndian) {
		*(dst++) = *(src++);
		*(dst++) = *(src++);
	} else {
		TSample b = *(src++);
		*(dst++) = *(src++);
		*(dst++) = b;
	}
	read += 2;
}

void TPCMConverter::write16to24Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian, const TSample dither) {
	if (isLittleEndian) {
		*(dst++) = dither;
		*(dst++) = *(src++);
		*(dst++) = *(src++);
	} else {
		TSample b = *(src++);
		*(dst++) = *(src++);
		*(dst++) = b;
		*(dst++) = dither;
	}
	read += 2;
}

void TPCMConverter::write16to32Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian, const TSample dither) {
	if (isLittleEndian) {
		*(dst++) = 0;
		*(dst++) = dither;
		*(dst++) = *(src++);
		*(dst++) = *(src++);
	} else {
		TSample b = *(src++);
		*(dst++) = *(src++);
		*(dst++) = b;
		*(dst++) = dither;
		*(dst++) = 0;
	}
	read += 2;
}

void TPCMConverter::write24to24Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian) {
	if (isLittleEndian) {
		*(dst++) = *(src++);;
		*(dst++) = *(src++);
		*(dst++) = *(src++);
	} else {
		TSample b1 = *(src++);
		TSample b2 = *(src++);
		*(dst++) = *(src++);;
		*(dst++) = b2;
		*(dst++) = b1;
	}
	read += 3;
}

void TPCMConverter::write24to32Bit(const TSample *& src, TSample *& dst, size_t& read, const bool isLittleEndian, const TSample dither) {
	if (isLittleEndian) {
		*(dst++) = dither;
		*(dst++) = *(src++);
		*(dst++) = *(src++);
		*(dst++) = *(src++);
	} else {
		TSample b1 = *(src++);
		TSample b2 = *(src++);
		*(dst++) = *(src++);;
		*(dst++) = b2;
		*(dst++) = b1;
		*(dst++) = dither;
	}
	read += 3;
}



void TPCMConverter::write16to16Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian) {
	TSample *p = dst;
	if (isLittleEndian) {
		*(p++) = *(src++);
		*p = *(src++);
	} else {
		TSample b = *(src++);
		*(p++) = *(src++);
		*p = b;
	}
	read += 2;
	dst += step;
}

void TPCMConverter::write16to24Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian, const TSample dither) {
	TSample *p = dst;
	if (isLittleEndian) {
		*(p++) = dither;
		*(p++) = *(src++);
		*p = *(src++);
	} else {
		TSample b = *(src++);
		*(p++) = *(src++);
		*(p++) = b;
		*p = dither;
	}
	read += 2;
	dst += step;
}

void TPCMConverter::write16to32Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian, const TSample dither) {
	TSample *p = dst;
	if (isLittleEndian) {
		*(p++) = 0;
		*(p++) = dither;
		*(p++) = *(src++);
		*p = *(src++);
	} else {
		TSample b = *(src++);
		*(p++) = *(src++);
		*(p++) = b;
		*(p++) = dither;
		*p = 0;
	}
	read += 2;
	dst += step;
}

void TPCMConverter::write24to24Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian) {
	TSample *p = dst;
	if (isLittleEndian) {
		*(p++) = *(src++);;
		*(p++) = *(src++);
		*p = *(src++);
	} else {
		TSample b1 = *(src++);
		TSample b2 = *(src++);
		*(p++) = *(src++);
		*(p++) = b2;
		*p = b1;
	}
	read += 3;
	dst += step;
}

void TPCMConverter::write24to32Bit(const TSample *& src, TSample *& dst, size_t& read, const snd_pcm_int_t step, const bool isLittleEndian, const TSample dither) {
	TSample *p = dst;
	if (isLittleEndian) {
		*(p++) = dither;
		*(p++) = *(src++);
		*(p++) = *(src++);
		*p = *(src++);
	} else {
		TSample b1 = *(src++);
		TSample b2 = *(src++);
		*(p++) = *(src++);
		*(p++) = b2;
		*(p++) = b1;
		*p = dither;
	}
	read += 3;
	dst += step;
}



TAlsaMixer::TAlsaMixer() {
	open();
}

TAlsaMixer::~TAlsaMixer() {
	close();
}

void TAlsaMixer::open() {
	int errval = snd_mixer_open(&smx_handle, 0);
	if (errval != EXIT_SUCCESS)
		smx_handle = nil;
}

void TAlsaMixer::close() {
	if (util::assigned(smx_handle))
		snd_mixer_close(smx_handle);
	smx_handle = nil;
}



TAlsaElementId::TAlsaElementId() {
	init();
}

TAlsaElementId::~TAlsaElementId() {
	clear();
}

void TAlsaElementId::init() {
	snd_mixer_selem_id_alloca(&sid);
}

void TAlsaElementId::clear() {
	if (util::assigned(sid))
		snd_mixer_selem_id_free(sid);
	sid = nil;
}




TAlsaPlayer::TAlsaPlayer() : TThreadAffinity() {
	prime();
}

TAlsaPlayer::~TAlsaPlayer() {
	finalize();
	buffers.clear();
}

void TAlsaPlayer::prime() {
	thread = 0;
	changed = false;
	progressed = false;
	invalidated = false;
	terminate = false;
	running = false;
	started = false;
	streamed = 0;
	e_count = 0;
	m_state = p_state = e_state = EPS_CLOSED;
	onPlaybackProgressChanged = nil;
	onPlaybackPlaylistRequest = nil;
	onPlaybackStateChanged = nil;
	onOutputStateChanged = nil;
	errval = EXIT_SUCCESS;
	m_physicalwidth = 0;
	m_periodtime = 1000000; // 1 second
	m_skipframe = 10; // 10 seconds
	m_buffertime = 0;
	handler = nil;
	logfile = nil;
	verbosity = 0;
	debug = false;
	enabled = false;
	dithered = false;
	alsaMtx.open();
	clear();
}

void TAlsaPlayer::clear() {
	snd_handle = nil;
	hw_params = nil;
	sw_params = nil;
	snd_channels = 0;
	snd_samplerate = 0;
	snd_buffertime = 0;
	snd_buffersize = 0;
	snd_periodtime = 0;
	snd_periodsize = 0;
	snd_framesize = 0;
	snd_datawidth = 0;
	snd_activeperiodtime = 0;
	snd_format = SND_PCM_FORMAT_UNKNOWN;
	snd_physicalwidth = 0;
	periodEvent = false;
	ignoreMixer = false;
	forceReopen = false;
	muted = false;
	volume = 100;
	lastSong.clear();
	device.clear();
	card.clear();
	reset(EPS_CLOSED);
}

void TAlsaPlayer::reset(const EPlayerState state) {
	TPlayerState player;
	buffers.complete(currentSong.song);
	m_endian = util::EE_LITTLE_ENDIAN;
	m_state = p_state /*= e_state*/ = state;
	currentSong.clear();
	dop = false;
	lastSongProgress = 0;
	lastSongStreamed = 0;
	lastSongPlayed = 0;
	setPlayerStateWithNolock(m_state);
	getAndResetPlayerStateWithNolock(player);
	processStateChangeEvents(player, "Reset");
}

void TAlsaPlayer::initialize() {
	if (!enabled) {
		enabled = true;
		sema.open();
		createAlsaThread();
	}
}

void TAlsaPlayer::finalize() {
	if (enabled) {
		enabled = false;
		close();
		terminateAlsaThread();
		snd_config_update_free_global();
	}
}


void TAlsaPlayer::configure(const TAlsaConfig& config) {
	setPeriodTime(config.periodtime);
	setSkipFrame(config.skipframe);
	setVerbosity(config.verbosity);
	setDebug(config.debug);
	setDithered(config.dithered);
	setIgnoreMixer(config.ignoremixer);
	setLogFile(config.logger);
}

void TAlsaPlayer::logger(const std::string& text) {
	if (debug)
		std::cout << text << std::endl;
	if (util::assigned(logfile))
		logfile->write("[Alsa] " + text);
}

void TAlsaPlayer::logger(int level, const std::string& text) {
	if (verbosity >= level)
		logger(text);
}


void TAlsaPlayer::createAlsaThread() {
	createJoinableThread(thread, alsaThreadDispatcher, this);
}


void TAlsaPlayer::terminateAlsaThread() {
	if (started) {
		started = false;
		terminate = true;
		sema.post();
		int r = terminateThread(thread);
		running = false;
		if (util::checkFailed(r))
			throw util::sys_error("TAlsaPlayer::terminateAlsaThread: pthread_join() failed.");
	}
}


void TAlsaPlayer::formatOutput(const std::string& preamble) const {
	int format;
	std::cout << preamble << "Supported ALSA sound formats:" << std::endl;
    for (format = 0; format <= SND_PCM_FORMAT_LAST; ++format) {
       	std::cout << preamble << "  [" << format << "] " << formatToStr((snd_pcm_format_t)format) << std::endl;
    }
}

void TAlsaPlayer::parameterOutput(const std::string& preamble) const {
	std::cout << preamble << "ALSA parameter for device <" << device << ">" << std::endl;
	std::cout << preamble << "  Format      = " << formatToStr(snd_format) << std::endl;
	std::cout << preamble << "  Channels    = " << snd_channels << std::endl;
	std::cout << preamble << "  Phys. width = " << snd_physicalwidth << " Bits" << std::endl;
	std::cout << preamble << "  Data width  = " << snd_datawidth << " Bits" << std::endl;
	std::cout << preamble << "  Frame size  = " << snd_framesize << " Byte" << std::endl;
	std::cout << preamble << "  Sample rate = " << snd_samplerate << " Samples/sec" << std::endl;
	std::cout << preamble << "  Period time = " << (snd_periodtime / 1000) << " ms" << std::endl;
	std::cout << preamble << "  Period size = " << snd_periodsize << " Frames, " << snd_periodsize * snd_framesize << " Byte [" << util::sizeToStr(snd_periodsize * snd_framesize, 1, util::VD_BINARY) << "]" << std::endl;
	std::cout << preamble << "  Buffer time = " << (snd_buffertime / 1000) << " ms" << std::endl;
	std::cout << preamble << "  Buffer size = " << snd_buffersize << " Frames, " << snd_buffersize * snd_framesize << " Byte [" << util::sizeToStr(snd_buffersize * snd_framesize, 1, util::VD_BINARY) << "]" << std::endl;
}

std::string TAlsaPlayer::statusToStr(const EPlayerState value) {
	const struct TPlayerStateName *it;
	for (it = playerstates; util::assigned(it->name); ++it) {
		if (it->state == value) {
			if (util::assigned(it->name))
				return it->name;
		}
	}
	return "Unknown";
}

std::string TAlsaPlayer::bufferToStr(const EBufferState value) {
	const struct TBufferStateName *it;
	for (it = bufferstates; util::assigned(it->name); ++it) {
		if (it->state == value) {
			if (util::assigned(it->name))
				return it->name;
		}
	}
	return "Unknown";
}

std::string TAlsaPlayer::levelToStr(const EBufferLevel value) {
	const struct TBufferLevelName *it;
	for (it = levelstates; util::assigned(it->name); ++it) {
		if (it->level == value) {
			if (util::assigned(it->name))
				return it->name;
		}
	}
	return "Unknown";
}

std::string TAlsaPlayer::commandToStr(const EPlayerCommand value) {
	const struct TPlayerCommandName *it;
	for (it = commands; util::assigned(it->name); ++it) {
		if (it->command == value) {
			if (util::assigned(it->name))
				return it->name;
		}
	}
	return "Unknown";
}

std::string TAlsaPlayer::formatToStr(const snd_pcm_format_t format) const {
    std::string s = "<not supported>";
	const char *p = snd_pcm_format_name(format);
    if (util::assigned(p)) {
    	s = std::string(p);
    	const char *d = snd_pcm_format_description(format);
		if (util::assigned(d))
			s += " [" + std::string(d) + "]";
    }
    return s;
}

snd_pcm_format_t TAlsaPlayer::getHardwareFormat() {

	// Get possible hardware format
	snd_pcm_format_t val = SND_PCM_FORMAT_UNKNOWN;
	errval = snd_pcm_hw_params_get_format(hw_params, &val);

	// Check against possible endian modes
	if (success() && val != SND_PCM_FORMAT_UNKNOWN) {
		logger("[Hardware] Format reported from hardware is " + formatToStr(val) + " (" + std::to_string((size_s)val) + ")");
		util::EEndianType endian = getEndianFromFormat(val);
		if (endian != util::EE_UNKNOWN_ENDIAN)
			return val;
	}

	// No valid hardware format found
	return SND_PCM_FORMAT_UNKNOWN;
}

std::string TAlsaPlayer::getHardwareCard() const {
    std::string s = device;
    if (!device.empty()) {
    	size_t n = device.find_first_of(',');
    	if (n != std::string::npos) {
    		if (n > 1)
    			s = device.substr(0, n);
    	}
    }
    return s;
}

bool TAlsaPlayer::setHardwareFormat(const snd_pcm_format_t format) {
	bool ok = false;
	snd_format = format;
	if (format != SND_PCM_FORMAT_UNKNOWN) {
		util::EEndianType endian = getEndianFromFormat(format);
		if (endian != util::EE_UNKNOWN_ENDIAN) {
			m_endian = endian;
			int width = snd_pcm_format_physical_width(snd_format);
			if (width > 0) {
				snd_physicalwidth = m_physicalwidth = width;
				snd_framesize = snd_physicalwidth * snd_channels / 8;
				ok = true;
			}
		}
	}
	if (!ok) {
		m_endian = util::EE_UNKNOWN_ENDIAN;
		snd_format = SND_PCM_FORMAT_UNKNOWN;
		snd_physicalwidth = m_physicalwidth = 0;
		snd_framesize = 0;
	}
	return ok;
}

bool TAlsaPlayer::open(const CStreamData& stream) {
	return open(device, stream);
}

bool TAlsaPlayer::open(const std::string& device, const CStreamData& stream) {
	if (false) { //!running) {
		errmsg = "ALSA thread not initialized.";
		errval = -EAGAIN;
		return false;
	}
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return openDevice(device, stream);
}


bool TAlsaPlayer::openDevice(const std::string& device, const CStreamData& stream) {
	errval = EXIT_SUCCESS;
	bool retVal = false;
	if (!getOpen()) {
		if (!device.empty()) {
			bool retry = false;

			// Try to use little endian data transfer to sound card
    		logger("[Open] [LITTLE_ENDIAN]");
			if (setAlsaParams(stream, util::EE_LITTLE_ENDIAN)) {

				// Retrieve PCM sound handle
				errval = snd_pcm_open(&snd_handle, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
				if (success() && util::assigned(snd_handle)) {

					// Initialize hardware parameter
		    		if (setHardwareParams(SND_PCM_ACCESS_MMAP_INTERLEAVED)) {

						// Initialize software parameter
						if (setSoftwareParams()) {

							// Open device was successful
							this->device = device;
							card = getHardwareCard();

							// Set master volumes to 100%
							bool r = setMasterVolume(100);
							retVal = ignoreMixer ? true : r;

						}

					} else {
			    		logger("[Open] [LITTLE_ENDIAN] failed \"" + errmsg + "\"");
						retry = true;
					}

				} else {
					errmsg = util::csnprintf("TAlsaPlayer::openDevice() Device $ open failed.", device);
				}

			} else {
				errmsg = "TAlsaPlayer::openDevice() Invalid stream parameter (LITTLE_ENDIAN)";
				errval = -EINVAL;
			}

			if (retry) {

				// Close device before retry
				closeDevice();

				// Falback to big endian data transfer to sound card
	    		logger("[Open] [BIG_ENDIAN]");
				if (setAlsaParams(stream, util::EE_BIG_ENDIAN)) {

					// Retrieve PCM sound handle
					errval = snd_pcm_open(&snd_handle, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
					if (success() && util::assigned(snd_handle)) {

						// Initialize hardware parameter
			    		if (setHardwareParams(SND_PCM_ACCESS_MMAP_INTERLEAVED)) {

							// Initialize software parameter
							if (setSoftwareParams()) {

								// Open device was successful
								this->device = device;
								card = getHardwareCard();

								// Set master volumes to 100%
								bool r = setMasterVolume(100);
								retVal = ignoreMixer ? true : r;

							}

						} else {
				    		logger("[Open] [BIG_ENDIAN] failed \"" + errmsg + "\"");
							retry = true;
						}

					} else {
						errmsg = util::csnprintf("TAlsaPlayer::openDevice() Device $ open failed.", device);
					}

				} else {
					errmsg = "TAlsaPlayer::openDevice() Invalid stream parameter (BIG_ENDIAN)";
					errval = -EINVAL;
				}
			}

		} else {
			errmsg = "TAlsaPlayer::openDevice() No device.";
			errval = -ENODEV;
		}
	} else {
		errval = -EBUSY;
		errmsg = util::csnprintf("TAlsaPlayer::openDevice() Device $ is busy.", device);
	}

	if (retVal) {
		logger("[Open] Open <" + device + "> succeeded [" + card + "]");
		logger("[Open] Format " + formatToStr(snd_format));
		logger("[Open] Samplerate " + std::to_string((size_s)stream.sampleRate) + " Samples/sec");
	} else {
		logger("[Open] Open <" + device + "> failed \"" + errmsg + "\" (" + std::to_string((size_s)errval) + ")");
		closeDevice();
	}

	// Open sound device failed...
	return retVal;
}

void TAlsaPlayer::close() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	closeDevice();
	if (buffers.count() > 0)
		buffers.cleanup();
}

void TAlsaPlayer::closeDevice() {
	if (getOpen()) {
		dropDevice();
		snd_pcm_close(snd_handle);
		logger("[Close] Closed sound device <" + util::strToStr(device, "none") + ">");
	}
	lastSong.clear();
	clear();
}

bool TAlsaPlayer::dropDevice() {
	errval = EXIT_SUCCESS;
	m_state = EPS_CLOSED;
	errval = snd_pcm_drop(snd_handle);
	if (success()) {
		int c = 10;
		bool waited = false;
		snd_pcm_state_t state;
		do {
			state = snd_pcm_state(snd_handle);
			if (state != SND_PCM_STATE_SETUP) {
				util::wait(10);
				waited = true;
			}
			--c;
		} while (state != SND_PCM_STATE_SETUP && c > 0);
		if (!waited)
			util::wait(10);
	} else {
		m_state = EPS_ERROR;
		errmsg = "TAlsaPlayer::dropDevice() Drop device buffers failed.";
	}
	return success();
}

bool TAlsaPlayer::play(PSong song, const std::string& playlist, const std::string& device, bool& isOpen) {
	bool retVal = false;
	TPlayerState player;
	{
		app::TLockGuard<app::TMutex> lock(alsaMtx);
		errval = EXIT_SUCCESS;
		isOpen = getOpen();

		// Check prerequisites
		if (device.empty()) {
			errmsg = "TAlsaPlayer::play() No valid device to play song.";
			errval = -ENODEV;
			return false;
		}
		if (!util::assigned(song)) {
			errmsg = "TAlsaPlayer::play() No Song to play.";
			errval = -EINVAL;
			return false;
		}
		if (!song->getStreamData().isValid()) {
			errmsg = util::csnprintf("TAlsaPlayer::play() Invalid stream data for song $", song->getTitle());
			errval = -EINVAL;
			return false;
		}

		// Check for device change
		// --> Close player and reopen on next call
		if (!getDevice().empty()) {
			if (0 != util::strncasecmp(device, getDevice(), device.size())) {
				// Close device and set current song buffers as played
				errmsg = util::csnprintf("TAlsaPlayer::play() Device changed from $ to $", getDevice(), device);
				closeDevice();
				errval = -ENODEV;
				isOpen = false;
				return false;
			}
		}

		// Check for identical stream parameter of given song compared to current song
		// --> Close player and reopen on next call
		if (getOpen()) {
			bool reopen = false;
			if (forceReopen) {
				// Reopen was requested by playlist queue
				forceReopen = false;
				reopen = true;
			} else {
				// Compare hardware format to previous song
				if (util::assigned(lastSong.song)) {
					if (song->getStreamData().isValid()) {
						if (song->isChanged(true) || song->getStreamData() != lastSong.song->getStreamData()) {
							reopen = true;
						}
					}
				} else {
					// Last stream is unknown
					// --> Force reopen with current stream properties
					reopen = true;
				}
			}
			if (reopen) {
				// Close device and set current song buffers as played
				if (util::assigned(lastSong.song))
					errmsg = util::csnprintf("TAlsaPlayer::play() Current song $ has different stream than previous song $", song->getTitle(), lastSong.song->getTitle());
				else
					errmsg = util::csnprintf("TAlsaPlayer::play() Current song $ has no predecessor: Closing device to setup new stream.", song->getTitle());
				closeDevice();
				errval = -EPIPE;
				isOpen = false;
				return false;
			}
		}

		// Open device if needed
		bool opened = false;
		if (!getOpen()) {
			if (!openDevice(device, song->getStreamData())) {
				//setPlayerStateWithNolock(EPS_ERROR);
				//executeStatusChangeWithNolock(EPS_ERROR, currentSong);
				setErrorStateWithNolock();
				isOpen = false;
				return false;
			}
			// Initial open on ALSA device successful
			opened = true;
		}

		// Is device open now?
		if (getOpen()) {
			isOpen = true;

			// Cleanup current song object to be playable again from scratch
			if (!song->isStreamed()) {
				buffers.complete(currentSong.song);
				buffers.cleanup(song);
			}

			// Set current song to play by ALSA
			lastSong.song = song;
			lastSong.playlist = playlist;
			currentSong.song = song;
			currentSong.playlist = playlist;

			if (debug && verbosity >= 3)
				parameterOutput("TAlsaPlayer::play() ");

			// Was ALSA device opened before?
			if (opened) {

				// Start playback on closed device by writing period data to device before starting playback
				if (!writePeriodData()) {
					//setPlayerStateWithNolock(EPS_ERROR);
					//executeStatusChangeWithNolock(EPS_ERROR, currentSong);
					setErrorStateWithNolock();
					return false;
				}

				// Start playback on newly opened device
				errval = snd_pcm_start(snd_handle);
				if (!success()) {
					errmsg = "TAlsaPlayer::play() Starting ALSA stream failed.";
					//setPlayerStateWithNolock(EPS_ERROR);
					//executeStatusChangeWithNolock(EPS_ERROR, currentSong);
					setErrorStateWithNolock();
					return false;
				}

			}

			// ALSA seems to play stream...
			setPlayerStateWithNolock(EPS_PLAY);
			retVal = true;

		}

		// Get current state for further processing of events...
		getAndResetPlayerStateWithNolock(player);

	}

	// Trigger asynchronous event callback
	processStateChangeEvents(player, "Play");

	return retVal;
}

bool TAlsaPlayer::stop() {
	bool retVal = false;
	TPlayerState player;
	{
		app::TLockGuard<app::TMutex> lock(alsaMtx);
		if (m_state == EPS_STOP) {
			closeDevice();
			if (m_state == EPS_CLOSED)
				retVal = true;
		} else {
			retVal = stopDevice();
		}

		// Get current state for further processing of events...
		getAndResetPlayerStateWithNolock(player);
	}

	// Trigger asynchronous event callback
	processStateChangeEvents(player, "Stop");

	return retVal;
}

bool TAlsaPlayer::stopDevice() {
	if (util::isMemberOf(m_state, EPS_PLAY,EPS_PAUSE,EPS_HALT,EPS_WAIT,EPS_REOPEN,EPS_ERROR)) {
		// Mark current song as played in buffers and reset ALSA state
		EPlayerState state = getOpen() ? EPS_STOP : EPS_CLOSED;
		reset(state);
		return true;
	}
	if (!util::isMemberOf(m_state, EPS_STOP,EPS_CLOSED)) {
		errmsg = "TAlsaPlayer::stopDevice() Stopping not possible, invalid state <" + statusToStr(m_state) + ">";
		errval = -EBUSY;
		return false;
	}
	return true;
}

bool TAlsaPlayer::pause() {
	bool retVal = false;
	TPlayerState player;
	{
		app::TLockGuard<app::TMutex> lock(alsaMtx);
		retVal = pauseDevice();

		// Get current state for further processing of events...
		getAndResetPlayerStateWithNolock(player);
	}

	// Trigger asynchronous event callback
	processStateChangeEvents(player, "Pause");

	return retVal;
}

bool TAlsaPlayer::pauseDevice() {
	if (util::isMemberOf(m_state, EPS_PLAY,EPS_WAIT)) {
		p_state = m_state;
		setPlayerStateWithNolock(EPS_PAUSE);
		return true;
	}
	if (m_state != EPS_PAUSE) {
		errmsg = "TAlsaPlayer::pauseDevice() Pausing not possible, invalid state <" + statusToStr(m_state) + ">";
		errval = -EBUSY;
		return false;
	}
	return true;
}

bool TAlsaPlayer::resume() {
	bool retVal = false;
	TPlayerState player;
	{
		app::TLockGuard<app::TMutex> lock(alsaMtx);
		retVal = resumeDevice();

		// Get current state for further processing of events...
		getAndResetPlayerStateWithNolock(player);
	}

	// Trigger asynchronous event callback
	processStateChangeEvents(player, "Resume");

	return retVal;
}

bool TAlsaPlayer::resumeDevice() {
	if (m_state == EPS_PAUSE) {
		m_state = (p_state != EPS_IDLE) ? p_state : EPS_PLAY;
		p_state = EPS_IDLE;
		setPlayerStateWithNolock(m_state);
		return true;
	}
	errmsg = "TAlsaPlayer::resumeDevice() Resume not possible, invalid state.";
	errval = -EBUSY;
	return false;
}

bool TAlsaPlayer::halt() {
	bool retVal = false;
	TPlayerState player;
	{
		app::TLockGuard<app::TMutex> lock(alsaMtx);
		retVal = haltDevice();

		// Get current state for further processing of events...
		getAndResetPlayerStateWithNolock(player);
	}

	// Trigger asynchronous event callback
	processStateChangeEvents(player, "Halt");

	return retVal;
}

bool TAlsaPlayer::haltDevice() {
	if (util::isMemberOf(m_state, EPS_PLAY,EPS_WAIT)) {
		p_state = m_state;
		setPlayerStateWithNolock(EPS_HALT);
		return true;
	}
	if (m_state != EPS_HALT) {
		errmsg = "TAlsaPlayer::haltDevice() Halt not possible, invalid state <" + statusToStr(m_state) + ">";
		errval = -EBUSY;
		return false;
	}
	return true;
}

bool TAlsaPlayer::restart() {
	bool retVal = false;
	TPlayerState player;
	{
		app::TLockGuard<app::TMutex> lock(alsaMtx);
		retVal = resumeDevice();

		// Get current state for further processing of events...
		getAndResetPlayerStateWithNolock(player);
	}

	// Trigger asynchronous event callback
	processStateChangeEvents(player, "Restart");

	return retVal;
}

bool TAlsaPlayer::restartDevice() {
	if (m_state == EPS_HALT) {
		m_state = (p_state != EPS_IDLE) ? p_state : EPS_PLAY;
		p_state = EPS_IDLE;
		setPlayerStateWithNolock(m_state);
		return true;
	}
	errmsg = "TAlsaPlayer::restartDevice() Restart not possible, invalid state.";
	errval = -EBUSY;
	return false;
}

bool TAlsaPlayer::toggle(EPlayerState& state) {
	bool retVal = false;
	TPlayerState player;
	{
		app::TLockGuard<app::TMutex> lock(alsaMtx);
		retVal = toggleDevice(state);

		// Get current state for further processing of events...
		getAndResetPlayerStateWithNolock(player);
	}

	// Trigger asynchronous event callback
	processStateChangeEvents(player, "Toggle");

	return retVal;
}

bool TAlsaPlayer::toggleDevice(EPlayerState& state) {
	if (util::isMemberOf(m_state, EPS_PLAY,EPS_WAIT)) {
		state = EPS_PAUSE;
		p_state = m_state;
		setPlayerStateWithNolock(state);
		return true;
	}
	if (m_state == EPS_PAUSE) {
		state = (p_state != EPS_IDLE) ? p_state : EPS_PLAY;
		p_state = EPS_IDLE;
		setPlayerStateWithNolock(state);
		return true;
	}
	errmsg = "TAlsaPlayer::toggleDevice() Toggle pause mode not possible, invalid state.";
	errval = -EBUSY;
	state = EPS_ERROR;
	return false;
}


void TAlsaPlayer::addStreamCommand(const EPlayerCommand command) {
	if (m_state == EPS_PLAY) {
		TPlayerCommand cmd;
		cmd.command = command;
		queue.add(cmd);
	}
}

void TAlsaPlayer::addSeekCommand(const double position) {
	if (m_state == EPS_PLAY) {
		TPlayerCommand cmd;
		cmd.value = position;
		cmd.command = EPP_POSITION;
		queue.add(cmd);
	}
}

void TAlsaPlayer::clearStreamCommands() {
	queue.clear();
}


void TAlsaPlayer::updatePlayerState(TPlayerState& state, const EPlayerState value) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	updatePlayerStateWithNolock(state, value);
};

void TAlsaPlayer::updatePlayerStateWithNolock(TPlayerState& state, const EPlayerState value) {
	state.state = m_state = value;
	state.invalidated = invalidated = true;
	if (verbosity > 2) logger("[State] Update player state <" + statusToStr(state.state) + ">");
};


void TAlsaPlayer::setPlayerState(const EPlayerState value) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	setPlayerStateWithNolock(value);
};

void TAlsaPlayer::setPlayerStateWithNolock(const EPlayerState value) {
	m_state = value;
	invalidated = true;
	if (verbosity > 2) logger("[State] Set player state <" + statusToStr(value) + ">");
};


void TAlsaPlayer::setErrorState() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	setErrorStateWithNolock();
};

void TAlsaPlayer::setErrorStateWithNolock() {
	setPlayerStateWithNolock(EPS_ERROR);
	executeStatusChangeWithNolock(m_state, currentSong);
};


void TAlsaPlayer::executeStatusChange() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	executeStatusChangeWithNolock(m_state, currentSong);
};

void TAlsaPlayer::executeStatusChangeWithNolock(const EPlayerState value, const TCurrentSong& current) {
	onStateChangedCallback(value, current);
	if (verbosity > 2) logger("[State] Executed status change <" + statusToStr(value) + ">");
};


void TAlsaPlayer::executeOutputChange() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	executeOutputChangeWithNolock(m_state, currentSong);
};

void TAlsaPlayer::executeOutputChangeWithNolock(const EPlayerState value, const TCurrentSong& current) {
	onOutputChangedCallback(value, current);
};


void TAlsaPlayer::detectOutputChange(const EPlayerState& value) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	detectOutputChangeWithNolock(value);
};

void TAlsaPlayer::detectOutputChangeWithNolock(const EPlayerState& value) {
	if (value != e_state) {
		++e_count;
		// Looks like ALSA calls interrupt 4 times per period time...
		if (e_count > 3) {
			e_count = 0;
			e_state = value;
			changed = true;
		}
	} else {
		e_count = 0;
	}
};


void TAlsaPlayer::getPlayerState(TPlayerState& state) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	getPlayerStateWithNolock(state);
};

void TAlsaPlayer::getPlayerStateWithNolock(TPlayerState& state) {
	state.state = m_state;
	state.current = currentSong;
	state.invalidated = invalidated;
	state.progressed = progressed;
	state.changed = changed;
};

void TAlsaPlayer::getAndResetPlayerState(TPlayerState& state) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	getAndResetPlayerStateWithNolock(state);
};

void TAlsaPlayer::getAndResetPlayerStateWithNolock(TPlayerState& state) {
	state.state = m_state;
	state.current = currentSong;
	state.invalidated = invalidated;
	state.progressed = progressed;
	state.changed = changed;
	invalidated = false;
	progressed = false;
	changed = false;
	if (verbosity > 2) logger("[State] Get and reset state <" + statusToStr(state.state) + ">");
};

void TAlsaPlayer::executeProgressChange() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	executeProgressChangeWithNolock(currentSong);
};

void TAlsaPlayer::executeProgressChangeWithNolock(const TCurrentSong& current) {
	if (nil != onPlaybackProgressChanged) {
		onProgressChangedCallback(current);
	}
};


void TAlsaPlayer::getLastSong(CCurrentSong& song) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	song = lastSong;
}

void TAlsaPlayer::getLastSong(CSongData& song) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	getSongProperties(lastSong.song, song);
}

void TAlsaPlayer::getLastSong(CSongData& song, std::string& playlist) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	getSongProperties(lastSong.song, song);
	playlist = lastSong.playlist;
}

void TAlsaPlayer::getCurrentSong(CCurrentSong& song) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	song = currentSong;
}

void TAlsaPlayer::getCurrentSong(CSongData& song) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	getSongProperties(currentSong.song, song);
}

void TAlsaPlayer::getCurrentSong(CSongData& song, std::string& playlist) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	getSongProperties(currentSong.song, song);
	playlist = currentSong.playlist;
}

void TAlsaPlayer::getSongProperties(const music::TSong* song, music::CSongData& data) const {
	data.clear();
	if (util::assigned(song)) {
		data.artist = song->getArtist();
		data.album = song->getAlbum();
		data.title = song->getTitle();

		data.track = song->getTrackNumber();
		data.disk = song->getDiskNumber();
		data.tracks = song->getTrackCount();
		data.disks = song->getDiskCount();

		data.titleHash = song->getTitleHash();
		data.albumHash = song->getAlbumHash();
		data.artistHash = song->getArtistHash();
		data.fileHash = song->getFileHash();

		data.duration = song->getDuration();
		data.played = song->getPlayed();
		data.progress = song->getPercent();

		data.index = song->getIndex();
		data.valid = true;
	}
}

bool TAlsaPlayer::isOpen() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return getOpen();
};

bool TAlsaPlayer::isPlaying() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return getPlaying();
};

bool TAlsaPlayer::isPaused() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return getPaused();
};

bool TAlsaPlayer::isHalted() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return getHalted();
};

bool TAlsaPlayer::isStopped() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return getStopped();
};

bool TAlsaPlayer::isDoP() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return getDoP();
};

bool TAlsaPlayer::isDithered() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return dithered && (snd_physicalwidth > snd_datawidth);
};

void TAlsaPlayer::getBitDepth(std::string& bits) const {
	// Mutex will cause deadlock in callback methods, so be patient where to use this method!
	// app::TLockGuard<app::TMutex> lock(alsaMtx);
	if (dithered && (snd_physicalwidth > snd_datawidth) && (snd_datawidth > 7)) {
		bits = std::to_string(snd_datawidth) + "/" + std::to_string(snd_physicalwidth);
	} else {
		bits = std::to_string(snd_datawidth);
	}
};

EPlayerState TAlsaPlayer::getCurrentState() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return m_state;
};

const std::string& TAlsaPlayer::getCurrentDevice() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return device;
};

const std::string& TAlsaPlayer::getCurrentPlaylist() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return currentSong.playlist;
};

int64_t TAlsaPlayer::getStreamed() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return streamed;
};

void TAlsaPlayer::invalidate() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	invalidated = true;
};

int TAlsaPlayer::getAlsaHardwareDevices(util::TStringList& devices, util::TStringList& ignore) {
	devices.clear();
	int card = -1; // Start with first card
	int index = 0;
	int result = 0;
	void **hints;
	void **it;
	char *longname = nil;
	char *shortname = nil;
	char *name = nil;
	char *desc = nil;
	char *token = nil;
	char *io = nil;;
	const char *ifaces[] = {"card", "pcm", "rawmidi", "timer", "seq", "hwdep", nil};
	bool debug = false;

	// Force update of sound configuration tree
	snd_config_update();

	// Walk through cards
	for (;;) {

		// Get next sound card
		if ((result = snd_card_next(&card)) < 0) {
			break;
		}

		// No more cards?
		if (card < 0)
			break;

		if (EXIT_SUCCESS == snd_card_get_name(card, &shortname)) {
			if (debug) std::cout << "TAlsaPlayer::getSoundCards() Index (" << card << ") Shortname   <" << shortname << ">" << std::endl;
		}
		if (EXIT_SUCCESS == snd_card_get_longname(card, &longname)) {
			if (debug) std::cout << "TAlsaPlayer::getSoundCards() Index (" << card << ") Longname    <" << longname << ">" << std::endl;
		}

		// Get hints for given card
		index = 0;
		while (util::assigned(ifaces[index])) {
			if (EXIT_SUCCESS == snd_device_name_hint(card, ifaces[index], &hints)) {
				it = hints;

				while (*it) {

					bool add = false;
					name = snd_device_name_get_hint(*it, "NAME");
					desc = snd_device_name_get_hint(*it, "DESC");
					io = snd_device_name_get_hint(*it, "IOID");
					if (debug) std::cout << "TAlsaPlayer::getSoundCards() Index (" << card << ") Name        <" << name << ">" << std::endl;

					// Add hardware devices only?
					bool all = true;
					if (all || util::assigned(strcasestr(name, "DEV=0"))) {
						if (0 == strncasecmp(name, "hw:", 3)) {
							add = true;
						}
					}

					// Walk through comments, separated by '\n'
					util::TStringList comments;
					comments.add(std::string(shortname));
					token = strtok(desc, "\n");
					while (token != NULL) {
						if (debug) std::cout << "TAlsaPlayer::getSoundCards() Index (" << card << ") Description <" << token << ">" << std::endl;
						if (add) {
							// Display hardware devices as "BitPerfect"
							std::string entry;
							if (util::assigned(strcasestr(token, "direct hardware"))) {
								entry = "BitPerfect";
							} else {
								entry = std::string(token);
							}
							comments.add(entry);
						}
						token = strtok(NULL, "\n");
					}

					// Create comment from descriptions and shortname
					std::string comment;
					if (!comments.empty()) {
						comments.distinct(util::EC_COMPARE_PARTIAL);
						size_t last = util::pred(comments.size());
						for (size_t i=0; i<comments.size(); ++i) {
							if (i < last) {
								comment += comments[i] + " / ";
							} else {
								comment += comments[i];
							}
						}
					}

					// Ignore card name?
					if (add && !ignore.empty()) {
						for (size_t j=0; j<ignore.size(); ++j) {
							if (util::strcasestr(name, ignore[j])) {
								add = false;
								break;
							}
						}
					}

					// Add card name + comment
					if (add) {
						std::string entry;
						if (!comment.empty()) {
							entry = std::string(name) + " [" + comment + "]";
						} else {
							entry = std::string(name) + " [" + std::string(shortname) + "]";
						}
						devices.add(entry);
					}

					// Device is "Input" or "Output"
					if (util::assigned(io)) {
						if (debug) std::cout << "TAlsaPlayer::getSoundCards() Index (" << card << ") IO          <" << io << ">" << std::endl;
						free(io);
					}

					if (util::assigned(name))
						free(name);
					if (util::assigned(desc))
						free(desc);
					++it;
				}

				snd_device_name_free_hint(hints);
				++index;
			}
		}

		if (debug) std::cout << std::endl;
	}

	if (util::assigned(longname))
		free(longname);
	if (util::assigned(shortname))
		free(shortname);

	if (!devices.empty()) {
		devices.sort();
		if (debug) devices.debugOutput();
	}

	return devices.size();
}



bool TAlsaPlayer::setAlsaParams(const CStreamData& stream, const util::EEndianType endian) {
	//
	// Set parameters for ALSA hard- and software paramters
	// from given stream parameters
	//
	// snd_pcm_format_t 	snd_format;
	// unsigned int 		snd_channels;
	// unsigned int 		snd_samplerate;
	// unsigned int 		snd_buffertime;
	// snd_pcm_uframes_t	snd_buffersize;
	// unsigned int 		snd_periodtime;
	// snd_pcm_uframes_t	snd_periodsize;
	//
	if (stream.isValid()) {

		// Choose endianess
		m_endian = endian;
		bool little = isLE();

		// Take data width from stream property
		snd_datawidth = stream.bitsPerSample;
		switch (snd_datawidth) {
			case 1: // Newest endian
			case 2: // Oldest endian
				// DSD for DoP transfer 16 Bits at  a time + 1 Byte DoP marker
				snd_dataframesize = stream.channels * 16 / 8;
				m_physicalwidth = 3 * 8;
				break;
			default:
				snd_dataframesize = stream.channels * snd_datawidth / 8;
				m_physicalwidth = snd_datawidth;
				break;
		}

		// Compose PCM format from given parameters
		snd_pcm_format_t format = snd_pcm_build_linear_format(
				m_physicalwidth,
				m_physicalwidth,
				0,  // 0 == signed!
				little ? 0 : 1); // 0 == LE!
		logger("[Params] Suggested ALSA hardware format is " + formatToStr(format) + " (" + std::to_string((size_s)format) + ")");

		// Set format if valid...
		if (format == SND_PCM_FORMAT_UNKNOWN) {

			// Guess a suggestion, don't know if hardware supports it at this point
			switch(snd_datawidth) {
				// DSD for DoP needs a least 24 Bits physical size
				case 1: // Newest endian
				case 2: // Oldest endian
					switch(m_physicalwidth) {
						case 0:
						case 24:
							format = little ? SND_PCM_FORMAT_S24_LE : SND_PCM_FORMAT_S24_BE;
							break;
						case 32:
							format = little ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_S32_BE;
							break;
						default:
							break;
					}
					break;
				case 16:
					switch(m_physicalwidth) {
						case 0:
						case 16:
							format = little ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_S16_BE;
							break;
						case 24:
							format = little ? SND_PCM_FORMAT_S24_LE : SND_PCM_FORMAT_S24_BE;
							break;
						case 32:
							format = little ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_S32_BE;
							break;
						default:
							break;
					}
					break;
				case 24:
					switch(m_physicalwidth) {
						case 0:
						case 24:
							format = little ? SND_PCM_FORMAT_S24_LE : SND_PCM_FORMAT_S24_BE;
							break;
						case 32:
							format = little ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_S32_BE;
							break;
						default:
							break;
					}
					break;
				case 32:
					switch(m_physicalwidth) {
						case 0:
						case 32:
							format = little ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_S32_BE;
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
		}

		// Use given period time in us or set to 1 sec
		// --> 1000000 us = 1000 ms = 1 sec
		if (m_periodtime > 0)
			snd_periodtime = m_periodtime;
		else
			snd_periodtime = 1000000;

		// Use given buffer time
		// or set buffer time to 4 * period time
		if (m_buffertime > 0 && m_buffertime > m_periodtime)
			snd_buffertime = m_buffertime;
		else
			snd_buffertime = snd_periodtime * 4;

		// Set needed stream parameters needed for further processing...
		snd_channels = stream.channels;
		snd_samplerate = stream.sampleRate;

		// Set hardware format parameter
		bool r = setHardwareFormat(format);

		// Check parameters
		if (r && validStreamParams()) {
			this->stream = stream;
			return true;
		}
	}
	return false;
}

bool TAlsaPlayer::validStreamParams() {
	return (util::EE_UNKNOWN_ENDIAN != getEndianFromFormat(snd_format)) &&
			(snd_channels > 0) &&
			(snd_samplerate > 0) &&
			(snd_buffertime > 0) &&  // Ring buffer length in us, e.g. 500000 (us) --> 500 ms
			// Returned by Hardware! (snd_buffersize > 0) &&
			// Returned by Hardware! (snd_periodsize > 0) &&
			(snd_periodtime > 0); // Period size = Sample rate * Expected period time in sec
}

util::EEndianType TAlsaPlayer::getEndianFromFormat(const snd_pcm_format_t format) {
	switch (format) {
		case SND_PCM_FORMAT_S16_LE:
		case SND_PCM_FORMAT_S24_LE:
		case SND_PCM_FORMAT_S32_LE:
		case SND_PCM_FORMAT_S24_3LE:
		case SND_PCM_FORMAT_U16_LE:
		case SND_PCM_FORMAT_U24_LE:
		case SND_PCM_FORMAT_U32_LE:
		case SND_PCM_FORMAT_U24_3LE:
			return util::EE_LITTLE_ENDIAN;
			break;
		case SND_PCM_FORMAT_S16_BE:
		case SND_PCM_FORMAT_S24_BE:
		case SND_PCM_FORMAT_S32_BE:
		case SND_PCM_FORMAT_S24_3BE:
		case SND_PCM_FORMAT_U16_BE:
		case SND_PCM_FORMAT_U24_BE:
		case SND_PCM_FORMAT_U32_BE:
		case SND_PCM_FORMAT_U24_3BE:
			return util::EE_BIG_ENDIAN;
			break;
		default:
			break;
	}
	return util::EE_UNKNOWN_ENDIAN;
}

bool TAlsaPlayer::setHardwareParams(snd_pcm_access_t access) {
	errval = -EINVAL;

	// Allocate memory for hardware parameter
	snd_pcm_hw_params_alloca(&hw_params);
	if (!util::assigned(hw_params)) {
		errmsg = "TAlsaPlayer::setHardwareParams() Hardware parameter initialization failed.";
		errval = -EFAULT;
		return false;
	}

	// Choose all parameters
	errval = snd_pcm_hw_params_any(snd_handle, hw_params);
	if (!success()) {
		errmsg = "TAlsaPlayer::setHardwareParams() Broken configuration for playback: No configurations available.";
		return false;
	}

	// Disable hardware resampling
	errval = snd_pcm_hw_params_set_rate_resample(snd_handle, hw_params, 0);
	if (!success()) {
		errmsg = "TAlsaPlayer::setHardwareParams() Hardware resampling setup failed for playback.";
		return false;
	}

	/* set the interleaved read/write format */
	errval = snd_pcm_hw_params_set_access(snd_handle, hw_params, access);
	if (!success()) {
		errmsg = "TAlsaPlayer::setHardwareParams() Access type not available for playback.";
		return false;
	}

	/* set the count of channels */
	errval = snd_pcm_hw_params_set_channels(snd_handle, hw_params, snd_channels);
	if (!success()) {
		errmsg = util::csnprintf("TAlsaPlayer::setHardwareParams() Channels count (%) not available for playback.", snd_channels);
		return false;
	}

	/* set the stream rate */
	int dir = 0;
	unsigned int r = snd_samplerate;
	errval = snd_pcm_hw_params_set_rate_near(snd_handle, hw_params, &r, &dir);
	if (!success()) {
		errmsg = util::csnprintf("TAlsaPlayer::setHardwareParams() Sample rate % Hz not available for playback.", snd_samplerate);
		return false;
	}
	if (r != snd_samplerate) {
		errmsg = util::csnprintf("TAlsaPlayer::setHardwareParams() Sample rate doesn't match requested rate % Hz, returned rate is % Hz", snd_samplerate, r);
		errval = -EINVAL;
		return false;
	}

	// Check format from hardware against settings
	// --> Prefer hardware parameter instead of forcing other format value (which will usually fail!)
	snd_pcm_format_t format = getHardwareFormat();
	if (format != SND_PCM_FORMAT_UNKNOWN) {
		if (format != snd_format) {
			logger("[Params] Requested hardware format is " + formatToStr(snd_format));
			logger("[Params] Verified hardware format is " + formatToStr(format));
			if (!setHardwareFormat(format)) {
				errmsg = util::csnprintf("TAlsaPlayer::setHardwareParams() Unsupported hardware format ", formatToStr(format));
				errval = -EINVAL;
				return false;
			}
		} else {
			logger("[Params] Hardware validated format is " + formatToStr(snd_format));
		}
	} else {
		logger("[Params] Hardware validation failed, using format " + formatToStr(snd_format));
	}

	/* set the sample format */
	errval = snd_pcm_hw_params_set_format(snd_handle, hw_params, snd_format);
	if (!success()) {
		bool retry = false;

		// Try 32 Bit on 24 Bit data stream on unknown hardware format
		if (!retry) {
			switch (snd_format) {

				// Try upgrade from 16/24 bit to 32 bit
				case SND_PCM_FORMAT_S16_LE:
				case SND_PCM_FORMAT_S24_LE:
				case SND_PCM_FORMAT_S24_3LE:
					format = SND_PCM_FORMAT_S32_LE;
					retry = true;
					break;

				case SND_PCM_FORMAT_S16_BE:
				case SND_PCM_FORMAT_S24_BE:
				case SND_PCM_FORMAT_S24_3BE:
					format = SND_PCM_FORMAT_S32_BE;
					retry = true;
					break;

				case SND_PCM_FORMAT_U16_LE:
				case SND_PCM_FORMAT_U24_LE:
				case SND_PCM_FORMAT_U24_3LE:
					format = SND_PCM_FORMAT_U32_LE;
					retry = true;
					break;

				case SND_PCM_FORMAT_U16_BE:
				case SND_PCM_FORMAT_U24_BE:
				case SND_PCM_FORMAT_U24_3BE:
					format = SND_PCM_FORMAT_U32_BE;
					retry = true;
					break;

				default:
					retry = false;
					break;
			}
			if (retry) {
				retry = false;
				logger("[Params] Retry with altered format from <" + formatToStr(snd_format) + "> to <" + formatToStr(format) + ">");
				if (setHardwareFormat(format)) {
					retry = true;
				}
			}
		}

		// 2nd try with hardware parameter value
		if (retry) {
			// Check format from hardware against retry value
			snd_pcm_format_t fmt = getHardwareFormat();
			if (fmt != SND_PCM_FORMAT_UNKNOWN) {
				if (fmt != format) {
					logger("[Params] Requested format for retry is " + formatToStr(format));
					logger("[Params] Verified format for retry is " + formatToStr(fmt));
					format = fmt;
				} else {
					logger("[Params] Hardware validated format on retry is " + formatToStr(format));
				}
			} else {
				logger("[Params] Using unverified format " + formatToStr(format));
			}
			if (!setHardwareFormat(format)) {
				errmsg = util::csnprintf("TAlsaPlayer::setHardwareParams() Unsupported hardware format on retry ", formatToStr(format));
				errval = -EINVAL;
				return false;
			}
			errval = snd_pcm_hw_params_set_format(snd_handle, hw_params, snd_format);
			if (!success()) {
				errmsg = util::csnprintf("TAlsaPlayer::setHardwareParams() Sample format $ not available for playback on retry.", formatToStr(snd_format));
				setHardwareFormat(SND_PCM_FORMAT_UNKNOWN);
				return false;
			}
		} else {
			errval = -EINVAL;
			errmsg = util::csnprintf("TAlsaPlayer::setHardwareParams() Sample format $ not available for playback.", formatToStr(snd_format));
			setHardwareFormat(SND_PCM_FORMAT_UNKNOWN);
			return false;
		}
	}

	/* set the period time */
	dir = 0;
	snd_pcm_uframes_t snd_size = 0;
	errval = snd_pcm_hw_params_set_period_time_near(snd_handle, hw_params, &snd_periodtime, &dir);
	if (!success()) {
		errmsg = util::csnprintf("TAlsaPlayer::setHardwareParams() Unable to set period time % for playback.", snd_periodtime);
		return false;
	}
	snd_size = 0;
	errval = snd_pcm_hw_params_get_period_size(hw_params, &snd_size, &dir);
	if (!success()) {
		errmsg = "TAlsaPlayer::setHardwareParams() Unable to get period size for playback.";
		return false;
	}
	snd_periodsize = snd_size;

	/* set the buffer time */
	dir = 0;
	snd_size = 0;
	errval = snd_pcm_hw_params_set_buffer_time_near(snd_handle, hw_params, &snd_buffertime, &dir);
	if (!success()) {
		errmsg = util::csnprintf("TAlsaPlayer::setHardwareParams() Unable to set buffer time % for playback.", snd_buffertime);
		return false;
	}
	errval = snd_pcm_hw_params_get_buffer_size(hw_params, &snd_size);
	if (!success()) {
		errmsg = "TAlsaPlayer::setHardwareParams() Unable to get buffer size for playback.";
		return false;
	}
	snd_buffersize = snd_size;

	/* Get period time */
	dir = 0;
	snd_pcm_uint_t snd_time = 0;
	errval = snd_pcm_hw_params_get_period_time(hw_params, &snd_time, &dir);
	if (!success()) {
		errmsg = "TAlsaPlayer::setHardwareParams() Unable to get period size for playback.";
		return false;
	}
	snd_activeperiodtime = snd_time;

	/* write the parameters to device */
	errval = snd_pcm_hw_params(snd_handle, hw_params);
	if (!success()) {
		errmsg = "TAlsaPlayer::setHardwareParams() Unable to set hardware parameter for playback.";
		return false;
	}

	// Hardware parameters were successfully set!
	errval = EXIT_SUCCESS;
	return true;
}


bool TAlsaPlayer::setSoftwareParams() {
	errval = -EINVAL;

	// Allocate memory for software parameter
    snd_pcm_sw_params_alloca(&sw_params);
    if (!util::assigned(sw_params)) {
		errmsg = "Software parameter initialization failed.";
		errval = -EFAULT;
		return false;
    }

	/* get the current software parameters */
	errval = snd_pcm_sw_params_current(snd_handle, sw_params);
	if (!success()) {
		errmsg = "TAlsaPlayer::setSoftwareParams() Unable to determine current software parameter for playback.";
		return false;
	}

	/* start the transfer when the buffer is almost full: */
	/* (buffer_size / avail_min) * avail_min */
	errval = snd_pcm_sw_params_set_start_threshold(snd_handle, sw_params, (snd_buffersize / snd_periodsize) * snd_periodsize);
	if (!success()) {
		errmsg = "TAlsaPlayer::setSoftwareParams() Unable to set start threshold mode for playback.";
		return false;
	}

	/* allow the transfer when at least period_size samples can be processed */
	/* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
	errval = snd_pcm_sw_params_set_avail_min(snd_handle, sw_params, periodEvent ? snd_buffersize : snd_periodsize);
	if (!success()) {
		errmsg = "TAlsaPlayer::setSoftwareParams() Unable to set avail minimum buffer size for playback.";
		return false;
	}

	/* enable period events when requested */
	if (periodEvent) {
		errval = snd_pcm_sw_params_set_period_event(snd_handle, sw_params, 1);
		if (!success()) {
			errmsg = "TAlsaPlayer::setSoftwareParams() Unable to set period event.";
			return false;
		}
	}

	/* write the parameters to the playback device */
	errval = snd_pcm_sw_params(snd_handle, sw_params);
	if (!success()) {
		errmsg = "TAlsaPlayer::setSoftwareParams() Unable to set software parameter for playback.";
		return false;
	}

	// Asynchronous PCM callback handler
    errval = snd_async_add_pcm_handler(&handler, snd_handle, alsaAsyncCallback, (void*)this);
	if (!success()) {
		errmsg = "TAlsaPlayer::setSoftwareParams() Unable to register asynchronous callback handler.";
		return false;
    }

    // Software parameters were successfully set!
	errval = EXIT_SUCCESS;
	return true;
}

bool TAlsaPlayer::setMasterVolume(long volume) {
	// Check prerequisites
	if (card.empty()) {
		errmsg = "Unknown card.";
		errval = -EINVAL;
		return false;
	}

	// Adjust volume range
	if (volume > 100)
		volume = 100;
	if (volume < 0)
		volume = 0;

	long min, max;
	TAlsaMixer mixer;
	snd_mixer_elem_t* item;
	snd_mixer_selem_id_t * sid;
	const char *shw = card.c_str();

	mixer.open();
	if (!mixer.isValid()) {
		errmsg = util::csnprintf("TAlsaPlayer::setMasterVolume() Unable to open mixer for device $.", card);
		errval = -EFAULT;
		return false;
	}

	errval = snd_mixer_attach(mixer(), shw);
	if (!success()) {
		errmsg = util::csnprintf("TAlsaPlayer::setMasterVolume() Unable to attach card $ to mixer.", card);
		return false;
    }

	errval = snd_mixer_selem_register(mixer(), NULL, NULL);
	if (!success()) {
		errmsg = "TAlsaPlayer::setMasterVolume() Unable to register mixer.";
		return false;
    }

	errval = snd_mixer_load(mixer());
	if (!success()) {
		errmsg = "TAlsaPlayer::setMasterVolume() Unable to load mixer.";
		return false;
    }

	snd_mixer_selem_id_alloca(&sid);
	if (!util::assigned(sid)) {
		errmsg = "TAlsaPlayer::setMasterVolume() Unable allocate memory for mixer element.";
		errval = -EFAULT;
		return false;
    }

	// Iterate through mixer elements
	item = snd_mixer_first_elem(mixer());
	bool vctl;
	while (util::assigned(item)) {
		const char* name;

		// Get mixer properties
		snd_mixer_selem_get_id(item, sid);
		name = snd_mixer_selem_id_get_name(sid);
		vctl = snd_mixer_selem_has_playback_volume(item) > 0;

		// Set volume control to given value
		if (vctl) {

			// Unmute mixer channel
			int value = (volume > 0) ? 1 : 0;
			errval = snd_mixer_selem_set_playback_switch_all(item, value);
			if (value > 0) {
				if (!success()) {
					logger(util::csnprintf("[Volume] Unable to unmute device $ and mixer $", card, name));
				} else {
					logger(util::csnprintf("[Volume] Unmuted device $ and mixer $", card, name));
				}
			} else {
				if (!success()) {
					logger(util::csnprintf("[Volume] Unable to mute device $ and mixer $", card, name));
				} else {
					logger(util::csnprintf("[Volume] Muted device $ and mixer $", card, name));
				}
			}

			errval = snd_mixer_selem_get_playback_volume_range(item, &min, &max);
			if (!success()) {
				errmsg = util::csnprintf("TAlsaPlayer::setMasterVolume() Unable to get playback volume range for device $ and mixer $", card, name);
				return false;
		    }

			errval = snd_mixer_selem_set_playback_volume_all(item, min + volume * max / 100);
			if (!success()) {
				if (!ignoreMixer) {
					errmsg = util::csnprintf("TAlsaPlayer::setMasterVolume() Unable to set volume % %% for device $ and mixer $", volume, card, name);
					return false;
				} else {
					logger(util::csnprintf("[Volume] Unable to set volume % %% for device $ and mixer $", volume, card, name));
				}
		    } else {
		    	logger(util::csnprintf("[Volume] Set volume % %% for device $ and mixer $", volume, card, name));
		    }
		}

		item = snd_mixer_elem_next(item);
	}

	muted = volume <= 0;
	this->volume = volume;
	errval = EXIT_SUCCESS;
	return true;
}

bool TAlsaPlayer::mute() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);

	// Mute/unmute disabled for now!
	muted = true;
	return muted;

	if (!muted) {
		long v = volume;
		muted = setMasterVolume(0); // Muting DoP/DSD stream means interrupting DoP processing!
		volume = v;
	}
	return muted;
}

bool TAlsaPlayer::unmute() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);

	// Mute/unmute disabled for now!
	muted = false;
	return !muted;

	if (muted) {
		muted = !setMasterVolume(volume);
	}
	return !muted;
}

int TAlsaPlayer::recoverUnderrun(int error, int location) {
	logger(util::csnprintf("[Underrun] Underrun at % reason (%)", location, error));
	errval = error;

	// Underrun occurred
	if (errval == -EPIPE) {
		logger("[Underrun] Recovering from EPIPE");
		errval = snd_pcm_prepare(snd_handle);
		if (!success()) {
			errmsg = "TAlsaPlayer::recoverUnderrun() Can't recovery from underrun: prepare failed (EPIPE)";
			logger(util::csnprintf("[Underrun] [EPIPE] Prepare failed: $ [%]", strerr(), syserr()));
		}
		errval = EXIT_SUCCESS;
	} else {
		// Try to resume...
		if (errval == -ESTRPIPE) {
			logger("[Underrun] Recovering from ESTRPIPE");
			errval = snd_pcm_resume(snd_handle);
			if (!success()) {
				errmsg = "TAlsaPlayer::recoverUnderrun() Can't recovery from underrun: resume failed (ESTRIPE)";
				logger(util::csnprintf("TAlsaPlayer::recoverUnderrun(ESTRIPE) Resume failed: $ [%]", strerr(), syserr()));

				// Resume failed, try to prepare...
				errval = snd_pcm_prepare(snd_handle);
				if (!success()) {
					errmsg = "TAlsaPlayer::recoverUnderrun() an't recovery from underrun: prepare failed (ESTRIPE)";
					logger(util::csnprintf("TAlsaPlayer::recoverUnderrun(ESTRIPE) Prepare failed: $ [%]", strerr(), syserr()));
				}
			}
			errval = EXIT_SUCCESS;
		}
	}
	return errval;
}


int TAlsaPlayer::writeFrameData(const TSample * src, TSample *dst[], size_t& read,
		const snd_pcm_channel_area_t *areas, const snd_pcm_int_t steps[], snd_pcm_uframes_t& frames) {
	errmsg.clear();
	errval = EXIT_SUCCESS;
	snd_pcm_uframes_t frame;
	snd_pcm_uint_t channel;
	TSample dither = 0;
	bool isLittleEndian = isLE();

	if (verbosity >= 3)
		logger(util::csnprintf("[Frame]  Frames to read         : % Frames", frames));

	// Copy frames from sample buffer to memory mapped buffer
	for (frame=0; frame<frames; ++frame) {

		// Copy one frame for every channel
		for (channel=0; channel<snd_channels; ++channel) {

			// Detect source data width
			switch (snd_datawidth) {

				// CD source quality
				case 16:
					// Switch destination data width
					switch (snd_physicalwidth) {
						case 16:
							write16to16Bit(src, dst[channel], read, steps[channel], isLittleEndian);
							break;
						case 24:
							if (dithered) dither = getRandomDither();
							write16to24Bit(src, dst[channel], read, steps[channel], isLittleEndian, dither);
							break;
						case 32:
							if (dithered) dither = getRandomDither();
							write16to32Bit(src, dst[channel], read, steps[channel], isLittleEndian, dither);
							break;
						default:
							// Set frames to current value
							// and generate error!
							errval = -EINVAL;
							errmsg = util::csnprintf("TAlsaPlayer::writePeriodData() Invalid physical data width (%/%)", snd_physicalwidth, snd_datawidth);
							break;
					}
					break;

				// High resolution source like DVD, BluRay, etc.
				case 24:
					// Switch destination data width
					switch (snd_physicalwidth) {
						case 24:
							write24to24Bit(src, dst[channel], read, steps[channel], isLittleEndian);
							break;
						case 32:
							if (dithered) dither = getRandomDither();
							write24to32Bit(src, dst[channel], read, steps[channel], isLittleEndian, dither);
							break;
						default:
							// Set frames to current value
							// and generate error!
							errval = -EINVAL;
							errmsg = util::csnprintf("TAlsaPlayer::writePeriodData() Invalid physical data width (%/%)", snd_physicalwidth, snd_datawidth);
							break;
					}
					break;

				default:
					// Set frames to current value
					// and generate error!
					errval = -EINVAL;
					errmsg = util::csnprintf("TAlsaPlayer::writePeriodData() Invalid sample data width (%/%)", snd_physicalwidth, snd_datawidth);
					break;
			}

			// Escape on error
			if (errval != EXIT_SUCCESS) {
				break;

			}

		}

		// Escape on error
		if (errval != EXIT_SUCCESS) {
			frames = frame;
			break;

		}

	}

	if (verbosity >= 3)
		logger(util::csnprintf("[Frame]  Bytes read from buffer : % Byte [%]", read, util::sizeToStr(read, 1, util::VD_BINARY)));
	return errval;
}



int TAlsaPlayer::writeStereoData(const TSample * src, TSample *dst[], size_t& read, snd_pcm_uframes_t& frames) {
	errmsg.clear();
	errval = EXIT_SUCCESS;

	if (verbosity >= 3)
		logger(util::csnprintf("[Stereo] Frames to read         : % Frames", frames));

	// Copy interleaved PCM data like LLRRLLRRLLRR.... from source buffer to memory mapped region
	switch (snd_datawidth) {

		// DSD 1 Bit stream encapsulated as 24 Bit DoP data
		case 1: // Newest endian
		case 2: // Oldest endian
			// Switch destination data width
			switch (snd_physicalwidth) {
				case 24:
					write24BitDSDData(src, dst[0], read, frames);
					break;
				case 32:
					write32BitDSDData(src, dst[0], read, frames);
					break;
				default:
					// Set frames to current value
					// and generate error!
					errval = -EINVAL;
					errmsg = util::csnprintf("TAlsaPlayer::writeStereoData() Invalid physical data width (%/%)", snd_physicalwidth, snd_datawidth);
					break;
			}
			break;

		// CD source quality
		case 16:
			// Switch destination data width
			switch (snd_physicalwidth) {
				case 16:
					write16to16Bit(src, dst[0], read, frames, m_endian);
					break;
				case 24:
					write16to24Bit(src, dst[0], read, frames, m_endian, dithered);
					break;
				case 32:
					write16to32Bit(src, dst[0], read, frames, m_endian, dithered);
					break;
				default:
					// Set frames to current value
					// and generate error!
					errval = -EINVAL;
					errmsg = util::csnprintf("TAlsaPlayer::writeStereoData() Invalid physical data width (%/%)", snd_physicalwidth, snd_datawidth);
					break;
			}
			break;

		// High resolution source like DVD, BluRay, etc.
		case 24:
			// Switch destination data width
			switch (snd_physicalwidth) {
				case 24:
					write24to24Bit(src, dst[0], read, frames, m_endian);
					break;
				case 32:
					write24to32Bit(src, dst[0], read, frames, m_endian, dithered);
					break;
				default:
					// Set frames to current value
					// and generate error!
					errval = -EINVAL;
					errmsg = util::csnprintf("TAlsaPlayer::writeStereoData() Invalid physical data width (%/%)", snd_physicalwidth, snd_datawidth);
					break;
			}
			break;

		default:
			// Set frames to current value
			// and generate error!
			errval = -EINVAL;
			errmsg = util::csnprintf("TAlsaPlayer::writeStereoData() Invalid sample data width (%/%)", snd_physicalwidth, snd_datawidth);
			break;
	}

	if (verbosity >= 3)
		logger(util::csnprintf("[Stereo] Bytes read from buffer : % Byte [%]", read, util::sizeToStr(read, 1, util::VD_BINARY)));
	return errval;
}

void TAlsaPlayer::write24BitDSDData(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames) {
	bool isLittleEndian = isLE();
	TSample marker = getMarker();

	for (snd_pcm_uframes_t i=0; i<frames; ++i) {

		if (isLittleEndian) {

			// Left channel
			*(dst++) = *(src++);
			*(dst++) = *(src++);
			*(dst++) = marker;

			// Right channel
			*(dst++) = *(src++);
			*(dst++) = *(src++);
			*(dst++) = marker;

		} else {

			TSample l1 = *(src++);
			TSample l2 = *(src++);
			TSample r1 = *(src++);
			TSample r2 = *(src++);

			// Left channel
			*(dst++) = marker;
			*(dst++) = l2;
			*(dst++) = l1;

			// Right channel
			*(dst++) = marker;
			*(dst++) = r2;
			*(dst++) = r1;

		}

		// Get next marker
		marker = getNextMarker();

		// Set read sample bytes
		read += 4;
	}
}

void TAlsaPlayer::write32BitDSDData(const TSample *src, TSample *& dst, size_t& read, snd_pcm_uframes_t frames) {
	bool isLittleEndian = isLE();
	TSample marker = getMarker();

	for (snd_pcm_uframes_t i=0; i<frames; ++i) {

		if (isLittleEndian) {

			// Left channel
			*(dst++) = 0;
			*(dst++) = *(src++);
			*(dst++) = *(src++);
			*(dst++) = marker;

			// Right channel
			*(dst++) = 0;
			*(dst++) = *(src++);
			*(dst++) = *(src++);
			*(dst++) = marker;

		} else {

			TSample l1 = *(src++);
			TSample l2 = *(src++);
			TSample r1 = *(src++);
			TSample r2 = *(src++);

			// Left channel
			*(dst++) = marker;
			*(dst++) = l2;
			*(dst++) = l1;
			*(dst++) = 0;

			// Right channel
			*(dst++) = marker;
			*(dst++) = r2;
			*(dst++) = r1;
			*(dst++) = 0;

		}

		// Get next marker
		marker = getNextMarker();

		// Set read sample bytes
		read += 4;
	}
}



int TAlsaPlayer::writeDSDSilence(TSample *dst[], snd_pcm_uframes_t& frames) {
	errmsg.clear();
	errval = EXIT_SUCCESS;

	if (verbosity >= 3)
		logger(util::csnprintf("[Silence] [DSD] Frames to silence      : % Frames", frames));

	// Copy interleaved PCM data like LLRRLLRRLLRR.... from source buffer to memory mapped region
	switch (snd_datawidth) {

		// DSD 1 Bit stream encapsulated as 24 Bit DoP data
		case 1: // Newest endian
		case 2: // Oldest endian
			// Switch destination data width
			switch (snd_physicalwidth) {
				case 24:
					write24BitDSDSilence(dst[0], frames);
					break;
				case 32:
					write32BitDSDSilence(dst[0], frames);
					break;
				default:
					// Set frames to current value
					// and generate error!
					errval = -EINVAL;
					errmsg = util::csnprintf("TAlsaPlayer::writeDSDSilence() Invalid physical data width (%/%)", snd_physicalwidth, snd_datawidth);
					break;
			}
			break;

		default:
			// Set frames to current value
			// and generate error!
			errval = -EINVAL;
			errmsg = util::csnprintf("TAlsaPlayer::writeDSDSilence() Invalid sample data width (%/%)", snd_physicalwidth, snd_datawidth);
			break;
	}

	return errval;
}

void TAlsaPlayer::write24BitDSDSilence(TSample *& dst, snd_pcm_uframes_t& frames) {
	bool isLittleEndian = isLE();
	TSample marker = getMarker();

	// One of the possible 16 Bit DSD silence pattern 0x69 = 0110 1001 0110 1001
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {

		if (isLittleEndian) {

			// Left channel
			*(dst++) = 0x69;
			*(dst++) = 0x69;
			*(dst++) = marker;

			// Right channel
			*(dst++) = 0x69;
			*(dst++) = 0x69;
			*(dst++) = marker;

		} else {

			// Left channel
			*(dst++) = marker;
			*(dst++) = 0x69;
			*(dst++) = 0x69;

			// Right channel
			*(dst++) = marker;
			*(dst++) = 0x69;
			*(dst++) = 0x69;

		}

		// Set next marker
		marker = getNextMarker();
	}
}

void TAlsaPlayer::write32BitDSDSilence(TSample *& dst, snd_pcm_uframes_t& frames) {
	bool isLittleEndian = isLE();
	TSample marker = getMarker();

	// One of the possible 16 Bit DSD silence pattern 0x69 = 0110 1001 0110 1001
	for (snd_pcm_uframes_t i=0; i<frames; ++i) {

		if (isLittleEndian) {

			// Left channel
			*(dst++) = 0;
			*(dst++) = 0x69;
			*(dst++) = 0x69;
			*(dst++) = marker;

			// Right channel
			*(dst++) = 0;
			*(dst++) = 0x69;
			*(dst++) = 0x69;
			*(dst++) = marker;

		} else {

			// Left channel
			*(dst++) = marker;
			*(dst++) = 0x69;
			*(dst++) = 0x69;
			*(dst++) = 0;

			// Right channel
			*(dst++) = marker;
			*(dst++) = 0x69;
			*(dst++) = 0x69;
			*(dst++) = 0;

		}

		// Set next marker
		marker = getNextMarker();
	}
}


bool TAlsaPlayer::doSeek(TAudioBuffer*& buffer, PSong song, double position) {
	if (util::assigned(song) && util::assigned(buffer) && position > 0.0) {
		if (song->isBuffered()) {
			size_t size = song->getSampleSize();
			size_t bytes = (size_t)(position * (double)size / 100.0);
			size_t align = song->getWordWidth();
			size_t aligned = util::align(bytes, align);
			logger(util::csnprintf("[Seek] Seek to position [%/% bytes] aligned by % bytes [% of %]", bytes, aligned, align, util::sizeToStr(bytes, 1, util::VD_BINARY), util::sizeToStr(size, 1, util::VD_BINARY)));

			// Find buffer for given song by adding written bytes
			size_t offset;
			PAudioBuffer next = buffers.getSeekBuffer(song, aligned, offset);
			if (util::assigned(next)) {
				TKeyValue key = next->getKey();

				// Mark all buffer before seek buffer as played
				// and mark all buffers after seek buffer as playable
				PSong s;
				PAudioBuffer o;
				for (size_t i=0; i<buffers.count(); ++i) {
					o = buffers[i];
					if (util::assigned(o)) {
						s = o->getSong();
						if (util::assigned(s)) {
							if (*s == *song) {

								// Reset all buffers to be playable again
								// --> Not needed in any case, but clean...
								o->resetReader();

								// Mark previous buffers as played
								if (o->getKey() < key) {
									buffers.operate(o, EBS_PLAYED, true);
								}

								// Mark current buffer as playing
								if (o->getKey() == key) {
									buffers.operate(o, EBS_PLAYING, true);
									o->read(offset);
								}

								// Mark following buffers as playable
								if (o->getKey() > key) {
									buffers.operate(o, EBS_LOADED, true);
								}
							}
						}
					}
				}

				// Set song statistics
				song->setRead(aligned);
				song->updateStatsistics();

				// Return seek buffer
				if (buffer->getKey() != next->getKey()) {
					if (next->getKey() > buffer->getKey())
						logger(util::csnprintf("[Seek] Seek from buffer <%> to next buffer <%>", buffer->getKey(), next->getKey()));
					else
						logger(util::csnprintf("[Seek] Seek from buffer <%> to previous buffer <%>", buffer->getKey(), next->getKey()));
				} else {
					logger(util::csnprintf("[Seek] Seek in same buffer <%>", next->getKey()));
				}
				buffer = next;
				return true;
			}
		}
	}
	return false;
}

bool TAlsaPlayer::doFastForward(TAudioBuffer*& buffer, PSong song) {
	if (util::assigned(song) && util::assigned(buffer)) {
		util::TTimePart frame = getSkipFrame(song);
		size_t bytes = getSkipBytes(song, frame);
		size_t align = song->getWordWidth();
		size_t aligned = util::align(bytes, align);
		logger(util::csnprintf("[Forward] Fast forward % seconds [%/% bytes] aligned by % bytes.", frame, bytes, aligned, align));

		// Get buffer reader state
		size_t read = buffer->getRead();
		size_t written = buffer->getWritten();

		// Do fast forward fit in current buffer?
		// --> Current position + aligned amount of bytes to skip + save rest
		if ((read + aligned) < written) {
			// Do fast forward by adjusting read buffer
			buffer->read(aligned);
			song->addRead(aligned);
			song->updateStatsistics();
			return true;
		}

		// Calculate space needed from next buffer
		// --> Set rest of space skipped in current buffer
		bytes = 0;
		size_t rest = 0;
		if (written > read) {
			rest = written - read;
			if (rest > 0) {
				if (aligned > rest) {
					bytes = aligned - rest;
				}
			}
		}

		// Try to forward into next buffer
		if (bytes > 0) {

			// Skip exceeds upper buffer limit
			// --> look for next possible buffer
			PAudioBuffer next = buffers.getForwardBuffer(buffer, song);
			if (util::assigned(next)) {

				// Check if buffer has more space than needed to forward
				aligned = util::align(bytes, align);
				written = next->getWritten();

				// Size of next buffer has enough space for forwarding
				if (written > aligned) {

					// Mark current buffer as played
					// --> Add skipped content to read data for song
					buffers.operate(buffer, EBS_PLAYED, true);
					if (rest > 0)
						song->addRead(rest);

					// Set next buffer back to loaded defaults
					next->resetReader();
					buffers.operate(next, EBS_PLAYING, true);

					// Set new read offset given bytes to skip from end
					// Do fast forward by adjusting read buffer
					next->read(aligned);
					song->addRead(aligned);
					song->updateStatsistics();

					// Set current buffer to next buffer
					if (buffer->getKey() != next->getKey())
						logger(util::csnprintf("[Forward] Fast forward from buffer <%> to next buffer <%>", buffer->getKey(), next->getKey()));
					else
						logger(util::csnprintf("[Forward] Fast forward in same buffer <%>", next->getKey()));
					buffer = next;
					return true;
				}
			}
		}

		// Seek to end of song
		return doSeek(buffer, song, 99.9);

	}
	return false;
}

bool TAlsaPlayer::doFastRewind(TAudioBuffer*& buffer, PSong song) {
	if (util::assigned(song) && util::assigned(buffer)) {
		util::TTimePart frame = getSkipFrame(song);
		size_t bytes = getSkipBytes(song, frame);
		size_t align = song->getWordWidth();
		size_t aligned = util::align(bytes, align);
		logger(util::csnprintf("[Rewind] Fast rewind % seconds [%/% bytes] aligned by % bytes.", frame, bytes, aligned, align));

		// Get buffer reader state
		size_t read = buffer->getRead();

		// Do fast rewind fit in current buffer?
		if (read > aligned) {
			// Do fast rewind by adjusting read buffer
			buffer->rewind(aligned);
			song->subRead(aligned);
			song->updateStatsistics();
			return true;
		}

		// Calculate space needed from previous buffer
		// --> Set rest of space skipped in current buffer
		bytes = 0;
		size_t rest = 0;
		if (read > 0) {
			rest = read;
			bytes = aligned - rest;
		}

		// Try to forward into next buffer
		if (bytes > 0) {

			// Skip underruns lower buffer limit
			// --> look for possible previous buffer for song
			PAudioBuffer prev = buffers.getRewindBuffer(buffer, song);
			if (util::assigned(prev)) {

				// Check if buffer has more space than needed to forward
				aligned = util::align(bytes, align);
				size_t written = prev->getWritten();

				// Size of next buffer has enough space for forwarding
				if (written > aligned) {

					// Mark current buffer as loaded to be played again
					// --> Remove skipped content to read data for song
					buffers.operate(buffer, EBS_LOADED, true);
					buffer->resetReader();
					if (rest > 0)
						song->subRead(rest);

					// Set previous buffer back to playing state
					prev->resetReader();
					buffers.operate(prev, EBS_PLAYING, true);

					// Calculate now position from end of previous buffer
					bytes = written - aligned;
					aligned = util::align(bytes, align);
					bytes = written - aligned;

					// Set new read offset given bytes to skip from end
					// Do fast forward by adjusting read buffer
					prev->read(aligned);
					song->subRead(bytes);
					song->updateStatsistics();

					// Set current buffer to next buffer
					if (buffer->getKey() != prev->getKey())
						logger(util::csnprintf("[Rewind] Fast rewind from buffer <%> to previous buffer <%>", buffer->getKey(), prev->getKey()));
					else
						logger(util::csnprintf("[Rewind] Fast rewind in same buffer <%>", buffer->getKey()));
					buffer = prev;
					return true;
				}
			}
		}

		// Rewind to beginning of song
		return doSeek(buffer, song, 0.1);

	}
	return false;
}

util::TTimePart TAlsaPlayer::getSkipFrame(const TSong* song) {
	util::TTimePart f = 0;
	if (util::assigned(song)) {
		if (song->isStreamed()) {
			f = m_skipframe; // 2 skipframes for streams
		} else {
			util::TTimePart d = song->getDuration();
			f = (((d * (util::TTimePart)1000) / (util::TTimePart)480) * m_skipframe) / (util::TTimePart)1000;
		}
	}
	return m_skipframe + f;
}

size_t TAlsaPlayer::getSkipBytes(const TSong* song, const util::TTimePart frame) {
	if (song->isStreamed()) {
		size_t bytesPerSecond = currentSong.song->getSampleRate() * currentSong.song->getChannelCount() * currentSong.song->getBytesPerSample();
		return (size_t)frame * bytesPerSecond;
	}
	return (size_t)frame * song->getBytesPerSecond();
}

bool TAlsaPlayer::writePeriodData() {
	logger(2, "[Period] Begin");

	// Check prerequisites
	if (!util::assigned(currentSong.song)) {
		errmsg = "TAlsaPlayer::writePeriodData() No Song to play.";
		errval = -EINVAL;
		return false;
	}
	if (buffers.count() <= 0) {
		errmsg = "TAlsaPlayer::writePeriodData() No list of buffers to play.";
		errval = -EINVAL;
		return false;
	}
	if (snd_channels <= 0) {
		errmsg = util::csnprintf("TAlsaPlayer::writePeriodData() Invalid channel size (%)", snd_channels);
		errval = -EINVAL;
		return false;
	}
	if (snd_periodsize <= 0) {
		errmsg = util::csnprintf("TAlsaPlayer::writePeriodData() Invalid period size (%)", snd_periodsize);
		errval = -EINVAL;
		return false;
	}

	// Initialize locals with hardware parameters
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t offset, frames, commit;
	snd_pcm_sframes_t available, committed;
	snd_pcm_sframes_t size = 2 * snd_periodsize;
	snd_pcm_uint_t channel;
	snd_pcm_int_t steps[snd_channels];
	TSample * samples[snd_channels];

	// Remember if DoP played for playing silence afterwards
	if (!dop) {
		dop = currentSong.song->isDSD();
	}

	// Get available size of memory mapped buffer
	bool retry = false;
	while (true) {
		available = snd_pcm_avail_update(snd_handle);
		if (available < 0) {
			errval = recoverUnderrun(available, 1);
			if (!success()) {
				errmsg = util::csnprintf("TAlsaPlayer::writePeriodData() Getting available memory mapped buffer size failed (%)", available);
				return false;
			}
			retry = true;
			continue;
		}
		if (available < size) {
			if (retry) {
				retry = false;
				errval = snd_pcm_start(snd_handle);
				if (!success()) {
					errmsg = "TAlsaPlayer::writePeriodData() Restart PCM streaming after underrun recovered.";
					return false;
				}
			} else {
				break;
			}
			continue;
		}
		if (available >= size)
			break;
	}

	// Check available space
	if (available < 0) {
		errmsg = util::csnprintf("TAlsaPlayer::writePeriodData() No available space in memory mapped buffer (%)", available);
		errval = -ENOMEM;
		return false;
	}

	// Write at least available frames
	if (available < size) {
		size = available;
	}

	if (verbosity >= 3) {
		logger(util::csnprintf("[Period] ALSA period time       : % (%) milliseconds", getActivePeriodTime(), getPeriodTime()));
		logger(util::csnprintf("[Period] Max. frames to be read : % Frames", size));
	}

	// Write period size of data
	bool exit = false;
	errval = EXIT_SUCCESS;
	while (success() && size > 0 && !exit) {
		frames = size;

		// Check next song from buffer
		if (!util::assigned(currentSong.song)) {
			errmsg = "TAlsaPlayer::writePeriodData() No song to play during memory map loop.";
			setPlayerStateWithNolock(EPS_STOP);
			return true;
		}

		// Get current song from buffer list that is playing or next buffer that has not yet played
		buffer = buffers.getNextSongBuffer(currentSong.song); //, true);
		if (!util::assigned(buffer)) {
			logger(util::csnprintf("[Period] No buffer ready for song $ (1)", currentSong.song->getTitle()));
			setPlayerStateWithNolock(EPS_WAIT);
			return true;
		}

		if (verbosity >= 3) {
			logger(util::csnprintf("[Period] Current song playing   : $ (%)", currentSong.song->getTitle(), buffer->getKey()));
			logger(util::csnprintf("[Period] Current codec          : % ", currentSong.song->getCodec()));
			logger(util::csnprintf("[Period] Progress for song      : % %%, % (%)",
					currentSong.song->getPercent(), TSong::timeToStr(currentSong.song->getPlayed()), TSong::timeToStr(currentSong.song->getDuration())));
		}

		// Get stream command
		bool forward = false;
		bool rewind = false;
		bool seek = false;
		double position = 0.0;
		TPlayerCommand command;
		if (queue.next(command)) {
			switch (command.command) {
				case EPP_FORWARD:
					forward = true;
					logger("[Period] \"FAST FORWARD\" requested.");
					break;
				case EPP_REWIND:
					rewind = true;
					logger("[Period] \"FAST REWIND\" requested.");
					break;
				case EPP_POSITION:
					seek = true;
					position = command.value;
					logger(util::csnprintf("[Period] \"POSITION\" requested (% %%)", position));
					break;
				default:
					break;
			}
		}

		// Signal progress to main process by calling progress handler
		// 2020/06/26 This is done by checking the played seconds below!!!
//		if (!progressed && currentSong.song->getPercent() != lastSongProgress) {
//			lastSongProgress = currentSong.song->getPercent();
//			progressed = true;
//		}

		// Raise progress update when played seconds had changed
		if (!progressed && !currentSong.song->isStreamed() && currentSong.song->getPlayed() != lastSongPlayed) {
			lastSongPlayed = currentSong.song->getPlayed();
			progressed = true;
		}

		// Raise progress update every 22 seconds second for stream
		if (!progressed && currentSong.song->isStreamed()) {
			int64_t bytesPerSecond = currentSong.song->getSampleRate() * currentSong.song->getChannelCount() * currentSong.song->getBytesPerSample();
			if (streamed > (lastSongStreamed + (bytesPerSecond * 22))) {
				lastSongStreamed = streamed;
				progressed = true;
			}
		}

		// Set current buffer to playing state when buffered to avaid state collisons with buffering task
		buffers.operate(buffer, EBS_PLAYING);

		// Get memory map area
		errval = snd_pcm_mmap_begin(snd_handle, &areas, &offset, &frames);
		if (!success()) {
			errval = recoverUnderrun(errval, 2);
			if (!success()) {
				errmsg = "TAlsaPlayer::writePeriodData() Recover from memory map begin failed";
				return false;
			}
		}

		// Available frame size reduced in snd_pcm_mmap_begin()
		if ((snd_pcm_sframes_t)frames < size) {
			size -= size - frames;
		}

		// Verify areas and prepare memory mapped buffer as copy destination.
		// Store values in array, e.g. for stereo it looks like:
		//
		//   samples[0] = Address of left channel
		//   samples[1] = Address of right channel
		//
		for (channel=0; channel<snd_channels; ++channel) {

			// Is channel buffer byte aligned?
			if ((areas[channel].first % 8) != 0) {
				errmsg = util::csnprintf("TAlsaPlayer::writePeriodData() Area is not byte aligned, first = % on channel = %", areas[channel].first, channel);
				errval = -EINVAL;
				return false;
			}

			// Calculate first address for given channel (areas.first is given in bits)
			samples[channel] = (((TSample *)areas[channel].addr) + (areas[channel].first / 8));

			// Is step for frame size aligned to 16 bits?
			// --> At least 2 bytes for one frame!
			if ((areas[channel].step % 16) != 0) {
				errmsg = util::csnprintf("TAlsaPlayer::writePeriodData() Area frame is not word aligned, step = % on channel = %", areas[channel].step, channel);
				errval = -EINVAL;
				return false;
			}

			// Calculate step size in "frames" for given channel
			// --> Step is framesize in bytes for given single channel object!
			steps[channel] = areas[channel].step / 8;

			// Take given frame offset for memory map area in account
			// --> Add offset in frame size for one single channel to channel buffer address
			samples[channel] += offset * steps[channel];
		}

		// Check for fast forward/reverse in current buffer
		if (forward) {
			if (!doFastForward(buffer, currentSong.song)) {
				queue.clear();
			}
		}
		if (rewind) {
			if (!doFastRewind(buffer, currentSong.song)) {
				queue.clear();
			}
		}
		if (!currentSong.song->isStreamed()) {
			if (seek) {
				if (!doSeek(buffer, currentSong.song, position)) {
					queue.clear();
				}
			}
		}

		// Buffer size in bytes to be copied from sample buffer
		// --> Variable framesize is size for one frame for all channels, derived from sum of channel step size!
		size_t bytes = frames * snd_dataframesize;
		size_t rest = buffer->getWritten() - buffer->getRead();

		// Debug output
		if (verbosity >= 3) {
			logger(util::csnprintf("[Period] Physical frame size    : % Byte [% Bit, % Bit per channel]", snd_framesize, snd_framesize * 8, snd_framesize * 8 / snd_channels));
			logger(util::csnprintf("[Period] Data frame size        : % Byte [% Bit, % Bit per channel]", snd_dataframesize, snd_dataframesize * 8, snd_dataframesize * 8  / snd_channels));
			logger(util::csnprintf("[Period] Frames to write        : % Frames", size));
			logger(util::csnprintf("[Period] Updated avail. frames  : % Frames", available));
			logger(util::csnprintf("[Period] Frames in memory map   : % Frames", frames));
			logger(util::csnprintf("[Period] Buffer size            : % Byte [%]", bytes, util::sizeToStr(bytes, 1, util::VD_BINARY)));
			logger(util::csnprintf("[Period] Data to write          : % Byte [%] % Frames", rest, util::sizeToStr(rest, 1, util::VD_BINARY), rest / snd_dataframesize));
			if (debug) std::cout << std::endl;
			if (verbosity >= 4) {
				for (channel=0; channel<snd_channels; ++channel) {
					logger(util::csnprintf("[Period] Step size  channel[%]  : % Byte", channel, steps[channel]));
					logger(util::csnprintf("[Period] First pos. channel[%]  : % Byte", channel, areas[channel].first / 8));
					logger(util::csnprintf("[Period] Offset for channel[%]  : % Byte", channel, offset * steps[channel]));
				}
				if (debug) std::cout << std::endl;
			}
		}

		// Something to read in current buffer?
		commit = frames;
		if (rest > 0) {

			// Check frame size in bytes against available space in bytes in read buffer
			if (rest <= bytes) {
				// Last frames for current buffer are streamed
				// --> set status to draining
				buffers.operate(buffer, EBS_DRAINING);

				// Commit all frames until end of current read buffer
				commit = rest / snd_dataframesize;

				if (verbosity >= 3) {
					logger(util::csnprintf("[Period] Planned frames         : % Frames", frames));
					logger(util::csnprintf("[Period] Frames to write        : % Frames", size));
				}
			}
			if (verbosity >= 3) {
				logger(util::csnprintf("[Period] Frames to commit       : % Frames", commit));
				logger(util::csnprintf("[Period] Available buffer size  : % Byte [%]", rest, util::sizeToStr(rest, 1, util::VD_BINARY)));
			}

			// Copy frames from sample buffer to memory mapped buffer
			size_t read = 0;
			switch (snd_channels) {
				case 2:
					errval = writeStereoData(buffer->reader(), samples, read, commit);
					break;
				default:
					errval = writeFrameData(buffer->reader(), samples, read, areas, steps, commit);
					break;
			}

			if (success()) {
				// Set read bytes from buffer
				buffer->read(read);
				currentSong.song->addRead(read);
				currentSong.song->updateStatsistics();
				if (verbosity >= 3)
					logger(util::csnprintf("[Period] Read buffer size       : % Byte [%]", read, util::sizeToStr(read, 1, util::VD_BINARY)));
			} else {
				// Writing bytes failed
				commit = 0;
				logger("[Period] Writing frame data failed:");
				logger(util::csnprintf("[Period] Message : $", strerr()));
				logger(util::csnprintf("[Period] Error   : $", syserr()));
			}

		} else {
			// Nothing can be written!
			// --> Should never happen for songs, can happen after buffer underrung for streams
			commit = 0;
			logger(util::csnprintf("[Period] No data to be written for buffer <%>", buffer->getKey()));
			logger(util::csnprintf("[Period] % Bytes read [%]", buffer->getRead(), util::sizeToStr(buffer->getRead(), 1, util::VD_BINARY)));
			logger(util::csnprintf("[Period] % Bytes written [%]", buffer->getWritten(), util::sizeToStr(buffer->getWritten(), 1, util::VD_BINARY)));

		}

		if (verbosity >= 3)
			logger(util::csnprintf("[Period] Committing frames      : % Frames", commit));

		// Commit written frames
		committed = snd_pcm_mmap_commit(snd_handle, offset, commit);
		if (committed < 0 || (snd_pcm_uframes_t)committed != commit) {
			errval = recoverUnderrun(-EPIPE, 3);
			if (!success()) {
				errmsg = "TAlsaPlayer::writePeriodData() Memory map commit failed on underrun.";
				return false;
			}
		}

		if (verbosity >= 3) {
			logger(util::csnprintf("[Period] Committed frames       : % Frames", commit));
			logger(util::csnprintf("[Period] Bytes written          : % Byte", buffer->getWritten()));
			logger(util::csnprintf("[Period] Bytes read             : % Byte", buffer->getRead()));
			if (debug) std::cout << std::endl;
		}

		// Is stream data playable?
		bool streamable = currentSong.song->isStreamed();
		bool exhausted = streamable ? buffer->isUnderrun() : buffer->isExhausted();

		// Mark buffer as played if all written bytes were read!
		if (exhausted) {
			bool goon = true;

			// Check if stream is playing
			if (streamable) {
				EBufferLevel level = buffer->getLevel();
				if (EBL_FULL != level) {
				//if (buffer->isUnderrun()) {
					// Buffer still not full
					// --> Stream buffer underrun happens
					// --> play some silence!
					if (haltDevice()) {
						logger("[Period] Stream halted on buffer underrun.");
					} else {
						logger("[Period] Halt stream on buffer underrun failed <" + errmsg + "> (" + std::to_string((size_s)errval) + ")");
					}
					goon = false;
					exit = true;
				}
			}

			if (goon) {
				// Mark buffer as played!
				buffers.operate(buffer, EBS_PLAYED);

				// Check if no further buffer for current song to play
				PAudioBuffer nextBuffer = buffers.getNextSongBuffer(currentSong.song);
				if (!util::assigned(nextBuffer)) {

					logger("[Period] All bytes written, get next song from playlist.");

					// Get next song from playlist queue management
					std::string playlist;
					bool reopen = false;
					PSong nextSong = nil;
					PSong prevSong = currentSong.song;
					onPlaylistRequest(currentSong, nextSong, playlist, reopen);
					if (util::assigned(nextSong)) {

						// Switch gapless to next song
						forceReopen = reopen;
						lastSong.song = currentSong.song;
						lastSong.playlist = currentSong.playlist;
						currentSong.song = nextSong;
						currentSong.playlist = playlist;

						// Check if song changed
						if (*prevSong != *nextSong || forceReopen) {

							// Test for different stream parameters
							if (prevSong->getStreamData() != nextSong->getStreamData() || forceReopen) {
								// New stream parameters detected
								// --> Device must be reopened for new song after draining buffer
								logger("[Period] Wait to reopen device with different stream properties.");
								setPlayerStateWithNolock(EPS_REOPEN);
								reopen = true;
								exit = true;
							}

						}

						// A) Reset buffers and song properties for current song to played (again?)
						// B) Release last buffer for streamed playback
						if (streamable) {
							logger(util::csnprintf("[Period] Reset stream buffer <%> for song $", buffer->getKey(), currentSong.song->getTitle()));
							buffers.reset(buffer);
						} else {
							buffers.cleanup(currentSong.song);
						}

						// Check if buffer for next song is availiable
						if (!reopen) {
							nextBuffer = buffers.getNextSongBuffer(currentSong.song);
							if (!util::assigned(nextBuffer)) {
								logger(util::csnprintf("[Period] No buffer ready for song $ (2)", currentSong.song->getTitle()));
								setPlayerStateWithNolock(EPS_WAIT);
								exit = true;
							}
						}

					} else {
						logger("[Period] Playlist processed, no more song to play.");
						currentSong.clear();
						setPlayerStateWithNolock(EPS_STOP);
						forceReopen = false;
						exit = true;
					}

					// Signal song change to calling process
					if (!exit) {
						setPlayerStateWithNolock(m_state);
					}

					// Clear command queue for next song
					queue.clear();

				} else {
					if (buffer->getKey() != nextBuffer->getKey()) {
						logger(util::csnprintf("[Period] Look ahead buffer <%> after current buffer <%> for song $", nextBuffer->getKey(), buffer->getKey(), currentSong.song->getTitle()));
						if (streamable) {
							logger(util::csnprintf("[Period] Reset stream buffer <%> for song $", buffer->getKey(), currentSong.song->getTitle()));
							buffers.reset(buffer);
						}
					}
				}
			}
		}

		// Accumulate committed frames
		size -= commit;
		streamed += commit * snd_framesize;
		if (verbosity >= 2) {
			if (verbosity == 2) {
				if (size > 0)
					logger(util::csnprintf("[Period] Remaining % Frames", size));
				logger(util::csnprintf("[Period] Streamed = %, Avail = %, Committed = %, Framesize = % Bytes", streamed, available, commit, snd_framesize));
			} else {
				if (size > 0)
					logger(util::csnprintf("[Period] Remaining frames       : % Frames", size));
				logger(util::csnprintf("[Period] Stream progress        : %/%/%/% Bytes", streamed, available, commit, snd_framesize));
			}
		}

	} // while (success() && size > 0 && OK)

	logger(2, "TAlsaPlayer::writePeriodData(end)");
	return true;
}


bool TAlsaPlayer::writeSilence() {
	logger(2, "[Silence] [PCM] Begin");

	// Check prerequisites
	if (snd_channels <= 0) {
		errmsg = util::csnprintf("TAlsaPlayer::writeSilence() Invalid channel size (%)", snd_channels);
		errval = -EINVAL;
		return false;
	}
	if (snd_periodsize <= 0) {
		errmsg = util::csnprintf("TAlsaPlayer::writeSilence() Invalid period size (%)", snd_periodsize);
		errval = -EINVAL;
		return false;
	}

	// Initialize locals with hardware parameters
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t offset, frames, commit;
	snd_pcm_sframes_t available, committed;
	snd_pcm_sframes_t size = 2 * snd_periodsize;
	snd_pcm_uint_t channel;
	snd_pcm_int_t steps[snd_channels];
	TSample * samples[snd_channels];

	// Get available size of memory mapped buffer
	bool retry = false;
	while (true) {
		available = snd_pcm_avail_update(snd_handle);
		if (available < 0) {
			errval = recoverUnderrun(available, 4);
			if (!success()) {
				errmsg = util::csnprintf("TAlsaPlayer::writeSilence() Getting available memory mapped buffer size failed (%)", available);
				return false;
			}
			retry = true;
			continue;
		}
		if (available < size) {
			if (retry) {
				retry = false;
				errval = snd_pcm_start(snd_handle);
				if (!success()) {
					errmsg = "TAlsaPlayer::writeSilence() Restart PCM streaming after underrun recovered.";
					return false;
				}
			} else {
				break;
			}
			continue;
		}
		if (available >= size)
			break;
	}

	// Check available space
	if (available < 0) {
		errmsg = util::csnprintf("TAlsaPlayer::writeSilence() No available space in memory mapped buffer (%)", available);
		errval = -ENOMEM;
		return false;
	}

	// Write at least available frames
	if (available < size) {
		size = available;
	}

	if (verbosity >= 3) {
		logger(util::csnprintf("[Silence] ALSA period time          : % (%) milliseconds", getActivePeriodTime(), getPeriodTime()));
		logger(util::csnprintf("[Silence] Max. frames to be read    : % Frames", size));
	}

	// Write period size of data
	errval = EXIT_SUCCESS;
	while (success() && size > 0) {
		frames = size;

		// Get memory map area
		errval = snd_pcm_mmap_begin(snd_handle, &areas, &offset, &frames);
		if (!success()) {
			errval = recoverUnderrun(errval, 5);
			if (!success()) {
				errmsg = "TAlsaPlayer::writeSilence() Recover from memory map begin failed";
				return false;
			}
		}

		// Available frame size reduced in snd_pcm_mmap_begin()
		if ((snd_pcm_sframes_t)frames < size) {
			size -= size - frames;
		}

		// Verify areas and prepare memory mapped buffer as copy destination.
		// Store values in array, e.g. for stereo it looks like:
		//
		//   samples[0] = Address of left channel
		//   samples[1] = Address of right channel
		//
		for (channel=0; channel<snd_channels; ++channel) {

			// Is channel buffer byte aligned?
			if ((areas[channel].first % 8) != 0) {
				errmsg = util::csnprintf("TAlsaPlayer::writeSilence() Area is not byte aligned, first = % on channel = %", areas[channel].first, channel);
				errval = -EINVAL;
				return false;
			}

			// Calculate first address for given channel (areas.first is given in bits)
			samples[channel] = (((TSample *)areas[channel].addr) + (areas[channel].first / 8));

			// Is step for frame size aligned to 16 bits?
			// --> At least 2 bytes for one frame!
			if ((areas[channel].step % 16) != 0) {
				errmsg = util::csnprintf("TAlsaPlayer::writeSilence() Area frame is not word aligned, step = % on channel = %", areas[channel].step, channel);
				errval = -EINVAL;
				return false;
			}

			// Calculate step size in "frames" for given channel
			// --> Step is framesize in bytes for given single channel object!
			steps[channel] = areas[channel].step / 8;

			// Take given frame offset for memory map area in account
			// --> Add offset in frame size for one single channel to channel buffer address
			samples[channel] += offset * steps[channel];
		}

		// Buffer size in bytes to be copied from sample buffer
		// --> Variable framesize is size for one frame for all channels, derived from sum of channel step size!
		size_t bytes = frames * snd_dataframesize;

		// Debug output
		if (verbosity >= 3) {
			logger(util::csnprintf("[Silence] Physical frame size       : % Byte [% Bit, % Bit per channel]", snd_framesize, snd_framesize * 8, snd_framesize * 8 / snd_channels));
			logger(util::csnprintf("[Silence] Data frame size           : % Byte [% Bit, % Bit per channel]", snd_dataframesize, snd_dataframesize * 8, snd_dataframesize * 8  / snd_channels));
			logger(util::csnprintf("[Silence] Frames to write           : % Frames", size));
			logger(util::csnprintf("[Silence] Updated avail. frames     : % Frames", available));
			logger(util::csnprintf("[Silence] Frames in memory map      : % Frames", frames));
			logger(util::csnprintf("[Silence] Buffer size               : % Byte [%]", bytes, util::sizeToStr(bytes, 1, util::VD_BINARY)));
			if (debug) std::cout << std::endl;
			if (verbosity >= 4) {
				for (channel=0; channel<snd_channels; ++channel) {
					logger(util::csnprintf("[Silence] Step size  channel[%]     : % Byte", channel, steps[channel]));
					logger(util::csnprintf("[Silence] First pos. channel[%]     : % Byte", channel, areas[channel].first / 8));
					logger(util::csnprintf("[Silence] Offset for channel[%]     : % Byte", channel, offset * steps[channel]));
				}
				if (debug) std::cout << std::endl;
			}
		}

		// Copy silence to memory mapped buffer
		commit = frames;
		switch (snd_datawidth) {
			case 1: // Newest endian
			case 2: // Oldest endian
				errval = EXIT_SUCCESS;
				writeDSDSilence(samples, frames);
				break;
			default:
				errval = snd_pcm_areas_silence(areas, offset, snd_channels, frames, snd_format);
				break;
		}

		if (!success()) {
			// Writing bytes failed
			commit = 0;
			logger("[Silence] Writing silence failed:");
			logger(util::csnprintf("[Silence] Message : $", strerr()));
			logger(util::csnprintf("[Silence] Error   : $", syserr()));
		}

		// Commit written frames
		if (verbosity >= 3)
			logger(util::csnprintf("[Silence] Committing frames         : % Frames", commit));
		committed = snd_pcm_mmap_commit(snd_handle, offset, commit);
		if (committed < 0 || (snd_pcm_uframes_t)committed != commit) {
			errval = recoverUnderrun(-EPIPE, 6);
			if (!success()) {
				errmsg = "TAlsaPlayer::sendData() Memory map commit failed on underrun.";
				return false;
			}
		}

		// Accumulate committed frames
		size -= commit;
		if (verbosity >= 3) {
			logger(util::csnprintf("[Silence] Committed frames          : % Frames", committed));
			logger(util::csnprintf("[Silence] Remaining frames          : % Frames", size));
		}

	}

	logger(2, "[Silence] [PCM] End");
	return true;
}

void TAlsaPlayer::alsaCallbackHandler() {
	try {
		sema.post();
	} catch (...) {}
}


int TAlsaPlayer::alsaThreadHandler() {
	logger("[Tread] Thread started.");
	started = true;
	running = true;

	// Cling thread to CPUx
	pid_t tid = gettid();
	ssize_t cpu = setAffinity(0, tid);
	if (cpu > 0)
		logger(util::csnprintf("[Tread] Set thread affinity to CPU%", cpu));

	// Loop on semaphore...
	while (sema.wait()) {
		if (terminate)
			break;
		alsaThreadMethod();
	}

	running = false;
	logger("[Tread] Thread terminated.");
	return EXIT_SUCCESS;
}


void TAlsaPlayer::alsaThreadMethod() {
	TPlayerState player;
	try {
		app::TLockGuard<app::TMutex> lock(alsaMtx);
		logger(2, "[Thread] Begin");
		bool stop = false;
		util::TDateTime time;
		time.start();

		// Check deviced state
		if (!getOpen()) {
			logger("[Thread] Device is not open.");
			return;
		}
		if (util::isMemberOf(m_state, EPS_ERROR)) {
			logger(util::csnprintf("[Tread] Invalid device state <%>", statusToStr(m_state)));
			return;
		}

		// Check ALSA state
		snd_pcm_state_t state = snd_pcm_state(snd_handle);
		if (state == SND_PCM_STATE_XRUN) {
			logger("[Tread] Invalid ALSA state <SND_PCM_STATE_XRUN>");
			errval = recoverUnderrun(-EPIPE, 7);
			if (!success()) {
				errmsg = "TAlsaPlayer::alsaThreadMethod() Recover from memory underrun failed";
				stop = true;
			}
		} else if (state == SND_PCM_STATE_SUSPENDED) {
			logger("[Tread] Invalid ALSA state <SND_PCM_STATE_SUSPENDED>");
			errval = recoverUnderrun(-ESTRPIPE, 8);
			if (!success()) {
				errmsg = "TAlsaPlayer::alsaCallbackHandler() Recover from memory suspended state failed";
				stop = true;
			}
		}

		// Unrecoverable error happened
		if (stop) {
			logger("[Thread] ALSA callback handler failed:");
			logger(util::csnprintf("[Tread] Message : $", strerr()));
			logger(util::csnprintf("[Tread] Error   : $", syserr()));
		}

		// Detect transition from play to pause/stop mode and vice versa
		detectOutputChangeWithNolock(m_state);

		// Copy data to ALSA buffer when playing and not stopped
		if (!stop && !getPaused() && !getStopped() && !getHalted()) {
			if (!writePeriodData()) {
				logger("[Thread] Writing samples to ALSA buffer failed:");
				logger(util::csnprintf("[Tread] Message : $", strerr()));
				logger(util::csnprintf("[Tread] Error   : $", syserr()));
				stop = true;
			}
		}

		// Write silence when paused, stopped or halted
		if (!stop && (getPaused() || getStopped() || getHalted())) {
			if (!writeSilence()) {
				logger("[Thread] Writing silence to ALSA buffer failed:");
				logger(util::csnprintf("[Tread] Message : $", strerr()));
				logger(util::csnprintf("[Tread] Error   : $", syserr()));
				stop = true;
			}
			bool restart = false;
			if (getHalted()) {
				if (util::assigned(buffer)) {
					restart = buffer->isStreamable();
				}
			}
			if (!stop && restart) {
				restartDevice();
				logger("[Thread] Stream restarted after buffer underrun.");
			}
		}

		if (verbosity >= 2) {
			util::TTimePart t = time.stop(util::ETP_MICRON);
			logger(util::csnprintf("[Tread] Duration = % microseconds", t));
			if (debug) std::cout << std::endl;
		}

		// Unrecoverable write error happened
		// --> Drain output!
		if (stop) {
			logger(util::csnprintf("[Thread] Drop device on erroneous state (%)", state));
			dropDevice();
		}

		// Get current state for further processing of events...
		getAndResetPlayerStateWithNolock(player);

	} catch (const std::exception& e)	{
		std::string sExcept = e.what();
		logger(util::csnprintf("[Tread] Exception $", sExcept));
		updatePlayerState(player, EPS_ERROR);
	} catch (...)	{
		logger("[Thread] Unknown exception.");
	}

	// Trigger asynchronous event callbacks
	processStateChangeEvents(player, "Stream");
}


void TAlsaPlayer::processStateChangeEvents(TPlayerState& player, const std::string& action) {
	if (player.invalidated || player.changed || player.progressed) {
		app::TLockGuard<app::TMutex> lock(eventMtx);
		std::string location = "anonimous";
		if (util::assigned(player.current.song)) {
			location = player.current.song->isStreamed() ? "stream" : "song";
		}
		if (player.invalidated) {
			player.invalidated = false;
			executeStatusChangeWithNolock(player.state, player.current);
			logger(util::csnprintf("[Event] Asynchronous % state change event processed for $", location, action));
		}
		if (player.changed) {
			player.changed = false;
			executeOutputChangeWithNolock(player.state, player.current);
			logger(util::csnprintf("[Event] Asynchronous % mode change event processed for $", location, action));
		}
		if (player.progressed) {
			player.progressed = false;
			executeProgressChangeWithNolock(player.current);
		}
	}
}


void TAlsaPlayer::onStateChangedCallback(const EPlayerState state, const TCurrentSong& current) {
	if (enabled) {
		if (verbosity >= 2) logger("[State] Begin");
		if (nil != onPlaybackStateChanged) {
			try {
				onPlaybackStateChanged(*this, state, current.song, current.playlist);
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				logger(util::csnprintf("[State] Exception $", sExcept));
			} catch (...)	{
				logger("[State] Unknown exception.");
			}
		}
		if (verbosity >= 2) logger("[State] End");
	}
}

void TAlsaPlayer::onOutputChangedCallback(const EPlayerState state, const TCurrentSong& current) {
	if (enabled) {
		if (verbosity >= 2) logger("[Output] Begin");
		if (nil != onOutputStateChanged) {
			try {
				onOutputStateChanged(*this, state, current.song, current.playlist);
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				logger(util::csnprintf("[Output] Exception $", sExcept));
			} catch (...)	{
				logger("[Output] Unknown exception.");
			}
		}
		if (verbosity >= 2) logger("[Output] End");
	}
}

void TAlsaPlayer::onProgressChangedCallback(const TCurrentSong& current) {
	if (enabled) {
		if (verbosity >= 2) logger("[Progress] Begin");
		if (nil != onPlaybackProgressChanged) {
			try {
				onPlaybackProgressChanged(*this, m_state, current.song, streamed, current.playlist);
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				logger(util::csnprintf("[Progress] Exception $", sExcept));
			} catch (...)	{
				logger("[Progress] Unknown exception.");
			}
		}
		if (verbosity >= 2) logger("[Progress] End");
	}
}

void TAlsaPlayer::onPlaylistRequest(const TCurrentSong& current, TSong*& next, std::string& playlist, bool& reopen) {
	next = nil;
	if (enabled) {
		if (verbosity >= 2) logger("[Playlist] Begin");
		if (nil != onPlaybackPlaylistRequest) {
			try {
				playlist = current.playlist;
				onPlaybackPlaylistRequest(*this, current.song, next, playlist, reopen);
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				logger(util::csnprintf("[Playlist] Exception $", sExcept));
			} catch (...)	{
				logger("[Playlist] Unknown exception.");
			}
		}
		if (verbosity >= 2) logger("[Playlist] End");
	}
}


void TAlsaPlayer::createBuffers(const size_t fraction, const size_t maxBufferCount,
		const size_t minBufferSize, const size_t maxBufferSize) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	buffers.setDebug(debug);
	buffers.create(fraction, maxBufferCount, minBufferSize, maxBufferSize);
	logger(util::csnprintf("Prepared % buffers with % each.", buffers.count(), util::sizeToStr(buffers.buffer(), 1, util::VD_BINARY)));
}

size_t TAlsaPlayer::bufferCount() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.count();
};

size_t TAlsaPlayer::freeBufferSize() const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.free();
};

bool TAlsaPlayer::isSongBuffered(const TSong* song) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.hasSong(song);
};

bool TAlsaPlayer::isFileBuffered(const std::string& fileHash) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.hasFile(fileHash);
};

bool TAlsaPlayer::isAlbumBuffered(const std::string& albumHash) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.hasAlbum(albumHash);
};

size_t TAlsaPlayer::resetStreamBuffers() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.resetStreamBuffers();
}

size_t TAlsaPlayer::resetBuffers(const util::hash_type hash, const PSong current, const size_t index, const ECompareType type, size_t range) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	size_t r = buffers.reset(hash, current, index, type, range);
	if (r > 0) {
		buffers.sort();
	}
	return r;
}

void TAlsaPlayer::resetBuffers(PSong song) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	buffers.reset(song);
}

void TAlsaPlayer::resetBuffers(PTrack track) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	if (util::assigned(track))
		buffers.reset(track->getSong());
}

void TAlsaPlayer::resetBuffers() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	buffers.reset();
}

void TAlsaPlayer::resetBuffer(PAudioBuffer buffer) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	buffers.reset(buffer);
}

size_t TAlsaPlayer::bufferGarbageCollector(const music::TCurrentSongs& songs) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.garbageCollector(songs);
}

void TAlsaPlayer::debugOutputBuffers(const std::string& preamble) const {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.debugOutput(preamble);
}


PAudioBuffer TAlsaPlayer::getNextEmptyBuffer() {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.getNextEmptyBuffer();
}

PAudioBuffer TAlsaPlayer::getNextEmptyBuffer(const TSong* song) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.getNextEmptyBuffer(song);
}

PAudioBuffer TAlsaPlayer::getNextEmptyBuffer(const TTrack* track) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	return buffers.getNextEmptyBuffer(track);
}


void TAlsaPlayer::operateBuffers(PAudioBuffer buffer, const PTrack track, const EBufferState state, const EBufferLevel level) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	buffers.operate(buffer, track, state, level);

}

void TAlsaPlayer::operateBuffers(PAudioBuffer buffer, const PTrack track, const EBufferState state) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	buffers.operate(buffer, track, state);

}

void TAlsaPlayer::operateBuffers(PAudioBuffer buffer, const EBufferState state, const bool force) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	buffers.operate(buffer, state, force);
}

void TAlsaPlayer::setBufferLevel(PAudioBuffer buffer, const EBufferLevel level) {
	app::TLockGuard<app::TMutex> lock(alsaMtx);
	buffers.setLevel(buffer, level);
}

} /* namespace music */
