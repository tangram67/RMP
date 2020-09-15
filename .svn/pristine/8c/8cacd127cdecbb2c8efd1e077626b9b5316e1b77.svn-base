/*
 * flac.cpp
 *
 *  Created on: 30.09.2015
 *      Author: Dirk Brinkmeier
 */

#include "flac.h"
#include "bitmap.h"
#include "nullptr.h"
#include "convert.h"
#include "exception.h"
#include "templates.h"
#include "audioconsts.h"
#include "audiobuffer.h"
#include "stringutils.h"


#define GET_VORBIS_COMMENT(comment, name, namelen, len)  (char*) \
			(((strncasecmp(name, (char*)(comment).entry, namelen) == 0) && \
			((comment).entry[strlen(name)] == '=')) ? \
			((*(len) = (comment).length - (namelen + 1)), \
			(&((comment).entry[namelen + 1]))) : nil)


FLAC__StreamDecoderWriteStatus flacDecoderCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *ctx) {
	if (util::assigned(ctx))
		return (static_cast<music::PFLACStream>(ctx))->decoderCallback(frame, buffer);
	return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

FLAC__StreamDecoderWriteStatus flacWriterCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *ctx) {
	if (util::assigned(ctx))
		return (static_cast<music::PFLACStream>(ctx))->writerCallback(frame, buffer);
	return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

FLAC__StreamDecoderReadStatus flacReaderCallback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *ctx) {
	if (util::assigned(ctx))
		return (static_cast<music::PFLACStream>(ctx))->readerCallback(buffer, bytes);
	return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

void flacMetadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *ctx) {
	if (util::assigned(ctx))
		(static_cast<music::PFLACStream>(ctx))->metadataCallback(metadata);
}

void flacErrorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *ctx) {
	if (util::assigned(ctx))
		(static_cast<music::PFLACStream>(ctx))->errorCallback(status);
}


