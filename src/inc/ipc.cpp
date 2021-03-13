/*
 * ipc.cpp
 *
 *  Created on: 24.06.2018
 *      Author: dirk
 */

#include <limits.h>
#include <sys/eventfd.h>
#include "exception.h"
#include "fileutils.h"
#include "sysutils.h"
#include "random.h"
#include "gcc.h"
#include "ipc.h"

namespace app {

TNotifyEvent::TNotifyEvent() {
	prime();
	open();
}

TNotifyEvent::~TNotifyEvent() {
	close();
}

void TNotifyEvent::prime() {
	event = INVALID_HANDLE_VALUE;
	selected = false;
}

void TNotifyEvent::open() {
	if (event == INVALID_HANDLE_VALUE) {
		errno = EXIT_SUCCESS;
		int r = eventfd(0, EFD_NONBLOCK);
		if (r >= 0) {
			event = r;
		} else {
			throw util::sys_error("TNotifyEvent::open() failed: No event file descriptor available.");
		}
	}
}

void TNotifyEvent::close() {
	if (selected) {
		notify();
		while (selected) util::wait(30);
	}	
	if (event != INVALID_HANDLE_VALUE) {
		::close(event);
		event = INVALID_HANDLE_VALUE;
	}
}

TEventResult TNotifyEvent::wait(util::TTimePart milliseconds) {
	if (event != INVALID_HANDLE_VALUE) {
		selected = true;
		util::TBooleanGuard<bool> bg(selected);
		bool timed = milliseconds > 0;
		fd_set rfds;

		// Watch event file descriptor
		FD_ZERO(&rfds);
		FD_SET(event, &rfds);

		// Wait...
		int r;
		if (timed) {
			r = sigSaveWait(event+1, &rfds, milliseconds);
		} else {
			r = sigSelect(event+1, &rfds);
		}
		selected = false;

		// No file descriptor ready?
		if (r == 0 && timed) {
			return EV_TIMEDOUT;
		}

		// Event file descriptor fired?
		if (r > 0) {
			uint64_t value;
			r = read(event, &value, sizeof(value));
			if (r > 0)
				return EV_SIGNALED;
		}
	}
    return EV_ERROR;
}


TEventResult TNotifyEvent::flush() {
	if (event != INVALID_HANDLE_VALUE) {
		ssize_t r = 0;
		do {
			// Event file descriptor fired?
			uint64_t value;
			r = read(event, &value, sizeof(value));
		} while (r > 0);
		return EV_SUCCESS;
	}
    return EV_ERROR;
}


int TNotifyEvent::sigSelect(int ndfs, fd_set *rfds) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = select(ndfs, rfds, NULL, NULL, NULL);
	} while (r < 0 && errno == EINTR);
	return r;
}

int TNotifyEvent::sigWait(int ndfs, fd_set *rfds, util::TTimePart ms) {
	if (ms > 0) {
		timeval tv;
		tv.tv_sec = ms / 1000;
		tv.tv_usec = (ms % 1000) * 1000;
		errno = EXIT_SUCCESS;
		return select(ndfs, rfds, NULL, NULL, &tv);
	}
	errno = EINVAL;
	return EXIT_ERROR;
}

int TNotifyEvent::sigSaveWait(int ndfs, fd_set *rfds, util::TTimePart ms) {
	util::TDateTime ts;
	util::TTimePart tm = ms, dt = 0;
	int r = EXIT_SUCCESS, c = 0;
	do {
		ts.start();
		r = sigWait(ndfs, rfds, tm);
		if (r < 0 && errno == EINTR) {
			// select() was interrupted
			dt = ts.stop(util::ETP_MILLISEC);
			if (dt > (tm * 95 / 100))
				break;
			tm -= dt;
			if (tm < 5)
				break;
			c++;
		} else {
			// select() returned on error!
			break;
		}
	} while (c < 10);
	return r;
}

TEventResult TNotifyEvent::notify() {
	if (event != INVALID_HANDLE_VALUE) {
    	uint64_t value = 1;
    	int r = write(event, &value, sizeof(value));
    	if (r > 0)
    		return EV_SUCCESS;
	}
	return EV_ERROR;
}



TFileWatch::TFileWatch() {
	prime();
}

TFileWatch::~TFileWatch() {
	close();
}

