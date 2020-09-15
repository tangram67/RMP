/*
 * blob.h
 *
 *  Created on: 20.04.2020
 *      Author: dirk
 */

#ifndef INC_BLOB_H_
#define INC_BLOB_H_

#include <string>
#include "memory.h"
#include "fileutils.h"


namespace util {

class TBlob : public util::TBuffer {
private:
	mutable TFile file;

public:
	void clear();
	bool saveToFile(const std::string fileName) const;
	void loadFromFile(const std::string fileName);

	void assign(const void *const data, const size_t size);
	void assign(const std::string& data);
	void assign(const TBlob& data);

	TBlob& operator = (const TBlob& value);
	TBlob& operator = (const std::string& value);

	TBlob(const void *const data, const size_t size);
	TBlob(const std::string& data);
	TBlob(const TBlob& data);
	TBlob();

	virtual ~TBlob();
};


} /* namespace util */

#endif /* INC_BLOB_H_ */
