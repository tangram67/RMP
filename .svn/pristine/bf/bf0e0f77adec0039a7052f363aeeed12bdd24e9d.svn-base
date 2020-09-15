/*
 * fileutils.cpp
 *
 *  Created on: 04.02.2015
 *      Author: Dirk Brinkmeier
 */

#include "fileutils.h"
#include "stringutils.h"
#include "exception.h"
#include "mimetypes.h"
#include "sysconsts.h"
#include "encoding.h"
#include "compare.h"
#include "convert.h"
#include "ASCII.h"
#include "ansi.h"
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <fnmatch.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace util {

/*
 * Local signal save wrapper for system calls
 */
int __s_mkdir(const std::string& dir, mode_t mode) {
	int r = EINVAL;
	if (!dir.empty()) {
		do {
			errno = EXIT_SUCCESS;
			r = ::mkdir(dir.c_str(), mode);
		} while (r == EXIT_ERROR && errno == EINTR);
	}
	return r;
}

struct dirent * __s_readdir(DIR * dir) {
	struct dirent * r;
	do {
		errno = EXIT_SUCCESS;
		r = ::readdir(dir);
	} while (!util::assigned(r) && errno == EINTR);
	return r;
}

int __s_rename(const std::string& oldFileName, const std::string& newFileName) {
	int r = EINVAL;
	if (!(oldFileName.empty() || newFileName.empty())) {
		do {
			errno = EXIT_SUCCESS;
			r = ::rename(oldFileName.c_str(), newFileName.c_str());
		} while (r == EXIT_ERROR && errno == EINTR);
	}
	return r;
}


TMount::TMount() {
	prime();
}

TMount::~TMount() {
	close();
}

void TMount::prime() {
	file = nil;
}

bool TMount::read(struct mntent * entity, struct mntent * result, TBuffer& buffer) {
	result = nil;
	if (util::assigned(file) && util::assigned(entity)) {
		struct mntent * r = getmntent_r(file, entity, buffer.data(), buffer.size());
		if (NULL != r) {
			result = r;
			return true;
		}
	}
	return false;
}

FILE * TMount::open(const std::string& path) {
	close();
	if (path.empty())
		return nil;

	FILE * fd = setmntent(path.c_str(), "r");
	if (NULL != fd)
		file = fd;
	if (isOpen())
		this->path = path;

	return file;
}

void TMount::close() {
	if (isOpen()) {
		endmntent(file);
		path.clear();
	}
	prime();
}


TDirectory::TDirectory() {
	prime();
}

TDirectory::~TDirectory() {
	close();
}

void TDirectory::prime() {
	dir = nil;
}

struct dirent * TDirectory::read() {
	if (util::assigned(dir))
		return __s_readdir(dir);
	return nil;
}

DIR * TDirectory::open(const std::string& path) {
	close();
	if (!path.empty()) {
		do {
			errno = EXIT_SUCCESS;
			dir = ::opendir(path.c_str());
		} while (!assigned(dir) && errno == EINTR);
		if (isOpen())
			this->path = path;
		return dir;
	}
	return nil;
}

void TDirectory::close() {
	if (isOpen()) {
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = ::closedir(dir);
		} while (r == EXIT_ERROR && errno == EINTR);
		path.clear();
	}
	prime();
}




TStdioFile::TStdioFile() {
	prime();
}

TStdioFile::~TStdioFile() {
	close();
}

void TStdioFile::prime() {
	fh = nil;
}

FILE * TStdioFile::open(const std::string& fileName, const std::string& mode) {
	close();
	if (!fileName.empty() && !mode.empty()) {
		do {
			errno = EXIT_SUCCESS;
			fh = ::fopen(fileName.c_str(), mode.c_str());
		} while (!assigned(fh) && errno == EINTR);
		if (isOpen()) {
			this->file = fileName;
			this->mode = mode;
		}
	}
	return fh;
}

void TStdioFile::close() {
	if (isOpen()) {
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = ::fclose(fh);
		} while (r == EXIT_ERROR && errno == EINTR);
		file.clear();
		mode.clear();
	}
	prime();
}

bool TStdioFile::flush() const {
	int r = EXIT_ERROR;
	if (isOpen()) {
		do {
			errno = EXIT_SUCCESS;
			r = ::fflush(fh);
		} while (r == EXIT_ERROR && errno == EINTR);
	}
	return EXIT_SUCCESS == r;
}

bool TStdioFile::seek(const off_t offset, const ESeekOffset whence) const {
	int r = EXIT_ERROR;
	if (isOpen()) {
		do {
			errno = EXIT_SUCCESS;
			r = ::fseek(fh, offset, whence);
		} while (r == EXIT_ERROR && errno == EINTR);
	}
	return EXIT_SUCCESS == r;
}

ssize_t TStdioFile::read(void *const data, size_t const size) const {
	size_t r;
	int errval;

    do {
		errno = EXIT_SUCCESS;
		r = ::fread(data, 1, size, fh);
    } while (r == (size_t)0 && errno == EINTR);
    errval = errno;

    // Read some bytes...
    if (r > (size_t)0) {
    	return (ssize_t)r;
    }

    // Check for EOF
    if (feof(fh) > 0) {
    	return (ssize_t)0;
    }

	// Reader on error
	if (EXIT_SUCCESS == errval)
		errno = EIO;
    return (ssize_t)EXIT_ERROR;
}


ssize_t TStdioFile::write(void const *const data, size_t const size) const {
    size_t r;
	int errval = EXIT_ERROR;
    char const *p = (char const *)data;
    char const *const q = (char const *)data + size;

    while (p < q) {
    	do {
    		errno = EXIT_SUCCESS;
    		r = ::fwrite(p, 1, (size_t)(q - p), fh);
    	} while (r == (size_t)0 && errno == EINTR);
    	errval = errno;

    	// Write failed
    	if (r < (size_t)1) {
    		if (EXIT_SUCCESS == errval)
    			errno = EIO;
        	return (ssize_t)EXIT_ERROR;
    	}

    	p += r;
    }

    // Unexpected pointer after data transfer
    if (p != q) {
		if (EXIT_SUCCESS == errval) {
			errno = EFAULT;
		}
    	return (ssize_t)EXIT_ERROR;
    }

    // Buffer has been fully written
    // Possible overflow on result value (ssize_t)size !!!
    return (ssize_t)size;
}

ssize_t TStdioFile::write(const std::string& data) const {
	if (!data.empty()) {
		return write(data.c_str(), data.size());
	}
    return (ssize_t)0;
}





TFileHandle::TFileHandle() {
	prime();
};

TFileHandle::~TFileHandle() {
	close();
};


void TFileHandle::prime() {
	fd = INVALID_HANDLE_VALUE;
	errval = EXIT_SUCCESS;
}


int TFileHandle::open(const std::string& fileName, const int flags) {
	if (!fileName.empty())
		return open(fileName.c_str(), flags, 0);
	return INVALID_HANDLE_VALUE;
}

int TFileHandle::open(const char * fileName, const int flags) {
	return open(fileName, flags, 0);
}

int TFileHandle::open(const std::string& fileName, const int flags, const mode_t mode) {
	if (!fileName.empty())
		return open(fileName.c_str(), flags, mode);
	return INVALID_HANDLE_VALUE;
}

int TFileHandle::open(const char * fileName, const int flags, const mode_t mode) {
	// Open file
	if (util::assigned(fileName) && !isOpen()) {
		do {
			errno = EXIT_SUCCESS;
			if (mode > 0)
				fd = ::open(fileName, flags, mode);
			else
				fd = ::open(fileName, flags);
		} while (fd == EXIT_ERROR && errno == EINTR);
		errval = errno;
	}
	return fd;
}

void TFileHandle::close() {
	if (isOpen()) {
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = ::close(fd);
		} while (r == EXIT_ERROR && errno == EINTR);
		prime();
	}
}

ssize_t TFileHandle::read(void *const data, size_t const size) const {
	ssize_t r;

    do {
		errno = EXIT_SUCCESS;
		r = ::read(fd, data, size);
    } while (r == (ssize_t)EXIT_ERROR && errno == EINTR);
	errval = errno;

    // Read some bytes...
    if (r >= (ssize_t)0) {
    	return r;
    }

    // Ignore error result of read() if errno not set
    if (r == (ssize_t)EXIT_ERROR) {
    	if (errno != EXIT_SUCCESS)
    		return (ssize_t)EXIT_ERROR;
    	else
    		return (ssize_t)0;
    }

    // Invalid result of ::read()
    // Should not happen by design!
	if (EXIT_SUCCESS == errno)
		errno = EIO;

	return (ssize_t)EXIT_ERROR;
}

ssize_t TFileHandle::write(void const *const data, size_t const size) const {
    char const *p = (char const *)data;
    char const *const q = (char const *)data + size;
    ssize_t r;

    while (p < q) {
    	do {
    		errno = EXIT_SUCCESS;
    		r = ::write(fd, p, (size_t)(q - p));
    	} while (r == (ssize_t)EXIT_ERROR && errno == EINTR);
		errval = errno;

    	// Write failed
    	if (r == (ssize_t)EXIT_ERROR)
        	return (ssize_t)EXIT_ERROR;

		// Something else went wrong
    	if (r < (ssize_t)1) {
    		if (EXIT_SUCCESS == errno)
    			errno = EIO;
        	return (ssize_t)EXIT_ERROR;
    	}

    	p += (size_t)r;
    }

    // Unexpected pointer after data transfer
    if (p != q) {
		if (EXIT_SUCCESS == errno) {
			errno = EFAULT;
			errval = errno;
		}
    	return (ssize_t)EXIT_ERROR;
    }

    // Buffer has been fully written
    // Possible overflow on result value (ssize_t)size !!!
    return (ssize_t)size;
}


