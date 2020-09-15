/*
 * playlist.cpp
 *
 *  Created on: 29.11.2015
 *      Author: Dirk Brinkmeier
 */

#include "../inc/audiotypes.h"
#include "../inc/htmlutils.h"
#include "../inc/mimetypes.h"
#include "../inc/datetime.h"
#include "../inc/encoding.h"
#include "../inc/convert.h"
#include "../inc/compare.h"
#include "../inc/json.h"
#include "../inc/flac.h"
#include "../inc/pcm.h"
#include "../inc/dsd.h"
#include "../inc/aiff.h"
#include "../inc/alac.h"
#include "../inc/mp3.h"
#include "../config.h"
#include "library.h"

namespace music {

#define LIBRARY_ROOT_URL "library/"

#define GOOGLE_SEARCH_URL "https://www.googleapis.com/customsearch/v1?q=____&cx=013458064193606455778:1euzugtetd3&key=AIzaSyB_lt-1v8Kf8RPz7W-EvxMzugt5ykOEJRM&fields=items(pagemap.cse_image/src)&enableImageSearch=true&image_size=huge&image_sort_by=%22%22"

#define WIKIPEDIA_SEARCH_URL "https://en.wikipedia.org/wiki/Special:Search?search="
#define ALLMUSIC_ARTIST_URL "http://www.allmusic.com/search/artist/"
#define ALLMUSIC_ALBUM_URL "http://www.allmusic.com/search/album/"
#define ALLMUSIC_SONG_URL "http://www.allmusic.com/search/song/"

#define ARTIST_SEARCH_URL WIKIPEDIA_SEARCH_URL
#define ALBUM_SEARCH_URL WIKIPEDIA_SEARCH_URL
#define SONG_SEARCH_URL ALLMUSIC_SONG_URL

TLibrary::TLibrary() {
	onProgressCallback = nil;
	init();
}

TLibrary::~TLibrary() {
	clear();
}

void TLibrary::init() {
	debug = false;
	cdCount = 0;
	hdcdCount = 0;
	dvdCount = 0;
	bdCount = 0;
	hrCount = 0;
	dsdCount = 0;
	albumsCount = 0;
	searchCount = 0;
	updatedCount = 0;
	contentSize = 0;
	duration = 0;
	artistResult.albums = 0;
	artistResult.artists = 0;
	artistView = ELV_UNKNOWN;
	artistMedia = EMT_UNKNOWN;
	albumMedia = EMT_UNKNOWN;
	searchMedia = EMT_UNKNOWN;
	searchDomain = FD_UNKNOWN;
	albumsDomain = FD_UNKNOWN;
	allowFullNameSwap = false;
	allowGroupNameSwap = true;
	allowArtistNameRestore = true;
	allowTheBandPrefixSwap = false;
	allowDeepNameInspection = false;
	allowVariousArtistsRename = false;
	allowMovePreamble = false;
	setSortMode(ELS_DEFAULT);
	thread.setName("app::TLibrary::unlink()");
	thread.setExecHandler(&music::TLibrary::unlinkThreadMethod, this);
}

void TLibrary::setSortMode(const ELibrarySortMode value) {
	sortMode = value;
	sortCaseInsensitive = sortMode == ELS_CASE_INSENSITIVE;
};


void TLibrary::configure(const TLibraryConfig& config) {
	setDebug(config.debug);
	setRoot(config.documentRoot);
	setFullNameSwap(config.allowFullNameSwap);
	setGroupNameSwap(config.allowGroupNameSwap);
	setArtistNameRestore(config.allowArtistNameRestore);
	setTheBandPrefixSwap(config.allowTheBandPrefixSwap);
	setDeepNameInspection(config.allowDeepNameInspection);
	setVariousArtistsRename(config.allowVariousArtistsRename);
	setMovePreamble(config.allowMovePreamble);
	setSortMode(config.sortCaseSensitive ? music::TLibrary::ELS_CASE_SENSITIVE : music::TLibrary::ELS_CASE_INSENSITIVE);
}

void TLibrary::configure(const CConfigValues& config) {
	setFullNameSwap(config.allowFullNameSwap);
	setGroupNameSwap(config.allowGroupNameSwap);
	setArtistNameRestore(config.allowArtistNameRestore);
	setTheBandPrefixSwap(config.allowTheBandPrefixSwap);
	setDeepNameInspection(config.allowDeepNameInspection);
	setVariousArtistsRename(config.allowVariousArtistsRename);
	setMovePreamble(config.allowMovePreamble);
	setSortMode(config.sortCaseSensitive ? music::TLibrary::ELS_CASE_SENSITIVE : music::TLibrary::ELS_CASE_INSENSITIVE);
}

void importSong(TLibrary& owner, const std::string& fileName, const bool rebuild) {
	owner.importFile(fileName, rebuild);
}

int TLibrary::import(const std::string& path, const app::TStringVector& patterns, const bool recursive) {
	clear();
	readDirektory(path, patterns, importSong, true, recursive);
	updateLibraryMappings();
	return (int)library.tracks.songs.size();
}

void checkSong(TLibrary& owner, const std::string& fileName, const bool rebuild) {
	owner.updateFile(fileName, rebuild);
}

int TLibrary::rescan(const std::string& path, const app::TStringVector& patterns, const bool rebuild, const bool recursive) {
	prepare();
	readDirektory(path, patterns, checkSong, rebuild, recursive);
	commit();
	return (int)library.tracks.songs.size();
}

void TLibrary::prepare() {
	updatedCount = 0;
	rescanCount = 0;
	errorList.clear();
	invalidateSongList();
}

int TLibrary::update(const std::string& path, const app::TStringVector& patterns, const bool rebuild, const bool recursive) {
	readDirektory(path, patterns, checkSong, rebuild, recursive);
	return (int)library.tracks.songs.size();
}

size_t TLibrary::commit() {
	size_t r = deleteInvalidatedSongs();
	updateLibraryMappings();
	return r;
}

int TLibrary::garbageCollector() {
	int collected = garbage.size();
	if (collected > 0)
		util::clearObjectList(garbage);
	return collected;
}

int TLibrary::readDirektory(const std::string& path, const app::TStringVector& patterns, TLibraryAddFunction addfn, const bool rebuild, const bool recursive) {
	util::TDirectory dir;
	struct dirent *file;
	std::string pattern;
	util::TStringList folders;
	int r = 0;

	if (util::fileExists(path + "noscan"))
		return 0;

	if(patterns.empty())
		return 0;

	if (!recursive)
		clear();

	dir.open(path);
	if (dir.isOpen()) {
		while(util::assigned((file = readdir(dir())))) {
			if(file->d_name[0] == '.')
				continue;

			// Add file to list if pattern matches filename
			if (file->d_type == DT_REG) {
				app::TStringVector::const_iterator it = patterns.begin();
				while (it != patterns.end()) {
					pattern = *it;
					if (!pattern.empty()) {
						if (match(file, pattern)) {
							addfn(*this, path + file->d_name, rebuild);
							++r;
						}
					}
					++it;
				}
			}

			// Prepare recursive folder scan
			if (recursive && file->d_type == DT_DIR) {
				folders.add(util::validPath(path + file->d_name));
			}
		}
		dir.close();
	}

	// Scan sub folders
	if (!folders.empty()) {
		folders.sort(util::SO_ASC);
		for (size_t i=0; i<folders.size(); ++i) {
			r += readDirektory(folders[i], patterns, addfn, rebuild, recursive);
		}
	}

	return r;
}

bool TLibrary::match(struct dirent *file, const std::string& pattern) {
	if (!pattern.empty())
		return (0 == ::fnmatch(pattern.c_str(), file->d_name, FNM_NOESCAPE | FNM_CASEFOLD));
	return false;
}

PSong TLibrary::newSong(const std::string& fileName) {
	ECodecType type = TSong::getFileType(fileName);
	return newSong(type);
}

PSong TLibrary::newSong(const ECodecType type) {
	PSong o = nil;
	switch (type) {
		case EFT_FLAC:
			o = new TFLACFile();
			break;
		case EFT_WAV:
			o = new TPCMFile();
			break;
		case EFT_DSF:
			o = new TDSFFile();
			break;
		case EFT_DFF:
			o = new TDFFFile();
			break;
		case EFT_AIFF:
			o = new TAIFFFile();
			break;
		case EFT_ALAC:
			o = new TALACFile();
			break;
#ifdef SUPPORT_AAC_FILES
		case EFT_AAC:
			o = new TALACFile(EFT_AAC);
			break;
#endif
		case EFT_MP3:
			o = new TMP3File();
			break;
		default:
			break;
	}
	return o;
}

PSong TLibrary::addFile(const std::string& fileName) {
	PSong o = newSong(fileName);

	// Add file to list if tags are valid
	if (util::assigned(o)) {
		o->setFileProperties(fileName);
		PSong p = o;
		util::TObjectGuard<TSong> og(&p);
		int r = updateSong(o);
		if (EXIT_SUCCESS == r) {
			addSong(o);
			p = nil; // Do NOT (!) delete p via RAII guard!
		} else {
			// Return nil again!
			std::cout << "TLibrary::addSong() Invalid song <" << fileName << "> (" << r << ")" << std::endl;
			addError(fileName, r, o->getError());
			o = nil;
		}
	} else {
		std::cout << "TPlaylist::addSong() Unknown song type <" << fileName << ">" << std::endl;
		addError(fileName, -999, util::fileExtName(fileName));
	}

	return o;
}

PSong TLibrary::addFile(const std::string& fileName, const TFileTag tag) {
	PSong o = newSong(fileName);

	// Add file to list if tags are valid
	if (util::assigned(o)) {
		//o->setFileProperties(fileName);
		o->setFileProperties(tag);
		PSong p = o;
		util::TObjectGuard<TSong> og(&p);
		int r = updateSong(o);
		if (EXIT_SUCCESS == r) {
			addSong(o);
			p = nil; // Do NOT (!) delete p via RAII guard!
		} else {
			// Return nil again!
			std::cout << "TLibrary::addSong() Invalid song <" << fileName << "> (" << r << ")" << std::endl;
			addError(fileName, r, o->getError());
			o = nil;
		}
	} else {
		std::cout << "TPlaylist::addSong() Unknown song type <" << fileName << ">" << std::endl;
		addError(fileName, -999, util::fileExtName(fileName));
	}

	return o;
}

void TLibrary::addError(const std::string& fileName, const int error, const std::string& hint) {
	std::string text;
	switch (error) {
		case -1 :
			text = "Invalid metadata tags or corrupt media stream information, please check ID3 tags and stream information";
			break;
		case -2 :
			text = "Unsupported file type, sample rate or channel count, e.g. one channel only or surround sound content";
			break;
		case -10 :
			text = "Unrecoverable error in metadata parser";
			break;
		case -999 :
			text = "Invalid or unsupported song type, e.g. check valid extension for media content";
			break;
		default:
			text = "Unexpected error for invalid, unsupported or corrupted file";
			break;
	}
	music::TErrorSong song;
	song.file = fileName;
	song.error = error;
	song.text = text;
	song.hint = hint;
	song.url = util::TURL::encode(util::filePath(fileName));
//	music::TFileTag::sanitize(song.file);
//	music::TFileTag::sanitize(song.text);
//	music::TFileTag::sanitize(song.hint);
	errorList.push_back(song);
}

int TLibrary::updateSong(PSong song) {
	if (util::assigned(song)) {
		duration -= song->getDuration();
		contentSize -= song->getFileSize();
		song->tags.meta.clear();
		song->tags.sort.clear();
		song->tags.stream.clear();
		song->tags.cover.clear();
		int r = song->readMetadataTags(allowArtistNameRestore, allowFullNameSwap, allowGroupNameSwap,
							allowTheBandPrefixSwap, allowVariousArtistsRename, allowMovePreamble);
		if (r == EXIT_SUCCESS) {
			song->updateProperties();
		}
		return r;
	}
	return -100;
}

void TLibrary::addSong(PSong song, const EPlayListAction action) {
	if (util::assigned(song)) {
		song->updateProperties();
		duration += song->getDuration();
		contentSize += song->getFileSize();
		if (song->getScannerParams() < 0) {
			song->setScannerParams(allowArtistNameRestore, allowFullNameSwap, allowGroupNameSwap,
						allowTheBandPrefixSwap, allowVariousArtistsRename, allowMovePreamble);
		}
		switch (action) {
			case EPA_INSERT:
				library.tracks.songs.insert(library.tracks.songs.begin(), song);
				reindex();
				break;
			case EPA_APPEND:
			default:
				song->setIndex(library.tracks.songs.size());
				library.tracks.songs.push_back(song);
				break;
		}
		song->setIterator(library.tracks.files.insert(TSongItem(song->getFileHash(), song)).first);
	}
}

void TLibrary::removeSong(PSong song) {
	duration -= song->getDuration();
	contentSize -= song->getFileSize();
	song->setDeleted(true);
}

void TLibrary::deleteSong(PSong song) {
	removeSong(song);
	deleteRemovedSongs();
}

struct CSongDeleter
{
	CSongDeleter(bool isOwner, TSongList* deleted) : _owner(isOwner), _deleted(deleted) {}
	bool _owner;
	TSongList* _deleted;
	bool operator() (PSong song) const {
		bool r = false;
		if (util::assigned(song)) {
			if (song->isDeleted())
				r = true;
			if (r && _owner) {
				if (util::assigned(_deleted)) {
					_deleted->push_back(song);
				} else {
					util::freeAndNil(song);
				}
			}
		}
		return r;
	}
};

void TLibrary::deleteRemovedSongs() {
	if (deleteRemovedFiles() > 0) {
		library.tracks.songs.erase(std::remove_if(library.tracks.songs.begin(), library.tracks.songs.end(), CSongDeleter(true, &garbage)), library.tracks.songs.end());
		reindex();
		if (hasLibraryMappings())
			updateLibraryMappings();
	}
}

size_t TLibrary::deleteRemovedFiles() {
	size_t size = library.tracks.files.size();
	if (size > 0) {
		PSong o;
		TSongMap::iterator it = library.tracks.files.begin();
		while (it != library.tracks.files.end()) {
			o = it->second;
			if (util::assigned(o)) {
				if (o->isDeleted()) {
					it = library.tracks.files.erase(it);
					continue;
				}
			}
			++it;
		}
	}
	return size - library.tracks.files.size();
}

void TLibrary::reindex() {
	PSong o;
	for (size_t i=0; i<library.tracks.songs.size(); ++i) {
		o = library.tracks.songs[i];
		if (util::assigned(o)) {
			o->setIndex(i);
			o->setDeleted(false);
		}
	}
}


PSong TLibrary::updateFile(const std::string& fileName, const bool rebuild) {
	TFileTag tag;
	TAudioFile file(fileName, tag);
	PSong p = findFile(tag.file.hash);
	if (util::assigned(p)) {
		// Song is in list
		p->setLoaded(true);

		// Check if timestamp or size has been changed
		if (tag.file.time != p->getFileTime() ||
			tag.file.size != p->getFileSize()) {

			// Update song in list
			p->setLoaded(false);
			if (debug) std::cout << "TLibrary::updateFile() Song \"" << p->getFileName() << "\" was changed." << std::endl;

			// Update existing song object
			int r = updateSong(p);
			if (EXIT_SUCCESS == r) {
				p->tags.file = tag.file;
				p->setLoaded(true);
				p->setInsertedTime(util::now());
				++updatedCount;
				if (debug) std::cout << "TLibrary::updateFile() Song \"" << p->getFileName() << "\" is updated." << std::endl;
			} else {
				if (debug) std::cout << "TLibrary::updateFile() Song \"" << p->getFileName() << "\" failed (" << r << ")" << std::endl;
			}
		}

	} else {
		// File not yet in list
		// --> Add new song to list!
		p = addFile(fileName, tag);
		if (util::assigned(p)) {
			p->setLoaded(true);
			p->setInsertedTime(rebuild ? p->getFileTime() : util::now());
			++updatedCount;
			if (debug) std::cout << "TLibrary::updateFile() Song \"" << p->getFileName() << "\" added (" << library.tracks.songs.size() << ")" << std::endl;
		}

	}

	// Event callback for (re)scanned files
	++rescanCount;
	if (0 == rescanCount % 333) {
		onScannerCallback(rescanCount, p->getFileName());
	}

	return p;
}

PSong TLibrary::importFile(const std::string& fileName, const bool rebuild) {
	PSong p = addFile(fileName);
	if (util::assigned(p)) {
		p->setLoaded(true);
		p->setInsertedTime(p->getFileTime());
	}
	return p;
}

void TLibrary::clear() {
	cdCount = 0;
	hdcdCount = 0;
	dvdCount = 0;
	bdCount = 0;
	hrCount = 0;
	dsdCount = 0;
	albumsCount = 0;
	artistResult.albums = 0;
	artistResult.artists = 0;
	name.clear();
	json.clear();
	artistsHTML.clear();
	albumsHTML.clear();
	searchHTML.clear();
	albumsFilter.clear();
	searchFilter.clear();
	errorList.clear();
	clearLibraryMappings();
	library.tracks.files.clear();
	util::clearObjectList(library.tracks.songs);
	util::clearObjectList(library.recent);
	garbageCollector();
	updatedCount = 0;
	rescanCount = 0;
	contentSize = 0;
	duration = 0;
}

void TLibrary::destroy() {
	size_t r = library.tracks.songs.size();
	clear();
	updatedCount = r;
}

bool TLibrary::validIndex(const size_t index) const {
	return (index >= 0 && index < library.tracks.songs.size());
}

PSong TLibrary::findFile(const std::string& fileHash) const {
	if (!fileHash.empty()) {
		if (!library.tracks.files.empty()) {
			TSongMap::const_iterator it = library.tracks.files.find(fileHash);
			if (it != library.tracks.files.end())
				return it->second;
		} else {
			if (!library.tracks.songs.empty()) {
				PSong o;
				for (size_t i=0; i<library.tracks.songs.size(); ++i) {
					o = library.tracks.songs[i];
					if (util::assigned(o)) {
						if (o->getFileHash() == fileHash)
							return o;
					}
				}
			}
		}
	}
	return nil;
}

PSong TLibrary::findAlbum(const std::string& albumHash) const {
	if (!albumHash.empty()) {
		if (!library.albums.empty()) {
			THashedMap::const_iterator it = library.albums.find(albumHash);
			if (it != library.albums.end()) {
				if (!it->second.songs.empty()) {
					return it->second.songs[0];
				}
			}
		} else {
			if (!library.tracks.songs.empty()) {
				PSong o;
				for (size_t i=0; i<library.tracks.songs.size(); ++i) {
					o = library.tracks.songs[i];
					if (util::assigned(o)) {
						if (o->getAlbumHash() == albumHash)
							return o;
					}
				}
			}
		}
	}
	return nil;
}

std::string TLibrary::path(const std::string& albumHash) const {
	if (!library.albums.empty()) {
		THashedMap::const_iterator it = library.albums.find(albumHash);
		if (it != library.albums.end()) {
			if (!it->second.songs.empty()) {
				PSong o = it->second.songs[0];
				if (util::assigned(o)) {
					return o->getFolder();
				}
			}
		}
	}
	return "";
}

PSong TLibrary::getSong(const std::size_t index) const {
	if (validIndex(index))
		return library.tracks.songs.at(index);
	return nil;
};

PSong TLibrary::getSong(const std::string& fileHash) const {
	return findFile(fileHash);
}

PSong TLibrary::operator[] (const std::size_t index) const {
	return getSong(index);
};

PSong TLibrary::operator[] (const std::string& fileHash) const {
	return getSong(fileHash);
};

PSong TLibrary::findFileName(const std::string& fileName) const {
	PSong o;
	for (size_t i=0; i<library.tracks.songs.size(); ++i) {
		o = library.tracks.songs[i];
		if (0 == util::strncasecmp(o->getFileName(), fileName, fileName.size())) {
			return o;
		}
	}
	return nil;
}


bool artistListSorterDesc(const TArtist* o, const TArtist* p) {
	if (util::assigned(o) && util::assigned(p)) {
		return util::strnatsort(o->name, p->name);
	}
	return false;
}

bool artistListCaseSorterDesc(const TArtist* o, const TArtist* p) {
	if (util::assigned(o) && util::assigned(p)) {
		return util::strnatcasesort(o->name, p->name);
	}
	return false;
}

bool artistListSorterAsc(const TArtist* o, const TArtist* p) {
	if (util::assigned(o) && util::assigned(p)) {
		return util::strnatsort(p->name, o->name);
	}
	return false;
}

bool artistListCaseSorterAsc(const TArtist* o, const TArtist* p) {
	if (util::assigned(o) && util::assigned(p)) {
		return util::strnatcasesort(p->name, o->name);
	}
	return false;
}


bool yearSorterDesc(const TAlbum* o, const TAlbum* p) {
	if (util::assigned(o) && util::assigned(p)) {
		if (!(o->songs.empty() || p->songs.empty())) {
			if (util::assigned(o->songs[0]) && util::assigned(p->songs[0])) {
				if (o->songs[0]->getYearSort() == p->songs[0]->getYearSort())
					return o->songs[0]->getAlbumSort() < p->songs[0]->getAlbumSort();
				return o->songs[0]->getYearSort() < p->songs[0]->getYearSort();
			}
		}
	}
	return false;
}

bool dateSorterDesc(const TAlbum* o, const TAlbum* p) {
	if (util::assigned(o) && util::assigned(p)) {
		if (o->inserted == p->inserted)
			return o->date > p->date;
		else
			return o->inserted > p->inserted;
	}
	return false;
}


bool trackSorterAsc(const TSong* o, const TSong* p) {
	if (o->getDiskNumber() == p->getDiskNumber())
		return o->getTrackNumber() < p->getTrackNumber();
	else
		return o->getDiskNumber() < p->getDiskNumber();
}

bool trackSorterDesc(const TSong* o, const TSong* p) {
	if (o->getDiskNumber() == p->getDiskNumber())
		return o->getTrackNumber() > p->getTrackNumber();
	else
		return o->getDiskNumber() > p->getDiskNumber();
}


/* Time sorting algorithms */
bool timeSorterAsc(const TSong* o, const TSong* p) {
	if (o->getModTime() == p->getModTime())
		return o->getIndex() > p->getIndex();
	return o->getModTime() < p->getModTime();
}

bool timeSorterDesc(const TSong* o, const TSong* p) {
	if (o->getModTime() == p->getModTime())
		return o->getIndex() < p->getIndex();
	return o->getModTime() > p->getModTime();
}


/* Case sensitive sorter algorithms */
bool albumNatSorterAsc(const TSong* o, const TSong* p) {
	if (o->getAlbumSortHash() == p->getAlbumSortHash()) {
		return trackSorterAsc(o, p);
	} else {
		return util::strnatsort(o->getAlbumSort(), p->getAlbumSort());
	}
}

bool albumNatSorterDesc(const TSong* o, const TSong* p) {
	if (o->getAlbumSortHash() == p->getAlbumSortHash()) {
		return trackSorterDesc(o, p);
	} else {
		return util::strnatsort(p->getAlbumSort(), o->getAlbumSort());
	}
}


bool artistNatSorterAsc(const TSong* o, const TSong* p) {
	if (o->getArtistSortHash() == p->getArtistSortHash()) {
		return albumNatSorterAsc(o, p);
	} else {
		return util::strnatsort(o->getArtistSort(), p->getArtistSort());
	}
}

bool artistNatSorterDesc(const TSong* o, const TSong* p) {
	if (o->getArtistSortHash() == p->getArtistSortHash()) {
		return albumNatSorterDesc(o, p);
	} else {
		return util::strnatsort(p->getArtistSort(), o->getArtistSort());
	}
}


bool albumArtistNatSorterAsc(const TSong* o, const TSong* p) {
	if (o->getAlbumArtistSortHash() == p->getAlbumArtistSortHash()) {
		return albumNatSorterAsc(o, p);
	} else {
		return util::strnatsort(o->getAlbumArtistsSort(), p->getAlbumArtistsSort());
	}
}

bool albumArtistNatSorterDesc(const TSong* o, const TSong* p) {
	if (o->getAlbumArtistSortHash() == p->getAlbumArtistSortHash()) {
		return albumNatSorterDesc(o, p);
	} else {
		return util::strnatsort(p->getAlbumArtistsSort(), o->getAlbumArtistsSort());
	}
}


/* Case insensitive sorter algorithms */
bool albumNatCaseSorterAsc(const TSong* o, const TSong* p) {
	if (o->getAlbumSortHash() == p->getAlbumSortHash()) {
		return trackSorterAsc(o, p);
	} else {
		return util::strnatcasesort(o->getAlbumSort(), p->getAlbumSort());
	}
}

bool albumNatCaseSorterDesc(const TSong* o, const TSong* p) {
	if (o->getAlbumSortHash() == p->getAlbumSortHash()) {
		return trackSorterDesc(o, p);
	} else {
		return util::strnatcasesort(p->getAlbumSort(), o->getAlbumSort());
	}
}


bool artistNatCaseSorterAsc(const TSong* o, const TSong* p) {
	if (o->getArtistSortHash() == p->getArtistSortHash()) {
		return albumNatCaseSorterAsc(o, p);
	} else {
		return util::strnatcasesort(o->getArtistSort(), p->getArtistSort());
	}
}

bool artistNatCaseSorterDesc(const TSong* o, const TSong* p) {
	if (o->getArtistSortHash() == p->getArtistSortHash()) {
		return albumNatCaseSorterDesc(o, p);
	} else {
		return util::strnatcasesort(p->getArtistSort(), o->getArtistSort());
	}
}


bool albumArtistNatCaseSorterAsc(const TSong* o, const TSong* p) {
	if (o->getAlbumArtistSortHash() == p->getAlbumArtistSortHash()) {
		return albumNatCaseSorterAsc(o, p);
	} else {
		return util::strnatcasesort(o->getAlbumArtistsSort(), p->getAlbumArtistsSort());
	}
}

bool albumArtistNatCaseSorterDesc(const TSong* o, const TSong* p) {
	if (o->getAlbumArtistSortHash() == p->getAlbumArtistSortHash()) {
		return albumNatSorterDesc(o, p);
	} else {
		return util::strnatcasesort(p->getAlbumArtistsSort(), o->getAlbumArtistsSort());
	}
}


bool albumSorterAsc(const TSong* o, const TSong* p) {
	if (o->getAlbumSortHash() == p->getAlbumSortHash())
		return trackSorterAsc(o, p);
	else
		return o->getAlbumSort() > p->getAlbumSort();
}

bool albumSorterDesc(const TSong* o, const TSong* p) {
	if (o->getAlbumSortHash() == p->getAlbumSortHash())
		return trackSorterDesc(o, p);
	else
		return p->getAlbumSort() < o->getAlbumSort();
}

bool artistSorterAsc(const TSong* o, const TSong* p) {
	if (o->getArtistSortHash() == p->getArtistSortHash())
		return albumSorterAsc(o, p);
	else
		return o->getArtistSort() > p->getArtistSort();
}

bool artistSorterDesc(PSong o, PSong p) {
	if (o->getArtistSortHash() == p->getArtistSortHash())
		return albumSorterDesc(o, p);
	else
		return p->getArtistSort() < o->getArtistSort();
}

bool albumArtistSorterAsc(const TSong* o, const TSong* p) {
	if (o->getAlbumArtistSortHash() == p->getAlbumArtistSortHash())
		return albumSorterAsc(o, p);
	else
		return o->getAlbumArtistsSort() > p->getAlbumArtistsSort();
}

bool albumArtistSorterDesc(const TSong* o, const TSong* p) {
	if (o->getAlbumArtistSortHash() == p->getAlbumArtistSortHash())
		return albumSorterDesc(o, p);
	else
		return p->getAlbumArtistsSort() < o->getAlbumArtistsSort();
}



void TLibrary::sort(util::ESortOrder order, TSongSorter asc, TSongSorter desc) {
	TSongSorter sorter;
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

void TLibrary::sort(TSongSorter sorter) {
	std::sort(library.tracks.songs.begin(), library.tracks.songs.end(), sorter);
	reindex();
}


void TLibrary::sortByTime(util::ESortOrder order) {
	sort(order, timeSorterAsc, timeSorterDesc);
}

void TLibrary::sortByAlbum(util::ESortOrder order) {
	if (sortCaseInsensitive) {
		sort(order, albumNatCaseSorterAsc, albumNatCaseSorterDesc);
	} else {
		sort(order, albumNatSorterAsc, albumNatSorterDesc);
	}
}

void TLibrary::sortByArtist(util::ESortOrder order) {
	if (sortCaseInsensitive) {
		sort(order, artistNatCaseSorterAsc, artistNatCaseSorterDesc);
	} else {
		sort(order, artistNatSorterAsc, artistNatSorterDesc);
	}
}

void TLibrary::sortByAlbumArtist(util::ESortOrder order) {
	if (sortCaseInsensitive) {
		sort(order, albumArtistNatCaseSorterAsc, albumArtistNatCaseSorterDesc);
	} else {
		sort(order, albumArtistNatSorterAsc, albumArtistNatSorterDesc);
	}
}


void TLibrary::validateSongList() {
	setValidateSongList(true);
}

void TLibrary::invalidateSongList() {
	setValidateSongList(false);
}

void TLibrary::setValidateSongList(const bool value) {
	PSong o;
	for (size_t i=0; i<library.tracks.songs.size(); ++i) {
		o = library.tracks.songs[i];
		if (util::assigned(o)) {
			o->setLoaded(value);
		}
	}
}

struct CSongEraser {
	CSongEraser(bool isOwner) : _owner(isOwner) {}
    bool _owner;
    bool operator()(PSong o) const {
    	bool r = false;
    	if (util::assigned(o)) {
			r = !o->isLoaded();
			if (r) {
				std::cout << "TLibrary::deleteInvalidatedSongs() Song \"" << o->getFileName() << "\" deleted." << std::endl;
				if (_owner)
					util::freeAndNil(o);
			}
    	}
    	return r;
    }
};

size_t TLibrary::deleteInvalidatedSongs() {
	// Delete songs in file map
	PSong o;
	TSongMap::iterator it = library.tracks.files.begin();
	while (it != library.tracks.files.end()) {
		o = it->second;
    	if (util::assigned(o)) {
			if (!o->isLoaded()) {
				it = library.tracks.files.erase(it);
				continue;
			}
    	}
		it++;
	}
	// Delete songs in song list
	size_t size = library.tracks.songs.size();
	library.tracks.songs.erase(std::remove_if(library.tracks.songs.begin(), library.tracks.songs.end(), CSongEraser(true)), library.tracks.songs.end());
	return size - library.tracks.songs.size();
}

bool TLibrary::hasLibraryMappings() {
	return library.artists.all.size() > 0 ||
			library.albums.size() > 0 ||
			library.ordered.size() > 0;
}

void TLibrary::clearLibraryMappings() {
	clearArtistMap(library.artists.all);
	clearArtistMap(library.artists.cd);
	clearArtistMap(library.artists.hdcd);
	clearArtistMap(library.artists.dsd);
	clearArtistMap(library.artists.dvd);
	clearArtistMap(library.artists.bd);
	clearArtistMap(library.artists.hr);

	library.letters.all.clear();
	library.letters.cd.clear();
	library.letters.hdcd.clear();
	library.letters.dsd.clear();
	library.letters.dvd.clear();
	library.letters.bd.clear();
	library.letters.hr.clear();

	library.albums.clear();
	library.ordered.clear();
}

void TLibrary::saveLibraryMappings() {
	saveArtistMap(library.artists.all,  "m_all_artists.map");
	saveArtistMap(library.artists.cd,   "m_cd_artists.map");
	saveArtistMap(library.artists.hdcd, "m_hdcd_artists.map");
	saveArtistMap(library.artists.dsd,  "m_dsd_artists.map");
	saveArtistMap(library.artists.dvd,  "m_dvd_artists.map");
	saveArtistMap(library.artists.bd,   "m_bd_artists.map");
	saveArtistMap(library.artists.hr,   "m_hr_artists.map");
	saveHashedMap(library.albums, "m_hashed_albums.map");
	saveAlbumMap(library.ordered, "m_ordered_albums.map");
}

void TLibrary::updateLibraryMappings() {

	// Update various artists tags first
	updateVariousArtists();

	// Prepare library for update
	clearLibraryMappings();
	sortByAlbumArtist();

	// Reset statistic values
	contentSize = 0;
	duration = 0;
	cdCount = 0;
	hdcdCount = 0;
	dvdCount = 0;
	bdCount = 0;
	hrCount = 0;
	dsdCount = 0;

	// Iterate through all songs
	std::string c_album, c_sort;
	music::EMediaType media = EMT_UNKNOWN;
	util::TTimePart inserted = util::epoch();
	util::TTimePart date = util::epoch();
	bool compilation = false;
	PSong o;
	for (size_t i=0; i<library.tracks.songs.size(); ++i) {
		o = library.tracks.songs[i];
		if (util::assigned(o)) {
			duration += o->getDuration();
			contentSize += o->getFileSize();
			const std::string& artist = o->getAlbumArtist();
			const std::string& displayartist = o->getDisplayAlbumArtist();

			const std::string& originalartist = o->getOriginalAlbumArtist();
			const std::string& displayoriginalartist = o->getDisplayOriginalAlbumArtist();

			const std::string& album = o->getAlbum();
			const std::string& displayalbum = o->getDisplayAlbum();

			const std::string& genre = o->getGenre();
			const std::string& displaygenre = o->getDisplayGenre();

			const std::string& sortartist = o->getAlbumArtist();
			const std::string& ordered = o->getAlbumSort();
			const std::string& albumHash = o->getAlbumHash();
			const std::string& artistHash = o->getAlbumArtistHash();
			const std::string& url = o->getURL();

			if (debug) std::cout << "TLibrary::updateLibraryMappings() Add album \"" << album << "\"/\"" << displayalbum << "\"" << std::endl;

			if (c_sort != sortartist) {
				c_sort = sortartist;
				library.artists.all[sortartist].name = artist;
				library.artists.all[sortartist].originalname = originalartist;
				library.artists.all[sortartist].displayname = displayartist;
				library.artists.all[sortartist].displayoriginalname = displayoriginalartist;
				library.artists.all[sortartist].hash = artistHash;
				library.artists.all[sortartist].url = url;

				// ATTENTION: Artist change is detected once only:
				// --> Add artist to all list and do cleanup later
				// --> remove artists with no albums from maps afterwards
				library.artists.cd[sortartist].name = artist;
				library.artists.cd[sortartist].originalname = originalartist;
				library.artists.cd[sortartist].displayname = displayartist;
				library.artists.cd[sortartist].displayoriginalname = displayoriginalartist;
				library.artists.cd[sortartist].hash = artistHash;
				library.artists.cd[sortartist].url = url;

				library.artists.hdcd[sortartist].name = artist;
				library.artists.hdcd[sortartist].originalname = originalartist;
				library.artists.hdcd[sortartist].displayname = displayartist;
				library.artists.hdcd[sortartist].displayoriginalname = displayoriginalartist;
				library.artists.hdcd[sortartist].hash = artistHash;
				library.artists.hdcd[sortartist].url = url;

				library.artists.dsd[sortartist].name = artist;
				library.artists.dsd[sortartist].originalname = originalartist;
				library.artists.dsd[sortartist].displayname = displayartist;
				library.artists.dsd[sortartist].displayoriginalname = displayoriginalartist;
				library.artists.dsd[sortartist].hash = artistHash;
				library.artists.dsd[sortartist].url = url;

				library.artists.dvd[sortartist].name = artist;
				library.artists.dvd[sortartist].originalname = originalartist;
				library.artists.dvd[sortartist].displayname = displayartist;
				library.artists.dvd[sortartist].displayoriginalname = displayoriginalartist;
				library.artists.dvd[sortartist].hash = artistHash;
				library.artists.dvd[sortartist].url = url;

				library.artists.bd[sortartist].name = artist;
				library.artists.bd[sortartist].originalname = originalartist;
				library.artists.bd[sortartist].displayname = displayartist;
				library.artists.bd[sortartist].displayoriginalname = displayoriginalartist;
				library.artists.bd[sortartist].hash = artistHash;
				library.artists.bd[sortartist].url = url;

				library.artists.hr[sortartist].name = artist;
				library.artists.hr[sortartist].originalname = originalartist;
				library.artists.hr[sortartist].displayname = displayartist;
				library.artists.hr[sortartist].displayoriginalname = displayoriginalartist;
				library.artists.hr[sortartist].hash = artistHash;
				library.artists.hr[sortartist].url = url;
			}
			if (c_album != album) {
				c_album = album;

				// Compilation flag and media type is fixed for one album!
				media = o->getMediaType();
				date = o->getFileTime();
				inserted = o->getInsertedTime();
				compilation = o->getCompilation();

				// For one artist the album should be unique!!!
				// --> use ordered map here...
				library.artists.all[sortartist].albums[album].name = album;
				library.artists.all[sortartist].albums[album].artist = artist;
				library.artists.all[sortartist].albums[album].originalartist = originalartist;
				library.artists.all[sortartist].albums[album].genre = genre;
				library.artists.all[sortartist].albums[album].displayname = displayalbum;
				library.artists.all[sortartist].albums[album].displayartist = displayartist;
				library.artists.all[sortartist].albums[album].displayoriginalartist = displayoriginalartist;
				library.artists.all[sortartist].albums[album].displaygenre = displaygenre;
				library.artists.all[sortartist].albums[album].hash = albumHash;
				library.artists.all[sortartist].albums[album].url = url;
				library.artists.all[sortartist].albums[album].compilation = compilation;
				library.artists.all[sortartist].albums[album].inserted = inserted;
				library.artists.all[sortartist].albums[album].date = date;

				switch (media) {
					case EMT_CD:
						if (debug) std::cout << "TLibrary::updateLibraryMappings() CD: Add album \"" << artist << "\"/\"" << album << "\"" << std::endl;
						library.artists.cd[sortartist].albums[album].name = album;
						library.artists.cd[sortartist].albums[album].artist = artist;
						library.artists.cd[sortartist].albums[album].originalartist = originalartist;
						library.artists.cd[sortartist].albums[album].genre = genre;
						library.artists.cd[sortartist].albums[album].displayname = displayalbum;
						library.artists.cd[sortartist].albums[album].displayartist = displayartist;
						library.artists.cd[sortartist].albums[album].displayoriginalartist = displayoriginalartist;
						library.artists.cd[sortartist].albums[album].displaygenre = displaygenre;
						library.artists.cd[sortartist].albums[album].hash = albumHash;
						library.artists.cd[sortartist].albums[album].url = url;
						library.artists.cd[sortartist].albums[album].compilation = compilation;
						library.artists.cd[sortartist].albums[album].inserted = inserted;
						library.artists.cd[sortartist].albums[album].date = date;
						++cdCount;
						break;
					case EMT_HDCD:
						if (debug) std::cout << "TLibrary::updateLibraryMappings() HDCD: Add album \"" << artist << "\"/\"" << album << "\"" << std::endl;
						library.artists.hdcd[sortartist].albums[album].name = album;
						library.artists.hdcd[sortartist].albums[album].artist = artist;
						library.artists.hdcd[sortartist].albums[album].originalartist = originalartist;
						library.artists.hdcd[sortartist].albums[album].genre = genre;
						library.artists.hdcd[sortartist].albums[album].displayname = displayalbum;
						library.artists.hdcd[sortartist].albums[album].displayartist = displayartist;
						library.artists.hdcd[sortartist].albums[album].displayoriginalartist = displayoriginalartist;
						library.artists.hdcd[sortartist].albums[album].displaygenre = displaygenre;
						library.artists.hdcd[sortartist].albums[album].hash = albumHash;
						library.artists.hdcd[sortartist].albums[album].url = url;
						library.artists.hdcd[sortartist].albums[album].compilation = compilation;
						library.artists.hdcd[sortartist].albums[album].inserted = inserted;
						library.artists.hdcd[sortartist].albums[album].date = date;
						++hdcdCount;
						break;
					case EMT_DSD:
						if (debug) std::cout << "TLibrary::updateLibraryMappings() DSD: Add album \"" << artist << "\"/\"" << album << "\"" << std::endl;
						library.artists.dsd[sortartist].albums[album].name = album;
						library.artists.dsd[sortartist].albums[album].artist = artist;
						library.artists.dsd[sortartist].albums[album].originalartist = originalartist;
						library.artists.dsd[sortartist].albums[album].genre = genre;
						library.artists.dsd[sortartist].albums[album].displayname = displayalbum;
						library.artists.dsd[sortartist].albums[album].displayartist = displayartist;
						library.artists.dsd[sortartist].albums[album].displayoriginalartist = displayoriginalartist;
						library.artists.dsd[sortartist].albums[album].displaygenre = displaygenre;
						library.artists.dsd[sortartist].albums[album].hash = albumHash;
						library.artists.dsd[sortartist].albums[album].url = url;
						library.artists.dsd[sortartist].albums[album].compilation = compilation;
						library.artists.dsd[sortartist].albums[album].inserted = inserted;
						library.artists.dsd[sortartist].albums[album].date = date;
						++dsdCount;
						break;
					case EMT_DVD:
						if (debug) std::cout << "TLibrary::updateLibraryMappings() DVD: Add album \"" << artist << "\"/\"" << album << "\"" << std::endl;
						library.artists.dvd[sortartist].albums[album].name = album;
						library.artists.dvd[sortartist].albums[album].artist = artist;
						library.artists.dvd[sortartist].albums[album].originalartist = originalartist;
						library.artists.dvd[sortartist].albums[album].genre = genre;
						library.artists.dvd[sortartist].albums[album].displayname = displayalbum;
						library.artists.dvd[sortartist].albums[album].displayartist = displayartist;
						library.artists.dvd[sortartist].albums[album].displayoriginalartist = displayoriginalartist;
						library.artists.dvd[sortartist].albums[album].displaygenre = displaygenre;
						library.artists.dvd[sortartist].albums[album].hash = albumHash;
						library.artists.dvd[sortartist].albums[album].url = url;
						library.artists.dvd[sortartist].albums[album].compilation = compilation;
						library.artists.dvd[sortartist].albums[album].inserted = inserted;
						library.artists.dvd[sortartist].albums[album].date = date;
						++dvdCount;
						break;
					case EMT_BD:
						if (debug) std::cout << "TLibrary::updateLibraryMappings() BD: Add album \"" << artist << "\"/\"" << album << "\"" << std::endl;
						library.artists.bd[sortartist].albums[album].name = album;
						library.artists.bd[sortartist].albums[album].artist = artist;
						library.artists.bd[sortartist].albums[album].originalartist = originalartist;
						library.artists.bd[sortartist].albums[album].genre = genre;
						library.artists.bd[sortartist].albums[album].displayname = displayalbum;
						library.artists.bd[sortartist].albums[album].displayartist = displayartist;
						library.artists.bd[sortartist].albums[album].displayoriginalartist = displayoriginalartist;
						library.artists.bd[sortartist].albums[album].displaygenre = displaygenre;
						library.artists.bd[sortartist].albums[album].hash = albumHash;
						library.artists.bd[sortartist].albums[album].url = url;
						library.artists.bd[sortartist].albums[album].compilation = compilation;
						library.artists.bd[sortartist].albums[album].inserted = inserted;
						library.artists.bd[sortartist].albums[album].date = date;
						++bdCount;
						break;
					case EMT_HR:
						if (debug) std::cout << "TLibrary::updateLibraryMappings() HR: Add album \"" << artist << "\"/\"" << album << "\"" << std::endl;
						library.artists.hr[sortartist].albums[album].name = album;
						library.artists.hr[sortartist].albums[album].artist = artist;
						library.artists.hr[sortartist].albums[album].originalartist = originalartist;
						library.artists.hr[sortartist].albums[album].genre = genre;
						library.artists.hr[sortartist].albums[album].displayname = displayalbum;
						library.artists.hr[sortartist].albums[album].displayartist = displayartist;
						library.artists.hr[sortartist].albums[album].displayoriginalartist = displayoriginalartist;
						library.artists.hr[sortartist].albums[album].displaygenre = displaygenre;
						library.artists.hr[sortartist].albums[album].hash = albumHash;
						library.artists.hr[sortartist].albums[album].url = url;
						library.artists.hr[sortartist].albums[album].compilation = compilation;
						library.artists.hr[sortartist].albums[album].inserted = inserted;
						library.artists.hr[sortartist].albums[album].date = date;
						++hrCount;
						break;
					default:
						break;
				}

				library.albums[albumHash].name = album;
				library.albums[albumHash].artist = artist;
				library.albums[albumHash].originalartist = originalartist;
				library.albums[albumHash].genre = genre;
				library.albums[albumHash].displayname = displayalbum;
				library.albums[albumHash].displayartist = displayartist;
				library.albums[albumHash].displayoriginalartist = displayoriginalartist;
				library.albums[albumHash].displaygenre = displaygenre;
				library.albums[albumHash].hash = albumHash;
				library.albums[albumHash].url = url;
				library.albums[albumHash].compilation = compilation;
				library.albums[albumHash].inserted = inserted;
				library.albums[albumHash].date = date;

				library.ordered[ordered].name = album;
				library.ordered[ordered].artist = artist;
				library.ordered[ordered].originalartist = originalartist;
				library.ordered[ordered].genre = genre;
				library.ordered[ordered].displayname = displayalbum;
				library.ordered[ordered].displayartist = displayartist;
				library.ordered[ordered].displayoriginalartist = displayoriginalartist;
				library.ordered[ordered].displaygenre = displaygenre;
				library.ordered[ordered].hash = albumHash;
				library.ordered[ordered].url = url;
				library.ordered[ordered].compilation = compilation;
				library.ordered[ordered].inserted = inserted;
				library.ordered[ordered].date = date;

			}

			// Build artist/media lists
			library.artists.all[sortartist].albums[album].songs.push_back(o);

			switch (media) {
				case EMT_CD:
					if (debug) std::cout << "TLibrary::updateLibraryMappings() CD: Add song \"" << artist << "\"/\"" << album << "\"/\"" << o->getTitle() << "\"" << std::endl;
					library.artists.cd[sortartist].albums[album].songs.push_back(o);
					break;
				case EMT_HDCD:
					if (debug) std::cout << "TLibrary::updateLibraryMappings() HDCD: Add song \"" << artist << "\"/\"" << album << "\"/\"" << o->getTitle() << "\"" << std::endl;
					library.artists.hdcd[sortartist].albums[album].songs.push_back(o);
					break;
				case EMT_DSD:
					if (debug) std::cout << "TLibrary::updateLibraryMappings() DSD: Add song \"" << artist << "\"/\"" << album << "\"/\"" << o->getTitle() << "\"" << std::endl;
					library.artists.dsd[sortartist].albums[album].songs.push_back(o);
					break;
				case EMT_DVD:
					if (debug) std::cout << "TLibrary::updateLibraryMappings() DVD: Add song \"" << artist << "\"/\"" << album << "\"/\"" << o->getTitle() << "\"" << std::endl;
					library.artists.dvd[sortartist].albums[album].songs.push_back(o);
					break;
				case EMT_BD:
					if (debug) std::cout << "TLibrary::updateLibraryMappings() BD: Add song \"" << artist << "\"/\"" << album << "\"/\"" << o->getTitle() << "\"" << std::endl;
					library.artists.bd[sortartist].albums[album].songs.push_back(o);
					break;
				case EMT_HR:
					if (debug) std::cout << "TLibrary::updateLibraryMappings() HR: Add song \"" << artist << "\"/\"" << album << "\"/\"" << o->getTitle() << "\"" << std::endl;
					library.artists.hr[sortartist].albums[album].songs.push_back(o);
					break;
				default:
					break;
			}

			library.albums[albumHash].songs.push_back(o);
			library.ordered[ordered].songs.push_back(o);
		}
	}

	// Remove artists without album entries
	cleanArtistMap(library.artists.all);
	cleanArtistMap(library.artists.cd);
	cleanArtistMap(library.artists.hdcd);
	cleanArtistMap(library.artists.dsd);
	cleanArtistMap(library.artists.dvd);
	cleanArtistMap(library.artists.bd);
	cleanArtistMap(library.artists.hr);

	// Create letter lookup lists for artists
	createLetterMap(library.artists.all,  library.letters.all);
	createLetterMap(library.artists.cd,   library.letters.cd);
	createLetterMap(library.artists.hdcd, library.letters.hdcd);
	createLetterMap(library.artists.dsd,  library.letters.dsd);
	createLetterMap(library.artists.dvd,  library.letters.dvd);
	createLetterMap(library.artists.bd,   library.letters.bd);
	createLetterMap(library.artists.hr,   library.letters.hr);

	// Update tracks count on real album song data
	updateTracksCount();
	updateRecentAlbums();

	// Save mappings to file...
	if (debug) {
		saveLibraryMappings();
	}
}

void TLibrary::updateVariousArtists() {
	if (!library.tracks.songs.empty()) {
		PSong song;
		TSongList songs;
		std::string c_album, c_mainartist, c_albumartist;
		bool mainartistchanged = false;
		bool albumartistchanged = false;
		std::string va(VARIOUS_ARTISTS_NAME);
		std::string vn;

		// Sort by album to detect album change
		sortByAlbum();
		
		// Scan all tracks for artist change in the same album
		for (size_t i=0; i<library.tracks.songs.size(); ++i) {
			song = library.tracks.songs[i];
			if (util::assigned(song)) {
				// Artist may be renamed and stored in library as "Various Artists"
				// --> Use original artist name to setup compilation tag
				const std::string& mainartist = song->getOriginalArtist();
				const std::string& albumartist = song->getOriginalAlbumArtist();
				const std::string& album = song->getAlbumSort();
				if (c_album != album) {
					// New album with given artist detected
					c_album = album;
					c_mainartist = mainartist;
					c_albumartist = albumartist;

					// Check if song list belongs to compilation
					if (mainartistchanged) {
						// Move song objects from album list to various artists list
						for (size_t j=0; j<songs.size(); ++j) {
							PSong o = songs[j];
							const std::string& vv = albumartistchanged ? va : vn;
							o->setCompilation(vv);
						}
					}

					// Clear song list
					songs.clear();
					mainartistchanged = false;
					albumartistchanged = false;

				} else {
					// Scan for artist change in album
					if (!mainartistchanged) {
						if (c_mainartist != mainartist) {
							mainartistchanged = true;
						}
					}
					if (!albumartistchanged) {
						if (c_albumartist != albumartist) {
							albumartistchanged = true;
						}
					}
				}

				// Add song to album song list
				songs.push_back(song);
			}
		}

		// Last album in list was compilation?
		if (!songs.empty()) {
			if (mainartistchanged) {
				// Move song objects from last album list to various artists list
				for (size_t j=0; j<songs.size(); ++j) {
					PSong o = songs[j];
					const std::string& vv = albumartistchanged ? va : vn;
					o->setCompilation(vv);
				}
			}
		}

	}
}

void TLibrary::clearArtistMap(TArtistMap& artists) {
	if (!artists.empty()) {
		TArtistMap::iterator artist = artists.begin();
		while (artist != artists.end()) {
			TAlbumMap::iterator album = artist->second.albums.begin();
			while (album != artist->second.albums.end()) {
				album->second.songs.clear();
				++album;
			}
			artist->second.albums.clear();
			++artist;
		}
		artists.clear();
	}
}

void TLibrary::cleanArtistMap(TArtistMap& artists) {
	if (!artists.empty()) {
		TArtistMap::const_iterator artist = artists.begin();
		while (artist != artists.end()) {
			if (artist->second.albums.size() <= 0) {
				artist = artists.erase(artist);
			} else {
				++artist;
			}
		}
	}
}

void TLibrary::createLetterMap(TArtistMap& artists, TLetterMap& letters) {
	if (!artists.empty()) {
		size_t count = 0;
		std::string name;
		TArtistMap::const_iterator artist;
		for (char c = 'A'; c <= 'Z'; ++c) {
			artist = artists.begin();
			while (artist != artists.end()) {
				// Do not add compilations to letter list
				name = artist->second.name;
				if (filterArtistName(name, c)) {
					letters[c] = ++count;
				}
				++artist;
			}
		}
	}
}

void TLibrary::saveArtistMap(const TArtistMap& artists, const std::string& fileName) {
	util::deleteFile(fileName);
	util::TStringList list;
	TArtistMap::const_iterator artist = artists.begin();
	while (artist != artists.end()) {
		list.add("Artist \"" + artist->second.name + "\" [" + artist->second.hash + "]");
		TAlbumMap::const_iterator album = artist->second.albums.begin();
		while (album != artist->second.albums.end()) {
			list.add("  Album \"" + album->second.name + "\" [" + album->second.hash + "]");
			for (size_t i=0; i<album->second.songs.size(); ++i) {
				list.add("    Track " + album->second.songs[i]->getTrack() + " \"" + album->second.songs[i]->getTitle() + "\"");
			}
			++album;
		}
		++artist;
	}
	if (!list.empty())
		list.saveToFile(fileName);
}

void TLibrary::saveHashedMap(const THashedMap& albums, const std::string& fileName) {
	util::deleteFile(fileName);
	util::TStringList list;
	THashedMap::const_iterator album = albums.begin();
	while (album != albums.end()) {
		const std::string& compilation = album->second.compilation ? "yes" : "no";
		list.add("Album \"" + album->first + "\"");
		list.add("  Hash \"" + album->second.hash + "\"");
		list.add("  Name \"" + album->second.name + "\"");
		list.add("  Artist \"" + album->second.artist + "\"");
		list.add("  Originalartist \"" + album->second.originalartist + "\"");
		list.add("  Compilation = " + compilation);
		++album;
	}
	if (!list.empty())
		list.saveToFile(fileName);
}

void TLibrary::saveAlbumMap(const TAlbumMap& albums, const std::string& fileName) {
	util::deleteFile(fileName);
	util::TStringList list;
	TAlbumMap::const_iterator album = albums.begin();
	while (album != albums.end()) {
		const std::string& compilation = album->second.compilation ? "yes" : "no";
		list.add("Album \"" + album->first + "\"");
		list.add("  Hash \"" + album->second.hash + "\"");
		list.add("  Name \"" + album->second.name + "\"");
		list.add("  Artist \"" + album->second.artist + "\"");
		list.add("  Originalartist \"" + album->second.originalartist + "\"");
		list.add("  Compilation = " + compilation);
		++album;
	}
	if (!list.empty())
		list.saveToFile(fileName);
}

void TLibrary::updateTracksCount() {
	if (!library.albums.empty()) {
		THashedMap::const_iterator album = library.albums.begin();
		while (album != library.albums.end()) {
			size_t count = album->second.songs.size();
			for (size_t i=0; i<count; ++i) {
				PSong o = album->second.songs[i];
				if (util::assigned(o))
					o->setTrackCount(count);
			}
			++album;
		}
	}
}

void TLibrary::updateRecentAlbums() {
	util::clearObjectList(library.recent);
	if (!library.albums.empty()) {
		THashedMap::const_iterator albums = library.albums.begin();
		while (albums != library.albums.end()) {
			const TAlbum& album = albums->second;
			if (album.inserted > util::epoch()) {
				TAlbum* o = new TAlbum;
				*o = album;
				library.recent.push_back(o);
			}
			++albums;
		}
	}
	if (!library.recent.empty())
		std::sort(library.recent.begin(), library.recent.end(), dateSorterDesc);
}


TLibraryResults TLibrary::updateArtistsHTML(const std::string& filter, bool& changed, music::EMediaType type, const music::CConfigValues& config, const EViewType view) {

	// Check if something has to be done...
	if (artistMedia == type && artistView == view) {
		if (!filter.empty()) {
			// Check for same filter as before
			if (filter == artistFilter && !artistsHTML.empty()) {
				changed = false;
				return artistResult;
			}
		} else {
			// Filter is empty and last filter is also empty
			// --> return if string list is not empty
			if (!artistsHTML.empty() && artistFilter.empty()) {
				changed = false;
				return artistResult;
			}
		}
	}

	// Changed data requested
	artistView = view;
	artistMedia = type;
	artistFilter = filter;
	changed = true;

	// Create dynamic HTML content
	artistResult = getArtistsHTML(artistsHTML, filter, type, config, view);

	return artistResult;
}


std::string TLibrary::getMediaName(const music::EMediaType type) {
	const struct music::TMediaType *it;
	for (it = music::formats; util::assigned(it->description); ++it) {
		if (it->media == type) {
			return it->shortcut;
		}
	}
	return "Unknown";
}

TLibraryResults TLibrary::getArtistsHTML(util::TStringList& html, const std::string& filter, music::EMediaType type, const music::CConfigValues& config, const EViewType view) {

	// Link to artist or media page?
	std::string format = (type == EMT_ALL) ? "albums" : "format";       // Link to all albums or albums of special type (CD, HDCD, DVD, ...)
	std::string style  = (view == ELV_ARTIST) ? "artists" : "listview"; // Basename of the web page media.html or listview.html
	std::string base   = (type == EMT_ALL) ? style : "media";           // Display only special media type (CD, HDCD, DVD, ...)

	// Changed data requested
	char c = 'A';
	char f = 'A';
	size_t count = 0, entries = 0;
	std::string next, prev;
	std::string nlink, plink;
	html.clear();

	// Get letter map for given media type
	const TLetterMap * letters = &library.letters.all;
	switch (type) {
		case EMT_CD:
			letters = &library.letters.cd;
			break;
		case EMT_HDCD:
			letters = &library.letters.hdcd;
			break;
		case EMT_DSD:
			letters = &library.letters.dsd;
			break;
		case EMT_DVD:
			letters = &library.letters.dvd;
			break;
		case EMT_BD:
			letters = &library.letters.bd;
			break;
		case EMT_HR:
			letters = &library.letters.hr;
			break;
		default:
			break;
	}

	if (!letters->empty()) {

		// Get first and last letter for current media type
		char first = letters->begin()->first;
		char last = util::pred(letters->end())->first;

		// Set next and previous search character mask in upper case letters
		c = first;
		char n, p;
		if (!filter.empty()) {
			// Is given filter in current letter map or is it wildcard?
			f = toupper(filter[0]);
			if (letters->end() != letters->find(f) || f == CHAR_NUMERICAL_ARTIST || f == CHAR_VARIOUS_ARTIST) {
				c = f;
			}
		}

		// Get current letter from map
		TLetterConstIterator at, it = letters->find(c);
		if (letters->end() != it) {

			// Get next letter in map
			at = it; ++at;
			if (letters->end() != at) {
				n = at->first;
			} else {
				n = CHAR_NUMERICAL_ARTIST;
			}

			// Get previous letter in map
			at = it; --at;
			if (letters->end() != at) {
				p = at->first;
			} else {
				p = CHAR_VARIOUS_ARTIST;
			}

		} else {
			switch (f) {
				case CHAR_NUMERICAL_ARTIST:
					n = CHAR_VARIOUS_ARTIST;
					p = last;
					break;
				case CHAR_VARIOUS_ARTIST:
					n = first;
					p = CHAR_NUMERICAL_ARTIST;
					break;
				default:
					n = first;
					p = last;
					break;
			}
		}

		// Next and previous search strings
		next = std::string(&n, 1);
		prev = std::string(&p, 1);
		plink = root + LIBRARY_ROOT_URL + base + ".html?prepare=yes&title=" + base + "&filter=" + prev;
		nlink = root + LIBRARY_ROOT_URL + base + ".html?prepare=yes&title=" + base + "&filter=" + next;

		// Add header buttons for next and previous compare strings
		addArtistQuickLinks(html, *letters, base, c);
		addNavigationQuickLinks(html, plink, nlink, prev, next);

	} // if (!letters->empty()) {


	bool empty = true;
	switch (type) {
		case EMT_CD:
			empty = library.artists.cd.empty();
			break;
		case EMT_HDCD:
			empty = library.artists.hdcd.empty();
			break;
		case EMT_DSD:
			empty = library.artists.dsd.empty();
			break;
		case EMT_DVD:
			empty = library.artists.dvd.empty();
			break;
		case EMT_BD:
			empty = library.artists.bd.empty();
			break;
		case EMT_HR:
			empty = library.artists.hr.empty();
			break;
		default:
			empty = library.artists.all.empty();
			break;
	}

	// Generate dynamic HTML
	if (!empty) {
		bool ok = true;
		bool goon = false;
		bool various = false;
		bool compilation = false;
		std::string albumGlyph, albumHint, albumLink, addGlyph, addHint, addLink;
		std::string urlalbum, artisthash, albumhash, albumname, albumyear, mediatype, tracks, text;
		std::string displayalbumname;
		TConstArtistList artists;

		// Get iterator through artis for given media type
		TArtistMap::const_iterator artist;
		switch (type) {
			case EMT_CD:
				artist = library.artists.cd.begin();
				goon = true;
				break;
			case EMT_HDCD:
				artist = library.artists.hdcd.begin();
				goon = true;
				break;
			case EMT_DSD:
				artist = library.artists.dsd.begin();
				goon = true;
				break;
			case EMT_DVD:
				artist = library.artists.dvd.begin();
				goon = true;
				break;
			case EMT_BD:
				artist = library.artists.bd.begin();
				goon = true;
				break;
			case EMT_HR:
				artist = library.artists.hr.begin();
				goon = true;
				break;
			default:
				artist = library.artists.all.begin();
				goon = true;
				break;
		}

		while (goon) {

			// Compare current iterator to end of list
			switch (type) {
				case EMT_CD:
					goon = artist != library.artists.cd.end();
					break;
				case EMT_HDCD:
					goon = artist != library.artists.hdcd.end();
					break;
				case EMT_DSD:
					goon = artist != library.artists.dsd.end();
					break;
				case EMT_DVD:
					goon = artist != library.artists.dvd.end();
					break;
				case EMT_BD:
					goon = artist != library.artists.bd.end();
					break;
				case EMT_HR:
					goon = artist != library.artists.hr.end();
					break;
				default:
					goon = artist != library.artists.all.end();
					break;
			}

			// End of given artist list?
			if (!goon) {
				break;
			}

			// Check for wildcard filter
			ok = false;
			various = false;
			compilation = false;
			const std::string& artistname = artist->second.name;
			TAlbumMap::const_iterator album = artist->second.albums.begin();
			if (album != artist->second.albums.end()) {
				if (album->second.compilation) {
					compilation = true;
				}
			}

			// Check for varios artist category
			if (compilation) {
				various = filterVariousArtistName(artist->second.originalname, config.categories);
			}

			if (!artistname.empty()) {
				if (c == CHAR_NUMERICAL_ARTIST) {
					if (!various) {
						ok = !util::isAlpha(artistname[0]);
					}
				} else {
					if (c == CHAR_VARIOUS_ARTIST) {
						if (various) {
							ok = true;
						}
					} else {
						if (!various) {
							ok = filterArtistName(artistname, c);
						}
					}
				}
			}

			if (ok) {
				// Add filtered artist to list
				artists.push_back(&artist->second);
				++count;
			};

			++artist;
		}

		// Add HTML "header" for thumbnail list
		html.add("");
		html.add("<!-- SOT -->");
		if (view == ELV_ARTIST) {
			html.add("  <div id=\"thumbnails\" class=\"row\">");
		} else {
			html.add("  <div id=\"thumbnails\" class=\"caption\">");
			html.add("    <div class=\"spacer2\"></div>");
		}

		// Create HTML from sorted list
		if (!artists.empty()) {

			// Sort artist list
			if (sortCaseInsensitive) {
				std::sort(artists.begin(), artists.end(), artistListCaseSorterDesc);
			} else {
				std::sort(artists.begin(), artists.end(), artistListSorterDesc);
			}

			// Loop through filtered artists...
			for (size_t i=0; i<artists.size(); i++) {
				const TArtist* o = artists[i];
				if (util::assigned(o)) {
					const TArtist artist = *o;
					ok = false;

					if (!ok && ELV_ARTIST == view) {
						const std::string& artistname = artist.name;
						const std::string& searchname = artistname;
						const std::string& urlname = util::TURL::encode(artistname);
						const std::string& displayartistname = compilation ? artist.displayname : artist.displayoriginalname;
						const std::string& artisthash = artist.hash;
						const std::string& search = util::TURL::encode(artist.originalname);
						size_t albums = artist.albums.size();

						// Take first album of artist for preview
						TAlbumMap::const_iterator album = artist.albums.begin();
						if (album != artist.albums.end()) {
							albumhash = album->second.hash;
							albumname = album->second.name;
							urlalbum = util::TURL::encode(albumname);
							displayalbumname = album->second.displayname;
						}

						// Check for multiple albums per artist...
						if (albums == 1) {
							// Link to tracks view for single artist album
							albumLink = root + LIBRARY_ROOT_URL "tracks.html?prepare=yes&title=tracks&filter=" + albumhash;
							albumHint = "Show tracks for album &quot;" + displayalbumname + "&quot;";
							albumGlyph = "glyphicon-cd";
							addHint = "Add album &quot;" + displayalbumname + "&quot; to current playlist";
							addGlyph = "glyphicon-file";
							text = " album";
						} else {
							// Link to album view
							std::string all = type == EMT_ALL ? " all " : " ";
							albumLink = root + LIBRARY_ROOT_URL + format + ".html?prepare=yes&title=" + format + "&filter=" + urlname;
							albumHint = "Show" + all + "albums of &quot;" + displayartistname + "&quot;";
							albumGlyph = "glyphicon-cd";
							addHint = "Add" + all + "albums for &quot;" + displayartistname + "&quot; to current playlist";
							addGlyph = "glyphicon-duplicate";
							text = " albums";
						}

						html.add("  <div class=\"col-xs-5 col-sm-4 col-md-3\">");
						html.add("    <div class=\"thumbnail\">");

						// Lazy load thumbnails when threshold reached
						if (entries > LAZY_LOADER_THRESHOLD_LOW) {
							html.add("      <img style=\"box-shadow: 6px 6px 8px gray; border-radius: " + std::string(IMAGE_BORDER_RADIUS) + ";\" class=\"lazy-loader\" src=\"/images/rectangular200.png\" data-src=\"/rest/thumbnails/" + albumhash + "-200.jpg\" destination=\"" + albumLink + "\" onclick=\"onCoverClick(event)\" data-toggle=\"tooltip\" data-placement=\"bottom\" title=\"" + albumHint + "\">");
						} else {
							html.add("      <img style=\"box-shadow: 6px 6px 8px gray; border-radius: " + std::string(IMAGE_BORDER_RADIUS) + ";\" src=\"/rest/thumbnails/" + albumhash + "-200.jpg\" destination=\"" + albumLink + "\" onclick=\"onCoverClick(event)\" alt=\"Thumbnail requested...\" data-toggle=\"tooltip\" data-placement=\"bottom\" title=\"" + albumHint + "\">");
						}

						html.add("      <div class=\"caption\">");
						html.add("        <span>");
						html.add("          <h4 class=\"text-ellipsis thumbnail-header\">" + displayartistname + "</h4>");
						html.add("        </span>");
						html.add("        <p>" + std::to_string((size_u)albums) + text + "</p>");
						if (albums < 2) {
							html.add("          <div class=\"btn-group\" role=\"group\">");
						}
						html.add("          <button id=\"btnAdd\" name=\"Check\" value=\"ADDARTIST\" artist=\"" + urlname + "\" tvname=\"" + searchname + "\" hash=\"" + artisthash + "\" album=\"" + albumhash + "\" onclick=\"onAddArtistClick(event)\" class=\"btn btn-responsive-md btn-default\" data-toggle=\"tooltip\" title=\"" + addHint + "\">");
						html.add("            <span class=\"glyphicon glyphicon " + addGlyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
						html.add("          </button>");
						if (albums < 2) {
							html.add("          <button id=\"btnPlay\" name=\"Check\" value=\"ADDPLAYALBUM\" album=\"" + urlalbum + "\" tvname=\"" + albumname + "\" hash=\"" + albumhash + "\" onclick=\"onPlayAlbumClick(event)\" class=\"btn btn-responsive-md btn-default\" data-toggle=\"tooltip\" title=\"Play songs of album &quot;" + displayalbumname + "&quot; now\">");
							html.add("            <span class=\"glyphicon glyphicon-play\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
							html.add("          </button>");
							html.add("        </div>");
						}
						html.add("          <a href=\"" + albumLink + "\" class=\"btn btn-responsive-md btn-default pull-right\" role=\"button\" aria-disabled=\"true\" data-toggle=\"tooltip\" title=\"" + albumHint + "\">");
						html.add("            <span class=\"glyphicon " + albumGlyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
						html.add("          </a>");

						html.add("      </div>");
						html.add("    </div>");
						html.add("  </div>");
						html.add("");

						entries++;
						ok = true;
					}

					if (!ok && ELV_ALBUM == view) {
						const std::string& artistname = artist.name;
						const std::string& searchname = artistname;
						const std::string& displayartistname = compilation ? artist.displayname : artist.displayoriginalname;
						const std::string& urlname = util::TURL::encode(artistname);
						size_t albums = artist.albums.size();

						// Add only artist with albums...
						if (albums > 0) {

							if (albums > 1) {
								albumLink = root + LIBRARY_ROOT_URL + "albums.html?prepare=yes&title=albums&filter=" + urlname;
							} else {
								TAlbumMap::const_iterator album = artist.albums.begin();
								if (album != artist.albums.end()) {
									albumLink = root + LIBRARY_ROOT_URL + "tracks.html?prepare=yes&title=tracks&filter=" + album->second.hash;
								} else {
									albumLink = root + LIBRARY_ROOT_URL + "albums.html?prepare=yes&title=albums&filter=" + urlname;
								}
							}

							html.add("  <h4 style=\"font-weight: bold;\" uri=\"" + albumLink + "\" onclick=\"onListViewHeaderClick(event)\">" + displayartistname + "</h4>");
							html.add("  <hr style=\"margin-top: 0px; margin-bottom: 7px;\" />");

							// Take first album of artist for preview
							TAlbumMap::const_iterator album = artist.albums.begin();
							if (album != artist.albums.end()) {
								TConstAlbumList list;

								html.add("  <table style=\"display: table; width: 100%;\">");
								html.add("    <tbody>");

								// Add albums from ordered map to vector list
								do {
									list.push_back(&album->second);
									album++;
								} while (album != artist.albums.end());

								// Sort albums by year?
								if (config.sortAlbumsByYear) {
									std::sort(list.begin(), list.end(), yearSorterDesc);
								}

								// Walk through the album list
								for (size_t i=0; i<list.size(); ++i) {
									const TAlbum* album = list[i];
									if (util::assigned(album)) {
										albumhash = album->hash;
										albumname = album->name;
										displayalbumname = album->displayname;
										albumLink = root + LIBRARY_ROOT_URL + "tracks.html?prepare=yes&title=tracks&filter=" + albumhash;
										urlalbum  = util::TURL::encode(albumname);
										albumyear = "1900";
										mediatype = "CD";
										tracks = "0 Tracks";
										text = "Undefined";

										if (!album->songs.empty()) {
											const music::TSong* o = album->songs[0];
											if (util::assigned(o)) {
												mediatype = getMediaName(o->media);
												albumyear = o->getDate();
												text = "<i>" + displayartistname + "</i><br>" + displayalbumname + " (" + albumyear + ")";
											}
											tracks = 1 == album->songs.size() ? "1 Track" : std::to_string((size_u)album->songs.size()) + " Tracks";
										}

										html.add("      <tr>");
										html.add("        <td width=\"58px\">");
										html.add("          <a href=\"/rest/thumbnails/" + albumhash + "-600.jpg\" data-lightbox=\"listview-" + albumhash + "\" data-title=\"" + text + "\">");

										// Lazy load thumbnails when threshold reached
										if (entries > LAZY_LOADER_THRESHOLD_HIGH) {
											html.add("            <img addClick=\"false\" class=\"lazy-loader\" src=\"/images/rectangular48.png\" data-src=\"/rest/thumbnails/" + albumhash + "-48.jpg\">");
										} else {
											html.add("            <img addClick=\"false\" src=\"/rest/thumbnails/" + albumhash + "-48.jpg\">");
										}

										html.add("          </a>");
										html.add("        </td>");
										html.add("        <td onclick=\"onListViewCellClick(event)\">");
										html.add("          <h4 uri=\"" + albumLink + "\">" + displayalbumname);
										html.add("            <i><small uri=\"" + albumLink + "\"><br/>" + mediatype + " - " + tracks + " (" + albumyear + ")</small></i>");
										html.add("          </h4>");
										html.add("        </td>");
										html.add("        <td width=\"128px\" style=\"text-align: right;\">");
										html.add("          <div class=\"btn-group\" role=\"group\">");
										html.add("            <button value=\"ADDPLAYALBUM\" album=\"" + urlalbum + "\" tvname=\"" + searchname + "\" hash=\"" + albumhash + "\" onclick=\"onPlayAlbumClick(event)\" class=\"btn btn-responsive-md btn-default\" data-toggle=\"tooltip\" title=\"Play songs of album &quot;" + displayalbumname + "&quot; now\">");
										html.add("              <span class=\"glyphicon glyphicon-play\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
										html.add("            </button>");
										html.add("            <button value=\"ADDALBUM\" album=\"" + urlalbum + "\" tvname=\"" + searchname + "\" hash=\"" + albumhash + "\" onclick=\"onAddAlbumClick(event)\" class=\"btn btn-responsive-md btn-default\" data-toggle=\"tooltip\" title=\"Add album &quot;" + displayalbumname + "&quot; to current playlist\">");
										html.add("              <span class=\"glyphicon glyphicon-file\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
										html.add("            </button>");
										html.add("            <button value=\"SHOWALBUMTRACKS\" album=\"" + urlalbum + "\" tvname=\"" + searchname + "\" hash=\"" + albumhash + "\" uri=\"" + albumLink + "\" onclick=\"onShowAlbumClick(event)\" class=\"btn btn-responsive-md btn-default\" data-toggle=\"tooltip\" title=\"Show tracks for albums &quot;" + displayalbumname + "&quot;\">");
										html.add("              <span class=\"glyphicon glyphicon-cd\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
										html.add("            </button>");
										html.add("          </div>");
										html.add("        </td>");
										html.add("      </tr>");

										entries++;
									}
								}

								html.add("    </tbody>");
								html.add("  </table>");
								html.add("  <br />");
							}
						}
					}
				}
			}

		} // if (!artists.empty())

		// Add swipe area if no artists found
		if (count <= 0) {
			html.add("    <p style=\"height: 300px; width: auto;\">&nbsp;</p>");
		} else {
			html.add("    <br />");
		}

		// Close "Artist" diverter
		html.add("  </div>");
		html.add("<!-- EOT -->");
		html.add("");

		// Add navigation links at bottom when more than 4 artists were found
		if (count > 4 || entries > 10) {
			addNavigationQuickLinks(html, plink, nlink, prev, next);
		}

		if (debug) {
			html.saveToFile("artists.html");
		}

	} // if (!empty)

	TLibraryResults r;
	r.albums = entries;
	r.artists = count;
	return r;
}


void TLibrary::addNavigationQuickLinks(util::TStringList& html, const std::string& plink, const std::string& nlink, const std::string& prev, const std::string& next) {
	if (!plink.empty() && !nlink.empty() && !prev.empty() && !next.empty()) {
		char p = prev[0];
		char n = next[0];
		bool isPrevGlyph = (p == CHAR_NUMERICAL_ARTIST) || (p == CHAR_VARIOUS_ARTIST);
		bool isNextGlyph = (n == CHAR_NUMERICAL_ARTIST) || (n == CHAR_VARIOUS_ARTIST);

		html.add("  <div id=\"navigation\" prev=\"" + plink + "\" next=\"" + nlink + "\" class=\"caption\">");
		html.add("    <a href=\"" + plink  + "\" class=\"btn btn-default\" role=\"button\" aria-disabled=\"true\" data-toggle=\"tooltip\" title=\"Previous letter is &quot;" + prev + "&quot;\">");
		if (isPrevGlyph) {
			std::string glyph = (p == CHAR_NUMERICAL_ARTIST) ? "glyphicon-star" : "glyphicon-menu-hamburger";
			html.add("      <span class=\"glyphicon glyphicon-chevron-left\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
			html.add("      <span class=\"glyphicon " + glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
		} else {
			html.add("      <span class=\"glyphicon glyphicon-chevron-left\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>&nbsp;" + prev);
		}
		html.add("    </a>");
		html.add("    <a href=\"" + nlink + "\" class=\"btn btn-default pull-right\" role=\"button\" aria-disabled=\"true\" data-toggle=\"tooltip\" title=\"Next letter is &quot;" + next + "&quot;\">");
		if (isNextGlyph) {
			std::string glyph = (n == CHAR_NUMERICAL_ARTIST) ? "glyphicon-star" : "glyphicon-menu-hamburger";
			html.add("      <span class=\"glyphicon " + glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
			html.add("      <span class=\"glyphicon glyphicon-chevron-right\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
		} else {
			html.add("      " + next + "&nbsp;<span class=\"glyphicon glyphicon-chevron-right\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
		}
		html.add("    </a>");
		html.add("    <h3> </h3>");
		html.add("  </div>");
	}
	html.add("");
}


void TLibrary::addArtistQuickLinks(util::TStringList& html, const TLetterMap& letters, const std::string& base, const char active) {
	if (!letters.empty()) {
		TLetterConstIterator it = letters.begin();
		char c = it->first;
		std::string caption;
		html.add("  <div id=\"quicklinks-" + std::to_string((size_u)html.size()) + "\">");
		html.add("    <div class=\"btn-group\" role=\"group\" aria-label=\"Letter Shortcuts\">");
		while (it != letters.end()) {
			c = it->first;
			caption = std::string(&c, 1);
			if (c != active)
				html.add("      <a href=\"" + root + LIBRARY_ROOT_URL + base + ".html?prepare=yes&title=" + base + "&filter=" + caption + "\" class=\"btn btn-default\">&nbsp;" + caption + "&nbsp;</a>");
			else
				html.add("      <a href=\"" + root + LIBRARY_ROOT_URL + base + ".html?prepare=yes&title=" + base + "&filter=" + caption + "\" class=\"btn btn-default active\">&nbsp;" + caption + "&nbsp;</a>");
			++it;
		}
		caption = std::string(&CHAR_NUMERICAL_ARTIST, 1);
		if (CHAR_NUMERICAL_ARTIST != active)
			html.add("      <a href=\"" + root + LIBRARY_ROOT_URL + base + ".html?prepare=yes&title=" + base + "&filter=" + caption + "\" data-toggle=\"tooltip\" title=\"Show non alphanumeric artists\" class=\"btn btn-default\"><span class=\"glyphicon glyphicon-star\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></a>");
		else
			html.add("      <a href=\"" + root + LIBRARY_ROOT_URL + base + ".html?prepare=yes&title=" + base + "&filter=" + caption + "\" data-toggle=\"tooltip\" title=\"Show non alphanumeric artists\" class=\"btn btn-default active\"><span class=\"glyphicon glyphicon-star\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></a>");
		caption = std::string(&CHAR_VARIOUS_ARTIST, 1);
		if (CHAR_VARIOUS_ARTIST != active)
			html.add("      <a href=\"" + root + LIBRARY_ROOT_URL + base + ".html?prepare=yes&title=" + base + "&filter=" + caption + "\" data-toggle=\"tooltip\" title=\"Show compilations albums\" class=\"btn btn-default\"><span class=\"glyphicon glyphicon-menu-hamburger\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></a>");
		else
			html.add("      <a href=\"" + root + LIBRARY_ROOT_URL + base + ".html?prepare=yes&title=" + base + "&filter=" + caption + "\" data-toggle=\"tooltip\" title=\"Show compilations albums\" class=\"btn btn-default active\"><span class=\"glyphicon glyphicon-menu-hamburger\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></a>");
		html.add("    </div>");
		html.add("  </div>");
		html.add("  <div class=\"caption\">");
		html.add("    <h3> </h3>");
		html.add("  </div>");
		html.add("");
	}
}


bool TLibrary::filterVariousArtistName(const std::string& name, const util::TStringList& names) {
	if (!names.empty()) {
		for (size_t i=0; i<names.size(); ++i) {
			const std::string& filter = names[i];
			if (util::strcasestr(name, filter))
				return true;
		}
	}
	return false;
}

bool TLibrary::filterArtistName(const std::string& name, char filter) {
	filter = tolower(filter);
	if (!name.empty()) {
		if (allowDeepNameInspection) {
			// Check each first letter of all valid words (size > 3) in name
			bool space = false;
			for (size_t i=0; i<name.size(); ++i) {
				if (i == 0) {
					if (filter == tolower(name[i])) {
						return true;
					}
				}
				if (space) {
					// All word beginnings after space are valid...
					if (filter == tolower(name[i])) {
						return true;
					}
				}
				// Space followed by at least 3 alphanumeric characters
				space = isValidSpace(name, i);// (' ' == name[i]);
			}
		} else {
		    // Ignore the first "The " in "The whatever group beginning with The"
			if (name.size() > 4) {
		    	if (0 == util::strncasecmp(name, "The ", 4)) {
					for (size_t i=4; i<name.size(); ++i) {
						if (name[i] != ' ') {
							return (filter == tolower(name[i]));
						}
					}
		    	}
		    }
			// Compare first letter only
			return (filter == tolower(name[0]));
		}
	}
	return false;
}

#define MIN_SPACE_LENGTH 3

bool TLibrary::isValidSpace(const std::string& name, size_t offset) {
	// Space followed by at least 3 alphanumeric characters
	if (' ' == name[offset]) {
		int alpha = 0;
		if (offset < (name.size() - MIN_SPACE_LENGTH)) {
			for (size_t i=offset+1; i<name.size(); ++i) {
				if (' ' == name[i]) {
					if (alpha <= MIN_SPACE_LENGTH)
						return false;
					else
						return true;
				} else {
					++alpha;
				}
			}
			return (alpha > MIN_SPACE_LENGTH);
		}
	}
	return false;
}

#undef MIN_SPACE_LENGTH


size_t TLibrary::updateAlbumsHTML(const std::string& filter, bool& changed, const EFilterDomain domain, music::EMediaType type, EArtistFilter partial, const music::CConfigValues& config) {

	// Check if something has to be done...
	if (albumMedia == type) {
		if (!filter.empty()) {
			// Check for same filter as before
			if (filter == albumsFilter && domain == albumsDomain && type == albumMedia && !albumsHTML.empty()) {
				changed = false;
				return albumsCount;
			}
		} else {
			// Filter is empty and last filter is also empty
			// --> return if string list is not empty
			if (albumsFilter.empty() && domain == albumsDomain && type == albumMedia && !albumsHTML.empty()) {
				changed = false;
				return albumsCount;
			}
		}
	}

	// Changed data requested
	changed = true;
	albumsFilter = filter;
	albumsDomain = domain;
	albumMedia = type;

	// Generate dynamic HTML
	size_t albums = getAlbumsHTML(albumsHTML, filter, domain, type, partial, config);

	// Store cached content properties
	if (type == EMT_ALL) {
		albumsCount = albums;
	}

	return albums;
}


size_t TLibrary::getAlbumsHTML(util::TStringList& html, const std::string& filter, const EFilterDomain domain, const music::EMediaType type, const EArtistFilter partial, const music::CConfigValues& config) {

	// Changed data requested
	size_t albums = 0;
	html.clear();

	// Generate dynamic HTML
	if (!library.ordered.empty()) {
		html.add("");
		html.add("<!-- SOT -->");
		html.add("  <div class=\"row\">");

		bool found, compilation;
		TConstAlbumList list;
		util::TStringList search(filter, ' ');
		music::EMediaType media = music::EMT_UNKNOWN;
		TAlbumMap::iterator album = library.ordered.begin();
		while (album != library.ordered.end()) {
			compilation = album->second.compilation;
			found = true;

			// Check for media type filter
			// --> Get media type from first song of album
			if (found && EMT_ALL != type) {
				found = false;
				media = music::EMT_ALL;
				if (!album->second.songs.empty()) {
					PSong o = album->second.songs[0];
					if (util::assigned(o)) {
						media = o->getMediaType();
						found = true;
					}
				}
				if (found) {
					switch (type) {
						case EMT_CD:
						case EMT_HDCD:
						case EMT_DSD:
						case EMT_DVD:
						case EMT_BD:
						case EMT_HR:
							found = media == type;
							break;
						case EMT_ALL:
							found = true;
							break;
						default:
							found = false;
							break;
					}
				}
			}

			// Apply artist filter:
			// --> Empty filter string adds album to list
			if (found && !filter.empty()) {
				switch (domain) {
					case FD_TITLE:
						for (size_t i=0; i<album->second.songs.size(); ++i) {
							PSong o = album->second.songs[i];
							if (util::assigned(o)) {
								found = filterArtistValue(filter, o->getTitle(), partial, search);
								if (found)
									break;
							}
						}
						break;
					case FD_CONDUCTOR:
						for (size_t i=0; i<album->second.songs.size(); ++i) {
							PSong o = album->second.songs[i];
							if (util::assigned(o)) {
								found = filterArtistValue(filter, o->getConductor(), partial, search);
								if (found)
									break;
							}
						}
						break;
					case FD_COMPOSER:
						for (size_t i=0; i<album->second.songs.size(); ++i) {
							PSong o = album->second.songs[i];
							if (util::assigned(o)) {
								found = filterArtistValue(filter, o->getComposer(), partial, search);
								if (found)
									break;
							}
						}
						break;
					case FD_ARTIST:
						for (size_t i=0; i<album->second.songs.size(); ++i) {
							PSong o = album->second.songs[i];
							if (util::assigned(o)) {
								if (compilation) {
									found = util::strcasestr(o->getOriginalArtist(), filter);
								} else {
									found = util::strcasestr(o->getArtist(), filter);
								}
								if (found)
									break;
							}
						}
						break;
					case FD_ALBUMARTIST:
					default:
						found = filterArtistValue(filter, album->second.artist, partial, search);
						break;
				}
			}

			// Add album to list
			if (found) {
				list.push_back(&album->second);
				++albums;
			}

			++album;
		}

		// Sort albums by year and add to HTML
		albums = addSortedAlbums(html, list, config.sortAlbumsByYear);

		// Close HTML
		html.add("  </div>");
		html.add("<!-- EOT -->");
		html.add("");
		if (debug) {
			html.saveToFile("albums.html");
		}
	}

	return albums;
}


bool TLibrary::filterArtistValue(const std::string& filter, const std::string& value, const EArtistFilter partial, const util::TStringList& search) {
	bool found = false;
	if (!filter.empty() && !value.empty()) {
		if (partial == AF_FILTER_FULL) {
			found = value.size() == filter.size();
			if (found)
				found = (0 == util::strncasecmp(value, filter, filter.size()));
		} else {
			// Search for all words...
			size_t count = 0;
			for (size_t i=0; i<search.size(); ++i) {
				if (util::strcasestr(value, search[i])) {
					++count;
				}
			}
			found = count == search.size();
		}
	}
	return found;
}


size_t TLibrary::updateSearchHTML(const std::string& filter, const EFilterDomain domain, const EMediaType media, bool& changed, const size_t max, const bool sortByYear) {
	changed = false;

	// Check if something has to be done...
	if (!filter.empty()) {
		// Check for same filter as before
		if (filter == searchFilter && !searchHTML.empty()) {
			if (domain == searchDomain && media == searchMedia) {
				return searchCount;
			}
		}
	} else {
		// Filter is empty and last filter is also empty
		// --> return if string list is not empty
		if (!searchHTML.empty() && searchFilter.empty()) {
			if (domain == searchDomain && media == searchMedia) {
				return searchCount;
			}
		}
	}

	// Changed data requested
	searchFilter = filter;
	searchDomain = domain;
	searchMedia = media;
	changed = true;

	// Return empty HTML on empty filter string
	if (filter.size() < 3) {
		searchCount = 0;
		return searchCount;
	}

	// Generate dynamic HTML
	util::TStringList genres;
	searchCount = getSearchHTML(searchHTML, filter, "", domain, media, max, sortByYear, genres);

	return searchCount;
}

size_t TLibrary::getSearchHTML(util::TStringList& html, const std::string& filter, const std::string& genre, const EFilterDomain domain, const EMediaType media, const size_t max, const bool sortByYear, util::TStringList& genres) {

	// Changed data requested
	bool overflow = false;
	size_t albums = 0;
	util::TStringList searchList;
	searchList.assign(filter, ' ');
	html.clear();

	// Return empty HTML on empty filter string
	if (filter.size() < 3) {
		searchCount = albums;
		return searchCount;
	}

	// Generate dynamic HTML
	if (!library.ordered.empty()) {
		html.add("<!-- SOT -->");
		html.add("  <div class=\"row\">");
		html.add("");
		bool ok = true;
		TConstAlbumList list;
		TAlbumMap::const_iterator album = library.ordered.begin();
		while (album != library.ordered.end()) {

			// Apply filter word list...
			ok = filterSearchAlbum(album->second, domain, media, searchList, genre);

			// Add album to list
			if (ok) {
				list.push_back(&album->second);

				// Check if genre in result set
				const std::string& genre = album->second.genre;
				if (!genre.empty() && std::string::npos == genres.find(genre, util::EC_COMPARE_FULL)) {
					genres.add(genre);
				}

				++albums;
			}

			++album;
			if (albums >= max) {
				overflow = true;
				break;
			}
		}

		// Sort albums by year and add to HTML
		albums = addSortedAlbums(html, list, sortByYear);

		// Close HTML
		html.add("  </div>");
		if (overflow) {
			html.add("  <hr />");
			html.add("  <div>");
			html.add("    <h2><i>Too many search results, please adjust search pattern...</i></h2>");
			html.add("  </div>");
		}
		html.add("");
		html.add("<!-- EOT -->");
		if (debug) {
			html.saveToFile("search.html");
		}
	}

	genres.sort(util::SO_ASC);
	return albums;
}


size_t TLibrary::getRecentHTML(util::TStringList& html, const size_t max) {

	// Changed data requested
	size_t albums = 0;
	html.clear();

	// Generate dynamic HTML
	if (!library.ordered.empty()) {
		html.add("<!-- SOT -->");
		html.add("  <div class=\"row\">");
		html.add("");

		// Add albums from ordered recent list
		size_t i = 0;
		TConstAlbumList list;
		while (i < library.recent.size() && i < max) {
			PAlbum o = library.recent[i];
			if (util::assigned(o)) {
				list.push_back(o);
				++albums;
			}
			++i;
		}

		// Sort albums by year and add to HTML
		albums = addSortedAlbums(html, list, false);

		// Close HTML
		html.add("");
		html.add("  </div>");
		html.add("<!-- EOT -->");
		if (debug) {
			html.saveToFile("recent.html");
		}
	}

	return albums;
}


size_t TLibrary::addSortedAlbums(util::TStringList& html, TConstAlbumList& albums, bool sortByYear) {
	size_t entries = 0;
	if (albums.size() > 0) {
		bool ok;
		const TAlbum* album;
		std::string icon, year;
		if (sortByYear)
			std::sort(albums.begin(), albums.end(), yearSorterDesc);
		for (size_t i=0; i<albums.size(); ++i) {
			album = albums[i];

			// Get media type from first song of album
			ok = false;
			if (!album->songs.empty()) {
				PSong o = album->songs[0];
				if (util::assigned(o)) {
					icon = o->getIcon();
					year = o->getYear();
					ok = true;
				}
			}
			if (ok && icon.empty()) {
				icon = "cd-audio";
			}

			if (ok) {
				size_t songs = album->songs.size();
				const std::string& artistName = album->artist;
				const std::string& albumName = album->name;
				const std::string& displayAlbumName = album->displayname;
				const std::string& albumHash = album->hash;
				addAlbumHTML(html, artistName, albumName, displayAlbumName, albumHash, year, icon, songs, entries > LAZY_LOADER_THRESHOLD_LOW);
				++entries;
			}
		}
	}
	return entries;
}


bool TLibrary::filterCheckDomain(const EFilterDomain domain, const EFilterDomain value) {
	return ((FD_ALL == domain) || (value == domain));
}

bool TLibrary::filterSearchAlbum(const TAlbum& album, const EFilterDomain domain, const EMediaType media, const util::TStringList& search, const std::string& genre) {
	bool found = false;
	size_t count = 0;

	// Check album media type
	if (media != EMT_ALL) {
		if (!album.songs.empty()) {
			PSong o = album.songs[0];
			if (util::assigned(o)) {
				if (o->getMediaType() != media)
					return false;
			}
		}
	}

	// Walk through search list...
	if (!search.empty()) {
		const std::string& artistName = album.artist;
		const std::string& albumName = album.name;
		bool compilation = album.compilation;

		for (size_t i=0; i<search.size(); ++i) {

			// Process current filter string
			const std::string& filter = search[i];
			found = false;

			// Check artist name
			if (!found && filterCheckDomain(domain, FD_ARTIST)) {
				found = util::strcasestr(artistName, filter);
				if (!found) {
					// Artist name is album artist
					// --> Take artist from first song
					PSong o = album.songs[0];
					if (util::assigned(o)) {
						if (compilation)
							found = util::strcasestr(o->getOriginalArtist(), filter);
						else
							found = util::strcasestr(o->getArtist(), filter);
					}
				}
			}

			// Check band/orchestra name
			if (!found && filterCheckDomain(domain, FD_ALBUMARTIST)) {
				// Artist name is album artist
				found = util::strcasestr(artistName, filter);
			}

			// Check conductor name
			if (!found && filterCheckDomain(domain, FD_CONDUCTOR)) {
				if (!album.songs.empty()) {
					PSong o = album.songs[0];
					if (util::assigned(o)) {
						found = util::strcasestr(o->getConductor(), filter);
					}
				}
			}

			// Check composer name per song
			if (!found && filterCheckDomain(domain, FD_COMPOSER)) {
				if (!album.songs.empty()) {
					for (size_t j=0; j<album.songs.size(); ++j) {
						PSong o = album.songs[j];
						if (util::assigned(o)) {
							found = util::strcasestr(o->getComposer(), filter);
							if (found)
								break;
						}
					}
				}
			}

			// Check album name
			if (!found && filterCheckDomain(domain, FD_ALBUM)) {
				found = util::strcasestr(albumName, filter);
			}

			// Check album year
			if (!found && filterCheckDomain(domain, FD_ALL)) {
				if (!album.songs.empty()) {
					PSong o = album.songs[0];
					if (util::assigned(o)) {
						found = util::strcasestr(o->getYear(), filter);
					}
				}
			}

			// Check album codec type
			if (!found && filterCheckDomain(domain, FD_ALL)) {
				if (!album.songs.empty()) {
					PSong o = album.songs[0];
					if (util::assigned(o)) {
						found = util::strcasestr(o->getCodec(), filter);
					}
				}
			}

			// Check all song titles and song artist for compilation albums
			if (!found) {
				if (!album.songs.empty()) {
					for (size_t j=0; j<album.songs.size(); ++j) {
						PSong o = album.songs[j];
						if (util::assigned(o)) {
							if (filterCheckDomain(domain, FD_ALBUM))
								found = util::strcasestr(o->getTitle(), filter);
							if (!found && compilation && filterCheckDomain(domain, FD_ARTIST)) {
								found = util::strcasestr(o->getOriginalArtist(), filter);
								if (!found)
									found = util::strcasestr(o->getOriginalAlbumArtist(), filter);
							}
							if (!found && compilation && filterCheckDomain(domain, FD_ALBUMARTIST)) {
								found = util::strcasestr(o->getOriginalAlbumArtist(), filter);
							}
							if (!found && filterCheckDomain(domain, FD_CONDUCTOR)) {
								found = util::strcasestr(o->getConductor(), filter);
							}
							// Composer was checked before on all songs of album!
							if (found)
								break;
						}
					}
				}
			}

			// Check album genre after album fits all other criteria...
			if (found && !genre.empty()) {
				if (!album.songs.empty()) {
					PSong o = album.songs[0];
					if (util::assigned(o)) {
						if (0 != util::strcasecmp(o->getGenre(), genre))
							found = false;
					}
				}
			}

			if (found) {
				++count;
			}

		}

		// All words found in album
		if (count >= search.size()) {
			return true;
		}

	}

	return false;
}


void TLibrary::addAlbumHTML(util::TStringList& html, const std::string& artistName, const std::string& albumName, const std::string& displayAlbumName, const std::string& albumHash, const std::string& year, const std::string& icon, const size_t songs, const bool lazy) {
	std::string value = util::TURL::encode(albumName);
	std::string search = getSearchAlbum(artistName, albumName);
	std::string link = root + LIBRARY_ROOT_URL "tracks.html?prepare=yes&title=tracks&filter=" + albumHash;
	std::string text = (songs == 1) ? " song" : " songs";

	html.add("  <div class=\"col-xs-5 col-sm-4 col-md-3\">");
	html.add("    <div class=\"thumbnail\">");

	// Lazy load thumbnails when threshold reached
	if (lazy) {
		html.add("      <img style=\"box-shadow: 6px 6px 8px gray; border-radius: " + std::string(IMAGE_BORDER_RADIUS) + ";\"  class=\"lazy-loader\" src=\"/images/rectangular200.png\" data-src=\"/rest/thumbnails/" + albumHash + "-200.jpg\" destination=\"" + link + "\" onclick=\"onCoverClick(event)\" alt=\"Thumnail Album request\" data-toggle=\"tooltip\" data-placement=\"bottom\" title=\"Show tracks for &quot;" + displayAlbumName + "&quot;\">");
	} else {
		html.add("      <img style=\"box-shadow: 6px 6px 8px gray; border-radius: " + std::string(IMAGE_BORDER_RADIUS) + ";\" src=\"/rest/thumbnails/" + albumHash + "-200.jpg\" destination=\"" + link + "\" onclick=\"onCoverClick(event)\" alt=\"Thumbnail requested...\" data-toggle=\"tooltip\" data-placement=\"bottom\" title=\"Show tracks for &quot;" + displayAlbumName + "&quot;\">");
	}

	html.add("      <div class=\"caption\">");
	html.add("        <span>");
	html.add("          <h4 class=\"text-ellipsis thumbnail-header\">" + displayAlbumName + "</h4>");
	html.add("        </span>");
	html.add("        <p>");
	html.add("          <span>" + std::to_string((size_u)songs) + text + "</span>");
	html.add("          <span class=\"pull-right\">(" + year + ")</span>");
	html.add("        </p>");
	html.add("        <div class=\"text-center-xxs\">");
	html.add("          <div class=\"btn-group pull-left-xxs\" role=\"group\">");
	html.add("            <button id=\"btnAdd\" name=\"Check\" value=\"ADDALBUM\" album=\"" + value + "\" tvname=\"" + albumName + "\" hash=\"" + albumHash + "\" onclick=\"onAddAlbumClick(event)\" class=\"btn btn-responsive-md btn-default\" data-toggle=\"tooltip\" title=\"Add album &quot;" + displayAlbumName + "&quot; to current playlist\">");
	html.add("              <span class=\"glyphicon glyphicon-file\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
	html.add("            </button>");
	html.add("            <button id=\"btnPlay\" name=\"Check\" value=\"ADDPLAYALBUM\" album=\"" + value + "\" tvname=\"" + albumName + "\" hash=\"" + albumHash + "\" onclick=\"onPlayAlbumClick(event)\" class=\"btn btn-responsive-md btn-default\" data-toggle=\"tooltip\" title=\"Play songs of album &quot;" + displayAlbumName + "&quot; now\">");
	html.add("              <span class=\"glyphicon glyphicon-play\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
	html.add("            </button>");
	html.add("          </div>");
	html.add("          <img style=\"height: 32px; border: 0 none; box-shadow: none;\" id=\"img-media\" class=\"img-thumbnail text-center-xxs hidden-xxs\" src=\"/rest/icons/" + icon + ".jpg\" alt=\"" + icon + "\">");
	html.add("          <a href=\"" + link + "\" class=\"btn btn-responsive-md btn-default pull-right\" role=\"button\" aria-disabled=\"true\" data-toggle=\"tooltip\" title=\"Show tracks for album &quot;" + displayAlbumName + "&quot;\">");
	html.add("            <span class=\"glyphicon glyphicon-cd\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>");
	html.add("          </a>");
	html.add("        </div>");
	html.add("      </div>");
	html.add("    </div>");
	html.add("  </div>");
	html.add("");
}

std::string TLibrary::getSearchAlbum(const std::string& artistName, const std::string& albumName) {
	std::string search;
	std::string album;
	size_t p1 = albumName.find_first_of('[');
	if (p1 != std::string::npos && p1 == 0) {
		size_t p2 = albumName.find_first_of(']');
		if (p2 != std::string::npos) {
			album = util::trim(albumName.substr(p2+1, std::string::npos));
		}
	}
	if (!album.empty()) {
		search = artistName + " " + album;
	} else {
		search = artistName + " " + albumName;
	}
	return util::TURL::encode(search);
}

std::string TLibrary::getHeadLine(const char delimiter) {
	std::string d(&delimiter, 1);
	std::string r;
	r.reserve(280);

	// Tag meta data
	r += "Codec" + d;
	r += "Artist" + d;
	r += "OriginalArtist" + d;
	r += "AlbumArtist" + d;
	r += "Album" + d;
	r += "Title" + d;
	r += "Genre" + d;
	r += "Composer" + d;
	// r += "Comment" + d;
	r += "Conductor" + d;
	r += "Description" + d;
	r += "Titlehash" + d;
	r += "Albumhash" + d;

	// Track meta data
	r += "Track" + d;
	r += "Disk" + d;
	r += "Year" + d;
	r += "Date" + d;

	// Stream data
	r += "Samplecount" + d;
	r += "Samplesize" + d;
	r += "Samplerate" + d;
	r += "BitsPerSample" + d;
	r += "BytesPerSample" + d;
	r += "Channels" + d;
	r += "Bitrate" + d;
	r += "Chunksize" + d;
	r += "Duration" + d;
	r += "Seconds" + d;
	r += "Time" + d;

	// File properties
	r += "URL" + d;
	r += "Filetime" + d;
	r += "Filesize" + d;
	r += "Filehash" + d;

	// Scanner config params
	r += "Configuration" + d;

	// Scanner config params
	r += "Inserttime";

	return r;
}


void TLibrary::cleanup() {
	thread.run();
}

int TLibrary::unlink() {
	std::lock_guard<std::mutex> lock(threadMtx);
	util::TFolderList content;
	std::string folder = util::filePath(database);
	std::string basename = util::fileBaseName(database);
	return content.deleteOldest(folder + basename + ".*.bak", 5);
}


void TLibrary::unlinkThreadMethod(app::TDetachedThread& thread) {
	unlink();
}


void TLibrary::setErrorFileName(const std::string& fileName) {
	std::string value;
	if (!fileName.empty())
		value = fileName;
	if (value.empty())
		value = database;
	if (!value.empty()) {
		std::string path = util::filePath(fileName);
		errorFile = path + "missing.txt";
	}
}

std::string TLibrary::getErrorFileName(const std::string& fileName) {
	if (errorFile.empty())
		setErrorFileName(fileName);
	return errorFile;
}


void TLibrary::saveToFile(const bool addHeader, const char delimiter) {
	if (!database.empty())
		saveToFile(database, addHeader, delimiter);
}

void TLibrary::saveToFile(const std::string& fileName, const bool addHeader, const char delimiter) {
	std::lock_guard<std::mutex> lock(saveMtx);
	if (!empty()) {
		util::TStringList list;
		if (addHeader)
			list.add(getHeadLine(delimiter));
		PSong o;
		for (size_t i=0; i<size(); ++i) {
			o = library.tracks.songs[i];
			list.add(o->text(delimiter));
		}
		std::string backupName = util::fileReplaceExt(fileName, "bak");
		util::moveFile(fileName, util::uniqueFileName(backupName, util::UN_TIME));
		list.saveToFile(fileName);
		if (util::fileExists(fileName)) {
			cleanup();
		}
	} else {
		// Delete library file on empty song database
		if (util::fileExists(fileName)) {
			std::string backupName = util::fileReplaceExt(fileName, "bak");
			util::moveFile(fileName, util::uniqueFileName(backupName, util::UN_TIME));
			util::deleteFile(fileName);
		}
		errorList.clear();
	}

	// Write new erroneous item file or delete existing file
	setErrorFileName(fileName);
	if (!errorList.empty()) {
		saveErrorsToFile(fileName);
	} else {
		util::deleteFile(errorFile);
	}
}

void TLibrary::saveErrorsToFile(const std::string& fileName) {
	if (!errorList.empty()) {
		util::TStringList list;
		for (size_t i=0; i<errorList.size(); ++i) {
			const music::TErrorSong& song = errorList[i];
			list.add(std::to_string((size_s)(i+1)) + ";" + song.file + ";" + std::to_string((size_s)song.error) + ";" + song.text + ";" + song.hint);
		}
		list.saveToFile(errorFile);
	}
}

std::string TLibrary::escape(const std::string& text) {
	return util::TJsonValue::escape(html::THTML::encode(text));
}

std::string TLibrary::getErrorFilesAsJSON(const size_t index, const size_t count) {

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
	size_t size = errorList.size();

	// Is limit valid?
	if (errorList.empty() || index >= size)
		return util::JSON_EMPTY_TABLE;

	// Begin new JSON object
	util::TStringList json;
	json.add("{");

	// Begin new JSON array
	json.add("\"total\": " + std::to_string((size_u)size) + ",");
	json.add("\"rows\": [");

	// Calculate limits
	size_t idx = index;
	size_t end = idx + count;
	size_t last = 0;

	// Check ranges...
	if (end > size)
		end = size;
	if (end > 1)
		last = util::pred(end);

	// Write JSON entries
	for(; idx < end; ++idx) {
		const TErrorSong& song = errorList[idx];
		json.add("{");
		json.add("  \"Index\": " + std::to_string((size_u)util::succ(idx)) + ",");
		json.add("  \"Error\": \"(" + std::to_string((size_s)song.error) + ")\",");
		json.add("  \"File\": \"" + escape(song.file) + "\",");
		json.add("  \"Text\": \"" + escape(song.text) + "\",");
		json.add("  \"Hint\": \"" + escape(song.hint) + "\",");
		json.add("  \"URL\": \"" + song.url + "\"");
		json.add(idx < last ? "}," : "}");
	}

	// Close JSON array and object
	json.add("]}");

	return json.text();
}


void TLibrary::loadFromFile(const std::string& fileName, const bool hasHeader, const char delimiter) {
	if (!empty())
		clear();

	if(!util::fileExists(fileName))
		return;

	util::TStringList list;
	list.loadFromFile(fileName, app::ECodepage::CP_DEFAULT);
	setDatabase(fileName);
	cleanup();

	if(list.empty())
		return;

	// Only header row present
	if (list.size() <= 1 && hasHeader)
		return;

	// Ignore header?
	size_t start = hasHeader ? 1 : 0;

	// Load data from rows
	size_t idx;
	ECodecType type;
	std::string row;
	PSong o = nil;
	for (idx=start; idx<list.size(); idx++) {
		row = list[idx];
		type = codecFromStr(row);
		if (type != EFT_UNKNOWN) {
			switch (type) {
				case EFT_FLAC:
					o = new TFLACFile;
					break;
				case EFT_WAV:
					o = new TPCMFile;
					break;
				case EFT_DSF:
					o = new TDSFFile;
					break;
				case EFT_DFF:
					o = new TDFFFile;
					break;
				case EFT_AIFF:
					o = new TAIFFFile;
					break;
				case EFT_ALAC:
					o = new TALACFile;
					break;
#ifdef SUPPORT_AAC_FILES
				case EFT_AAC:
					o = new TALACFile(EFT_AAC);
					break;
#endif
				case EFT_MP3:
					o = new TMP3File;
					break;
				default:
					std::cout << "TPlaylist::loadFromFile() Unknown song type <" << row << ">" << std::endl;
					break;
			}
			if (util::assigned(o)) {
				PSong p = o;
				util::TObjectGuard<TSong> og(&p);
				if (o->assign(row, delimiter)) {
					addSong(o);
					p = nil; // Do not delete p via RAII guard!
				} else {
					// Return nil again!
					o = nil;
				}
			}
		}
	}

	if (library.tracks.songs.size() > 0) {
		updateLibraryMappings();
	}

	// Set erroneous file list location for given library file
	setErrorFileName(fileName);
}

ECodecType TLibrary::codecFromStr(const std::string& value) {
	if (value.empty())
		return EFT_UNKNOWN;

	char* q;
	const char *const p = value.c_str();
	errno = EXIT_SUCCESS;
	ECodecType retVal = (ECodecType)strtol(p, &q, 10);
	if (EXIT_SUCCESS != errno || p == q)
		return EFT_UNKNOWN;

#ifdef SUPPORT_AAC_FILES
	if (!util::isMemberOf(retVal, EFT_FLAC,EFT_DSF,EFT_DFF,EFT_WAV,EFT_AIFF,EFT_ALAC,EFT_AAC,EFT_MP3))
		return EFT_UNKNOWN;
#else
	if (!util::isMemberOf(retVal, EFT_FLAC,EFT_DSF,EFT_DFF,EFT_WAV,EFT_AIFF,EFT_ALAC,EFT_MP3))
		return EFT_UNKNOWN;
#endif

	return retVal;
}

std::string TLibrary::albumAsJSON(const std::string& hash, const bool extended) const {
	util::TStringList tracks;
	size_t count = 0;

	// Find album in mapped list
	THashedMap::const_iterator album = library.albums.find(hash);
	if (album != library.albums.end()) {
		count = album->second.songs.size();
	}

	// Begin new JSON object
	tracks.add("{");

	// Begin new JSON array
	tracks.add("  \"total\": " + std::to_string((size_u)count) + ",");
	tracks.add("  \"rows\": [");

	if (count > 0) {
		const_iterator it = album->second.songs.begin();
		const_iterator end = album->second.songs.end();
		const_iterator last = util::pred(album->second.songs.end());

		// Add JSON object with trailing separator
		for(; it != last; ++it) {
			tracks.add((*it)->asJSON("    ", false, "", extended));
			tracks[util::pred(tracks.size())] += ",";
		}

		// Add last object without separator
		if (it != end) {
			tracks.add((*it)->asJSON("    ", false, "", extended));
		}
	}

	// Close JSON array and object
	tracks.add("  ]");
	tracks.add("}");
	// tracks.saveToFile("tracks.json");

	return tracks.raw('\n');
}


std::string TLibrary::albumAsAudioElements(const std::string& hash, const std::string& root) const {
	util::TStringList tracks;
	size_t count = 0;
	std::string type, codec;

	// Find album in mapped list
	THashedMap::const_iterator album = library.albums.find(hash);
	if (album != library.albums.end()) {
		count = album->second.songs.size();
	}

	//
	// Example for HTML5 Audio tag
	//   <audio
	//     id="E97181EACFBA2A797E46361966BBD84E"
	//     data-media5="media-collection"
	//     title-media5="01 - Symphonie Nr.5 c-moll op.67 - I. Allegro con brio.flac"
	//     cover-media5="/fs1/data/musik/files/Ludwig van Beethoven/CD/Symphonien 5 &amp; 7 - Wiener Philharmoniker - Carlos Kleiber/folder.jpg"
	//     src="/fs0/data/musik/files/Ludwig van Beethoven/CD/Symphonien 5 &amp; 7 - Wiener Philharmoniker - Carlos Kleiber/01 - Symphonie Nr.5 c-moll op.67 - I. Allegro con brio.flac"
	//     type="audio/x-flac"
	//     preload="none"
	//  ></audio>
	//
	if (count > 0) {
		const_iterator it = album->second.songs.begin();
		const_iterator end = album->second.songs.end();

		// Add JSON object with trailing separator
		for(; it != end; ++it) {
			if (util::getAudioCodec((*it)->getFileExtension(), type, codec)) {
				// Begin new media object
				tracks.add("<audio");

				// Add audio tags
				tracks.add("id=" + util::quote((*it)->getFileHash()));
				tracks.add("data-media5=" + util::quote("media-collection"));
				tracks.add("title-media5=" + util::quote((*it)->getDisplayTitle()));
				tracks.add("cover-media5=" + util::quote("/rest/thumbnails/" + (*it)->getAlbumHash() + "-200.jpg"));
				tracks.add("src=" + util::quote(root + (*it)->getFileName()));
				if (codec.empty()) {
					tracks.add("type=" + util::quote(type));
				} else {
					tracks.add("type=" + util::quote(type + "; codecs=" + codec));
				}
				tracks.add("preload=" + util::quote("none"));

				// Close media object
				tracks.add("></audio>\n");
			}
		}
	}

	//tracks.saveToFile("audio.html");
	return tracks.raw(' ');
}


PSong TLibrary::getAlbumData(const std::string& hash, CTrackData& track, std::string& codec, std::string& format, std::string& samples, std::string& rate, size_t& tracks) {
	track.clear();
	tracks = 0;
	THashedMap::const_iterator it = library.albums.find(hash);
	if (it != library.albums.end()) {
		if (!it->second.songs.empty()) {
			PSong o = it->second.songs[0];
			if (util::assigned(o)) {
				int sampleRate = (o->getBitsPerSample() < 8) ? 16 * o->getSampleRate() : o->getSampleRate();
				track.meta = o->getMetaData();
				track.file = o->getFileData();
				track.stream = o->getStreamData();
				format = std::to_string((size_u)track.stream.bitsPerSample) + " Bit";
				if (o->getBitsPerSample() < 8)
					samples = std::to_string((size_u)sampleRate) + " Samples/sec at " + std::to_string((size_u)o->getSampleRate()) + " Hz";
				else
					samples = std::to_string((size_u)sampleRate) + " Samples/sec";
				rate = std::to_string((size_u)o->getBitRate()) + " kiBit/sec";
				codec = o->getCodec() + " - " + o->getModulation();
				tracks = it->second.songs.size();
				return o;
			}
		}
	}
	return nil;
}


bool TLibrary::getArtistInfo(const std::string& artist, std::string& displayname, std::string& hash) {
	TArtistMap::const_iterator it = library.artists.all.find(artist);
	if (it != library.artists.all.end()) {
		bool compilation = false;
		if (!it->second.albums.empty()) {
			TAlbumMap::const_iterator album = it->second.albums.begin();
			if (album->second.compilation)
				compilation = true;
		}
		displayname = compilation ? it->second.name : it->second.originalname;
		hash = it->second.hash;
		return !(hash.empty() || displayname.empty());
	}
	return false;
}


void TLibrary::onScannerCallback(const size_t count, const std::string& current) {
	if (nil != onProgressCallback) {
		try {
			onProgressCallback(*this, count, current);
		} catch (...) {}
	}
}



TPlaylist::TPlaylist() {
	hash = 0;
	owner = nil;
	debug = false;
	changed = false;
	deleted = false;
	permanent = true;
	thread.setExecHandler(&music::TPlaylist::unlinkThreadMethod, this);
	thread.setName("app::TLibrary::unlink()");
	prepare();
}

TPlaylist::~TPlaylist() {
	clear();
}

void TPlaylist::clear() {
	garbageCollector();
	util::clearObjectList(tracks);
	files.clear();
	json.clear();
	m3u.clear();
}

void TPlaylist::prepare() {
	c_deleted = 0;
	c_added = 0;
}

void TPlaylist::cleanup() {
	thread.run();
}

int TPlaylist::unlink() {
	std::lock_guard<std::mutex> lock(threadMtx);
	util::TFolderList content;
	std::string folder = util::filePath(database);
	std::string basename = util::fileBaseName(database);
	return content.deleteOldest(folder + basename + ".*.bak", 5);
}

void TPlaylist::unlinkThreadMethod(app::TDetachedThread& thread) {
	unlink();
}

bool TPlaylist::validIndex(const size_t index) const {
	return (index >= 0 && index < tracks.size());
}

void TPlaylist::setName(const std::string& value) {
	hash = util::calcHash(value);
	name = value;
	state = checkRecent();
};

bool TPlaylist::checkRecent() const {
	std::string state = STATE_PLAYLIST_NAME;
	if (name.size() == state.size()) {
		if (name == state)
			return true;
	}
	return false;
};

bool TPlaylist::isName(const std::string& value) const {
	if (name.size() == value.size()) {
		if (name == value)
			return true;
	}
	return false;
};


struct CAlbumHashDeleter
{
	CAlbumHashDeleter(const std::string& albumHash) : _albumHash(albumHash) {}
    std::string _albumHash;
    bool operator()(PSong o) const {
    	if (util::assigned(o)) {
			if (o->getAlbumHash() == _albumHash) {
				return true;
			}
    	}
    	return false;
    }
};

int TPlaylist::deleteOldest(const size_t size) {
	int deleted = 0;
	if (size > 0) {
		if (tracks.size() > size) {
			int remove = tracks.size() - size;

			// Create new list from playlist items
			PSong song;
			PTrack track;
			TSongList songs;
			for (size_t i=0; i<tracks.size(); ++i) {
				track = tracks[i];
				if (util::assigned(track)) {
					song = track->getSong();
					if (util::assigned(song)) {
						songs.push_back(song);
					}
				}
			}

			// Sort by time and delete oldest albums until size fits in given value
			std::string albumHash;
			if (!songs.empty()) {
				// Sort ASC --> oldest on top, DESC youngest on top
				std::sort(songs.begin(), songs.end(), timeSorterAsc);
				do {
					song = songs.empty() ? nil : songs[0];
					if (util::assigned(song)) {
						if (getDebug()) std::cout << "TPlaylist::deleteOldest() Oldest song = " << song->getModTime().asString() << std::endl;
						albumHash = song->getAlbumHash();
						deleted += removeAlbum(albumHash);
						songs.erase(std::remove_if(songs.begin(), songs.end(), CAlbumHashDeleter(albumHash)), songs.end());
					}
				} while (deleted < remove && !songs.empty());
			}
		}
	}
	if (deleted > 0) {
		c_deleted += deleteRemovedTracks();
	}
	return deleted;
}


void TPlaylist::rebuild() {
	if (hasOwner() && !tracks.empty()) {
		size_t deleted = 0;
		for (size_t i=0; i<tracks.size(); ++i) {
			PTrack track = tracks[i];
			if (util::assigned(track)) {
				music::PSong song = owner->getSong(track->getFile());
				if (util::assigned(song)) {
					track->setSong(song);
					track->setDeleted(false);
				} else {
					track->setSong(nil);
					track->setDeleted(true);
					++deleted;
				}
			}
		}
		if (deleted > 0) {
			c_deleted += deleteRemovedTracks(true);
		}
	}
}


void TPlaylist::remove() {
	if (hasOwner() && !tracks.empty()) {
		for (size_t i=0; i<tracks.size(); ++i) {
			PTrack track = tracks[i];
			if (util::assigned(track)) {
				track->setRemoved(true);
				garbage.push_back(track);
			}
		}
	}
}

void TPlaylist::reset() {
	if (hasOwner() && !tracks.empty()) {
		for (size_t i=0; i<tracks.size(); ++i) {
			PTrack track = tracks[i];
			if (util::assigned(track)) {
				track->setRemoved(false);
			}
		}
	}
}

void TPlaylist::release() {
	if (hasGarbage()) {
		for (size_t i=0; i<garbage.size(); ++i) {
			PTrack track = tracks[i];
			if (util::assigned(track)) {
				track->setDeferred(false);
			}
		}
	}
}


void TPlaylist::saveToFile() {
	if (!getDatabase().empty())
		saveToFile(getDatabase());
}

void TPlaylist::saveToFile(const std::string& fileName) {
	if (permanent) {
		changed = false;
		if (!empty()) {
			util::TStringList list;
			PTrack o;
			PSong p;
			std::string line;

			// Write header line
			std::string s = getName();
			if (s.empty())
				s = util::fileBaseName(fileName);
			if (!s.empty()) {
				line = "Name:" + s;
				list.add(line);
			}

			// Save lines to file
			for (size_t idx=0; idx<size(); ++idx) {
				o = tracks[idx];
				if (util::assigned(o)) {
					p = o->getSong();
					if (util::assigned(p)) {
						line = util::csnprintf("%:%:%:%", idx, p->getFileHash(), p->getFileName(), p->getModTime().asString());
						list.add(line);
					}
				}
			}

			// Save, backup and cleanup file(s)
			bool exists = false;
			if (util::fileExists(fileName)) {
				std::string backupName = util::fileReplaceExt(fileName, "bak");
				util::moveFile(fileName, util::uniqueFileName(backupName, util::UN_TIME));
				exists = true;
			}
			list.saveToFile(fileName);
			if (exists && util::fileExists(fileName)) {
				cleanup();
			}
		}
	}
}

void TPlaylist::loadFromFile() {
	if (!getDatabase().empty())
		loadFromFile(getDatabase());
}

void TPlaylist::loadFromFile(const std::string& fileName) {
	changed = false;

	if (!empty()) {
		clear();
	}

	if(!util::fileExists(fileName)) {
		setDatabase(fileName);
		return;
	}

	util::TStringList list;
	list.loadFromFile(fileName, app::ECodepage::CP_DEFAULT);
	setDatabase(fileName);
	cleanup();

	if(list.empty())
		return;

	// Only header row present
	if (list.size() <= 0)
		return;

	// Load data from rows
	PSong song;
	bool ok;
	bool modified = false;
	util::TDateTime now;
	size_t p1, p2, p3;
	std::string file, hash, date, row;
	for (size_t idx=0; idx<list.size(); idx++) {
		hash.clear();
		file.clear();
		date.clear();
		row = list[idx];
		p1 = row.find_first_of(':');
		if (p1 != std::string::npos) {

			// Check first row for playlist name entry
			if (idx == 0) {
					if (p1 > 3) {
					std::string s = row.substr(0, p1);
					if (s == "Name") {
						s = row.substr(p1+1, std::string::npos);
						setName(s);
						//std::cout << "TPlaylist::loadFromFile() Load playlist \"" << getName() << "\"" << std::endl;
						continue;
					}
				}
			}

			// 1876:9fe1db8cf34e0bc5fd9636e749d49b0a:/mnt/music/Gary Moore/Wild Frontier/01 - Over The Hills And Far Away.flac:2017-07-29 10:22:35
			//     |123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456
			//     |                                |                                                                         |
			//  p1 = 0                           p2 = p1 + 33                                                              p3 = size - 20
			p2 = p1 + 33;
			p3 = row.size() - 20;

			// Read hash value
			ok = false;
			if (row.size() > p2) {
				if (row[p2] == ':') {
					ok = true;
				}
			}
			if (ok && p2 > p1) {
				hash = row.substr(p1+1, p2-p1-1);
			}

			// Read file name
			ok = false;
			if (row.size() > 53) {
				if (row[p3] == ':') {
					ok = true;
				}
			}
			if (ok && p3 > p2) {
				file = row.substr(p2+1, p3-p2-1);
			}

			// Read timestamp
			if (ok && p3 < row.size()) {
				date = row.substr(p3+1, std::string::npos);
			}

			// Add entry to playlist
			song = addTrack(hash, EPA_APPEND, true);

			// Set timestamp
			if (util::assigned(song)) {
				if (!date.empty()) {
					if (getDebug()) std::cout << "TPlaylist::loadFromFile() File = " << file << " Hash = " << hash << " Date = " << date << std::endl;
					song->setModTime(date);
					if (getDebug()) std::cout << "TPlaylist::loadFromFile() Date = " << song->getModTime().asString() << std::endl;
				} else {
					if (getDebug()) std::cout << "TPlaylist::loadFromFile() File = " << file << " Hash = " << hash << " (date missing)" << std::endl;
					song->setModTime(now);
					if (getDebug()) std::cout << "TPlaylist::loadFromFile() Date = " << song->getModTime().asString() << std::endl;
					modified = true;
				}
			}
		}
	}

	// Get name from filename
	std::string s = getName();
	if (s.empty())
		s = util::fileBaseName(fileName);
	if (!s.empty()) {
		setName(s);
	}

	// List was modified...
	changed = modified;
}


int TPlaylist::importFromMPD(const std::string& fileName, const std::string& root) {
	changed = false;

	if (!empty())
		clear();

	// Check prerequsites
	if(fileName.empty() || root.empty())
		return 0;

	if (!util::fileExists(fileName))
		return 0;

	if (!util::folderExists(root))
		return 0;

	// Load MPD playlist (state file)
	util::TStringList list;
	list.loadFromFile(fileName);

	// Nothing to do?
	if (list.empty())
		return 0;

	//
	// Iterate through playlist
	// Parse entry like:
	// 2838:Explosions in the Sky/All of a Sudden I Miss Everyone/01 - The Birth and Death of the Day.flac
	//
	util::TStringList parser;
	std::string csv, name, file, path = util::validPath(root);
	for (size_t i=0; i<list.size(); ++i) {
		csv = list[i];
		if (!csv.empty()) {
			parser.split(csv, ':');
			if (parser.size() > 1) {
				name = parser[1];
				if (!name.empty()) {
					file = path + name;
					addFile(file, EPA_APPEND, true);
				}
			}
		}
	}

	changed = false;
	return tracks.size();
}


PSong TPlaylist::addFile(const std::string& fileName, const EPlayListAction action, bool rebuild) {
	PSong o = nil;
	if (!fileName.empty() && hasOwner()) {
		o = owner->findFileName(fileName);
		addSong(o, action, rebuild);
	}
	return o;
}

PSong TPlaylist::addTrack(const std::string& fileHash, const EPlayListAction action, bool rebuild) {
	PSong o = nil;
	if (!fileHash.empty() && hasOwner()) {
		o = owner->findFile(fileHash);
		addSong(o, action, rebuild);
	}
	return o;
}

void TPlaylist::addSong(PSong song, const EPlayListAction action, bool rebuild) {
	if (util::assigned(song)) {

		// Add new track to list
		PTrack o = new TTrack;
		o->setSong(song);
		o->setHash(hash);
		o->setFile(song->getFileHash());
		switch (action) {
			case EPA_INSERT:
				tracks.insert(tracks.begin(), o);
				if (rebuild)
					reindex();
				break;
			case EPA_APPEND:
			default:
				if (rebuild)
					o->setIndex(tracks.size());
				tracks.push_back(o);
				break;
		}

		// Add track to map hashed by file
		files[song->getFileHash()] = o;
		changed = true;
		++c_added;

	}
}

bool TPlaylist::removeFile(const std::string& fileHash) {
	// Remove means to mark item as deleted
	TTrackMap::iterator it = files.find(fileHash);
	if (it != files.end()) {
		PTrack o = it->second;
		if (util::assigned(o)) {
			o->setDeleted(true);
			return true;
		}
	}
	return false;
}

bool TPlaylist::deleteFile(const std::string& fileHash) {
	// Delete means to move item to garbage list
	if (removeFile(fileHash)) {
		deleteRemovedTracks();
		return true;
	}
	return false;
}

bool TPlaylist::removeSong(PSong song) {
	// Remove means to mark item as deleted
	if (util::assigned(song)) {
		TTrackMap::iterator it = files.find(song->getFileHash());
		if (it != files.end()) {
			PTrack o = it->second;
			if (util::assigned(o)) {
				o->setDeleted(true);
				return true;
			}
		}
	}
	return false;
}

bool TPlaylist::deleteSong(PSong song) {
	// Delete means to move item to garbage list
	if (removeSong(song)) {
		deleteRemovedTracks();
		return true;
	}
	return false;
}

bool TPlaylist::removeTrack(PTrack track) {
	// Remove means to mark item as deleted
	if (util::assigned(track)) {
		track->setDeleted(true);
		return true;
	}
	return false;
}

bool TPlaylist::deleteTrack(PTrack track) {
	// Delete means to move item to garbage list
	if (removeTrack(track)) {
		deleteRemovedTracks();
		return true;
	}
	return false;
}

void TPlaylist::commit() {
	int deleted = 0;
	if (c_deleted > 0) {
		deleted = deleteRemovedTracks(false);
	}
	if (deleted > 0 || c_added > 0) {
		reindex();
	}
	prepare();
}


struct CTrackDeleter
{
	CTrackDeleter(TTrackList* deleted) : _deleted(deleted) {}
	TTrackList* _deleted;
	bool operator() (PTrack track) const {
		if (util::assigned(track)) {
			if (track->isDeleted()) {
				if (util::assigned(_deleted)) {
					track->setRemoved(true);
					_deleted->push_back(track);
				} else {
					util::freeAndNil(track);
				}
				return true;
			}
		}
		return false;
	}
};

int TPlaylist::deleteRemovedTracks(const bool rebuild) {
	int deleted = deleteRemovedFiles();
	if (deleted > 0) {
		tracks.erase(std::remove_if(tracks.begin(), tracks.end(), CTrackDeleter(&garbage)), tracks.end());
		changed = true;
		if (rebuild)
			reindex();
	}
	return deleted;
}

int TPlaylist::deleteRemovedFiles() {
	int deleted = 0;
	if (!files.empty()) {
		PTrack track;
		TTrackMap::iterator it = files.begin();
		while (it != files.end()) {
			track = it->second;
			if (util::assigned(track)) {
				if (track->isDeleted()) {
					it = files.erase(it);
					++deleted;
					continue;
				}
			}
			++it;
		}
	}
	return deleted;
}


struct CGarbageDeleter
{
	CGarbageDeleter() {}
    bool operator() (PTrack track) const {
    	if (util::assigned(track)) {
			if (!track->isDeferred()) {
				util::freeAndNil(track);
				return true;
			}
    	}
    	return false;
    }
};

int TPlaylist::garbageCollector() {
	int size = garbage.size();
	if (hasGarbage())
		garbage.erase(std::remove_if(garbage.begin(), garbage.end(), CGarbageDeleter()), garbage.end());
	return size - garbage.size();
}


int TPlaylist::removeAlbum(const std::string& albumHash) {
	int deleted = 0;
	if (!albumHash.empty()) {
		PTrack track;
		PSong song;
		for (size_t i=0; i<tracks.size(); ++i) {
			track = tracks[i];
			if (util::assigned(track)) {
				song = track->getSong();
				if (util::assigned(song)) {
					if (song->getAlbumHash() == albumHash) {
						++deleted;
						track->setDeleted(true);
						if (getDebug()) std::cout << "TPlaylist::removeAlbum() Removed song(" << i << ") \"" << song->getTitle() << "\"" << std::endl;
					}
				}
			}
		}
	}
	c_deleted += deleted;
	return deleted;
}


int TPlaylist::deleteAlbum(const std::string& albumHash) {
	int deleted = removeAlbum(albumHash);
	if (deleted > 0) {
		deleteRemovedTracks();
	}
	return deleted;
}


int TPlaylist::addAlbum(const std::string& albumHash, const EPlayListAction action, bool rebuild) {
	int inserted = 0;
	if (hasOwner() && !albumHash.empty()) {
		// Find album hash in owner album map
		const THashedMap& albums = owner->getAlbums();
		THashedConstIterator it = albums.find(albumHash);
		if (it != albums.end()) {
			if (!it->second.songs.empty()) {
				util::TDateTime now;
				PSong song;
				switch (action) {
					case EPA_INSERT:
						for (ssize_t i=util::pred(it->second.songs.size()); i>=0; --i) {
							song = it->second.songs[i];
							if (util::assigned(song)) {
								song->setModTime(now);
								addSong(song, EPA_INSERT, false);
								if (getDebug()) std::cout << "TPlaylist::addAlbum() Inserted song(" << i << ") \"" << song->getTitle() << "\"" << std::endl;
								++inserted;
							}
						}
						break;
					case EPA_APPEND:
						for (size_t i=0; i<it->second.songs.size(); ++i) {
							song = it->second.songs[i];
							if (util::assigned(song)) {
								song->setModTime(now);
								addSong(song, EPA_APPEND, false);
								if (getDebug()) std::cout << "TPlaylist::addAlbum() Appended song(" << i << ") \"" << song->getTitle() << "\"" << std::endl;
								++inserted;
							}
						}
						break;
				}
			}
		} else {
			if (getDebug()) std::cout << "TPlaylist::addAlbum() Hash <" << albumHash << "> not found." << std::endl;
		}
	}
	if (inserted > 0) {
		changed = true;
		if (rebuild)
			reindex();
	}
	return inserted;
}


int TPlaylist::reorder(const data::TTable& table) {
	int r = 0;
	if (!table.empty()) {

		// Set indices before reorganize playlist
		reindex();

		// Get all tracks to reorganize
		size_t index = 1 + size();
		TTrackList list;
		for (size_t i=0; i<table.size(); ++i) {
			std::string hash = table[i]["Filehash"].asString();
			PTrack track = getTrack(hash);
			if (util::assigned(track)) {
				// Find lowest/start index
				if (track->getIndex() < index)
					index = track->getIndex();
				list.push_back(track);
			}
		}

		// Reorder tracks by list content
		if ((table.size() == list.size()) && (size() >= list.size()) && ((index + list.size()) <= size())) {
			for (size_t i=0; i<list.size(); ++i) {
				PTrack track = list[i];
				PSong song = track->getSong();
				if (util::assigned(song)) {
					size_t idx = index + i;
					if (validIndex(idx)) {
						tracks[idx] = track;
						track->setIndex(idx);
						++r;
					}
				}
			}
		}

	}

	if (!changed)
		changed = r > 0;

	return r;
}


PTrack TPlaylist::findTrack(const std::string& fileHash) const {
	if (!fileHash.empty()) {
		if (!files.empty()) {
			TTrackMap::const_iterator it = files.find(fileHash);
			if (it != files.end())
				return it->second;
		}
	}
	return nil;
}

PSong TPlaylist::findSong(const std::string& fileHash) const {
	PTrack o = findTrack(fileHash);
	if (util::assigned(o))
		return o->getSong();
	return nil;
}

PSong TPlaylist::findAlbum(const std::string& albumHash) const {
	if (!albumHash.empty()) {
		if (!tracks.empty()) {
			PSong song;
			PTrack track;
			for (size_t i=0; i<tracks.size(); ++i) {
				track = tracks[i];
				if (util::assigned(track)) {
					song = track->getSong();
					if (util::assigned(song)) {
						if (song->getAlbumHash() == albumHash)
							return song;
					}
				}
			}
		}
	}
	return nil;
}

bool TPlaylist::findAlbumRange(const std::string& albumHash, size_t& first, size_t& last, size_t& size) const {
	bool found = false;
	first = app::nsizet;
	last = app::nsizet;
	size = 0;
	if (!albumHash.empty()) {
		if (!tracks.empty()) {
			PSong song;
			PTrack track;
			for (size_t i=0; i<tracks.size(); ++i) {
				track = tracks[i];
				if (util::assigned(track)) {
					song = track->getSong();
					if (util::assigned(song)) {
						if (song->getAlbumHash() == albumHash) {
							if (!found) {
								found = true;
								first = i;
							}
							++size;
						} else {
							if (found) {
								last = util::pred(i);
								break;
							}
						}
					}
				}
			}
		}
	}
	if (found && last == app::nsizet) {
		last = util::pred(tracks.size());
	}
	return found;
}

int TPlaylist::touchAlbum(const std::string& albumHash) {
	int touched = 0;
	if (hasOwner() && !albumHash.empty()) {
		// Find album hash in owner album map
		const THashedMap& albums = owner->getAlbums();
		THashedConstIterator it = albums.find(albumHash);
		if (it != albums.end()) {
			if (!it->second.songs.empty()) {
				util::TDateTime now;
				for (size_t i=0; i<it->second.songs.size(); ++i) {
					PSong o = it->second.songs[i];
					if (util::assigned(o)) {
						o->setModTime(now);
						if (getDebug()) std::cout << "TPlaylist::touchAlbum() Touched song(" << i << ") \"" << o->getTitle() << "\"" << std::endl;
						++touched;
					}
				}
			}
		} else
			if (getDebug()) std::cout << "TPlaylist::touchAlbum() Hash <" << albumHash << "> not found." << std::endl;
	}
	if (touched > 0)
		changed = true;
	return touched;
}

bool TPlaylist::isFirstAlbum(const std::string& albumHash) {
	if (!tracks.empty() && !albumHash.empty()) {
		PTrack track = tracks[0];
		if (util::assigned(track)) {
			PSong song = track->getSong();
			if (util::assigned(song)) {
				if (albumHash == song->getAlbumHash())
					return true;
			}
		}
	}
	return false;
}


PSong TPlaylist::getSong(const std::size_t index) const {
	if (validIndex(index)) {
		PTrack o = tracks.at(index);
		if (util::assigned(o))
			return o->getSong();
	}
	return nil;
};

PSong TPlaylist::getSong(const std::string& fileHash) const {
	return findSong(fileHash);
}

PTrack TPlaylist::getTrack(const std::size_t index) const {
	if (validIndex(index))
		return tracks.at(index);
	return nil;
};

PTrack TPlaylist::getTrack(const std::string& fileHash) const {
	return findTrack(fileHash);
}

PTrack TPlaylist::operator[] (const std::size_t index) const {
	return getTrack(index);
};

PTrack TPlaylist::operator[] (const std::string& fileHash) const {
	return getTrack(fileHash);
};


PSong TPlaylist::nextSong(const TTrack* track) const {
	if (util::assigned(track)) {
		size_t idx = track->getIndex();
		if (std::string::npos != idx) {
			++idx;
			return getSong(idx);
		}
	}
	return nil;
}

PSong TPlaylist::nextSong(const TSong* song) const {
	if (util::assigned(song)) {
		PTrack track = findTrack(song->getFileHash());
		return nextSong(track);
	}
	return nil;
}

PTrack TPlaylist::nextTrack(const TTrack* track) const {
	if (util::assigned(track)) {
		size_t idx = track->getIndex();
		if (std::string::npos != idx) {
			++idx;
			return getTrack(idx);
		}
	}
	return nil;
}

PTrack TPlaylist::nextTrack(const TSong* song) const {
	if (util::assigned(song)) {
		PTrack track = findTrack(song->getFileHash());
		return nextTrack(track);
	}
	return nil;
}


void TPlaylist::reindex() {
	PTrack o;
	for (size_t i=0; i<tracks.size(); ++i) {
		o = tracks[i];
		if (util::assigned(o)) {
			o->setIndex(i);
			o->setDeleted(false);
		}
	}
}


size_t TPlaylist::clearRandomMarkers() {
	size_t r = 0;
	for (size_t i=0; i<tracks.size(); ++i) {
		PTrack o = tracks[i];
		if (util::assigned(o)) {
			if (o->isRandomized()) {
				o->setRandomized(false);
				++r;
			}
		}
	}
	return r;
}

size_t TPlaylist::songsToShuffleLeft(const std::string& albumHash) const {
	size_t r = 0;
	if (!albumHash.empty()) {
		if (!tracks.empty()) {
			PSong song;
			PTrack track;
			for (size_t i=0; i<tracks.size(); ++i) {
				track = tracks[i];
				if (util::assigned(track)) {
					song = track->getSong();
					if (util::assigned(song)) {
						if (song->getAlbumHash() == albumHash) {
							if (!track->isRandomized())
								++r;
						}
					}
				}
			}
		}
	}
	return r;
}

size_t TPlaylist::songsToShuffleLeft() const {
	size_t r = 0;
	if (!tracks.empty()) {
		PTrack track;
		for (size_t i=0; i<tracks.size(); ++i) {
			track = tracks[i];
			if (util::assigned(track)) {
				if (!track->isRandomized())
					++r;
			}
		}
	}
	return r;
}


util::TStringList& TPlaylist::asJSON(size_t limit, size_t offset, const std::string& filter, EFilterType type, const std::string& active, const bool extended) const {
	if (filter.empty())
		return asPlainJSON(limit, offset, active, extended);
	return asFilteredJSON(limit, offset, filter, type, active, extended);
}


util::TStringList& TPlaylist::asFilteredJSON(size_t limit, size_t offset, const std::string& filter, EFilterType type, const std::string& active, const bool extended) const {
	if (!json.empty())
		json.clear();

	// Support for bootstrap-table server side pagination
	// See: https://github.com/wenzhixin/bootstrap-table/issues/617
	// Example:
	//	{
	//	  "total": 367,
	//	  "rows": [
	//	    {
	//	      "Artist": "Tangerine Dream",
	//	      "Albumartist": "Tangerine Dream",
	//	      "Album": "[DVD48] Electric Mandarine Tour",
	//	      ...
	//	      "Filetime": "2014-12-25 09:49:28",
	//	      "Filesize": 84881799,
	//	      "Filehash": "1735dc24518f51c0a161aa3bfaf65e8b-1419497368-000084881799"
	//	    },
	//	    {
	//	      "Artist": "Tangerine Dream",
	//	      "Albumartist": "Tangerine Dream",
	//	      "Album": "[DVD48] Electric Mandarine Tour",
	//	      ...
	//	      "Filesize": 76001462,
	//	      "Filehash": "5144cd3a94aa02df2bd50bab542477b3-1419497368-000076001462"
	//	    }
	//	  ]
	//	}

	// Is limit valid?
	if (limit < 0)
		return json;

	// If limit = 0 --> return all entries!
	if (limit == 0)
		limit = size();

	// Add song objects to filtered list
	bool found = true;
	bool added = false;
	PSong song;
	TSongList query;
	const_iterator first = end();
	const_iterator last = end();
	const_iterator it1 = begin();
	const_iterator last1 = util::pred(end());
	for(; it1 != last1; ++it1) {
		song = (*it1)->getSong();
		if (util::assigned(song)) {
			if (!filter.empty()) {
				found = song->filter(filter, type);
			}
			if (found) {
				if (!added) {
					first = it1;
					added = true;
				}
				query.push_back(song);
				last = it1;
			}
		}
	}

	// Add last song object
	if (it1 != end()) {
		song = (*it1)->getSong();
		if (util::assigned(song)) {
			if (!filter.empty()) {
				found = song->filter(filter, type);
			}
			if (found) {
				query.push_back(song);
				last = it1;
			}
		}
	}

	// Check for complete album
	bool album = false;
	if (FT_ALBUM == type && first != end()) {
		song = (*first)->getSong();
		if (util::assigned(song)) {
			if (song->getTrackCount() == (int)query.size()) {
				album = true;
			}
		}
	}

	// Is song part of a valid (not part of "state") playlist
	// --> fill front and back with other songs of playlist
	if (!album && !state && query.size() < 15) {
		size_t diff = 15 - query.size();
		size_t front = diff * 10 / 30; // 1/3 of songs to front
		size_t back = diff * 20 / 30;  // 2/3 of songs at end
		size_t rest = 0;

		if (first != end() && first != begin()) {
			for (size_t i=0; i<front; ++i) {
				--first;
				if (first != begin()) {
					song = (*first)->getSong();
					query.insert(query.begin(), song);
				} else {
					song = (*first)->getSong();
					query.insert(query.begin(), song);
					if (i > 0) {
						size_t offs = i - 1;
						if (front > offs) {
							rest = front - offs;
						}
					}
					break;
				}
			}

			// Add skipped song count count at end of list
			if (rest > 0)
				back += rest;
		}

		if (last != end()) {
			for (size_t i=0; i<back; ++i) {
				++last;
				if (last != end()) {
					song = (*last)->getSong();
					query.push_back(song);
				} else {
					break;
				}
			}
		}
	}

	// Is offset valid?
	if (offset < 0 || offset >= query.size())
		return json;

	// Range check and adjustment
	if ((limit + offset) > query.size())
		limit = query.size() - offset;

	// Begin new JSON object
	json.add("{");

	// Begin new JSON array
	json.add("  \"total\": " + std::to_string((size_u)query.size()) + ",");
	json.add("  \"rows\": [");

	if (!query.empty()) {
		size_t cnt = 0;
		size_t max = util::pred(limit);
		TSongList::const_iterator it2;
		TSongList::const_iterator last2;

		// Add JSON object with trailing separator
		it2 = query.begin() + offset;
		last2 = util::pred(query.end());
		for(; it2 != last2 && cnt < max; ++it2, ++cnt) {
			song = *it2;
			bool activate = false;
			if (!active.empty()) {
				activate = song->compareByTitleHash(active);
			}
			json.add(song->asJSON("    ", activate, getName(), extended));
			json[util::pred(json.size())] += ",";
		}

		// Add last object without separator
		if (it2 != query.end() && cnt < limit) {
			song = *it2;
			bool activate = false;
			if (!active.empty()) {
				activate = song->compareByTitleHash(active);
			}
			json.add(song->asJSON("    ", activate, getName(), extended));
		}
	}

	// Close JSON array and object
	json.add("  ]");
	json.add("}");

	return json;
}


util::TStringList& TPlaylist::asPlainJSON(size_t limit, size_t offset, const std::string& active, const bool extended) const {
	if (!json.empty())
		json.clear();

	// Support for bootstrap-table server side pagination
	// See: https://github.com/wenzhixin/bootstrap-table/issues/617
	// Example:
	//      {
	//        "total": 367,
	//        "rows": [
	//          {
	//            "Artist": "Tangerine Dream",
	//            "Albumartist": "Tangerine Dream",
	//            "Album": "[DVD48] Electric Mandarine Tour",
	//            ...
	//            "Filetime": "2014-12-25 09:49:28",
	//            "Filesize": 84881799,
	//            "Filehash": "1735dc24518f51c0a161aa3bfaf65e8b-1419497368-000084881799"
	//          },
	//          {
	//            "Artist": "Tangerine Dream",
	//            "Albumartist": "Tangerine Dream",
	//            "Album": "[DVD48] Electric Mandarine Tour",
	//            ...
	//            "Filesize": 76001462,
	//            "Filehash": "5144cd3a94aa02df2bd50bab542477b3-1419497368-000076001462"
	//          }
	//        ]
	//      }

	// Is limit valid?
	if (limit < 0)
		return json;

	// Is offset valid?
	if (!validIndex(offset))
		return json;

	// If limit = 0 --> return all entries!
	if (limit == 0)
		limit = size();

	// Range check and adjustment
	if ((limit + offset) > size())
		limit = size() - offset;

	// Begin new JSON object
	json.add("{");

	// Begin new JSON array
	json.add("  \"total\": " + std::to_string((size_u)size()) + ",");
	json.add("  \"rows\": [");

	if (!empty()) {
		PSong song;
		size_t cnt = 0;
		size_t max = util::pred(limit);

		// Add JSON object with trailing separator
		const_iterator it = begin() + offset;
		const_iterator last = util::pred(end());
		for(; it != last && cnt < max; ++it, ++cnt) {
			song = (*it)->getSong();
			bool activate = false;
			if (!active.empty()) {
				activate = song->compareByTitleHash(active);
			}
			json.add(song->asJSON("    ", activate, getName(), extended));
			json[util::pred(json.size())] += ",";
		}

		// Add last object without separator
		if (it != end() && cnt < limit) {
			song = (*it)->getSong();
			bool activate = false;
			if (!active.empty()) {
				activate = song->compareByTitleHash(active);
			}
			json.add(song->asJSON("    ", activate, getName(), extended));
		}
	}

	// Close JSON array and object
	json.add("  ]");
	json.add("}");

	return json;
}


util::TStringList& TPlaylist::asM3U(const std::string& webroot, size_t limit, size_t offset) const {
	if (!m3u.empty())
		m3u.clear();

	// Is limit valid?
	if (limit < 0)
		return m3u;

	// Is offset valid?
	if (!validIndex(offset))
		return m3u;

	// If limit = 0 --> return all entries!
	if (limit == 0)
		limit = size();

	// Range check and adjustment
	if ((limit + offset) > size())
		limit = size() - offset;

	// Check protocol...
	bool isHTTP = 0 == util::strncasecmp(webroot, "http", 4);

	// Begin extended M3U playlist
	m3u.add("#EXTM3U");
	m3u.add("");
	if (!empty()) {
		// Add M3U entries
		PSong song;
		const_iterator it = begin() + offset;
		for(size_t cnt = 0; it != end() && cnt < limit; ++it, ++cnt) {
			song = (*it)->getSong();
			if (util::assigned(song))
				m3u.add(song->asM3U(webroot, isHTTP));
		}
		m3u.add("");
	}

	return m3u;
}



TPlaylists::TPlaylists() {
	prime();
}

TPlaylists::~TPlaylists() {
	destroy();
}

void TPlaylists::prime() {
	ext = "pls";
	state = STATE_PLAYLIST_NAME;
	owner = nil;
	init();
}

void TPlaylists::init() {
	nolist.clear();
	m_selected = nil;
	m_playing = nil;
	m_recent = nil;
}

void TPlaylists::destroy() {
	init();
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			util::freeAndNil(o);
			it = map.erase(it);
		}
	}
}

void TPlaylists::clear() {
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			if (util::assigned(o)) {
				o->clear();
			}
			++it;
		}
	}
}

bool TPlaylists::changed() const {
	if (!map.empty()) {
		TPlaylistMap::const_iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			if (util::assigned(o)) {
				if (o->wasChanged()) {
					return true;
				}
			}
			++it;
		}
	}
	return false;
}

size_t TPlaylists::commit(const bool force) {
	//std::cout << "TPlaylists::commit()" << std::endl;
	size_t r = 0;
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			if (util::assigned(o)) {
				if (force || o->wasChanged()) {
					//std::cout << "TPlaylists::commit() Save playlist <" << o->getDatabase() << ">" << std::endl;
					o->saveToFile();
					++r;
				}
			}
			++it;
		}
	}
	return r;
}

void TPlaylists::reload() {
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			if (util::assigned(o)) {
				o->loadFromFile();
			}
			++it;
		}
	}
}

void TPlaylists::rebuild() {
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			if (util::assigned(o)) {
				o->rebuild();
			}
			++it;
		}
	}
}

void TPlaylists::resize(const size_t size) {
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			if (util::assigned(o)) {
				o->deleteOldest(size);
				if (o->wasChanged()) {
					o->saveToFile();
				}
			}
			++it;
		}
	}
}

void TPlaylists::getNames(app::TStringVector& items) {
	items.clear();
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			if (util::assigned(o)) {
				if (o->isPermanent()) {
					const std::string& name = o->getName();
					bool ok = true;
					if (name.size() == state.size())
						ok = name != state;
					if (ok) {
						items.push_back(name);
					}
				}
			}
			++it;
		}
	}
}