void TFileWatch::prime() {
	watch = INVALID_HANDLE_VALUE;
	errval = EXIT_SUCCESS;
	selected = false;
	debug = false;
	test = 0;
}

void TFileWatch::open() {
	if (!isOpen()) {
		errno = EXIT_SUCCESS;
		int r = inotify_init();
		if (r < 0)
			throw util::sys_error("TFileWatch::open() failed: No notify file descriptor available.");
		watch = r;
		createDefaultWatch();
	}
}

void TFileWatch::close() {
	if (isOpen()) {
		if (selected) {
			notify();
			while (selected) util::wait(30);
		}
		errno = EXIT_SUCCESS;
		::close(watch);
		watch = INVALID_HANDLE_VALUE;
		errval = errno;
		watches.clear();
		deleteDefaultWatch();
	}
}

void TFileWatch::reopen() {
	close();
	open();
}

std::string TFileWatch::getLastErrorMessage() {
	return "Watch file <" + errFileName + "> caused error \"" + sysutil::getSysErrorMessage(errval) + "\"";
}


void TFileWatch::createDefaultWatch() {
	defFileName = "/tmp/.INOTIFY-" + util::fastCreateUUID(true, true);
	std::string s = "INOTIFY-CREATED\n";
	if (!util::writeFile(defFileName, s))
		throw util::sys_error("TFileWatch::createDefaultWatch() Writing default file failed.");
	defFileWatch = addFile(defFileName);
	if (INVALID_HANDLE_VALUE == defFileWatch)
		throw util::sys_error("TFileWatch::createDefaultWatch() Adding default file failed.");
}

void TFileWatch::changeDefaultWatch() {
	std::string s = "INOTIFY-FIRED\n";
	if (!util::writeFile(defFileName, s))
		throw util::sys_error("TFileWatch::changeDefaultWatch() failed.");
}

void TFileWatch::deleteDefaultWatch() {
	util::deleteFile(defFileName);
}


size_t TFileWatch::browser(const std::string& folder, util::TStringList& folders) {
	folders.clear();
	util::TFolderList browser;
	int count = browser.scan(folder, "", util::SD_RECURSIVE, false);
	if (count > 0) {
		for (size_t i=0; i<browser.size(); ++i) {
			util::PFile file = browser[i];
			if (util::assigned(file)) {
				if (file->isFolder()) {
					folders.add(util::validPath(file->getFile()));
				}
			}
		}
	}
	return folders.size();
}


void TFileWatch::rebuild() {

	// Find all primary watches from active watch list
	int wd = 0;
	TFileWatchMap list;
	TFileWatchMap::const_iterator it = watches.begin();
	while (it != watches.end()) {
		if (it->second.generation == 1) {
			if (debug) {
				std::string type = (util::FT_FILE == it->second.type) ? "file" : "folder";
				std::cout << "TFileWatch::rebuild() Save " << type << " watch for <" << it->second.file << ">" << std::endl;
			}
			list[++wd] = it->second;
		}
		++it;
	}

	// Reopen file watch
	reopen();

	// Add primary watches
	if (isOpen()) {
		TFileWatchMap::const_iterator it = list.begin();
		while (it != list.end()) {
			const TWatchMapItem& watch = it->second;
			if (util::FT_FILE == watch.type) {
				if (debug)
					std::cout << "TFileWatch::rebuild() Restore file watch for <" << watch.file << ">" << std::endl;
				addWatch(watch.file, watch.depth, watch.flags, 1, util::FT_FILE);
			} else {
				if (debug)
					std::cout << "TFileWatch::rebuild() Restore folder watch for <" << watch.file << ">" << std::endl;
				addPath(watch.file, watch.depth, watch.flags, 1);
			}
			++it;
		}
	}
}


int TFileWatch::addWatch(const std::string& file, const util::ESearchDepth depth, const int flags, const int generation, const util::EFileType type) {
	errno = EINVAL;
	if (!file.empty()) {
		errFileName = file;
		return addWatch(file.c_str(), depth, flags, generation, type);
	}
	return INVALID_HANDLE_VALUE;
}