bool TFileHandle::control(int flags) const {
	int r = EXIT_ERROR;
	if (isOpen()) {
		do {
			errno = EXIT_SUCCESS;
			r = ::fcntl(fd, F_SETFL, flags);
		} while (r == EXIT_ERROR && errno == EINTR);
	} else
		errno = EBADF;
	errval = errno;
	return (r != EXIT_ERROR);
}

bool TFileHandle::truncate(off_t size) const {
	int r = EXIT_ERROR;
	if (isOpen()) {
		do {
			errno = EXIT_SUCCESS;
			r = ::ftruncate(handle(), size);
		} while (r == EXIT_ERROR && errno == EINTR);
	} else
		errno = EBADF;
	errval = errno;
	return (r == EXIT_SUCCESS);
}

bool TFileHandle::chmode(mode_t mode) const {
	int r = EXIT_ERROR;
	if (isOpen()) {
		do {
			errno = EXIT_SUCCESS;
			r = ::fchmod(fd, mode);
		} while (r == EXIT_ERROR && errno == EINTR);
	} else
		errno = EBADF;
	errval = errno;
	return (r == EXIT_SUCCESS);
}

bool TFileHandle::chown(uid_t owner, gid_t group) const {
	int r = EXIT_ERROR;
	if (isOpen()) {
		do {
			errno = EXIT_SUCCESS;
			r = ::fchown(fd, owner, group);
		} while (r == EXIT_ERROR && errno == EINTR);
	} else
		errno = EBADF;
	errval = errno;
	return (r == EXIT_SUCCESS);
}



TBaseFile::TBaseFile() : TFileHandle() {
	prime();
	finalize();
}

TBaseFile::TBaseFile(const std::string& file) : TFileHandle() {
	prime();
	initialize(file);
}

TBaseFile::~TBaseFile() {
	clear();
}

void TBaseFile::initialize(const std::string& file) {
	finalize();
	this->file = file;
}

void TBaseFile::finalize() {
	close();
	clear();
	reset();
}


void TBaseFile::clear() {
	// Nothing to do here...
}

void TBaseFile::reset() {
	fileExists = false;
	execExists = false;
}

void TBaseFile::prime() {
	onChunkReadMethod = nil;
}

bool TBaseFile::create(const size_t size, const mode_t mode) {
	bool retVal = false;
	if (!file.empty() && /*!isLoaded() &&*/ !isOpen() && !exists()) {
		// Open file in current scope
		TFileGuard<util::TBaseFile> o(*this, O_WRONLY | O_CREAT, mode);
		if (size)
			retVal = truncate(size);
	}
	// Check if file is successfully created
	reset();
	exists();
	return retVal;
}


bool TBaseFile::resize(const size_t size, const mode_t mode) {
	// std::cout << "TBaseFile::resize(0) size = " << size << ", file = " << file << ", exists = " << fileExists << std::endl;
	bool retVal = false;
	if (/*!isLoaded() &&*/ size && exists()) {
		if (!isOpen()) {
			// Open file for truncate in local scope!
			// --> Closed again by file guard template
			TFileGuard<util::TBaseFile> o(*this, O_WRONLY | O_CREAT, mode);
			retVal = truncate(size);
		} else
			retVal = truncate(size);
	}
	return retVal;
}


app::THandle TBaseFile::open(const int flags, const mode_t mode) {
	// Open file
	if (!file.empty() && !isOpen()) {
		if (0 > TFileHandle::open(file, flags, mode)) {
			throw util::sys_error("TBaseFile::open() Access to file <" + file + "> failed.", errno);
		}
	}
	// Check if file was created by given mode
	if (flags & O_CREAT) {
		reset();
		exists();
	}
	return handle();
}

app::THandle TBaseFile::open(const std::string& fileName, const int flags, const mode_t mode) {
	initialize(fileName);
	return open(flags, mode);
}


bool TBaseFile::status(struct stat * const status) const {
	bool r = false;
	if (!file.empty()) {
		r = fileStatus(file, status);
		if (!r) {
			execExists = true;
			fileExists = false;
		}
	}
	return r;
}


bool TBaseFile::link(struct stat * const status) const {
	return ((file.empty()) ? false : fileLink(file, status));
}

bool TBaseFile::exists() const {
	if (!execExists) {
		fileExists = util::fileExists(file);
		execExists = true;
	}
	return fileExists;
}


ssize_t TBaseFile::seek(const off_t offset, const ESeekOffset whence) const {
	if (isOpen()) {
		/* Move FD's file position to OFFSET bytes from the
		   beginning of the file (if WHENCE is SEEK_SET),
		   the current position (if WHENCE is SEEK_CUR),
		   or the end of the file (if WHENCE is SEEK_END).
		   Return the new file position.  */
		off_t r;
		do {
			errno = EXIT_SUCCESS;
			r = ::lseek(handle(), offset, whence);
		} while (r == (off_t)EXIT_ERROR && errno == EINTR);
		errval = errno;
		return (ssize_t)r;
	}
	return (ssize_t)EXIT_ERROR;
}

ssize_t TBaseFile::read(void *const data, const size_t size, const off_t offset, const ESeekOffset whence) const {
	ssize_t retVal = 0;
	if (isOpen()) {

		if (offset > 0) {
			if (seek(offset, whence) <= 0) {
				throw util::sys_error("TBaseFile::read() Seek failed for <" + file + ">");
			}
		}

		retVal = TFileHandle::read(data, size);
		if (retVal < 0) {
			throw util::sys_error("TBaseFile::read() failed for <" + file + ">");
		}

	}

	if (retVal > (ssize_t)0)
		return (ssize_t)retVal;

	return (ssize_t)0;
}

ssize_t TBaseFile::write(const std::string data, const off_t offset, const ESeekOffset whence) const {
	return write(data.c_str(), data.size(), offset, whence);
}

ssize_t TBaseFile::write(const void *const data, const size_t size, const off_t offset, const ESeekOffset whence) const {
	ssize_t retVal = 0;
	if (isOpen()) {

		if (offset > 0) {
			if (seek(offset, whence) != (ssize_t)offset) {
				throw util::sys_error("TBaseFile::write() Seek failed for <" + file + ">");
			}
		}

		retVal = TFileHandle::write(data, size);

		if (EXIT_ERROR == retVal) {
			throw util::sys_error("TBaseFile::write() failed for <" + file + ">");
		}

		if (retVal != (ssize_t)size) {
			throw util::sys_error("TBaseFile::write() Could not write all data to file <" + file + ">, " + std::to_string((size_u)retVal) +
					" bytes written from " + std::to_string((size_u)size) + " bytes.");
		}

	}

	// All data transfered?
	if (retVal == (ssize_t)size)
		return (ssize_t)size;

	// Return size of transferred bytes
	if (retVal > (ssize_t)0)
		return (size_t)retVal;

	return (ssize_t)0;
}


void TBaseFile::chunkedRead(const size_t chunkSize) {
	// std::cout << "TBaseFile::chunkedRead() chunkSize = " << chunkSize << ", file = " << file << ", exists = " << fileExists << std::endl;
	if (onChunkReadMethod != nil) {
		if (exists() && !isOpen()) {
			TFileGuard<TBaseFile> o(*this, O_RDONLY);
			if (isOpen()) {
				chunk.resize(chunkSize, false);
				size_t bytesRead = 0;
				do {
					bytesRead = read(chunk.data(), chunkSize);
					if (bytesRead > 0)
						onChunkRead(chunk.data(), bytesRead);
				} while (bytesRead > 0);
				chunk.clear();
			}
		}
		// Terminate chunked read
		onChunkRead(nil, 0);
	}
}

void TBaseFile::onChunkRead(void *const data, const size_t size) {
	if (onChunkReadMethod != nil && size > 0 && util::assigned(data)) {
		onChunkReadMethod(data, size);
	}
}




TFile::TFile() : TBaseFile() {
	prime();
	finalize();
}

TFile::TFile(const std::string& file) : TBaseFile(file) {
	prime();
	initialize(file);
}


TFile::TFile(const std::string& file, const std::string& root) : TBaseFile(file)  {
	prime();
	initialize(file, root);
}


TFile::~TFile() {
	clear();
}


void TFile::initialize(const std::string& file, const std::string& root) {
	finalize();
	this->root = root;
	this->file = file;
	if (!file.empty()) {
		path = util::filePath(file);
		name = util::fileExtName(file);
		base = util::fileBaseName(file);
		ext  = util::fileExt(file);
		mime = util::getMimeType(ext);
		bCSS  = isMime(CSS_MIME_TYPE);
		bHTML = isMime(HTML_MIME_TYPE);
		bText = isMime(TEXT_MIME_TYPE);
		bJPG  = isMime(JPEG_MIME_TYPE);
		bJSON = isMime(JSON_MIME_TYPE);
		bJava = isMime(JAVA_MIME_TYPE);
		readProperties();
	}
}


void TFile::finalize() {
	close();
	clear();
	reset();
}

void TFile::clear() {
	if (util::assigned(parser))
		util::freeAndNil(parser);
	if (util::assigned(content)) {
		content->clear();
		util::freeAndNil(content);
	}
	deleteZippedBuffer();
	deleteFileBuffer();
}


void TFile::reset() {
	time.clear();
	base64.clear();
	html.clear();
	size = 0;
	zsize = 0;
	inode = 0;
	fileHash = 0;
	pathHash = 0;
	nameHash = 0;
	rootHash = 0;
	urlHash  = 0;
	bFolder = false;
	bFile = false;
	bExec = false;
	bCGI = false;
	bLink = false;
	bEllipsis = false;
	bHidden = false;
	bLoaded = false;
	file.clear();
	root.clear();
	path.clear();
	name.clear();
	ext.clear();
	mime.clear();
	url.clear();
	etag.clear();
	bHTML = false;
	bCSS = false;
	bText = false;
	bJPG = false;
	bJSON = false;
	bJava = false;
	TBaseFile::reset();
}