namespace music {


TFLACObject::TFLACObject() {
	valid = false;
}


TFLACMetaDataChain::TFLACMetaDataChain() {
	chain = FLAC__metadata_chain_new();
	valid = util::assigned(chain);
}

TFLACMetaDataChain::~TFLACMetaDataChain() {
	clear();
}

void TFLACMetaDataChain::clear() {
	if (valid) {
		valid = false;
		FLAC__metadata_chain_delete(chain);
		chain = nil;
	}
}



TFALCMetaDataIterator::TFALCMetaDataIterator() {
	iterator = FLAC__metadata_iterator_new();
	valid = util::assigned(iterator);
}

TFALCMetaDataIterator::~TFALCMetaDataIterator() {
	clear();
}

void TFALCMetaDataIterator::clear() {
	if (valid) {
		valid = false;
		FLAC__metadata_iterator_delete(iterator);
		iterator = nil;
	}
}



TFLACStream::TFLACStream() {
	prime();
}

TFLACStream::~TFLACStream() {
}

void TFLACStream::prime() {
	streaming = false;
	streamdataOK = false;
	headerdataOK = false;
	metadataOK = false;
}

void TFLACStream::open(const TDecoderParams& params) {
	running = configured = params;
	streaming = true;
}

void TFLACStream::close() {
	configured.clear();
	running.clear();
	metadata.clear();
	prime();
}

bool TFLACStream::getConfiguredValues(TDecoderParams& params) {
	if (streaming) {
		params = this->configured;
		return true;
	}
	params.clear();
	return false;
}

bool TFLACStream::getRunningValues(TDecoderParams& params) {
	if (streaming) {
		params = this->running;
		return true;
	}
	params.clear();
	return false;
}

bool TFLACStream::getMetaData(TMetaData& data) {
	if (streaming) {
		data = this->metadata;
		return true;
	}
	data.clear();
	return false;
}

void TFLACStream::setRunningValues(const TDecoderParams& params) {
	running = params;
}

void TFLACStream::setMetaData(TMetaData& data) {
	metadata = data;
}



TFLACDecoder::TFLACDecoder() {
	prime();
}

TFLACDecoder::~TFLACDecoder() {
	clear();
}

void TFLACDecoder::decoderNeeded() {
	if (!valid())
		decoder = FLAC__stream_decoder_new();
	if (!valid())
		throw util::app_error("TFlacDecoder::TFlacDecoder() : Cannot allocate FLAC decoder.");
}

void TFLACDecoder::prime() {
	initval = FLAC__STREAM_DECODER_INIT_STATUS_ERROR_OPENING_FILE;
	errval = FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC;
	decoder = nil;
	debug = false;
	updated = false;
	buffer = nil;
	inbuf = nil;
	insize = 0;
}

void TFLACDecoder::clear() {
	if (util::assigned(decoder))
		FLAC__stream_decoder_delete(decoder);
	TAudioStream::clear();
	prime();
}

std::string TFLACDecoder::errmsg() const {
	return std::string(FLAC__StreamDecoderErrorStatusString[errorcode()]);
}

std::string TFLACDecoder::initmsg() const {
	return std::string(FLAC__StreamDecoderInitStatusString[initcode()]);
}

std::string TFLACDecoder::statusmsg() const {
	return std::string(FLAC__StreamDecoderStateString[statuscode()]);
}

std::string TFLACDecoder::statusmsg(const FLAC__StreamDecoderState state) const {
	return std::string(FLAC__StreamDecoderStateString[state]);
}

FLAC__StreamDecoderState TFLACDecoder::statuscode() const {
	return stateval = FLAC__stream_decoder_get_state(decoder);
}

FLAC__StreamDecoderWriteStatus TFLACDecoder::decoderCallback(const FLAC__Frame *frame, const FLAC__int32 * const buffer[]) {
	
	// Cancel operation on invalid buffer
	if (good()) {
		size_t bytes = stream.bytesPerSample * frame->header.blocksize * stream.channels;
		addRead(bytes);

		// Write decoded 32 Bit PCM samples in destination word size
		music::TAudioBuffer& data = *getBuffer();
		for(size_t i = 0; i < frame->header.blocksize; i++) {
			switch (stream.bitsPerSample) {
				case 16:
					data.write16Bit((FLAC__int16)(buffer[0][i] & (uint16_t)-1)); // Left channel
					data.write16Bit((FLAC__int16)(buffer[1][i] & (uint16_t)-1)); // Right channel
					break;
				case 24:
					data.write24Bit(buffer[0][i]); // Left channel
					data.write24Bit(buffer[1][i]); // Right channel
					break;
				case 32:
					data.write32Bit(buffer[0][i]); // Left channel
					data.write32Bit(buffer[1][i]); // Right channel
					break;
				default:
					// Unsupported bit size
					return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
					break;
			}
		}

		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

FLAC__StreamDecoderWriteStatus TFLACDecoder::writerCallback(const FLAC__Frame *frame, const FLAC__int32 * const buffer[]) {

	// Cancel operation on invalid buffer
	if (good()) {

		// Get stream properties from given frame
		if (!streamdataOK && !headerdataOK) {
			TDecoderParams params;
			getRunningValues(params);
			params.sampleRate = frame->header.sample_rate;
			params.bitsPerSample = frame->header.bits_per_sample;
			params.bytesPerSample = 8 * params.bitsPerSample;
			params.channels = frame->header.channels;
	    	params.bitRate = params.sampleRate * params.bitsPerSample * params.channels / 1024;
	    	params.valid = true;
			setRunningValues(params);
			headerdataOK = true;
		}

		// Estimate bytes to be written to output buffer
		size_t bytes = 8 * frame->header.bits_per_sample * frame->header.blocksize * frame->header.channels;
		addRead(bytes);

		// Write decoded 32 Bit PCM samples in destination word size
		if (debug) std::cout << "TFLACDecoder::writerCallback() bytes = " << bytes << std::endl;
		music::TAudioBuffer& data = *getBuffer();
		for(size_t i = 0; i < frame->header.blocksize; i++) {
			switch (frame->header.bits_per_sample) {
				case 16:
					data.write16Bit((FLAC__int16)(buffer[0][i] & (uint16_t)-1)); // Left channel
					data.write16Bit((FLAC__int16)(buffer[1][i] & (uint16_t)-1)); // Right channel
					break;
				case 24:
					data.write24Bit(buffer[0][i]); // Left channel
					data.write24Bit(buffer[1][i]); // Right channel
					break;
				case 32:
					data.write32Bit(buffer[0][i]); // Left channel
					data.write32Bit(buffer[1][i]); // Right channel
					break;
				default:
					// Unsupported bit size
					if (debug) std::cout << "TFLACDecoder::writerCallback() Abort (1)" << std::endl;
					return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
					break;
			}
		}

		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

	if (debug) std::cout << "TFLACDecoder::writerCallback() Abort (2)" << std::endl;
	return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

FLAC__StreamDecoderReadStatus TFLACDecoder::readerCallback(FLAC__byte buffer[], size_t *bytes) {
	if (debug) std::cout << "TFLACDecoder::readerCallback(0) bytes = " << *bytes << ", buffer = " << util::assigned(buffer) << ", inbuf = " << util::assigned(inbuf) << std::endl;
	if (*bytes > 0 && util::assigned(buffer) && util::assigned(inbuf)) {
		size_t size = *bytes * sizeof(FLAC__byte);
		if (debug) std::cout << "TFLACDecoder::readerCallback(1) size = " << size << ", insize = " << insize << std::endl;
		if (size > 0) {
			if (insize > 0) {
				// Copy requested buffer size (or less) to given buffer address
				if (size > insize) {
					size = insize;
				}
				insize -= size;
				inbuf += size;
				addConsumed(size);
				memcpy(buffer, inbuf, size);
				*bytes = (size / sizeof(FLAC__byte));
				if (debug) std::cout << "TFLACDecoder::readerCallback(2) read = " << size << ", bytes = " << *bytes << ", insize = " << insize << ", consumed = " << getConsumed() << std::endl;
				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
			} else {
				*bytes = 0;
				if (debug) std::cout << "TFLACDecoder::readerCallback(3) read = " << size << ", bytes = " << *bytes << ", insize = " << insize << ", consumed = " << getConsumed() << std::endl;
				return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
			}
		}
	}
	if (debug) std::cout << "TFLACDecoder::readerCallback(4) ABORT" << std::endl;
	return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}

void TFLACDecoder::metadataCallback(const FLAC__StreamMetadata *block) {
	
	// Valid metadata block?
	if (!util::assigned(block))
		return;

	TMetaData tag;
	TDecoderParams params;
	util::TTimePart sec, ms;
	FLAC__uint32 i;
	char *value;
	size_t len;
	CTagValues r;
	std::string s;

	if (block->type == FLAC__METADATA_TYPE_STREAMINFO) {
		if (debug) std::cout << "TFLACDecoder::metadataCallback() FLAC__METADATA_TYPE_STREAMINFO" << std::endl;

		// Info is crap, escape divide-by-zero
		if (block->data.stream_info.sample_rate <= 0) {
			return;
		}

		// Calculate frame size, sample count and duration
		ms = (util::TTimePart)(block->data.stream_info.total_samples * 1000 / block->data.stream_info.sample_rate);
		sec = ms / 1000;

    	// Set running stream parameter
    	params.duration = ms;
		params.seconds = sec;
    	params.bitsPerSample = normalizeSampleSize(block->data.stream_info.bits_per_sample);
    	params.bytesPerSample = params.bitsPerSample / 8;
    	params.channels = block->data.stream_info.channels;
    	params.sampleRate = block->data.stream_info.sample_rate;
		params.sampleCount = block->data.stream_info.total_samples; // Simple sample count for all channels
		params.sampleSize = params.sampleCount * params.bytesPerSample * params.channels; // Storage size for all samples in bytes
    	params.bitRate = params.sampleRate * params.bitsPerSample * params.channels / 1024;
    	params.valid = true;

    	// Set running values for current stream
    	streamdataOK = true;
    	if (params.isValid()) {
    		setRunningValues(params);
    	}
	}

	if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		if (debug) std::cout << "TFLACDecoder::metadataCallback() FLAC__METADATA_TYPE_VORBIS_COMMENT" << std::endl;

		// Read tag meta data from file
		for (i=0; i<block->data.vorbis_comment.num_comments; i++) {
			if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "ARTIST", 6, &len))) {
				if (util::assigned(value) && len > 0)
					tag.text.artist.assign(value, len);
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "TITLE", 5, &len))) {
				if (util::assigned(value) && len > 0)
					tag.text.title.assign(value, len);
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "ALBUMARTIST", 11, &len))) {
				if (util::assigned(value) && len > 0)
					tag.text.albumartist.assign(value, len);
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "ALBUM", 5, &len))) {
				if (util::assigned(value) && len > 0)
					tag.text.album.assign(value, len);
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "GENRE", 5, &len))) {
				if (util::assigned(value) && len > 0)
					tag.text.genre.assign(value, len);
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "COMPOSER", 8, &len))) {
				if (util::assigned(value) && len > 0)
					tag.text.composer.assign(value, len);
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "PERFORMER", 9, &len))) {
				if (util::assigned(value) && len > 0)
					tag.text.conductor.assign(value, len);
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "COMMENT", 7, &len))) {
				if (util::assigned(value) && len > 0)
					tag.text.comment.assign(value, len);
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "DESCRIPTION", 11, &len))) {
				if (util::assigned(value) && len > 0)
					tag.text.comment.assign(value, len);
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "TRACKNUMBER", 11, &len))) {
				if (util::assigned(value) && len > 0) {
					s.assign(value, len);
					r = tagToValues(s, 1);
					tag.track.tracknumber = r.value;
					tag.track.trackcount = r.count;
					tag.text.track = util::cprintf("%02.2d", tag.track.tracknumber);

				}
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "DISCNUMBER", 10, &len))) {
				if (util::assigned(value) && len > 0) {
					s.assign(value, len);
					r = tagToValues(s, 0);
					tag.track.disknumber = r.value;
					tag.track.diskcount = r.count;
					tag.text.disk = util::cprintf("%02.2d", tag.track.disknumber);
				}
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "YEAR", 4, &len))) {
				if (util::assigned(value) && len > 0) {
					s.assign(value, len);
					tag.text.year = util::cprintf("%4.4d", util::strToInt(s));
				}
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "DATE", 4, &len))) {
				if (util::assigned(value) && len > 0) {
					s.assign(value, len);
					if (tag.text.year.empty())
						tag.text.year = util::cprintf("%4.4d", util::strToInt(s));
					else
						tag.text.date = s;
				}
			} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "COMPILATION", 11, &len))) {
				if (util::assigned(value) && len > 0) {
					s.assign(value, len);
					int val = util::strToInt(s);
					if (val > 0)
						tag.track.compilation = true;
				}
			}
		}

    	metadataOK = true;
		setMetaData(tag);
	}
}


