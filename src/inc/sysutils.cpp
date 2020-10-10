/*
 * sysutils.cpp
 *
 *  Created on: 25.01.2016
 *      Author: Dirk Brinkmeier
 */
#include "memory.h"
#include "process.h"
#include "sockets.h"
#include "compare.h"
#include "convert.h"
#include "sysutils.h"
#include "fileutils.h"
#include "stringutils.h"
#include "templates.h"
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/kd.h>
#include <linux/input.h>
#include <termios.h>
#include <unistd.h>
#include <fstream>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>


int __s_ioctl(int fd, unsigned long int request, int param) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::ioctl(fd, request, param);
	} while (r == EXIT_ERROR && errno == EINTR);
	return r;
}


using namespace util;


bool sysutil::isLinux() {
	bool retVal = false;
	struct utsname p;
	if (EXIT_SUCCESS == ::uname(&p)) {
		if (0 == util::strncasecmp(p.sysname, "linux", 5))
			retVal = true;
	}
	return retVal;
}

bool sysutil::uname(TSysInfo& system) {
	bool retVal = false;
	struct utsname p;
	if (EXIT_SUCCESS == ::uname(&p)) {
		system.machine  = p.machine;
		system.nodename = p.nodename;
		system.release  = p.release;
		system.sysname  = p.sysname;
		system.version  = p.version;
		retVal = true;
	}
	return retVal;
}


bool sysutil::lockMemory(const int flags) {
	errno = EXIT_SUCCESS;
	return EXIT_SUCCESS == mlockall(flags);
}

bool sysutil::lockAddress(void * addr, const size_t size) {
	// Lock memory region
	errno = EXIT_SUCCESS;
	int r = mlock(addr, size);

	// Access memory region and write 0 to prevent delayed copy-on-write page faults in critical application sections
	if (EXIT_SUCCESS == r) {
		memset(addr, 0, size);
		return true;	
	}

	return false;
}

bool sysutil::unlockAddress(void * addr, const size_t size) {
	// Lock memory region
	errno = EXIT_SUCCESS;
	return EXIT_SUCCESS == munlock(addr, size);
}


rlim_t sysutil::getCurrentRessourceLimit(const sysutil::ERessourceLimit ressoure) {
	struct rlimit limit;
    getrlimit((int)ressoure, &limit);
    return limit.rlim_cur;
}

rlim_t sysutil::getMaxRessourceLimit(const sysutil::ERessourceLimit ressoure) {
	struct rlimit limit;
    getrlimit((int)ressoure, &limit);
    return limit.rlim_max;
}

bool sysutil::setHardRessourceLimit(const sysutil::ERessourceLimit ressoure, const rlim_t max) {
	struct rlimit limit;
	limit.rlim_cur = (rlim_t)0;
	limit.rlim_max = max;
    return EXIT_SUCCESS == setrlimit((int)ressoure, &limit);
}

bool sysutil::setSoftRessourceLimit(const sysutil::ERessourceLimit ressoure, const rlim_t cur) {
	struct rlimit limit;
	limit.rlim_cur = cur;
	limit.rlim_max = getMaxRessourceLimit(ressoure);
    return EXIT_SUCCESS == setrlimit((int)ressoure, &limit);
}

/**
 * See http://nadeausoftware.com/articles/2012/07/c_c_tip_how_get_process_resident_set_size_physical_memory_use
 * Returns the peak (maximum so far) resident set size (physical memory use) measured in bytes
 */
size_t sysutil::getPeakMemoryUsage() {
    struct rusage mem;
    getrusage(RUSAGE_SELF, &mem);
    return (size_t)(mem.ru_maxrss * 1024L);
}

/**
 * Returns the current resident set size (physical memory usage) measured in bytes
 */
size_t sysutil::getCurrentMemoryUsage() {
	long mem = 0L;
	TStdioFile file;
	file.open(APP_MEMORY_USAGE, "r");
	if (!file.isOpen())
		return (size_t)0; // Can't open!
	if (fscanf(file(), "%*s%ld", &mem) != 1)
		return (size_t)0; // Can't read!
	return (size_t)mem * (size_t)sysconf(_SC_PAGESIZE);
}

