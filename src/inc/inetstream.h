/*
 * inetstream.h
 *
 *  Created on: 18.11.2019
 *      Author: dirk
 */

#ifndef INC_INETSTREAM_H_
#define INC_INETSTREAM_H_

#include "webclient.h"
#include "mimetypes.h"
#include "gcc.h"

namespace inet {

class TInetStream;

typedef struct CStreamInfo {
	std::string url;
	std::string stream;
	std::string name;
	std::string description;
	std::string mime;
	size_t bitrate;
	size_t streamed;
	bool valid;

	void init() {
		mime = DEFAULT_MIME_TYPE;
		bitrate = 0;
		streamed = 0;
		valid = false;
	}

	void clear() {
		url.clear();
		stream.clear();
		name.clear();
		description.clear();
		init();
	}

	const std::string getDescription() const {
		return (description.size() > name.size()) ? description : name;
	}

	CStreamInfo& operator= (const CStreamInfo& value) {
		url = value.url;
		stream = value.stream;
		name = value.name;
		description = value.description;
		mime = value.mime;
		bitrate = value.bitrate;
		streamed = value.streamed;
		valid = value.valid;
		return *this;
	}

	CStreamInfo() {
		init();
	}
} TStreamInfo;


#ifdef STL_HAS_TEMPLATE_ALIAS

using TInetStreamData = std::function<void(const TInetStream&, const void *const, const size_t)>;
using TInetStreamInfo = std::function<void(const TInetStream&, const TStreamInfo& info)>;
using TInetStreamTitle = std::function<void(const TInetStream&, const std::string& title)>;
using TInetStreamError = std::function<void(const TInetStream&, const int error, const std::string& text, bool& terminate)>;

#else

typedef std::function<void(const TInetStream&, const char *, const size_t)> TInetStreamData;
typedef std::function<void(const TInetStream&, const TStreamInfo& info)> TInetStreamInfo;
typedef std::function<void(const TInetStream&, const std::string& title)> TInetStreamTitle;
typedef std::function<void(const TInetStream&, const int error, const std::string& text, bool& terminate)> TInetStreamError;

#endif


class TInetStream {
private:
	bool debug;
	std::string url;
	app::TWebClient curl;
	util::TBuffer chunk;
	util::TBuffer buffer;
	TInetStreamData onInetStreamData;
	TInetStreamTitle onInetStreamTitle;
	TInetStreamInfo onInetStreamInfo;
	TInetStreamError onInetStreamError;
	TStreamInfo info;
	std::string errtxt;
	int errval;
	bool headersSent;
	size_t bytesReceived;
	size_t bytesProcessed;
	size_t icyMetaint;
	size_t byteIndex;

	void init();
	void clear();
	void abort(const int error, const std::string& text);

	bool findMetaData(const void *const data, const size_t size, size_t& pos, size_t& start, size_t& length);
	bool parseMetaData(const void *const data, const size_t pos, const size_t length, std::string& title, std::string& metadata);
	bool parseMetaDataInplace(const char *const data, const size_t length, std::string& title, std::string& metadata, size_t& broken);

	std::string convertToMultibyteString(const std::string& s);
	std::string convertToMultibyteString(const char* s, const size_t size);

	void onDownloadRead(const app::TWebClient& sender, const size_t size, const size_t position, const size_t relative);
	void onDownloadHeader(const app::TWebClient& sender, const std::string& key, const std::string& value);
	void onDownloadReceived(const app::TWebClient& sender, const void *const data, const size_t size);

	void onStreamError(const int error, const std::string& text, bool& terminate);
	void onReceived(const void *const data, const size_t size);
	void onMetaData(const std::string& title);
	void onHeaderData();

public:
	enum EPlaylistType { EPT_NONE, EPT_M3U, EPT_PLS, EPT_XSPF };

	void setDebug(const bool value);
	void setTimeout(const long value);
	void setChunkSize(const size_t value);

	int error() const { return errval; };
	std::string errmsg() const { return errtxt; };
	size_t received() const { return curl.received(); };
	bool isAborted() const { return curl.isAborted(); };
	bool isTerminated() const { return curl.isTerminated(); };
	bool hasHeaderInfo() const { return info.valid; };

	const std::string& getMimeType() const { return info.mime; };
	const std::string& getDescription() const { return info.description; };
	const std::string& getName() const { return info.name; };
	const std::string& getURL() const { return info.url; };
	const std::string& getStream() const { return info.stream; };
	size_t getBitrate() const { return info.bitrate; };

	bool receive(const std::string& URL, const std::string& fileName);
	bool receive(const std::string& URL);
	void terminate();

	template<typename reader_t, typename class_t>
		inline void bindStreamDataEvent(reader_t &&onReceived, class_t &&owner) {
			onInetStreamData = std::bind(onReceived, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		}

	template<typename info_t, typename class_t>
		inline void bindStreamInfoEvent(info_t &&onInfo, class_t &&owner) {
			onInetStreamInfo = std::bind(onInfo, owner, std::placeholders::_1, std::placeholders::_2);
		}

	template<typename title_t, typename class_t>
		inline void bindStreamTitleEvent(title_t &&onTitleData, class_t &&owner) {
			onInetStreamTitle = std::bind(onTitleData, owner, std::placeholders::_1, std::placeholders::_2);
		}

	template<typename error_t, typename class_t>
		inline void bindStreamErrorEvent(error_t &&onStreamError, class_t &&owner) {
			onInetStreamError = std::bind(onStreamError, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		}

	TInetStream();
	virtual ~TInetStream();
};

} /* namespace music */

#endif /* INC_INETSTREAM_H_ */