void TFLACDecoder::errorCallback(FLAC__StreamDecoderErrorStatus status) {
	if (debug) std::cout << "TFLACDecoder::errorCallback() ERROR: " << FLAC__StreamDecoderErrorStatusString[status] << std::endl;
	errval = status;
	error = true;
}

bool TFLACDecoder::open(const TDecoderParams& params) {
	if (!opened) {

		// Do debug output?
		if (!debug) {
			debug = params.debug;
		}

		// Initialize stream decoder
		initval = FLAC__STREAM_DECODER_INIT_STATUS_UNSUPPORTED_CONTAINER;
		if (util::isMemberOf(params.container, ECT_NATIVE,ECT_FLAC)) {
			// Open native FLAC stream
			decoderNeeded();
			initval = FLAC__stream_decoder_init_stream( decoder,              // FLAC__StreamDecoder *decoder
														flacReaderCallback,   // FLAC__StreamDecoderReadCallback read_callback
														NULL,                 // FLAC__StreamDecoderSeekCallback seek_callback
														NULL,                 // FLAC__StreamDecoderTellCallback tell_callback
														NULL,                 // FLAC__StreamDecoderLengthCallback length_callback
														NULL,                 // FLAC__StreamDecoderEofCallback eof_callback
														flacWriterCallback,   // FLAC__StreamDecoderWriteCallback write_callback
														flacMetadataCallback, // FLAC__StreamDecoderMetadataCallback metadata_callback
														flacErrorCallback,    // FLAC__StreamDecoderErrorCallback error_callback
														(void*)this );        // void *client_data
			if (debug) std::cout << "TFLACDecoder::open() FLAC container: " << FLAC__StreamDecoderInitStatusString[initval] << std::endl;
		} else if (ECT_OGG == params.container) {
			// Open encapsulated OGG stream
			decoderNeeded();
			initval = FLAC__stream_decoder_init_ogg_stream( decoder,              // FLAC__StreamDecoder *decoder
															flacReaderCallback,   // FLAC__StreamDecoderReadCallback read_callback
															NULL,                 // FLAC__StreamDecoderSeekCallback seek_callback
															NULL,                 // FLAC__StreamDecoderTellCallback tell_callback
															NULL,                 // FLAC__StreamDecoderLengthCallback length_callback
															NULL,                 // FLAC__StreamDecoderEofCallback eof_callback
															flacWriterCallback,   // FLAC__StreamDecoderWriteCallback write_callback
															flacMetadataCallback, // FLAC__StreamDecoderMetadataCallback metadata_callback
															flacErrorCallback,    // FLAC__StreamDecoderErrorCallback error_callback
															(void*)this );        // void *client_data
			if (debug) std::cout << "TFLACDecoder::open() OGG container: " << FLAC__StreamDecoderInitStatusString[initval] << std::endl;
		}

		// Check for success
		opened = initval == FLAC__STREAM_DECODER_INIT_STATUS_OK;
		if (opened) {
			TFLACStream::open(params);
		}
		if (debug) {
			std::cout << "TFLACDecoder::open() Decoder opened : " << opened << std::endl;
		}

		return opened;
	}
	return false;
}

