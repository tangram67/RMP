/*
 * blob.cpp
 *
 *  Created on: 20.04.2020
 *      Author: dirk
 */

#include "blob.h"

namespace util {

TBlob::TBlob() : util::TBuffer(0, false) {
}

TBlob::TBlob(const void *const data, const size_t size) : util::TBuffer(0, false) {
	util::TBuffer::assign(data, size);
}

TBlob::TBlob(const std::string& data) : util::TBuffer(0, false) {
	util::TBuffer::assign(data.c_str(), data.size());
}

TBlob::TBlob(const TBlob& data) : util::TBuffer(0, false) {
	util::TBuffer::assign(data.data(), data.size());
}

TBlob::~TBlob() {
	clear();
}

bool TBlob::saveToFile(const std::string fileName) const {
	if (util::assigned(data()) && size() > 0) {
		util::TFileGuard<TFile> fg(file);
		file.assign(fileName);
		file.create(size());
		fg.open(O_WRONLY);
		ssize_t written = file.write(data(), size());
		if (written > 0) {
			if (size() == (size_t)written)
				return true;
		}
	}
	return false;
}

void TBlob::loadFromFile(const std::string fileName) {
	if (util::fileExists(fileName)) {
		file.assign(fileName);
		size_t n = file.getSize();
		if (n > 0) {
			util::TFileGuard<TFile> fg(file);
			clear();
			resize(n);
			fg.open(O_RDONLY);
			file.read(data(), size());
		}
	}
}

void TBlob::clear() {
	util::TBuffer::clear();
	file.release();
}

TBlob& TBlob::operator = (const TBlob& value) {
	clear();
	if (value.size() > 0) {
		util::TBuffer::assign(value.data(), value.size());
	}
	return *this;
}

TBlob& TBlob::operator = (const std::string& value) {
	clear();
	if (value.size() > 0) {
		assign(value);
	}
	return *this;
}

void TBlob::assign(const void *const data, const size_t size) {
	util::TBuffer::assign(data, size);
}

void TBlob::assign(const std::string& data) {
	util::TBuffer::assign(data.c_str(), data.size());
}

void TBlob::assign(const TBlob& data) {
	util::TBuffer::assign(data.data(), data.size());
}


} /* namespace util */