void sysutil::getMemoryUsage(size_t& current, size_t& peak) {
	current = getCurrentMemoryUsage();
	peak = getPeakMemoryUsage();
}


size_t sysutil::getWatchFileLimit() {
	long limit = 0L;
	TStdioFile file;
	file.open(USER_MAX_WATCHES_FILE, "r");
	if (!file.isOpen())
		return (size_t)0; // Can't open!
	if (fscanf(file(), "%ld", &limit) != 1)
		return (size_t)0; // Can't read!
	return (size_t)limit;
}

#define ISOLATED_CPU_SIZE 100

bool sysutil::getIsolatedCPUs(std::string& cpu) {
	cpu.clear();
	char buf[ISOLATED_CPU_SIZE+1];
	memset (buf, 0, 101);
	TStdioFile file;
	file.open(KERNEL_CPUS_ISOLATED, "r");
	if (file.isOpen()) {
		char* p = fgets(buf, ISOLATED_CPU_SIZE, file());
		if (NULL != p) {
			size_t len = strnlen(p, ISOLATED_CPU_SIZE);
			if (len > 0) {
				cpu = trim(std::string(p, len));
				return true;
			}
		}
	}	
	return false;
}

#undef ISOLATED_CPU_SIZE


bool sysutil::getUserID(const std::string& userName, uid_t& uid, gid_t& gid) {
	uid = 0;
	gid = 0;
	size_t size;
	struct passwd pwd;
	struct passwd *result = NULL;
	bool retVal = false;
	int r;

	if (!userName.empty()) {
		size = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (size <= 0)    /* Value was indeterminate */
			size = 16384; /* Should be more than enough */
		TBuffer buffer(size);

		r = getpwnam_r(userName.c_str(), &pwd, buffer.data(), size, &result);
		if ( (assigned(result)) && (r == EXIT_SUCCESS) ) {
			uid = pwd.pw_uid;
			gid = pwd.pw_gid;
			retVal = true;
		}
	}

	return retVal;
}

bool sysutil::getGroupID(const std::string& groupName, gid_t& gid) {
	gid = 0;
	size_t size;
	struct group grp;
	struct group *result = NULL;
	bool retVal = false;
	int r;

	if (!groupName.empty()) {
		size = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (size <= 0)    /* Value was indeterminate */
			size = 16384; /* Should be more than enough */
		TBuffer buffer(size);

		r = getgrnam_r(groupName.c_str(), &grp, buffer.data(), size, &result);
		if ( (assigned(result)) && (r == EXIT_SUCCESS) ) {
			gid = grp.gr_gid;
			retVal = true;
		}
	}

	return retVal;
}


std::string sysutil::getUserName(const uid_t uid) {
	size_t size;
	struct passwd pwd;
	struct passwd *result = NULL;
	int r;

	size = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (size <= 0)    /* Value was indeterminate */
		size = 16384; /* Should be more than enough */
	TBuffer buffer(size);

	r = getpwuid_r(uid, &pwd, buffer.data(), size, &result);
	return ( ((assigned(result)) && (r == EXIT_SUCCESS)) ? pwd.pw_name : std::string() );
}

std::string sysutil::getGroupName(const gid_t gid) {
	size_t size;
	struct group grp;
	struct group *result = NULL;
	int r;

	size = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (size <= 0)    /* Value was indeterminate */
		size = 16384; /* Should be more than enough */
	TBuffer buffer(size);

	r = getgrgid_r(gid, &grp, buffer.data(), size, &result);
	return ( ((assigned(result)) && (r == EXIT_SUCCESS)) ? grp.gr_name : std::string() );
}


size_t sysutil::getProcessorCount() {
	size_t retVal = sysconf(_SC_NPROCESSORS_ONLN);
	if (retVal == (size_t)-1)
		retVal = std::string::npos;
	return retVal;
}


const char* strposlast(const char* str, char c) {
	const char* r = str;
	if (str != NULL) {
		const char* p = str;
		while (*p != '\0') {
			if (*p == c) {
				r = ++p;
			} else {
				++p;
			}
		}
	}
	return r;
}