bool TFLACDecoder::open(const TSong& song) {
	return open(song.getFileName(), song.getStreamData());
}

bool TFLACDecoder::open(const PSong song) {
	return open(song->getFileName(), song->getStreamData());
}

bool TFLACDecoder::open(const std::string& fileName, const CStreamData& properties) {
	if (!opened && !fileName.empty()) {
		if (properties.isValid()) {
			decoderNeeded();
			if (debug) std::cout << "TFlacDecoder::open() File \"" << fileName << "\"" << std::endl;
			initval = FLAC__stream_decoder_init_file(decoder, fileName.c_str(), flacDecoderCallback, NULL, flacErrorCallback, (void*)this);
			opened = initval == FLAC__STREAM_DECODER_INIT_STATUS_OK;
			if (opened)
				setStream(properties);
			if (debug) {
				stream.debugOutput();
				std::cout << "TFlacDecoder::open() Decoder opened : " << opened << std::endl;
			}
			return opened;
		}
	}
	return false;
}

bool TFLACDecoder::update(PAudioBuffer buffer, size_t& read) {
	read = 0;

	// Decode single chunk of data from file
	if (opened && !eof && !error && util::assigned(buffer)) {
		setRead(0);
		setBuffer(buffer);
		FLAC__bool r = updated ?
				FLAC__stream_decoder_process_single(decoder) :
				FLAC__stream_decoder_process_until_end_of_metadata(decoder);
		eof = FLAC__STREAM_DECODER_END_OF_STREAM == statuscode();
		updated = true;
		read = getRead();
		if (debug && eof) {
			 std::cout << "TFlacDecoder::update() EOF detected." << std::endl;
		}
		return (r > 0);
	}

	// Create internal error
	errval = FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC;
	error = true;
	setBuffer(nil);
	return false;
}

