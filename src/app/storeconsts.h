/*
 * storeconsts.h
 *
 *  Created on: 18.07.2020
 *      Author: dirk
 */

#ifndef APP_STORECONSTS_H_
#define APP_STORECONSTS_H_

#include "../inc/gcc.h"

namespace app {

STATIC_CONST char STORE_HOST_NAME[] = "ExternalHostName";
STATIC_CONST char STORE_REFRESH_TIMER[] = "RefreshTimer";
STATIC_CONST char STORE_REFRESH_INTERVAL[] = "RefreshInterval";
STATIC_CONST char STORE_WEB_SOCKETS[] = "AllowWebSockets";

STATIC_CONST char STORE_CURRENT_SONG_HASH[] = "CurrentSongHash";
STATIC_CONST char STORE_CURRENT_SONG_NAME[] = "CurrentSongName";
STATIC_CONST char STORE_CURRENT_PLAYLIST[]  = "CurrentPlaylist";
STATIC_CONST char STORE_SELECTED_PLAYLIST[] = "SelectedPlaylist";

} /* namespace app */

#endif /* APP_STORECONSTS_H_ */