pid_t sysutil::getProcessID(const std::string& name) {
	if (!name.empty()) {
		const char* proc = name.c_str();
		DIR* dir;
		FILE* fp;
		struct dirent* ent;
		size_t size = sysutil::maxPathSize();
		char buf[size+1];
		char* end;

		// Open "/proc" folder
		if ((dir = opendir("/proc")) == NULL) {
			return (pid_t)EXIT_ERROR;
		}

		// Read each process entries
		while((ent = readdir(dir)) != NULL) {

			// Ignore non numeric entries, only valid numerical PIDs are processes
			long int pid = strtol(ent->d_name, &end, 10);
			if (*end != '\0' || pid <= 0) {
				continue;
			}

			// Try to open "cmdline" entry for given PID
			snprintf(buf, size, "/proc/%ld/cmdline", pid);
			if ((fp = fopen(buf, "r")) != NULL) {
				if (fgets(buf, sizeof(buf), fp) != NULL) {
					// Compare the first token in the file for given program name
					const char* line = strtok(buf, " ");
					if (line != NULL) {
						// Compare full path on file name like "/..."
						const char* base;
						bool fullname = name[0] == '/';
						base = fullname ? line : strposlast(line, '/');
						if (base != NULL) {
							if (!strcmp(base, proc)) {
								fclose(fp);
								closedir(dir);
								return (pid_t)pid;
							}
						}
					}
				}
				fclose(fp);
			}

		}
		closedir(dir);
	}
    return (pid_t)EXIT_ERROR;
}

bool sysutil::isProcessRunning(const util::TStringList& processes) {
	if (!processes.empty()) {
		DIR* dir;
		FILE* fp;
		struct dirent* ent;
		size_t size = sysutil::maxPathSize();
		char buf[size+1];
		char* end;

		// Open "/proc" folder
		if ((dir = opendir("/proc")) == NULL) {
			return false;
		}

		// Read each process entries
		while((ent = readdir(dir)) != NULL) {

			// Ignore non numeric entries, only valid numerical PIDs are processes
			long int pid = strtol(ent->d_name, &end, 10);
			if (*end != '\0' || pid <= 0) {
				continue;
			}

			// Try to open "cmdline" entry for given PID
			snprintf(buf, size, "/proc/%ld/cmdline", pid);
			if ((fp = fopen(buf, "r")) != NULL) {
				if (fgets(buf, sizeof(buf), fp) != NULL) {
					// Compare the first token in the file for given program names
					const char* line = strtok(buf, " ");
					if (line != NULL) {
						const char* base = strposlast(line, '/');
						for (size_t i=0; i<processes.size(); ++i) {
							const std::string& name = processes[i];
							if (!name.empty()) {
								const char* proc = name.c_str();
								if (proc != NULL) {
									// Compare full path on file name like "/..."
									bool fullname = name[0] == '/';
									const char* app = fullname ? line : base;
									if (app != NULL) {
										if (!strcmp(app, proc)) {
											fclose(fp);
											closedir(dir);
											return true;
										}
									}
								}
							}
						}
					}
				}
				fclose(fp);
			}

		}
		closedir(dir);
	}
    return false;
}


bool sysutil::isProcessRunning(const long int pid) {
	if (pid > 0) {
		DIR* dir;
		struct dirent* ent;
		char* end;

		// Open "/proc" folder
		if ((dir = opendir("/proc")) == NULL) {
			return false;
		}

		// Read each process entries
		while((ent = readdir(dir)) != NULL) {

			// Ignore non numeric entries, only valid numerical PIDs are processes
			long int proc = strtol(ent->d_name, &end, 10);
			if (*end != '\0' || proc <= 0) {
				continue;
			}

			if (proc == pid) {
				return true;
			}
		}
		closedir(dir);
	}
    return false;
}


size_t sysutil::maxPathSize() {
	size_t size = pathconf(".", _PC_PATH_MAX);
	if (size == (size_t)EXIT_ERROR)   /* Value was indeterminate */
#ifdef PATH_MAX
		size = PATH_MAX;
#else
		size = 4096;
#endif
	return size;
}

size_t sysutil::maxHostNameSize() {
	size_t size = sysconf(_SC_HOST_NAME_MAX);
	if (size == (size_t)-1)   /* Value was indeterminate */
#ifdef HOST_NAME_MAX
		size = HOST_NAME_MAX; /* Use default value  */
#else
		size = 64;
#endif
	return size;
}