const std::string& TPlaylists::getSelected() const {
	if (util::assigned(m_selected))
		return m_selected->getName();
	return nolist;
}

const std::string& TPlaylists::getPlaying() const {
	if (util::assigned(m_playing))
		return m_playing->getName();
	return nolist;
}

size_t TPlaylists::getSelectedSize() const {
	PPlaylist o = find(getSelected());
	if (util::assigned(o))
		return o->size();
	return 0;
}

size_t TPlaylists::getPlayingSize() const {
	PPlaylist o = find(getPlaying());
	if (util::assigned(o))
		return o->size();
	return 0;
}

bool TPlaylists::isValid(const std::string name) const {
	return util::assigned(find(name));
};

bool TPlaylists::isSelected(const std::string name) const {
	if (util::assigned(m_selected)) {
		if (m_selected->getName().size() == name.size()) {
			if (m_selected->getName() == name)
				return true;
		}
	}
	return false;
};

bool TPlaylists::isPlaying(const std::string name) const {
	if (util::assigned(m_playing)) {
		if (m_playing->getName().size() == name.size()) {
			if (m_playing->getName() == name)
				return true;
		}
	}
	return false;
};

bool TPlaylists::isRecent(const std::string name) const {
	if (util::assigned(m_recent)) {
		if (m_recent->getName().size() == name.size()) {
			if (m_recent->getName() == name)
				return true;
		}
	}
	return false;
};

