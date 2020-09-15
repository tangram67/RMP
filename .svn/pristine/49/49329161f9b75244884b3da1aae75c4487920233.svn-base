/*
 * audiobuffer.h
 *
 *  Created on: 14.08.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef AUDIOBUFFER_H_
#define AUDIOBUFFER_H_

#include <vector>
#include "audioconsts.h"
#include "audiotypes.h"
#include "endianutils.h"
#include "stringtypes.h"
#include "semaphore.h"
#include "nullptr.h"
#include "memory.h"
#include "tags.h"

namespace music {

class TAudioBuffer;
class TAudioBufferList;

#ifdef STL_HAS_TEMPLATE_ALIAS

using TKeyValue = size_t;
using PAudioBuffer = TAudioBuffer*;
using PAudioBufferList = TAudioBufferList*;
using TBufferList = std::vector<PAudioBuffer>;
using TBufferSorter = std::function<bool(const TAudioBuffer *, const TAudioBuffer *)>;

#else

typedef size_t TKeyValue;
typedef TAudioBuffer* PAudioBuffer;
typedef TAudioBufferList* PAudioBufferList;
typedef std::vector<PAudioBuffer> TBufferList;
typedef std::function<bool(const TAudioBuffer *, const TAudioBuffer *)> TBufferSorter;

#endif


enum EBufferState {
	EBS_EMPTY = 0,
	EBS_ASSIGNED = 1,
	EBS_BUFFERING = 2,
	EBS_CONTINUE = 3,
	EBS_BUFFERED = 4,
	EBS_LOADED = 5,
	EBS_FINISHED = 6,
	EBS_PLAYING = 7,
	EBS_DRAINING = 8,
	EBS_PLAYED = 9,
	EBS_NONE = 99
};

struct TBufferStateName {
	EBufferState state;
	const char* name;
};

static const struct TBufferStateName bufferstates[] =
{
	{ EBS_EMPTY,     "Empty"     },
	{ EBS_ASSIGNED,  "Assigned"  },
	{ EBS_BUFFERING, "Buffering" },
	{ EBS_CONTINUE,  "Continue"  },
	{ EBS_BUFFERED,  "Buffered"  },
	{ EBS_LOADED,    "Loaded"    },
	{ EBS_FINISHED,  "Finished"  },
	{ EBS_PLAYING,   "Playing"   },
	{ EBS_DRAINING,  "Daining"   },
	{ EBS_PLAYED,    "Played"    },
	{ EBS_NONE,  	  nil        }
};


enum EBufferLevel {
	EBL_EMPTY = 0,
	EBL_BUFFERING = 1,
	EBL_BUFFERED = 2,
	EBL_FULL = 3,
	EBL_NONE = 99
};

struct TBufferLevelName {
	EBufferLevel level;
	const char* name;
};

static const struct TBufferLevelName levelstates[] =
{
	{ EBL_EMPTY,     "Empty"     },
	{ EBL_BUFFERING, "Buffering" },
	{ EBL_BUFFERED,  "Buffered"  },
	{ EBL_FULL,      "Full"      },
	{ EBL_NONE,  	  nil        }
};


enum ECompareType {
	ECT_LESSER,
	ECT_LESSER_OR_EQUAL,
	ECT_RANGE,
	ECT_EQUAL,
	ECT_NOT_EQUAL,
	ECT_GREATER_OR_EQUAL,
	ECT_GREATER
};


class TAudioBuffer {
private:
	TSampleBitBuffer buffer;
	PSample r_ptr;
	PSample w_ptr;
	PSample r_max;
	PSample w_max;
	size_t m_written;
	size_t m_read;
	TKeyValue m_key;
	PTrack m_track;
	bool m_last;
	bool m_first;
	bool m_allocated;
	EBufferState m_status;
	EBufferLevel m_level;
	std::string dummy;

	void prime();
	void init();

public:
	void clear();
	void reset();
	void prepare();
	void resetReader();
	void resetWriter();
	void resize(const size_t size);
	void cleanup();

	size_t size() const { return buffer.size(); };
	PSample data() const { return buffer.data(); };
	size_t getWritten() const { return m_written; };
	size_t getRead() const { return m_read; };

	const std::string& getFile() const;
	const std::string& getHash() const;

	PSample reader() { return r_ptr; };
	PSample writer() { return w_ptr; };

	bool validBuffer() const;
	bool hasTrack() const;
	bool hasSong() const;
	bool empty() const;

	static std::string bufferStatusToStr(const EBufferState value);
	EBufferState getStatus() const { return m_status; };
	void setStatus(const EBufferState value) { if (value > m_status) m_status = value; };
	EBufferLevel getLevel() const { return m_level; };
	void setLevel(const EBufferLevel value) { m_level = value; };
	void forceStatus(const EBufferState value) { m_status = value; };
	void clearStatus(const EBufferState value) { m_status = EBS_EMPTY; };

	bool isAllocated() const { return m_allocated; };
	void setAllocated(const bool value) { m_allocated = value; };
	bool isBuffered() const { return util::isMemberOf(m_status, EBS_BUFFERED,EBS_LOADED); };
	bool isDraining() const { return (m_status == EBS_DRAINING); };
	bool isLoaded() const { return (m_status == EBS_LOADED); };
	bool isUnused() const { return (m_status == EBS_EMPTY); };
	bool isPlaying() const { return (m_status == EBS_PLAYING); }; //util::isMemberOf(m_status, EBS_PLAYING,EBS_DRAINING); };
	bool isPlayed() const { return (m_status == EBS_PLAYED); };
	bool isFull() const { return (m_level == EBL_FULL); };

	bool isUnderrun() const;
	bool isStreamable() const;
	bool isExhausted() const;

	bool isFirst() const { return m_first; };
	void setFirst(const bool value) { m_first = value; };
	bool isLast() const { return m_last; };
	void setLast(const bool value) { m_last = value; };
	bool isStandalone() const { return m_last && m_first; };

	PSample write16Bit(uint16_t value, const util::EEndianType endian = util::EE_LITTLE_ENDIAN);
	PSample write24Bit(uint32_t value, const util::EEndianType endian = util::EE_LITTLE_ENDIAN);
	PSample write32Bit(uint32_t value, const util::EEndianType endian = util::EE_LITTLE_ENDIAN);

	PSample rewind(const size_t size);
	PSample read(const size_t size);
	PSample write(const size_t size);
	void setWritten(const size_t size);

	bool validReader() const;
	bool validWriter() const;

	void setKey(const TKeyValue value) { m_key = value; };
	TKeyValue getKey() const { return m_key; };
	void setTrack(const PTrack value) { m_track = value; };
	PTrack getTrack() const { return m_track; };
	PSong getSong() const;

	void debugSong(const std::string& preamble = "") const;
	void debugOutput(const std::string& preamble = "") const;

	bool saveToFile(const std::string fileName, const EPcmType type = EP_PCM_WAVE) const;
	bool saveToFile(const std::string fileName, const TAudioBufferList* buffers, const EPcmType type = EP_PCM_WAVE) const;
	bool saveToFile(const std::string fileName, const TAudioBufferList& buffers, const EPcmType type = EP_PCM_WAVE) const;

	TAudioBuffer();
	~TAudioBuffer();
};


class TAudioBufferList {
private:
	TBufferList list;
	size_t m_size;
	size_t m_empty;
	size_t m_allocated;
	size_t m_free;
	bool debug;
	TKeyValue m_key;

	bool validIndex(const size_t index) const;
	void sort(const util::ESortOrder order, const TBufferSorter asc, const TBufferSorter desc);
	void sort(const TBufferSorter sorter);

	void allocate(PAudioBuffer buffer);
	void release(PAudioBuffer buffer);

	TKeyValue getNextKey() { return ++m_key; };
	PAudioBuffer getNextBuffer(const TSong* song, const bool debug = false);
	PAudioBuffer getCurrentBuffer(const TSong* song, const bool debug = false);

public:
	void create(const size_t fraction, const size_t maxBufferCount = MAX_BUFFER_COUNT,
			const size_t minBufferSize = MIN_BUFFER_SIZE, const size_t maxBufferSize = MAX_BUFFER_SIZE);
	void clear();
	void cleanup();
	void cleanup(PSong song);
	void cleanup(PAudioBuffer buffer);
	void complete();
	void complete(const TSong* song);

	void operate(PAudioBuffer buffer, PTrack track, const EBufferState state, const EBufferLevel level);
	void operate(PAudioBuffer buffer, const PTrack track, const EBufferState state);
	void operate(PAudioBuffer buffer, const EBufferState state, const bool force = false);
	void setLevel(PAudioBuffer buffer, const EBufferLevel level);

	size_t garbageCollector(const music::TCurrentSongs& songs);
	size_t resetStreamBuffers();

	size_t reset(const util::hash_type hash, const TSong* current, const size_t index, const ECompareType type = ECT_LESSER, size_t range = 0);
	void reset(const EBufferState state);
	void reset(PSong song);
	void reset(PAudioBuffer buffer);
	void reset();

	void setDebug(const bool value) { debug = value; };
	bool getDebug() const { return debug; };

	size_t count() const { return list.size(); };
	size_t free() const { return m_free; };
	size_t allocated() const { return m_allocated; };
	size_t empty() const { return m_empty; };
	size_t buffer() const { return m_size; };
	bool isSufficient(const size_t needed) const;
	bool hasSong(const TSong* song) const;
	bool hasFile(const std::string& fileHash) const;
	bool hasAlbum(const std::string& albumHash) const;

	PAudioBuffer getNextEmptyBuffer();
	PAudioBuffer getNextEmptyBuffer(const TSong* song);
	PAudioBuffer getNextEmptyBuffer(const TTrack* track);
	PAudioBuffer getNextSongBuffer(const TSong* song, const bool debug = false);
	PAudioBuffer getNextSongBuffer(const TTrack* track, const bool debug = false);
	void sort(const util::ESortOrder order = util::SO_ASC);

	PAudioBuffer getRewindBuffer(const PAudioBuffer buffer, const TSong* song);
	PAudioBuffer getForwardBuffer(const PAudioBuffer buffer, const TSong* song);
	PAudioBuffer getSeekBuffer(const TSong* song, const size_t position, size_t& offset);

	PAudioBuffer at(const std::size_t index) const;
	PAudioBuffer operator[] (const std::size_t index) const;

	void debugOutput(const std::string& preamble = "") const;

	TAudioBufferList();
	~TAudioBufferList();
};

} /* namespace music */

#endif /* AUDIOBUFFER_H_ */