bool TFLACDecoder::update(const TSample * const data, const size_t size, PAudioBuffer buffer, size_t& written, size_t& consumed) {
	written = 0;
	consumed = 0;

	// Set pointer to input data buffer
	inbuf = data;
	insize = size;

	if (debug) {
		FLAC__StreamDecoderState state = statuscode();
		std::cout << "TFLACDecoder::update(0) size = " << insize << ", opened = " << opened << ", eof = " << eof << ", error = " << error << ", buffer = " << util::assigned(buffer) << ", written = " << written << std::endl;
		std::cout << "TFLACDecoder::update(0) state = " << FLAC__StreamDecoderStateString[state] << " ("  << state << ")" << std::endl;
	}

	// Decode single chunk of data from file
	if (opened && !error && util::assigned(buffer)) {

		// Setup decoding process
		setRead(0);
		setConsumed(0);
		setBuffer(buffer);
		FLAC__bool r = updated ?
				FLAC__stream_decoder_process_single(decoder) :
				FLAC__stream_decoder_process_until_end_of_metadata(decoder);
		consumed = getConsumed();
		written = getRead();
		updated = true;

		if (debug) {
			FLAC__StreamDecoderState state = statuscode();
			std::cout << "TFLACDecoder::update(1) error = " << error << ", written = " << written << ", result = " << r << std::endl;
			std::cout << "TFLACDecoder::update(1) state = " << FLAC__StreamDecoderStateString[state] << " ("  << state << ") written = " << written << ", consumed = " << consumed << ", result = " << r << std::endl;
		}

		return (r > 0);
	}

	// Create internal error
	if (debug) std::cout << "TFLACDecoder::update(2) eof = " << eof << ", error = " << error << ", written = " << written << ", consumed = " << consumed << std::endl;
	errval = FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC;
	error = true;
	setBuffer(nil);
	return false;
}


