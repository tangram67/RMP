/*
 * controltypes.cpp
 *
 *  Created on: 01.05.2017
 *      Author: Dirk Brinkmeier
 */

#include "controltypes.h"
#include "../inc/exception.h"

namespace app {

TActionNames fillActionMap() {
	TActionNames map;

	// Assign tags to different event values from web actions
	map["EXIT"]   = ECA_APP_EXIT;
	map["UPDATE"] = ECA_APP_UPDATE;

	// Add player actions
	map["PLAY"]  = ECA_PLAYER_PLAY;
	map["PAUSE"] = ECA_PLAYER_PAUSE;
	map["STOP"]  = ECA_PLAYER_STOP;
	map["FF"]    = ECA_PLAYER_FF;
	map["FR"]    = ECA_PLAYER_FR;
	map["NEXT"]  = ECA_PLAYER_NEXT;
	map["PREV"]  = ECA_PLAYER_PREV;
	map["RAND"]  = ECA_PLAYER_RAND;

	// Add playlist tags to web action list
	map["ADDSONG"]    = ECA_PLAYLIST_ADD_SONG;
	map["PLAYSONG"]   = ECA_PLAYLIST_PLAY_SONG;
	map["NEXTSONG"]   = ECA_PLAYLIST_NEXT_SONG;
	map["DELETESONG"] = ECA_PLAYLIST_DELETE_SONG;

	map["ADDALBUM"]    = ECA_PLAYLIST_ADD_ALBUM;
	map["PLAYALBUM"]   = ECA_PLAYLIST_PLAY_ALBUM;
	map["NEXTALBUM"]   = ECA_PLAYLIST_NEXT_ALBUM;
	map["DELETEALBUM"] = ECA_PLAYLIST_DELETE_ALBUM;
	map["ADDARTIST"]   = ECA_PLAYLIST_ADD_ARTIST;

	// Add library tags to web action list
	map["ADDPLAYSONG"]   = ECA_PLAYLIST_ADD_PLAY_SONG;
	map["ADDPLAYALBUM"]  = ECA_PLAYLIST_ADD_PLAY_ALBUM;
	map["ADDPLAYARTIST"] = ECA_PLAYLIST_ADD_PLAY_ARTIST;
	map["ADDNEXTSONG"]   = ECA_PLAYLIST_ADD_NEXT_SONG;
	map["ADDNEXTALBUM"]  = ECA_PLAYLIST_ADD_NEXT_ALBUM;

	// Add image actions
	map["CLEARIMAGE"] = ECA_LIBRARY_CLEAR_IMAGE;
	map["CLEARCACHE"] = ECA_LIBRARY_CLEAR_CACHE;

	// Hardware scanner actions
	map["SCANAUDIO"]     = ECA_SYSTEM_RESCAN_AUDIO;
	map["SCANDRIVES"]    = ECA_SYSTEM_RESCAN_MOUNTS;
	map["SCANPORTS"]     = ECA_SYSTEM_RESCAN_PORTS;
	map["DEVICEREFRESH"] = ECA_SYSTEM_REFRESH_DEVICE;

	// Add system actions
	map["LOADLOG"]     = ECA_SYSTEM_LOAD_LOGFILES;
	map["MOUNTFS"]     = ECA_SYSTEM_MOUNT_FILESYSTEM;
	map["UNMOUNTFS"]   = ECA_SYSTEM_UNMOUNT_FILESYSTEM;
	map["EXECDEFRAG"]  = ECA_SYSTEM_EXEC_DEFRAG;
	map["LOADLICENSE"] = ECA_SYSTEM_LOAD_LICENSE;

	// Add settings actions
	map["RESCAN"]  = ECA_LIBRARY_RESCAN_LIBRARY;
	map["REBUILD"] = ECA_LIBRARY_REBUILD_LIBRARY;
	map["CLEAR"]   = ECA_LIBRARY_CLEAR_LIBRARY;

	// Add player repeat actions
	map["RANDOM"] = ECA_PLAYER_MODE_RANDOM;
	map["REPEAT"] = ECA_PLAYER_MODE_REPEAT;
	map["SINGLE"] = ECA_PLAYER_MODE_SINGLE;
	map["DIRECT"] = ECA_PLAYER_MODE_DIRECT;
	map["HALT"]   = ECA_PLAYER_MODE_HALT;
	map["DISK"]   = ECA_PLAYER_MODE_DISK;

	// Add remote command actions
	map["SR48K"] = ECA_REMOTE_48K;
	map["SR44K"] = ECA_REMOTE_44K;

	map["INPUT0"] = ECA_REMOTE_INPUT0;
	map["INPUT1"] = ECA_REMOTE_INPUT1;
	map["INPUT2"] = ECA_REMOTE_INPUT2;
	map["INPUT3"] = ECA_REMOTE_INPUT3;
	map["INPUT4"] = ECA_REMOTE_INPUT4;

	map["MUTE"]  = ECA_REMOTE_MUTE;

	map["PHASE0"] = ECA_REMOTE_PHASE0;
	map["PHASE1"] = ECA_REMOTE_PHASE1;

	map["FILTER0"] = ECA_REMOTE_FILTER0;
	map["FILTER1"] = ECA_REMOTE_FILTER1;
	map["FILTER2"] = ECA_REMOTE_FILTER2;
	map["FILTER3"] = ECA_REMOTE_FILTER3;
	map["FILTER4"] = ECA_REMOTE_FILTER4;
	map["FILTER5"] = ECA_REMOTE_FILTER5;

	// Optional command codes
	map["RED"]    = ECA_OPTION_RED;
	map["GREEN"]  = ECA_OPTION_GREEN;
	map["YELLOW"] = ECA_OPTION_YELLOW;
	map["BLUE"]   = ECA_OPTION_BLUE;

	// Internet radio command codes
	map["PLAYSTREAM"]   = ECA_STREAM_PLAY;
	map["PAUSESTREAM"]  = ECA_STREAM_PAUSE;
	map["STOPSTREAM"]   = ECA_STREAM_STOP;
	map["ABORTSTREAM"]  = ECA_STREAM_ABORT;
	map["REVERTSTREAM"] = ECA_STREAM_UNDO;

	return map;
}

TActionNames fillControlMap() {
	TActionNames map;

	// Add player actions
	map["PLAY"]    = ECA_PLAYER_PLAY;
	map["PAUSE"]   = ECA_PLAYER_PAUSE;
	map["STOP"]    = ECA_PLAYER_STOP;
	map["FORWARD"] = ECA_PLAYER_FF;
	map["REWIND"]  = ECA_PLAYER_FR;
	map["SKIP"]    = ECA_PLAYER_NEXT;
	map["REPLAY"]  = ECA_PLAYER_PREV;

	// Special key assignments
	map["RIGHT"]   = ECA_PLAYER_RAND;
	map["MUTE"]    = ECA_PLAYER_PAUSE;
	map["VOLDOWN"] = ECA_PLAYER_PAUSE;

	// Add remote command actions
	map["CHANUP"]   = ECA_REMOTE_48K;
	map["CHANDOWN"] = ECA_REMOTE_44K;

	map["ONE"]   = ECA_REMOTE_INPUT0;
	map["TWO"]   = ECA_REMOTE_INPUT1;
	map["THREE"] = ECA_REMOTE_INPUT2;
	map["FOUR"]  = ECA_REMOTE_INPUT3;
	map["FIVE"]  = ECA_REMOTE_INPUT4;

	// Optional command codes
	map["RED"]    = ECA_OPTION_RED;
	map["GREEN"]  = ECA_OPTION_GREEN;
	map["YELLOW"] = ECA_OPTION_YELLOW;
	map["BLUE"]   = ECA_OPTION_BLUE;

	// Internet radio command codes
	map["PLAYSTREAM"]  = ECA_STREAM_PLAY;
	map["PAUSESTREAM"] = ECA_STREAM_PAUSE;
	map["STOPSTREAM"]  = ECA_STREAM_STOP;
	map["ABORTSTREAM"] = ECA_STREAM_ABORT;

	return map;
}



TBaseMap::TBaseMap() {
	defVal = "INVALID";
}

TBaseMap::~TBaseMap() {
	clear();
}


void TBaseMap::clear() {
	map.clear();
	values.clear();
}

void TBaseMap::init(TMapInitFunction fn) {
	map = fn();
	fillValuesMap();
}

void TBaseMap::fillValuesMap() {
	const_iterator it = map.begin();
	while (it != map.end()) {
		values[it->second] = it->first;
		++it;
	}
}

const std::string& TBaseMap::getValue(const ECommandAction value) const {
	TActionValues::const_iterator it = values.find(value);
	if (it != values.end())
		return it->second;
	return defVal;
}

ECommandAction TBaseMap::getAction(const std::string& value) const {
	const_iterator it = map.find(value);
	if (it != end())
		return it->second;
	return ECA_INVALID_ACTION;
}

ECommandAction TBaseMap::operator[] (const std::string& value) const {
	return getAction(value);
}


void TBaseMap::add(const std::string& value, ECommandAction action) {
	if (end() != map.find(value))
		util::app_error_fmt("TCommandMap::add() failed: Value $ duplicated.", value);
	map.insert(TActionNamesItem(value, action));
}



void TActionMap::prime() {
	init(fillActionMap);
}

void TControlMap::prime() {
	init(fillControlMap);
}


} /* namespace app */