std::string sysutil::getHostName(bool FQDN) {
	size_t size = maxHostNameSize();
	TBuffer buffer;
	int result;
	std::string name, domain;

	buffer.resize(succ(size));
	buffer.fill('\0');
	result = gethostname(buffer.data(), size);
	name = (result == EXIT_SUCCESS)
				? std::string(buffer.data(), buffer.strlen()) : std::string();

	domain.clear();
	if (!name.empty() && FQDN) {
		buffer.fill('\0');
		result = getdomainname(buffer.data(), size);
		domain = (result == EXIT_SUCCESS)
					? std::string(buffer.data(), buffer.strlen()) : std::string();
	}

	if (!domain.empty())
		return name + "." + domain;
	else
		return name;
}


std::string sysutil::getDomainName() {
	size_t size = maxHostNameSize();
	TBuffer buffer;
	int result;
	std::string domain;

	buffer.resize(succ(size));
	buffer.fill('\0');
	result = getdomainname(buffer.data(), size);
	domain = ((result == EXIT_SUCCESS) && (size > 0))
				? std::string(buffer.data(), buffer.strlen()) : std::string();

	return domain;
}


std::string sysutil::getSysErrorMessage(int errnum) {
	char* c_str;
	size_t size = 128;
	TBuffer buffer(size);
	c_str = strerror_r(errnum, buffer.data(), size);
	if (assigned(c_str))
		return std::string(c_str);
	return "Getting error text via strerror_r(" + std::to_string((size_s)errnum) + ") failed.";
}


std::string sysutil::getInetErrorMessage(int errnum)
{
	const char* c_str = gai_strerror(errnum);
	if (assigned(c_str))
		return std::string(c_str);
	return "Getting error text via gai_strerror(" + std::to_string((size_s)errnum) + ") failed.";
}

std::string sysutil::getSignalName(int signal)
{
	std::string name = "SIGUNKNOWN";
	char *p = strsignal(signal);
	size_t len = strnlen(p, 255);
	if (util::assigned(p) && len < 255) {
		name = std::string(p, len);
	}
	return name;
}

// Use getch() of TApplication instead!!!
// --> avoids unnecessary tcgetattr/tcsetattr operations on STDIN_FILENO
char sysutil::getch() {
	char retVal = 0, buffer = 0;
	if (STDIN_FILENO >= 0) {

		struct termios settings, save;
		memset(&settings, 0, sizeof(settings));

		if (tcgetattr(STDIN_FILENO, &settings) == EXIT_SUCCESS) {

			memcpy(&save, &settings, sizeof(settings));
			settings.c_lflag &= ~ICANON;
			settings.c_lflag &= ~ECHO;
			settings.c_cc[VMIN] = 1;
			settings.c_cc[VTIME] = 0;
			if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) == EXIT_SUCCESS) {

				ssize_t r;
				do {
					r = ::read(STDIN_FILENO, &buffer, 1);
				} while (r == (ssize_t)-1 && errno == EINTR);

				if (r > (ssize_t)0) {
					retVal = buffer;
				}

				tcsetattr(STDIN_FILENO, TCSADRAIN, &save);
			}
		}
	}
	return retVal;
}


std::string sysutil::getMacAddress(const std::string& interface) {
	if (!interface.empty()) {
		std::string fileName = cprintf(ETH_MAC_FILE, interface.c_str());
		if (fileExists(fileName)) {
			std::string mac;
			std::ifstream is;
			TStreamGuard<std::ifstream> strm(is);
			strm.open(fileName, std::ios_base::in);
			std::getline(is, mac);
			if (is.good() && (mac.size() == 17)) {
				std::transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
				return mac;
			}
		}
	}
	return std::string();
}

