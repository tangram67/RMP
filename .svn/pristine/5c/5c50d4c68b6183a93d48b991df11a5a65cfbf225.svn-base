/*
 * ipc.h
 *
 *  Created on: 24.06.2018
 *      Author: dirk
 */

#ifndef INC_IPC_H_
#define INC_IPC_H_

#include <sys/inotify.h>
#include "filetypes.h"
#include "ipctypes.h"
#include "classes.h"
#include "datetime.h"
#include "stringutils.h"


namespace app {

#ifndef ALL_WATCH_EVENTS
#  define ALL_WATCH_EVENTS (IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE \
			  | IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM \
			  | IN_MOVED_TO | IN_CREATE | IN_DELETE \
			  | IN_DELETE_SELF | IN_MOVE_SELF)
#endif

#ifndef WRITE_WATCH_EVENTS
#  define WRITE_WATCH_EVENTS (IN_CLOSE_WRITE \
			  | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE | IN_DELETE	\
			  | IN_DELETE_SELF | IN_MOVE_SELF)
#endif

#ifndef READ_WATCH_EVENTS
#  define READ_WATCH_EVENTS (IN_CLOSE_NOWRITE | IN_CLOSE_WRITE \
			  | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE | IN_DELETE \
			  | IN_DELETE_SELF | IN_MOVE_SELF)
#endif

#ifndef MODIFY_WATCH_EVENTS
#  define MODIFY_WATCH_EVENTS (WRITE_WATCH_EVENTS | IN_ATTRIB)
#endif

#ifndef WRITE_COMPLETED_WATCH_EVENTS
#  define WRITE_COMPLETED_WATCH_EVENTS (IN_CLOSE_WRITE)
#endif

#ifndef DEFAULT_WATCH_EVENTS
#  define DEFAULT_WATCH_EVENTS WRITE_WATCH_EVENTS
#endif


#ifndef WATCH_BUFFER_COUNT
#  define WATCH_BUFFER_COUNT (32)
#endif


typedef struct CWatchMapItem {
	std::string file;
	util::ESearchDepth depth;
	util::EFileType type;
	int generation;
	int flags;

	CWatchMapItem() {
		depth = util::SD_ROOT;
		type = util::FT_UNDEFINED;
		generation = 0;
		flags = 0;
	}
} TWatchMapItem;


#ifdef STL_HAS_TEMPLATE_ALIAS

using TFileWatchMap = std::map<int, TWatchMapItem>;
using TFileWatchMapItem = std::pair<int, TWatchMapItem>;

#else

typedef std::map<int, TWatchMapItem> TFileWatchMap;
typedef std::pair<int, TWatchMapItem> TFileWatchMapItem;

#endif


class TNotifyEvent {
private:
	int event;
	bool selected;
	
	void prime();
	void open();
	void close();

	int sigWait(int ndfs, fd_set *rfds, util::TTimePart ms);
	int sigSaveWait(int ndfs, fd_set *rfds, util::TTimePart ms);
	int sigSelect(int ndfs, fd_set *rfds);

public:
	TEventResult notify();
	TEventResult wait(util::TTimePart milliseconds = 0);
	TEventResult flush();

	TNotifyEvent();
	virtual ~TNotifyEvent();
};


class TFileWatch {
private:
	int watch;
	int errval;
	bool debug;
	TFileWatchMap watches;
	std::string defFileName;
	std::string errFileName;
	int defFileWatch;
	bool selected;
	int test;

	void prime();
	void reopen();

	void createDefaultWatch();
	void changeDefaultWatch();
	void deleteDefaultWatch();

	TEventResult processEvent(struct inotify_event *event, util::TStringList& files);
	bool getFileFromEvent(struct inotify_event *event, std::string& file, TWatchMapItem& watch);

	int addWatch(const char* file, const util::ESearchDepth depth, const int flags, const int generation, const util::EFileType type);
	int addWatch(const std::string& file, const util::ESearchDepth depth, const int flags, const int generation, const util::EFileType type);
	int addPath(const std::string& file, const util::ESearchDepth depth, const int flags, const int generation);

	size_t browser(const std::string& folder, util::TStringList& folders);
	int select(int ndfs, fd_set *rfds);
	void rebuild();

public:
	void open();
	void close();
	bool isOpen() const { return INVALID_HANDLE_VALUE != watch; };

	bool addFile(const std::string& fileName, const int flags = DEFAULT_WATCH_EVENTS);
	bool addFolder(const std::string& path, const util::ESearchDepth depth = util::SD_ROOT, const int flags = DEFAULT_WATCH_EVENTS);

	std::string getLastErrorMessage();

	bool wait();
	TEventResult read(util::TStringList& files);
	void notify();

	TFileWatch();
	virtual ~TFileWatch();
};

} /* namespace app */

#endif /* INC_IPC_H_ */