PPlaylist TPlaylists::find(const std::string name) const {
	if (!name.empty()) {
		TPlaylistMap::const_iterator it = map.find(name);
		if (it != map.end())
			return it->second;
	}
	return nil;
}

bool TPlaylists::select(const std::string name) {
	TPlaylistMap::const_iterator it = map.find(name);
	if (it != map.end()) {
		PPlaylist o = it->second;
		if (util::assigned(o)) {
			m_selected = o;
			return true;
		}
	}
	return false;
}

bool TPlaylists::play(const std::string name) {
	TPlaylistMap::const_iterator it = map.find(name);
	if (it != map.end()) {
		PPlaylist o = it->second;
		if (util::assigned(o)) {
			m_playing = o;
			return true;
		}
	}
	return false;
}

PPlaylist TPlaylists::add(const std::string name) {
	if (!name.empty()) {
		PPlaylist o = find(name);
		if (!util::assigned(o)) {
			std::string database = root + util::sanitizeFileName(name) + "." + ext;
			o = new TPlaylist;
			o->invalidate();
			o->setName(name);
			o->setOwner(owner);
			o->setDatabase(database);
			map[o->getName()] = o;
			return o;
		}
	}
	return nil;
}

void TPlaylists::remove(const std::string name) {
	TPlaylistMap::const_iterator it = map.find(name);
	if (it != map.end()) {
		PPlaylist o = it->second;
		if (!isRecent(o->getName())) {
			util::deleteFile(o->getDatabase());
			if (isSelected(name))
				m_selected = nil;
			if (isPlaying(name))
				m_playing = nil;
			map.erase(it);
			o->remove();
			garbage.push_back(o);
		} else {
			// Clear state list only
			o->clear();
		}
	}
}

