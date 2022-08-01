/*
 * avahi.cpp
 *
 *  Created on: 10.04.2021
 *      Author: dirk
 */

#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>
#include <avahi-common/malloc.h>

#include "avahi.h"

#include "../inc/templates.h"
#include "../inc/exception.h"
#include "../inc/threads.h"


static void avahiCallbackDispather(AvahiClient* client, AvahiClientState state, AVAHI_GCC_UNUSED void* cls) {
	if (util::assigned(client) && util::assigned(cls)) {
		(static_cast<avahi::TAvahiBase*>(cls))->clientCallback(client, state);
	}
}

static void avahiGroupCallback(AvahiEntryGroup* group, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void* cls) {
	if (util::assigned(group) && util::assigned(cls)) {
		(static_cast<avahi::TAvahiBase*>(cls))->groupCallback(group, state);
	}
}


namespace avahi {

TAvahiClient::TAvahiClient(app::TThreadController& threads, app::TLogFile& logger) {
	this->threads = &threads;
	this->logger = &logger;
	prime();
}

TAvahiClient::~TAvahiClient() {
	util::clearObjectList(groups);
}

void TAvahiClient::prime() {
	client = nil;
	group = nil;
	poll = nil;
	error = 0;
	thread = nil;
	debug = false;
	running = false;
	paranoid = false;
}

void TAvahiClient::clear() {
	finalize();
	if (util::assigned(client)) {
        avahi_client_free(client);
        client = nil;
	}
	prime();
}

void TAvahiClient::finalize() {
	if (util::assigned(poll)) {
        avahi_simple_poll_free(poll);
        poll = nil;
	}
}


std::string TAvahiClient::getAvahiErrorString(AvahiEntryGroup* group) {
	if (util::assigned(group)) {
		AvahiClient *owner = avahi_entry_group_get_client(group);
		if (util::assigned(owner)) {
			return getAvahiErrorString(avahi_client_errno(owner));
		}
		return "No Avahi client available";
	}
	return "Avahi group unassigned";
}

std::string TAvahiClient::getAvahiErrorString() {
	if (util::assigned(client)) {
		return getAvahiErrorString(avahi_client_errno(client));
	}
	return "No Avahi client available";
}

std::string TAvahiClient::getAvahiErrorString(const int error) {
	const char* s = avahi_strerror(error);
	return util::assigned(s) ? std::string(s) : "Unknown Avahi client error (" + std::to_string(error) + ")";
}

std::string TAvahiClient::getLastErrorMessage() {
	return getAvahiErrorString(error);
}


void TAvahiClient::writeLog(const std::string text) {
	if (util::assigned(logger)) {
		logger->write("[Avahi] " + text);
		if (debug) {
			std::cout << text << std::endl;
		}
	}
}


bool TAvahiClient::start() {
	if (util::assigned(client))
		return true;

	poll = avahi_simple_poll_new();
    if(!util::assigned(poll)) {
    	clear();
    	error = EXIT_FAILURE;
    	writeLog("TAvahiClient::start() Poll object could not be aquired");
        return false;
    }

	int r;
	client = avahi_client_new(avahi_simple_poll_get(poll), (AvahiClientFlags)0, avahiCallbackDispather, (void*)this, &r);
    if (!util::assigned(client)) {
        clear();
        error = r;
        writeLog("TAvahiClient::start() Client object could not be aquired: \"" + getAvahiErrorString(error) + "\"");
        return false;
    }

	if (!util::assigned(thread) && util::assigned(threads)) {
		thread = threads->addThread("Avahi-Poll",
									&avahi::TAvahiClient::loop,
									this, app::THD_START_ON_DEMAND);
		if (util::assigned(thread)) {
			if (!thread->isStarted()) {
				thread->execute();
			}
			running = thread->isStarted();
		}
	}

	writeLog("Avahi client successfully installed.");
    return true;
}

void TAvahiClient::terminate() {
	if (util::assigned(thread)) {
		thread->setTerminate(true);
	}
	if (util::assigned(poll)) {
	    avahi_simple_poll_quit(poll);
	}
}

void TAvahiClient::waitFor() {
	if (util::assigned(thread)) {
		while (!thread->isTerminated())
			util::wait(75);
		thread->wait();
	}
	finalize();
}


void TAvahiClient::signal(void) {
	if (debug) writeLog("Signaling waiters...");
	app::TLockGuard<app::TCondition> lock(condition);
	condition.signal();
}

void TAvahiClient::wait(void* object) {
	if (debug) writeLog("Waiting for condition on object assigned = " + std::string((util::assigned(object) ? "yes" : "no")));
	app::TLockGuard<app::TCondition> lock(condition);
    while(!util::assigned(object)) {
    	condition.wait();
    }
	if (debug) writeLog("Done waiting.");
}


void TAvahiClient::addGroupEntry(const std::string& name, const std::string& type, const std::string& page, int port) {
	bool ok = false;

	if (!util::assigned(client))
		throw util::app_error("Adding group entry failed, Avahi service must be running.");

	TAvahiGroupEntry* entry = new TAvahiGroupEntry;
	if (util::assigned(entry)) {
		entry->name = name;
		entry->type = type;
		entry->page = page;
		entry->port = port;
		writeLog(util::csnprintf("Add service \"%@@%:%/%\"", entry->type, entry->name, entry->port, entry->page));

		app::TLockGuard<app::TCondition> lock(condition);
	    if(util::assigned(group)) {
	        avahi_entry_group_reset(group);
	    }
		groups.push_back(entry);
		ok = true;
	}

	if (ok) {
	    createServiceGroups();
	}
}


bool TAvahiClient::createServiceGroups() {
	AvahiStringList *psl;
	int r;

	if (!util::assigned(client)) {
		writeLog("TAvahiClient::createServiceGroups() No client assigned, skipping service group creation.");
		error = EXIT_FAILURE;
		return false;
	}
    if (groups.empty()) {
		writeLog("TAvahiClient::createServiceGroups() No entries yet, skipping service group creation.");
		error = EXIT_FAILURE;
		return false;
	}

	// Create Avahi group object
	if (!util::assigned(group)) {
		group = avahi_entry_group_new(client, avahiGroupCallback, (void*)this);
		if (!util::assigned(group)) {
			error = avahi_client_errno(client);
			writeLog("TAvahiClient::createServiceGroups() Could not create Avahi groups: \"" + getAvahiErrorString(error) + "\"");
			return false;
		}
	}

    // Wait for entry group to be created
	wait(group);
	{ // Protect service list loop...
		app::TLockGuard<app::TCondition> lock(condition);
		for (auto entry : groups) {
			psl = nil;
			psl = avahi_string_list_add(psl,"cn=db::solutions");
			if (entry->page.size() > 2) {
				std::string url = "path=/" + entry->page;
				psl = avahi_string_list_add(psl, url.c_str());
			}

			r = avahi_entry_group_add_service_strlst(
					group,
					AVAHI_IF_UNSPEC,
					AVAHI_PROTO_UNSPEC,
					(AvahiPublishFlags)0,
					avahi_strdup(entry->name.c_str()),
					avahi_strdup(entry->type.c_str()),
					nil,
					nil,
					entry->port,
					psl);

			if (r < 0) {
				error = avahi_client_errno(client);
				writeLog("TAvahiClient::createServiceGroups() Could not add Avahi services: \"" + getAvahiErrorString(error) + "\"");
				avahi_string_list_free(psl);
				return false;
			} else {
				writeLog(util::csnprintf("Registered service \"%@@%:%/%\"", entry->type, entry->name, entry->port, entry->page));
			}

			avahi_string_list_free(psl);
		}
	}

	// Commit service groups
	r = avahi_entry_group_commit(group);
    if (r < 0) {
		error = avahi_client_errno(client);
		writeLog("TAvahiClient::createServiceGroups() Could not commit Avahi services: \"" + getAvahiErrorString(error) + "\"");
        return false;
    }

    return true;
}


void TAvahiClient::clientCallback(AvahiClient* client, AvahiClientState state) {
	switch(state) {
		case AVAHI_CLIENT_S_RUNNING:
			writeLog("Client is running.");
			if(!util::assigned(group) && util::assigned(this->client)) {
				createServiceGroups();
			}
			break;
		case AVAHI_CLIENT_S_COLLISION:
			writeLog("Client collision.");
			if(util::assigned(group)) {
				avahi_entry_group_reset(group);
			}
			break;
		case AVAHI_CLIENT_FAILURE:
			writeLog("Client failure.");
			if (paranoid)
				avahi_simple_poll_quit(poll);
			break;
		case AVAHI_CLIENT_S_REGISTERING:
			writeLog("Client registering.");
			if(util::assigned(group)) {
				avahi_entry_group_reset(group);
			}
			break;
		case AVAHI_CLIENT_CONNECTING:
			writeLog("Client connecting.");
			break;
	}
}

void TAvahiClient::groupCallback(AvahiEntryGroup* group, AvahiEntryGroupState state) {
	/* Called whenever the entry group state changes */
	switch (state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED :
			/* The entry group has been established successfully */
			writeLog("Successfully added Avahi service group.");
			signal();
			break;
		case AVAHI_ENTRY_GROUP_COLLISION :
			/* A service name collision with a remote service happened */
			writeLog("Service name collision for Avahi service group.");
			break;
		case AVAHI_ENTRY_GROUP_FAILURE :
			/* Some kind of failure happened while we were registering our services */
			writeLog("Entry group name failure \"" + getAvahiErrorString(group) + "\"");
			if (paranoid)
				avahi_simple_poll_quit(poll);
			break;
		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:
			break;
	}
}


int TAvahiClient::loop(app::TManagedThread& sender) {
	running = true;
	int r;
	writeLog("Avahi poll thread started.");

	// Be sure to reset running flag...
	util::TBooleanGuard<bool> bg(running);
    do {
    	r = avahi_simple_poll_iterate(poll, -1);
    	if (r == 0 && !sender.isTerminated()) {
    		util::wait(150);
    	}
    } while (r == 0);
    if(r < 0) {
    	writeLog("Avahi poll thread terminated on error \"" + getAvahiErrorString() + "\"");
    } else {
    	writeLog("Avahi poll thread terminated.");
    }

    return EXIT_SUCCESS;
}


bool TAvahiClient::isTerminated() const {
	return !running && thread->isTerminated();
}

bool TAvahiClient::isRunning() const {
    return util::assigned(poll);
}

} /* namespace avahi */
