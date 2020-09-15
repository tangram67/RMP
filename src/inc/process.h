/*
 * process.h
 *
 *  Created on: 16.05.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef PROCESS_H_
#define PROCESS_H_

#include <map>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>  // Needed for waitpid in process.tpp
#include "fileutils.h" // Needed for template definition in process.tpp
#include "exception.h" // Needed for template definition in process.tpp
#include "templates.h" // Needed for template definition in process.tpp
#include "classes.h"   // Needed for template definition in process.tpp
#include "variant.h"   // Needed for template definition in process.tpp
#include "gcc.h"       // Needed for template definition in process.tpp

namespace util {

#define PROCESS_OUTPUT_BUFFER_SIZE 1024

class TEnvironment;

enum EProcessFileDescriptor {
  PFD_READ  = 0,
  PFD_WRITE = 1,
  PFD_ERROR = 1
};


struct CEnvironmentEntry {
	std::string key;
	std::string value;
	std::string env;
};


#ifdef STL_HAS_TEMPLATE_ALIAS

using PEnvironment = TEnvironment*;
using PEnvironmentEntry = CEnvironmentEntry*;
using TEnvironmentMap = std::map<std::string, PEnvironmentEntry>;

#else

typedef TEnvironment* PEnvironment;
typedef CEnvironmentEntry* PEnvironmentEntry;
typedef std::map<std::string, PEnvironmentEntry> TEnvironmentMap;

#endif

/* Deprecated, use templates instead */
bool __executeProcess(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, std::string& output, int& result);

template<typename output_t>
bool executeProcess(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, output_t&& output, int& result, util::TTimePart timeout = 30);
template<typename output_t>
bool fastExecuteProcess(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, output_t&& output, int& result, util::TTimePart timeout = 30);

template<typename output_t>
bool executeCommand(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, output_t&& output, int& result, util::TTimePart timeout = 30);
template<typename output_t>
bool fastExecuteCommand(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, output_t&& output, int& result, util::TTimePart timeout = 30);


template<typename output_t>
bool executeCommandLine(const std::string& commandLine, output_t&& output, int& result, const bool debug = false, util::TTimePart timeout = 30) {
	util::TEnvironment env;
	util::TVariantValues params;
	util::TStringList list;
	std::string line = util::trim(commandLine);
	if (debug) std::cout << "Commandline: \"" << line << "\"" <<std::endl;
	list.assign(line, ' ');
	list.compress();
	if (list.size() > 0) {
		std::string command = list[0];
		if (debug) std::cout << "Command: \"" << command << "\"" << std::endl;
		for (size_t i=1; i<list.size(); ++i) {
			params.addEntry(list[i]);
		}
		if (debug) {
			if (debug) std::cout << "Parameterlist:" << std::endl;
			params.debugOutput("Parameter", "  ");
		}
		try {
			bool r = util::executeProcess(command, params, "", env, output, result, timeout);
			if (r /*&& !output.empty()*/) {
				return true;
			}
		} catch (const std::exception& e) {
			std::string s = e.what();
			std::cout << "Exception: \"" << s << "\"" << std::endl;
			result = -201;
		} catch (...) {
			result = -202;
		}
	}
	return false;
}


template<typename output_t>
bool fastExecuteCommandLine(const std::string& commandLine, output_t&& output, int& result, const bool debug = false, util::TTimePart timeout = 30) {
	util::TEnvironment env;
	util::TVariantValues params;
	util::TStringList list;
	std::string line = util::trim(commandLine);
	if (debug) std::cout << "Commandline: \"" << line << "\"" <<std::endl;
	list.assign(line, ' ');
	list.compress();
	if (list.size() > 0) {
		std::string command = list[0];
		if (debug) std::cout << "Command: \"" << command << "\"" << std::endl;
		for (size_t i=1; i<list.size(); ++i) {
			params.addEntry(list[i]);
		}
		if (debug) {
			if (debug) std::cout << "Parameterlist:" << std::endl;
			params.debugOutput("Parameter", "  ");
		}
		try {
			bool r = util::fastExecuteProcess(command, params, "", env, output, result, timeout);
			if (r /*&& !output.empty()*/) {
				return true;
			}
		} catch (const std::exception& e) {
			std::string s = e.what();
			std::cout << "Exception: \"" << s << "\"" << std::endl;
			result = -201;
		} catch (...) {
			result = -202;
		}
	}
	return false;
}


std::string getEnvironmentVariable(const std::string& key);

class TEnvironment {
private:
	bool loaded;
	TEnvironmentMap env;
	void buildEntry(PEnvironmentEntry entry) const;
	inline std::string getVariable(const std::string& key) const;

public:
	void clear();
	size_t size() const { return env.size(); };
	int createStdEnvironment();
	bool addVariable(const std::string& key);
	void addEntry(const std::string& key, const std::string& value);
	void assign(util::TCharPointerArray& environment);
	void debugOutput();

	explicit TEnvironment();
	~TEnvironment();
};


} /* namespace util */

#include "process.tpp"

#endif /* PROCESS_H_ */
