/*
 * streamlist.h
 *
 *  Created on: 01.12.2019
 *      Author: dirk
 */

#ifndef APP_STREAMLIST_H_
#define APP_STREAMLIST_H_

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include "../inc/gcc.h"
#include "../inc/tables.h"

namespace radio {

enum EMetadataOrder {
	EMO_UNKNOWN,
	EMO_DISABLED,
	EMO_ARTIST_TITLE,
	EMO_TITLE_ARTIST,
	EMO_DEFAULT = EMO_ARTIST_TITLE
};

typedef struct CRadioStreamProperties {
	std::string name;
	std::string url;
	std::string mode;
	EMetadataOrder order;
} TRadioStreamProperties;

typedef struct CRadioStream {
	size_t index;
	std::string hash;
	bool deleted;
	TRadioStreamProperties values;
	TRadioStreamProperties encoded;
	TRadioStreamProperties display;

	CRadioStream() {
		index = 0;
		deleted = false;
	}

} TRadioStream;


typedef struct CRadioStreamHistoryEntry {
	std::string name;
	std::string url;
	std::string hash;
	EMetadataOrder order;
	size_t index;
	bool valid;

	CRadioStreamHistoryEntry& operator = (const CRadioStreamHistoryEntry& value) {
		name = value.name;
		url = value.url;
		hash = value.hash;
		order = value.order;
		index = value.index;
		valid = value.valid;
		return *this;
	}

	void clear() {
		name.clear();
		url.clear();
		hash.clear();
		order = EMO_DEFAULT;
		index = 0;
		valid = false;
	}

	CRadioStreamHistoryEntry() {
		index = 0;
		valid = false;
		order = EMO_DEFAULT;
	}

} TRadioStreamHistoryEntry;

typedef struct CRadioStreamHistory {
	TRadioStreamHistoryEntry previous;
	TRadioStreamHistoryEntry current;
	std::string action;
	bool deleted;

	void clear() {
		action.clear();
		previous.clear();
		current.clear();
		deleted = false;
	}

	void setPrevious(const TRadioStream* value) {
		previous.valid = false;
		if (util::assigned(value)) {
			previous.hash = value->hash;
			previous.name = value->values.name;
			previous.url = value->values.url;
			previous.order = value->values.order;
			previous.index = value->index;
			previous.valid = true;
		}
	}

	void setCurrent(const TRadioStream* value) {
		current.valid = false;
		if (util::assigned(value)) {
			current.hash = value->hash;
			current.name = value->values.name;
			current.url = value->values.url;
			current.order = value->values.order;
			current.index = value->index;
			current.valid = true;
		}
	}

	CRadioStreamHistory& operator= (const CRadioStreamHistory& value) {
		previous = value.previous;
		current = value.current;
		action = value.action;
		return *this;
	}

	CRadioStreamHistory() {
		deleted = false;
	}

} TRadioStreamHistory;


enum EEditMode {
	ESE_NONE,
	ESE_TRANSACT,
	ESE_DEFAULT = ESE_TRANSACT
};


#ifdef STL_HAS_TEMPLATE_ALIAS

using PRadioStream = TRadioStream*;
using PRadioStreamHistory = TRadioStreamHistory*;
using TStationList = std::vector<PRadioStream>;
using TStationMap = std::map<std::string, PRadioStream>;
using TStationHistoryList = std::vector<PRadioStreamHistory>;

#else

typedef TRadioStream* PRadioStream;
typedef TRadioStreamHistory* PRadioStreamHistory;
typedef std::vector<PRadioStream> TStationList;
typedef std::map<std::string, PRadioStream> TStationMap;
typedef std::vector<PRadioStreamHistory> TStationHistoryList;

#endif


class TStations {
private:
	TStationList stations;
	TStationMap map;
	std::string database;
	TRadioStreamHistory updated;
	TStationHistoryList history;
	util::TStringList exported;
	util::TStringList deflist;
	std::mutex saveMtx;
	bool changed;
	size_t limit;
	size_t offset;

	std::string encode(const std::string& text, const bool encoded);
	std::string decode(const std::string& text, const bool encoded);
	bool assign(PRadioStream stream, const std::string& text, const char delimiter);
	bool update(PRadioStream stream);
	void updateMappings();
	void removeDeleted();
	void reindex();

	void invalidate();
	bool isInvalidated() const { return changed; };

	void beginTransaction(const TRadioStream* value);
	void commitTransaction(const TRadioStream* value);
	void shrinkHistory(size_t rows);
	void cleanupHistory();

public:
	typedef TStationList::const_iterator const_iterator;

	const_iterator begin() const { return stations.begin(); };
	const_iterator end() const { return stations.end(); };

	static std::string orderModeToStr(const EMetadataOrder mode);
	static EMetadataOrder strToOrderMode(const std::string& mode);

	void clear();
	void undo();
	void clearUndoList();

	bool empty() const { return stations.empty(); };
	size_t size() const { return stations.size(); };
	bool isChanged() const { return changed; };

	PRadioStream find(const std::string hash) const;
	bool validIndex(const size_t index) const;
	int reorder(const data::TTable& table);
	bool edit(const std::string hash, const std::string name, const std::string url, const EMetadataOrder order, const EEditMode mode = ESE_DEFAULT);
	bool add(const std::string name, const std::string url, const EMetadataOrder order, const EEditMode mode = ESE_DEFAULT);
	bool insert(const std::string name, const std::string url, const EMetadataOrder order, const size_t position, const EEditMode mode = ESE_DEFAULT);
	bool remove(const std::string hash, const EEditMode mode = ESE_DEFAULT);

	void setDatabase(const std::string& file) { database = file; };
	const std::string& getDatabase() const { return database; };

	void saveToFile(const bool addHeader = true, const char delimiter = ';');
	void saveToFile(const std::string& fileName, const bool addHeader = true, const char delimiter = ';');
	size_t loadFromFile(const std::string& fileName, const bool hasHeader = true, const char delimiter = ';');
	size_t loadDefaultStations();

	std::string asJSON(const size_t index, const size_t count, const std::string& filter, const std::string& active = "");
	const util::TStringList& asM3U(size_t limit = 0, size_t offset = 0);

	TStations();
	virtual ~TStations();
};

} /* namespace radio */

#endif /* APP_STREAMLIST_H_ */