std::string sysutil::getDefaultAdapter(const bool active) {
	// Find first active network interface
	DIR* dir;
	FILE* fp;
	struct dirent* ent;
	size_t size = maxPathSize();
	char buf[size+1];
	std::string eth;

	// Open "/sys/class/net" folder
	if ((dir = opendir(ETH_NET_FOLDER)) == NULL) {
		return eth;
	}

	// Read each interface entries
	while((ent = readdir(dir)) != NULL) {
		bool found = false;
		size_t len = strnlen(ent->d_name, 64);
		if (len > 3) {
			if (!active) {
				eth = std::string(ent->d_name, len);
			}
			snprintf(buf, size, "/sys/class/net/%s/carrier", ent->d_name);
			if ((fp = fopen(buf, "r")) != NULL) {
				int carrier = 0;
				if (fscanf(fp, "%d", &carrier) == 1) {
					if (carrier > 0) {
						if (active) {
							eth = std::string(ent->d_name, len);
						}
						found = true;
					}
				}
				fclose(fp);
			}
		}
		if (found)
			break;
	}
	closedir(dir);

	return eth;
}

bool sysutil::getLocalIpAddress(util::TStringList& list, const EAddressType type) {
	list.clear();
	std::string eth = getDefaultAdapter(true);
	if (!eth.empty()) {
		struct ifaddrs * addresses = nil;
		struct ifaddrs * it;
		std::string addr;
		bool ok;
		int r = getifaddrs(&addresses);
		if (r == EXIT_SUCCESS && util::assigned(addresses)) {
			it = addresses;
			while (it) 	{
				if (it->ifa_addr) {
					ok = false;
					if (0 == util::strncasecmp(it->ifa_name, eth, eth.size())) {
						addr = inet::inetAddrToStr(it->ifa_addr);

						if (!ok && it->ifa_addr->sa_family == AF_INET && (type == EAT_IPV4 || type == EAT_ALL)) {
							ok = true;
						}

						if (!ok && it->ifa_addr->sa_family == AF_INET6 && (type == EAT_IPV6 || type == EAT_ALL)) {
							ok = (0 != strncasecmp(addr, "fe80:", 5));
						}

						if (ok) {
							list.add(addr);
						}
					}
				}
				it = it->ifa_next;
			}
		}
		if (util::assigned(addresses)) {
			freeifaddrs(addresses);
		}
	}
	return list.size() > 0;
}

bool sysutil::getLocalIpAddresses(util::TStringList& list, const EAddressType type) {
	struct ifaddrs * addresses = nil;
	struct ifaddrs * it;
	std::string addr;
	list.clear();
	bool ok;
	int r = getifaddrs(&addresses);
	if (r == EXIT_SUCCESS && util::assigned(addresses)) {
		it = addresses;
		while (it) 	{
			if (it->ifa_addr) {
				ok = false;
				if (0 != util::strncasecmp(it->ifa_name, "lo", 2)) {
					addr = inet::inetAddrToStr(it->ifa_addr);

					if (!ok && it->ifa_addr->sa_family == AF_INET && (type == EAT_IPV4 || type == EAT_ALL)) {
						ok = true;
					}

					if (!ok && it->ifa_addr->sa_family == AF_INET6 && (type == EAT_IPV6 || type == EAT_ALL)) {
						ok = (0 != strncasecmp(addr, "fe80:", 5));
					}

					if (ok) {
						list.add(addr);
					}
				}
			}
			it = it->ifa_next;
		}
	}
	if (util::assigned(addresses)) {
		freeifaddrs(addresses);
	}
	return list.size() > 0;
}


/*
 * Read system memory state from file /proc/meminfo
 *
 * MemTotal:        7873480 kB
 * MemFree:          142612 kB
 * Buffers:          946004 kB
 * Cached:          5865556 kB
 * SwapCached:            0 kB
 * Active:          1655996 kB
 * Inactive:        5668120 kB
 * Active(anon):     350248 kB
 * Inactive(anon):   174092 kB
 * Active(file):    1305748 kB
 * Inactive(file):  5494028 kB
 * Unevictable:        4148 kB
 * Mlocked:            4148 kB
 * SwapTotal:       1983484 kB
 * SwapFree:        1983484 kB
 * Dirty:                60 kB
 * Writeback:             0 kB
 * AnonPages:        516704 kB
 * Mapped:            44328 kB
 * Shmem:              9296 kB
 * Slab:             333272 kB
 * SReclaimable:     308344 kB
 * SUnreclaim:        24928 kB
 * KernelStack:        2400 kB
 * PageTables:        12084 kB
 * NFS_Unstable:          0 kB
 * Bounce:                0 kB
 * WritebackTmp:          0 kB
 * CommitLimit:     5920224 kB
 * Committed_AS:    1448788 kB
 * VmallocTotal:   34359738367 kB
 * VmallocUsed:      369896 kB
 * VmallocChunk:   34359365208 kB
 * HardwareCorrupted:     0 kB
 * AnonHugePages:         0 kB
 * HugePages_Total:       0
 * HugePages_Free:        0
 * HugePages_Rsvd:        0
 * HugePages_Surp:        0
 * Hugepagesize:       2048 kB
 * DirectMap4k:       55296 kB
 * DirectMap2M:     8036352 kB
 */

