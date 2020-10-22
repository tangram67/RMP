/*
 * fileutils.h
 *
 *  Created on: 04.02.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef FILEUTILS_H_
#define FILEUTILS_H_

#include <map>
#include <string>
#include <vector>
#include <ctime>
#include <cstdio>
#include <sys/stat.h>
#include <mntent.h>
#include <dirent.h>
#include <fcntl.h>
#include "gcc.h"
#include "stringutils.h"
#include "fileconsts.h"
#include "semaphores.h"
#include "filetypes.h"
#include "classes.h"
#include "nullptr.h"
#include "memory.h"
#include "parser.h"
#include "gzip.h"
#include "hash.h"

namespace util {

class TFile;
class TFileList;
class TFolderList;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PFile = TFile*;
using PFileList = TFileList*;
using PFolderList = TFolderList*;
using TFileMap = std::map<std::string, util::PFile>;
using TFileMapItem = std::pair<std::string, util::PFile>;
using PFileMap = TFileMap*;
using TFolderContent = std::vector<util::PFile>;
using PFolderContent = TFolderContent*;
using TFileDecider = std::function<bool(TFile&)>;
using TFileSorter = std::function<bool(PFile, PFile)>;
using TOnChunkRead = std::function<void(void *const data, const size_t size)>;
using TOnFileFound = std::function<void(const util::TFile&)>;
using TScanMap = std::map<std::string, size_t>;
using TScanMapItem = std::pair<std::string, size_t>;

#else

typedef TFile* PFile;
typedef TFileList* PFileList;
typedef TFolderList* PFolderList;
typedef std::map<std::string, util::PFile> TFileMap;
typedef std::pair<std::string, util::PFile> TFileMapItem;
typedef TFileMap* PFileMap;
typedef std::vector<util::PFile> TFolderContent;
typedef TFolderContent* PFolderContent;
typedef std::function<bool(TFile&)> TFileDecider;
typedef std::function<bool(PFile, PFile)> TFileSorter;
typedef std::function<void(void *const data, const size_t size)> TOnChunkRead;
typedef std::function<void(const util::TFile&)> TOnFileFound;
typedef std::map<std::string, size_t> TScanMap;
typedef std::pair<std::string, size_t> TScanMapItem;

#endif

ssize_t readFile(const std::string& fileName, void *const data, const size_t size);
bool writeFile(const std::string& fileName, const std::string& text, const mode_t mode = 0644);
bool writeFile(const std::string& fileName, const void *const data, const size_t size, const mode_t mode = 0644);
bool setFileOwner(const std::string& fileName, const std::string& userName, const std::string& groupName = "");

std::string urlHostName(const std::string& url);
std::string nfsHostName(const std::string& nfs);

std::string stripFirstPathSeparator(const std::string& fileName);
std::string stripLastPathSeparator(const std::string& fileName);
std::string fileBaseName(const std::string& fileName);
std::string fileExtName(const std::string& fileName);
std::string filePath(const std::string& fileName);
std::string fileExt(const std::string& fileName);
std::string fileReplaceExt(const std::string& fileName, const std::string& ext);
std::string fileReplaceBaseName(const std::string& fileName, const std::string& name);
std::string uniqueFileName(const std::string& fileName, const EUniqueName type = UN_COUNT);
bool fileMatch(const std::string& pattern, const char* fileName, bool tolower);
bool fileMatch(const std::string& pattern, const std::string& fileName, bool tolower = true);
int fileNumber(const std::string& fileName);

bool fileStatus(const std::string& fileName, struct stat * const status);
bool folderStatus(const std::string& directoryName, struct stat * const status);
bool fileLink(const std::string& fileName, struct stat * const status);
bool fileExists(const std::string& fileName);
bool folderExists(const std::string& directoryName);
std::size_t fileSize(const std::string& fileName);
std::time_t fileAge(const std::string& fileName);
bool linkName(const std::string& fileName, std::string& linkName);

bool deleteFile(const std::string& fileName);
bool unlinkFile(const std::string& fileName);
int deleteFiles(const std::string& files, const ESearchDepth depth, const bool hidden);
int deleteFolders(const std::string& path);
int deleteFolder(const std::string& path);
bool copyFile(const std::string& oldFileName, const std::string& newFileName);
bool copyFile(const std::string& oldFileName, const std::string& newFileName, size_t& size);
bool sendFile(const std::string& oldFileName, const std::string& newFileName);
bool sendFile(const std::string& oldFileName, const std::string& newFileName, size_t& size);
bool moveFile(const std::string& oldFileName, const std::string& newFileName);

size_t maxPathSize();
void validPath(std::string& directoryName);
std::string validPath(const std::string& directoryName);
std::string sanitizeFileName(const std::string& fileName);
std::string stripPathSeparators(const std::string& fileName);

std::string currenFolder();
std::string realName(const std::string& fileName);
std::string realPath(const std::string& directoryName);
//std::string tempFileName(const std::string& fileName);

bool createDirektory(std::string directoryName, mode_t mode = 0755);
bool readDirektory(const std::string& path, app::TStringVector& content, const ESearchDepth depth = SD_ROOT, const bool hidden = false);
bool readDirektoryContent(const std::string& path, const std::string& filter, app::TStringVector& content, const ESearchDepth depth = SD_ROOT, const bool hidden = false);
bool readDirectoryTree(const std::string& path, app::TStringVector& content, const ESearchDepth depth = SD_ROOT, const bool hidden = false);

size_t getMountPoints(util::TStringList& mounts, const EMountType type = MT_DISK);
bool mount(const std::string& source, const std::string& target, const std::string& filesystem, const std::string& options, unsigned long flags = 0);
bool umount(const std::string& target, int flags = 0);

class TMount {
private:
	FILE * file;
	std::string path;
	void prime();

public:
	bool isOpen() const { return util::assigned(file); };
	std::string getPath() const { return path; };

	FILE * operator () () { return file; };

	FILE * open(const std::string& path = "/proc/mounts");
	bool read(struct mntent * entity, struct mntent * result, TBuffer& buffer);
	void close();

	TMount();
	virtual ~TMount();
};


class TDirectory {
private:
	DIR *dir;
	std::string path;
	void prime();

public:
	bool isOpen() const { return util::assigned(dir); };
	DIR * getDirectory() const { return dir; };
	std::string getPath() const { return path; };

	DIR * operator () () { return dir; };

	DIR * open(const std::string& path);
	struct dirent * read();
	void close();

	TDirectory();
	virtual ~TDirectory();
};


class TStdioFile : public app::TObject {
private:
	FILE *fh;
	std::string file;
	std::string mode;
	void prime();

public:
	bool isOpen() const { return util::assigned(fh); };
	FILE * getFile() const { return fh; };
	std::string getName() const { return file; };
	std::string getMode() const { return mode; };
	bool seek(const off_t offset, const ESeekOffset whence = SO_FROM_START) const;
	ssize_t read(void *const data, size_t const size) const;
	ssize_t write(void const *const data, size_t const size) const;
	ssize_t write(const std::string& data) const;
	bool flush() const;

	FILE * operator () () { return fh; };

	FILE * open(const std::string& fileName, const std::string& mode);
	void close();

	TStdioFile();
	virtual ~TStdioFile();
};


class TFileHandle : public app::TObject {
private:
	app::THandle fd;
	void prime();

protected:
	mutable int errval;

	void assign(const app::THandle fd) { this->fd = fd; };

	app::THandle open(const std::string& fileName, const int flags);
	app::THandle open(const char* fileName, const int flags);
	app::THandle open(const std::string& fileName, const int flags, const mode_t mode);
	app::THandle open(const char* fileName, const int flags, const mode_t mode);

	ssize_t read(void *const data, size_t const size) const;
	ssize_t write(void const *const data, size_t const size) const;

public:
	bool isOpen() const { return (fd != INVALID_HANDLE_VALUE); }
	app::THandle handle() const { return fd; };

	bool control(int flags) const;
	bool truncate(off_t size) const;
	bool chmode(mode_t mode) const;
	bool chown(uid_t owner, gid_t group) const;

	int error() const { return errval; };
	app::THandle operator () () const { return fd; };

	void close();

	TFileHandle();
	virtual ~TFileHandle();
};



class TBaseFile : public TFileHandle {
friend class TFile;

private:
	std::string file;
	util::TBuffer chunk;
	TOnChunkRead onChunkReadMethod;
	mutable bool fileExists;
	mutable bool execExists;
	void onChunkRead(void *const data, const size_t size);

	void initialize(const std::string& file);
	void finalize();
	void clear();
	void reset();
	void prime();

public:
	const std::string& getFile() const { return file; }

	void assign(const std::string& file) { initialize(file); };
	void release() { finalize(); };

	app::THandle open(const std::string& fileName, const int flags, const mode_t mode = 0644);
	app::THandle open(const int flags, const mode_t mode = 0644);
	bool create(const size_t size = 0, const mode_t mode = 0644);
	bool resize(const size_t size, const mode_t mode = 0644);

	ssize_t seek(const off_t offset, const ESeekOffset whence = SO_FROM_START) const;
	ssize_t read(void *const data, const size_t size, const off_t offset = 0, const ESeekOffset whence = SO_FROM_START) const;
	ssize_t write(const void *const data, const size_t size, const off_t offset = 0, const ESeekOffset whence = SO_FROM_START) const;
	ssize_t write(const std::string data, const off_t offset = 0, const ESeekOffset whence = SO_FROM_START) const;
	void chunkedRead(const size_t chunkSize = CHUNK_SIZE);

	bool status(struct stat * const status) const;
	bool link(struct stat * const status) const;
	bool exists() const;

	template<typename data_t, typename class_t>
		inline void bindChunkReader(data_t &&onData, class_t &&owner) {
			onChunkReadMethod = std::bind(onData, owner, std::placeholders::_1, std::placeholders::_2);
		}

	TBaseFile();
	TBaseFile(const std::string& file);
	virtual ~TBaseFile();
};


class TFile : public util::TBaseFile {
friend class TFileList;

private:
	TZLib zip;
	char* buffer;
	char* zbuffer;
	std::string base64;
	std::string html;
	size_t size;
	size_t zsize;
	util::TDateTime time;
	ino_t inode;
	bool bFolder;
	bool bFile;
	bool bExec;
	bool bCGI;
	bool bLink;
	bool bEllipsis;
	bool bHidden;
	bool bLoaded;
	bool bHTML;
	bool bCSS;
	bool bJPG;
	bool bJSON;
	bool bJava;
	bool bText;
	std::string path;
	std::string base;
	std::string ext;
	std::string root;
	std::string mime;
	mutable std::string url;  // Getter declared as const
	mutable std::string etag; // Getter declared as const
	util::hash_type fileHash;
	util::hash_type pathHash;
	util::hash_type nameHash;
	util::hash_type rootHash;
	util::hash_type urlHash;
	PFolderContent content;
	mutable PTokenParser parser; // Parser only created when needed, otherwise const!
	app::TMutex localMtx;

	bool readProperties();
	std::string makeURL() const;
	void parserNeeded(bool create = true) const;
	void setName(const std::string& name);
	void calculateHashValues();

	void initialize(const std::string& file, const std::string& root = "");
	void finalize();
	void clear();
	void reset();
	void prime();

	bool binaryLoad();
	bool minimizedLoad();

public:
	void release() { finalize(); };
	void assign(const std::string& file, const std::string& root = "");
	bool create(const size_t size = 0, const mode_t mode = 0644);
	bool load(const std::string& file, const ELoadType type = LT_BINARY);
	bool load(const ELoadType type = LT_BINARY);
	bool compress();
	bool match(const std::string& pattern);
	void deleteFileBuffer();
	void deleteZippedBuffer();

	void createParser(const std::string& startMask, const std::string& endMask);
	bool hasParser() const { return util::assigned(parser); }
	bool hasToken() const;

	bool empty() const { return 0 == getSize(); }
	bool valid() const { return !empty() && !isFolder() && exists(); }

	bool isEllipsis() const { return bEllipsis; }
	bool isFile() const { return bFile; }
	bool isExec() const { return bExec; }
	bool isCGI() const { return bCGI; }
	bool isFolder() const { return bFolder; }
	bool isHidden() const { return bHidden; }
	bool isLink() const { return bLink; }
	bool isLoaded() const { return bLoaded; }
	bool isZipped() const { return (util::assigned(zbuffer) && (zsize > 0)); }
	bool isText() const { return bText; }
	bool isHTML() const { return bHTML; }
	bool isCSS() const { return bCSS; }
	bool isJPG() const { return bJPG; }
	bool isJSON() const { return bJSON; }
	bool isJava() const { return bJava; }
	bool isMime(const std::string& mime) const;

	const char* getData() const { return buffer; }
	const size_t getSize() const { return size; }
	const char* getZippedData() const { return zbuffer; }
	const size_t getZippedSize() const { return zsize; }
	const std::string& asBase64();
	const std::string& asHTML();

	const PTokenParser getParser() const;
	const PParserBuffer getParserData();
	size_t deleteParserData() const;
	app::TMutex& getMutex() { return localMtx; };

	bool isContentEmpty() const;
	const size_t getContentSize() const;

	ino_t getInode() const { return inode; }
	const util::TDateTime& getTime() const { return time; }
	const std::string& getPath() const { return path; }
	const std::string& getBaseName() const { return base; }
	const std::string& getExtension() const { return ext; }
	const std::string& getRoot() const { return root; }
	const std::string& getMime() const { return mime; }
	const std::string& getETag() const;
	const std::string& getURL() const;
	util::hash_type getFileHash();
	util::hash_type getNameHash();
	util::hash_type getPathHash();
	util::hash_type getRootHash();
	util::hash_type getUrlHash();

	TFile();
	TFile(const std::string& file);
	TFile(const std::string& file, const std::string& root);
	virtual ~TFile();

};


class TFileList {
private:
	bool debug;
	bool load;
	ELoadType type;
	size_t fileSize;
	EFileListKey key;
	std::string root;
	TFileMap files;
	size_t zipThreshold;
	bool zipEnabled;

	void init();
	bool validIndex(const std::string::size_type idx);
	int readDirektory(const std::string& path, const ESearchDepth depth = SD_ROOT, const PFile folder = nil, const bool hidden = false);
	void debugBufferOutput(const std::string& text, const char* p, size_t size);

public:
	typedef TFileMap::const_iterator const_iterator;

	void clear();
	PFile add(const std::string& file);

	std::string::size_type size() const { return files.size(); }
	bool empty() const { return files.empty(); }
	const_iterator begin() { return files.begin(); };
	const_iterator end() { return files.end(); };

	PFileMap getFiles() { return &files; }
	size_t getFileSize() const { return fileSize; }

	void setDebug(const bool value) { debug = value; };
	void setZipEnabled(bool value) { zipEnabled = value; };
	size_t getZipEnabled() const { return zipEnabled; };
	void setZipThreshold(const size_t size) { zipThreshold = size; };
	size_t getZipThreshold() const { return zipThreshold; };

	PFile operator[] (std::string::size_type idx);
	PFile operator[] (const std::string& file);
	operator bool() const { return !empty(); };

	PFile find(const std::string& file);
	int scan(const std::string& root, const EFileListKey key = FLK_DEFAULT,
			const bool load = false, const ESearchDepth depth = SD_ROOT,
			const ELoadType type = LT_BINARY, const bool hidden = false);
	int parse(const std::string& startMask, const std::string& endMask,
			TFileDecider parseDecider = (TFileDecider)nil, TFileDecider zipDecider = (TFileDecider)nil);

	std::string trim(const std::string& file);

	TFileList();
	virtual ~TFileList();
};


class TFolderList {
private:
	std::string root;
	TFolderContent files;
	size_t fileSize;
	TOnFileFound onFileFoundMethod;

	bool validIndex(const std::string::size_type idx);
	PFile addFile(const std::string& file);
	void onFileFound(const util::TFile& file);
	int readDirektory(const std::string& path, const std::string& pattern, const ESearchDepth depth, const bool hidden = false);
	int readDirektory(const std::string& path, const app::TStringVector& pattern, const ESearchDepth depth, const bool hidden = false);
	bool match(struct dirent *file, const std::string& pattern);

public:
	typedef TFolderContent::const_iterator const_iterator;

	void clear();
	std::string::size_type size() const { return files.size(); }
	bool empty() const { return files.empty(); }
	operator bool() const { return !empty(); };
	const_iterator begin() { return files.begin(); };
	const_iterator end() { return files.end(); };

	const TFolderContent& getFiles() { return files; }
	PFile operator[] (std::string::size_type idx);
	PFile operator[] (const std::string& file);

	PFile find(const std::string& file);
	int scan(const std::string& files, const ESearchDepth depth = SD_ROOT, const bool hidden = false);
	int scan(const std::string& files, const app::TStringVector& patterns, const ESearchDepth depth = SD_ROOT, const bool hidden = false);
	int scan(const std::string& files, const std::string& pattern, const ESearchDepth depth = SD_ROOT, const bool hidden = false);
	int deleteOldest(const std::string& files, const size_t top, bool dryRun = false);
	int deleteByAge(const std::string& files, const TTimePart days, bool dryRun = false);
	void sort(TFileSorter sorter);
	void sort(ESortOrder order, TFileSorter asc, TFileSorter desc);
	void sortByMime(ESortOrder order = SO_ASC);
	void sortByName(ESortOrder order = SO_ASC);
	void sortByTime(ESortOrder order = SO_ASC);
	void debugOutput();

	template<typename data_t, typename class_t>
	inline void bindFileFound(data_t &&onFound, class_t &&owner) {
		onFileFoundMethod = std::bind(onFound, owner, std::placeholders::_1);
	}

	TFolderList();
	virtual ~TFolderList();
};


class TFileScanner {
private:
	typedef TScanMap::iterator iterator;
	
	TScanMap map;
	size_t breakOffset;
	size_t defaultSize;
	size_t bufferSize;
	size_t patternSize;
	std::string breakPattern;
	bool hasBreakPattern;
	bool hasBreakSize;
	bool isCaseSensitive;
	bool debug;

	void prime();
	void reset();
	ssize_t scanner(TFile& file, size_t offset, const app::TStringVector& pattern);
	ssize_t lookup(TBuffer& buffer, size_t size, size_t position, bool& underrun);
	bool compare(const char buffer, const char pattern) const;

public:
	typedef TScanMap::const_iterator const_iterator;

	bool getCaseSensitive() const { return isCaseSensitive; };
	void setCaseSensitive(const bool value) { isCaseSensitive = value; };
	size_t getBufferSize() const { return defaultSize; };
	void setBufferSize(const size_t value) { defaultSize = value; };

	size_t size() const { return map.size(); };
	bool empty() const { return map.empty(); };
	const_iterator begin() const { return map.begin(); };
	const_iterator end() const { return map.end(); };

	void clear();
	bool add(const std::string& pattern, size_t value);
	size_t find(const std::string& pattern);
	size_t operator[] (const std::string& pattern);

	ssize_t scan(const std::string& fileName, size_t offset, const app::TStringVector& pattern, size_t breakSize);
	ssize_t scan(const std::string& fileName, size_t offset, const app::TStringVector& pattern, const std::string& breakPattern);
	ssize_t scan(TFile& file, size_t offset, const app::TStringVector& pattern, const std::string& breakPattern);
	ssize_t scan(TFile& file, size_t offset, const app::TStringVector& pattern, size_t breakSize);

	void debugOutput(const std::string& preamble) const;

	TFileScanner();
	virtual ~TFileScanner();
};


} /* namespace util */

#endif /* FILEUTILS_H_ */
