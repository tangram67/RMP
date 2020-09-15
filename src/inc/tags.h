/*
 * tags.h
 *
 *  Created on: 02.07.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef TAGS_H_
#define TAGS_H_

#include "tagtypes.h"

namespace music {

class TMetaData {
public:
	CTextMetaData text;
	CTrackMetaData track;

	void prime() {
		text.prime();
		track.prime();
	}

	void clear() {
		text.clear();
		track.clear();
	}

	TMetaData& operator = (const music::TMetaData &value) {
		text = value.text;
		track = value.track;
		return *this;
	}

	TMetaData() {
		prime();
	};
};

class TFileTag {
private:
	static void sanitize(std::string& tag);

public:
	CMetaData meta;
	CSortData sort;
	CStreamData stream;
	CFileData file;
	CStatisticsData statistics;
	CCoverData cover;
	std::string error;

	bool isValid() const;
	void clear();
	bool parseJSON(const std::string& json);

	TFileTag& operator = (const std::string &value);
	TFileTag& operator = (const music::TFileTag &value);

	TFileTag();
	virtual ~TFileTag();
};

} /* namespace music */

#endif /* TAGS_H_ */