int TFileWatch::addWatch(const char* file, const util::ESearchDepth depth, const int flags, const int generation, const util::EFileType type) {
	errno = EINVAL;
	if (util::assigned(file)) {
		errno = EXIT_SUCCESS;
		int wd = inotify_add_watch(watch, file, flags);
		if (wd >= 0) {
			errval = EXIT_SUCCESS;
			TWatchMapItem watch;
			watch.file = file;
			watch.type = type;
			watch.depth = depth;
			watch.flags = flags;
			watch.generation = generation;
			watches[wd] = watch;
			return wd;
		}
	}
	errval = errno;
	return INVALID_HANDLE_VALUE;
}

int TFileWatch::addPath(const std::string& file, const util::ESearchDepth depth, const int flags, const int generation) {
	int fd = addWatch(file, depth, flags, generation, util::FT_FOLDER);
	if (INVALID_HANDLE_VALUE != fd) {
		if (util::SD_RECURSIVE == depth) {
			// Add recursive subfolder
			size_t added = 0;
			util::TStringList folders;
			size_t scanned = browser(file, folders);
			if (scanned > 0) {
				size_t limit = sysutil::getWatchFileLimit() * 9 / 10;
				if ((scanned + watches.size()) < limit) {
					for (size_t i=0; i<folders.size(); ++i) {
						if (INVALID_HANDLE_VALUE == addWatch(folders[i], depth, flags, generation + 1, util::FT_FOLDER)) {
							return INVALID_HANDLE_VALUE;
						}
						++added;
					}
				} else {
					// Too many files to add to watch list
					errno = errval = E2BIG;
					return INVALID_HANDLE_VALUE;
				}
			}
		}
	}
	return fd;
}

bool TFileWatch::addFile(const std::string& fileName, const int flags) {
	std::string path = util::stripLastPathSeparator(fileName);
	return (INVALID_HANDLE_VALUE != addWatch(fileName, util::SD_ROOT, flags, 1, util::FT_FILE));
}

bool TFileWatch::addFolder(const std::string& path, const util::ESearchDepth depth, const int flags) {
	std::string root = util::validPath(path);
	return (INVALID_HANDLE_VALUE != addPath(root, depth, flags, 1));
}


int TFileWatch::select(int ndfs, fd_set *rfds) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::select(ndfs, rfds, NULL, NULL, NULL);
	} while (r < 0 && errno == EINTR);
	errval = errno;
	return r;
}

bool TFileWatch::wait() {
	if (isOpen()) {
		selected = true;
		util::TBooleanGuard<bool> bg(selected);
		fd_set rfds;

		// Watch event file descriptor
		FD_ZERO(&rfds);
		FD_SET(watch, &rfds);

		// Event file descriptor fired?
		int r = select(watch+1, &rfds);
		if (r > 0 && FD_ISSET(watch, &rfds)) {
			return true;
		}
	}
    return false;
}

#ifndef WATCH_BUFFER_SIZE
#  define WATCH_BUFFER_SIZE (WATCH_BUFFER_COUNT * (sizeof(struct inotify_event) + NAME_MAX + 1))
#endif

TEventResult TFileWatch::read(util::TStringList& files) {
	files.clear();
	char buf[WATCH_BUFFER_SIZE] __attribute__ ((aligned(8)));
	int r = ::read(watch, buf, WATCH_BUFFER_SIZE);
	if (r > 0) {
		// Process all of the events in buffer returned by read()
		char *p;
		struct inotify_event *event;
		for (p = buf; p < buf + r; ) {
			event = (struct inotify_event *)p;
			TEventResult retVal = processEvent(event, files);
			p += sizeof(struct inotify_event) + event->len;
			if (retVal == EV_TERMINATE)
				return EV_TERMINATE;
			if (retVal == EV_ERROR)
				break;
		}
	}
	if (!files.empty())
		return EV_SIGNALED;
    return EV_SUCCESS;
}


bool TFileWatch::getFileFromEvent(struct inotify_event *event, std::string& file, TWatchMapItem& watch) {
	bool retVal = false;
	int wd = event->wd;
	if (event->len > 0) {
		size_t size = strnlen(event->name, event->len);
		if (size > 0) {
			TFileWatchMap::const_iterator it = watches.find(wd);
			if (it != watches.end()) {
				watch = it->second;
				file = watch.file + std::string(event->name, size);
				retVal = true;
			}
		}
	} else {
		TFileWatchMap::const_iterator it = watches.find(wd);
		if (it != watches.end()) {
			watch = it->second;
			file = watch.file;
			retVal = true;
		}
	}
	return retVal;
}