void TFile::prime() {
	content = nil;
	buffer = nil;
	zbuffer = nil;
	parser = nil;
}

void TFile::deleteFileBuffer() {
	if (util::assigned(buffer)) {
		delete[] buffer;
		buffer = nil;
	}
	bLoaded = false;
}


void TFile::deleteZippedBuffer() {
	if (util::assigned(zbuffer)) {
		delete[] zbuffer;
		zbuffer = nil;
	}
	zsize = 0;
}


void TFile::assign(const std::string& file, const std::string& root) {
	initialize(file, root);
}

std::string TFile::makeURL() const {
	std::string retVal;
	if (!root.empty()) {
		if (file.size() > root.size()) {
			// Check if path starts with root
			// --> otherwise URL is inconsistent!
			if (0 == util::strncasecmp(file, root, root.size())) {
				retVal = file.substr(util::pred(root.size()), std::string::npos);
			} else
				throw util::app_error("TFile::makeURL() failed: Root <" + root + "> is not member of file path <" + file + ">");
		} else
			retVal = URL_SEPERATOR;
	}
	return retVal;
}


const size_t TFile::getContentSize() const {
	size_t retVal = 0;
	if (util::assigned(content))
		retVal = content->size();
	return retVal;
}


const PParserBuffer TFile::getParserData() {
	parserNeeded(false);
	if (assigned(parser))
		return parser->getData();
	else
		return nil;
}


const PTokenParser TFile::getParser() const {
	parserNeeded(false);
	return parser;
}


bool TFile::hasToken() const {
	bool retVal = false;
	if (hasParser())
		 retVal = getParser()->hasToken();
	return retVal;
}


size_t TFile::deleteParserData() const {
	if (hasParser())
		return getParser()->deleteInvalidatedBuffers();
	return (size_t)0;
}



bool TFile::isContentEmpty() const {
	bool retVal = true;
	if (util::assigned(content))
		retVal = content->empty();
	return retVal;
}


void TFile::calculateHashValues() {
	fileHash = getFileHash();
	pathHash = getPathHash();
	nameHash = getNameHash();
	rootHash = getRootHash();
	urlHash  = getUrlHash();
}

util::hash_type TFile::getFileHash() {
	if (fileHash == 0)
		fileHash = util::calcHash(file);
	return fileHash; 
}

util::hash_type TFile::getNameHash() {
	if (nameHash == 0)
		nameHash = util::calcHash(name);
	return nameHash;
}

util::hash_type TFile::getPathHash() {
	if (pathHash == 0)
		pathHash = util::calcHash(path);
	return pathHash;
}

util::hash_type TFile::getRootHash() {
	if (rootHash == 0)
		rootHash = util::calcHash(root);
	return rootHash;
}

util::hash_type TFile::getUrlHash() {
	if (urlHash == 0)
		urlHash = util::calcHash(getURL());
	return urlHash;
}


const std::string& TFile::getETag() const {
	if (etag.empty())
		etag = util::cprintf("%lx-%lx-%lx", getInode(), getTime().time(), getSize());
	return etag;
}


const std::string& TFile::getURL() const {
	if (url.empty()) {
		url = makeURL();
	}
	return url;
}



bool TFile::readProperties() {
    bool retVal = false;
	struct stat buf;

	if (!name.empty()) {
		if (name.size() == 2) {
			bEllipsis = name[0] == '.' && name[1] == '.';
		} else {
			if (name.size() > 1)
				bHidden = name[0] == '.';
		}
	}

	if (!file.empty()) {

		// Read file properties (follow links to target file)
		if (status(&buf)) {
			inode   = buf.st_ino;
			size    = buf.st_size;
			time    = buf.st_mtim;
			bFile   = S_ISREG(buf.st_mode);
			bFolder = S_ISDIR(buf.st_mode);

			// Check for executable file
			if (bFile && (buf.st_mode & S_IXUSR))
				bExec = EXIT_SUCCESS == access(file.c_str(), X_OK);

			// Check for CGI executable script/binary
			if (bExec) {
				if (0 == util::strncasecmp(ext, "cgi", 3))
					bCGI = true;
			}

			retVal  = true;
		}

		// Check for symbolic link
		if (retVal) {
			if (link(&buf)) {
				bLink = S_ISLNK(buf.st_mode);
			} else
				retVal = false;
		}
	}

	return retVal;
}


bool TFile::isMime(const std::string& mime) const {
	return (0 == util::strncasecmp(this->mime, mime, mime.size()));
}

bool TFile::match(const std::string& pattern) {
	return fileMatch(pattern, name);
}


bool TFile::create(const size_t size, const mode_t mode) {
	bool retVal = TBaseFile::create(size, mode);
	if (retVal) {
		if (!readProperties())
			throw util::app_error("TFile::create() failed: No access to file <" + file + ">");
	}
	return retVal;
}


bool TFile::binaryLoad() {
    bool retVal = false;

    // Buffer size = file size + 1 for trailing '\0'
	buffer = new char[size+1];
	if (!util::assigned(buffer))
		return false;

    // Open file
	TFileGuard<util::TFile> o(*this, O_RDONLY);
	ssize_t n = 0;

    if (isOpen()) {
    	n = read(buffer, size);
		retVal = n == static_cast<ssize_t>(size);
    }

    if (retVal)
		buffer[n] = '\0';
    else
    	deleteFileBuffer();

    return retVal;
}

bool TFile::minimizedLoad() {
    bool retVal = false;
    util::TStringList list;
    list.loadFromFile(file);
    if (!list.empty()) {
    	size_t len = list.length();
    	if (len <= size) {
    		std::string p, s;
    		size_t r = 0;
    		p.reserve(len);
    		app::TStringVector::const_iterator it = list.begin();
    		while (it != list.end()) {
    			s = util::trim(*it);
    			if (!s.empty()) {
    				r += s.size();
    				p += (s + " ");
    			}
    			++it;
    		}
    		if (r > 0) {
    			// Size represents the minimized file size now!
    			size = r;

    			// Buffer size = file size + 1 for trailing '\0'
    			buffer = new char[size+1];

    			// Copy minimized file to file buffer
    			memcpy(buffer, p.c_str(), size);
    			buffer[size] = '\0';
    	    	retVal = true;

    		} else
    			deleteFileBuffer();
    	}
    }
    return retVal;
}


bool TFile::load(const ELoadType type) {
    bool retVal = false;
    if (bFile && size > 0 && !util::assigned(buffer)) {
    	switch (type) {
			case LT_MINIMIZED:
				retVal = minimizedLoad();
				break;
			default:
			case LT_BINARY:
				retVal = binaryLoad();
				break;
    	}
    	if (retVal)
    		bLoaded = true;
    } else {
    	// Return OK, if file empty, loaded before or not a regular file!
    	retVal = true;
    }
	return retVal;
}


bool TFile::load(const std::string& file, const ELoadType type) {
	bool r;
	assign(file);
	r = load(type);
	close();
	return r;
}


bool TFile::compress() {
    bool retVal = false;
    if (bLoaded && !util::assigned(zbuffer) && size > 0) {
    	size_t z = zip.gzip(buffer, zbuffer, size);
    	if (z > 0) {
			retVal = true;
	    	zsize = z;
		}	
    }
	return retVal;
}


void TFile::createParser(const std::string& startMask, const std::string& endMask) {
	parserNeeded();
	parser->initialize((char*)buffer, size, startMask, endMask);
}


void TFile::parserNeeded(bool create) const {
	if (create) {
		if (!assigned(parser))
			parser = new TTokenParser;
	} else
		app_error("TFile::parserNeeded() called out of sequence, createParser() omitted.");
}


const std::string& TFile::asBase64() {
	if (base64.empty()) {
		if (!isLoaded())
			load();
		if (isLoaded())
			base64 = TBase64::encode(buffer, size);
	}
	return base64;
}

const std::string& TFile::asHTML() {
	if (!asBase64().empty()) {
		if (html.empty())
			html = "\"data:" + getMime() + ";" + asBase64() + "\"";
	}
	return html;
}


TFileList::TFileList() {
	init();
}


TFileList::~TFileList() {
	clear();
}


void TFileList::init() {
	root.clear();
	key = FLK_DEFAULT;
	type = LT_BINARY;
	load = false;
	fileSize = 0;
	zipThreshold = 1024;
	zipEnabled = true;
	debug = false;
}


int TFileList::readDirektory(const std::string& path, const ESearchDepth depth, const PFile folder, const bool hidden)
{
	TDirectory dir;
	struct dirent *file;
	std::string root = util::validPath(path);

	if (!hidden && util::fileExists(root + "noscan"))
		return 0;

	if (SD_ROOT == depth)
		clear();

	dir.open(root);
	if (dir.isOpen()) {
		PFile o;
		while (assigned((file = dir.read()))) {
			bool ignore = hidden ? 0 == ::strcmp(file->d_name, ".") : file->d_name[0] == '.';
			if(ignore || (0 == ::strcmp(file->d_name, "..")))
				continue;

			// Add file to map
			o = add(root + file->d_name);
			if (util::assigned(folder)) {
				if (!util::assigned(folder->content))
					folder->content = new TFolderContent();
				folder->content->push_back(o);
			}

			// Recursive folder scan
			if (SD_RECURSIVE == depth && file->d_type == DT_DIR) {
            	readDirektory(validPath(root + file->d_name), depth, o, hidden);
            }
		}
		dir.close();
	}
	return files.size();
}


