/*
 * weblink.cpp
 *
 *  Created on: 07.07.2015
 *      Author: Dirk Brinkmeier
 */

#include "weblink.h"
#include "fileutils.h"
#include "mimetypes.h"
#include "compare.h"

namespace app {

TWebLink::TWebLink() {
	prime();
}

TWebLink::TWebLink(const std::string& url) {
	prime();
	setURL(url);
}

TWebLink::~TWebLink() {
}

void TWebLink::prime() {
	clear();
	requested = 0;
	bUseCache = false;
	bUseZip = false;
	setZipped(false);
	setDeferred(false);
	setReturnAll(false);
	timestamp = util::now();
}

void TWebLink::clear() {
	mime.clear();
	bIsFolder = false;
	bJSON = false;
	bXML = false;
	hash = 0;
}

void TWebLink::setURL(const std::string& url) {
	if (!url.empty()) {
		std::string ext = util::fileExt(url);
		if (!ext.empty()) {
			mime = util::getMimeType(ext);
			this->url = url;
		} else {
			mime.clear();
			this->url = url;
			bIsFolder = '/' == url[util::pred(url.size())];
			//std::cout << "TWebLink::setURL() URL = \"" << this->url << "\", folder = " << bIsFolder << std::endl;
		}
		bJSON = mime == JSON_MIME_TYPE;
		bXML = mime == XML_MIME_TYPE;
		hash = 0;
		return;
	}
	clear();
}

util::hash_type TWebLink::getHash() const {
	if (hash == 0)
		hash = util::calcHash(url);
	return hash;
}

void TWebLink::setDelay(const util::TTimePart delay) {
	data.setDelay(delay);
}

void TWebLink::setDeferred(const bool value) {
	deferred = value;
}

void TWebLink::finalize(PThreadDataItem& data) {
	this->data.finalize(data);
}

size_t TWebLink::garbageCollector() {
	return data.garbageCollector();
}

TThreadData& TWebLink::getDataHandler() {
	return data;
}

void TWebLink::setZipped(const bool value) {
	bUseZip = value;
	data.setZipped(value);
}

void TWebLink::setCached(const bool value) {
	bUseCache = value;
}

PThreadDataItem TWebLink::getData(const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers,
		bool& zipped, bool& cached, int& error, bool forceUpdate) {
	zipped = zipped && bUseZip;
	PThreadDataItem o = this->data.getData(data, size, params, session, headers, zipped, cached, error, forceUpdate);
	return o;
}

PThreadDataItem TWebLink::setData(util::TBuffer& data, const std::string& url, const util::TVariantValues& params, const util::TVariantValues& session, bool zipped, int& error) {
	PThreadDataItem o = this->data.setData(data, url, params, session, deferred, zipped, error);
	return o;
}


TWebLinkList::TWebLinkList() {
}

TWebLinkList::~TWebLinkList() {
	clear();
}

bool TWebLinkList::isRootURL(const std::string& url) {
	if (!root.empty()) {
		if (0 == util::strncasecmp(url, root, root.size()))
			return true;
	}
	return false;
}

PWebLink TWebLinkList::lookup(const std::string& url) {
	PWebLink retVal = nil;
	if (!url.empty()) {
		if (isRootURL(url)) {
			TWebLinkMap::const_iterator it = list.find(url);
			if (it != list.end()) {
				// URL is "full qualified" path or valid file name
				retVal = it->second;
			} else {
				// URL might be valid path (e.g. "images/") + any file:
				// e.g. /rest/thumbails/xxxx.yyy where the path part is a "full qualified" web link
				std::string path = util::filePath(url);
				if (!path.empty()) {
					it = list.find(path);
					if (it != list.end())
						retVal = it->second;
				}
			}
		}
	}
	return retVal;
}

PWebLink TWebLinkList::locate(const std::string& url) {
	PWebLink retVal = nil;
	if (!url.empty()) {
		if (isRootURL(url)) {
			TWebLinkMap::const_iterator it = list.find(url);
			if (it != list.end()) {
				// URL is "full qualified" path or valid file name
				retVal = it->second;
			}
		}
	}
	return retVal;
}

PWebLink TWebLinkList::find(const std::string& url) {
	app::TReadWriteGuard<app::TReadWriteLock> lock(listLck, RWL_READ);
	return lookup(url);
}

PWebLink TWebLinkList::verify(const std::string& url)
{
	app::TReadWriteGuard<app::TReadWriteLock> lock(listLck, RWL_READ);
	return locate(url);
}

void TWebLinkList::clear() {
	app::TReadWriteGuard<app::TReadWriteLock> lock(listLck, RWL_WRITE);
	if (!list.empty()) {
		PWebLink o;
		TWebLinkMap::const_iterator it = list.begin();
		while (it != list.end()) {
			o = it->second;
			util::freeAndNil(o);
			it++;
		}
		list.clear();
	}
}

void TWebLinkList::debugOutput(const std::string& preamble) {
	if (!list.empty()) {
		PWebLink o;
		TWebLinkMap::const_iterator it = list.begin();
		while (it != list.end()) {
			o = it->second;
			if (util::assigned(o)) {
				std::cout << preamble << ": \"" << o->getURL() << "\" (" << o->getURL().size() << ")" << std::endl;
			}
			it++;
		}
	}
}

size_t TWebLinkList::garbageCollector() {
	app::TReadWriteGuard<app::TReadWriteLock> lock(listLck, RWL_WRITE);
	size_t r = 0;
	if (!list.empty()) {
		PWebLink o;
		TWebLinkMap::const_iterator it = list.begin();
		while (it != list.end()) {
			o = it->second;
			if (util::assigned(o))
				r += o->garbageCollector();
			it++;
		}
	}
	return r;
}


} /* namespace app */