TEventResult TFileWatch::processEvent(struct inotify_event *event, util::TStringList& files) {
	bool goon = true;
	TEventResult retVal = EV_SUCCESS;

	// Do some debugging output...
	if (debug) {
		std::cout << "TFileWatch::processEvent() New event raised: ";
		if (event->mask & IN_ACCESS)        std::cout << "IN_ACCESS ";
		if (event->mask & IN_ATTRIB)        std::cout << "IN_ATTRIB ";
		if (event->mask & IN_CLOSE_NOWRITE) std::cout << "IN_CLOSE_NOWRITE ";
		if (event->mask & IN_CLOSE_WRITE)   std::cout << "IN_CLOSE_WRITE ";
		if (event->mask & IN_CREATE)        std::cout << "IN_CREATE ";
		if (event->mask & IN_DELETE)        std::cout << "IN_DELETE ";
		if (event->mask & IN_DELETE_SELF)   std::cout << "IN_DELETE_SELF ";
		if (event->mask & IN_IGNORED)       std::cout << "IN_IGNORED ";
		if (event->mask & IN_ISDIR)         std::cout << "IN_ISDIR ";
		if (event->mask & IN_MODIFY)        std::cout << "IN_MODIFY ";
		if (event->mask & IN_MOVE_SELF)     std::cout << "IN_MOVE_SELF ";
		if (event->mask & IN_MOVED_FROM)    std::cout << "IN_MOVED_FROM ";
		if (event->mask & IN_MOVED_TO)      std::cout << "IN_MOVED_TO ";
		if (event->mask & IN_OPEN)          std::cout << "IN_OPEN ";
		if (event->mask & IN_Q_OVERFLOW)    std::cout << "IN_Q_OVERFLOW ";
		if (event->mask & IN_UNMOUNT)       std::cout << "IN_UNMOUNT ";
		std::cout << std::endl;
	}

	// Do rebuild test in debug mode
	if (debug) {
		++test;
		if (test > 5) {
			std::cout << "TFileWatch::processEvent() Rebuild watch (Test)" << std::endl;
			debug = true;
			rebuild();
			test = 0;
			retVal = EV_ERROR;
			goon = false;
		}
	}

	// Reopen file watch on queue overflow
	if (event->mask & IN_Q_OVERFLOW) {
		rebuild();
		retVal = EV_ERROR;
		goon = false;
	}

	// File no longer existent, remove from watch list...
	if (event->mask & IN_IGNORED) {
		int wd = event->wd;
		TFileWatchMap::const_iterator it = watches.find(wd);
		if (it != watches.end())
			watches.erase(it);
		if (event->mask == IN_IGNORED)
			goon = false;
	}

	// New folder found, add to watch list if owners watch is recursive
	if (event->mask & IN_CREATE) {
		if (event->mask & IN_ISDIR) {
			std::string path;
			TWatchMapItem watch;
			if (getFileFromEvent(event, path, watch)) {
				if (util::SD_RECURSIVE == watch.depth) {
					util::validPath(path);
					if (!path.empty()) {
						addPath(path, watch.depth, watch.flags, watch.generation + 1);
						if (debug)
							std::cout << "TFileWatch::processEvent() Add new folder <" << path << ">" << std::endl;
					}
				}
			}
		}
		goon = false;
	}

	// Ignore deletion of folder, IN_DELETE_SELF detected before...
	if (goon && (event->mask & IN_DELETE) && (event->mask & IN_ISDIR)) {
		goon = false;
	}

	// Proceed with events handling
	if (goon) {
		int wd = event->wd;
		if (wd != defFileWatch) {
			std::string file;
			TWatchMapItem watch;
			getFileFromEvent(event, file, watch);
			if (!file.empty()) {
				if (std::string::npos == files.find(file, util::EC_COMPARE_FULL)) {
					files.add(file);
					retVal = EV_SIGNALED;
					if (debug)
						std::cout << "TFileWatch::processEvent() Add file <" << file << ">" << std::endl;
				}
			}
		} else {
			// Event was raised by default watch
			retVal = EV_TERMINATE;
		}
	}
	return retVal;
}


void TFileWatch::notify() {
	if (isOpen()) {
		changeDefaultWatch();
	}
}


} /* namespace app */