bool TPlaylists::rename(const std::string oldName, const std::string newName) {
	TPlaylistMap::const_iterator it = map.find(oldName);
	if (it != map.end()) {
		PPlaylist o = it->second;
		if (!isRecent(o->getName())) {
			std::string deleted = o->getDatabase();
			std::string database = root + util::sanitizeFileName(newName) + "." + ext;

			// Set new properties and write to file
			o->invalidate();
			o->setDatabase(database);
			o->setName(newName);
			o->saveToFile();

			// Check if current entry is selected
			bool selected = isSelected(oldName);
			bool playling = isPlaying(oldName);

			// Remove mapped entry
			map.erase(it);
			map[o->getName()] = o;

			// Select new entry
			if (selected)
				select(o->getName());
			if (playling)
				play(o->getName());

			// Success on playlist written to new filename
			bool success = util::fileExists(database);

			// Delete existing file on successful rename
			if (success && (database != deleted)) {
				util::deleteFile(deleted);
			}

			return success;
		}
	}
	return false;
}


void TPlaylists::loadFromFolder(const std::string& folder) {
	destroy();
	util::TFolderList browser;
	app::TStringVector pattern;
	pattern.push_back("*." + ext);
	root = util::validPath(folder);
	int count = browser.scan(root, pattern, util::SD_ROOT, false);
	if (count > 0) {
		for (size_t i=0; i<browser.size(); ++i) {
			util::PFile file = browser[i];
			if (file->getSize() > 0) {
				PPlaylist o = new TPlaylist;
				o->setOwner(owner);
				o->loadFromFile(file->getFile());

				// Set default name
				if (o->getName().empty()) {
					std::string name = "Playlist " + std::to_string((size_u)(i+1));
					o->setName(name);
					o->invalidate();
				} else {
					// Rename duplicates
					if (util::assigned(find(o->getName()))) {
						std::string name = "Playlist copy from '" + o->getName() + "' (Index " + std::to_string((size_u)(i+1)) + ")";
						o->setName(name);
						o->invalidate();
					}
				}

				// Add playlist
				map[o->getName()] = o;
			}
		}
	}

	// Set playlist for recently played songs
	m_recent = find(state);
	if (!util::assigned(m_recent)) {
		m_recent = add(state);
	}

}

