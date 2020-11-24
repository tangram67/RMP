/*
 * webrequest.h
 *
 *  Created on: 14.02.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef WEBREQUEST_H_
#define WEBREQUEST_H_

#include "classes.h"
#include "variant.h"
#include "semaphore.h"
#include "fileutils.h"
#include "webtypes.h"
#include "webdirectory.h"
#include "weblink.h"
#include "process.h"
#include "credentialtypes.h"
#include "microhttpd/microhttpd.h"
#include <time.h>
#include <string>
#include <mutex>
#include <map>


namespace app {


#ifdef STL_HAS_TEMPLATE_ALIAS

using TPostDataIterator = std::function<MHD_Result(PWebRequest, MHD_ValueKind, const char*, size_t, const char*, const char*, const char*, const char*, uint64_t, size_t)>;

#else

typedef std::function<MHD_Result(PWebRequest, MHD_ValueKind, const char*, size_t, const char*, const char*, const char*, const char*, uint64_t, size_t)> TPostDataIterator;

#endif


class TInode {
private:
	FILE * inode;
	TWebRangeList ranges;
	std::string boundary;
	size_t position;
	size_t size;
	ssize_t index;

	void prime();
	ssize_t read(void* buffer, const size_t size);
	std::string randomMultipartBoundary(const bool hyphen = false) const;

public:
	bool open(const std::string& fileName);
	void close();
	void clear();

	bool isOpen() const { return util::assigned(inode); };
	bool hasRanges() const { return !ranges.empty(); };
	bool isMultipart() const { return ranges.size() > 1; };
	const TWebRangeList& getRanges() const { return ranges; };
	size_t getRangeCount() const { return ranges.size(); };
	size_t getPosition() const { return position; };
	size_t getSize() const { return size; };
	PWebRange getActiveRange() const;
	const std::string& getBoundary() const { return boundary; };
	bool parseRanges(const std::string header);

	bool seek(const size_t pos);
	ssize_t read(void* buffer, const size_t position, const size_t size);

	TInode();
	virtual ~TInode();
};


class TWebRequest : public TObject {
private:
	char * callbackBuffer;
	size_t callbackSize;

	TInode currentInode;
	util::PFile parsedFile;
	app::PWebLink virtualFile;
	util::TFile const * currentFile;

	util::TBuffer postBuffer;
	util::TBuffer outputBuffer;
	util::PParserBuffer parsedBuffer;
	app::PThreadDataItem htmlPostBuffer;
	PWebSessionMap sessions;
	PWebSession session;

	std::mutex* sessionMtx;
	std::mutex* requestMtx;

	EHttpAuthType authType;
	EWebTransferMode transferMode;
	EWebPostMode postMode;
	EWebCookieMode cookieMode;

	util::TTimePart timestamp;
	size_t sessionDelta;
	int refC;
	int httpStatusCode;
	bool authenticated;
	bool finalized;
	bool secure;
	bool debug;

	mutable std::string ranges;
	mutable std::string contentType;
	mutable size_t contentSize;
	mutable bool ranged;
	mutable bool multipart;
	mutable bool xmlRequest;
	mutable bool upgradeRequest;
	mutable bool urlEncoded;
	mutable bool zipAllowed;
	mutable bool zipContent;
	mutable bool hasModifiedIf;
	mutable bool validMultipart;
	mutable bool validXMLRequest;
	mutable bool validUpgradeRequest;
	mutable bool validUrlEncoded;
	mutable bool validContentSize;
	mutable bool validContentType;
	mutable bool validZipAllowed;
	mutable bool validZipContent;
	mutable bool validModifiedIf;
	mutable bool validRange;

	std::string realm;
	TPostDataIterator postDataIterator;
	struct MHD_PostProcessor *postProcessor;
	struct MHD_Connection *connection;
	struct MHD_Response *response;
	app::TStringVector cookies;
	util::TVariantValues params;
	util::TVariantValues values;

	void prime();
	void clear();
	void release();

	void prepare(struct MHD_Connection *connection);
	void setConnection(struct MHD_Connection *value);
	void setApplicationValues(const util::TVariantValues& values);

	PWebSession findSession(struct MHD_Connection *connection);
	EWebCookieMode getCookieMode(const std::string& path);
	MHD_Result addResponseCookie(struct MHD_Response *response, const std::string& cookie, const EWebCookieMode mode);
	MHD_Result addSessionCookie(struct MHD_Response *response, const util::TTimePart age, const std::string& path = "/");
	MHD_Result addUserCookie(struct MHD_Response *response, const int level, const std::string& path = "");
	MHD_Result addLanguageCookie(struct MHD_Response *response, const std::string& path = "");
	MHD_Result addApplicationCookie(struct MHD_Response *response, const std::string& name, const std::string& value, const std::string& path = "");
	MHD_Result createResponseFromBuffer(struct MHD_Connection *connection, struct MHD_Response *& response, EHttpMethod method, EWebTransferMode mode,
			const void *const buffer, const size_t size, const util::TVariantValues& headers, const bool persist, const bool caching, const bool zipped,
			const std::string& mime, int& error);
	MHD_Result createResponseFromInode(struct MHD_Connection *connection, struct MHD_Response *& response, EHttpMethod method,
			const util::TFile& file, const bool caching, int64_t& send, int& error);
	MHD_Result buildResponseHeader(struct MHD_Connection *connection, struct MHD_Response *& response, const util::TFile* file, const app::TWebLink* link,
			const size_t size, const util::TVariantValues& headers, const EHttpMethod method, const bool caching, const bool zipped, const std::string& mime, int error);
	MHD_Result addResponseHeader(struct MHD_Response *& response, const std::string& header, const std::string& value);
	MHD_Result addResponseHeader(struct MHD_Response *& response, const char *header, const std::string& value);
	size_t buildCGIEnvironment(struct MHD_Connection *connection, EHttpMethod method, const util::TFile& file, const std::string& param, util::TEnvironment& env);
	size_t buildUriArgumentList(struct MHD_Connection *connection, EHttpMethod method, const app::PWebLink link, const std::string& url);
	size_t buildPostArgumentList(struct MHD_Connection *connection, EHttpMethod method, const std::string& url);
	size_t addUriArgumentList(const std::string& mime, const bool zipped, const bool cached);
	bool lookupMimeType(char *& data, size_t& size, std::string& mime);
	bool hasDynamicContent(const util::TFile* file, const app::TWebLink* link);
	std::string createETag(const util::TFile* file, const app::TWebLink* link, bool upper = false);
	std::string randomETag(bool upper);

	bool hasModifiedIfHeader(struct MHD_Connection *connection) const;
	bool isMultipartMessage(struct MHD_Connection *connection) const;
	bool isRangedRequest(struct MHD_Connection *connection) const;
	bool isXMLHttpRequest(struct MHD_Connection *connection) const;
	bool isUpgradeRequest(struct MHD_Connection *connection) const;
	bool isUrlEncodedMessage(struct MHD_Connection *connection) const;
	bool isZippedContent(struct MHD_Connection *connection) const;
	bool isZipAllowed(struct MHD_Connection *connection) const;
	int64_t getContentLength(struct MHD_Connection *connection) const;
	std::string getContentType(struct MHD_Connection *connection) const;
	std::string getContentEncoding(struct MHD_Connection *connection) const;
	std::string getConnectionValue(const std::string key) const;
	std::string getWebSocketKey(struct MHD_Connection *connection) const;
	std::string getWebSocketProtocol(struct MHD_Connection *connection) const;
	std::string getWebSocketVersion(struct MHD_Connection *connection) const;
	void findSessionValue(struct MHD_Connection *connection);

	MHD_Result createPostProcessor(struct MHD_Connection *connection, const std::string& url, const EHttpMethod method, const size_t size);
	void deletePostProcessor();
	void deleteOutputBuffer();
	void deleteVirtualFileBuffer();
	void deleteResponseRessources();
	int decRequestRefCount();
	int decSessionRefCount();
	int decBufferRefCount();

public:
	bool hasPostProcessor() const { return util::assigned(postProcessor); };
	bool hasURIArguments() const { return !params.empty(); };
	bool hasModifiedIfHeader() const;
	bool isZipAllowed() const;
	bool isMultipartMessage() const;
	bool isXMLHttpRequest() const;
	bool isUpgradeRequest() const;
	bool isZippedContent() const;
	bool isRangedRequest() const;
	bool isUrlEncodedMessage() const;
	bool isAuthenticated() const { return authenticated; }

	size_t getContentLength() const;
	std::string getContentType() const;
	std::string getRangeValue() const;
	EWebTransferMode getTransferMode() const { return transferMode; }
	EWebPostMode getPostMode() const { return postMode; }
	PWebSession getSession() const { return session; }
	std::string getSessionValue(const std::string& key) const;
	util::PFile getFile() const { return parsedFile; }
	int getRefCount() const { return refC; }
	util::TTimePart getTimeStamp() const { return timestamp; }
	void setTimeStamp() { timestamp = util::now(); }
	int getStatusCode() const { return httpStatusCode; };
	const util::TVariantValues& getURIArguments() const { return params; };

	util::TBuffer& getPostData() { return postBuffer; };
	void finalizePostData();

	void setSessionDelta(const size_t value) { sessionDelta = value; };
	void initialize(struct MHD_Connection *connection, const size_t sessionDelta);
	void finalize();

	MHD_Result executePostProcess(const char *upload_data, size_t *upload_data_size);

	MHD_Result sendUpgradeResponse(struct MHD_Connection *connection, MHD_UpgradeHandler method, TWebServer* owner);
	MHD_Result sendResponseFromBuffer(struct MHD_Connection *connection, EHttpMethod method, const EWebTransferMode mode,
			const void *const buffer, const size_t size, const util::TVariantValues& headers, const bool persist, const bool caching, const bool zipped,
			const std::string& mime, int& error);
	MHD_Result sendResponseFromInode(struct MHD_Connection *connection, EHttpMethod method, const util::TFile& file,
			const bool caching, int64_t& send, int& error);

	MHD_Result sendResponseFromFile(struct MHD_Connection *connection, EHttpMethod method, const EWebTransferMode mode,
			const bool useCaching, const util::PFile file, int64_t& send, int& error);
	MHD_Result sendResponseFromVirtualFile(struct MHD_Connection *connection, EHttpMethod method, const EWebTransferMode mode,
			const bool useCaching, const app::PWebLink link, const std::string& url, int64_t& send, int& error);
	MHD_Result sendResponseFromDirectory(struct MHD_Connection *connection, EHttpMethod method, const bool useCaching,
			const app::TWebDirectory& directory, const util::TFile& file, int64_t& send, int& error);

	void readUriArguments(struct MHD_Connection *connection);
	void readConnectionValues(struct MHD_Connection *connection);
	std::string getHeaderValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, const std::string& key) const;
	std::string getHeaderValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, const char* key) const;

	std::string getUserName(struct MHD_Connection *connection);
	int authUserCheck(struct MHD_Connection *connection, const TCredential& user, long int timeout);
	bool authenticate(struct MHD_Connection *connection, const TCredentialMap& users, long int timeout, int defaultlevel, int& error);
	bool authenticate(const int userlevel);

	bool loginUserName(const std::string& username, const std::string& password, const int userlevel, const bool overwrite);
	void logoffSessionUser();

	MHD_Result uriArgumentReader(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
	MHD_Result connectionValueReader(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
	MHD_Result connectionCookieReader(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
	MHD_Result connectionSessionFinder(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);

	void webSocketConnectionCallback(void *cls, struct MHD_Connection *connection, void *con_cls, const char *extra_in, size_t extra_in_size, MHD_socket sock, struct MHD_UpgradeResponseHandle *urh);
	ssize_t contentReaderCallback (void *cls, uint64_t pos, char *buf, size_t max);
	void contentReaderFreeCallback (void *cls);
	ssize_t inodeReaderCallback (void *cls, uint64_t pos, char *buf, size_t max);
	void inodeReaderFreeCallback (void *cls);
	MHD_Result postIteratorHandler (void *cls,
							 enum MHD_ValueKind kind,
							 const char *key,
							 const char *filename,
							 const char *content_type,
							 const char *transfer_encoding,
							 const char *data, uint64_t off, size_t size);

	template<typename iterator_t, typename class_t>
		inline TPostDataIterator bindIteratorHandler(iterator_t &&iterator, class_t &&owner) {
		TPostDataIterator method = std::bind<MHD_Result>(iterator, owner,
					std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,	std::placeholders::_5,
					std::placeholders::_6, std::placeholders::_7, std::placeholders::_8, std::placeholders::_9,	std::placeholders::_10);
	    	return method;
		}

	template<typename iterator_t, typename class_t>
		inline MHD_Result addPostIterator(struct MHD_Connection *connection, const std::string& url, const EHttpMethod method, const size_t size, iterator_t &&iterator, class_t &&owner) {
			this->postDataIterator = bindIteratorHandler(iterator, owner);
			return createPostProcessor(connection, url, method, size);
		}

	TWebRequest(struct MHD_Connection *connection, TWebSessionMap& sessions, std::mutex& sessionMtx, std::mutex& requestMtx, const size_t sessionDelta, const TWebConfig& config);
	virtual ~TWebRequest();
};


} /* namespace app */

#endif /* WEBREQUEST_H_ */
