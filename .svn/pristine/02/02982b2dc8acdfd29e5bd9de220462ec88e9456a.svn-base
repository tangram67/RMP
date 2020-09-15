/*
 * playlist.h
 *
 *  Created on: 29.11.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef LIBRARY_H_
#define LIBRARY_H_

#include <map>
#include <vector>
#include <fnmatch.h>
#include <string>
#include "../inc/gcc.h"
#include "../inc/nullptr.h"
#include "../inc/templates.h"
#include "../inc/fileutils.h"
#include "../inc/audiotypes.h"
#include "../inc/audiofile.h"
#include "../inc/threads.h"
#include "../inc/tables.h"
#include "../inc/hash.h"
#include "musicplayer.h"

namespace music {

class TLibrary;
class TPlaylist;
class TPlaylist;


STATIC_CONST size_t DEFAULT_RECENT_RESULTS = 36;
STATIC_CONST size_t DEFAULT_SEARCH_RESULTS = 8 * DEFAULT_RECENT_RESULTS;

STATIC_CONST size_t LAZY_LOADER_THRESHOLD_LOW = 18;
STATIC_CONST size_t LAZY_LOADER_THRESHOLD_HIGH = LAZY_LOADER_THRESHOLD_LOW / 2 * 3;

STATIC_CONST char STATE_PLAYLIST_NAME[] = "state";
STATIC_CONST char IMAGE_BORDER_RADIUS[] = "0px"; // "5px";

STATIC_CONST char BLANK_PNG_IMAGE_48[]  = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAIAAADYYG7QAAAABGdBTU" \
										  "EAAYagMeiWXwAAAANzQklUCAgI2+FP4AAAAF5JREFUWIXtzrENgEAMBMEDfTfuv5N3P58gIZFsaI" \
										  "KdCubae+dPVpKqmm48uvuePnwZIoaIIWKIGCKGiCFiiBgihoghYogYIoaIIWKIGCKGiCFiiBgihs" \
										  "hK0t3TjdcBzL4IXyIxWMAAAAAASUVORK5CYII=";
STATIC_CONST char BLANK_PNG_IMAGE_200[] = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAMgAAADICAIAAAAiOjnJAAAABGdBTU" \
										  "EAAYagMeiWXwAAAANzQklUCAgI2+FP4AAAAiZJREFUeJzt0kENAzEQwMBtdWzCn8mFT0nUihTNIP" \
										  "DDn/d9B/7tmZm11ukMrrL3/p5u4E7GImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEs" \
										  "YiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGIm" \
										  "EsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLB" \
										  "LGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxi" \
										  "JhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYS" \
										  "wSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEs" \
										  "YiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGIm" \
										  "EsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLB" \
										  "LGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSxiJhLBLGImEsEsYiYSwSz8" \
										  "zsvU9ncJsf+wIJjxwwwn4AAAAASUVORK5CYII=";

#ifdef STL_HAS_TEMPLATE_ALIAS

using PLibrary = TLibrary*;
using TLibraryScannerCallback = std::function<void(const TLibrary& sender, const size_t count, const std::string& current)>;
using TLibraryAddFunction = std::function<void(TLibrary& owner, const std::string&, const bool)>;

using PPlaylist = TPlaylist*;
using TPlaylistList = std::vector<PPlaylist>;
using TPlaylistMap = std::map<std::string, PPlaylist>;

#else

typedef TLibrary* PLibrary;
typedef std::function<void(const TLibrary& sender, const size_t count, const std::string& current)> TLibraryScannerCallback;
typedef std::function<void(TLibrary& owner, const std::string&, const bool)> TLibraryAddFunction;

typedef TPlaylist_1* PPlaylist;
typedef td::vector<PPlaylist> TPlaylistList;
typedef std::map<std::string, PPlaylist> TPlaylistMap;

#endif



typedef struct CLibraryResults {
	size_t artists;
	size_t albums;
} TLibraryResults;


class TLibrary {
public:
	typedef TSongList::const_iterator const_iterator;
	enum EViewType { ELV_UNKNOWN, ELV_ARTIST, ELV_ALBUM };
	enum  ELibrarySortMode { ELS_CASE_SENSITIVE, ELS_CASE_INSENSITIVE, ELS_DEFAULT = ELS_CASE_SENSITIVE };

private:
	bool debug;
	util::TStringList json;
	util::TStringList artistsHTML;
	util::TStringList albumsHTML;
	util::TStringList searchHTML;
	std::string albumsFilter;
	std::string searchFilter;
	EFilterDomain searchDomain;
	EFilterDomain albumsDomain;
	EMediaType searchMedia;
	size_t albumsCount;
	size_t searchCount;
	size_t cdCount;
	size_t hdcdCount;
	size_t dvdCount;
	size_t bdCount;
	size_t hrCount;
	size_t dsdCount;
	std::string artistFilter;
	EMediaType artistMedia;
	EMediaType albumMedia;
	EViewType artistView;
	ELibrarySortMode sortMode;
	music::TLibraryResults artistResult;
	TLibraryScannerCallback onProgressCallback;
	bool sortCaseInsensitive;
	bool allowArtistNameRestore;
	bool allowFullNameSwap;
	bool allowTheBandPrefixSwap;
	bool allowDeepNameInspection;
	bool allowGroupNameSwap;
	bool allowVariousArtistsRename;
	bool allowMovePreamble;
	size_t updatedCount;
	size_t rescanCount;
	std::string database;
	std::string name;
	app::TDetachedThread thread;
	std::mutex threadMtx;
	std::mutex saveMtx;

	void init();

	bool match(struct dirent *file, const std::string& pattern);
	void sort(util::ESortOrder order, TSongSorter asc, TSongSorter desc);
	void sort(TSongSorter sorter);

	std::string getHeadLine(const char delimiter);
	ECodecType codecFromStr(const std::string& value);
	std::string getMediaName(const music::EMediaType type);

	int readDirektory(const std::string& path, const app::TStringVector& patterns, TLibraryAddFunction addfn, const bool rebuild, const bool recursive = true);
	bool hasLibraryMappings();
	void saveLibraryMappings();
	void clearLibraryMappings();
	void updateLibraryMappings();
	void updateVariousArtists();
	void updateTracksCount();
	void updateRecentAlbums();
	void clearArtistMap(TArtistMap& artists);
	void cleanArtistMap(TArtistMap& artists);
	void createLetterMap(TArtistMap& artists, TLetterMap& letters);
	void saveArtistMap(const TArtistMap& artists, const std::string& fileName);
	void saveAlbumMap(const TAlbumMap& albums, const std::string& fileName);
	void saveHashedMap(const THashedMap& albums, const std::string& fileName);
	void addArtistQuickLinks(util::TStringList& html, const TLetterMap& letters, const std::string& base, const char active);
	void addNavigationQuickLinks(util::TStringList& html, const std::string& plink, const std::string& nlink, const std::string& prev, const std::string& next);
	bool filterVariousArtistName(const std::string& name, const util::TStringList& names);
	bool filterArtistName(const std::string& name, char filter);
	bool isValidSpace(const std::string& name, size_t offset);
	std::string escape(const std::string& text);

	void validateSongList();
	void invalidateSongList();
	void setValidateSongList(const bool value);
	size_t deleteInvalidatedSongs();
	size_t deleteRemovedFiles();

	void onScannerCallback(const size_t count, const std::string& current);
	void unlinkThreadMethod(app::TDetachedThread& thread);
	int unlink();

protected:
	TSongs library;
	TSongList garbage;
	std::string errorFile;
	TErrorList errorList;
	util::TTimePart duration;
	int64_t contentSize;
	std::string root;

	void addSong(PSong song, const EPlayListAction action = EPA_DEFAULT);
	PSong newSong(const std::string& fileName);
	PSong newSong(const ECodecType type);
	int updateSong(PSong song);
	void removeSong(PSong song);
	void deleteSong(PSong song);
	void deleteRemovedSongs();
	void reindex();

	std::string getSearchAlbum(const std::string& artistName, const std::string& albumName);

public:
	void clear();
	void destroy();
	void cleanup();

	PSong addFile(const std::string& fileName);
	PSong addFile(const std::string& fileName, const TFileTag tag);
	PSong updateFile(const std::string& fileName, const bool rebuild);
	PSong importFile(const std::string& fileName, const bool rebuild);
	void addError(const std::string& fileName, const int error, const std::string& hint);

	size_t size() const { return library.tracks.songs.size(); }
	bool empty() const { return library.tracks.songs.empty(); }
	bool isChanged() const { return updatedCount > 0; };
	bool hasErrors() const { return errorList.size() > 0; };
	bool validIndex(const size_t index) const;

	operator bool() const { return !empty(); };
	size_t songs() const { return size(); };
	size_t albums() const { return library.albums.size(); };
	size_t artists() const { return library.artists.all.size(); };
	size_t erroneous() const { return errorList.size(); };

	PSong findFile(const std::string& fileHash) const;
	PSong findAlbum(const std::string& albumHash) const;
	PSong findFileName(const std::string& file) const;

	std::string path(const std::string& albumHash) const;
	PSong getSong(const std::size_t index) const;
	PSong getSong(const std::string& fileHash) const;
	PSong operator[] (const std::size_t index) const;
	PSong operator[] (const std::string& fileHash) const;

	const_iterator begin() const { return library.tracks.songs.begin(); };
	const_iterator end() const { return library.tracks.songs.end(); };

	int64_t getContentSize() const { return contentSize; };
	util::TTimePart getDuration() const { return duration; };
	const TSongList& getSongs() const { return library.tracks.songs; }
	const TSongMap& getFiles() const { return library.tracks.files; }
	const THashedMap& getAlbums() const { return library.albums; }

	const TArtistMap& getAllArtists() const { return library.artists.all; }
	const TArtistMap& getCDArtists() const { return library.artists.cd; }
	const TArtistMap& getHDCDArtists() const { return library.artists.hdcd; }
	const TArtistMap& getDVDArtists() const { return library.artists.dvd; }
	const TArtistMap& getBDArtists() const { return library.artists.bd; }
	const TArtistMap& getHRArtists() const { return library.artists.hr; }
	const TArtistMap& getDSDArtists() const { return library.artists.dsd; }

	size_t getCDCount() const { return cdCount; }
	size_t getHDCDCount() const { return hdcdCount; }
	size_t getDVDCount() const { return dvdCount; }
	size_t getBDCount() const { return bdCount; }
	size_t getHRCount() const { return hrCount; }
	size_t getDSDCount() const { return dsdCount; }

	bool getDebug() const { return debug; };
	void setDebug(const bool value) { debug = value; };
	void setFullNameSwap(const bool value) { allowFullNameSwap = value; };
	void setGroupNameSwap(const bool value) { allowGroupNameSwap = value; };
	void setArtistNameRestore(const bool value) { allowArtistNameRestore = value; };
	void setTheBandPrefixSwap(const bool value) { allowTheBandPrefixSwap = value; };
	void setDeepNameInspection(const bool value) { allowDeepNameInspection = value; };
	void setVariousArtistsRename(const bool value) { allowVariousArtistsRename = value; };
	void setMovePreamble(const bool value) { allowMovePreamble = value; };
	void setSortMode(const ELibrarySortMode value);

	void setName(const std::string& value) { name = value; };
	const std::string& getName() const { return name; };
	void setDatabase(const std::string& file) { database = file; };
	const std::string& getDatabase() const { return database; };
	void setRoot(const std::string& path) { root = util::validPath(path); };
	const std::string& getRoot() const { return root; };
	void configure(const TLibraryConfig& config);
	void configure(const CConfigValues& config);

	TLibraryResults getArtistsHTML(util::TStringList& html, const std::string& filter, music::EMediaType type, const music::CConfigValues& config, const EViewType view);
	TLibraryResults updateArtistsHTML(const std::string& filter, bool& changed, music::EMediaType type, const music::CConfigValues& config, const EViewType view);
	const std::string& artistsAsHTML() const { return artistsHTML.html(); };

	size_t getAlbumsHTML(util::TStringList& html, const std::string& filter, const EFilterDomain domain, const music::EMediaType type, const EArtistFilter partial, const music::CConfigValues& config);
	size_t updateAlbumsHTML(const std::string& filter, bool& changed, const EFilterDomain domain, const music::EMediaType type, const EArtistFilter partial, const music::CConfigValues& config);
	const std::string& albumsAsHTML() const { return albumsHTML.html(); };
	bool filterArtistValue(const std::string& filter, const std::string& value, const EArtistFilter partial, const util::TStringList& search);

	size_t getSearchHTML(util::TStringList& html, const std::string& filter, const std::string& genre, const EFilterDomain domain, const EMediaType media, const size_t max, const bool sortByYear, util::TStringList& genres);
	size_t updateSearchHTML(const std::string& filter, const EFilterDomain domain, const EMediaType media, bool& changed, const size_t max, const bool sortByYear);
	bool filterSearchAlbum(const TAlbum& album, const EFilterDomain domain, const EMediaType media, const util::TStringList& search, const std::string& genre);
	bool filterCheckDomain(const EFilterDomain domain, const EFilterDomain value);

	size_t getRecentHTML(util::TStringList& html, const size_t max);

	size_t addSortedAlbums(util::TStringList& html, TConstAlbumList& albums, bool sortByYear);
	void addAlbumHTML(util::TStringList& html, const std::string& artistName, const std::string& albumName, const std::string& displayAlbumName, const std::string& albumHash, const std::string& year, const std::string& icon, const size_t songs, const bool lazy);
	const std::string& searchAsHTML() const { return searchHTML.html(); };
	std::string albumAsJSON(const std::string& hash, const bool extended = false) const;
	std::string albumAsAudioElements(const std::string& hash, const std::string& root) const;
	PSong getAlbumData(const std::string& hash, CTrackData& track, std::string& codec, std::string& format, std::string& samples, std::string& rate, size_t& tracks);
	bool getArtistInfo(const std::string& artist, std::string& displayname, std::string& hash);

	int import(const std::string& path, const app::TStringVector& patterns, const bool recursive = true);
	int rescan(const std::string& path, const app::TStringVector& patterns, const bool rebuild, const bool recursive = true);

	void prepare();
	int update(const std::string& path, const app::TStringVector& patterns, const bool rebuild, const bool recursive = true);
	size_t commit();

	bool hasGarbage() const { return !garbage.empty(); };
	int garbageCollector();

	void sortByTime(util::ESortOrder order = util::SO_ASC);
	void sortByAlbum(util::ESortOrder order = util::SO_ASC);
	void sortByArtist(util::ESortOrder order = util::SO_ASC);
	void sortByAlbumArtist(util::ESortOrder order = util::SO_ASC);

	void saveToFile(const bool addHeader = true, const char delimiter = ';');
	void saveToFile(const std::string& fileName, const bool addHeader = true, const char delimiter = ';');
	void loadFromFile(const std::string& fileName, const bool hasHeader = true, const char delimiter = ';');

	void setErrorFileName(const std::string& fileName);
	std::string getErrorFileName() const { return errorFile; };
	std::string getErrorFileName(const std::string& fileName);

	std::string getErrorFilesAsJSON(const size_t index, const size_t count);
	void saveErrorsToFile(const std::string& fileName);

	template<typename reader_t, typename class_t>
	inline void bindProgressEvent(reader_t &&onScannerCallback, class_t &&owner) {
		onProgressCallback = std::bind(onScannerCallback, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}

	TLibrary();
	virtual ~TLibrary();
};


class TPlaylist {
private:
	bool debug;
	bool changed;
	bool deleted;
	bool state;
	bool permanent;
	std::string name;
	util::hash_type hash;
	std::string database;
	const TLibrary* owner;
	std::mutex threadMtx;
	app::TDetachedThread thread;
	TTrackList garbage;
	TTrackList tracks;
	TTrackMap files;
	mutable util::TStringList json;
	mutable util::TStringList m3u;
	int c_deleted;
	int c_added;

	int unlink();
	void cleanup();
	void unlinkThreadMethod(app::TDetachedThread& thread);
	util::TStringList& asPlainJSON(size_t limit, size_t offset, const std::string& active = "", const bool extended = false) const;
	util::TStringList& asFilteredJSON(size_t limit, size_t offset, const std::string& filter, EFilterType type, const std::string& active = "", const bool extended = false) const;
	int deleteRemovedTracks(const bool rebuild = true);
	int deleteRemovedFiles();
	bool checkRecent() const;

public:
	typedef TTrackList::const_iterator const_iterator;

	void clear();
	void reset();
	void release();
	void rebuild();
	void reindex();
	void prepare();
	void remove();
	size_t clearRandomMarkers();
	size_t songsToShuffleLeft() const;
	size_t songsToShuffleLeft(const std::string& albumHash) const;
	int deleteOldest(const size_t size);

	bool hasGarbage() const { return !garbage.empty(); };
	int garbageCollector();

	bool hasOwner() const { return util::assigned(owner); };
	void setOwner(const TLibrary& owner) { this->owner = &owner; };
	void setOwner(const TLibrary* owner) { this->owner = owner; };
	const std::string& getName() const { return name; };
	util::hash_type getHash() const { return hash; };
	void setName(const std::string& value);
	bool isName(const std::string& value) const;
	bool getDebug() const { return debug; };
	void setDebug(const bool value) { debug = value; };
	bool isDeleted() const { return deleted; };
	void setDeleted(const bool value) { deleted = value; };
	bool isRecent() const { return state; };
	const std::string& getDatabase() const { return database; };
	void setDatabase(const std::string& file) { database = file; };
	void setPermanent(const bool value) { permanent = value; };
	bool isPermanent() const { return permanent; };

	PSong addFile(const std::string& fileName, const EPlayListAction action, bool rebuild);
	PSong addTrack(const std::string& fileHash, const EPlayListAction action, bool rebuild);
	void addSong(PSong song, const EPlayListAction action, bool rebuild);

	bool removeFile(const std::string& fileHash);
	bool deleteFile(const std::string& fileHash);
	bool removeSong(PSong song);
	bool deleteSong(PSong song);
	bool removeTrack(PTrack track);
	bool deleteTrack(PTrack track);
	void commit();

	int removeAlbum(const std::string& albumHash);
	int deleteAlbum(const std::string& albumHash);
	int addAlbum(const std::string& albumHash, const EPlayListAction action, bool rebuild);
	int reorder(const data::TTable& table);

	PTrack findTrack(const std::string& fileHash) const;
	PSong findSong(const std::string& fileHash) const;
	PSong findAlbum(const std::string& albumHash) const;
	bool findAlbumRange(const std::string& albumHash, size_t& first, size_t& last, size_t& size) const;
	int touchAlbum(const std::string& albumHash);
	bool isFirstAlbum(const std::string& albumHash);

	PSong getSong(const std::size_t index) const;
	PSong getSong(const std::string& fileHash) const;
	PTrack getTrack(const std::size_t index) const;
	PTrack getTrack(const std::string& fileHash) const;

	PSong nextSong(const TTrack* track) const;
	PSong nextSong(const TSong* song) const;
	PTrack nextTrack(const TTrack* track) const;
	PTrack nextTrack(const TSong* song) const;

	PTrack operator[] (const std::size_t index) const;
	PTrack operator[] (const std::string& fileHash) const;

	size_t size() const { return tracks.size(); }
	bool empty() const { return tracks.empty(); }
	void invalidate() { changed = true; };
	bool wasChanged() const { return changed; };
	bool validIndex(const size_t index) const;

	const_iterator begin() const { return tracks.begin(); };
	const_iterator end() const { return tracks.end(); };

	util::TStringList& asJSON(size_t limit = 0, size_t offset = 0, const std::string& filter = "",
			EFilterType type = FT_DEFAULT, const std::string& active = "", const bool extended = false) const;
	util::TStringList& asM3U(const std::string& webroot, size_t limit = 0, size_t offset = 0) const;

	void saveToFile();
	void saveToFile(const std::string& fileName);
	void loadFromFile();
	void loadFromFile(const std::string& fileName);
	int importFromMPD(const std::string& fileName, const std::string& root);

	TPlaylist();
	virtual ~TPlaylist();
};


class TPlaylists {
private:
	TPlaylistMap map;
	TPlaylistList garbage;
	std::string ext;
	std::string state;
	std::string root;
	std::string nolist;
	PPlaylist m_selected;
	PPlaylist m_playing;
	PPlaylist m_recent;
	const TLibrary* owner;

	void prime();
	void init();
	void destroy();

public:
	using const_iterator = TPlaylistMap::const_iterator;

	void clear();
	void rebuild();
	void reload();
	void resize(const size_t size);
	size_t commit(const bool force = false);

	bool changed() const;
	size_t size() const { return map.size(); };
	bool empty() const { return map.empty(); };
	const_iterator begin() const { return map.begin(); };
	const_iterator end() const { return map.end(); };

	void getNames(app::TStringVector& items);
	const std::string& getSelected() const;
	const std::string& getPlaying() const;
	size_t getSelectedSize() const;
	size_t getPlayingSize() const;

	bool hasGarbage() const;
	bool hasDeleted() const { return !garbage.empty(); };
	int garbageCollector();

	void setOwner(const TLibrary& owner) { this->owner = &owner; };
	void setOwner(const TLibrary* owner) { this->owner = owner; };
	const std::string& getExtension() const { return ext; };
	void setExtension(const std::string& name) { ext = name; };

	PPlaylist add(const std::string name);
	void remove(const std::string name);
	bool rename(const std::string oldName, const std::string newName);
	void loadFromFolder(const std::string& folder);

	int cleanup();
	void reset();
	void release();
	void undefer();

	PPlaylist find(const std::string name) const;
	PPlaylist selected() { return m_selected; };
	PPlaylist playing() { return m_playing; };
	PPlaylist recent() { return m_recent; };
	bool play(const std::string name);
	bool select(const std::string name);
	bool isValid(const std::string name) const;
	bool isSelected(const std::string name) const;
	bool isPlaying(const std::string name) const;
	bool isRecent(const std::string name) const;

	PPlaylist operator[] (const std::string name) const { return find(name); };

	TPlaylists();
	virtual ~TPlaylists();
};


} /* namespace music */


#endif /* LIBRARY_H_ */
