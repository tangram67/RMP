/*
 * sound.h
 *
 *  Created on: 18.02.2018
 *      Author: Dirk Brinkmeier
 */

#ifndef SOUND_H_
#define SOUND_H_

#include "../inc/gcc.h"
#include "../inc/semaphores.h"
#include "../inc/classes.h"
#include "../inc/memory.h"
#include "../inc/rs232.h"
#include "../inc/alsa.h"

namespace app {

#ifdef STL_HAS_TEMPLATE_ALIAS

using TCommandData = uint8_t;

#else

typedef uint8_t TCommandData;

#endif


enum EDeviceType {
	EDS_CLOCK = 1,
	EDS_DAC = 9,
	EDS_SAMPLER = 5,
	EDS_TRANSPORT = 10
};

enum ERemoteCommand {
	RS_STATUS = 76,  // 'L'
	RS_INPUT  = 113, // 'q'
	RS_FILTER = 33,  // '!'
	RS_EMPH   = 34,  // '"'
	RS_PHASE  = 112  // 'p'
};

enum EResponseType {
	ERT_ACKNOWLEDE,
	ERT_DATA,
	ERT_NONE
};

typedef struct CRemoteCommand {
	TCommandData id;
	TCommandData command;
	TCommandData size;
	TCommandData value;
	TCommandData modulo;
} TRemoteCommand;

typedef struct CRemoteParser {
	EResponseType type;
	TCommandData bytes[5];
	TCommandData modulo;
	size_t length;
	size_t index;
	bool acked;
	bool busy;
	int state;

	void clear() {
		state = 0;
		modulo = 0;
		length = 0;
		index = 0;
		acked = false;
		busy = false;
		bytes[0] = 0;
		bytes[1] = 0;
		bytes[2] = 0;
		bytes[3] = 0;
		bytes[4] = 0;
		type = ERT_NONE;
	}
	CRemoteParser() {
		clear();
	}

} TRemoteParser;

typedef struct CRemoteParameter {
	uint8_t phase;
	uint8_t input;
	uint8_t clock;
	uint8_t filter;

	void clear() {
		phase = 0;
		input = 0;
		clock = 0;
		filter = 0;
	}
	CRemoteParameter() {
		clear();
	}

} TRemoteParameter;


class TRemoteAlsaDevice {
private:
	music::TAlsaPlayer output;
	music::ESampleRate m_rate;
	std::string m_device;

	void prime();
	void clear();

public:
	bool isOpen() const { return output.isOpen(); };
	const std::string& getDevice() const { return m_device; };
	music::ESampleRate getRate() const { return m_rate; };

	int error() const { return output.error(); };
	std::string syserr() const { return output.syserr(); };
	std::string strerr() const { return output.strerr(); };

	bool open(const std::string& device, const music::ESampleRate rate, const bool canonical, bool& opened, bool& unchanged);
	void close();

	TRemoteAlsaDevice();
	virtual ~TRemoteAlsaDevice();
};


class TRemoteSerialDevice {
private:
	app::TSerial port;
	std::string m_device;
	std::string error;
	TRemoteParser parser;
	mutable app::TMutex mtx;

	void clear();
	void restart(const EResponseType type);
	void invalidate();

public:
	bool isOpen() const;
	bool isBusy() const;
	bool isWaiting() const;
	bool isAcknowledged() const;

	const std::string& getDevice() const { return m_device; };
	std::string strerr() const { return error; };

	bool send(const EDeviceType id, const ERemoteCommand command, const TCommandData value, const EResponseType type);
	int parse(util::TByteBuffer& data, TRemoteParameter& parameter);
	TEventResult wait(util::TByteBuffer& data, const size_t size);
	void activate(const EResponseType type);
	void cancel();
	void notify();

	bool open(const std::string& device, const app::TBaudRate rate);
	bool reopen(const std::string& device, const app::TBaudRate rate);
	void inhibit();
	void close();

	TRemoteSerialDevice();
	virtual ~TRemoteSerialDevice();
};


} /* namespace app */

#endif /* SOUND_H_ */
