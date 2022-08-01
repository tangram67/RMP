/*
 * avahi.h
 *
 *  Created on: 10.04.2021
 *      Author: dirk
 */

#ifndef SRC_INC_AVAHI_H_
#define SRC_INC_AVAHI_H_

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/simple-watch.h>

#include <vector>
#include <string>

#include "../inc/semaphores.h"
#include "../inc/threadtypes.h"
#include "../inc/logger.h"

namespace avahi {

typedef struct CAvahiGroupEntry {
    std::string name;
    std::string type;
    std::string page;
    int port;
} TAvahiGroupEntry, *PAvahiGroupEntry;

using TGroupList = std::vector<PAvahiGroupEntry>;


class TAvahiBase {
public:
	virtual void clientCallback(AvahiClient* client, AvahiClientState state) = 0;
	virtual void groupCallback(AvahiEntryGroup* group, AvahiEntryGroupState state) = 0;

	TAvahiBase() {};
	virtual ~TAvahiBase() {};
};


class TAvahiClient : TAvahiBase {
private:
	app::TCondition condition;
	app::PThreadController threads;
	app::PLogFile logger;
	app::PManagedThread thread;
	bool running;
	bool paranoid;

	AvahiClient* client;
	AvahiEntryGroup* group;
	AvahiSimplePoll* poll;
	TGroupList groups;
	int error;
	bool debug;

	void prime();
	void clear();
	void finalize();

	void writeLog(const std::string text);

	std::string getAvahiErrorString();
	std::string getAvahiErrorString(const int error);
	std::string getAvahiErrorString(AvahiEntryGroup* group);

	void signal(void);
	void wait(void* object);

	int loop(app::TManagedThread& sender);

	void clientCallback(AvahiClient* client, AvahiClientState state);
	void groupCallback(AvahiEntryGroup* group, AvahiEntryGroupState state);

public:
	bool start();
	void terminate();
	void waitFor();

	bool isRunning() const;
	bool isTerminated() const;

	std::string getLastErrorMessage();

	void addGroupEntry(const std::string& name, const std::string& type, const std::string& page, int port);
	bool createServiceGroups();

	TAvahiClient() = delete;
	TAvahiClient(app::TThreadController& threads, app::TLogFile& logger);
	virtual ~TAvahiClient();
};

} /* namespace avahi */

#endif /* SRC_INC_AVAHI_H_ */
