/*
 * rs232.h
 *
 *  Created on: 15.09.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef RS232_H_
#define RS232_H_

#include <termios.h>
#include <string>
#include <sstream>
#include <iostream>
#include "fileutils.h"
#include "semaphores.h"
#include "ipctypes.h"

namespace app {

class TSerial;

#ifdef STL_HAS_TEMPLATE_ALIAS

using TBaudRate = uint32_t;
using PSerial = TSerial*;

#else

typedef unsigned int TBaudRate;
typedef TSerial* PSerial;

#endif


enum ESerialBlockingType {
	SER_DEV_UNDEFINED,
	SER_DEV_NO_BLOCK,
	SER_DEV_BLOCKING,
	SER_DEV_DEFAULT = SER_DEV_NO_BLOCK
};

struct CBaudRate {
	TBaudRate value;
	speed_t baud;
};


bool isTerminalDevice(const std::string& device);
std::string getTerminalDriverName(const std::string& driver);
bool getTerminalDeviceDriver(const std::string& device, std::string& driver);
bool scanTerminalDevices(app::TStringVector& devices, const bool extended, const bool nolock = false);


class TSerial : public util::TFileHandle {
private:
	int baud;
	int delay;
	int chars;
	std::stringstream line;
	ESerialBlockingType blocking;
	std::string device;

	bool selected;
	std::string defFileName;
	int defFileFd;
	app::TMutex mtx;

	int defaultDescriptorNeeded();
	void createDefaultDescriptor();
	void changeDefaultDescriptor(const char* event);
	void deleteDefaultDescriptor();
	ssize_t readDefaultDescriptor(void *const data, const size_t size) const;
	ssize_t writeDefaultDescriptor(void const *const data, size_t const size) const;

    TEventResult event();
	int select(int ndfs, fd_set *rfds, util::TTimePart ms);
	int sigSaveSelect(int ndfs, fd_set *rfds);
	int sigTimedSelect(int ndfs, fd_set *rfds, util::TTimePart ms);
	int sigSaveTimedSelect(int ndfs, fd_set *rfds, util::TTimePart ms);

    void resetios();
	speed_t getBaudRate(TBaudRate value);
	void finalize();
	void shutdown();

    ssize_t recv(void *const data, size_t const size) const;
    ssize_t send(void const *const data, size_t const size) const;

public:
	const std::string& getDevice() const { return device; };
	app::THandle getHandle() const { return handle(); };

	void open(const std::string& device, TBaudRate baud, ESerialBlockingType blocking = SER_DEV_DEFAULT);
	void reopen(const std::string& device, TBaudRate baud, ESerialBlockingType blocking = SER_DEV_DEFAULT);
	void close();
	void inhibit();
	void mode(int chars, int delay = 0);
	
	size_t write(void const *const data, size_t const size) const;
	void write(const std::string& s) const;
	void write(const char c) const;
	void write(std::stringstream& sstrm);
	
	size_t read(void *const data, size_t const size) const;
	void read(std::string& s) const;
	std::string read() const;

	void notify();
	void terminate();
	TEventResult wait(util::TByteBuffer& data, const size_t size, const util::TTimePart milliseconds);

	const TSerial& operator<< (const std::string& s) const;
	const TSerial& operator>> (std::string& s) const;

	// Overwrite operator<< for stream functions:
	// Handling is always the same for all calls of (*func)!!!
	// --> Write stringstream "line" to serial port and clear line stream
	inline TSerial& operator<< (std::ostream&(*func)(std::ostream&)) {
		// Add CR (LF from endl) on serial port
		line << "\r" << func;
		write(line);
		return *this;
	}

    // Overwrite stream operator<<
	// Add stream content to internal stringstream "line"
	template<typename stream_t>
		inline TSerial& operator<< (const stream_t& stream) {
			line << stream;
			return *this;
		}

	friend std::ostream& operator<< (std::ostream& os, TSerial& o);
	friend std::istream& operator>> (std::istream& is, TSerial& o);
	
	TSerial();
	~TSerial();
};


} /* namespace app */

#endif /* RS232_H_ */