bool getProcItem(const std::string& line, const std::string& item, size_t& value) {
	// Read same value only once!
	if (0 == value) {
		if (0 == util::strncasecmp(line, item, item.size())) {
			TStringList list;
			list.split(line, ':');
			if (list.size() > 1) {
				value = strToSize(list[1]);
				return true;
			}
		}
	}
	return false;
}

bool sysutil::getSystemMemory(sysutil::TMemInfo& memory) {
	int found = 0;
	memory.clear();
	std::string fileName = "/proc/meminfo";
	if (fileExists(fileName)) {
		TStringList items;
		items.loadFromFile(fileName);
		if (!items.empty()) {
			// Read entries for:
			//   MemTotal
			//   MemFree
			//   Buffers
			//   Cached
			//   SwapCached
			//   SwapTotal
			//   SwapFree
			//   Shmem
			std::string line;
			size_t free = 0, available = 0;
			for (size_t i=0; i<items.size(); ++i) {
				line = trim(items[i]);

				if (getProcItem(line, "MemTotal", memory.memTotal)) {
					++found;
					continue;
				}
				if (getProcItem(line, "MemAvailable", available)) {
					++found;
					continue;
				}
				if (getProcItem(line, "MemFree", free)) {
					++found;
					continue;
				}
				if (getProcItem(line, "Buffers", memory.buffers)) {
					++found;
					continue;
				}
				if (getProcItem(line, "Cached", memory.cached)) {
					++found;
					continue;
				}
				if (getProcItem(line, "SwapCached", memory.swapCached)) {
					++found;
					continue;
				}
				if (getProcItem(line, "SwapTotal", memory.swapTotal)) {
					++found;
					continue;
				}
				if (getProcItem(line, "SwapFree", memory.swapFree)) {
					++found;
					continue;
				}
				if (getProcItem(line, "Shmem", memory.shmem)) {
					++found;
					continue;
				}

				// All items found?
				if (found >= 8)
					break;
			}

			// Calculate free memory
			if (available > 0) {
				memory.memFree = available;
			} else {
				if (free > 0) {
					memory.memFree = free + memory.buffers;
				}
			}
		}
	}
	return (found > 0);
}

bool getProcValue(const std::string& line, const std::string& item, uint64_t& value) {
	// Read same value only once!
	if (0 == value) {
		if (0 == util::strncasecmp(line, item, item.size())) {
			TStringList list;
			list.split(line, ':');
			if (list.size() > 1) {
				value = util::strToUnsigned64(list[1], 0, syslocale, 16);
				return true;
			}
		}
	}
	return false;
}

uint64_t sysutil::getProcessorSerial() {
	int found = 0;
	size_t serial = 0;
	std::string fileName = "/proc/cpuinfo";
	if (fileExists(fileName)) {
		uint64_t psn = 0;
		TStringList items;
		items.loadFromFile(fileName);
		if (!items.empty()) {
			// Read entry for serial number:
			//   Hardware	: BCM2835
			//   Revision	: a02100
			//   Serial		: 000000001661b9b7 <-- Read this entry
			//   Model		: Raspberry Pi Compute Module 3 Plus Rev 1.0
			std::string line;
			for (size_t i=0; i<items.size(); ++i) {
				line = trim(items[i]);

				if (getProcValue(line, "Serial", psn)) {
					serial = psn;
					++found;
				}

				// All items found?
				if (found > 0)
					break;
			}

		}
	}
	return serial;
}