int TFileList::scan(const std::string& root, const EFileListKey key, const bool load, const ESearchDepth depth, const ELoadType type, const bool hidden)
{
	clear();
	if (key == FLK_DEFAULT)
		this->key = FLK_FILE;
	else
		this->key = key;
	this->root = util::validPath(root);
	this->load = load;
	this->type = type;

	// Add root folder to list
	PFile o = add(this->root);
	return readDirektory(this->root, depth, o, hidden);
}


bool stdFileParseDecider(TFile& file) {
	return file.isHTML() || file.isJSON() || file.isText();
}

bool stdFileZipDecider(TFile& file) {
	return !file.hasToken();
}

int TFileList::parse(const std::string& startMask, const std::string& endMask, TFileDecider parseDecider, TFileDecider zipDecider) {
	int retVal = 0;
	int ratio = 0;
	if (!files.empty()) {
		PFile o;
		if (!assigned(parseDecider))
			parseDecider = stdFileParseDecider;
		TFileMap::iterator it = files.begin();
		while (it != files.end()) {
			o = it->second;
			if (util::assigned(o)) {
				if (parseDecider(*o)) {
					if (debug) std::cout << app::blue << "TFileList::parse(" << o->getFile() << ")" << app::reset << std::endl;
					if (!o->isLoaded())
						o->load();
					o->createParser(startMask, endMask);
					retVal += o->parser->parse();
				}
				if (assigned(zipDecider) && o->isLoaded() && zipEnabled && (o->size > zipThreshold)) {
					if (zipDecider(*o)) {
						o->compress();
						if (util::assigned(o->getZippedData())) {
							ratio = 0;
							if (o->getSize() && o->getZippedSize()) {
								// Force zipped files to be at least 20% smaller than original file buffer
								ratio = o->getZippedSize() * 100 / o->getSize();
								if (ratio > 80)
									o->deleteZippedBuffer();
							}
							if (debug && util::assigned(o->getZippedData())) {
								std::cout << app::white << "  TFileList::compress(" << o->getFile() << ") " << o->getSize() << "/" << o->getZippedSize() << " ratio " << ratio << "%" << app::reset <<  std::endl;
								debugBufferOutput("    Buffer 00..15 : ", o->getZippedData(), 16);
								debugBufferOutput("    Buffer 16..31 : ", o->getZippedData()+16, 16);
							}
						}
					}
				}
			}
			it++;
		}
	}
	return retVal;
}


void TFileList::debugBufferOutput(const std::string& text, const char* p, size_t size) {
	if (util::assigned(p) && size) {
		std::string s = util::TBinaryConvert::binToHexA(p, size);
		std::cout << text << s << std::endl;
	}
}


PFile TFileList::add(const std::string& file) {
	// std::cout << "TFileList::addFile(" << file << ")" << std::endl;
	PFile o = new TFile(file, root);
	if (load) {
		ELoadType lt = LT_BINARY;
		if (type == LT_MINIMIZED) {
			if (o->isHTML() || o->isCSS() || o->isText())
				lt = LT_MINIMIZED;
		}
		o->load(lt);
	}

	fileSize += o->size;

	// Add file depending on key value
	switch (key) {
		case FLK_DEFAULT:
		case FLK_FILE:
			files.insert(TFileMapItem(o->getFile(), o));
			break;
		case FLK_NAME:
			files.insert(std::make_pair(o->getName(), o));
			break;
		case FLK_URL:
			files[o->getURL()] = o;
			break;
	}

	return o;
}


PFile TFileList::find(const std::string& file) {
	PFile retVal = nil;
	if (!files.empty()) {
		TFileMap::iterator it = files.find(file);
		if (it != files.end())
			retVal = it->second;
	}
	return retVal;
}


std::string TFileList::trim(const std::string& file) {
	// Strip ending char '/' for folders
	// Retain root folder --> file.size() > 1!
	std::string s(file);
	if (s.size() > 1) {
		while (s[util::pred(s.size())] == '/' && s.size() > 1)
#ifdef STL_HAS_STRING_POPS
			s.pop_back();
#else
		s.erase(util::pred(s.size()));
#endif
	}
	return s;
}


void TFileList::clear() {
	if (!files.empty()) {
		PFile o;
		TFileMap::iterator it = files.begin();
		while (it != files.end()) {
			o = it->second;
			util::freeAndNil(o);
			it++;
		}
		files.clear();
	}
	fileSize = 0;
}


bool TFileList::validIndex(const std::string::size_type idx) {
	return idx >= 0 && idx < files.size();
}


PFile TFileList::operator[] (const std::string::size_type idx) {
	PFile o = nil;
	if (validIndex(idx)) {
		TFileMap::const_iterator it = files.begin();
		for (std::string::size_type i=0; i<idx; i++)
			it++;
		o = it->second;
	}
	return o;
}

PFile TFileList::operator[] (const std::string& file) {
	return find(file);
}




TFolderList::TFolderList() {
	root.clear();
	fileSize = 0;
	onFileFoundMethod = nil;
}


TFolderList::~TFolderList() {
	clear();
}

void TFolderList::clear() {
	util::clearObjectList(files);
}

bool TFolderList::validIndex(const std::string::size_type idx) {
	return idx >= 0 && idx < files.size();
}

PFile TFolderList::find(const std::string& file)
{
	PFile o;
	size_t i,n;
	n = files.size();
	for (i=0; i<n; i++) {
		o = files[i];
		if (util::assigned(o)) {
			if (o->getName() == file)
				return o;
		}
	}
	return nil;
}

PFile TFolderList::operator[] (const std::string::size_type idx) {
	if (validIndex(idx))
		return files[idx];
	return nil;
}

PFile TFolderList::operator[] (const std::string& file) {
	return find(file);
}

int TFolderList::scan(const std::string& files, const ESearchDepth depth, const bool hidden) {
	fileSize = 0;
	root = util::filePath(files);
	std::string pattern = util::fileExtName(files);
	if (pattern.empty())
		pattern = "*";
	return readDirektory(root, pattern, depth, hidden);
}

int TFolderList::scan(const std::string& files, const app::TStringVector& patterns, const ESearchDepth depth, const bool hidden) {
	fileSize = 0;
	root = util::filePath(files);
	if (patterns.empty())
		return 0;
	return readDirektory(root, patterns, depth, hidden);
}

int TFolderList::scan(const std::string& files, const std::string& pattern, const ESearchDepth depth, const bool hidden) {
	app::TStringVector list;
	if (!pattern.empty())
		list.push_back(pattern);
	fileSize = 0;
	root = util::filePath(files);
	return readDirektory(root, list, depth, hidden);
}

int TFolderList::readDirektory(const std::string& path, const std::string& pattern, const ESearchDepth depth, const bool hidden) {
	app::TStringVector list;
	if (!pattern.empty())
		list.push_back(pattern);
	return readDirektory(path, list, depth, hidden);
}

int TFolderList::readDirektory(const std::string& path, const app::TStringVector& patterns, const ESearchDepth depth, const bool hidden) {
	TDirectory dir;
	struct dirent *file;
	std::string pattern;
	util::TStringList folders;
	std::string root = util::validPath(path);

	if (SD_ROOT == depth)
		clear();

	dir.open(root);
	if (dir.isOpen()) {
		while(assigned((file = dir.read()))) {
			bool ignore = hidden ? 0 == ::strcmp(file->d_name, ".") : file->d_name[0] == '.';
			if(ignore || (0 == ::strcmp(file->d_name, "..")))
				continue;

			// Add file to list if pattern matches filename
			bool added = false;
			if (!patterns.empty()) {
				app::TStringVector::const_iterator it = patterns.begin();
				while (it != patterns.end()) {
					pattern = *it;
					if (!pattern.empty()) {
						if (match(file, pattern)) {
							addFile(root + file->d_name);
							added = true;
						}
					}
					it++;
				}
			}

			// Prepare recursive folder scan
			if (SD_RECURSIVE == depth && file->d_type == DT_DIR) {

				// Scan folders one by one afterwards...
				folders.add(util::validPath(root + file->d_name));

				// Add folders to scanned files on empty search pattern
				if (patterns.empty() && !added) {
					addFile(root + file->d_name);
				}
			}
		}
		dir.close();
	}

	// Scan sub folders
	if (!folders.empty()) {
		for (size_t i=0; i<folders.size(); ++i) {
			readDirektory(folders[i], patterns, depth, hidden);
		}
	}

	return files.size();
}

bool TFolderList::match(struct dirent *file, const std::string& pattern) {
	if (!pattern.empty() && util::assigned(file)) {
		const char* f = file->d_name;
		if (util::assigned(f)) {
			const char* p = pattern.c_str();
			return (0 == ::fnmatch(p, f, FNM_NOESCAPE | FNM_CASEFOLD));
		}
	}
	return false;
}

PFile TFolderList::addFile(const std::string& file)
{
	PFile o = new TFile(file, root);
	fileSize += o->getSize();
	files.push_back(o);
	onFileFound(*o);
	return o;
}

void TFolderList::onFileFound(const util::TFile& file) {
	if (onFileFoundMethod != nil) {
		onFileFoundMethod(file);
	}
}

bool mimeSorterAsc  (PFile o, PFile p) { return (o->getMime() >= p->getMime()); }
bool mimeSorterDesc (PFile o, PFile p) { return (o->getMime() < p->getMime()); }
bool nameSorterAsc  (PFile o, PFile p) { return (o->getNameHash() >= p->getNameHash()); }
bool nameSorterDesc (PFile o, PFile p) { return (o->getNameHash() < p->getNameHash()); }
bool timeSorterAsc  (PFile o, PFile p) { return (o->getTime() >= p->getTime()); }
bool timeSorterDesc (PFile o, PFile p) { return (o->getTime() < p->getTime()); }