bool TPlaylists::hasGarbage() const {
	if (!map.empty()) {
		TPlaylistMap::const_iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist pls = it->second;
			if (util::assigned(pls)) {
				if (pls->hasGarbage())
					return true;
			}
			++it;
		}
	}
	return false;
}

int TPlaylists::garbageCollector() {
	int collected = 0;
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist pls = it->second;
			if (util::assigned(pls)) {
				collected += pls->garbageCollector();
			}
			++it;
		}
	}
	return collected;
}

struct CPlaylistDeleter
{
	CPlaylistDeleter() {}
    bool operator() (PPlaylist pls) const {
    	if (util::assigned(pls)) {
    		if (pls->isDeleted()) {
    			util::freeAndNil(pls);
    			return true;
    		}
    	}
    	return false;
    }
};

int TPlaylists::cleanup() {
	int deleted = 0;
	bool marked = false;
	if (hasDeleted()) {
		size_t i,n = garbage.size();
		for (i=0; i<n; i++) {
			PPlaylist pls = garbage[i];
			if (util::assigned(pls)) {
				deleted += pls->garbageCollector();
			}
			if (pls->empty()) {
				pls->setDeleted(true);
				marked = true;
			}
		}
		if (marked)
			garbage.erase(std::remove_if(garbage.begin(), garbage.end(), CPlaylistDeleter()), garbage.end());
	}
	return deleted;
}

void TPlaylists::reset() {
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			if (util::assigned(o)) {
				o->reset();
			}
			++it;
		}
	}
}

void TPlaylists::release() {
	if (!map.empty()) {
		TPlaylistMap::iterator it = map.begin();
		while (it != map.end()) {
			PPlaylist o = it->second;
			if (util::assigned(o)) {
				o->release();
			}
			++it;
		}
	}
}

void TPlaylists::undefer() {
	if (hasDeleted()) {
		size_t i,n = garbage.size();
		for (i=0; i<n; i++) {
			PPlaylist pls = garbage[i];
			if (util::assigned(pls)) {
				pls->release();
			}
		}
	}
}

} /* namespace music */
