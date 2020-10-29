/*
 * audiofile.h
 *
 *  Created on: 22.12.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef AUDIOFILE_H_
#define AUDIOFILE_H_

#include <string>
#include "nullptr.h"
#include "audiotypes.h"
#include "audiostream.h"
#include "datetime.h"
#include "tagtypes.h"
#include "tags.h"

namespace music {


class TTrack {
private:
	PSong song;
	size_t index;
	bool deleted;
	bool removed;
	bool deferred;
	bool randomized;
	bool streamable;
	util::hash_type hash;
	std::string file;

	void prime();

public:
	PSong getSong() const { return song; };
	void setSong(const PSong value);
	size_t getIndex() const { return index; };
	void setIndex(const size_t value) { index = value; };
	const std::string& getFile() const { return file; };
	void setFile(const std::string& value) { file = value; };
	bool isDeleted() const { return deleted; };
	void setDeleted(const bool value) { deleted = value; };
	bool isRemoved() const { return removed; };
	void setRemoved(const bool value) { removed = value; };
	bool isDeferred() const { return deferred; };
	void setDeferred(const bool value) { deferred = value; };
	bool isStreamable() const { return streamable; };
	void setStreamable(const bool value) { streamable = value; };
	bool isRandomized() const { return randomized; };
	void setRandomized(const bool value) { randomized = value; };
	util::hash_type getHash() const { return hash; };
	void setHash(const util::hash_type value) { hash = value; };
	void setHash(const std::string& value) { hash = util::calcHash(value); };

	TTrack();
	virtual ~TTrack();
};


class TAudioFile {
private:
	bool readFileProperties(TFileTag& tag);
	std::string createFileHash(TFileTag& tag);

public:
	void setDefaultProperties(const std::string& fileName, TFileTag& tag);
	void setFileProperties(const std::string& fileName, TFileTag& tag);

	TAudioFile();
	TAudioFile(const std::string& fileName, TFileTag& tag);
	virtual ~TAudioFile();
};


class TAudioConvert {
private:
	static int strToInt(const char * const value, char ** next, const int defValue);

public:
	static int tagToInt(const std::string& value, const int defValue);
	static CTagValues tagToValues(const std::string& value, const int defValue);
	static int normalizeSampleSize(const int bits);
	static size_t bitRateFromTags(const TFileTag& tag);

	TAudioConvert();
	virtual ~TAudioConvert();
};


class TSong : public TAudioStreamAdapter, public TAudioConvert {
friend class TLibrary;

private:
	ECodecType type;
	EMediaType media;
	std::string icon;
	TFileTag tags;
	util::TInodeHandle node;
	util::TDateTime modtime;
	util::TStringList json;
	util::TStringList m3u;
	TSongIterator iterator;
	bool streamable;
	size_t index;
	bool deleted;
	bool loaded;
	bool buffered;
	bool streamed;
	bool changed;
	bool valid;
	int params;

	std::string c_playlist;
	bool c_activated;
	bool c_extended;

	void prime();
	void addKeyValue(const std::string& preamble, const std::string& key, const std::string& value,
			const bool quote = false, const bool last = false);
	void setFileProperties(const std::string& fileName);
	void setFileProperties(const TFileTag tag);
	util::hash_type hash(const std::string& value);
	std::string md5(const std::string& value);
	std::string typeAsString(const ECodecType value) const;

	std::string encode(const std::string& text, const bool encoded);
	std::string decode(const std::string& text, const bool encoded);
	int extractTrackNumber(const std::string& name, size_t& offset, const int defaultTrack = 1);
	std::string extractTrackTitle(const std::string& name, size_t offset);
	bool extractAlbumArtist(const std::string& path, std::string& album, std::string& artist);
	int readMetadataTags(const bool allowArtistNameRestore, const bool allowFullNameSwap, const bool allowGroupNameSwap,
			const bool allowTheBandPrefixSwap, const bool allowVariousArtistsRename, const bool allowMovePreamble);
	int evalScannerParams(const bool allowArtistNameRestore, const bool allowFullNameSwap, const bool allowGroupNameSwap,
			const bool allowTheBandPrefixSwap, const bool allowVariousArtistsRename, const bool allowMovePreamble);
	void setScannerParams(const bool allowArtistNameRestore, const bool allowFullNameSwap, const bool allowGroupNameSwap,
			const bool allowTheBandPrefixSwap, const bool allowVariousArtistsRename, const bool allowMovePreamble);
	void normalizeTags(const bool allowGroupNameSwap);
	void setCompilationTags(const std::string& artist);
	void setSortTags();
	void setDisplayTags();
	void setCompareTags();
	void setExtendedTags(CTextMetaData& info);
	bool getStreamable(const ECodecType type) const;
	size_t findArtistSeparator(const std::string& name);
	size_t findArtistLastName(const std::string& name);
	bool moveAlbumPreamble(std::string& name);
	bool restoreArtistName(std::string& name);
	bool restoreArtistName(std::string& name, const size_t separator);
	bool swapTheBandPrefix(std::string& name);
	bool swapTheBandPrefix(std::string& name, const size_t separator);
	void swapArtistName(std::string& name, const size_t separator);
	void renameVariousArtists();
	void sanitizeTags();
	std::string paramsAsString(int param) const;
	int paramsFromString(const std::string& param) const;
	void updateProperties();

protected:
	bool isSupported(std::string& hint) const;
	bool compare(const std::string& s1, const std::string& s2) const;
	std::string getAlbumSortName() const;
	bool isVariousArtistName(const std::string& name) const;

public:

	virtual bool readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) = 0;
	virtual bool readPictureData(const std::string& fileName, TCoverData& cover) = 0;

	void setDefaultTags(const std::string& fileName, const int defaultTrack = 1);
	void setDefaultFileProperties(const std::string& fileName, const ESampleRate sampleRate, int bitsPerSample);
	void setDefaultStreamProperties(const ESampleRate sampleRate, int bitsPerSample);
	void updateStreamProperties(const ESampleRate sampleRate, int bitsPerSample);

	size_t getBytesPerSample() const;
	size_t getBitsPerSample() const { return tags.stream.bitsPerSample; };
	size_t getBytesPerSecond() const { return tags.stream.sampleSize / tags.stream.seconds; };
	size_t getWordWidth() const { return getChannelCount() * getBytesPerSample(); };
	size_t getBitRate() const { return tags.stream.bitRate; };
	int getSampleRate() const { return tags.stream.sampleRate; };
	size_t getSampleSize() const { return tags.stream.sampleSize; };
	size_t getSampleCount() const { return tags.stream.sampleCount; };
	size_t getChunkSize() const { return tags.stream.chunkSize; };
	void setChunkSize(const size_t value) { tags.stream.chunkSize = value; };
	size_t getChannelCount() const { return tags.stream.channels; };
	bool isStreamable() const { return streamable; };
	bool isStreamed() const { return streamed; };
	void setStreamed(const bool value) { streamed = value; };

	bool isDSD() const { return util::isMemberOf(type, EFT_DSF,EFT_DFF); };
	bool isHDCD() const { return (tags.stream.sampleRate == (int)SR44K) && (tags.stream.bitsPerSample > (int)ES_CD); };

	void setIndex(const size_t value) { index = value; };
	size_t getIndex() const { return index; };
	void setIterator(const TSongIterator& iterator) { this->iterator = iterator; };
	const TSongIterator& getIterator() const { return iterator; };
	int getScannerParams() const { return params; };

	const util::TDateTime& getModTime() const { return modtime; };
	void setModTime(const std::string& time) { modtime = time; };
	void setModTime(const util::TDateTime& time) { modtime = time; };
	void setModTime(const util::TTimePart& time) { modtime = time; };
	void touch() { modtime.sync(); };

	void setNode(const util::TInodeHandle value) { node = value; };
	util::TInodeHandle getNode() const { return node; };

	CMetaData& getTags() { return tags.meta; };
	const TFileTag& getFileTags() const { return tags; };
	const CMetaData& getMetaData() const { return tags.meta; };
	const CStreamData& getStreamData() const { return tags.stream; };
	const CFileData& getFileData() const { return tags.file; };
	const std::string& getError() const { return tags.error; };
	const std::string& getIcon() const { return icon; };

	const std::string& getDate() const { return tags.meta.text.year; };
	const std::string& getAlbum() const { return tags.meta.text.album; };
	const std::string& getArtist() const { return tags.meta.text.artist; };
	const std::string& getComposer() const { return tags.meta.text.composer; };
	const std::string& getConductor() const { return tags.meta.text.conductor; };
	const std::string& getOriginalArtist() const { return tags.meta.text.originalartist; };
	const std::string& getAlbumArtist() const { return tags.meta.text.albumartist; };
	const std::string& getOriginalAlbumArtist() const { return tags.meta.text.originalalbumartist; };
	const std::string& getExtendedAlbum() const { return tags.meta.text.extendedalbum; };
	const std::string& getExtendedInfo() const { return tags.meta.text.extendedinfo; };
	const std::string& getTitle() const { return tags.meta.text.title; };
	const std::string& getTrack() const { return tags.meta.text.track; };
	const std::string& getGenre() const { return tags.meta.text.genre; };
	const std::string& getYear() const { return tags.meta.text.year; };

	const std::string& getDisplayDate() const { return tags.meta.display.year; };
	const std::string& getDisplayAlbum() const { return tags.meta.display.album; };
	const std::string& getDisplayArtist() const { return tags.meta.display.artist; };
	const std::string& getDisplayComposer() const { return tags.meta.display.composer; };
	const std::string& getDisplayConductor() const { return tags.meta.display.conductor; };
	const std::string& getDisplayOriginalArtist() const { return tags.meta.display.originalartist; };
	const std::string& getDisplayAlbumArtist() const { return tags.meta.display.albumartist; };
	const std::string& getDisplayOriginalAlbumArtist() const { return tags.meta.display.originalalbumartist; };
	const std::string& getDisplayExtendedAlbum() const { return tags.meta.display.extendedalbum; };
	const std::string& getDisplayExtendedInfo() const { return tags.meta.display.extendedinfo; };
	const std::string& getDisplayTitle() const { return tags.meta.display.title; };
	const std::string& getDisplayTrack() const { return tags.meta.display.track; };
	const std::string& getDisplayGenre() const { return tags.meta.display.genre; };
	const std::string& getDisplayYear() const { return tags.meta.display.year; };

	const std::string& getAlbumSort() const { return tags.sort.album; };
	const std::string& getArtistSort() const { return tags.sort.artist; };
	const std::string& getAlbumArtistsSort() const { return tags.sort.albumartist; };
	int getYearSort() const { return tags.sort.year; };

	util::hash_type getAlbumSortHash() const { return tags.sort.albumHash; };
	util::hash_type getArtistSortHash() const { return tags.sort.artistHash; };
	util::hash_type getAlbumArtistSortHash() const { return tags.sort.albumartistHash; };

	const std::string& getTitleHash() const { return tags.meta.hash.title; };
	const std::string& getArtistHash() const { return tags.meta.hash.artist; };
	const std::string& getAlbumArtistHash() const { return tags.meta.hash.albumartist; };
	const std::string& getAlbumHash() const { return tags.meta.hash.album; };

	const std::string& getFileHash() const { return tags.file.hash; };
	const std::string& getFileName() const { return tags.file.filename; };
	const std::string& getFileExtension() const { return tags.file.extension; };
	const std::string& getFolder() const { return tags.file.folder; };
	const std::string& getURL() const { return tags.file.url; };
	util::TTimePart getFileTime() const { return tags.file.time; };
	util::TTimePart getInsertedTime() const { return tags.file.inserted; };
	size_t getFileSize() const { return tags.file.size; };

	int getTrackNumber() const { return tags.meta.track.tracknumber; };
	int getDiskNumber() const { return tags.meta.track.disknumber; };
	int getTrackCount() const { return tags.meta.track.trackcount; };
	void setTrackCount(const int value) { tags.meta.track.trackcount = value; };
	int getDiskCount() const { return tags.meta.track.diskcount; };
	void setDiskCount(const int value) { tags.meta.track.diskcount = value; };

	bool compareByTitleHash(const std::string& hash) const;
	bool compareByTitleHash(const TSong* song) const;
	bool compareByTitleHash(const TSong& song) const;
	bool compareByFileHash(const std::string& hash) const;
	bool compareByFileHash(const TSong* song) const;
	bool compareByFileHash(const TSong& song) const;

	inline bool operator == (const TSong& song) const { return compareByTitleHash(song); };
	inline bool operator != (const TSong& song) const { return !compareByTitleHash(song); };

	static ECodecType getFileType(const std::string& fileName);
	EMediaType getMediaType() const { return media; };
	ECodecType getType() const { return type; };
	void setType(const ECodecType value);
	bool hasCodec() const { return !tags.stream.codec.empty(); };
	std::string getCodec() const { return tags.stream.codec; };
	std::string getModulation() const { return isDSD() ? "DSD over PCM (PWM)" : "Pulse Code Modulation (PCM)"; };

	bool isValid() const { return valid; };
	void setValid(const bool value) { valid = value; };
	bool isDeleted() const { return deleted; };
	void setDeleted(const bool value) { deleted = value; };
	bool isLoaded() const { return loaded; };
	void setLoaded(const bool value) { loaded = value; };
	bool isBuffered() const { return buffered; };
	void setBuffered(const bool value) { buffered = value; };
	bool getCompilation() const { return tags.meta.track.compilation; };
	void setCompilation(const std::string& artist);
	void setInsertedTime(const util::TTimePart time);
	bool isChanged(const bool reset = false);

	void clear();
	void reset();
	void cleanup();

	std::string text(const char delimiter = ';', const bool encoded = true);
	util::TStringList& asJSON(std::string preamble = "", const bool active = false, const std::string& playlist = "", const bool extended = false);
	util::TStringList& asM3U(const std::string& webroot, const bool isHTTP = true);
	bool assign(const std::string& text, const char delimiter);

	bool filter(const std::string& filter, EFilterType type);
	bool isArtist(const std::string& artist);
	bool isAlbum(const std::string& hash);
	bool isString(const std::string& filter);
	void invalidate();

	static std::string timeToStr(const util::TTimePart seconds);

	void clearStatsistics() { tags.statistics.clear(); };
	void resetStatsistics() { tags.statistics.reset(); };
	void updateStatsistics() { tags.statistics.update(tags.stream.seconds); };
	size_t setWritten(const size_t value) { tags.statistics.written = value; return tags.statistics.written; };
	size_t addWritten(const size_t value) { tags.statistics.written += value; return tags.statistics.written; };
	size_t subWritten(const size_t value) { tags.statistics.written -= value; return tags.statistics.written; };
	size_t getWritten() const { return tags.statistics.written; };
	size_t setRead(const size_t value) { tags.statistics.read = value; return tags.statistics.read; };
	size_t addRead(const size_t value) { tags.statistics.read += value; return tags.statistics.read; };
	size_t subRead(const size_t value) { tags.statistics.read -= value; return tags.statistics.read; };
	size_t getRead() const { return tags.statistics.read; };
	size_t getPercent() const { return tags.statistics.percent; };
	size_t getPromille() const { return tags.statistics.promille; };
	util::TTimePart getDuration() const { return tags.stream.seconds; };
	util::TTimePart getPlayed() const { return tags.statistics.played; };

	TSong& operator = (const TSong& song);

	TSong(const ECodecType type);
	TSong(const std::string& fileName);
	TSong(const std::string& fileName, const ECodecType type);
	virtual ~TSong();
};

} /* namespace music */

#endif /* AUDIOFILE_H_ */