bool TFLACDecoder::getConfiguredValues(TDecoderParams& params) {
	return TFLACStream::getConfiguredValues(params);
}

bool TFLACDecoder::getRunningValues(TDecoderParams& params) {
	return TFLACStream::getRunningValues(params);
}

void TFLACDecoder::close() {
	if (opened) {
		TFLACStream::close();
		FLAC__stream_decoder_finish(decoder);
	}
	clear();
	if (debug) std::cout << "TFlacDecoder::close()" << std::endl;
}




TFLACFile::TFLACFile() : TSong(EFT_FLAC) {
	prime();
}

TFLACFile::TFLACFile(const std::string& fileName) : TSong(fileName, EFT_FLAC) {
	prime();
}

TFLACFile::~TFLACFile() {
}

void TFLACFile::prime() {
}

void TFLACFile::decoderNeeded() {
	if (!util::assigned(stream))
		stream = new TFLACDecoder;
}

bool TFLACFile::readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) {
	if (fileName.empty())
		return false;

	FLAC__StreamMetadata *block;
	util::TTimePart sec, ms;
	int found = 0;
	FLAC__uint32 i;
	char *value;
	size_t len;
	std::string s;
	CTagValues r;

	TFLACMetaDataChain chain;
	if (!chain.isValid()) {
		tag.error = "Cannot allocate FLAC metadata chain, TFLACMetaDataChain object is invalid";
		return false;
	}

	if (!FLAC__metadata_chain_read(chain(), fileName.c_str())) {
		tag.error = "Cannot allocate FLAC metadata chain, FLAC__metadata_chain_read() failed";
		return false;
	}

	TFALCMetaDataIterator iterator;
	if (!iterator.isValid()) {
		tag.error = "Cannot allocate FLAC metadata iterator, TFALCMetaDataIterator is invalid";
		return false;
	}

	FLAC__metadata_iterator_init(iterator(), chain());
	do {
		block = FLAC__metadata_iterator_get_block(iterator());

		if (block->type == FLAC__METADATA_TYPE_STREAMINFO) {

			// Info is crap, escape divide-by-zero
			if (block->data.stream_info.total_samples <= 0 || block->data.stream_info.sample_rate <= 0) {
				tag.error = util::csnprintf("Missing or zeroed sound properties: TotalSamples=%, SampleRate=%", block->data.stream_info.total_samples, block->data.stream_info.sample_rate);
				break;
			}

			// Calculate frame size, sample count and duration
			ms = (util::TTimePart)(block->data.stream_info.total_samples * 1000 / block->data.stream_info.sample_rate);
			sec = ms / 1000;

			// Time is crap, escape divide-by-zero
			if (ms <= 0 || sec <= 0) {
				tag.error = "Invalid song duration in metadata";
				break;
			}

			// Get stream information
			tag.stream.duration = ms;
			tag.stream.seconds = sec;
			tag.stream.channels = block->data.stream_info.channels;
			tag.stream.sampleRate = block->data.stream_info.sample_rate;
			tag.stream.bitsPerSample = normalizeSampleSize(block->data.stream_info.bits_per_sample);
			tag.stream.bytesPerSample = tag.stream.bitsPerSample / 8;
			tag.stream.sampleCount = block->data.stream_info.total_samples; // Simple sample count for all channels
			tag.stream.sampleSize = tag.stream.sampleCount * tag.stream.bytesPerSample * tag.stream.channels; // Storage size for all samples in bytes
			tag.stream.bitRate = bitRateFromTags(tag);

			// Estimated buffer size for frame decoder
			tag.stream.chunkSize = std::max(block->data.stream_info.max_framesize, block->data.stream_info.max_blocksize);
			if (tag.stream.chunkSize <= 0)
				tag.stream.chunkSize = AUDIO_CHUNK_SIZE;

        	if (!tag.stream.isValid()) {
				tag.stream.addErrorHint(tag.error);
				tag.stream.debugOutput(" FLAC: ");
				tag.meta.debugOutput(" META: ");
				break;
        	}

			found |= 1;
			if (found == 3)
				break;
		}

		if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {

			// Read tag meta data from file
			for (i=0; i<block->data.vorbis_comment.num_comments; i++) {
				if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "ARTIST", 6, &len))) {
					if (util::assigned(value) && len > 0)
						tag.meta.text.artist.assign(value, len);
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "TITLE", 5, &len))) {
					if (util::assigned(value) && len > 0)
						tag.meta.text.title.assign(value, len);
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "ALBUMARTIST", 11, &len))) {
					if (util::assigned(value) && len > 0)
						tag.meta.text.albumartist.assign(value, len);
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "ALBUM", 5, &len))) {
					if (util::assigned(value) && len > 0)
						tag.meta.text.album.assign(value, len);
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "GENRE", 5, &len))) {
					if (util::assigned(value) && len > 0)
						tag.meta.text.genre.assign(value, len);
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "COMPOSER", 8, &len))) {
					if (util::assigned(value) && len > 0)
						tag.meta.text.composer.assign(value, len);
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "PERFORMER", 9, &len))) {
					if (util::assigned(value) && len > 0)
						tag.meta.text.conductor.assign(value, len);
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "COMMENT", 7, &len))) {
					if (util::assigned(value) && len > 0)
						tag.meta.text.comment.assign(value, len);
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "DESCRIPTION", 11, &len))) {
					if (util::assigned(value) && len > 0)
						tag.meta.text.comment.assign(value, len);
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "TRACKNUMBER", 11, &len))) {
					if (util::assigned(value) && len > 0) {
						s.assign(value, len);
						r = tagToValues(s, 1);
						tag.meta.track.tracknumber = r.value;
						tag.meta.track.trackcount = r.count;
						tag.meta.text.track = util::cprintf("%02.2d", tag.meta.track.tracknumber);

					}
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "DISCNUMBER", 10, &len))) {
					if (util::assigned(value) && len > 0) {
						s.assign(value, len);
						r = tagToValues(s, 0);
						tag.meta.track.disknumber = r.value;
						tag.meta.track.diskcount = r.count;
						tag.meta.text.disk = util::cprintf("%02.2d", tag.meta.track.disknumber);
					}
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "YEAR", 4, &len))) {
					if (util::assigned(value) && len > 0) {
						s.assign(value, len);
						tag.meta.text.year = util::cprintf("%4.4d", util::strToInt(s));
					}
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "DATE", 4, &len))) {
					if (util::assigned(value) && len > 0) {
						s.assign(value, len);
						if (tag.meta.text.year.empty())
							tag.meta.text.year = util::cprintf("%4.4d", util::strToInt(s));
						else
							tag.meta.text.date = s;
					}
				} else if ((value = GET_VORBIS_COMMENT(block->data.vorbis_comment.comments[i], "COMPILATION", 11, &len))) {
					if (util::assigned(value) && len > 0) {
						s.assign(value, len);
						int val = util::strToInt(s);
						if (val > 0)
							tag.meta.track.compilation = true;
					}
				}
			}
			found |= 2;
			if (found == 3)
				break;
		}
	} while (FLAC__metadata_iterator_next(iterator()));

	return (found == 3);
}

