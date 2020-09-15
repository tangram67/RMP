/*
 * rs232.cpp
 *
 *  Created on: 15.09.2014
 *      Author: Dirk Brinkmeier
 */

#include <iostream>
#include <iomanip>
#include <cstring>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/serial.h>
#include <fcntl.h>
#include <unistd.h>
#include "rs232.h"
#include "compare.h"
#include "datetime.h"
#include "templates.h"
#include "exception.h"
#include "fileutils.h"


namespace app {

CBaudRate BaudRates[] = {
#ifdef B0
    { UINT32_C(0), B0 }, /* hang up */
#endif
#ifdef B50
    { UINT32_C(50), B50 },
#endif
#ifdef B75
    { UINT32_C(75), B75 },
#endif
#ifdef B110
    { UINT32_C(110), B110 },
#endif
#ifdef B134
    { UINT32_C(134), B134 },
#endif
#ifdef B150
    { UINT32_C(150), B150 },
#endif
#ifdef B200
    { UINT32_C(200), B200 },
#endif
#ifdef B300
    { UINT32_C(300), B300 },
#endif
#ifdef B600
    { UINT32_C(600), B600 },
#endif
#ifdef B1200
    { UINT32_C(1200), B1200 },
#endif
#ifdef B1200
    { UINT32_C(1200), B1200 },
#endif
#ifdef B1800
    { UINT32_C(1800), B1800 },
#endif
#ifdef B2400
    { UINT32_C(2400), B2400 },
#endif
#ifdef B4800
    { UINT32_C(4800), B4800 },
#endif
#ifdef B9600
    { UINT32_C(9600), B9600 },
#endif
#ifdef B19200
    { UINT32_C(19200), B19200 },
#endif
#ifdef B38400
    { UINT32_C(38400), B38400 },
#endif
#ifdef B57600
    { UINT32_C(57600), B57600 },
#endif
#ifdef B76800
    { UINT32_C(76800), B76800 },
#endif
#ifdef B115200
    { UINT32_C(115200), B115200 },
#endif
#ifdef B153600
    { UINT32_C(153600), B153600 },
#endif
#ifdef B230400
    { UINT32_C(230400), B230400 },
#endif
#ifdef B307200
    { UINT32_C(307200), B307200 },
#endif
#ifdef B460800
    { UINT32_C(460800), B460800 },
#endif
#ifdef B500000
    { UINT32_C(500000), B500000 },
#endif
#ifdef B576000
    { UINT32_C(576000), B576000 },
#endif
#ifdef B921600
    { UINT32_C(921600), B921600 },
#endif
#ifdef B1000000
    { UINT32_C(1000000), B1000000 },
#endif
#ifdef B1152000
    { UINT32_C(1152000), B1152000 },
#endif
#ifdef B1500000
    { UINT32_C(1500000), B1500000 },
#endif
#ifdef B2000000
    { UINT32_C(2000000), B2000000 },
#endif
#ifdef B2000000
    { UINT32_C(2000000), B2000000 },
#endif
#ifdef B3000000
    { UINT32_C(3000000), B3000000 },
#endif
#ifdef B3500000
    { UINT32_C(3500000), B3500000 },
#endif
#ifdef B4000000
    { UINT32_C(4000000), B4000000 }
#endif
};

#define kBaudRates (sizeof(BaudRates) / sizeof(BaudRates[0]))

/*
 *
 *  Here: chars = MIN, delay = TIME
 *  ===============================
 *
 *	MIN == 0; TIME == 0:
 *	If data is available, read(2) returns immediately, with the lesser of the number of bytes available,
 *	or the number of bytes requested. If no data is available, read(2) returns 0.
 *
 *	MIN > 0; TIME == 0:
 *	read(2) blocks until the lesser of MIN bytes or the number of bytes requested are available,
 *	and returns the lesser of these two values.
 *
 *	MIN == 0; TIME > 0:
 *	TIME specifies the limit for a timer in tenth of a second.
 *	The timer is started when read(2) is called. read(2) returns either
 *	when at least one byte of data is available, or when the timer expires.
 *	If the timer expires without any input becoming available, read(2) returns 0.
 *
 *	MIN > 0; TIME > 0:
 *	TIME specifies the limit for a timer in tenth of a second.
 *	Once an initial byte of input becomes available, the timer is restarted
 *	after each further byte is received. read(2) returns either when the lesser
 *	of the number of bytes requested or MIN byte have been read, or when the
 *	inter-byte timeout expires. Because the timer is only started after
 *	the initial byte becomes available, at least one byte will be read.
 *
 */

std::string getTerminalDriverName(const std::string& driver) {
	if (!driver.empty()) {
		if (0 == util::strcasecmp(driver, "serial"))
			return "RS232";
		if (util::strcasestr(driver, "ftdi"))
			return "FTDI/USB";
		if (util::strcasestr(driver, "pl23"))
			return "Prolific/USB";
		if (0 == util::strcasecmp(driver, "serial8250"))
			return "Terminal";
	}
	return driver;
}

bool getTerminalDeviceDriver(const std::string& device, std::string& driver) {
	if (!device.empty()) {
		struct stat st;

		// Get device root and driver path
		std::string root = device + "/device";
		std::string path = root + "/driver";

		// Check for device driver symlink
		if (0 == lstat(root.c_str(), &st) && S_ISLNK(st.st_mode)) {
			util::TBuffer buffer(1024);

			// Read symbolic driver name link
			ssize_t size = readlink(path.c_str(), buffer.data(), buffer.size());
			if (size > 0) {
				std::string name = std::string(buffer.data(), size);
				if (!name.empty()) {
					driver = util::fileBaseName(name);
					return !driver.empty();
				}
			}
		}
	}
	return false;
}

bool isTerminalDevice(const std::string& device) {
	bool r = false;
	if (!device.empty()) {
		struct serial_struct info;
		int fd = open(device.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);
		if (fd >= 0) {
			if (0 == ioctl(fd, TIOCGSERIAL, &info)) {
				// if (info.type != PORT_UNKNOWN)
				r = true;
			}
			close(fd);
		}
	}
	return r;
}

bool scanTerminalDevices(app::TStringVector& devices, const bool extended, const bool nolock) {
	devices.clear();
	bool retVal = false;
	util::TDirectory dir;
	struct dirent *file;
	std::string device, name, driver;
	std::string root = "/sys/class/tty/";
	dir.open(root);
	if (dir.isOpen()) {
		while(util::assigned((file = dir.read()))) {
			if((0 == ::strcmp(file->d_name, ".")) || (0 == ::strcmp(file->d_name, "..")))
				continue;

			// Device is always symbolic link
			if (file->d_type == DT_LNK) {
            	// Get device driver name
				name = root + file->d_name;
            	if (getTerminalDeviceDriver(name, driver)) {
					// Check for "real world" device
					if (0 != util::strcasecmp(driver, "serial8250")) {
						// Check for valid serial device
						bool ok = true;
						device = "/dev/" + util::fileBaseName(name);
						if (!nolock)
							ok = isTerminalDevice(device);
						if (ok) {
							if (extended)
								device += " [" + getTerminalDriverName(driver) + "]";
							devices.push_back(device);
							continue;
						}
					}
				}
            }
		}
		retVal = (devices.size() > 0);
		dir.close();
	}
	return retVal;
}


TSerial::TSerial() : TFileHandle() {
	device = "/dev/null";
	baud = 0;
	chars = 0;
	delay = 5;
	blocking = SER_DEV_UNDEFINED;
	defFileFd = INVALID_HANDLE_VALUE;
	selected = false;
}

TSerial::~TSerial() {
	close();
}


speed_t TSerial::getBaudRate(TBaudRate value) {
	int i, n = kBaudRates;
	for (i=0; i<n; i++) {
		if (BaudRates[i].value == value)
			return BaudRates[i].baud;
	}
	return B0;
}

void TSerial::open(const std::string& device, TBaudRate baud, ESerialBlockingType blocking) {
	// To use serial device as user for Ubuntu:
	// sudo usermod -a -G dialout $USER
    int retVal;
    speed_t speed;
    struct termios settings;
    memset (&settings, 0, sizeof(settings));

    if (!device.size()) {
		throw util::app_error("TSerial::open: No device set.");
    }

    if (!util::fileExists(device)) {
		throw util::app_error("TSerial::open: Device <" + device + "> not found." );
    }

    speed = getBaudRate(baud);
    if (speed == B0) {
		throw util::app_error("TSerial::open: Invalid baud rate " + std::to_string((size_u)baud));
    }

    if (EXIT_ERROR == TFileHandle::open(device, O_RDWR | O_NOCTTY | O_SYNC)) {
		throw util::sys_error("TSerial::open: Access to device <" + device + "> denied.", errno);
    }

	if (blocking == SER_DEV_NO_BLOCK) {
		if (!control(O_NONBLOCK)) {
			close();
			throw util::sys_error("TSerial::open: fcntl() failed for device <" + device + ">, fd " + std::to_string((size_s)handle()), errno);
		}
	}

    do {
		errno = EXIT_SUCCESS;
    	retVal = tcgetattr(handle(), &settings);
    } while (retVal == EXIT_ERROR && errno == EINTR);
    if (retVal == EXIT_ERROR) {
		close();
		throw util::sys_error("TSerial::open: tcgetattr() failed for device <" + device + ">, fd " + std::to_string((size_s)handle()), errno);
    }

    // Initialize to raw mode and set speed
    cfmakeraw(&settings);
    cfsetispeed(&settings, speed);
    cfsetospeed(&settings, speed);

    // Set bits per byte
    settings.c_cflag &= ~CSIZE;
    settings.c_cflag |=  CS8;

    // Ignore modem lines and enable receiver
    settings.c_cflag |= (CLOCAL | CREAD);

    // No flow control and no parity
    settings.c_cflag &= ~CRTSCTS;        	// No hardware flow control
    settings.c_iflag &= ~(IXON | IXOFF); 	// No software flow control
    settings.c_iflag &= ~IGNBRK;			// Disable break processing
    settings.c_cflag &= ~(PARENB | PARODD);	// No parity

    // Set blocking parameters
	settings.c_cc[VMIN]  = chars; // Wait for n chars received before returning
    settings.c_cc[VTIME] = delay; // Delay in deciseconds for read timeout

    // Set all attributes
    do {
		errno = EXIT_SUCCESS;
    	retVal = tcsetattr(handle(), TCSANOW, &settings);
    } while (retVal == EXIT_ERROR && errno == EINTR);
    if (retVal == EXIT_ERROR) {
		close();
		throw util::sys_error("TSerial::open: tcsetattr() failed for device <" + device + ">", errno);
    }

    // Clear input & output buffer
    do {
		errno = EXIT_SUCCESS;
    	tcflush(handle(), TCIOFLUSH);
    } while (retVal == EXIT_ERROR && errno == EINTR);
    if (retVal == EXIT_ERROR) {
		close();
		throw util::sys_error("TSerial::open: tcflush() failed for device <" + device + ">", errno);
    }

    /* Note: The above call may succeed even if only some of
     *       the flags were supported. We could tcgetattr()
     *       into a temporary struct, and compare fields
     *       (masked by the significant modes) to make sure.
     */
	this->blocking = blocking;
    this->device = device;
	this->baud = baud;
}

void TSerial::reopen(const std::string& device, TBaudRate baud, ESerialBlockingType blocking) {
	if (isOpen()) {
		inhibit();
	}
	if (!isOpen()) {
		open(device, baud, blocking);
	} else
		throw util::app_error("TSerial::reopen() failed for device <" + device + ">");
}

void TSerial::mode(int chars, int delay) {
	if ((this->chars == chars) && (this->delay == delay))
		return;

	int retVal;
	struct termios settings;
	memset (&settings, 0, sizeof(settings));

	do {
		errno = EXIT_SUCCESS;
		retVal = tcgetattr(handle(), &settings);
	} while (retVal == EXIT_ERROR && errno == EINTR);
	if (retVal == EXIT_ERROR) {
		close();
		throw util::sys_error("TSerial::mode: tcgetattr() failed for device <" + device + ">", errno);
	}

	// Set minimum count of chars before returning from read
	// OR deciseconds to wait for receiving some chars
	settings.c_cc[VMIN]  = chars;
	settings.c_cc[VTIME] = delay;

    do {
		errno = EXIT_SUCCESS;
    	retVal = tcsetattr(handle(), TCSANOW, &settings);
    } while (retVal == EXIT_ERROR && errno == EINTR);
    if (retVal == EXIT_ERROR) {
		close();
		throw util::sys_error("TSerial::mode: tcsetattr() failed for device <" + device + ">", errno);
    }

	this->chars = chars;
	this->delay = delay;
}

void TSerial::close() {
	// Wait for select select()
	if (selected) {
		terminate();
		while (selected) util::wait(30);
	}	
	shutdown();
}

void TSerial::inhibit() {
	// Wait for select select()
	if (selected) {
		notify();
		while (selected) util::wait(30);
	}
	finalize();
}

void TSerial::finalize() {
	app::TLockGuard<app::TMutex> lock(mtx);
	device = "/dev/null";
	util::TFileHandle::close();
}

void TSerial::shutdown() {
	finalize();
	deleteDefaultDescriptor();
}


ssize_t TSerial::recv(void *const data, size_t const size) const {
	ssize_t r;

    // Receive data from device
	do {
	    do {
			errno = EXIT_SUCCESS;
			r = ::read(handle(), data, size);
	    } while (r == (ssize_t)EXIT_ERROR && errno == EINTR);

    	if (r == (ssize_t)EXIT_ERROR && errno == EWOULDBLOCK) {
    		// Sleep for 10 millisecond, then retry
    		util::saveWait(10);
    	}
    } while ((r == 0) && chars);

    // Read some bytes...
    if (r >= (ssize_t)0) {
    	return r;
    }

	// Ignore blocking serial device
	if (r == EXIT_ERROR && (errno == EWOULDBLOCK) && (blocking == SER_DEV_NO_BLOCK)) {
		errno = EXIT_SUCCESS;
		return (ssize_t)0;
	}

	// Ignore error result of read() if errno not set
    if (r == (ssize_t)EXIT_ERROR) {
    	if (errno != EXIT_SUCCESS)
    		return (ssize_t)EXIT_ERROR;
    	else
    		return (ssize_t)0;
    }

    // Invalid result of ::read()
    // Should not happen by design!
	if (EXIT_SUCCESS == errno)
		errno = EIO;

	return (ssize_t)EXIT_ERROR;
}

ssize_t TSerial::send(void const *const data, size_t const size) const {
	char const *p = (char const *)data;
    char const *const q = (char const *)data + size;
    ssize_t r;

    while (p < q) {
    	do {
    		errno = EXIT_SUCCESS;
    		r = ::write(handle(), p, (size_t)(q - p));
    	} while (r == (ssize_t)EXIT_ERROR && errno == EINTR);

    	if (r == (ssize_t)EXIT_ERROR && errno == EWOULDBLOCK) {
    		// Sleep for 10 millisecond, then retry
    		util::saveWait(10);
    		continue;
    	}

        // Write failed
    	if (r == (ssize_t)EXIT_ERROR)
        	return (ssize_t)EXIT_ERROR;

		// Something else went wrong
		// --> error must be EIO
    	if (r < (ssize_t)1) {
    		if (EXIT_SUCCESS == errno)
    			errno = EIO;
        	return (ssize_t)EXIT_ERROR;
    	}

    	p += (size_t)r;
    }

    // Unexpected pointer after data transfer
    if (p != q) {
		if (EXIT_SUCCESS == errno)
			errno = EIO;
    	return (ssize_t)EXIT_ERROR;
    }

    // Buffer has been fully written
    // Possible overflow on result value (ssize_t)size !!!
    return (ssize_t)size;
}


size_t TSerial::read(void *const data, size_t const size) const {
	// Receive data from device
	ssize_t r = recv(data, size);

    // Read failed
	if (r == EXIT_ERROR)
    	throw util::sys_error("TSerial::read() : Could not read from device <" + device + ">, return value = " + std::to_string((size_s)r));

	return (size_t)r;
}

void TSerial::read(std::string& s) const {
	util::TBuffer buffer(256);
	size_t r = read(buffer.data(), buffer.size());
	if (r > (ssize_t)0) s.assign(buffer.data(), r);
	else s.clear();
}

inline std::string TSerial::read() const {
	std::string s;
	read(s);
	return s;
}

/*
 *	Nice example for using const qualifier...
 *
 *	char       *       --> mutable pointer to mutable char
 *	char const *       --> mutable pointer to constant char
 *	char       * const --> constant pointer to mutable char
 *	char const * const --> constant pointer to constant char
 */
size_t TSerial::write(void const *const data, size_t const size) const {
	// Write buffer to device
	ssize_t r = send(data, size);

	if (EXIT_ERROR == r)
		throw util::sys_error("TSerial::write() : Could not write to device <" + device + ">, return value = " + std::to_string((size_s)r));

	if (size != (size_t)r)
		throw util::sys_error("TSerial::write() : Could not write all data to device <" + device + ">, " + std::to_string((size_u)r) +
				" bytes written from " + std::to_string((size_u)size) + " bytes.");

	return (size_t)r;
}

void TSerial::write(const std::string& s) const {
	write(s.data(), s.size());
}

void TSerial::write(const char c) const {
	write(&c, 1);
}

void TSerial::write(std::stringstream& sstrm) {
	sstrm.seekg(0, std::ios::end);
	size_t size = sstrm.tellg();
	if (size > 0) {
		write(sstrm.str().data(), sstrm.str().size());
		sstrm.str(std::string());
		resetios();
	}			
}


void TSerial::resetios() {
	line.copyfmt(std::ios(NULL));
}


int TSerial::defaultDescriptorNeeded() {
	if (defFileFd == INVALID_HANDLE_VALUE) {
		createDefaultDescriptor();
	}
	return defFileFd;
}

void TSerial::createDefaultDescriptor() {
	defFileName = "/tmp/." + util::toupper(util::fileBaseName(device)) + "-" + util::fastCreateUUID(true, true);
	int r = mkfifo(defFileName.c_str(), 0644);
	if (EXIT_ERROR == r) {
		throw util::sys_error("TFileWatch::createDefaultDescriptor() Create FIFO failed.");
	}
	defFileFd = ::open(defFileName.c_str(), O_RDWR | O_NONBLOCK);
	if (defFileFd <= 0) {
		defFileFd = INVALID_HANDLE_VALUE;
		throw util::sys_error("TFileWatch::createDefaultDescriptor() Open FIFO failed.");
	}
}

void TSerial::changeDefaultDescriptor(const char* event) {
	if (util::assigned(event)) {
		ssize_t s = strlen(event);
		if (s != writeDefaultDescriptor(event, s))
			throw util::sys_error("TFileWatch::changeDefaultDescriptor() Writing default file descriptor failed.");
	}
}

void TSerial::deleteDefaultDescriptor() {
	if (defFileFd != INVALID_HANDLE_VALUE) {
		::close(defFileFd);
		defFileFd = INVALID_HANDLE_VALUE;
		util::deleteFile(defFileName);
	}
}


ssize_t TSerial::readDefaultDescriptor(void *const data, const size_t size) const {
	ssize_t r;

    do {
		errno = EXIT_SUCCESS;
		r = ::read(defFileFd, data, size);
    } while (r == (ssize_t)EXIT_ERROR && errno == EINTR);
	errval = errno;

    // Read some bytes...
    if (r >= (ssize_t)0) {
    	return r;
    }

    // Ignore error result of read() if errno not set
    if (r == (ssize_t)EXIT_ERROR) {
    	if (errno != EXIT_SUCCESS)
    		return (ssize_t)EXIT_ERROR;
    	else
    		return (ssize_t)0;
    }

    // Invalid result of ::read()
    // Should not happen by design!
	if (EXIT_SUCCESS == errno)
		errno = EIO;

	return (ssize_t)EXIT_ERROR;
}

ssize_t TSerial::writeDefaultDescriptor(void const *const data, size_t const size) const {
    char const *p = (char const *)data;
    char const *const q = (char const *)data + size;
    ssize_t r;

	// Write data
    while (p < q) {
    	do {
    		errno = EXIT_SUCCESS;
    		r = ::write(defFileFd, p, (size_t)(q - p));
    	} while (r == (ssize_t)EXIT_ERROR && errno == EINTR);

    	// Write failed
    	if (r == (ssize_t)EXIT_ERROR)
        	return (ssize_t)EXIT_ERROR;

		// Something else went wrong
    	if (r < (ssize_t)1) {
    		if (EXIT_SUCCESS == errno)
    			errno = EIO;
        	return (ssize_t)EXIT_ERROR;
    	}

    	p += (size_t)r;
    }

    // Unexpected pointer values after data transfer
    if (p != q) {
		if (EXIT_SUCCESS == errno) {
			errno = EFAULT;
		}
    	return (ssize_t)EXIT_ERROR;
    }

    // Buffer has been fully written
    return (ssize_t)size;
}

void TSerial::notify() {
	if (INVALID_HANDLE_VALUE != defFileFd) {
		char p[3] = "x\n";
		p[0] = (char)EVENT_CHANGED;
		changeDefaultDescriptor(p);
	}
}

void TSerial::terminate() {
	if (INVALID_HANDLE_VALUE != defFileFd) {
		char p[3] = "x\n";
		p[0] = (char)EVENT_TERMINATE;
		changeDefaultDescriptor(p);
	}
}

TEventResult TSerial::event() {
	if (INVALID_HANDLE_VALUE != defFileFd) {
		util::TBuffer data(128);
		ssize_t r = readDefaultDescriptor(data(), data.size());
		if (r > 0) {
			if (data[0] == (char)EVENT_CHANGED)
				return EV_CHANGED;
			if (data[0] == (char)EVENT_CLOSED)
				return EV_CLOSED;
			if (data[0] == (char)EVENT_TERMINATE)
				return EV_TERMINATE;
		}
		return EV_TERMINATE;
	}
	return EV_ERROR;
}

int TSerial::select(int ndfs, fd_set *rfds, util::TTimePart ms) {
	errno = EXIT_SUCCESS;
	if (ms > 0) {
		return sigSaveTimedSelect(ndfs, rfds, ms);
	} else {
		return sigSaveSelect(ndfs, rfds);
	}
	return EXIT_ERROR;
}

int TSerial::sigSaveSelect(int ndfs, fd_set *rfds) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::select(ndfs, rfds, NULL, NULL, NULL);
	} while (r < 0 && errno == EINTR);
	return r;
}

