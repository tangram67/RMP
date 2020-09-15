/*
 * udev.cpp
 *
 *  Created on: 25.10.2018
 *      Author: dirk
 */

#include "udev.h"
#include "exception.h"
#include "fileutils.h"
#include "random.h"

namespace app {


THotplugMap fillHotplugActions() {
	THotplugMap actions;
	actions.insert(THotplugItem("add",    HA_ADD));
	actions.insert(THotplugItem("remove", HA_REMOVE));
	actions.insert(THotplugItem("bind",   HA_BIND));
	actions.insert(THotplugItem("unbind", HA_UNBIND));
	return actions;
}

THotplugMap hotplugActionsList = fillHotplugActions();


THotplugEvent::THotplugEvent() {
	dev = nil;
	event = HA_UNKNOWN;
}

THotplugEvent::~THotplugEvent() {
	clear();
}

void THotplugEvent::assign(const THotplugMonitor& owner) {
	clear();
	dev = udev_monitor_receive_device(owner());
}

void THotplugEvent::clear() {
	if (isAssigned()) {
		udev_device_unref(dev);
		dev = nil;
		action.clear();
		name.clear();
		path.clear();
		address.clear();
		event = HA_UNKNOWN;
	}
}

THotplugEvent& THotplugEvent::operator = (const THotplugMonitor& value) {
	assign(value);
	return *this;
}

const std::string& THotplugEvent::getAction() const {
	if (action.empty()) {
		if (isAssigned()) {
			const char* p = udev_device_get_action(dev);
			if (util::assigned(p))
				action = util::tolower(std::string(p));
		}
	}
	return action;
}

const std::string& THotplugEvent::getName() const {
	if (name.empty()) {
		if (isAssigned()) {
			const char* p = udev_device_get_sysname(dev);
			if (util::assigned(p))
				name = std::string(p);
		}
	}	
	return name;
}

const std::string& THotplugEvent::getPath() const {
	if (path.empty()) {
		if (isAssigned()) {
			const char* p = udev_device_get_devpath(dev);
			if (util::assigned(p))
				path = std::string(p);
		}
	}	
	return path;
}

const std::string& THotplugEvent::getAddress() const {
	if (address.empty()) {
		if (isAssigned()) {
			const char* p = udev_device_get_sysattr_value(dev, "address");
			if (util::assigned(p))
				address = std::string(p);
		}
	}	
	return address;
}

const std::string& THotplugEvent::getProduct() const {
	if (product.empty()) {
		if (isAssigned()) {
			const char* p = udev_device_get_sysattr_value(dev, "product");
			if (util::assigned(p))
				product = std::string(p);
		}
	}
	return product;
}

const std::string& THotplugEvent::getMaker() const {
	if (maker.empty()) {
		if (isAssigned()) {
			const char* p = udev_device_get_sysattr_value(dev, "manufacturer");
			if (util::assigned(p))
				maker = std::string(p);
		}
	}
	return maker;
}

EHotplugAction THotplugEvent::getEvent() const {
	if (event == HA_UNKNOWN && !getAction().empty()) {
		THotplugMap::const_iterator it = hotplugActionsList.find(getAction());
		if (it != hotplugActionsList.end())
			event = it->second;
	}
	return event;
}



THotplugMonitor::THotplugMonitor() {
	mon = nil;
	filters = 0;
	fd = 0;
}

THotplugMonitor::~THotplugMonitor() {
	close();
}

void THotplugMonitor::open(const THotplugWatch& owner) {
	if (!isOpen()) {
		// Create device monitor
		mon = udev_monitor_new_from_netlink(owner(), "udev");
		if (!isOpen())
			throw util::app_error("TDeviceMonitor::open() failed to open new device monitor.");
	}
}

void THotplugMonitor::close() {
	if (isOpen()) {
		udev_monitor_unref(mon);
		mon = nil;
		filters = 0;
		fd = 0;
	}
}

void THotplugMonitor::enable() {
	if (isOpen()) {
		if (!isEnabled()) {
			udev_monitor_enable_receiving(mon);
			fd = udev_monitor_get_fd(mon);
			if (fd <= 0) {
				throw util::app_error("TDeviceMonitor::enable() Failed to enable monitor.");
			}
		}
	} else {
		throw util::app_error("TDeviceMonitor::enable() Device monitor must be opened.");
	}
}

void THotplugMonitor::addDeviceType(const std::string filter) {
	addDeviceType(filter.c_str());
}

void THotplugMonitor::addDeviceType(const char* filter) {
	if (isOpen()) {
		if (util::assigned(filter)) {
			udev_monitor_filter_add_match_subsystem_devtype(mon, filter, NULL);
			++filters;
		} else {
			throw util::app_error("TDeviceMonitor::addDeviceType() Empty device filter.");
		}
	} else {
		throw util::app_error("TDeviceMonitor::addDeviceType() Device monitor must be opened.");
	}
}




THotplugWatch::THotplugWatch() {
	udev = nil;
	count = 0;
	selected = false;
	defFileFd = INVALID_HANDLE_VALUE;
}

THotplugWatch::~THotplugWatch() {
	close();
}

void THotplugWatch::open(const char* filter) {
	if (!isOpen()) {
		if (util::assigned(filter)) {
			// Create udev object
			udev = udev_new();
			if (!isOpen())
				throw util::app_error("THotplug::open() failed to create new udev object.");

			// Add file descriptor to terminate select loop
			createDefaultDescriptor();

			// Add udev monitoring object
			monitor.open(*this);
			monitor.addDeviceType(filter);
			monitor.enable();

		} else {
			throw util::app_error("THotplug::open() Empty device name.");
		}
	}
}

void THotplugWatch::open(const std::string filter) {
	open(filter.c_str());
}

void THotplugWatch::close() {
	if (selected) {
		notify();
		while (selected) util::wait(30);
	}
	monitor.close();
	if (isOpen()) {
		udev_unref(udev);
		udev = nil;
	}
	deleteDefaultDescriptor();
	count = 0;
}

void THotplugWatch::createDefaultDescriptor() {
	defFileName = "/tmp/.HOTPLUG-" + util::fastCreateUUID(true, true);
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

void THotplugWatch::changeDefaultDescriptor() {
	const char* p = "HOTPLUG-FIRED\n";
	ssize_t s = strlen(p);
	if (s != writeDefaultDescriptor(p, s))
		throw util::sys_error("TFileWatch::changeDefaultDescriptor() Writing default file descriptor failed.");
}

void THotplugWatch::deleteDefaultDescriptor() {
	if (defFileFd != INVALID_HANDLE_VALUE) {
		::close(defFileFd);
		defFileFd = INVALID_HANDLE_VALUE;
		util::deleteFile(defFileName);
	}
}

ssize_t THotplugWatch::writeDefaultDescriptor(void const *const data, size_t const size) const {
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

void THotplugWatch::notify() {
	if (INVALID_HANDLE_VALUE != defFileFd) {
		changeDefaultDescriptor();
	}
}

int THotplugWatch::select(int ndfs, fd_set *rfds, util::TTimePart ms) {
	errno = EXIT_SUCCESS;
	if (ms > 0) {
		return sigSaveTimedSelect(ndfs, rfds, ms);
	} else {
		return sigSaveSelect(ndfs, rfds);
	}
	return EXIT_ERROR;
}

int THotplugWatch::sigSaveSelect(int ndfs, fd_set *rfds) {
	int r;
	do {
		errno = EXIT_SUCCESS;
		r = ::select(ndfs, rfds, NULL, NULL, NULL);
	} while (r < 0 && errno == EINTR);
	return r;
}

int THotplugWatch::sigTimedSelect(int ndfs, fd_set *rfds, util::TTimePart ms) {
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

int THotplugWatch::sigSaveTimedSelect(int ndfs, fd_set *rfds, util::TTimePart ms)
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

TEventResult THotplugWatch::wait(THotplugEvent& event, const util::TTimePart milliseconds) {
	selected = true;
	util::TBooleanGuard<bool> bg(selected);
	event.clear();

	if (!isOpen())
		throw util::app_error("THotplug::wait() Watch is closed.");
	if (!monitor.isOpen())
		throw util::app_error("THotplug::wait() Monitor not initialized.");
	if (!monitor.isEnabled())
		throw util::app_error("THotplug::wait() Monitor is not enabled.");

	// Hotplug event file descriptors
	int fd1 = defFileFd;
	int fd2 = monitor.getDescriptor();
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

	// Event file descriptor fired?
	if (r > 0) {
		if (FD_ISSET(fd1, &rfds)) {
			if (count > 0)
				return EV_TERMINATE;
			++count;
		}
		if (FD_ISSET(fd2, &rfds)) {
			// Get event from monitor
			event = monitor;
		}
		return EV_SIGNALED;
	}

	return EV_ERROR;
}


} /* namespace app */
