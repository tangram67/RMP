/*
 * process.cpp
 *
 *  Created on: 16.05.2015
 *      Author: Dirk Brinkmeier
 */

#include <array>
#include <string>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "process.h"
#include "templates.h"
#include "exception.h"


using namespace util;


std::string util::getEnvironmentVariable(const std::string& key)
{
	std::string retVal("");
	if (!key.empty()) {
		char* p = getenv(key.c_str());
		if (util::assigned(p))
			retVal = std::string(p);
	}
	return retVal;
}


TEnvironment::TEnvironment() {
	loaded = false;
}

TEnvironment::~TEnvironment() {
	clear();
}

int TEnvironment::createStdEnvironment() {
	if (!loaded) {
		addVariable("PATH");
		addVariable("TERM");
		addVariable("LANG");
		addVariable("USER");
		addVariable("LD_LIBRARY_PATH");
		loaded = true;
	}
	return env.size();
}

inline std::string TEnvironment::getVariable(const std::string& key) const {
	return getEnvironmentVariable(key);
}


// http://tools.ietf.org/html/draft-robinson-www-interface-00
// In all cases, a missing environment variable is
// equivalent to a zero-length (NULL) value, and vice versa.
bool TEnvironment::addVariable(const std::string& key) {
	bool retVal = false;
	std::string s = getVariable(key);
	if (!s.empty()) {
		addEntry(key, s);
		retVal = true;
	}
	return retVal;
}

void TEnvironment::addEntry(const std::string& key, const std::string& value) {
	PEnvironmentEntry o;
	TEnvironmentMap::const_iterator it = env.find(key);
	if (it != env.end()) {
		o = it->second;
		if (util::assigned(o)) {
			if (o->value != value) {
				o->value = value;
				buildEntry(o);
			}
		}
	} else {
		o = new CEnvironmentEntry;
		o->key = key;
		o->value = value;
		buildEntry(o);
		env[key] = o;
	}
}

void TEnvironment::buildEntry(PEnvironmentEntry entry) const {
	entry->env = entry->key + "=" + entry->value;
}

void TEnvironment::clear() {
	if (!env.empty()) {
		PEnvironmentEntry o;
		TEnvironmentMap::const_iterator it = env.begin();
		while (it != env.end()) {
			o = it->second;
			if (util::assigned(o))
				util::freeAndNil(o);
			it++;
		}
		env.clear();
	}
	loaded = false;
}

void TEnvironment::debugOutput() {
	if (!env.empty()) {
		PEnvironmentEntry o;
		TEnvironmentMap::const_iterator it = env.begin();
		while (it != env.end()) {
			o = it->second;
			std::cout << "Environment : " << o->env << std::endl;
			it++;
		}
	}
}

void TEnvironment::assign(util::TCharPointerArray& environment) {
	size_t n = env.size();
	size_t i = 0;
	environment.resize(n+1, false);

	// Add environment strings to array
	if (n > 0) {
		PEnvironmentEntry o;
		TEnvironmentMap::const_iterator it = env.begin();
		while (it != env.end()) {
			o = it->second;
			if (util::assigned(o)) {
				environment[i] = o->env.empty() ? "none" : o->env.c_str();
			} else {
				environment[i] = (char*)NULL;
			}
			i++;
			it++;
		}
	}

	// Last environment entry must be null!
	environment[n] = (char*)NULL;
}