int TSerial::sigTimedSelect(int ndfs, fd_set *rfds, util::TTimePart ms) {
	if (ms > 0) {
		timeval tv;
		tv.tv_sec = ms / 1000;
		tv.tv_usec = (ms % 1000) * 1000;
		errno = EXIT_SUCCESS;
		return ::select(ndfs, rfds, NULL, NULL, &tv);
	}
	errno = EINVAL;
	return EXIT_ERROR;
}

int TSerial::sigSaveTimedSelect(int ndfs, fd_set *rfds, util::TTimePart ms)
{
	util::TDateTime ts;
	util::TTimePart tm = ms, dt = 0;
	int r = EXIT_SUCCESS, c = 0;
	do {
		ts.start();
		r = sigTimedSelect(ndfs, rfds, tm);
		if (r < 0 && errno == EINTR) {
			// select() was interrupted
			dt = ts.stop(util::ETP_MILLISEC);
			if (dt > (tm * 95 / 100))
				break;
			tm -= dt;
			if (tm < 5)
				break;
			c++;
		} else {
			// select() returned on error!
			break;
		}
	} while (c < 10);
	return r;
}

TEventResult TSerial::wait(util::TByteBuffer& data, const size_t size, const util::TTimePart milliseconds) {
	app::TLockGuard<app::TMutex> lock(mtx);
	util::TBooleanGuard<bool> bg(selected);
	selected = true;
	data.clear();

	if (!isOpen())
		return EV_CLOSED;

	// Get file descriptors
	int fd1 = defaultDescriptorNeeded();
	int fd2 = handle();
	int max = (fd1 > fd2) ? fd1 : fd2;
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(fd1, &rfds);
	FD_SET(fd2, &rfds);

	// Wait...
	int r = select(max+1, &rfds, milliseconds);
	selected = false;

	// No file descriptor ready?
	if (r == 0 && milliseconds > 0) {
		return EV_TIMEDOUT;
	}

	// File descriptor fired?
	if (r > 0) {
		if (FD_ISSET(fd1, &rfds)) {
			return event();
		}
		if (FD_ISSET(fd2, &rfds)) {
			data.reserve(size+1, false);
			ssize_t s = recv(data(), size);
			if (s > 0) {
				data.resize(s, true);
				return EV_SIGNALED;
			}
		}
	}

	if (!isOpen())
		return EV_CLOSED;

	return EV_ERROR;
}


const TSerial& TSerial::operator<< (const std::string& s) const {
	write(s);
	return *this;
}

const TSerial& TSerial::operator>> (std::string& s) const {
	s = read();
	return *this;
}


inline std::ostream& operator<< (std::ostream& os, app::TSerial& o) {
	std:: string s;
	o >> s;
	os << s;
	return os;
}

inline std::istream& operator>> (std::istream& is, app::TSerial& o) {
	std:: string s;
	is >> s;
	o << s;
	return is;
}


} /* namespace app */
