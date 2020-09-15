/*
 * sound.cpp
 *
 *  Created on: 18.02.2018
 *      Author: Dirk Brinkmeier
 */

#include "sound.h"

namespace app {

TRemoteAlsaDevice::TRemoteAlsaDevice() {
	prime();
}

TRemoteAlsaDevice::~TRemoteAlsaDevice() {
	close();
}

void TRemoteAlsaDevice::prime() {
	m_rate = music::SR0K;
}

void TRemoteAlsaDevice::clear() {
	m_device.clear();
	prime();
}

bool TRemoteAlsaDevice::open(const std::string& device, const music::ESampleRate rate, const bool canonical, bool& opened, bool& unchanged) {
	opened = false;
	unchanged = false;
	music::ESampleRate requested = rate;
	if (canonical) {
		requested = (rate % music::SR48K) == 0 ? music::SR48K : music::SR44K;
	}
	bool open = output.isOpen();
	if (device != m_device || requested != m_rate) {
		if (open) {
			output.close();
		}
		music::CStreamData stream;
		stream.setFixedRate(requested);
		if (output.open(device, stream)) {
			m_device = device;
			m_rate = requested;
			opened = true;
			open = true;
		} else {
			clear();
			open = false;
		}
	} else {
		unchanged = true;
	}
	return open;
}

void TRemoteAlsaDevice::close() {
	output.close();
}



TRemoteSerialDevice::TRemoteSerialDevice() {
	invalidate();
}

TRemoteSerialDevice::~TRemoteSerialDevice() {
	close();
}

bool TRemoteSerialDevice::open(const std::string& device, const app::TBaudRate rate) {
	if (!isOpen()) {
		try {
			port.open(device, rate);
			if (isOpen()) {
				m_device = port.getDevice();
			}
		} catch (const std::exception& e)	{
			error = e.what();
		} catch (...)	{
			error = "Unknown exception on open port";
		}
	}
	return isOpen();
}

bool TRemoteSerialDevice::reopen(const std::string& device, const app::TBaudRate rate) {
	try {
		port.reopen(device, rate);
		if (isOpen()) {
			m_device = port.getDevice();
		}
	} catch (const std::exception& e)	{
		error = e.what();
	} catch (...)	{
		error = "Unknown exception on reopen port";
	}
	return isOpen();
}

void TRemoteSerialDevice::inhibit() {
	port.inhibit();
}

void TRemoteSerialDevice::close() {
	port.close();
}

bool TRemoteSerialDevice::isOpen() const {
	return port.isOpen();
};

bool TRemoteSerialDevice::isBusy() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return parser.busy;
}

bool TRemoteSerialDevice::isWaiting() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	if (parser.busy && !parser.acked) {
		return true;
	}
	return false;
}

bool TRemoteSerialDevice::isAcknowledged() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return parser.acked;
}


void TRemoteSerialDevice::notify() {
	port.notify();
}

void TRemoteSerialDevice::invalidate() {
	parser.clear();
}

void TRemoteSerialDevice::restart(const EResponseType type) {
	invalidate();
	parser.type = type;
	parser.busy = true;
	parser.state = 2;
}

void TRemoteSerialDevice::activate(const EResponseType type) {
	app::TLockGuard<app::TMutex> lock(mtx);
	restart(type);
}

void TRemoteSerialDevice::cancel() {
	app::TLockGuard<app::TMutex> lock(mtx);
	invalidate();
}

TEventResult TRemoteSerialDevice::wait(util::TByteBuffer& data, const size_t size) {
	return port.wait(data, size, 0);
}


bool TRemoteSerialDevice::send(const EDeviceType id, const ERemoteCommand command, const TCommandData value, const EResponseType type) {
	if (isOpen()) {
		app::TLockGuard<app::TMutex> lock(mtx);
		TRemoteCommand csr;
		size_t size = sizeof(csr);

		// Create remote command
		csr.id = (TCommandData)(id & 0xFF);
		csr.command = (TCommandData)(command & 0xFF);
		csr.size = (TCommandData)1;
		csr.value = value;
		csr.modulo = value;

		// Send remote command
		try {
			size_t r = port.write(&csr, size);
			if (r == size) {
				restart(type);
				return true;
			}
			error = util::csnprintf("Send command failed (%)", r);
		} catch (const std::exception& e)	{
			error = e.what();
		} catch (...)	{
			error = "Unknown error.";
		}
	} else {
		error = "Device not open.";
	}
	return false;
}

