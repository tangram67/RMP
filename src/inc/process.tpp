/*
 * process.tpp
 *
 *  Created on: 21.05.2015
 *      Author: Dirk Brinkmeier
 *      
 *  Template defintion in conjunction with process.h
 *  This file must NOT be included in project make/build files!
 *  
 *  Needed includes (via process.h)
 *  
 *    include <sys/wait.h>  // Needed for waitpid in process.tpp
 *    include "utilities.h" // Needed for template definition in process.tpp
 *    include "exception.h" // Needed for template definition in process.tpp
 *    include "classes.h"   // Needed for template definition in process.tpp
 *    include "helper.h"    // Needed for template definition in process.tpp
 *    include "gcc.h"       // Needed for template definition in process.tpp
 *       
 *  executeProcess() can be instantiated for std::string or util::CBuffer
 *        
 */
namespace util {

template<typename output_t>
bool fastExecuteProcess(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, output_t&& output, int& result, util::TTimePart timeout)
{
	bool retVal = false;
	util::TTimePart bailout = util::now() + timeout; // Maximum process execution time
	std::string error = "executeProcess(" + command + ") : ";
	std::string path = folder;
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

	// Try forking child process for executing command
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
	
			// Close unused pipes
			if (rpr.close() < 0)
				throw util::sys_error(error + "close(STDIN_FILENO::1) failed.", errno);
			if (wpw.close() < 0)
				throw util::sys_error(error + "close(STDOUT_FILENO::1) failed.", errno);

			readerPipe[PFD_READ] = -1;
			writerPipe[PFD_WRITE] = -1;
	
			// Read process output from pipe
			bool killer = false;
			bool terminate = false;
			TBuffer p(PROCESS_OUTPUT_BUFFER_SIZE - 1);
			int reader = writerPipe[PFD_READ];
			ssize_t size;
			int r;
			do {

				// Prepare select on reader pipe
				struct timeval tv;
				struct timeval* pv;
				fd_set set;
				FD_ZERO(&set);
				FD_SET(reader, &set);
				tv.tv_sec = 0;
				tv.tv_usec = 500000;
				pv = (timeout > 0) ? &tv : NULL;

				// Wait for data or timeout
				r = select(reader + 1, &set, NULL, NULL, pv);
				if(r == EXIT_ERROR) {
					if (!terminate)
						killer = true;
				} else if (r == 0) {
					// Terminate running process group on timeot exceeded
					if (!terminate && timeout > 0 && util::now() > bailout) {
						killer = true;
					}
				} else {
					// Read data from process output pipe
					size = read(reader, p.data(), p.size());
					switch(size) {
		
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
							if (size > (ssize_t)0) {
								output.append(p.data(), size);
								output.reserve(output.size() + 2 * PROCESS_OUTPUT_BUFFER_SIZE);
							} else {
								// Unknown return value: r < -1
								killer = true;
							}
							break;
		
					} // switch(s)

					// Terminate running process group on timout exeeded...
					if (!terminate && util::now() > bailout) {
						killer = true;
					}
				}

				// Kill running process tree on error or timeout exceeded
				if (killer) {
					terminate = true;
					kill(-pid, SIGTERM);
					util::sleep(1);
					kill(-pid, SIGKILL);
				}
	
			} while (!terminate);
	
			// Wait for process terminated
			int status;
			if (pid != waitpid(pid, &status, 0))
				throw util::sys_error(error + "waitpid() failed.", errno);
			result = WEXITSTATUS(status);
			break; // default

	} // switch(pid = fork())

	if (failure || 200 == result) {
		result = -200;
		retVal = false;
	}

	return retVal;
}

