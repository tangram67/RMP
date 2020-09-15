/*
 * systypes.h
 *
 *  Created on: 28.10.2019
 *      Author: dirk
 */

#ifndef INC_SYSTYPES_H_
#define INC_SYSTYPES_H_

#include <sys/resource.h>
#include "gcc.h"
#include "sysconsts.h"

namespace sysutil {

/* Kinds of resource limit.  */
enum ERessourceLimit {

  /* Per-process CPU limit, in seconds.  */
  ERL_LIMIT_CPU = RLIMIT_CPU,

  /* Largest file that can be created, in bytes.  */
  ERL_LIMIT_FSIZE = RLIMIT_FSIZE,

  /* Maximum size of data segment, in bytes.  */
  ERL_LIMIT_DATA = RLIMIT_DATA,

  /* Maximum size of stack segment, in bytes.  */
  ERL_LIMIT_STACK = RLIMIT_STACK,

  /* Largest core file that can be created, in bytes.  */
  ERL_LIMIT_CORE = RLIMIT_CORE,

  /* Largest resident set size, in bytes.
     This affects swapping; processes that are exceeding their
     resident set size will be more likely to have physical memory
     taken from them.  */
  ERL_LIMIT_RSS = RLIMIT_RSS,

  /* Number of open files.  */
  ERL_LIMIT_NOFILE = RLIMIT_NOFILE,

  /* Address space limit.  */
  ERL_LIMIT_AS = RLIMIT_AS,

  /* Number of processes.  */
  ERL_LIMIT_NPROC = RLIMIT_NPROC,

  /* Locked-in-memory address space.  */
  ERL_LIMIT_MEMLOCK = RLIMIT_MEMLOCK,

  /* Maximum number of file locks.  */
  ERL_LIMIT_LOCKS = RLIMIT_LOCKS,

  /* Maximum number of pending signals.  */
  ERL_LIMIT_SIGPENDING = RLIMIT_SIGPENDING,

  /* Maximum bytes in POSIX message queues.  */
  ERL_LIMIT_MSGQUEUE = RLIMIT_MSGQUEUE,

  /* Maximum nice priority allowed to raise to.
     Nice levels 19 .. -20 correspond to 0 .. 39
     values of this resource limit.  */
  ERL_LIMIT_NICE = RLIMIT_NICE,

  /* Maximum realtime priority allowed for non-priviledged
     processes.  */
  ERL_LIMIT_NICELIMIT_RTPRIO = RLIMIT_RTPRIO,

  /* Maximum CPU time in µs that a process scheduled under a real-time
     scheduling policy may consume without making a blocking system
     call before being forcibly descheduled.  */
  ERL_LIMIT_RTTIME = RLIMIT_RTTIME,
  ERL_LIMIT_NLIMITS = RLIMIT_NLIMITS
};

enum EAddressType {
	EAT_IPV4,
	EAT_IPV6,
	EAT_MCAST,
	EAT_UCAST,
	EAT_LOCL,
	EAT_ALL,
	EAT_DEFAULT = EAT_ALL
};

typedef struct CSysInfo {
    std::string sysname;    /* Operating system name (e.g., "Linux") */
    std::string nodename;   /* Name within "some implementation-defined network" */
    std::string release;    /* Operating system release (e.g., "2.6.28") */
    std::string version;    /* Operating system version */
    std::string machine;    /* Hardware identifier */
} TSysInfo;

typedef struct CMemInfo {
    size_t memTotal;
    size_t memFree;
    size_t buffers;
    size_t cached;
    size_t swapCached;
    size_t swapTotal;
    size_t swapFree;
    size_t shmem;

    void clear() {
        memTotal = 0;
        memFree = 0;
        buffers = 0;
        cached = 0;
        swapCached = 0;
        swapTotal = 0;
        swapFree = 0;
        shmem = 0;
    }

    CMemInfo() { clear(); };
} TMemInfo;

typedef struct CBeeperParams {
	float freq;  /* Tone frequency (Hz)     */
	int length;  /* Tone length    (ms)     */
	int repeats; /* No. of repetitions      */
	int delay;   /* Delay between reps (ms) */
	bool wait;   /* Wait after last rep     */

	void clear() {
		freq    = DEFAULT_BEEPER_FREQ;
		length  = DEFAULT_BEEPER_LENGTH;
		repeats = DEFAULT_BEEPER_REPS;
		delay   = DEFAULT_BEEPER_DELAY;
		wait = true;
	}

	CBeeperParams() { clear(); };
} TBeeperParams;

typedef struct CDiskSpace {
	unsigned long int block;
	size_t usage;

#ifdef GCC_HAS_LARGEFILE64
    __fsblkcnt64_t total;
    __fsblkcnt64_t avail;
#else
    __fsblkcnt_t total;
    __fsblkcnt_t avail;
#endif

    void clear() {
    	block = 0;
    	total = 0;
    	avail = 0;
    	usage = 0;
    }

    CDiskSpace() { clear(); };
} TDiskSpace;

} // namespace sysutil

#endif /* INC_SYSTYPES_H_ */
