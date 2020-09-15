/*
 * streamlist.cpp
 *
 *  Created on: 01.12.2019
 *      Author: dirk
 */

#include "streamlist.h"
#include "streamconsts.h"
#include "../inc/templates.h"
#include "../inc/fileutils.h"
#include "../inc/htmlutils.h"
#include "../inc/encoding.h"
#include "../inc/compare.h"
#include "../inc/convert.h"
#include "../inc/json.h"
#include "../inc/ssl.h"

namespace radio {

TStations::TStations() {
	changed = false;
	limit = std::string::npos;
	offset = std::string::npos;
}

TStations::~TStations() {
	clear();
}

void TStations::clear() {
	map.clear();
	util::clearObjectList(stations);
	util::clearObjectList(history);
	invalidate();
}


std::string TStations::encode(const std::string& text, const bool encoded) {
	return encoded ? util::TURL::encode(text, util::TURL::URL_EXTENDED, util::TURL::UST_REPLACED) : text;
}

std::string TStations::decode(const std::string& text, const bool encoded) {
	return encoded ? util::TURL::decode(util::unquote(text)) : util::unquote(text);
}


std::string TStations::orderModeToStr(const EMetadataOrder mode) {
	switch (mode) {
		case EMO_ARTIST_TITLE:
			return "Artist - Track";
			break;
		case EMO_TITLE_ARTIST:
			return "Track - Artist";
			break;
		case EMO_DISABLED:
			return "Disabled";
			break;
		default:
			break;
	}
	return "Unknown";
}

EMetadataOrder TStations::strToOrderMode(const std::string& mode) {
	if (!mode.empty()) {
		char c = tolower(mode[0]);
		switch (c) {
			case 'a':
				return EMO_ARTIST_TITLE;
				break;
			case 't':
				return EMO_TITLE_ARTIST;
				break;
			case 'd':
				return EMO_DISABLED;
				break;
			default:
				break;
		}
	}
	return EMO_DEFAULT;
}


bool TStations::assign(PRadioStream stream, const std::string& text, const char delimiter) {
	if (text.empty())
		return false;

	if (!util::assigned(stream))
		return false;

	util::TStringList csv;
	csv.split(text, delimiter);
	if (csv.size() < 3)
		return false;

	// Read params
	EMetadataOrder order = EMO_UNKNOWN;
	bool encoded = true;
	for (size_t i=0; i<csv.size(); ++i) {
		switch (i) {
			case 0:
				// Ignore hash value
				break;
			case 1:
				stream->values.name = decode(csv[i], encoded);
				break;
			case 2:
				stream->values.url = decode(csv[i], encoded);
				break;
			case 7:
				order = (EMetadataOrder)util::strToInt(csv[i], EMO_UNKNOWN);
				break;
			default:
				break;
		}
	}

	// Set metadata order mode
	if (util::isMemberOf(order, EMO_ARTIST_TITLE,EMO_TITLE_ARTIST)) {
		stream->values.order = order;
	} else {
		stream->values.order = EMO_DEFAULT;
	}

	// Check for valid entry
	if (!stream->values.name.empty() && !stream->values.url.empty()) {
		return true;
	}

	// Invalid content read from file
	csv.debugOutput();
	return false;
}

bool TStations::update(PRadioStream stream) {
	if (util::assigned(stream)) {
		if (!stream->values.name.empty() && !stream->values.url.empty()) {

			// Set unique hash value
			util::TDigest MD5(util::EDT_MD5);
			stream->hash = MD5(stream->values.name + "/" + stream->values.url);

			// Set URL encoded properties
			stream->encoded.name = encode(stream->values.name, true);
			stream->encoded.url = encode(stream->values.url, true);

			// Set HTML encoded properties
			stream->display.name = html::THTML::encode(stream->values.name);
			stream->display.url = html::THTML::applyFlowControl(html::THTML::encode(stream->values.url));

			// Set metadata order properties
			stream->values.mode = orderModeToStr(stream->values.order);
			stream->encoded.mode = encode(stream->values.mode, true);
			stream->display.mode = html::THTML::encode(stream->values.mode);

			stream->encoded.order = stream->values.order;
			stream->display.order = stream->values.order;

			return true;
		}
	}
	return false;
}

void TStations::updateMappings() {
	map.clear();
	if (!empty()) {
		for (size_t i=0; i<size(); ++i) {
			PRadioStream o = stations[i];
			if (util::assigned(o)) {
				if (!o->hash.empty()) {
					map[o->hash] = o;
				}
			}
		}
	}
}

void TStations::reindex() {
	if (!empty()) {
		for (size_t i=0; i<size(); ++i) {
			PRadioStream o = stations[i];
			if (util::assigned(o)) {
				o->index = i;
				o->deleted = false;
			}
		}
	}
}


void TStations::invalidate() {
	if (!exported.empty())
		exported.clear();
	limit = std::string::npos;
	offset = std::string::npos;
	changed = true;
}


struct CStreamDeleter {
	CStreamDeleter() {}
    bool operator()(PRadioStream o) const {
    	if (util::assigned(o)) {
			if (o->deleted) {
				util::freeAndNil(o);
				return true;
			}
    	}
    	return false;
    }
};

void TStations::removeDeleted() {
	stations.erase(std::remove_if(stations.begin(), stations.end(), CStreamDeleter()), stations.end());
}


size_t TStations::loadDefaultStations() {
	if (!empty())
		clear();

	PRadioStream o = nil;
	const TRadioStreamNames *it;
	for (it = radiostreams; util::assigned(it->name); ++it) {
		o = new TRadioStream;
		o->values.name = it->name;
		o->values.url = it->url;
		util::TObjectGuard<TRadioStream> og(&o);
		if (update(o)) {
			stations.push_back(o);
			o = nil;
		}
	}

	if (!stations.empty()) {
		updateMappings();
		invalidate();
		reindex();
	}

	return isInvalidated();
}


size_t TStations::loadFromFile(const std::string& fileName, const bool hasHeader, const char delimiter) {
	setDatabase(fileName);

	if (!empty())
		clear();

	if(!util::fileExists(fileName))
		return 0;

	util::TStringList list;
	list.loadFromFile(fileName, app::ECodepage::CP_DEFAULT);

	if(list.empty())
		return 0;

	// Only header row present
	if (list.size() <= 1 && hasHeader)
		return 0;

	// Ignore header?
	size_t start = hasHeader ? 1 : 0;

	// Load data from rows
	size_t idx;
	std::string row;
	PRadioStream o = nil;
	for (idx=start; idx<list.size(); idx++) {
		row = list[idx];
		o = new TRadioStream;
		util::TObjectGuard<TRadioStream> og(&o);
		if (assign(o, row, delimiter)) {
			if (update(o)) {
				stations.push_back(o);
				o = nil;
			}
		}
	}

	if (stations.size() > 0) {
		updateMappings();
		reindex();
	}

	return stations.size();
}


void TStations::saveToFile(const bool addHeader, const char delimiter) {
	if (!database.empty())
		saveToFile(database, addHeader, delimiter);
}

void TStations::saveToFile(const std::string& fileName, const bool addHeader, const char delimiter) {
	std::lock_guard<std::mutex> lock(saveMtx);
	util::deleteFile(fileName);

	if (!empty()) {
		std::string d(&delimiter, 1);

		util::TStringList list;
		if (addHeader)
			list.add("Hash" + d + "Name" + d + "URL" + d + "EncodedName" + d + "EncodedURL" + d + "DisplayName" + d + "DisplayURL");

		std::string s;
		PRadioStream o;
		bool encoded = true;
		for (size_t i=0; i<size(); ++i) {
			o = stations[i];
			if (util::assigned(o)) {
				size_t len = 2 * (o->hash.size() + o->values.name.size() + o->values.url.size() + o->display.name.size() +
								  o->display.url.size() + o->encoded.name.size() + o->encoded.url.size());
				s.clear();
				s.reserve(len);
				s += util::quote(encode(o->hash, encoded)).append(d);	      // Index 0
				s += util::quote(encode(o->values.name, encoded)).append(d);  // Index 1
				s += util::quote(encode(o->values.url, encoded)).append(d);	  // Index 2
				s += util::quote(o->encoded.name).append(d);                  // Index 3
				s += util::quote(o->encoded.url).append(d);                   // Index 4
				s += util::quote(o->display.name).append(d);                  // Index 5
				s += util::quote(o->display.url).append(d);                   // Index 6
				s += std::to_string(o->values.order).append(d);               // Index 7
				list.add(s);
			}
		}

		list.saveToFile(fileName);
	}

	changed = false;
}

std::string TStations::asJSON(const size_t index, const size_t count, const std::string& filter, const std::string& active) {

	// Support for bootstrap-table server side pagination
	// Example:
	//   {
	//     "total": 2,
	//     "rows": [
	//       {
	//         "Index": 0,
	//         "Row": "...."
	//       },
	//       {
	//         "Index": 1,
	//         "Row": "...."
	//       }
	//     ]
	//   }

	// Check for filter to apply
	TStationList list;
	bool filtered = !filter.empty();
	for (size_t i=0; i< size(); ++i) {
		PRadioStream o = stations[i];
		if (filtered) {
			bool found = false;
			if (!found) {
				if (util::strcasestr(o->values.name, filter))
					found = true;
			}
			if (!found) {
				if (util::strcasestr(o->values.url, filter))
					found = true;
			}
			if (found) {
				list.push_back(o);
			}
		} else {
			list.push_back(o);
		}
	}

	// Filtered list size
	size_t size = list.size();

	// Is limit valid?
	if (list.empty() || index >= size)
		return util::JSON_EMPTY_TABLE;


	// Begin new JSON object
	util::TStringList json;
	json.add("{");

	// Begin new JSON array
	json.add("\"total\": " + std::to_string((size_u)size) + ",");
	json.add("\"rows\": [");

	// Calculate limits
	size_t idx = index;
	size_t last = 0;
	size_t end = (count == 0 || count == std::string::npos) ? size : idx + count;

	// Check ranges...
	if (end > size)
		end = size;
	if (end > 1)
		last = util::pred(end);

	// Write JSON entries
	for(; idx < end; ++idx) {
		const PRadioStream& stream = list[idx];
		if (util::assigned(stream)) {
			bool selected = stream->hash == active;
			std::string activated = selected ? "true" : "false";
			json.add("{");
			json.add("  \"Index\": " + std::to_string((size_u)stream->index) + ",");
			json.add("  \"Active\": " + activated + ",");
			json.add("  \"Hash\": \"" + stream->hash + "\",");
			json.add("  \"Mode\": \"" + stream->values.mode + "\",");
			if (selected) {
				json.add("  \"Name\": \"<strong>" + util::TJsonValue::escape(stream->display.name) + "</strong>\",");
				json.add("  \"URL\": \"<strong>" + util::TJsonValue::escape(stream->display.url) + "</strong>\",");
			} else {
				json.add("  \"Name\": \"" + util::TJsonValue::escape(stream->display.name) + "\",");
				json.add("  \"URL\": \"" + util::TJsonValue::escape(stream->display.url) + "\",");
			}
			json.add("  \"Originalname\": \"" + util::TJsonValue::escape(stream->display.name) + "\",");
			json.add("  \"OriginalURL\": \"" + util::TJsonValue::escape(stream->display.url) + "\"");
			json.add(idx < last ? "}," : "}");
		}
	}

	// Close JSON array and object
	json.add("]}");

	return json.text();
}

const util::TStringList& TStations::asM3U(size_t limit, size_t offset) {

	// Is limit valid?
	if (limit < 0)
		return deflist;

	// Is offset valid?
	if (!validIndex(offset))
		return deflist;

	// Same values as requested before=
	if (this->limit == limit && this->offset == offset)
		return exported;

	// Store current values
	this->limit = limit;
	this->offset = offset;

	// If limit = 0 --> return all entries!
	if (limit == 0)
		limit = size();

	// Range check and adjustment
	if ((limit + offset) > size())
		limit = size() - offset;

	// Begin extended M3U playlist
	exported.add("#EXTM3U");
	exported.add("");
	if (!empty()) {
		// Add M3U entries
		PRadioStream stream;
		const_iterator it = begin() + offset;
		for(size_t cnt = 0; it != end() && cnt < limit; ++it, ++cnt) {
			stream = *it;
			if (util::assigned(stream)) {
				// Add stream name and URL like:
				// #EXTINF:0,Radio Bob
				// http://streams.radiobob.de/bob-live/mp3-192/mediaplayer
				exported.add("#EXTINF:0," + stream->values.name);
				exported.add(stream->values.url);
				exported.add("");
			}
		}
		exported.add("");
	}

	return exported;
}

bool TStations::validIndex(const size_t index) const {
	return (index >= 0 && index < stations.size());
}

PRadioStream TStations::find(const std::string hash) const {
	TStationMap::const_iterator it = map.find(hash);
	if (it != map.end()) {
		return it->second;
	}
	return nil;
}

bool TStations::edit(const std::string hash, const std::string name, const std::string url, const EMetadataOrder order, const EEditMode mode) {
	PRadioStream stream = find(hash);
	if (util::assigned(stream)) {
		if (ESE_TRANSACT == mode) beginTransaction(stream);
		stream->values.name = name;
		stream->values.url = url;
		stream->values.order = order;
		if (update(stream)) {
			updateMappings();
			invalidate();
			reindex();
			if (ESE_TRANSACT == mode) commitTransaction(stream);
			return true;
		}
	}
	return false;
}

bool TStations::add(const std::string name, const std::string url, const EMetadataOrder order, const EEditMode mode) {
	if (!name.empty() && !url.empty()) {
		PRadioStream stream = new TRadioStream;
		stream->values.name = name;
		stream->values.url = url;
		stream->values.order = order;
		if (ESE_TRANSACT == mode) beginTransaction(nil);

		// Add new radio stream
		util::TObjectGuard<TRadioStream> og(&stream);
		if (update(stream)) {
			stations.push_back(stream);
			if (ESE_TRANSACT == mode) commitTransaction(stream);
			stream = nil;

			// Update mappings
			updateMappings();
			invalidate();
			reindex();
			return true;
		}

	}
	return false;
}

bool TStations::insert(const std::string name, const std::string url, const EMetadataOrder order, const size_t position, const EEditMode mode) {
	if (!name.empty() && !url.empty()) {
		PRadioStream stream = new TRadioStream;
		stream->values.name = name;
		stream->values.url = url;
		stream->values.order = order;
		if (ESE_TRANSACT == mode) beginTransaction(nil);

		// Add new radio stream
		util::TObjectGuard<TRadioStream> og(&stream);
		if (update(stream)) {

			size_t index = 0;
			bool inserted = false;
			for (auto it = stations.begin(); it != stations.end(); ++it, ++index) {
				if (index == position) {
					stations.insert(it, stream);
					stream = nil;
					inserted = true;
				}
			}

			if (!inserted) {
				stations.push_back(stream);
				stream = nil;
				inserted = true;
			}

			// Update mappings
			if (inserted) {
				updateMappings();
				invalidate();
				reindex();
				if (ESE_TRANSACT == mode) commitTransaction(stream);
				return true;
			}
		}

	}
	return false;
}

bool TStations::remove(const std::string hash, const EEditMode mode) {
	if (!hash.empty()) {
		PRadioStream stream = find(hash);
		if (util::assigned(stream)) {
			if (ESE_TRANSACT == mode) beginTransaction(stream);
			stream->deleted = true;
			removeDeleted();
			updateMappings();
			invalidate();
			reindex();
			if (ESE_TRANSACT == mode) commitTransaction(nil);
			return true;
		}
	}
	return false;
}

int TStations::reorder(const data::TTable& table) {
	int r = 0;
	if (!table.empty()) {

		// Set indices before reorganize station list
		reindex();

		// Get all tracks to reorganize
		size_t index = 1 + size();
		TStationList list;

		for (size_t i=0; i<table.size(); ++i) {
			std::string hash = table[i]["Hash"].asString();
			PRadioStream stream = find(hash);
			if (util::assigned(stream)) {
				// Find lowest/start index
				if (stream->index < index)
					index = stream->index;
				list.push_back(stream);
			}
		}

		// Reorder stations by list content
		if ((table.size() == list.size()) && (size() >= list.size()) && ((index + list.size()) <= size())) {
			for (size_t i=0; i<list.size(); ++i) {
				PRadioStream stream = list[i];
				if (util::assigned(stream)) {
					size_t idx = index + i;
					if (validIndex(idx)) {
						stations[idx] = stream;
						stream->index = idx;
						++r;
					}
				}
			}
		}

	}

	if (!isInvalidated() && r > 0) {
		invalidate();
	}

	return r;
}

void TStations::beginTransaction(const TRadioStream* value) {
	updated.clear();
	updated.setPrevious(value);
}

void TStations::commitTransaction(const TRadioStream* value) {
	updated.setCurrent(value);
	PRadioStreamHistory entry = new TRadioStreamHistory;
	if (util::assigned(entry)) {
		*entry = updated;
		history.insert(history.begin(), entry);
		shrinkHistory(5);
	}
}

void TStations::clearUndoList() {
	shrinkHistory(0);
}

void TStations::shrinkHistory(size_t rows) {
	for (size_t i=rows; i<history.size(); ++i) {
		PRadioStreamHistory entry = history[i];
		if (util::assigned(entry)) {
			entry->deleted = true;
		}
	}
	cleanupHistory();
}

struct CHistoryDeleter {
	CHistoryDeleter() {}
    bool operator()(PRadioStreamHistory o) const {
    	if (util::assigned(o)) {
			if (o->deleted) {
				util::freeAndNil(o);
				return true;
			}
    	}
    	return false;
    }
};

void TStations::cleanupHistory() {
	history.erase(std::remove_if(history.begin(), history.end(), CHistoryDeleter()), history.end());
}

void TStations::undo() {
	if (!history.empty()) {
		PRadioStreamHistory entry = history[0];
    	if (util::assigned(entry)) {
    		bool found = false;

    		// Remove newly created entry
    		if (!found && entry->current.valid && !entry->previous.valid) {
    			for (size_t i=0; i<stations.size(); ++i) {
    				PRadioStream stream = stations[i];
    				if (util::assigned(stream)) {
    					if (stream->hash == entry->current.hash) {
    						stream->deleted = true;
    						removeDeleted();
    						break;
    					}
    				}
    			}
    			found = true;
    		}

			// Restore deleted entry
    		if (!found && !entry->current.valid && entry->previous.valid) {
				PRadioStream stream = find(entry->previous.hash);
				if (!util::assigned(stream)) {
					// Add deleted entry again
					insert(entry->previous.name, entry->previous.url, entry->previous.order, entry->previous.index, ESE_NONE);
				} else {
					// Edit existing properties
					edit(entry->previous.hash, entry->previous.name, entry->previous.url, entry->previous.order);
				}
    			found = true;
    		}

			// Restore last value before edited
    		if (!found && entry->current.valid && entry->previous.valid) {
				PRadioStream stream = find(entry->current.hash);
				if (util::assigned(stream)) {
					// Edit existing properties
					edit(entry->current.hash, entry->previous.name, entry->previous.url, entry->previous.order, ESE_NONE);
				} else {
					// Add deleted entry if needed
					insert(entry->previous.name, entry->previous.url, entry->previous.order, entry->previous.index, ESE_NONE);
				}
    			found = true;
    		}

    		// Invalid entry...
    		if (!found && !entry->current.valid && !entry->previous.valid) {
    			found = true;
    		}

    		// Remove undo entry from list
    		if (found) {
				entry->deleted = true;
				cleanupHistory();
    		}

    	} else {
    		// Remove first invalid entry
    		history.erase(history.begin());
    	}
	}
}

} /* namespace radio */