template<typename output_t>
bool executeProcess(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, output_t&& output, int& result, util::TTimePart timeout)
{
	bool retVal = false;
	util::TTimePart bailout = util::now() + timeout; // Maximum process execution time
	std::string error = "executeProcess(" + command + ") : ";
	std::string path = folder;
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

	// Piped I/O handles
	int readerPipe[2] = { -1, -1 };
	int writerPipe[2] = { -1, -1 };
	int syncPipe1[2] = { -1, -1 };
	int syncPipe2[2] = { -1, -1 };
	char c = '*';

	// Create pipes for process output
	if (pipe(readerPipe) < 0)
		throw util::sys_error(error + "pipe(readerPipe) failed.", errno);
	if (pipe(writerPipe) < 0)
		throw util::sys_error(error + "pipe(writerPipe) failed.", errno);

	// Create pipes for process synchronization
	if (EXIT_SUCCESS != pipe(syncPipe1))
		throw util::sys_error(error + "pipe(syncPipe1) failed.", errno);
	if (EXIT_SUCCESS != pipe(syncPipe2))
		throw util::sys_error(error + "pipe(syncPipe2) failed.", errno);

	// RAAI guards for pipe handles
	TDescriptorGuard<int> rpr(readerPipe[PFD_READ]);
	TDescriptorGuard<int> rpw(readerPipe[PFD_WRITE]);
	TDescriptorGuard<int> wpr(writerPipe[PFD_READ]);
	TDescriptorGuard<int> wpw(writerPipe[PFD_WRITE]);
	TDescriptorGuard<int> spr1(syncPipe1[PFD_READ]);
	TDescriptorGuard<int> spw1(syncPipe1[PFD_WRITE]);
	TDescriptorGuard<int> spr2(syncPipe2[PFD_READ]);
	TDescriptorGuard<int> spw2(syncPipe2[PFD_WRITE]);

	// Try forking child process for executing command
	pid_t pid = fork();
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

			// Write to sync pipe to unlock parent process
			spw2.write(&c, 1);

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

			// Wait for parent to set PGID
			spr1.read(&c, 1);

			// Excute command depending on program path set or not
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

			// Wait for parent child process to continue
			spr2.read(&c, 1);

			// setpgid() sets the PGID of the process specified by pid to pgid.
			// If pid is zero, then the process ID of the calling process is used.
			// If pgid is zero, then the PGID of the process specified by pid is made
			// the same as its process ID.
			if (EXIT_SUCCESS != setpgid(pid, 0)) {
				spw1.write(&c, 1);
				throw util::sys_error(error + "setting PGID failed.", errno);
			}

			// Signal that PID was set to child process
			spw1.write(&c, 1);

			// Close unused pipes
			if (rpr.close() < 0)
				throw util::sys_error(error + "close(STDIN_FILENO::1) failed.", errno);
			if (wpw.close() < 0)
				throw util::sys_error(error + "close(STDOUT_FILENO::1) failed.", errno);

			readerPipe[PFD_READ] = -1;
			writerPipe[PFD_WRITE] = -1;

			// Read process output from pipe
			bool killer = false;
			bool terminate = false;
			TBuffer p(PROCESS_OUTPUT_BUFFER_SIZE - 1);
			int reader = writerPipe[PFD_READ];
			ssize_t size;
			int r;
			do {

				// Prepare select on reader pipe
				struct timeval tv;
				struct timeval* pv;
				fd_set set;
				FD_ZERO(&set);
				FD_SET(reader, &set);
				tv.tv_sec = 0;
				tv.tv_usec = 500000;
				pv = (timeout > 0) ? &tv : NULL;

				// Wait for data or timeout
				r = select(reader + 1, &set, NULL, NULL, pv);
				if(r == EXIT_ERROR) {
					if (!terminate)
						killer = true;
				} else if (r == 0) {
					// Terminate running process group on timeot exceeded
					if (!terminate && timeout > 0 && util::now() > bailout) {
						killer = true;
					}
				} else {
					// Read data from process output pipe
					size = read(reader, p.data(), p.size());
					switch(size) {

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
							if (size > (ssize_t)0) {
								output.append(p.data(), size);
								output.reserve(output.size() + 2 * PROCESS_OUTPUT_BUFFER_SIZE);
							} else {
								// Unknown return value: r < -1
								killer = true;
							}
							break;

					} // switch(s)

					// Terminate running process group on timout exeeded...
					if (!terminate && util::now() > bailout) {
						killer = true;
					}
				}

				// Kill running process tree on error or timeout exceeded
				if (killer) {
					terminate = true;
					kill(-pid, SIGTERM);
					util::sleep(1);
					kill(-pid, SIGKILL);
				}

			} while (!terminate);

			// Wait for process terminated
			int status;
			if (pid != waitpid(pid, &status, 0))
				throw util::sys_error(error + "waitpid() failed.", errno);
			result = WEXITSTATUS(status);
			break; // default

	} // switch(pid = fork())

	if (failure || 200 == result) {
		result = -200;
		retVal = false;
	}

	return retVal;
}