bool sysutil::getDiskSpace(const std::string& path, TDiskSpace& disk) {
	disk.clear();
	if (!path.empty()) {
#ifdef GCC_HAS_LARGEFILE64
		struct statvfs64 stat;
		if (EXIT_SUCCESS != ::statvfs64(path.c_str(), &stat)) {
			return false;
		}
#else
		struct statvfs stat;
		if (EXIT_SUCCESS != ::statvfs(path.c_str(), &stat)) {
			return false;
		}
#endif
		if (stat.f_bsize > 0 && stat.f_blocks > 0) {
			disk.block = stat.f_bsize;
			disk.avail = stat.f_bsize * stat.f_bavail;
			disk.total = stat.f_bsize * stat.f_blocks;
			disk.usage = stat.f_bavail * 100 / stat.f_blocks;
			return true;
		}
	}
	return false;
}

size_t sysutil::getThreadCount() {
	size_t threads = 0;
	bool found = false;
	std::string fileName = "/proc/self/status";
	if (fileExists(fileName)) {
		TStringList items;
		items.loadFromFile(fileName);
		if (!items.empty()) {

			// Iterate through entries
			std::string line;
			size_t i;

			// Start at estimated position
			if (items.size() > 19) {
				for (i=20; i<items.size(); ++i) {
					line = trim(items[i]);
					if (getProcItem(line, "Threads", threads)) {
						found = true;
						break;
					}
				}
			}

			// Iterate through other entries
			if (!found) {
				size_t end = items.size() > 19 ? 20 : items.size();
				for (i=0; i<end; ++i) {
					line = trim(items[i]);
					if (getProcItem(line, "Threads", threads)) {
						break;
					}
				}
			}

		}
	}
	return threads;
}

bool sysutil::beep() {
	TBeeperParams params;
	return beep(params);
}

bool sysutil::beep(const TBeeperParams& params) {
	TBaseFile file;
	bool ok = false;
	if (!ok) {
		// See https://superuser.com/questions/1150768/how-do-i-grant-access-dev-console-to-an-executable
		// --> apt-get install beep
		//     modprobe pcspkr
		std::string speaker = "/dev/input/by-path/platform-pcspkr-event-spkr";
		try {
			file.open(speaker, O_WRONLY);
			if (file.isOpen()) {
				input_event on;
				on.time.tv_sec = 0;
				on.time.tv_usec = 0;
				on.type = EV_SND;
				on.code = 2;
				on.value = (int)params.freq;

				input_event off;
				off.time.tv_sec = 0;
				off.time.tv_usec = 0;
				off.type = EV_SND;
				off.code = 2;
				off.value = (int)0;

				int i;
				ssize_t r;
				for (i=0; i<params.repeats; i++) {
					r = file.write(&on, sizeof(on));
					if (r > 0) {
						saveWait(params.length);
						r = file.write(&off, sizeof(off));
						if (r > 0) {
							if(params.wait || (i+1 < params.repeats))
								saveWait(params.delay);
						} else {
							break;
						}
					} else {
						break;
					}
				}
				ok = true;
			}
		} catch (...) {};
	}
	if (!ok) {
		try {
			std::string console = "/dev/console";
			file.open(console, O_WRONLY);
			if (file.isOpen()) {
				// See http://www.johnath.com/beep/beep.c
				int i;
				for (i=0; i<params.repeats; i++) {
					if (__s_ioctl(file(), KIOCSOUND, (int)(CLOCK_TICK_RATE / params.freq)) < 0) {
						break;
					}
					saveWait(params.length);
					__s_ioctl(file(), KIOCSOUND, (int)0);
					if(params.wait || (i+1 < params.repeats))
						saveWait(params.delay);
				}
				ok = true;
			}
		} catch (...) {};
	}
	if (!ok) {
		std::string command = "/usr/bin/beep";
		if (util::fileExists(command)) {
			int i;
			for (i=0; i<params.repeats; i++) {
				int result;
				std::string output;
				std::string cmdline = util::csnprintf("% -f % -l %", command, params.freq, params.length);
				try {
					util::fastExecuteCommandLine(cmdline, output, result);
					if(params.wait || (i+1 < params.repeats))
						saveWait(params.delay);
				} catch (const std::exception& e) {
					break;
				} catch (...) {
					break;
				}
			}
			ok = true;
		}
	}
	return ok;
}
