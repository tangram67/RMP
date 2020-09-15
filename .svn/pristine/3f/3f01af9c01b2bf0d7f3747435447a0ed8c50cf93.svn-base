/*
 * audiostream.cpp
 *
 *  Created on: 16.08.2016
 *      Author: Dirk Brinkmeier
 */

#include "audiostream.h"
#include "audioconsts.h"

namespace music {

TAudioStream::TAudioStream() {
	prime();
}

TAudioStream::~TAudioStream() {
}

void TAudioStream::prime() {
	read = 0;
	consumed = 0;
	buffer = nil;
	error = false;
	opened = false;
	eof = false;
}

void TAudioStream::clear() {
	prime();
	stream.clear();
}

bool TAudioStream::update(const TSample *const data, const size_t size, PAudioBuffer buffer, size_t& written, size_t& consumed) {
	written = 0;
	consumed = 0;
	return false;
}

bool TAudioStream::getConfiguredValues(TDecoderParams& params) {
	params.clear();
	return false;
}

bool TAudioStream::getRunningValues(TDecoderParams& params) {
	params.clear();
	return false;
}


TAudioStreamAdapter::TAudioStreamAdapter() {
	stream = nil;
}

TAudioStreamAdapter::~TAudioStreamAdapter() {
	clear();
}

PAudioStream TAudioStreamAdapter::getStream() {
	decoderNeeded();
	return stream;
}

void TAudioStreamAdapter::clear() {
	util::freeAndNil(stream);
}

} /* namespace music */