template<typename output_t>
bool fastExecuteCommand(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, output_t&& output, int& result, util::TTimePart timeout)
{
	bool retVal = false;
	util::TTimePart bailout = util::now() + timeout; // Maximum process execution time
	std::string error = "executeCommand(" + command + ") : ";
	std::string path = folder;
	bool failure = false;
	result = EXIT_FAILURE;
	errno = EXIT_SUCCESS;
	output.clear();

	// Check if command file exists
	if (!util::fileExists(command))
		return retVal;

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

	// Piped I/O handles
	int readerPipe[2] = { -1, -1 };
	int writerPipe[2] = { -1, -1 };

	// Create pipes for process output
	if (EXIT_SUCCESS != pipe(readerPipe))
		throw util::sys_error(error + "pipe(readerPipe) failed.", errno);
	if (EXIT_SUCCESS != pipe(writerPipe))
		throw util::sys_error(error + "pipe(writerPipe) failed.", errno);

	// RAAI guards for pipe handles
	TDescriptorGuard<int> rpr(readerPipe[PFD_READ]);
	TDescriptorGuard<int> rpw(readerPipe[PFD_WRITE]);
	TDescriptorGuard<int> wpr(writerPipe[PFD_READ]);
	TDescriptorGuard<int> wpw(writerPipe[PFD_WRITE]);

	// Try forking child process for executing command
#ifdef GLIBC_HAS_VFORK
	pid_t pid = vfork();
#else
	pid_t pid = fork();
#endif

	switch (pid) {

		case (pid_t)-1:
			std::cout << "executeCommand()::fork() failed: " << pid << std::endl;
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

			// Execute command with parameters (max. 10 parameters supported + $0 as self!):
			// int execle(const char *path, const char *arg, ..., (char *) NULL, char * const envp[]);
			// Attention: Use (char*)NULL als last parameter (not nullptr!)
			switch (parameter.size()) {
				case 0:
					execle(command.c_str(), command.c_str(), (char*)NULL, env());
					break;
				case 1:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), (char*)NULL, env());
					break;
				case 2:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), (char*)NULL, env());
					break;
				case 3:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(),
							parameter[2].c_str(), (char*)NULL, env());
					break;
				case 4:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(),
							parameter[2].c_str(), parameter[3].c_str(), (char*)NULL, env());
					break;
				case 5:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(),
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(), (char*)NULL, env());
					break;
				case 6:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(),
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(),
							parameter[5].c_str(), (char*)NULL, env());
					break;
				case 7:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(),
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(),
							parameter[5].c_str(), parameter[6].c_str(), (char*)NULL, env());
					break;
				case 8:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(),
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(),
							parameter[5].c_str(), parameter[6].c_str(), parameter[7].c_str(), (char*)NULL, env());
					break;
				case 9:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(),
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(),
							parameter[5].c_str(), parameter[6].c_str(), parameter[7].c_str(),
							parameter[8].c_str(), (char*)NULL, env());
					break;
				default:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(),
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(),
							parameter[5].c_str(), parameter[6].c_str(), parameter[7].c_str(),
							parameter[8].c_str(), parameter[9].c_str(), (char*)NULL, env());
					break;
			} // switch (parameter.size())

			// Execution failed
			failure = true;
			_exit(200);

			break; // case (pid_t)0

		// Parent process execution continues here!
		default:

			// Unknown return value: pid < -1
			if (pid < (pid_t)-1)
				throw util::sys_error(error + "forking process failed (pid=" + std::to_string((size_s)pid) + ")", errno);

			// Close unused pipes
			if (rpr.close() < 0)
				throw util::sys_error(error + "close(STDIN_FILENO::1) failed.", errno);
			if (wpw.close() < 0)
				throw util::sys_error(error + "close(STDOUT_FILENO::1) failed.", errno);

			readerPipe[PFD_READ] = -1;
			writerPipe[PFD_WRITE] = -1;

			// Read process output from pipe
			bool killer = false;
			bool terminate = false;
			TBuffer p(PROCESS_OUTPUT_BUFFER_SIZE - 1);
			int reader = writerPipe[PFD_READ];
			ssize_t size;
			int r;
			do {

				// Prepare select on reader pipe
				struct timeval tv;
				struct timeval* pv;
				fd_set set;
				FD_ZERO(&set);
				FD_SET(reader, &set);
				tv.tv_sec = 0;
				tv.tv_usec = 500000;
				pv = (timeout > 0) ? &tv : NULL;

				// Wait for data or timeout
				r = select(reader + 1, &set, NULL, NULL, pv);
				if(r == EXIT_ERROR) {
					if (!terminate)
						killer = true;
				} else if (r == 0) {
					// Terminate running process group on timeot exceeded
					if (!terminate && timeout > 0 && util::now() > bailout) {
						killer = true;
					}
				} else {
					// Read data from process output pipe
					size = read(reader, p.data(), p.size());
					switch(size) {

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
							if (size > (ssize_t)0) {
								output.append(p.data(), size);
								output.reserve(output.size() + 2 * PROCESS_OUTPUT_BUFFER_SIZE);
							} else {
								// Unknown return value: s < -1
								killer = true;
							}
							break;

					} // switch(s)

					// Terminate running process group on timout exeeded...
					if (!terminate && util::now() > bailout) {
						killer = true;
					}
				}

				// Kill running process tree on error or timeout exceeded
				if (killer) {
					terminate = true;
					kill(-pid, SIGTERM);
					util::sleep(1);
					kill(-pid, SIGKILL);
				}

			} while (!terminate);

			// Wait for process terminated
			int status;
			if (pid != waitpid(pid, &status, 0))
				throw util::sys_error(error + "waitpid() failed.", errno);
			result = WEXITSTATUS(status);
			break; // default

	} // switch(pid = fork())

	if (failure || 200 == result) {
		result = -200;
		retVal = false;
	}

	return retVal;
}


