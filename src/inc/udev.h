/*
 * udev.h
 *
 *  Created on: 25.10.2018
 *      Author: dirk
 */
#ifndef INC_UDEV_H_
#define INC_UDEV_H_

#include <libudev.h>
#include "ipctypes.h"
#include "templates.h"
#include "datetime.h"
#include "ipc.h"

namespace app {

enum EHotplugAction {
	HA_ADD,
	HA_BIND,
	HA_REMOVE,
	HA_UNBIND,
	HA_UNKNOWN
};


#ifdef STL_HAS_TEMPLATE_ALIAS

using THotplugMap = std::map<std::string, EHotplugAction>;
using THotplugItem = std::pair<std::string, EHotplugAction>;

#else

typedef std::map<std::string, EHotplugAction> THotplugMap;
typedef std::pair<std::string, EHotplugAction> THotplugItem;

#endif


class THotplugWatch;
class THotplugMonitor;

class THotplugEvent {
private:
	struct udev_device *dev;

	mutable std::string action;
	mutable std::string name;
	mutable std::string path;
	mutable std::string address;
	mutable std::string product;
	mutable std::string maker;
	mutable EHotplugAction event;

public:
	void assign(const THotplugMonitor& owner);
	bool isAssigned() const { return util::assigned(dev); };
	void clear();

	struct udev_device * getObject() const { return dev; };
	struct udev_device * operator () () const { return getObject(); };

	EHotplugAction getEvent() const;
	const std::string& getAction() const;
	const std::string& getName() const;
	const std::string& getPath() const;
	const std::string& getAddress() const;
	const std::string& getProduct() const;
	const std::string& getMaker() const;

	THotplugEvent& operator = (const THotplugMonitor& value);

	THotplugEvent();
	virtual ~THotplugEvent();
};

class THotplugMonitor {
private:
	struct udev_monitor *mon;
	size_t filters;
	int fd;

public:
	void open(const THotplugWatch& owner);
	void close();
	void enable();

	bool isOpen() const { return util::assigned(mon); };
	bool isEnabled() const { return fd > 0; };

	struct udev_monitor * getObject() const { return mon; };
	struct udev_monitor * operator () () const { return getObject(); };
	int getDescriptor() const { return fd; };

	void addDeviceType(const std::string filter);
	void addDeviceType(const char * filter);

	THotplugMonitor();
	virtual ~THotplugMonitor();
};

class THotplugWatch {
private:
	struct udev *udev;
	THotplugMonitor monitor;
	std::string defFileName;
	bool selected;
	size_t count;
	int defFileFd;

	int select(int ndfs, fd_set *rfds, util::TTimePart ms);
	int sigSaveSelect(int ndfs, fd_set *rfds);
	int sigTimedSelect(int ndfs, fd_set *rfds, util::TTimePart ms);
	int sigSaveTimedSelect(int ndfs, fd_set *rfds, util::TTimePart ms);

	void createDefaultDescriptor();
	void changeDefaultDescriptor();
	void deleteDefaultDescriptor();
	ssize_t writeDefaultDescriptor(void const *const data, size_t const size) const;


public:
	void open(const char* filter);
	void open(const std::string filter);
	void close();

	bool isOpen() const { return util::assigned(udev); };
	struct udev * getObject() const { return udev; };
	struct udev * operator () () const { return getObject(); };

	TEventResult wait(THotplugEvent& event, const util::TTimePart milliseconds);
	void notify();

	THotplugWatch();
	virtual ~THotplugWatch();
};

} /* namespace app */

#endif /* INC_UDEV_H_ */
