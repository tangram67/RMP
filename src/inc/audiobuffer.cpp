/*
 * audiobuffer.cpp
 *
 *  Created on: 14.08.2016
 *      Author: Dirk Brinkmeier
 */

#include <iostream>
#include "audiobuffer.h"
#include "audiofile.h"
#include "exception.h"
#include "fileutils.h"
#include "sysutils.h"
#include "convert.h"
#include "ASCII.h"
#include "pcm.h"

namespace music {

TAudioBuffer::TAudioBuffer() {
	prime();
}

TAudioBuffer::~TAudioBuffer() {
	clear();
}

void TAudioBuffer::prime() {
	r_ptr = nil;
	w_ptr = nil;
	r_max = nil;
	w_max = nil;
	dummy = "<empty>";
	init();
}

void TAudioBuffer::reset() {
	resetReader();
	resetWriter();
	init();
}

void TAudioBuffer::prepare() {
	resetReader();
	resetWriter();
	m_last = false;
	m_first = false;
}

void TAudioBuffer::resetReader() {
	m_read = 0;
	r_ptr = data();
	r_max = r_ptr + size();
}

void TAudioBuffer::resetWriter() {
	m_written = 0;
	w_ptr = data();
	w_max = w_ptr + size();
}

void TAudioBuffer::init() {
	m_level = EBL_EMPTY;
	m_status = EBS_EMPTY;
	m_track = nil;
	m_key = 0;
	m_last = false;
	m_first = false;
	m_allocated = false;
}

void TAudioBuffer::clear() {
	prime();
	resize(0);
}

void TAudioBuffer::cleanup() {
	if (m_written > 0 && m_status != EBS_EMPTY) {
		// Reset buffer to intial loaded state
		// --> Playback buffers is possible again
		resetReader();
		if (getStatus() >= EBS_BUFFERED)
			forceStatus(EBS_LOADED);
		PSong song = getSong();
		if (util::assigned(song)) {
			song->cleanup();
		}
	} else {
		// No song to play from buffer
		// --> Reset buffer to empty state
		reset();
	}
}

void TAudioBuffer::resize(const size_t size) {
#ifdef USE_MEMORY_MAPPED_SAMPLE_BUFFER
	if (buffer.size() > 0) {
		if (!buffer.unlock())
			throw util::sys_error("TAudioBuffer::resize() Unlocking memory mapped region failed.", buffer.error());
	}
#endif
	if (size > 0) {
		buffer.resize(size, false);
		if (util::assigned(data())) {
#ifdef USE_MEMORY_MAPPED_SAMPLE_BUFFER
			if (!buffer.lock())
				throw util::sys_error("TAudioBuffer::resize() Locking memory mapped region failed.", buffer.error());
#else
			buffer.fillchar(0);
#endif
			reset();
		} else {
			prime();
			throw util::app_error("TAudioBuffer::resize() Insufficient memory.");
		}
	} else {
		buffer.clear();
		prime();
	}
}

PSong TAudioBuffer::getSong() const {
	if (util::assigned(m_track))
		return m_track->getSong();
	return nil;
};

PSample TAudioBuffer::read(const size_t size) {
	PSample p = r_ptr;
	r_ptr += size;
	m_read += size;
	return p;
}

PSample TAudioBuffer::rewind(const size_t size) {
	PSample p = r_ptr;
	r_ptr -= size;
	m_read -= size;
	return p;
}

PSample TAudioBuffer::write(const size_t size) {
	PSample p = w_ptr;
	w_ptr += size;
	m_written += size;
	return p;
}

void TAudioBuffer::setWritten(const size_t size) {
	w_ptr = data() + size;
	m_written = size;
}

PSample TAudioBuffer::write16Bit(uint16_t value, const util::EEndianType endian) {
	if (validBuffer() && validWriter()) {
		switch (endian) {
			case util::EE_LITTLE_ENDIAN:
				*(w_ptr++) = (TSample)(value & AUDIO_SAMPLE_MASK);
				*(w_ptr++) = (TSample)(value >> 8 & AUDIO_SAMPLE_MASK);
				break;
			case util::EE_BIG_ENDIAN:
				*(w_ptr++) = (TSample)(value >> 8 & AUDIO_SAMPLE_MASK);
				*(w_ptr++) = (TSample)(value & AUDIO_SAMPLE_MASK);
				break;
			default:
				break;
		}
		m_written += 2 * AUDIO_SAMPLE_SIZE;
	}
	return nil;
}

PSample TAudioBuffer::write24Bit(uint32_t value, const util::EEndianType endian) {
	if (validBuffer() && validWriter()) {
		switch (endian) {
			case util::EE_LITTLE_ENDIAN:
				*w_ptr++ = (TSample)(value & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value >> 8 & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value >> 16 & AUDIO_SAMPLE_MASK);
				break;
			case util::EE_BIG_ENDIAN:
				*w_ptr++ = (TSample)(value >> 16 & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value >> 8 & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value & AUDIO_SAMPLE_MASK);
				break;
			default:
				break;
		}
		m_written += 3 * AUDIO_SAMPLE_SIZE;
	}
	return nil;
}

PSample TAudioBuffer::write32Bit(uint32_t value, const util::EEndianType endian) {
	if (validBuffer() && validWriter()) {
		switch (endian) {
			case util::EE_LITTLE_ENDIAN:
				*w_ptr++ = (TSample)(value & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value >> 8 & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value >> 16 & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value >> 24 & AUDIO_SAMPLE_MASK);
				break;
			case util::EE_BIG_ENDIAN:
				*w_ptr++ = (TSample)(value >> 24 & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value >> 16 & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value >> 8 & AUDIO_SAMPLE_MASK);
				*w_ptr++ = (TSample)(value & AUDIO_SAMPLE_MASK);
				break;
			default:
				break;
		}
		m_written += 4 * AUDIO_SAMPLE_SIZE;
	}
	return nil;
}


bool TAudioBuffer::validReader() const {
	if (util::assigned(r_ptr))
		return (r_ptr <= r_max);
	return false;
}

bool TAudioBuffer::validWriter() const {
	if (util::assigned(w_ptr))
		return (w_ptr <= w_max);
	return false;
}

bool TAudioBuffer::validBuffer() const {
	return (size() > 0) && util::assigned(data());
}

bool TAudioBuffer::isExhausted() const {
	return getRead() >= getWritten();
}

bool TAudioBuffer::isUnderrun() const {
	return getRead() >= (getWritten() - music::MP3_OUTPUT_CHUNK_SIZE);
}

bool TAudioBuffer::isStreamable() const {
	return (getRead() < (getWritten() - 2 * music::MP3_OUTPUT_CHUNK_SIZE)) || isFull();
}


bool TAudioBuffer::hasTrack() const {
	return util::assigned(m_track);
}

bool TAudioBuffer::hasSong() const {
	if (util::assigned(m_track))
		return util::assigned(m_track->getSong());
	return false;
}

bool TAudioBuffer::empty() const {
	bool used = false;
	if (validBuffer()) {
		used = (m_key > 0) || (r_ptr != data()) || (w_ptr != data()) || (m_written > 0) || (m_read > 0);
	}
	return !used;
}

std::string TAudioBuffer::bufferStatusToStr(const EBufferState value) {
	switch (value) {
		case EBS_EMPTY		: return "Empty";
		case EBS_ASSIGNED	: return "Assigned";
		case EBS_BUFFERING	: return "Buffering";
		case EBS_CONTINUE	: return "Continue";
		case EBS_BUFFERED	: return "Buffered";
		case EBS_LOADED		: return "Loaded";
		case EBS_FINISHED	: return "Finished";
		case EBS_PLAYING	: return "Playing";
		case EBS_DRAINING	: return "Draining";
		case EBS_PLAYED		: return "Played";
		case EBS_NONE		: return "Undefined";
	}
	return "unknown";
}

const std::string& TAudioBuffer::getHash() const {
	if (hasSong()) {
		if (m_track->getSong()->getFileData().isValid()) {
			return m_track->getSong()->getFileData().hash;
		}
	}
	return dummy;
}

const std::string& TAudioBuffer::getFile() const {
	if (hasSong()) {
		if (m_track->getSong()->getFileData().isValid()) {
			return m_track->getSong()->getFileData().filename;
		}
	}
	return dummy;
}

void TAudioBuffer::debugSong(const std::string& preamble) const {
	if (hasSong())
		std::cout << preamble << " Filename <" << m_track->getSong()->getFileName() << ">" << std::endl;
	else
		std::cout << preamble << " Song is unassigned." << std::endl;
}

void TAudioBuffer::debugOutput(const std::string& preamble) const {
	std::cout << preamble << "Unique ID      : " << m_key << std::endl;
	std::cout << preamble << "Status         : " << bufferStatusToStr(m_status) << std::endl;
	std::cout << preamble << "Buffer size    : " << buffer.size() << " Bytes [" << util::sizeToStr(buffer.size(), 1, util::VD_BINARY) << "]" << std::endl;
	std::cout << preamble << "Bytes read     : " << m_read << " Bytes [" << util::sizeToStr(m_read, 1, util::VD_BINARY) << "]" << std::endl;
	std::cout << preamble << "Bytes written  : " << m_written << " Bytes [" << util::sizeToStr(m_written, 1, util::VD_BINARY) << "]" << std::endl;
	std::cout << preamble << "Is allocated   : " << isAllocated() << std::endl;
	std::cout << preamble << "Is buffered    : " << isBuffered() << std::endl;
	std::cout << preamble << "Is loaded      : " << isLoaded() << std::endl;
	std::cout << preamble << "Is standalone  : " << isStandalone() << std::endl;
	std::cout << preamble << "Is last        : " << isLast() << std::endl;
	if (hasSong()) {
		std::cout << preamble << "File size      : " << m_track->getSong()->getSampleSize() << " Bytes [" << util::sizeToStr(m_track->getSong()->getSampleSize(), 1, util::VD_BINARY) << "]" << std::endl;
		std::cout << preamble << "Filename       : " << m_track->getSong()->getFileName() << std::endl;
	} else {
		std::cout << preamble << "Song           : <unassigned>" << std::endl;
	}
}

bool TAudioBuffer::saveToFile(const std::string fileName, const EPcmType type) const {
	return saveToFile(fileName, nil, type);
}

bool TAudioBuffer::saveToFile(const std::string fileName, const TAudioBufferList& buffers, const EPcmType type) const {
	return saveToFile(fileName, &buffers, type);
}

bool TAudioBuffer::saveToFile(const std::string fileName, const TAudioBufferList* buffers, const EPcmType type) const {
	util::deleteFile(fileName);
	if (util::assigned(data()) && m_written > 0) {
		TPCMEncoder encoder;
		if (type == EP_PCM_RAW) {
			return encoder.saveRawFile(fileName, this, buffers);
		}
		if (type == EP_PCM_WAVE) {
			return encoder.saveToFile(fileName, this, buffers);
		}
	}
	return false;
}



TAudioBufferList::TAudioBufferList() {
	m_key = 0;
	m_size = 0;
	m_free = 0;
	m_allocated = 0;
	m_empty = 0;
	debug = true;
}

TAudioBufferList::~TAudioBufferList() {
	clear();
}


void TAudioBufferList::debugOutput(const std::string& preamble) const {
	std::cout << preamble << " ===========================================" << std::endl;
	for (size_t i=0; i<count(); ++i) {
		PAudioBuffer o = list.at(i);
		if (util::assigned(o)) {
			std::cout << preamble << " Audio buffer item [" << util::succ(i) << "]" << std::endl;
			o->debugOutput(preamble + "  ");
		}
	}
	std::cout << preamble << " Audio free size = " << util::sizeToStr(free()) << std::endl;
}

void TAudioBufferList::clear() {
	sysutil::TMemInfo mem;
	size_t m;

	if (debug && count() > 0) {
		sysutil::getSystemMemory(mem);
		m = mem.memFree;
	}

	util::clearObjectList(list);

	if (debug && count() > 0) {
		sysutil::getSystemMemory(mem);
		std::cout << "TAudioBufferList::clear() Free memory before = " << util::sizeToStr(m) << std::endl;
		std::cout << "TAudioBufferList::clear() Free memory after  = " << util::sizeToStr(mem.memFree) << std::endl;
	}
}


void TAudioBufferList::create(const size_t fraction, const size_t maxBufferCount, const size_t minBufferSize, const size_t maxBufferSize) {
	sysutil::TMemInfo mem;
	if (sysutil::getSystemMemory(mem)) {

		// Scale memory for all buffers from 10% to 90% of available/free system memory
		size_t factor = fraction;
		if (fraction > 90) factor = 90;
		if (fraction < 10) factor = 10;
		factor /= 10;
		size_t free = mem.memFree * factor / 10;

		if (debug) {
			std::cout << "TAudioBufferList::create() Percent usage = " << fraction << std::endl;
			std::cout << "TAudioBufferList::create() Usable memory = " << util::sizeToStr(free) << std::endl;
			std::cout << "TAudioBufferList::create() Max. buffer count = " << maxBufferCount << std::endl;
			std::cout << "TAudioBufferList::create() Min. buffer size  = " << util::sizeToStr(minBufferSize) << std::endl;
			std::cout << "TAudioBufferList::create() Max. buffer size  = " << util::sizeToStr(maxBufferSize) << std::endl;
		}

		// Change buffer count until min. size fits into free memory
		bool found = false;
		size_t b_count = maxBufferCount;
		m_size = util::align(free / b_count, 8);
		m_allocated = 0;
		m_free = 0;
		do {
			// Calculate buffer size for given buffer count
			// --> Reduce buffer count until lower buffer size limit is exceeded
			while (m_size < minBufferSize && b_count >= 2) {
				b_count /= 2;
				m_size = util::align(free / b_count, 8);
			}

			// Reduce buffer size to some handsome value if needed
			// --> Increase buffer count instead
			if (m_size > maxBufferSize && b_count < maxBufferCount) {

				// Buffer size exceeds maximum, try to add 2 extra buffer
				m_size = util::align(maxBufferSize, 8);
				b_count += 2;
				if (b_count >= util::pred(maxBufferCount))
					found = true;

			} else {
				// Stop if buffer size is below limit
				found = true;
			}

		} while (!found);

		// Limit buffer size to given value
		if (m_size > maxBufferSize)
			m_size = util::align(maxBufferSize, 8);

		if (debug) {
			std::cout << "TAudioBufferList::create() Buffer count = " << b_count << std::endl;
			std::cout << "TAudioBufferList::create() Buffer size  = " << util::sizeToStr(buffer()) << std::endl;
			std::cout << "TAudioBufferList::create() Space used   = " << util::sizeToStr(b_count * buffer()) << std::endl;
		}

		// Create count of buffers for given size
		if (b_count >= 2) {
			for (size_t i=0; i<b_count; ++i) {
				PAudioBuffer o = new TAudioBuffer;
				o->resize(m_size);
				list.push_back(o);
				m_free += m_size;
				++m_empty;
				if (debug) {
					std::cout << "  TAudioBufferList::create() Buffer[" << util::succ(i) << "] created, size = " << util::sizeToStr(o->size()) << std::endl;
				}
			}
		} else {
			m_size = 0;
			throw util::app_error_fmt("TAudioBufferList::create() failed: Not enough memory to create buffers [%]", util::sizeToStr(mem.memFree));
		}

		if (debug) {
			size_t m1 = mem.memFree;
			sysutil::getSystemMemory(mem);
			size_t m2 = mem.memFree;
			std::cout << "TAudioBufferList::create() Free memory before = " << util::sizeToStr(m1) << std::endl;
			std::cout << "TAudioBufferList::create() Free memory after  = " << util::sizeToStr(m2) << std::endl;
			std::cout << "TAudioBufferList::create() Percent usage = " << 100 - ( m2 * 100 / m1 ) << "%" << std::endl;
		}

	} else {
		throw util::app_error("TAudioBufferList::create() failed: System memory read failed.");
	}
}

bool TAudioBufferList::isSufficient(const size_t needed) const {
	return (m_size - m_free) > needed;
}

bool TAudioBufferList::validIndex(const size_t index) const {
	return (index >= 0 && index < list.size());
}

PAudioBuffer TAudioBufferList::at(const std::size_t index) const {
	if (validIndex(index))
		return list.at(index);
	return nil;

}

PAudioBuffer TAudioBufferList::operator[] (const std::size_t index) const {
	return at(index);
};

bool listSorterAsc(const TAudioBuffer * o, const TAudioBuffer * p) {
	return o->getKey() < p->getKey();
}

bool listSorterDesc(const TAudioBuffer * o, const TAudioBuffer * p) {
	return o->getKey() > p->getKey();
}

void TAudioBufferList::sort(const util::ESortOrder order) {
	sort(order, listSorterAsc, listSorterDesc);
}

void TAudioBufferList::sort(const util::ESortOrder order, const TBufferSorter asc, const TBufferSorter desc) {
	TBufferSorter sorter;
	switch(order) {
		case util::SO_DESC:
			sorter = desc;
			break;
		case util::SO_ASC:
		default:
			sorter = asc;
			break;
	}
	sort(sorter);
}

void TAudioBufferList::sort(const TBufferSorter sorter) {
	std::sort(list.begin(), list.end(), sorter);
}


void TAudioBufferList::complete() {
	PAudioBuffer o;
	for (size_t i=0; i<count(); ++i) {
		o = list[i];
		if (util::assigned(o)) {
			// Reset ALL buffers to initial played state
			// --> Playback buffers is blocked
			if (o->getWritten() > 0 && o->getStatus() != EBS_EMPTY) {
					o->setStatus(EBS_PLAYED);
			}
		}
	}
}

void TAudioBufferList::complete(const TSong* song) {
	if (util::assigned(song)) {
		PSong s;
		PAudioBuffer o;
		for (size_t i=0; i<count(); ++i) {
			o = list[i];
			if (util::assigned(o)) {
				s = o->getSong();
				if (util::assigned(s)) {
					// Reset buffers for given song only to played state
					// --> Playback buffers is blocked
					if (o->getWritten() > 0 && o->getStatus() != EBS_EMPTY) {
						if (s->compareByTitleHash(song)) {
							o->setStatus(EBS_PLAYED);
						}
					}
				}
			}
		}
	}
}

void TAudioBufferList::cleanup() {
	PAudioBuffer o;
	for (size_t i=0; i<count(); ++i) {
		o = list[i];
		if (util::assigned(o)) {
			// Reset ALL buffer to initial loaded state
			// --> Playback buffers is possible again
			if (o->getWritten() > 0 && o->getStatus() != EBS_EMPTY) {
				o->cleanup();
			}
		}
	}
}

void TAudioBufferList::cleanup(PSong song) {
	if (util::assigned(song)) {
		PSong s;
		PAudioBuffer o;
		song->cleanup();
		for (size_t i=0; i<count(); ++i) {
			o = list[i];
			if (util::assigned(o)) {
				s = o->getSong();
				if (util::assigned(s)) {
					// Reset buffers for given song only to initial loaded state
					// --> Playback buffers is possible again
					if (o->getWritten() > 0 && o->getStatus() != EBS_EMPTY) {
						if (s->compareByTitleHash(song)) {
							o->cleanup();
						}
					}
				}
			}
		}
	}
}

void TAudioBufferList::cleanup(PAudioBuffer buffer) {
	if (util::assigned(buffer)) {
		buffer->cleanup();
	}
}

size_t TAudioBufferList::resetStreamBuffers() {
	PSong song;
	size_t r = 0;
	PAudioBuffer buffer;
	for (size_t i=0; i<count(); ++i) {
		buffer = list[i];
		if (util::assigned(buffer)) {
			song = buffer->getSong();
			if (util::assigned(song)) {
				// Reset stream buffers
				if (song->isStreamed()) {
					release(buffer);
					r++;
				}
			}
		}
	}
	return r;
}

size_t TAudioBufferList::reset(const util::hash_type hash, const TSong* current, const size_t index, const ECompareType type, size_t range) {
	PSong song;
	PTrack track;
	PAudioBuffer buffer;
	size_t idx, r = 0;
	for (size_t i=0; i<count(); ++i) {
		buffer = list[i];
		if (util::assigned(buffer)) {
			track = buffer->getTrack();
			if (util::assigned(track)) {
				song = track->getSong();
				idx = track->getIndex();
				// Reset all buffers with lower index
				// --> Must be either played or skipped songs
				bool destroy = hash != track->getHash();
				if (!destroy) {
					std::string name = "<unknown>";
					switch (type) {
						case ECT_LESSER:
							name = "ECT_LESSER";
							destroy = idx < index;
							break;
						case ECT_LESSER_OR_EQUAL:
							name = "ECT_LESSER_OR_EQUAL";
							destroy = idx <= index;
							break;
						case ECT_EQUAL:
							name = "ECT_EQUAL";
							destroy = idx == index;
							break;
						case ECT_NOT_EQUAL:
							name = "ECT_NOT_EQUAL";
							destroy = idx != index;
							break;
						case ECT_RANGE:
							name = "ECT_RANGE";
							destroy = (idx <= (index - range)) || (idx >= (index + range));
							break;
						case ECT_GREATER_OR_EQUAL:
							name = "ECT_GREATER_OR_EQUAL";
							destroy = idx >= index;
							break;
						case ECT_GREATER:
							name = "ECT_GREATER";
							destroy = idx > index;
							break;
						default:
							destroy = false;
							break;
					}
				}
				if (util::assigned(current)) {
					// Do NOT delete buffers for current song!
					if (util::assigned(song))
						destroy = destroy && (*current != *song);
				}
				if (destroy) {
					release(buffer);
					if (util::assigned(song))
						song->reset();
					++r;
				}
			}
		}
	}
	return r;
}

void TAudioBufferList::reset(const EBufferState state) {
	PSong s;
	PAudioBuffer o;
	for (size_t i=0; i<count(); ++i) {
		o = list[i];
		if (util::assigned(o)) {
			bool ok = true;
			if (state != EBS_NONE) {
				// Reset buffers for given state only
				ok = o->getStatus() == state;
			}
			if (ok) {
				release(o);
				s = o->getSong();
				if (util::assigned(s)) {
					s->reset();
				}
			}
		}
	}
}

void TAudioBufferList::reset(PSong song) {
	if (util::assigned(song)) {
		PSong s;
		PAudioBuffer buffer;
		song->reset();
		for (size_t i=0; i<count(); ++i) {
			buffer = list[i];
			if (util::assigned(buffer)) {
				s = buffer->getSong();
				if (util::assigned(s)) {
					// Reset buffers for given song only
					if (s->compareByTitleHash(song)) {
						release(buffer);
					}
				}
			}
		}
	}
}

void TAudioBufferList::reset(PAudioBuffer buffer) {
	if (util::assigned(buffer)) {
		release(buffer);
	}
}

void TAudioBufferList::reset() {
	PAudioBuffer buffer;
	for (size_t i=0; i<count(); ++i) {
		buffer = list[i];
		if (util::assigned(buffer)) {
			release(buffer);
		}
	}
}

size_t TAudioBufferList::garbageCollector(const music::TCurrentSongs& songs) {
	size_t r = 0;
	PAudioBuffer buffer;
	for (size_t i=0; i<count(); ++i) {
		buffer = list[i];
		if (util::assigned(buffer)) {
			PTrack track = buffer->getTrack();
			if (util::assigned(track)) {
				bool ok = true;
				track->setDeferred(false);
				PSong song = track->getSong();
				if (util::assigned(song)) {
					if (ok && util::assigned(songs.last)) {
						if (song->compareByTitleHash(songs.last))
							ok = false;
					}
					if (ok && util::assigned(songs.current)) {
						if (song->compareByTitleHash(songs.current))
							ok = false;
					}
					if (ok && util::assigned(songs.next)) {
						if (song->compareByTitleHash(songs.next))
							ok = false;
					}
				}
				if (track->isRemoved()) {
					if (ok) {
						release(buffer);
						++r;
					} else {
						track->setDeferred(true);
					}
				}
			}
		}
	}
	return r;
}

PAudioBuffer TAudioBufferList::getNextEmptyBuffer(const TTrack* track) {
	PSong song = nil;
	if (util::assigned(track))
		song = track->getSong();
	return getNextEmptyBuffer(song);
}

PAudioBuffer TAudioBufferList::getNextEmptyBuffer(const TSong* song) {
	PAudioBuffer o, buffer = nil;
	for (size_t i=0; i<count(); ++i) {
		o = list[i];
		if (util::assigned(o)) {
			// Find first free buffer
			if (!util::assigned(buffer) && o->empty()) {
				buffer = o;
				if (!util::assigned(song))
					break;
			}

			// Look for given song in buffer list
			// --> Prevent decoding same song/file twice in buffer list!
			if (util::assigned(song)) {
				PSong s = o->getSong();
				if (util::assigned(s)) {
					if (s->compareByTitleHash(song)) {
						buffer = nil;
						break;
					}
				}
			}
		}
	}
	if (util::assigned(buffer)) {
		// Allocate and sort list ascending to simplify finding next song entry
		allocate(buffer);
		sort(util::SO_ASC, listSorterAsc, listSorterDesc);
	}
	return buffer;
}

PAudioBuffer TAudioBufferList::getNextEmptyBuffer() {
	PAudioBuffer o, buffer = nil;
	for (size_t i=0; i<count(); ++i) {
		o = list[i];
		if (util::assigned(o)) {
			// Find first free buffer
			if (!util::assigned(buffer) && o->empty()) {
				buffer = o;
				break;
			}
		}
	}
	if (util::assigned(buffer)) {
		// Adjust allocated buffer sizes and sort buffer list ascending to simplify finding next song entry
		allocate(buffer);
		sort(util::SO_ASC, listSorterAsc, listSorterDesc);
	}
	return buffer;
}

void TAudioBufferList::allocate(PAudioBuffer buffer) {
	if (!buffer->isAllocated()) {
		m_allocated += buffer->size();
		if (m_free > buffer->size())
			m_free -= buffer->size();
		else
			m_free = 0;
		if (m_empty > 0)
			--m_empty;
		buffer->setAllocated(true);
	}
	buffer->setKey(getNextKey());
}

void TAudioBufferList::release(PAudioBuffer buffer) {
	if (buffer->isAllocated()) {
		if (m_allocated > buffer->size())
			m_allocated -= buffer->size();
		else
			m_allocated = 0;
		m_free += buffer->size();
		if (m_empty < count())
			++m_empty;
	}
	buffer->reset();
}


PAudioBuffer TAudioBufferList::getNextSongBuffer(const TTrack* track, const bool debug) {
	if (util::assigned(track))
		return getNextSongBuffer(track->getSong(), debug);
	return nil;
}

PAudioBuffer TAudioBufferList::getNextSongBuffer(const TSong* song, const bool debug) {
	if (this->debug || debug) {
		if (util::assigned(song))
			std::cout << "TAudioBufferList::getNextSongBuffer() Find buffer for song <" << song->getTitle() << ">" << std::endl;
		else
			std::cout << "TAudioBufferList::getNextSongBuffer() Invalid song." << std::endl;
	}
	if (util::assigned(song)) {
		PAudioBuffer o = getCurrentBuffer(song, debug);
		if (!util::assigned(o)) {
			if (this->debug || debug) {
				std::cout << "TAudioBufferList::getNextSongBuffer() Find next Buffer, given song is " << (util::assigned(song) ? "valid" : "invalid") << "." << std::endl;
			}
			// Is buffer a child of given song?
			o = getNextBuffer(song, debug);
		}
		if (this->debug || debug) {
			if (util::assigned(o))
				std::cout << "TAudioBufferList::getNextSongBuffer() Buffer <" << o->getKey() << "> found." << std::endl;
			else
				std::cout << "TAudioBufferList::getNextSongBuffer() No buffer found." << std::endl;
			}
		return o;
	}
	return nil;
}


PAudioBuffer TAudioBufferList::getCurrentBuffer(const TSong* song, const bool debug) {
	if (util::assigned(song)) {
		PSong s;
		PAudioBuffer o;
		for (size_t i=0; i<count(); ++i) {
			o = list[i];
			if (util::assigned(o)) {
				// Find first free buffer and occupy it by setting the key value
				// Sort list ascending to simplify finding next song entry
				s = o->getSong();
				if (util::assigned(s)) {
					if (s->compareByTitleHash(song)) {
						// Find buffer for given song that is playing
						if (o->isPlaying()) {
							return o;
						}
					}
				}
			}
		}
	}
	return nil;
}


PAudioBuffer TAudioBufferList::getNextBuffer(const TSong* song, const bool debug) {
	if (util::assigned(song)) {
		PSong s;
		PAudioBuffer o;
		for (size_t i=0; i<count(); ++i) {
			o = list[i];
			if (util::assigned(o)) {
				// Find first free buffer and occupy it by setting the key value
				// Sort list ascending to simplify finding next song entry
				s = o->getSong();
				if (util::assigned(s)) {
					// Find next buffer for same song that is not yet played
					if (this->debug || debug) {
						std::cout << "TAudioBufferList::getNextBuffer() Buffer <" << o->getKey() << "> buffered = " << o->isBuffered() << ", state = <" << TAudioBuffer::bufferStatusToStr(o->getStatus()) << "> (" << s->getTitleHash() << "/" << song->getTitleHash() << ")" << std::endl;
					}
					if (s->compareByTitleHash(song)) {
						if (song->isStreamed()) {
							// Draining buffer afer underrun is valid
							if ((o->isBuffered() || o->isDraining())) {
								if (this->debug || debug) {
									std::cout << "TAudioBufferList::getNextBuffer() Valid stream buffer <" << o->getKey() << "> found, state is <" << TAudioBuffer::bufferStatusToStr(o->getStatus()) << "> (" << s->getTitleHash() << "/" << song->getTitleHash() << ")" << std::endl;
								}
								return o;
							}
						} else {
							if (o->isBuffered()) {
								if (this->debug || debug) {
									std::cout << "TAudioBufferList::getNextBuffer() Valid son buffer <" << o->getKey() << "> found, buffered = " << o->isBuffered() << " (" << s->getTitleHash() << "/" << song->getTitleHash() << ")" << std::endl;
								}
								return o;
							}
						}
					}
				}
			}
		}
	}
	return nil;
}


PAudioBuffer TAudioBufferList::getForwardBuffer(const PAudioBuffer buffer, const TSong* song) {
	if (util::assigned(song)) {
		bool found = false;
		PAudioBuffer o;
		for (size_t i=0; i<count(); ++i) {
			o = list[i];
			if (util::assigned(o)) {
				if (found) {
					PSong s = o->getSong();
					if (util::assigned(s)) {
						// Find next buffer for same song that is not yet played
						if ((o->isLoaded() || o->isPlayed()) && (s->compareByTitleHash(song))) {
							return o;
						}
					}
					return nil;
				} else {
					// Current buffer found in list
					// --> Nest buffer in list might be the next buffer to play
					if (o->getKey() == buffer->getKey()) {
						found = true;
					}
				}
			}
		}
	}
	return nil;
}

PAudioBuffer TAudioBufferList::getRewindBuffer(const PAudioBuffer buffer, const TSong* song) {
	if (util::assigned(song)) {
		PAudioBuffer o, last = nil;
		for (size_t i=0; i<count(); ++i) {
			o = list[i];
			if (util::assigned(o)) {
				// Current buffer found in list
				// --> Previous buffer in list might be the next buffer to play
				if (o->getKey() == buffer->getKey()) {
					if (util::assigned(last)) {
						PSong s = last->getSong();
						if (util::assigned(s)) {
							// Find next buffer for same song that is not yet played
							if ((last->isLoaded() || last->isPlayed()) && (s->compareByTitleHash(song))) {
								return last;
							}
						}
					}
					return nil;
				}
			}
			last = o;
		}
	}
	return nil;
}

PAudioBuffer TAudioBufferList::getSeekBuffer(const TSong* song, const size_t position, size_t& offset) {
	offset = 0;
	if (util::assigned(song)) {
		if (position < song->getSampleSize()) {
			size_t size = 0;
			size_t last = 0;
			PAudioBuffer o;
			for (size_t i=0; i<count(); ++i) {
				o = list[i];
				if (util::assigned(o)) {
					PSong s = o->getSong();
					if (util::assigned(s)) {
						if (s->compareByTitleHash(song)) {
							size += o->getWritten();
							if (size > position) {
								offset = position - last;
								return o;
							}
							last = size;
						}
					}
				}
			}
		}
	}
	return nil;
}


bool TAudioBufferList::hasSong(const TSong* song) const {
	if (util::assigned(song)) {
		PSong s;
		PAudioBuffer o;
		for (size_t i=0; i<count(); ++i) {
			o = list[i];
			if (util::assigned(o)) {
				s = o->getSong();
				if (util::assigned(s)) {
					// Is requested song in buffer?
					if (s->compareByTitleHash(song))
						return true;
				}
			}
		}
	}
	return false;
}


bool TAudioBufferList::hasFile(const std::string& fileHash) const {
	PSong s;
	PAudioBuffer o;
	for (size_t i=0; i<count(); ++i) {
		o = list[i];
		if (util::assigned(o)) {
			s = o->getSong();
			if (util::assigned(s)) {
				// Is requested song in buffer?
				if (s->getFileHash() == fileHash)
					return true;
			}
		}
	}
	return false;
}


bool TAudioBufferList::hasAlbum(const std::string& albumHash) const {
	PSong s;
	PAudioBuffer o;
	for (size_t i=0; i<count(); ++i) {
		o = list[i];
		if (util::assigned(o)) {
			s = o->getSong();
			if (util::assigned(s)) {
				// Is requested song in buffer?
				if (s->getAlbumHash() == albumHash)
					return true;
			}
		}
	}
	return false;
}


void TAudioBufferList::operate(PAudioBuffer buffer, PTrack track, const EBufferState state, const EBufferLevel level) {
	operate(buffer, track, state);
	setLevel(buffer, level);
}


void TAudioBufferList::operate(PAudioBuffer buffer, PTrack track, const EBufferState state) {
	if (util::assigned(buffer) && util::assigned(track)) {
		PSong song = track->getSong();
		switch (state) {

			// Start buffering for new song
			case EBS_BUFFERING:
				buffer->prepare();
				buffer->setFirst(true);
				buffer->setStatus(EBS_BUFFERING);
				buffer->setTrack(track);
				if (util::assigned(song)) {
					song->clearStatsistics();
					song->setBuffered(false);
				}
				break;

			// Continue buffering with next buffer
			case EBS_CONTINUE:
				buffer->prepare();
				buffer->setStatus(EBS_BUFFERING);
				buffer->setTrack(track);
				break;

			// Buffering limit reached
			case EBS_BUFFERED:
				buffer->setStatus(music::EBS_BUFFERED);
				if (!util::assigned(buffer->getTrack())) {
					buffer->setTrack(track);
				}
				if (!util::assigned(song))
					song = buffer->getSong();
				if (util::assigned(song)) {
					song->setBuffered(true);
				}
				break;

			// Current buffer fully loaded with song data
			case EBS_LOADED:
				buffer->setStatus(music::EBS_LOADED);
				break;

			// Buffering finished, current buffer is last buffer for song
			case EBS_FINISHED:
				buffer->setStatus(music::EBS_LOADED);
				buffer->setLast(true);
				break;

			default:
				break;
		}
	}
}


void TAudioBufferList::operate(PAudioBuffer buffer, const EBufferState state, const bool force) {
	if (util::assigned(buffer)) {
		EBufferState current = buffer->getStatus();
		switch (state) {

			// Set playing state if fully buffered
			case EBS_PLAYING:
				if (current != EBS_PLAYING && current >= EBS_BUFFERED) {
					force ? buffer->forceStatus(EBS_PLAYING) : buffer->setStatus(EBS_PLAYING);
				}
				break;

			default:
				force ? buffer->forceStatus(state) : buffer->setStatus(state);
				break;
		}
	}
}


void TAudioBufferList::setLevel(PAudioBuffer buffer, const EBufferLevel level) {
	if (util::assigned(buffer)) {
		buffer->setLevel(level);
	}
}


} /* namespace music */
