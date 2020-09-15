/*
 * inetstream.cpp
 *
 *  Created on: 18.11.2019
 *      Author: dirk
 */

#include "inetstream.h"
#include "stringutils.h"
#include "charconsts.h"
#include "compare.h"
#include "convert.h"
#include "ASCII.h"
#include "ansi.h"

namespace inet {

STATIC_CONST char* ICY_METADATA_TITLE = "StreamTitle";
STATIC_CONST size_t ICY_METADATA_TITLE_LEN = strlen(ICY_METADATA_TITLE);

TInetStream::TInetStream() {
	init();
}

TInetStream::~TInetStream() {
}

void TInetStream::init() {
	onInetStreamData = nil;
	onInetStreamTitle = nil;
	onInetStreamInfo = nil;
	onInetStreamError = nil;
	debug = false;

	// Bind TURL events
	curl.bindReadEvent(&inet::TInetStream::onDownloadRead, this);
	curl.bindHeaderEvent(&inet::TInetStream::onDownloadHeader, this);
	curl.bindDataEvent(&inet::TInetStream::onDownloadReceived, this);

	// Enable Icecast metadata protocol
	curl.addHeader("Icy-MetaData: 1");

	// No compression
	curl.setHttpProtocol(app::EHP_VERSION_1);
	curl.setIgnoreContentLength(false);
	curl.setCompress(false);
	setChunkSize(1390);
	setTimeout(10);

	clear();
}

void TInetStream::clear() {
	byteIndex = 0;
	icyMetaint = 0;
	bytesReceived = 0;
	bytesProcessed = 0;
	headersSent = false;
	errval = EXIT_SUCCESS;
	chunk.clear();
	buffer.clear();
	errtxt.clear();
	info.clear();
	url.clear();
}

void TInetStream::abort(const int error, const std::string& text) {
	errval = error;
	errtxt = text;
	bool terminate = true;
	onStreamError(error, text, terminate);
	if (terminate) {
		curl.abort();
	}
}

void TInetStream::terminate() {
	errval = EXIT_SUCCESS;
	errtxt = "TInetStream::terminate() Terminated during normal execution.";
	curl.terminate();
}

void TInetStream::setDebug(const bool value) {
	debug = value;
	curl.setDebug(debug);
};

void TInetStream::setTimeout(const long value) {
	curl.setConnectTimeout(value);
}

void TInetStream::setChunkSize(const size_t value) {
	curl.setChunkSize(value);
}

bool TInetStream::receive(const std::string& URL, const std::string& fileName) {
	if (!fileName.empty()) {
		curl.setFileName(fileName);
	}
	return receive(URL);
}

bool TInetStream::receive(const std::string& URL) {
	bool _debug = debug;
	bool result = false;
	bool valid = true;
	bool ok = false;
	clear();

	// Set stream URL
	info.url = url = URL;

	// Get playlist type from extension mime type
	EPlaylistType type = EPT_NONE;
	std::string ext = util::fileExt(url);
	if (util::isMimeType(ext, M3U_MIME_TYPE)) {
		type = EPT_M3U;
	} else if (util::isMimeType(ext, PLS_MIME_TYPE)) {
		type = EPT_PLS;
	} else if (util::isMimeType(ext, XSPF_MIME_TYPE)) {
		type = EPT_XSPF;
	}

	// Check if URL is known playlist type
	if (EPT_NONE != type) {
		valid = false;
		ok = curl.readData(url);
		if (ok && curl.hasData()) {
			std::string data(curl.getData().data(), curl.getData().size());
			if (!data.empty()) {
				// Get first entry of M3U playlist
				util::TStringList pls;
				pls.assign(data, '\n');
				if (!pls.empty()) {
					for (size_t i=0; i<pls.size(); ++i) {
						const std::string& line = util::trim(pls.at(i));
						if (line.size() > 7) {
							//
							// Playlist as M3U:
							// ================
							//   #EXTM3U
							//   #EXTINF:-1,The Current
							//   http://current.stream.publicradio.org/current.mp3
							//
							// Playlist as PLS:
							// ================
							//   [playlist]
							//   numberofentries=1
							//   File1=http://94.23.201.38:8000/stream
							//   Title1=Audiophile Baroque
							//   Length1=-1
							//   version=2
							//
							// Playlist as XSPF:
							// =================
							//   <?xml version="1.0" encoding="UTF-8"?>
							//   <playlist xmlns="http://xspf.org/ns/0/" version="1">
							//     <title/>
							//     <creator/>
							//     <trackList>
							//   	<track>
							//   	  <location>http://thecheese.ddns.net:8004/stream</location>
							//   	  <title/>
							//   	  <annotation>Stream Title: The Cheese\n
							//   Stream Description: Hits of the 80s, 90s and Today\n
							//   Content Type:audio/ogg\n
							//   Current Listeners: 2\n
							//   Peak Listeners: 8\n
							//   Stream Genre: Pop</annotation>
							//   	  <info>https://www.thecheese.co.nz</info>
							//   	</track>
							//     </trackList>
							//   </playlist>
							//

							// Find possible start position depending on playlist type
							std::string sl = util::tolower(line);
							size_t start = std::string::npos;
							size_t end = std::string::npos;
							bool found = false;
							switch (type) {
								case EPT_M3U:
									start = 0;
									found = true;
									break;
								case EPT_PLS:
									start = sl.find("file");
									if (std::string::npos != start) {
										start += 4;
										found = true;
									}
									break;
								case EPT_XSPF:
									start = sl.find("<location>");
									if (std::string::npos != start) {
										start += 10;
										end = sl.find("</location>", start);
										if (std::string::npos != end)
											found = true;
									}
									break;
								default:
									break;
							}

							// Get URL from playlist file
							if (found) {
								size_t pos = sl.find("http://", start);
								if (std::string::npos == pos)
									pos = sl.find("https://", start);
								if (std::string::npos != pos) {
									url.clear();
									if (std::string::npos == end) {
										url = line.substr(pos);
									} else {
										if (end > pos)
											url = line.substr(pos, end - pos);
									}
									if (!url.empty()) {
										valid = true;
										if (_debug) std::cout << "Parsed " << util::toupper(ext) << " playlist \"" << url << "\"" << std::endl;
									}
								}
							}

						}
					}
				}
			}
		}
		if (!ok) {
			errval = curl.error();
			errtxt = "Receiving M3U playlist \"" + URL + "\" failed: " + curl.errmsg();
			if (_debug) std::cout << errtxt << std::endl;
		}
	}

	// Receive data from stream URL
	if (valid) {

		// Try to receive data stream
		info.stream = url;
		ok = curl.receive(url);
		if (_debug) {
			std::cout << "Receiving URL \"" << url << "\" Result   = " << ok << std::endl;
			std::cout << "Receiving URL \"" << url << "\" Success  = " << curl.success() << std::endl;
			std::cout << "Receiving URL \"" << url << "\" Aborted  = " << curl.isAborted() << std::endl;
			std::cout << "Receiving URL \"" << url << "\" Received = " << util::sizeToStr(curl.received()) << std::endl;
		}
		valid = false;

		// Retry on given 'Location' header
		if (curl.success() && !curl.isAborted() && curl.received() < 1024 && curl.getHeaders().hasKey("Location")) {
			url = curl.getHeaders().value("Location").asString();
			if (!url.empty()) {
				// Receive stream data from forwarded location
				if (_debug) std::cout << "Retry on forwarded URL \"" << url << "\"" << std::endl;
				info.stream = url;
				ok = curl.receive(url);
				valid = true;
			}
		}

		if (valid && _debug) {
			std::cout << "Receiving URL \"" << url << "\" Result   = " << ok << std::endl;
			std::cout << "Receiving URL \"" << url << "\" Success  = " << curl.success() << std::endl;
			std::cout << "Receiving URL \"" << url << "\" Aborted  = " << curl.isAborted() << std::endl;
			std::cout << "Receiving URL \"" << url << "\" Received = " << util::sizeToStr(curl.received()) << std::endl;
		}

		// Check result
		if (ok) {
			result = true;
		} else {
			errval = curl.error();
			errtxt = "Receiving URL \"" + URL + "\" failed: " + curl.errmsg();
			if (_debug) std::cout << errtxt << std::endl;
		}

	} else {
		errval = curl.error();
		errtxt = "No valid stream URL \"" + URL + "\" found for [" + util::getMimeType(ext) + "]";
		if (_debug) std::cout << errtxt << std::endl;
	}

	return result && EXIT_SUCCESS == errval;
}

std::string TInetStream::convertToMultibyteString(const std::string& s) {
	if (!s.empty())
		return convertToMultibyteString(s.c_str(), s.size());
	return std::string();
}

std::string TInetStream::convertToMultibyteString(const char* s, const size_t size) {
	if (util::assigned(s) && size > 0) {
		std::string text = util::TASCII::isValidUTF8MultiByteStr(s, size) ?
							util::TASCII::UTF8ToMultiByteStr(s, size) :
							 util::TASCII::CP1252ToMultiByteStr(s, size);
		return util::trim(util::unescape(text));
	}
	return std::string();
}

bool TInetStream::findMetaData(const void *const data, const size_t size, size_t& pos, size_t& start, size_t& length) {
	const char *a;
	const char *b = ICY_METADATA_TITLE;
	const char *p = (const char *)data;
	const char *q = p;
    bool found = false;
    int u1, u2;

    // Reset result values
    pos = 0;
    start = 0;
    length = 0;

    for (size_t i=0; i<size; ++p, ++i) {
		u1 = tolower(*p);
		u2 = tolower(*b);
		if (u1 != u2)
			continue;

		a = p;
		while (true) {
			++i;
			if (*b == 0) {
				pos = p - q;
				found = true;
				break;
			}
			if (i > size)
				break;
			u1 = tolower(*a++);
			u2 = tolower(*b++);
			if (u1 != u2)
				break;
		}

		if (found)
			break;

		b = ICY_METADATA_TITLE;
	}

    if (found && pos > 0) {
    	start = pos - 1;
    	p = (const char *)data;
    	q = p + start;
    	unsigned char* r = (unsigned char*)q;
    	length = 16 * (size_t)(*r);
        return (pos > 0) && (start >= 0) && (length > 0);
    }

    return false;
}

bool TInetStream::parseMetaData(const void *const data, const size_t pos, const size_t length, std::string& title, std::string& metadata) {
	title.clear();
	if (pos > 0) {
		unsigned char* p = (unsigned char *)data;
		if (length > 0 && length < 512) {
			size_t begin = pos + ICY_METADATA_TITLE_LEN;
			size_t end = pos + length;
			size_t idx1 = 0, idx2 = 0;
			bool quoted = false;
			bool found = false;
			int state = 0;
			char c = '*';
			char f = '*';
			char n = '*';
			for (size_t i=begin; i<end; ++i) {
				c = p[i];
				n = p[i + 1];
				switch (state) {
					case 0:
						if ('=' == c) {
							state = 10;
						}
						break;

					case 10:
						if ('\'' == c) {
							found = true;
						}
						if ('\"' == c) {
							found = true;
							quoted = true;
						}
						if (found) {
							state = 20;
							idx1 = i + 1;
							found = false;
						}
						break;

					case 20:
						if (util::NUL == c) {
							found = true;
						} else {
							if (quoted) {
								if ('\"' == c && f != '\\' /*&& (f != '\"' || f == util::NUL)*/ && (n == ';' || n == util::NUL)) {
									found = true;
								}
							} else {
								if ('\'' == c && f != '\\' /*&& (f != '\"' || f == util::NUL)*/ && (n == ';' || n == util::NUL)) {
									found = true;
								}
							}
						}
						if (found) {
							state = 30;
							idx2 = i - 1;
						}
						break;
				}
				if (found)
					break;
				f = c;
			}
			if (found && idx2 > idx1) {
				size_t size = idx2 - idx1 + 1;
				const char* s = (const char*)(p + idx1);
				metadata = util::TBinaryConvert::binToText(s, size, util::TBinaryConvert::EBT_HEX);
				title = convertToMultibyteString(s, size);
				return true;
			}
		}
	}
	return false;
}



bool TInetStream::parseMetaDataInplace(const char *const data, const size_t length, std::string& title, std::string& metadata, size_t& broken) {
	broken = 0;
	title.clear();
	metadata.clear();
	// Example: "StreamTitle='';StreamUrl='';adw_ad='true';durationMilliseconds='20427';adId='10233';insertionType='preroll';<00><00><00><00><8A>"
	if (length > 0 && length < 512) {
		char* p = (char*)data;
		size_t begin = ICY_METADATA_TITLE_LEN;
		size_t end = length;
		size_t idx1 = 0, idx2 = 0;
		bool quoted = false;
		bool found = false;
		int state = 0;
		char f = util::NUL;
		char c = '*';
		char n = '*';
		for (size_t i=begin; i<end; ++i) {
			c = p[i];
			n = p[i + 1];
			switch (state) {
				case 0:
					if ('=' == c) {
						state = 10;
					}
					break;

				case 10:
					if ('\'' == c) {
						found = true;
					}
					if ('\"' == c) {
						found = true;
						quoted = true;
					}
					if (found) {
						state = 20;
						idx1 = i + 1;
						found = false;
					}
					break;

				case 20:
					if (util::NUL == c) {
						found = true;
						broken = i;
					} else {
						if (quoted) {
							if ('\"' == c && f != '\\' /*&& (f != '\"' || f == util::NUL)*/ && (n == ';' || n == util::NUL)) {
								found = true;
							}
						} else {
							if ('\'' == c && f != '\\' /*&& (f != '\'' || f == util::NUL)*/ && (n == ';' || n == util::NUL)) {
								found = true;
							}
						}
					}
					if (found) {
						state = 30;
						idx2 = i - 1;
					}
					break;
			}
			if (found)
				break;
			f = c;
		}
		if (found && idx2 > idx1) {
			size_t size = idx2 - idx1 + 1;
			const char* s = (const char*)(p + idx1);
			metadata = util::TBinaryConvert::binToText(s, size, util::TBinaryConvert::EBT_HEX);
			title = convertToMultibyteString(s, size);
			return true;
		}
	}
	return false;
}

// Parse and strip ICECAST data from stream
// See: http://www.smackfu.com/stuff/programming/shoutcast.html
void TInetStream::onDownloadReceived(const app::TWebClient& sender, const void *const data, const size_t size) {
	if (size > 0) {
		size_t received = 0;
		size_t buffered = size;
		std::string title, metadata;
		size_t length;
		bool found = false;
		bool overflow = false;
		bool icecast = icyMetaint > 0;

		// Preallocate buffer sizes
		if (buffer.empty()) {
			buffer.reserve(2 * buffered, false);
	        chunk.resize(3 * buffered / 2, false);
		} else {
			buffer.reserve(2 * (buffered + buffer.size()), true);
	        chunk.resize(3 * (buffered + buffer.size()) / 2, false);
		}

        // Buffer and data pointers
		char* q = chunk.data();
		char* p = (char*)data;

		// Check for possible metadata buffer overlap
		if (icecast && buffer.empty()) {
			size_t icyIndex = 0;

			// Is next metaint in current buffer
			if (icyMetaint < (byteIndex + buffered) ) {
				length = 0;

				// Get length from next ICY metadata position
				icyIndex = icyMetaint - byteIndex;
				char* r = p + icyIndex;
		    	unsigned char* u = (unsigned char*)r;
		    	length = 16 * (size_t)(*u);
				if (debug) {
					std::string s = util::TBinaryConvert::binToText(r-2, 8, util::TBinaryConvert::EBT_HEX);
					std::cout << app::white << "Internet stream \"" << sender.getBaseName() << "\" Metadata index  = " << icyIndex << app::reset << std::endl;
					std::cout << app::white << "Internet stream \"" << sender.getBaseName() << "\" Metadata length = " << length << app::reset << std::endl;
					std::cout << app::white << "Internet stream \"" << sender.getBaseName() << "\" Metadata [" << s << "]" << app::reset << std::endl;
				}
			}

			// Does current metadata length exceeds buffer?
			if (length > 0 && icyIndex > 0) {
				if ((icyIndex + length + 1) > buffered) {
					overflow = true;
					if (debug) {
						std::cout << app::red << "Internet stream \"" << sender.getBaseName() << "\" Buffer overflow for data size = " << buffered << app::reset << std::endl;
					}
				}
			}
		}

		// Will overflow on metadata happen?
		if (overflow) {
			// Append data to buffer and wait for next chunk
			buffer.append(data, buffered);
		} else {

			// Has previous chunk stored in buffer?
			if (!buffer.empty()) {
				// Append current chunk and set data pointer
				buffer.append(data, buffered);
				p = buffer.data();
				buffered = buffer.size();
				if (debug) {
					std::cout << app::red << "Internet stream \"" << sender.getBaseName() << "\" Collected buffer size = " << buffered << app::reset << std::endl;
				}
			}

			// Check for Icecast metadata frame
			if (icecast) {
				size_t i = 0;
				bool exit = false;
				while (i < buffered && !exit) {
					length = 0;
					found = false;

					// Check for expected ICY data position
					if (icyMetaint == byteIndex) {
						byteIndex = 0;
						found = true;
						unsigned char* r = (unsigned char*)p;
						length = 16 * (size_t)(*r);
						if (debug) {
							std::string s = util::TBinaryConvert::binToText(p-2, 64, util::TBinaryConvert::EBT_HEX);
							std::cout << app::cyan << "Internet stream \"" << sender.getBaseName() << "\" Metadata [" << s << "]" << app::reset << std::endl;
							if (length > 0)
								std::cout << app::red << "Internet stream \"" << sender.getBaseName() << "\" Metadata length = " << length << app::reset << std::endl;
							else
								std::cout << app::green << "Internet stream \"" << sender.getBaseName() << "\" Metadata length = " << length << app::reset << std::endl;
						}
					}

					// Parse ICY metadata
					if (!exit && length > 0) {
						char* r = p + 1;
						if (debug && length < 512) {
							std::string s = util::TBinaryConvert::binToText(r, length + 1, util::TBinaryConvert::EBT_HEX);
							std::cout << app::green << "Internet stream \"" << sender.getBaseName() << "\" Metadata [" << s << "]" << app::reset << std::endl;
						}
						size_t broken = 0;
						if (parseMetaDataInplace(r, length, title, metadata, broken)) {
							if (broken > 0) {
								if (debug) {
									std::cout << app::red  << "Internet stream \"" << sender.getBaseName() << "\" Buffer metatdata overlapped (" << length << "/" << broken << ")" << app::reset << std::endl;
								}
								abort(20000, util::csnprintf("TInetStream::onDownloadReceived() Buffer metatdata overlapped (%/%)", length, broken));
								title.clear();
								received = 0;
								exit = true;
							}
							if (debug && !title.empty()) {
								std::cout << app::green  << "Internet stream \"" << sender.getBaseName() << "\" Current title \"" << title << "\"" << app::reset << std::endl;
								std::cout << app::green  << "Internet stream \"" << sender.getBaseName() << "\" Metadata \"" << metadata << "\"" << app::reset << std::endl;
							}
						}
					}

					// Store raw data only, ignore metadata
					if (!exit) {
						if (found) {
							// Skip metadata block
							if (debug) std::cout << app::magenta  << "Internet stream \"" << sender.getBaseName() << "\" Current index = " << i << app::reset << std::endl;
							if (length > 0) {
								p += (length + 1); // + 1 --> Length byte
								i += (length + 1); // + 1 --> Length byte
							} else {
								++p;
								++i;
							}
							if (debug) std::cout << app::magenta  << "Internet stream \"" << sender.getBaseName() << "\" Next index    = " << i << app::reset << std::endl;
							if (debug) std::cout << app::magenta  << "Internet stream \"" << sender.getBaseName() << "\" Data size     = " << size << app::reset << std::endl;
						} else {
							// Store MP3 data
							++bytesProcessed;
							++byteIndex;
							++received;
							++i;
							if (received < chunk.size()) {
								*q++ = *p++;
							} else {
								abort(20001, util::csnprintf("TInetStream::onDownloadReceived() Chunk buffer too small (%/%)", received, chunk.size()));
								exit = true;
							}
						}
					}
				}
			} else {
				bytesProcessed += buffered;
			}

			// Clear buffer after all data processed!
			if (!buffer.empty()) {
				buffer.release();
			}
		}

		// Send header information
		if (!headersSent) {
			headersSent = true;
			onHeaderData();
		}

		// Send metadata information
		if (!title.empty()) {
			onMetaData(title);
		}

		// Call received data event method
		if (icecast) {
			onReceived(chunk.data(), received);
		} else {
			onReceived(data, buffered);
		}

		// Count bytes received
		bytesReceived += size;
	}
}

void TInetStream::onDownloadRead(const app::TWebClient& sender, const size_t size, const size_t position, const size_t relative) {
	if (debug) {
		std::cout << app::blue << "Internet stream \"" << sender.getBaseName() << "\" Received " \
				<< util::sizeToStr(position, 1, util::VD_SI) << " of " << util::sizeToStr(size, 1, util::VD_SI) \
				<< " (" << relative << "%)     " << app::reset << "\r";
		if (relative >= 100)
			std::cout << std::endl;
	}
}

void TInetStream::onDownloadHeader(const app::TWebClient& sender, const std::string& key, const std::string& value) {
	if (debug) std::cout << app::blue << "Internet stream \"" << sender.getBaseName() << "\" Header : " << key << " = " << value << app::reset << std::endl;

	// Wait for ICY metaint header, e.g. "icy-metaint:16000"
	if (11 == key.size()) {
		if (0 == util::strncasecmp(key, "icy-metaint", 11)) {
			icyMetaint = util::strToInt(value, 0);
			if (debug) std::cout << app::white << "Internet stream \"" << sender.getBaseName() << "\" ICY metaint = " << icyMetaint << app::reset << std::endl;
		}
	}
}

void TInetStream::onReceived(const void *const data, const size_t size) {
	if (nil != onInetStreamData && size > 0) {
		try {
			onInetStreamData(*this, data, size);
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			abort(10000, "TInetStream::onReceived() Terminated by unhandled exception \"" + sExcept + "\"");
		} catch (...) {
			abort(10001, "TInetStream::onReceived() Terminated due to unknown exception.");
		}
	}
}

void TInetStream::onHeaderData() {
	// Content-Type = audio/mpeg
	// Date = Mon, 18 Nov 2019 18:36:35 GMT
	// icy-br = 128
	// ice-audio-info = bitrate=128
	// icy-br = 128
	// icy-description = ndr_ndr1_han_mp3
	// icy-name = ndr_ndr1_han_mp3
	// icy-pub = 0
	// Server = dg-ndr-http_fra-lg_edge_299b2e07804798da6fe2250934cc435d
	// Cache-Control = no-cache, no-store
	// Access-Control-Allow-Origin = *
	// Access-Control-Allow-Headers = Origin, Accept, X-Requested-With, Content-Type
	// Access-Control-Allow-Methods = GET, OPTIONS, HEAD
	// Connection = Close
	// Expires = Mon, 26 Jul 1997 05:00:00 GMT
	// icy-metaint = 16000
	if (!info.valid) {
		info.name = convertToMultibyteString(curl.getHeaders().value("icy-name").asString());
		info.description = convertToMultibyteString(curl.getHeaders().value("icy-description").asString());
		info.bitrate = curl.getHeaders().value("icy-br").asInteger32(0);
		info.mime = curl.getHeaders().value("Content-Type").asString();
		info.valid = true;
	}

	if (nil != onInetStreamInfo) {
		try {
			onInetStreamInfo(*this, info);
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			abort(10010, "TInetStream::onHeaderData() Terminated by unhandled exception \"" + sExcept + "\"");
		} catch (...) {
			abort(10011, "TInetStream::onHeaderData() Terminated due to unknown exception.");
		}
	}
}

void TInetStream::onMetaData(const std::string& title) {
	if (nil != onInetStreamTitle) {
		try {
			onInetStreamTitle(*this, title);
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			abort(10020, "TInetStream::onMetaData() Terminated by unhandled exception \"" + sExcept + "\"");
		} catch (...) {
			abort(10021, "TInetStream::onMetaData() Terminated due to unknown exception.");
		}
	}
}

void TInetStream::onStreamError(const int error, const std::string& text, bool& terminate) {
	if (nil != onInetStreamError) {
		try {
			onInetStreamError(*this, error, text, terminate);
		} catch (...) { /* Silent exception.. */ };
	}
}

} /* namespace music */
