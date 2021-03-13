/*
 * sysutils.h
 *
 *  Created on: 25.01.2016
 *      Author: Dirk Brinkmeier
 */
#ifndef SYSUTILS_H_
#define SYSUTILS_H_

#include <string>
#include <cmath>
#include "gcc.h"
#include "systypes.h"
#include "sysconsts.h"
#include "stringutils.h"

namespace sysutil {

enum EErrorLocale { EEL_SYSTEM, EEL_POSIX };

bool isLinux();
bool uname(TSysInfo& system);

std::string getHostName(bool FQDN = false);
std::string getDomainName();

std::string getUserName(const uid_t uid);
std::string getGroupName(const gid_t gid);

bool getUserID(const std::string& userName, uid_t& uid, gid_t& gid);
bool getGroupID(const std::string& groupName, gid_t& gid);

rlim_t getCurrentRessourceLimit(const sysutil::ERessourceLimit ressoure);
rlim_t getMaxRessourceLimit(const sysutil::ERessourceLimit ressoure);
bool setHardRessourceLimit(const sysutil::ERessourceLimit ressoure, const rlim_t max);
bool setSoftRessourceLimit(const sysutil::ERessourceLimit ressoure, const rlim_t cur);

void getMemoryUsage(size_t& current, size_t& peak);
size_t getPeakMemoryUsage();
size_t getCurrentMemoryUsage();
size_t getProcessorCount();
size_t getThreadCount();
pid_t getProcessID(const std::string& name);
bool isProcessRunning(const long int pid);
bool isProcessRunning(const util::TStringList& processes);
bool getSystemMemory(sysutil::TMemInfo& memory);
bool getDiskSpace(const std::string& path, TDiskSpace& disk);
bool getIsolatedCPUs(std::string& cpu);
size_t getWatchFileLimit();
uint64_t getProcessorSerial();

std::string getSysErrorMessage(int errnum, EErrorLocale language = EEL_POSIX);
std::string getInetErrorMessage(int errnum);
std::string getSignalName(int signal);

size_t maxPathSize();
size_t maxHostNameSize();

char getch();
bool beep();
bool beep(const TBeeperParams& params);

std::string getDefaultAdapter(const bool active = false);
std::string getMacAddress(const std::string& interface = "eth0");
bool getLocalIpAddress(util::TStringList& list, const EAddressType type = EAT_DEFAULT);
bool getLocalIpAddresses(util::TStringList& list, const EAddressType type = EAT_DEFAULT);

bool lockMemory(const int flags);
bool lockAddress(void * addr, const size_t size);
bool unlockAddress(void * addr, const size_t size);

template<typename T>
bool lockAddress(T* addr) {
	size_t size = sizeof(T);
	return lockAddress((void*)addr, size);
};

template<typename T>
T* createLockedObject() {
	T* o = new T;
	if (util::assigned(o)) {
		if (!lockAddress<T>(o)) {
			delete o;
			o = nil;
		}
	}
	return o;
};

} // namespace sysutil

#endif /* SYSUTILS_H_ */