void TFolderList::sort(ESortOrder order, TFileSorter asc, TFileSorter desc) {
	TFileSorter sorter;
	switch(order) {
		case SO_DESC:
			sorter = desc;
			break;
		case SO_ASC:
		default:
			sorter = asc;
			break;
	}
	sort(sorter);
}

void TFolderList::sort(TFileSorter sorter) {
	std::sort(files.begin(), files.end(), sorter);
}

void TFolderList::sortByMime(ESortOrder order) {
	sort(order, mimeSorterAsc, mimeSorterDesc);
}

void TFolderList::sortByName(ESortOrder order) {
	sort(order, nameSorterAsc, nameSorterDesc);
}

void TFolderList::sortByTime(ESortOrder order) {
	sort(order, timeSorterAsc, timeSorterDesc);
}

void TFolderList::debugOutput() {
	PFile o;
	size_t i,n;
	n = files.size();
	for (i=0; i<n; i++) {
		o = files[i];
		if (util::assigned(o)) {
			std::cout << "File: " << app::blue << o->getFile() << app::reset << std::endl;
			std::cout << "  Last changed at " << o->getTime().asString() << std::endl;
		}
	}
}

int TFolderList::deleteOldest(const std::string& files, const size_t top, bool dryRun) {
	int retVal = 0;
	scan(files);
	sortByTime(SO_ASC);
	if (this->files.size() > top) {
		PFile o;
		for (size_t idx = top; idx < this->files.size(); idx++) {
			o = this->files[idx];
			if (util::assigned(o)) {
				if (dryRun)
					std::cout << "Delete file: " << app::yellow << o->getFile() << app::reset << std::endl;
				else
					util::deleteFile(o->getFile());
				retVal++;
			}
		}
	}
	return retVal;
}

int TFolderList::deleteByAge(const std::string& files, const TTimePart days, bool dryRun) {
	int retVal = 0;
	TTimePart age = util::now() - days * 86400;
	scan(files);
	sortByTime(SO_ASC);
	if (!this->files.empty()) {
		PFile o;
		for (size_t idx = 0; idx < this->files.size(); idx++) {
			o = this->files[idx];
			if (util::assigned(o)) {
				if (o->getTime() < age) {
					if (dryRun)
						std::cout << "Delete file: " << app::yellow << o->getFile() << app::reset << std::endl;
					else
						util::deleteFile(o->getFile());
					retVal++;
				}
			}
		}
	}
	return retVal;
}


TFileScanner::TFileScanner() {
	prime();
}

TFileScanner::~TFileScanner() {
}

void TFileScanner::prime() {
	breakOffset = std::string::npos;
	defaultSize = SCAN_BUFFER_SIZE;
	bufferSize = defaultSize;
	patternSize = 0;
	hasBreakPattern = false;
	hasBreakSize = false;
	isCaseSensitive = true;
	debug = false;
}

void TFileScanner::clear() {
	map.clear();
	bufferSize = defaultSize;
	reset();
}

void TFileScanner::reset() {
	patternSize = 0;
	breakOffset = std::string::npos;
	breakPattern.clear();
	hasBreakPattern = false;
	hasBreakSize = false;
}


bool TFileScanner::add(const std::string& pattern, size_t value) {
	const_iterator it = map.find(pattern);
	if (it == end()) {
		map[pattern] = value;
		return true;
	}
	if (std::string::npos == it->second) {
		map[pattern] = value;
		return true;
	}
	return false;
}


size_t TFileScanner::find(const std::string& pattern) {
	const_iterator it = map.find(pattern);
	if (it != end())
		return it->second;
	return std::string::npos;
}

size_t TFileScanner::operator[] (const std::string& pattern) {
	return find(pattern);
}


ssize_t TFileScanner::scan(const std::string& fileName, size_t offset, const app::TStringVector& pattern, size_t breakSize) {
	if (!fileExists(fileName))
		return (ssize_t)-6;

	TFile file(fileName);
	file.open(O_RDONLY);

	if (file.isOpen())
		return scan(file, offset, pattern, breakSize);

	return (ssize_t)-10;
}

ssize_t TFileScanner::scan(const std::string& fileName, size_t offset, const app::TStringVector& pattern, const std::string& breakPattern) {
	if (!fileExists(fileName))
		return (ssize_t)-6;

	TFile file(fileName);
	file.open(O_RDONLY);

	if (file.isOpen())
		return scan(file, offset, pattern, breakPattern);

	return (ssize_t)-10;
}


ssize_t TFileScanner::scan(TFile& file, size_t offset, const app::TStringVector& pattern, size_t breakSize) {
	reset();

	// std::cout << "TFileScanner::scan() Count = " << pattern.size() << std::endl;

	// Check prerequisites
	if (breakSize <= 0 && offset <= 0)
		return (ssize_t)-1;

	// Set break offset size
	if (breakSize <= 0)
		breakSize = file.getSize();
	if (breakSize < bufferSize)
		bufferSize = breakSize;
	breakOffset = breakSize;
	hasBreakSize = true;

	return scanner(file, offset, pattern);
}

ssize_t TFileScanner::scan(TFile& file, size_t offset, const app::TStringVector& pattern, const std::string& breakPattern) {
	reset();

	// Check prerequisites
	if (breakPattern.empty())
		return (ssize_t)-2;

	// Add break pattern to map
	map[breakPattern] = std::string::npos;
	patternSize = breakPattern.size();
	this->breakPattern = breakPattern;
	hasBreakPattern = true;

	ssize_t r = scanner(file, offset, pattern);

	// Invalidate break pattern TODO Why????
	// map[breakPattern] = std::string::npos;

	return r;
}


ssize_t TFileScanner::scanner(TFile& file, size_t offset, const app::TStringVector& pattern) {
	size_t size = bufferSize;
	TBuffer buffer(size + 128);
	size_t bytesToRead = file.getSize();
	size_t bytesRead = 0;
	ssize_t m, needles = 0, found = 0;
	bool eof = false;
	bool last, underrun;
	size_t position, read;

	// Check prerequisites
	if (pattern.empty())
		return (ssize_t)-3;
	// std::cout << "TFileScanner::scanner() Count = " << pattern.size() << std::endl;

	if (!file.isOpen())
		return (ssize_t)-4;

	if (file.getSize() <= 0)
		return (ssize_t)-5;

	// Add pattern to map
	for (size_t i=0; i<pattern.size(); ++i) {
		if (!pattern[i].empty()) {
			++needles;
			map[pattern[i]] = std::string::npos;
			if (patternSize < pattern[i].size())
				patternSize = pattern[i].size();
		}
	}

	// Go to given file position
	if (offset == 0)
		file.seek(0, SO_FROM_START);
	else
		file.seek(offset, SO_FROM_START);

	do {
		if ((bytesRead + bufferSize) < bytesToRead)
			size = bufferSize;
		else
			size = bytesToRead - bytesRead;

		// Read next buffer from file
		eof = (0 == (read = file.read(buffer(), size)));
		position = offset + bytesRead;
		last = read < size;

		// Scan trough buffer
		if (!eof) {
			if ((m = lookup(buffer, read, position, underrun)) > 0)
				found += m;
			
			// Check for underrun:
			// --> Pattern doesn't fit in read buffer
			// --> Read buffer from later position
			if (!last && underrun) {

				// Set new position later in file
				bytesRead += patternSize;
				position += patternSize;

				// Seek to later position
				file.seek(position, SO_FROM_START);
				
			} else {
				bytesRead += read;
			}
		};
		
		if (debug) {
			std::cout << " TFileScanner::scanner() Bytes to read = <" << bytesToRead << ">" << std::endl;
			std::cout << " TFileScanner::scanner() Bytes read    = <" << bytesRead << ">" << std::endl;
			std::cout << " TFileScanner::scanner() Offset        = " << offset << std::endl;
			std::cout << " TFileScanner::scanner() Position      = " << position << std::endl;
			std::cout << " TFileScanner::scanner() Break size    = " << breakOffset << std::endl;
			std::cout << " TFileScanner::scanner() Has pattern   = " << hasBreakPattern << std::endl;
			std::cout << " TFileScanner::scanner() Has size      = " << hasBreakSize << std::endl;
			std::cout << " TFileScanner::scanner() Underrun      = " << underrun << std::endl;
		}

		// Check if search finished
		if (!eof)
			eof = found >= needles;
		if (!eof && hasBreakPattern)
			eof = std::string::npos != find(breakPattern);
		if (!eof && hasBreakSize)
			eof = bytesRead >= breakOffset;
		if (!eof)
			eof = bytesRead >= bytesToRead;

	} while (!eof);

	return found;
}


ssize_t TFileScanner::lookup(TBuffer& buffer, size_t size, size_t position, bool& underrun) {
	ssize_t found = 0;
	underrun = false;
	iterator it;
	size_t i,j,k;
	bool ok;

	for (i=0; i<size; ++i) {
		it = map.begin();
		while (it != map.end()) {
			if (it->second == std::string::npos) {

				// Check if first char of pattern is equal to current char in buffer
				if (compare(buffer[i], it->first[0])) {

					// Check the rest of given pattern
					ok = true;
					k = i + 1;
					for (j=1; j<it->first.size(); ++j) {

						// Check if pattern will fit in current buffer
						if (k >= size) {
							underrun = true;
							return found;
						}

						// Compare current pattern
						if (!compare(buffer[k], it->first[j])) {
							ok = false;
							break;
						}

						// Current pattern offset in buffer
						++k;
					}

					// Pattern found
					if (ok) {
						++found;
						it->second = position + i;
					}
				}
			}

			++it;
		}
	}
	return found;
}

