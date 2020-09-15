/*
 * sysconsts.h
 *
 *  Created on: 28.10.2019
 *      Author: dirk
 */

#ifndef INC_SYSCONSTS_H_
#define INC_SYSCONSTS_H_

#include "gcc.h"

// PC speaker beep
// See http://www.johnath.com/beep/beep.c
#ifndef CLOCK_TICK_RATE
#  define CLOCK_TICK_RATE 1193180
#endif

#define DEFAULT_BEEPER_FREQ   440.0 /* Middle A */
#define DEFAULT_BEEPER_LENGTH 200   /* Milliseconds */
#define DEFAULT_BEEPER_DELAY  100   /* Milliseconds */
#define DEFAULT_BEEPER_REPS   1

namespace sysutil {

STATIC_CONST char PATH_SEPERATOR = '/';
STATIC_CONST char ETH_NET_FOLDER[] = "/sys/class/net/";
STATIC_CONST char ETH_MAC_FILE[]   = "/sys/class/net/%s/address";
STATIC_CONST char USER_MAX_WATCHES_FILE[] = "/proc/sys/fs/inotify/max_user_watches";
STATIC_CONST char KERNEL_CPUS_ISOLATED[]  = "/sys/devices/system/cpu/isolated";
STATIC_CONST char APP_MEMORY_USAGE[] = "/proc/self/statm";

} // namespace sysutil

#endif /* INC_SYSCONSTS_H_ */