bool TFLACFile::readPictureData(const std::string& fileName, TCoverData& cover) {
	if (fileName.empty())
		return false;

	FLAC__StreamMetadata *block;
	util::TRGB *picture;
	bool found = false;
	size_t size;

	TFLACMetaDataChain chain;
	if (!chain.isValid()) {
		throw util::app_error("TFlacFile::readPictureData() : Cannot allocate FLAC metadata chain for file <" + fileName + ">");
	}

	if (!FLAC__metadata_chain_read(chain(), fileName.c_str())) {
		throw util::app_error("TFlacFile::readPictureData() : Cannot read FLAC metadata from file <" + fileName + ">");
	}

	TFALCMetaDataIterator iterator;
	if (!iterator.isValid()) {
		throw util::app_error("TFlacFile::readPictureData() : Cannot allocate FLAC metadata iterator for file <" + fileName + ">");
	}

	FLAC__metadata_iterator_init(iterator(), chain());
	do {
		block = FLAC__metadata_iterator_get_block(iterator());

		if (block->type == FLAC__METADATA_TYPE_PICTURE) {
			if (block->data.picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER ||
				block->data.picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_OTHER) {
				picture = (util::TRGB*)block->data.picture.data;
				size = block->data.picture.data_length;
				cover.artwork.assign(picture, size);
				cover.filename = fileName;
				found = true;
				break;
			}
		}

		if (found)
			break;

	} while (FLAC__metadata_iterator_next(iterator()));

	return cover.isValid();
}



TFLACSong::TFLACSong() : TSong(EFT_FLAC) {
}

TFLACSong::~TFLACSong() {
}

void TFLACSong::decoderNeeded() {
	if (!util::assigned(stream))
		stream = new TFLACDecoder;
}

bool TFLACSong::readMetaData(const std::string& fileName, TFileTag& tag, ECodecType& type) {
	tag.error = "No FLAC metadata for plain stream.";
	return false;
}

bool TFLACSong::readPictureData(const std::string& fileName, TCoverData& cover) {
	return false;
}

} /* namespace music */