bool TFileScanner::compare(const char c1, const char c2) const {
	return isCaseSensitive ? (c1 == c2) : (::tolower(c1) == ::tolower(c2));
}


void TFileScanner::debugOutput(const std::string& preamble) const {
	const_iterator it = begin();
	if (!map.empty()) {
		while (it != end()) {
			if (it->second != std::string::npos)
				std::cout << preamble << "Pattern: \"" << it->first << "\" Offset = " << it->second << std::endl;
			else
				std::cout << preamble << "Pattern: \"" << it->first << "\" not found." << std::endl;
			++it;
		}
	} else
		std::cout << preamble << "No pattern found." << std::endl;
}


} /* namespace util */


bool isValidMountPoint(const std::string& mount, const util::EMountType type) {
	bool ok = false;
	if (!mount.empty()) {
		ok = mount[0] == '/'; // Is valid path?
		if (ok) {
			ok = type == util::MT_ALL;
			if (!ok) {
				ok = mount.size() > 1; // Ignore root "/" folder
				if (ok) ok = 0 != util::strncasecmp(mount, "/dev", 4);
				if (ok) ok = 0 != util::strncasecmp(mount, "/run", 4);
				if (ok) ok = 0 != util::strncasecmp(mount, "/sys", 4);
				if (ok) ok = 0 != util::strncasecmp(mount, "/tmp", 4);
				if (ok) ok = 0 != util::strncasecmp(mount, "/proc", 4);
				if (ok) ok = !util::strstr(mount, "/system");
				if (ok) ok = !util::strstr(mount, "/cgroup");
				if (ok) ok = !util::strstr(mount, "/efi");
				if (ok) ok = !util::strstr(mount, "/shm");
				if (ok) ok = !util::strstr(mount, "/aufs");
				if (ok) ok = !util::strstr(mount, "/snap");
				if (ok) ok = !util::strstr(mount, "/.gvfs");
			}
		}
	}
	return ok;
}


size_t util::getMountPoints(util::TStringList& mounts, const EMountType type) {
	mounts.clear();
	size_t size = maxPathSize();
	struct mntent entity;
	struct mntent * result = &entity;
	memset(&entity, 0, sizeof(entity));
	TBuffer buffer(4 * size); // There are 4 path entries in mntent
	TMount browser;
	browser.open();
	if (browser.isOpen()) {
		while (browser.read(&entity, result, buffer)) {
			if (util::assigned(result)) {
				if (util::assigned(result->mnt_dir)) {
					std::string mount(result->mnt_dir);
					if (isValidMountPoint(mount, type)) {
						mounts.add(mount);
					}
				}
			}
		}
	}
	return mounts.size();
}


bool util::mount(const std::string& source, const std::string& target, const std::string& filesystem, const std::string& options, unsigned long flags) {
	int r = EXIT_ERROR;
	if (!source.empty() && !target.empty() && !filesystem.empty()) {
		void* data = NULL;
		if (!options.empty()) {
			data = (void*)options.c_str();
		}
		r = ::mount(source.c_str(), target.c_str(), filesystem.c_str(), flags, data);
	} else {
		errno = EINVAL;
	}
	return r == EXIT_SUCCESS;
}

bool util::umount(const std::string& target, int flags) {
	int r = EXIT_ERROR;
	if (!target.empty()) {
		if (flags > 0) {
			r = ::umount2(target.c_str(), flags);
		} else {
			r = ::umount(target.c_str());
		}
	} else {
		errno = EINVAL;
	}
	return r == EXIT_SUCCESS;
}


bool util::writeFile(const std::string& fileName, const void *const data, const size_t size, const mode_t mode) {
	bool retVal = false;
	if (util::assigned(data) && size > 0) {
		size_t fsz = fileSize(fileName);
		TBaseFile file(fileName);
		if (0 <= file.open((O_WRONLY | O_CREAT), mode)) {
			retVal = ((ssize_t)size == file.write(data, size));
		}
		if (retVal && fsz > size) {
			file.truncate(size);
		}
	}
	return retVal;
}

bool util::writeFile(const std::string& fileName, const std::string& text, const mode_t mode) {
	return writeFile(fileName, text.c_str(), text.size(), mode);
}

ssize_t util::readFile(const std::string& fileName, void *const data, const size_t size) {
	ssize_t retVal = (ssize_t)0;
	if (util::assigned(data) && size > 0) {
		TBaseFile file(fileName);
		if (0 <= file.open(O_RDONLY)) {
			retVal = file.read(data, size);
		}
	}
	return retVal;
}


bool util::setFileOwner(const std::string& fileName, const std::string& userName, const std::string& groupName) {
	if (!fileName.empty()) {
		uid_t uid;
		gid_t gid;
		struct passwd pwd;
		struct group  grp;
		struct passwd *rpwd;
		struct group  *rgrp;
		bool ok = false;
		int r;

		// Get buffer sizes
		size_t pwsize = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (pwsize <= 0)     /* Value was indeterminate */
			pwsize = 16384;  /* Should be more than enough */
		size_t grsize = sysconf(_SC_GETGR_R_SIZE_MAX);
		if (grsize <= 0)     /* Value was indeterminate */
			grsize = pwsize; /* Use same size as user */

		if (!userName.empty()) {
			TBuffer pwbuf(pwsize);
			r = getpwnam_r(userName.c_str(), &pwd, pwbuf(), pwbuf.size(), &rpwd);
			if (EXIT_SUCCESS == r && rpwd != NULL) {
				uid = pwd.pw_uid;
				if (groupName.empty()) {
					// Use group for given user as default
					gid = pwd.pw_gid;
					ok = true;
				} else {
					TBuffer grbuf(grsize);
					r = getgrnam_r(groupName.c_str(), &grp, grbuf(), grbuf.size(), &rgrp);
					if (EXIT_SUCCESS == r && rgrp != NULL) {
						gid = grp.gr_gid;
						ok = true;
					}
				}
				if (ok) {
					do {
						errno = EXIT_SUCCESS;
						r = ::chown(fileName.c_str(), uid, gid);
					} while (EXIT_ERROR == r && EINTR == errno);
					return (EXIT_SUCCESS == r);
				}
			}
		}
	}
	return false;
}


inline int isPathSeperator(char c) {
	unsigned char u = (unsigned char)c;
	if ((u <= USPC) || (u == (unsigned char)sysutil::PATH_SEPERATOR) || (u == (unsigned char)'\\'))
		return true;
	return false;
}

std::string util::stripFirstPathSeparator(const std::string& fileName) {
	// Strip preceeding char '/' for folders
	std::string s = fileName;
	if (!s.empty()) {
		util::trimLeft(s, isPathSeperator);
	}	
	return s;
}

std::string util::stripLastPathSeparator(const std::string& fileName) {
	// Strip ending char '/' for folders
	// Retain root folder --> > 1!
	std::string s = fileName;
	while (s.size() > 1 && isPathSeperator(s[util::pred(s.size())])) {
#ifdef STL_HAS_STRING_POPS
		s.pop_back();
#else
		s.erase(pred(s.size()));
#endif
	}
    return s;
}


std::string util::urlHostName(const std::string& url) {
	std::string s = stripFirstPathSeparator(url);
	size_t pos = 0;
	for (size_t i=0; i<s.size(); ++i) {
		unsigned char u = (unsigned char)s[i];
		if ((u <= USPC) || (u == (unsigned char)sysutil::PATH_SEPERATOR) || (u == (unsigned char)'\\')) {
			pos = i;
			break;
		}
	}
	if (pos > 0) {
		s = s.substr(0, pos);
	}
	return s;
}

std::string util::nfsHostName(const std::string& nfs) {
	std::string s = trim(nfs);
	size_t pos = s.find_first_of(':');
	if (std::string::npos != pos) {
		s = s.substr(0, pos);
	}
	return s;
}


std::string util::fileBaseName(const std::string& fileName) {
    size_t nLastSeparator = fileName.find_last_of(sysutil::PATH_SEPERATOR);
    size_t nLastDot = fileName.find_last_of('.');

    // Test if hidden file without extension after path separator : .../.hiddenfile
    // --> return substring up from dot until the end of string
    if (nLastSeparator != std::string::npos && nLastDot != std::string::npos) {
    	if (nLastDot <= (nLastSeparator + 1)) {
    	    return fileName.substr(nLastSeparator + 1, std::string::npos);
    	}
    }

    // Correct substring calculation for file without extension
    if (nLastDot == std::string::npos || nLastDot == 0)
    	nLastDot = fileName.size();

    return ( (nLastSeparator != std::string::npos)
    			? fileName.substr(nLastSeparator + 1, nLastDot - nLastSeparator - 1)
    			: fileName.substr(0, nLastDot) );
}


std::string util::fileExtName(const std::string& fileName) {
    size_t nLastSeparator = 0;
    return ( ((nLastSeparator = fileName.find_last_of(sysutil::PATH_SEPERATOR)) != std::string::npos)
    			? fileName.substr(nLastSeparator + 1) : fileName );
}


std::string util::filePath(const std::string& fileName) {
    size_t nLastSeparator = 0;
    return ( ((nLastSeparator = fileName.find_last_of(sysutil::PATH_SEPERATOR)) != std::string::npos)
    			? validPath(fileName.substr(0, nLastSeparator)) : "" );
}