template<typename output_t>
bool executeCommand(const std::string& command, const util::TVariantValues& parameter, const std::string& folder, TEnvironment& environment, output_t&& output, int& result, util::TTimePart timeout)
{
	bool retVal = false;
	util::TTimePart bailout = util::now() + timeout; // Maximum process execution time
	std::string error = "executeCommand(" + command + ") : ";
	std::string path = folder;
	bool failure = false;
	result = EXIT_FAILURE;
	errno = EXIT_SUCCESS;
	output.clear();

	// Check if command file exists
	if (!util::fileExists(command))
		return retVal;

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

	// Piped I/O handles
	int readerPipe[2] = { -1, -1 };
	int writerPipe[2] = { -1, -1 };
	int syncPipe1[2] = { -1, -1 };
	int syncPipe2[2] = { -1, -1 };
	char c = '*';

	// Create pipes for process output
	if (EXIT_SUCCESS != pipe(readerPipe))
		throw util::sys_error(error + "pipe(readerPipe) failed.", errno);
	if (EXIT_SUCCESS != pipe(writerPipe))
		throw util::sys_error(error + "pipe(writerPipe) failed.", errno);

	// Create pipes for process synchronization
	if (EXIT_SUCCESS != pipe(syncPipe1))
		throw util::sys_error(error + "pipe(syncPipe1) failed.", errno);
	if (EXIT_SUCCESS != pipe(syncPipe2))
		throw util::sys_error(error + "pipe(syncPipe2) failed.", errno);

	// RAAI guards for pipe handles
	TDescriptorGuard<int> rpr(readerPipe[PFD_READ]);
	TDescriptorGuard<int> rpw(readerPipe[PFD_WRITE]);
	TDescriptorGuard<int> wpr(writerPipe[PFD_READ]);
	TDescriptorGuard<int> wpw(writerPipe[PFD_WRITE]);
	TDescriptorGuard<int> spr1(syncPipe1[PFD_READ]);
	TDescriptorGuard<int> spw1(syncPipe1[PFD_WRITE]);
	TDescriptorGuard<int> spr2(syncPipe2[PFD_READ]);
	TDescriptorGuard<int> spw2(syncPipe2[PFD_WRITE]);

	// Try forking child process for executing command
	pid_t pid = fork();
	switch (pid) {
	
		case (pid_t)-1:
			std::cout << "executeCommand()::fork() failed: " << pid << std::endl;
			throw util::sys_error(error + "forking process failed.", errno);
			break;

		// Execution of new child process starts here...
		case (pid_t)0:

			// After exec, all signal handlers are restored to their default values,
			// with one exception of SIGCHLD. According to POSIX.1-2001 and Linux's
			// implementation, SIGCHLD's handler will leave unchanged after exec
			// if it was set to be ignored. Restore it to default action.
			//signal(SIGCHLD, SIG_DFL);
	
			// Write to sync pipe to unlock parent process
			spw2.write(&c, 1);

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

			// Wait for parent to set PGID
			spr1.read(&c, 1);

			// Execute command with parameters (max. 10 parameters supported + $0 as self!):
			// int execle(const char *path, const char *arg, ..., (char *) NULL, char * const envp[]);
			// Attention: Use (char*)NULL als last parameter (not nullptr!)
			switch (parameter.size()) {
				case 0:
					execle(command.c_str(), command.c_str(), (char*)NULL, env());
					break;
				case 1:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), (char*)NULL, env());
					break;
				case 2:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), (char*)NULL, env());
					break;
				case 3:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), 
							parameter[2].c_str(), (char*)NULL, env());
					break;
				case 4:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), 
							parameter[2].c_str(), parameter[3].c_str(), (char*)NULL, env());
					break;
				case 5:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), 
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(), (char*)NULL, env());
					break;
				case 6:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), 
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(), 
							parameter[5].c_str(), (char*)NULL, env());
					break;
				case 7:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), 
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(), 
							parameter[5].c_str(), parameter[6].c_str(), (char*)NULL, env());
					break;
				case 8:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), 
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(), 
							parameter[5].c_str(), parameter[6].c_str(), parameter[7].c_str(), (char*)NULL, env());
					break;
				case 9:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), 
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(), 
							parameter[5].c_str(), parameter[6].c_str(), parameter[7].c_str(), 
							parameter[8].c_str(), (char*)NULL, env());
					break;
				default:
					execle(command.c_str(), command.c_str(), parameter[0].c_str(), parameter[1].c_str(), 
							parameter[2].c_str(), parameter[3].c_str(), parameter[4].c_str(), 
							parameter[5].c_str(), parameter[6].c_str(), parameter[7].c_str(), 
							parameter[8].c_str(), parameter[9].c_str(), (char*)NULL, env());
					break;
			} // switch (parameter.size())

			// Execution failed
			failure = true;
			_exit(200);

			break; // case (pid_t)0

		// Parent process execution continues here!
		default:

			// Unknown return value: pid < -1
			if (pid < (pid_t)-1)
				throw util::sys_error(error + "forking process failed (pid=" + std::to_string((size_s)pid) + ")", errno);

			// Wait for parent child process to continue
			spr2.read(&c, 1);
	
			// setpgid() sets the PGID of the process specified by pid to pgid.
			// If pid is zero, then the process ID of the calling process is used.
			// If pgid is zero, then the PGID of the process specified by pid is made
			// the same as its process ID.
			if (EXIT_SUCCESS != setpgid(pid, 0)) {
				spw1.write(&c, 1);
				throw util::sys_error(error + "setting PGID failed.", errno);
			}

			// Signal that PID was set to child process
			spw1.write(&c, 1);

			// Close unused pipes
			if (rpr.close() < 0)
				throw util::sys_error(error + "close(STDIN_FILENO::1) failed.", errno);
			if (wpw.close() < 0)
				throw util::sys_error(error + "close(STDOUT_FILENO::1) failed.", errno);

			readerPipe[PFD_READ] = -1;
			writerPipe[PFD_WRITE] = -1;
	
			// Read process output from pipe
			bool killer = false;
			bool terminate = false;
			TBuffer p(PROCESS_OUTPUT_BUFFER_SIZE - 1);
			int reader = writerPipe[PFD_READ];
			ssize_t size;
			int r;
			do {

				// Prepare select on reader pipe
				struct timeval tv;
				struct timeval* pv;
				fd_set set;
				FD_ZERO(&set);
				FD_SET(reader, &set);
				tv.tv_sec = 0;
				tv.tv_usec = 500000;
				pv = (timeout > 0) ? &tv : NULL;

				// Wait for data or timeout
				r = select(reader + 1, &set, NULL, NULL, pv);
				if(r == EXIT_ERROR) {
					if (!terminate)
						killer = true;
				} else if (r == 0) {
					// Terminate running process group on timeot exceeded
					if (!terminate && timeout > 0 && util::now() > bailout) {
						killer = true;
					}
				} else {
					// Read data from process output pipe
					size = read(reader, p.data(), p.size());
					switch(size) {
		
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
							if (size > (ssize_t)0) {
								output.append(p.data(), size);
								output.reserve(output.size() + 2 * PROCESS_OUTPUT_BUFFER_SIZE);
							} else {
								// Unknown return value: s < -1
								killer = true;
							}
							break;
		
					} // switch(s)

					// Terminate running process group on timout exeeded...
					if (!terminate && util::now() > bailout) {
						killer = true;
					}
				}
	
				// Kill running process tree on error or timeout exceeded
				if (killer) {
					terminate = true;
					kill(-pid, SIGTERM);
					util::sleep(1);
					kill(-pid, SIGKILL);
				}

			} while (!terminate);
	
			// Wait for process terminated
			int status;
			if (pid != waitpid(pid, &status, 0))
				throw util::sys_error(error + "waitpid() failed.", errno);
			result = WEXITSTATUS(status);
			break; // default

	} // switch(pid = fork())

	if (failure || 200 == result) {
		result = -200;
		retVal = false;
	}

	return retVal;
}


}

