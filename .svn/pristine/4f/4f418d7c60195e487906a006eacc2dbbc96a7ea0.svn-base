/*
 * webclient.h
 *
 *  Created on: 16.07.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef WEBCLIENT_H_
#define WEBCLIENT_H_

//#include <curl/curl.h>
#include "fileutils.h"
#include "webtypes.h"
#include "variant.h"
#include "tables.h"

#include "curl/curl.h"


namespace app {


class TWebClient;
class TWebBaseClient;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PWebClient = TWebClient*;
using PWebBaseClient = TWebBaseClient*;
using TWebClientWriter = size_t (*)(void *, size_t, size_t, void *);
using TWebClientRead = std::function<void(const TWebClient&, const size_t, const size_t, const size_t)>;
using TWebClientData = std::function<void(const TWebClient&, const void *const, const size_t)>;
using TWebClientTable = std::function<void(const TWebClient&, data::TTable& table)>;
using TWebClientVariant = std::function<void(const TWebClient&, util::TVariantValues& variants)>;
using TWebClientHeaderData = std::function<void(const TWebClient&, const std::string&, const std::string&)>;

#else

typedef TWebClient* PWebClient;
typedef TWebBaseClient* PWebBaseClient;
typedef size_t (*TWebClientWriter)(void *, size_t, size_t, void *);
typedef std::function<void(const TWebClient&, const size_t, const size_t, const size_t)> TWebClientRead;
typedef std::function<void(const TWebClient&, const char *, const size_t)> TWebClientData;
typedef std::function<void(const TWebClient&, data::TTable& table)> TWebClientTable;
typedef std::function<void(const TWebClient&, util::TVariantValues& variants)> TWebClientVariant;
typedef std::function<void(const TWebClient&, const std::string&, const std::string&)> TWebClientHeaderData;

#endif

enum EHttpProtocol {
	EHP_DEFAULT,
	EHP_VERSION_1,
	EHP_VERSION_2,
	EHP_VERSION_2_TLS
};


class TURLInit {
public:
	TURLInit();
	~TURLInit();
};


class TURLClient {
private:
    CURL *curl;

    void init();

public:
    void initialize();
    void finalize();

    CURL * getClient() const { return curl; };
    CURL * operator () () const { return getClient(); };

    TURLClient();
	virtual ~TURLClient();
};


class TWebBaseClient {
public:
	virtual size_t writeNullHandler(void *buffer, size_t size, size_t count) = 0;
	virtual size_t writeHeaderHandler(void *buffer, size_t size, size_t count) = 0;
	virtual size_t writeFileHandler(void *buffer, size_t size, size_t count) = 0;
	virtual size_t writeDataHandler(void *buffer, size_t size, size_t count) = 0;
	virtual size_t writeReceiveHandler(void *buffer, size_t size, size_t count) = 0;
	virtual size_t writeStringHandler(void *buffer, size_t size, size_t count) = 0;

	virtual ~TWebBaseClient() = default;
};


class TWebClient : private TWebBaseClient {
private:
	struct curl_slist *headerList;

	TWebClientRead onReadMethod;
	TWebClientData onDataMethod;
	TWebClientTable onTableMethod;
	TWebClientVariant onVariantMethod;
	TWebClientHeaderData onHeaderDataMethod;

	std::string agent;
	std::string credentials;
	std::string fileName;
	std::string baseName;
	std::string url;
	util::TBlob chunk;
	util::TBlob data;
	util::TBuffer buffer;
	util::TBaseFile file;
	util::TVariantValues rxHeaders;
	util::TStringList txHeaders;
	util::TStdioFile streamFile;
	size_t bufferSize;
	size_t currentSize;
	size_t contentSize;
	size_t relativeSize;
	size_t chunkSize;
	EHttpProtocol httpVersion;
	long connectTimeout;
	long transferTimeout;
	bool allowCompress;
	bool ignoreLength;
	CURLcode errval;
	bool processing;
	bool overwrite;
	bool follow;
	bool aborted;
	bool terminated;
	bool readSize;
	bool debug;

	void prime();
	void clear();
	void cleanup();

	bool execute(TURLClient& curl, TWebClientWriter callback, const bool requestHeaders = true);
	EWebStatusCode getLastResponseCode(TURLClient& curl);
	std::string errmsg(CURLcode error) const;
	size_t alignToContentSize();
	size_t getContentSize();

	void onRead();
	void onReceived(const void *const data, const size_t size);
	void onHeaderData(const std::string& key, const std::string& value);
	bool parseHeaderData(const std::string& header, std::string& key, std::string& value);

	size_t writeNullHandler(void *buffer, size_t size, size_t count);
	size_t writeHeaderHandler(void *buffer, size_t size, size_t count);
	size_t writeFileHandler(void *buffer, size_t size, size_t count);
	size_t writeDataHandler(void *buffer, size_t size, size_t count);
	size_t writeReceiveHandler(void *buffer, size_t size, size_t count);
	size_t writeStringHandler(void *buffer, size_t size, size_t count);

public:
	void setDebug(const bool value) { debug = value; };

	bool isValid() const { return !url.empty(); };
	bool hasData() const { return !data.empty(); };
	const util::TBlob& getData() const { return data; };

	void setURL(const std::string& url) { this->url = util::trim(url); };
	const std::string& getURL() const { return url; };
	const std::string& getBaseName() const { return baseName; };
	const std::string& getFileName() const { return fileName; };
	void setFileName(const std::string& fileName) { this->fileName = fileName; };
	void setCredentials(const std::string& credentials);
	const std::string& getCredentials() const { return credentials; };
	void addHeader(const std::string& header) { txHeaders.add(header); };
	const util::TVariantValues& getHeaders() const { return rxHeaders; };

	void setOverwite(bool value) { overwrite = value; };
	bool getOverwrite() const { return overwrite; };
	void setFollowLinks(bool value) { follow = value; };
	bool getFollowLinks() const { return follow; };
	void setReadSize(bool value) { readSize = value; };
	bool getReadSize() const { return readSize; };
	void setCompress(bool value) { allowCompress = value; };
	bool getCompress() const { return allowCompress; };
	void setIgnoreContentLength(bool value) { ignoreLength = value; };
	bool getIgnoreContentLength() const { return ignoreLength; };
	void setConnectTimeout(long value) { connectTimeout = value; };
	long getConnectTimeout() const { return connectTimeout; };
	void setTransferTimeout(long value) { transferTimeout = value; };
	long getTransferTimeout() const { return transferTimeout; };
	void setChunkSize(size_t value) { chunkSize = value; };
	size_t getChunkSize() const { return chunkSize; };
	void setBufferSize(const size_t value) { bufferSize = value; };
	size_t getBufferSize() const { return bufferSize; };
	void setHttpProtocol(const EHttpProtocol value) { httpVersion = value; };
	EHttpProtocol getHttpProtocol() const { return httpVersion; };

	CURLcode error() const { return errval; };
	bool success() const { return (CURLE_OK == error()); };
	std::string errmsg() const;

	size_t received() const { return currentSize; };

	void abort();
	bool isAborted() const { return aborted; };
	void terminate();
	bool isTerminated() const { return terminated; };
	bool isInterrupted() const { return aborted || terminated; };

	const util::TBlob& operator() () const { return getData(); };

	bool readData(const std::string& url);
	bool readData();
	bool readString(const std::string& url, std::string& str);
	bool readString(std::string& str);
	bool readVariant(const std::string& url, util::TVariantValues& variants);
	bool readVariant(util::TVariantValues& variants);
	bool readTable(const std::string& url, data::TTable& table);
	bool readTable(data::TTable& table);

	bool download(const std::string& url, const std::string& folder);
	bool download(const std::string& folder);
	bool download();
	
	bool receive(const std::string& url);
	bool receive();

	template<typename reader_t, typename class_t>
		inline void bindReadEvent(reader_t &&onRead, class_t &&owner) {
			onReadMethod = std::bind(onRead, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		}

	template<typename reader_t, typename class_t>
		inline void bindDataEvent(reader_t &&onReceived, class_t &&owner) {
			onDataMethod = std::bind(onReceived, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		}

	template<typename reader_t, typename class_t>
		inline void bindTableEvent(reader_t &&onReceived, class_t &&owner) {
			onTableMethod = std::bind(onReceived, owner, std::placeholders::_1, std::placeholders::_2);
		}

	template<typename reader_t, typename class_t>
		inline void bindVariantEvent(reader_t &&onReceived, class_t &&owner) {
			onVariantMethod = std::bind(onReceived, owner, std::placeholders::_1, std::placeholders::_2);
		}

	template<typename reader_t, typename class_t>
		inline void bindHeaderEvent(reader_t &&onHeaderData, class_t &&owner) {
			onHeaderDataMethod = std::bind(onHeaderData, owner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		}

	TWebClient();
	TWebClient(const std::string& url);
	virtual ~TWebClient();
};


static TURLInit URLInit;

} /* namespace app */

#endif /* WEBCLIENT_H_ */