//
// Templated in process.tpp
//
bool util::__executeProcess(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, std::string& output, int& result)
{
	bool retVal = false;
	std::string error = "executeProcess(" + command + ") : ";
	std::string path = folder;
	int status;
	bool failure = false;
	result = EXIT_FAILURE;
	errno = EXIT_SUCCESS;
	output.clear();

	// Check if command file exists
	bool hasPath = (std::string::npos != command.find('/'));
	if (hasPath) {
		if (!util::fileExists(command))
			return retVal;
	}

	// Set working path
	if (path.empty()) {
		path = util::filePath(command);
		if (path.empty()) {
			path = util::realPath(".");
			if (path.empty())
				path = "/";
		}
	}
	util::validPath(path);

	// Create standard environment and
	// add command info to environment
	environment.createStdEnvironment();
	environment.addEntry("SCRIPT_NAME", command);
	environment.addEntry("SCRIPT_PATH", path);
	environment.addEntry("SCRIPT_PARAM", parameter[0].asString());

	// Assign environment to kernel structure
	util::TCharPointerArray env;
	environment.assign(env);

	// Create parameter header with first entry as command
	util::TStringList header;
	header.add(command);

	// Create parameter list from given list + header
	util::TCharPointerArray argv;
	parameter.assign(argv, header);

	for (int i=0; util::assigned(argv[i]); i++) {
		std::cout << util::succ(i) << ". Parameter: " << util::charToStr(argv[i], "<empty>") << std::endl;
	}

	// Piped I/O handles
	int readerPipe[2] = { -1, -1 };
	int writerPipe[2] = { -1, -1 };

	// Create pipes for process output
	if (pipe(readerPipe) < 0)
		throw util::sys_error(error + "pipe(readerPipe) failed.", errno);
	if (pipe(writerPipe) < 0)
		throw util::sys_error(error + "pipe(writerPipe) failed.", errno);

	// RAAI guards for pipe handles
	TDescriptorGuard<int> rpr(readerPipe[PFD_READ]);
	TDescriptorGuard<int> rpw(readerPipe[PFD_WRITE]);
	TDescriptorGuard<int> wpr(writerPipe[PFD_READ]);
	TDescriptorGuard<int> wpw(writerPipe[PFD_WRITE]);

	// Try forking child process and execute command
#ifdef GLIBC_HAS_VFORK
	pid_t pid = vfork();
#else
	pid_t pid = fork();
#endif

	switch (pid) {
		case (pid_t)-1:
			throw util::sys_error(error + "forking process failed.", errno);
			break;

		// Execution of new child process starts here...
		case (pid_t)0:

		    // After exec, all signal handlers are restored to their default values,
		    // with one exception of SIGCHLD. According to POSIX.1-2001 and Linux's
		    // implementation, SIGCHLD's handler will leave unchanged after exec
		    // if it was set to be ignored. Restore it to default action.
		    //signal(SIGCHLD, SIG_DFL);

			// Change to folder of command
			if (!path.empty()) {
				if (chdir(path.c_str()) < 0)
					throw util::sys_error(error + "chdir() for path <" + path + "> failed.", errno);
			}

			// Clone stdio pipes for child process
			if (dup2(readerPipe[PFD_READ], STDIN_FILENO) < 0)
				throw util::sys_error(error + "dup2(STDIN_FILENO) failed.", errno);

			if (dup2(writerPipe[PFD_WRITE], STDOUT_FILENO) < 0)
				throw util::sys_error(error + "dup2(STDOUT_FILENO) failed.", errno);

			if (dup2(writerPipe[PFD_WRITE], STDERR_FILENO) < 0)
				throw util::sys_error(error + "dup2(STDERR_FILENO) failed.", errno);

			// Close unused io pipes before execution of command
			// --> close PFD_WRITE for STDIN_FILENO
			// --> close PFD_READ for STDOUT_FILENO
			// Leave file descriptors untouched for later access after execle()
			if (rpw.close() < 0)
				throw util::sys_error(error + "close(STDIN_FILENO::0) failed.", errno);
			if (wpr.close() < 0)
				throw util::sys_error(error + "close(STDOUT_FILENO::0) failed.", errno);

			if (hasPath)
				execve(command.c_str(), (char* const*)argv(), (char* const*)env());
			else
				execvpe(command.c_str(), (char* const*)argv(), (char* const*)env());

			// Execution failed
			failure = true;
			_exit(200);

			break; // case (pid_t)0

		// Parent process execution continues here!
		default:

			// Unknown return value: pid < -1
			if (pid < (pid_t)-1)
				throw util::sys_error(error + "forking process failed (pid=" + std::to_string((size_s)pid) + ")", errno);

			// Close pipes here (blocked while executing command!!!)
			if (rpr.close() < 0)
				throw util::sys_error(error + "close(STDIN_FILENO::1) failed.", errno);
			if (wpw.close() < 0)
				throw util::sys_error(error + "close(STDOUT_FILENO::1) failed.", errno);

			readerPipe[PFD_READ] = -1;
			writerPipe[PFD_WRITE] = -1;

			// Read process output from pipe
			bool terminate = false;
			TBuffer p(PROCESS_OUTPUT_BUFFER_SIZE - 1);
			ssize_t r;
			do {
				// Read data from process output pipe
				r = read(writerPipe[PFD_READ], p.data(), p.size());
				switch(r) {

					// No more data available (EOF)
					case (ssize_t)0:
						terminate = true;
						retVal = true;
						break;

					// Check for read error or signal interrupt
					case (ssize_t)-1:
						if ((errno == EINTR) || (errno == EAGAIN) ) {
							errno = EXIT_SUCCESS;
						} else {
							terminate = true;
							// Ignore error here: just return with retVal = false!
							// throw util::sys_error(error + "read() failed.", errno);
						}
						break;

					// Append read data
					default:
						if (r > (ssize_t)0) {
							output.append(p.data(), r);
							output.reserve(output.size() + 2 * PROCESS_OUTPUT_BUFFER_SIZE);
						} else {
							// Unknown return value: r < -1
							terminate = true;
						}
						break;

				} // switch(r)

			} while (!terminate);

			// Wait for process terminated
			if (pid != waitpid(pid, &status, 0))
				throw util::sys_error(error + "waitpid() failed.", errno);
			result = WEXITSTATUS(status);

			break; // default:

	} // switch(pid = fork())

	if (failure || 200 == result) {
		result = -200;
		retVal = false;
	}

	for (int i=0; util::assigned(argv[i]); i++) {
		std::cout << util::succ(i) << ". Parameter after execution: " << util::charToStr(argv[i]) << std::endl;
	}

	return retVal;
}