std::string util::fileExt(const std::string& fileName) {
    size_t nLastSeparator = fileName.find_last_of(sysutil::PATH_SEPERATOR);
	size_t nLastDot = fileName.find_last_of(".");

	// Test if dot in filename
	if (nLastDot != std::string::npos) {

		// Ignore hidden file without extension like .../.hiddenfile
		if (nLastSeparator != std::string::npos) {
			if (nLastDot <= (nLastSeparator + 1))
				return std::string();
		}

		// Valid file with extension has size greater than 1 char
		if (fileName.size() > 1 && nLastDot > 0) {
			return ( (nLastDot != std::string::npos)
						? fileName.substr(nLastDot + 1) : std::string() );
		}

	}

	return std::string();
}


std::string util::fileReplaceExt(const std::string& fileName, const std::string& ext) {
	// Simple version, but more costly!
	//return filePath(fileName) + fileBaseName(fileName) + "." + ext;

	if (ext.empty())
		return fileName;

	if (fileName.empty())
		return std::string();

	size_t nLastDot = fileName.find_last_of(".");
	if (nLastDot != std::string::npos)
		return fileName.substr(0, nLastDot + 1) + ext;

	return fileName + "." + ext;
}


std::string util::fileReplaceBaseName(const std::string& fileName, const std::string& name) {
	if (name.empty())
		return fileName;

	if (fileName.empty())
		return std::string();

	std::string ext = fileExt(fileName);
	if (!ext.empty())
		return filePath(fileName) + name + "." + ext;

	return filePath(fileName) + name;
}


bool util::fileMatch(const std::string& pattern, const char* fileName, bool tolower) {
	if (pattern.empty() || !util::assigned(fileName))
		return false;
	int match = FNM_NOESCAPE;
	if (tolower) match |= FNM_CASEFOLD;
	return (0 == ::fnmatch(pattern.c_str(), fileName, match));
}

bool util::fileMatch(const std::string& pattern, const std::string& fileName, bool tolower) {
	if (pattern.empty() || fileName.empty())
		return false;
	return fileMatch(pattern, fileName.c_str(), tolower);
}


int util::fileNumber(const std::string& fileName) {
	size_t nLastParent = fileName.find_last_of(")");
	if (nLastParent != std::string::npos) {
		size_t nPredParent = fileName.find_last_of("(", nLastParent);
		if (nPredParent != std::string::npos) {
			if (nPredParent < (nLastParent-3)) {
				std::string v = fileName.substr(util::succ(nPredParent), util::pred(nLastParent));
				if (!v.empty())
					return util::strToInt(v, 0);
			}
		}
	}
	return 0;
}


std::string util::uniqueFileName(const std::string& fileName, const EUniqueName type) {
	std::string s = fileName;
	if (type == UN_TIME || fileExists(s)) {
		int i = 1;
		std::string path = filePath(fileName);
		std::string name = fileBaseName(fileName);
		std::string ext = fileExt(fileName);
		switch (type) {
			case UN_COUNT:
				while (fileExists(s)) {
					s = path + name + "(" + std::to_string((size_s)i) + ")";
					if (!ext.empty())
						s += "." + ext;
					i++;
				}
				break;
			case UN_TIME:
			default:
				s = path + name + "." + sanitizeFileName(dateTimeToStr(now()));
				if (!ext.empty())
					s += "." + ext;
				if (fileExists(s))
					return uniqueFileName(s, UN_COUNT);
				break;
		}
	}
	return s;
}



bool util::fileStatus(const std::string& fileName, struct stat * const status) {
	bool retVal = false;
	if (!fileName.empty() && util::assigned(status)) {
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = ::stat(fileName.c_str(), status);
		} while (r == EXIT_ERROR && errno == EINTR);
		retVal = (EXIT_SUCCESS == r);
	}
	return retVal;
}


bool util::folderStatus(const std::string& directoryName, struct stat * const status) {
	bool retVal = false;
	if (fileStatus(stripLastPathSeparator(directoryName), status)) {
		if (S_ISDIR(status->st_mode))
			retVal = true;
	}
	return retVal;
}


bool util::fileLink(const std::string& fileName, struct stat * const status) {
	bool retVal = false;
	if (!fileName.empty() && util::assigned(status)) {
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = ::lstat(fileName.c_str(), status);
		} while (r == EXIT_ERROR && errno == EINTR);
		retVal = (EXIT_SUCCESS == r);
	}
	return retVal;
}


bool util::linkName(const std::string& fileName, std::string& linkName) {
	linkName.clear();
	bool debug = false;
	bool retVal = false;
	if (!fileName.empty()) {
		std::string file = util::stripPathSeparators(fileName);
		if (debug) std::cout << app::magenta << "util::linkName() Stripped \"" << fileName << "\" --> \"" << file << "\"" << app::reset << std::endl;
		struct stat sb;
		if (fileLink(file, &sb)) {
			bool soft = sb.st_size > 0;
			size_t size = soft ? util::succ(sb.st_size) : util::maxPathSize();
			util::TBuffer link(size + 1);
			ssize_t r = readlink(file.c_str(), link.data(), link.size());
			if (r > 0) {
				linkName = std::string(link.data(), r);
				if (debug) std::cout << app::magenta << "util::linkName() Link name \"" << file << "\" --> \"" << linkName << "\"" << app::reset << std::endl;
				if (soft) {
					std::string realName = util::realName(linkName);
					if (!realName.empty()) {
						linkName = realName;
						if (debug) std::cout << app::magenta << "util::linkName() Real name \"" << file << "\" --> \"" << linkName << "\"" << app::reset << std::endl;
					}
				}
				if (!linkName.empty()) {
					retVal = true;
				}
			} else {
				if (debug) std::cout << app::magenta << "util::linkName() Invalid link size for file \"" << file << "\"" << app::reset << std::endl;
			}
		} else {
			if (debug) std::cout << app::magenta << "util::linkName() No link for file \"" << file << "\"" << app::reset << std::endl;
		}
	}
	return retVal;
}


bool util::fileExists(const std::string& fileName) {
	if (!fileName.empty()) {
		struct stat buf;
		return fileStatus(fileName, &buf);
	}
	return false;
}


bool util::folderExists(const std::string& directoryName) {
	if (!directoryName.empty()) {
		struct stat buf;
		return folderStatus(directoryName, &buf);
	}
	return false;
}


std::size_t util::fileSize(const std::string& fileName) {
	if (!fileName.empty()) {
		struct stat buf;
		return fileStatus(fileName, &buf) ? buf.st_size : (size_t)0;
	}
	return (size_t)0;
}


std::time_t util::fileAge(const std::string& fileName) {
	if (!fileName.empty()) {
		struct stat buf;
		return fileStatus(fileName, &buf) ? buf.st_mtime : (std::time_t)0;
	}
	return (std::time_t)0;
}


bool util::deleteFile(const std::string& fileName) {
	if (!fileName.empty()) {
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = ::remove(fileName.c_str());
		} while (EXIT_SUCCESS != r && EINTR == errno);
		return (EXIT_SUCCESS == r);
	}
	return false;
}

bool util::unlinkFile(const std::string& fileName) {
	if (!fileName.empty()) {
		int r;
		do {
			errno = EXIT_SUCCESS;
			r = ::unlink(fileName.c_str());
		} while (EXIT_SUCCESS != r && EINTR == errno);
		return (EXIT_SUCCESS == r);
	}
	return false;
}


static const std::string STRIP_FILE_CHARS   = " \\/:?\"<>|";
static const std::string REPLACE_FILE_CHARS = "_----'---";

static char replaceFileNameChar(char value) {
	uint8_t u = (uint8_t)value;
	if (u < UINT8_C(128)) {
		size_t idx = STRIP_FILE_CHARS.find(value);
		if(idx != std::string::npos) {
			return REPLACE_FILE_CHARS[idx];
		}
	}
	return value;
}

std::string util::sanitizeFileName(const std::string& fileName) {
    std::string s = fileName;
    std::transform(s.begin(), s.end(), s.begin(), replaceFileNameChar);
    return s;
}


static int isPathChar(char c) {
	unsigned char u = (unsigned char)c;
	return (u <= USPC || c == '/' || c == '\\');
}

std::string util::stripPathSeparators(const std::string& fileName) {
	std::string s = fileName;
	return util::trimRight(s, isPathChar);
}

bool util::copyFile(const std::string& oldFileName, const std::string& newFileName) {
	size_t size = 0;
	return copyFile(oldFileName, newFileName, size);
}


bool util::copyFile(const std::string& oldFileName, const std::string& newFileName, size_t& size) {
	size_t oldSize, newSize;
	struct stat buf;
	bool ok = false;
	try {
	    if (fileStatus(oldFileName, &buf)) {
	    	oldSize = fileSize(oldFileName);
			// Maybe using sendfile() form sys/sendfile.h is more efficient....
	    	// But this is the modern style ;-)
	    	if (oldSize != std::string::npos) {
				std::ifstream ifs(oldFileName, std::ios::binary);
				std::ofstream ofs(newFileName, std::ios::binary);
				ofs << ifs.rdbuf();
				ifs.close();
				ofs.flush();
				ofs.close();
				newSize = fileSize(newFileName);
				ok = (oldSize == newSize);
			}
			if (ok) {
				int r;
				do {
					errno = EXIT_SUCCESS;
					r = ::chmod(newFileName.c_str(), buf.st_mode);
				} while (r == EXIT_ERROR && errno == EINTR);
				ok = EXIT_SUCCESS == r;
			}
		}
	} catch (const std::exception& e) {
		ok = false;
	}
	size = ( ok ? newSize : 0 );
	if (size <= 0)
		deleteFile(newFileName);
	return ok;
}


bool util::sendFile (const std::string& oldFileName, const std::string& newFileName) {
	size_t size = 0;
	return sendFile(oldFileName, newFileName, size);
}