int TRemoteSerialDevice::parse(util::TByteBuffer& data, TRemoteParameter& parameter) {
	app::TLockGuard<app::TMutex> lock(mtx);
	error = "Unknown error";
	bool debug = false;
	int result = 0;
	if (parser.state > 0) {
		if (!data.empty()) {
			if (debug) std::cout << "TRemoteSerialDevice::parse() Bytes " << util::TBinaryConvert::binToHexA(data(), data.size(), false) << std::endl;

			for (size_t i=0; i<data.size(); ++i) {
				TCommandData value = data[i];
				bool exit;
				bool leave = false;
				do {
					exit = true;
					if (debug) std::cout << "TRemoteSerialDevice::parse() State = " << parser.state << std::endl;
					switch (parser.state) {
						case 2:
							// Wait for acknowledge
							if ((TCommandData)0xAAu == value) {
								parser.acked = true;
								if (ERT_ACKNOWLEDE == parser.type) {
									// Ignore further data for simple acknowledged request
									if (debug) std::cout << "TRemoteSerialDevice::parse() Acknowledge found." << std::endl;
									parser.busy = false;
									parser.state = 0;
									leave = true;
									result = 0;
								}
							} else {
								parser.state = 4;
								exit = false;
							}
							break;
						case 4:
							// Read status header
							if ((TCommandData)0x0Au == value) {
								parser.state = 6;
							} else {
								error = util::csnprintf("Invalid header byte (@:0x0a)", (int)value);
								result = -1;
							}
							break;
						case 6:
							// Read command byte
							if ((TCommandData)0x4Cu == value) {
								parser.state = 8;
							} else {
								error = util::csnprintf("Invalid command byte (@:0x4c)", (int)value);
								result = -2;
							}
							break;
						case 8:
							// Read length, at least 5 bytes...
							parser.length = value;
							if (debug) std::cout << "TRemoteSerialDevice::parse() Length = " << parser.length << std::endl;
							if (parser.length >= 5) {
								parser.index = 0;
								parser.state = 10;
							} else {
								error = util::csnprintf("Response too short (%:5)", parser.length);
								result = -3;
							}
							break;
						case 10:
							// Read payload bytes
							if (parser.length > 0) {
								if (parser.index < 5) {
									parser.bytes[parser.index] = value;
								}
								++parser.index;
								--parser.length;
								parser.modulo += value;
							} else {
								parser.state = 12;
								exit = false;
							}
							break;
						case 12:
							// Read checksum
							if (debug) std::cout << "TRemoteSerialDevice::parse() CHS = " << (int)parser.modulo << ":" << (int)value << std::endl;
							if (parser.modulo == value) {
								parameter.phase = parser.bytes[0];
								parameter.input = parser.bytes[1];
								parameter.clock = parser.bytes[2];
								parameter.filter = parser.bytes[4];
								parser.busy = false;
								parser.state = 0;
								leave = true;
								result = 1;
							} else {
								error = util::csnprintf("Invalid checksum (@:@)", (int)parser.modulo, (int)value);
								result = -4;
							}
							break;
						default:
							// Invalid state...
							error = util::csnprintf("Invalid state (%)", parser.state);
							result = -5;
							break;
					} // switch (state)
				} while (!exit);

				// Escape on error
				if (result < 0) {
					if (debug) std::cout << "TRemoteSerialDevice::parse() Parser failed: " << error << " [" << result << "]" << std::endl;
					invalidate();
					break;
				}

				// Exit on success
				if (result > 0 || leave) {
					break;
				}

			} // for ...
		} // if (!data.empty())
	} // if (state > 0)
	return result;
}


} /* namespace app */
