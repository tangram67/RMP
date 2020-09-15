/*
 * controltypes.h
 *
 *  Created on: 01.05.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef CONTROLTYPES_H_
#define CONTROLTYPES_H_

#include "../inc/gcc.h"
#include "../inc/classes.h"
#include "../inc/templates.h"
#include "../inc/datetime.h"
#include <string>
#include <vector>
#include <map>

namespace app {

enum ECommandAction {
	ECA_INVALID_ACTION = 0,

	ECA_APP_EXIT   = 1000,
	ECA_APP_UPDATE = 1001,

	ECA_PLAYER_PLAY  = 1005,
	ECA_PLAYER_PAUSE = 1006,
	ECA_PLAYER_STOP  = 1007,
	ECA_PLAYER_FF    = 1008,
	ECA_PLAYER_FR    = 1009,
	ECA_PLAYER_NEXT  = 1010,
	ECA_PLAYER_PREV  = 1011,
	ECA_PLAYER_RAND  = 1012,

	ECA_PLAYLIST_ADD_SONG      = 2000,
	ECA_PLAYLIST_PLAY_SONG     = 2001,
	ECA_PLAYLIST_NEXT_SONG     = 2002,
	ECA_PLAYLIST_DELETE_SONG   = 2003,
	ECA_PLAYLIST_ADD_ALBUM     = 2010,
	ECA_PLAYLIST_PLAY_ALBUM    = 2011,
	ECA_PLAYLIST_NEXT_ALBUM    = 2012,
	ECA_PLAYLIST_DELETE_ALBUM  = 2013,
	ECA_PLAYLIST_ADD_ARTIST    = 2014,

	ECA_PLAYLIST_ADD_PLAY_SONG   = 2020,
	ECA_PLAYLIST_ADD_PLAY_ALBUM  = 2021,
	ECA_PLAYLIST_ADD_PLAY_ARTIST = 2022,
	ECA_PLAYLIST_ADD_NEXT_SONG   = 2023,
	ECA_PLAYLIST_ADD_NEXT_ALBUM  = 2024,

	ECA_LIBRARY_CLEAR_IMAGE     = 3000,
	ECA_LIBRARY_CLEAR_CACHE     = 3001,
	ECA_LIBRARY_CLEAR_LIBRARY   = 3002,
	ECA_LIBRARY_RESCAN_LIBRARY  = 3003,
	ECA_LIBRARY_REBUILD_LIBRARY = 3004,

	ECA_SYSTEM_RESCAN_AUDIO       = 4000,
	ECA_SYSTEM_RESCAN_MOUNTS      = 4001,
	ECA_SYSTEM_RESCAN_PORTS       = 4002,
	ECA_SYSTEM_REFRESH_DEVICE     = 4003,
	ECA_SYSTEM_MOUNT_FILESYSTEM   = 4004,
	ECA_SYSTEM_UNMOUNT_FILESYSTEM = 4005,
	ECA_SYSTEM_LOAD_LOGFILES      = 4006,
	ECA_SYSTEM_LOAD_LICENSE       = 4007,
	ECA_SYSTEM_EXEC_DEFRAG        = 4008,
	
	ECA_PLAYER_MODE_RANDOM = 5000,
	ECA_PLAYER_MODE_REPEAT = 5001,
	ECA_PLAYER_MODE_SINGLE = 5002,
	ECA_PLAYER_MODE_DIRECT = 5003,
	ECA_PLAYER_MODE_HALT   = 5004,
	ECA_PLAYER_MODE_DISK   = 5005,

	ECA_REMOTE_48K = 6000,
	ECA_REMOTE_44K = 6001,

	ECA_REMOTE_INPUT0 = 6010,
	ECA_REMOTE_INPUT1 = 6011,
	ECA_REMOTE_INPUT2 = 6012,
	ECA_REMOTE_INPUT3 = 6013,
	ECA_REMOTE_INPUT4 = 6014,

	ECA_REMOTE_MUTE   = 6020,

	ECA_REMOTE_FILTER0 = 6030,
	ECA_REMOTE_FILTER1 = 6031,
	ECA_REMOTE_FILTER2 = 6032,
	ECA_REMOTE_FILTER3 = 6033,
	ECA_REMOTE_FILTER4 = 6034,
	ECA_REMOTE_FILTER5 = 6035,

	ECA_REMOTE_PHASE0  = 6040,
	ECA_REMOTE_PHASE1  = 6041,

	ECA_OPTION_RED    = 7000,
	ECA_OPTION_GREEN  = 7001,
	ECA_OPTION_YELLOW = 7002,
	ECA_OPTION_BLUE   = 7003,

	ECA_STREAM_PLAY  = 8001,
	ECA_STREAM_PAUSE = 8002,
	ECA_STREAM_STOP  = 8003,
	ECA_STREAM_ABORT = 8004,
	ECA_STREAM_UNDO  = 8005
};

enum ECommandState {
	ECS_IDLE,
	ECS_WAIT,
	ECS_BUSY,
	ECS_READY,
	ECS_FINISHED
};

typedef struct CCommand {
	std::string title;
	std::string album;
	std::string artist;
	std::string file;
	std::string playlist;
	ECommandAction action;
	ECommandState state;
	util::TTimePart delay;

	void setDelay(const util::TTimePart value) {
		delay = util::now() + value;
	}

	bool hasDelay() const {
		return (delay > (util::TTimePart)0);
	}

	bool isDelayed() const {
		if (delay > (util::TTimePart)0)
			return util::now() > delay;
		return false;
	}

	void prime() {
		action = ECA_INVALID_ACTION;
		state = ECS_IDLE;
		delay =  (util::TTimePart)0;
	}
	void clear() {
		if (!title.empty()) title.clear();
		if (!album.empty()) album.clear();
		if (!artist.empty()) artist.clear();
		if (!file.empty()) file.clear();
		if (!playlist.empty()) playlist.clear();
		prime();
	}

	CCommand() { prime(); }

} TCommand;


#ifdef STL_HAS_TEMPLATE_ALIAS

using PCommand = TCommand*;
using TCommandList = std::vector<PCommand>;
using TActionNames = std::map<std::string, ECommandAction>;
using TActionNamesItem = std::pair<std::string, ECommandAction>;
using TActionValues = std::map<ECommandAction, std::string>;
using TActionvaluesItem = std::pair<ECommandAction, std::string>;
using TMapInitFunction = std::function<TActionNames()>;

#else

typedef TCommand* PCommand;
typedef std::vector<PCommand> TCommandList;
typedef std::map<std::string, ECommandAction> TActionNames;
typedef std::pair<std::string, ECommandAction> TActionNamesItem;
typedef std::map<ECommandAction, std::string> TActionValues;
typedef std::pair<ECommandAction, std::string> TActionvaluesItem;
typedef std::function<TActionNames()> TMapInitFunction;

#endif


class TBaseMap : public app::TObject {
private:
	std::string defVal;
	TActionNames map;
	TActionValues values;

	void fillValuesMap();

protected:
	void init(TMapInitFunction fn);
	virtual void prime() = 0;

public:
	typedef TActionNames::const_iterator const_iterator;

	void clear();
	void add(const std::string& value, ECommandAction action);
	ECommandAction getAction(const std::string& value) const;
	const std::string& getValue(const ECommandAction value) const;

	inline const_iterator begin() const { return map.begin(); };
	inline const_iterator end() const { return map.end(); };
	inline const_iterator first() const { return begin(); };
	inline const_iterator last() const { return util::pred(end()); };

	bool empty() const { return map.empty(); };
	operator bool () const { return !empty(); };
	ECommandAction operator[] (const std::string& value) const;

	TBaseMap();
	virtual ~TBaseMap();
};


class TActionMap : public TBaseMap {
private:
	void prime();

public:
	TActionMap() : TBaseMap() { prime(); };
	virtual ~TActionMap() = default;
};

class TControlMap : public TBaseMap {
private:
	void prime();

public:
	TControlMap() : TBaseMap() { prime(); };
	virtual ~TControlMap() = default;
};

} /* namespace app */

#endif /* CONTROLTYPES_H_ */
