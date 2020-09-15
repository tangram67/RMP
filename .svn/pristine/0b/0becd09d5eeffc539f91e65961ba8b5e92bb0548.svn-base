/*
 * weblink.h
 *
 *  Created on: 07.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef WEBLINK_H_
#define WEBLINK_H_

#include <string>
#include <vector>
#include <map>
#include "gcc.h"
#include "hash.h"
#include "threads.h"
#include "datetime.h"
#include "templates.h"
#include "exception.h"

namespace app {

class TWebLink;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PWebLink = TWebLink*;
using TWebLinkMap = std::map<std::string, app::PWebLink>;
using TWebLinkMapItem = std::pair<std::string, app::PWebLink>;

#else

typedef TWebLink* PWebLink;
typedef std::map<std::string, app::PWebLink> TWebLinkMap;
typedef std::pair<std::string, app::PWebLink> TWebLinkMapItem;

#endif


class TWebLink {
private:
	bool bXML;
	bool bJSON;
	bool bUseZip;
	bool bUseCache;
	bool bIsFolder;
	std::string url;
	std::string mime;
	mutable util::hash_type hash; // Hash is calculated on demand in const getter!
	mutable util::TTimePart timestamp;
	mutable size_t requested;
	app::TThreadData data;
	app::TMutex localMtx;
	bool returnAll;
	bool deferred;
	void prime();
	void clear();

public:
	bool isFolder() const { return bIsFolder; };
	void setZipped(const bool value);
	bool useZip() const { return bUseZip; };
	void setCached(const bool value);
	bool useCache() const { return bUseCache; };
	bool isJSON() const { return bJSON; };
	bool isXML() const { return bXML; };
	void setURL(const std::string& url);
	void setDelay(const util::TTimePart delay);
	void setDeferred(const bool value);
	size_t getRequested() const { return requested; };
	void incRequested() const { requested++; timestamp = util::now(); };
	util::TTimePart getTimeStamp() const { return timestamp; };
	std::string& getURL() { return url; };
	std::string& getMime() { return mime; };
	util::hash_type getHash() const;
	PThreadDataItem setData(util::TBuffer& data, const std::string& url,
			const util::TVariantValues& params, const util::TVariantValues& session,
			bool zipped, int& error);
	PThreadDataItem getData(const void*& data, size_t& size,
			const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers,
			bool& zipped, bool& cached, int& error, bool forceUpdate = false);
	TThreadData& getDataHandler();
	void finalize(PThreadDataItem& data);
	void setReturnAll(const bool value) { returnAll = value; };
	bool getReturnAll() const { return returnAll; };
	size_t garbageCollector();
	app::TMutex& getMutex() { return localMtx; };

	TWebLink();
	TWebLink(const std::string& url);
	virtual ~TWebLink();
};


class TWebLinkList {
private:
	TReadWriteLock listLck;
	TWebLinkMap list;
	std::string root;

	PWebLink locate(const std::string& url);
	PWebLink lookup(const std::string& url);
	void clear();

public:
	typedef TWebLinkMap::const_iterator const_iterator;

	size_t size() const { return list.size(); };
	bool empty() const { return list.empty(); };

	const_iterator begin() const { return list.begin(); };
	const_iterator end() const { return list.end(); };

	void setRoot(const std::string& path) { root = util::validPath(path); };
	bool isRootURL(const std::string& url);

	PWebLink find(const std::string& url);
	PWebLink verify(const std::string& url);

	PWebLink operator[] (const std::string& url) { return find(url); };

	void debugOutput(const std::string& preamble);
	size_t garbageCollector();

	template<typename request_t, typename class_t>
		inline void addLink(const std::string& url, request_t &&onDataRequest, class_t &&owner,
				const bool zipped = false, const bool cached = false, const util::TTimePart delay = 0) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(listLck, RWL_WRITE);
			if (!util::assigned(locate(url))) {
				PWebLink o = new TWebLink(url);
				o->setDelay(delay);
				o->setZipped(zipped);
				o->setCached(cached);
				o->getDataHandler().bindDataRequestHandler(onDataRequest, owner);
				list[o->getURL()] = o;
			} else
				throw util::app_error("TWebLink::addLink(): Duplicated link <" + url + ">");
		}

	template<typename receive_t, typename class_t>
		inline void addData(const std::string& url, receive_t &&onDataReceive, class_t &&owner,
				const bool deferred, const util::TTimePart delay = 0) {
			app::TReadWriteGuard<app::TReadWriteLock> lock(listLck, RWL_WRITE);
			if (!util::assigned(locate(url))) {
				PWebLink o = new TWebLink(url);
				o->setDelay(delay);
				o->setDeferred(deferred);
				o->getDataHandler().bindDataReceivedHandler(onDataReceive, owner);
				list[o->getURL()] = o;
			} else
				throw util::app_error("TWebLink::addLink(): Duplicated link <" + url + ">");
		}

	TWebLinkList();
	virtual ~TWebLinkList();
};



} /* namespace app */

#endif /* WEBLINK_H_ */