bool util::sendFile(const std::string& oldFileName, const std::string& newFileName, size_t& size) {
	bool ok = false;
	if (true) { //util::isLinux()) {
		size_t oldSize, newSize;
		struct stat buf;
		try	{
			if (fileStatus(oldFileName, &buf)) {
				oldSize = fileSize(oldFileName);
				if (oldSize != std::string::npos) {
					TBaseFile oldFile(oldFileName);
					TBaseFile newFile(newFileName);

					// Template guardian to close files in any case...
					util::TFileGuard<TBaseFile> ofg(oldFile);
					util::TFileGuard<TBaseFile> nfg(newFile);

					// Open old file, create new file
					ofg.open(O_RDONLY);
					nfg.open((O_WRONLY | O_CREAT), buf.st_mode);
					newFile.resize(oldSize);

					// Copy file full file size
					ssize_t r;
					off_t offset = 0;
					do {
						errno = EXIT_SUCCESS;
						r = ::sendfile(newFile(), oldFile(), &offset, oldSize);
					} while (r == (ssize_t)-1 && errno == EINTR);
					if (r > 0) {
						newSize = fileSize(newFileName);
						ok = (oldSize == newSize);
					}
				}
			}
		} catch (const std::exception& e) {
			ok = false;
		}
		size = ( ok ? newSize : 0 );
		if (size <= 0)
			deleteFile(newFileName);
	} else
		ok = copyFile(oldFileName, newFileName, size);
	return ok;
}


bool util::moveFile(const std::string& oldFileName, const std::string& newFileName) {
	if (!fileExists(oldFileName))
		return false;

	int r = __s_rename(oldFileName, newFileName);
	if (r != EXIT_SUCCESS) {
		if (errno == EXDEV) {
			std::string srcFileName = oldFileName + ".org";
			std::string dstFileName = newFileName + ".new";

			r = __s_rename(oldFileName, srcFileName);
			if (r == EXIT_SUCCESS) {
				if (sendFile(srcFileName, dstFileName)) {
					r = __s_rename(dstFileName, newFileName);
				}
			}

			// Delete files in case of error...
			deleteFile(srcFileName);
			deleteFile(dstFileName);
		}
	}
	return (EXIT_SUCCESS == r);
}


void util::validPath(std::string& directoryName) {
	trim(directoryName); // Attention: Beginning and trailing spaces are invalid by doing this!!!
	if (directoryName.size() >= 1)
		if (directoryName[pred(directoryName.size())] != sysutil::PATH_SEPERATOR)
			directoryName += sysutil::PATH_SEPERATOR;
}


std::string util::validPath(const std::string& directoryName) {
	std::string s(directoryName);
	util::validPath(s);
	return s;
}

size_t util::maxPathSize() {
	size_t size = ::pathconf(".", _PC_PATH_MAX);
	if (size == (size_t)-1)   /* Value was indeterminate */
#ifdef PATH_MAX
		size = PATH_MAX;
#else
		size = 4096;
#endif
	return size;
}


std::string util::currenFolder() {
	size_t size = maxPathSize();
	TBuffer buffer;
	std::string retVal = ".";
	char *p;

	buffer.resize(size);
	p = getcwd(buffer.data(), size);
	if (assigned(p))
		retVal = p;

	return retVal;
}


std::string util::realName(const std::string& fileName) {
	if (!fileName.empty()) {
		char *p;
		TBuffer buffer;
		std::string retVal = fileName;
		size_t size = maxPathSize();
		buffer.resize(size);

		p = ::realpath(fileName.c_str(), buffer.data());
		if (assigned(p))
			retVal = p;

		return retVal;
	}
	return std::string();
}

std::string util::realPath(const std::string& directoryName) {
	std::string fileName = stripPathSeparators(directoryName);
	if (!fileName.empty()) {
		std::string retVal = realName(fileName);
		if (!retVal.empty()) {
			validPath(retVal);
			return retVal;
		}
	}
	return std::string();
}



bool util::createDirektory(std::string directoryName, mode_t mode) {
	if (!directoryName.empty()) {
		size_t pre=0, pos;
		std::string dir;
		int retVal;

		validPath(directoryName);

		while((pos = directoryName.find_first_of(sysutil::PATH_SEPERATOR, pre)) != std::string::npos){
			dir = directoryName.substr(0, pos++);
			pre = pos;
			if(dir.size() == 0) continue; // if leading / first time is 0 length
			if((retVal = __s_mkdir(dir, mode)) && errno != EEXIST){
				return (retVal == EXIT_SUCCESS);
			}
		}
		return true;
	}
	return false;
}


bool util::readDirektory(const std::string& path, app::TStringVector& content, const ESearchDepth depth, const bool hidden) {
	bool retVal = false;
	TDirectory dir;
	struct dirent *file;
	if (SD_ROOT == depth)
		content.clear();
	std::string root = validPath(path);
	dir.open(root);
	if (dir.isOpen()) {
		while(assigned((file = dir.read()))) {
			bool ignore = hidden ? 0 == ::strcmp(file->d_name, ".") : file->d_name[0] == '.';
			if(ignore || (0 == ::strcmp(file->d_name, "..")))
				continue;

            if (file->d_type == DT_REG) {
            	content.push_back(root + file->d_name);
            	continue;
            }

            if (SD_RECURSIVE == depth && file->d_type == DT_DIR) {
            	readDirektory(validPath(root + file->d_name), content, depth, hidden);
            }
		}
		retVal = (content.size() > 0);
		dir.close();
	}
	return retVal;
}


bool util::readDirectoryTree(const std::string& path, app::TStringVector& content, const ESearchDepth depth, const bool hidden) {
	bool retVal = false;
	TDirectory dir;
	struct dirent *file;
	if (SD_ROOT == depth)
		content.clear();
	std::string root = validPath(path);
	dir.open(root);
	if (dir.isOpen()) {
		while(assigned((file = dir.read()))) {
			bool ignore = hidden ? 0 == ::strcmp(file->d_name, ".") : file->d_name[0] == '.';
			if(ignore || (0 == ::strcmp(file->d_name, "..")))
				continue;

            if (file->d_type == DT_REG) {
            	continue;
            }

            if (file->d_type == DT_DIR) {
            	content.push_back(root + file->d_name);
            	if (SD_RECURSIVE == depth) {
            		readDirectoryTree(validPath(root + file->d_name), content, depth, hidden);
            	}
            }
		}
		retVal = (content.size() > 0);
		dir.close();
	}
	return retVal;
}


int util::deleteFolders(const std::string& path) {
	std::string root = util::validPath(path);
	TDirectory dir;
	struct dirent *file;
	int r = 0;

	if (!util::folderExists(root))
		return EXIT_ERROR;

	dir.open(root);
	if (dir.isOpen()) {
		while(assigned((file = dir.read()))) {
			bool ignore = 0 == ::strcmp(file->d_name, ".");
			if (ignore || (0 == ::strcmp(file->d_name, "..")))
				continue;

			std::string name = root + file->d_name;
			//std::cout << "util::deleteFolder() Delete file object \"" << name << "\"" << std::endl;

			// Delete folder recursive...
			if (file->d_type == DT_DIR) {
				int t = deleteFolder(name);
				if (t < 0) {
					//std::cout << "util::deleteFolder() Recursive delete failed for \"" << name << "\"" << std::endl;
					return t;
				}
				r += t;
				continue;
			}

			// Delete symbolic link
			if (file->d_type == DT_LNK) {
				if (!unlinkFile(name)) {
					//std::cout << "util::deleteFolder() Delete symbolic link failed for \"" << name << "\"" << std::endl;
					return EXIT_ERROR;
				}
				++r;
				continue;
			}

			// Delete regular file
			if (file->d_type == DT_REG) {
				if (!deleteFile(name)) {
					//std::cout << "util::deleteFolder() Delete file failed for \"" << name << "\"" << std::endl;
					return EXIT_ERROR;
				}
				++r;
				continue;
			}

		}

		// Close root folder
		dir.close();

	}

	return r;
}

int util::deleteFolder(const std::string& path) {
	std::string root = util::validPath(path);

	int r = deleteFolders(root);
	if (EXIT_ERROR != r) {

		// Delete root folder
		if (!deleteFile(root)) {
			//std::cout << "util::deleteFolder() Delete root folder failed for \"" << root << "\"" << std::endl;
			return EXIT_ERROR;
		}

		// Deleted root folder
		++r;
	}

	return r;
}

int util::deleteFiles(const std::string& files, const ESearchDepth depth, const bool hidden) {
	std::string root = util::filePath(files);
	std::string pattern = util::fileExtName(files);
	if (pattern.empty())
		pattern = "*";
	TDirectory dir;
	struct dirent *file;
	util::TStringList folders;
	int r = 0;

	dir.open(root);
	if (dir.isOpen()) {
		while(assigned((file = dir.read()))) {
			bool ignore = hidden ? 0 == ::strcmp(file->d_name, ".") : file->d_name[0] == '.';
			if (ignore || (0 == ::strcmp(file->d_name, "..")))
				continue;

			// Delete file that matches filename
			if (file->d_type == DT_REG) {
				if (fileMatch(pattern, file->d_name)) {
					std::string name = root + file->d_name;
					if (!deleteFile(name))
						return -1;
					++r;
				}	
			}

			// Prepare recursive folder scan
			if (SD_RECURSIVE == depth && file->d_type == DT_DIR) {
				folders.add(util::validPath(root + file->d_name) + pattern);
			}
		}
		dir.close();
	}

	// Scan sub folders
	if (!folders.empty()) {
		for (size_t i=0; i<folders.size(); ++i) {
			int t = deleteFiles(folders[i], depth, hidden);
			if (t < 0)
				return t;
			r += t;
		}
	}

	return r;
}
