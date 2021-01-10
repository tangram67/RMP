/*
 * webrequest.cpp
 *
 *  Created on: 14.02.2015
 *      Author: Dirk Brinkmeier
 */

#include <iostream>
#include <cstring>
#include "webtypes.h"
#include "webrequest.h"
#include "templates.h"
#include "exception.h"
#include "mimetypes.h"
#include "datetime.h"
#include "classes.h"
#include "convert.h"
#include "compare.h"
#include "sockets.h"
#include "bitmap.h"
#include "ASCII.h"
#include "json.h"
#include "sha.h"
#include "version.h"
#include "mimetypes.h"
#include "microhttpd/sha256.h"
#include "microhttpd/internal.h"


static const std::string OPAQUE_STRING = util::fastCreateHexStr(40);


static ssize_t contentReaderCallbackDispatcher (void *cls, uint64_t pos, char *buf, size_t max  ) {
	if (util::assigned(cls)) {
		return (static_cast<app::PWebRequest>(cls))->contentReaderCallback(cls, pos, buf, max);
	}
	return 0;
}


static void inodeReaderFreeCallbackDispatcher (void *cls) {
	if (util::assigned(cls)) {
		(static_cast<app::PWebRequest>(cls))->inodeReaderFreeCallback(cls);
	}
}


static ssize_t inodeReaderCallbackDispatcher (void *cls, uint64_t pos, char *buf, size_t max) {
	if (util::assigned(cls)) {
		return (static_cast<app::PWebRequest>(cls))->inodeReaderCallback(cls, pos, buf, max);
	}
	return 0;
}


static void contentReaderFreeCallbackDispatcher (void *cls) {
	if (util::assigned(cls)) {
		(static_cast<app::PWebRequest>(cls))->contentReaderFreeCallback(cls);
	}
}


static MHD_Result uriArgumentReaderDispatcher (void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
	if (util::assigned(cls)) {
		return (static_cast<app::PWebRequest>(cls))->uriArgumentReader(cls, kind, key, value);
	}
	return MHD_NO;
}

static MHD_Result connectionValueReaderDispatcher (void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
	if (util::assigned(cls)) {
		return (static_cast<app::PWebRequest>(cls))->connectionValueReader(cls, kind, key, value);
	}
	return MHD_NO;
}


static MHD_Result connectionSessionFinderDispatcher (void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
	if (util::assigned(cls)) {
		return (static_cast<app::PWebRequest>(cls))->connectionSessionFinder(cls, kind, key, value);
	}
	return MHD_NO;
}


static MHD_Result postIteratorDispatcher (	void *cls,
									enum MHD_ValueKind kind,
									const char *key,
									const char *filename,
									const char *content_type,
									const char *transfer_encoding,
									const char *data, uint64_t off, size_t size ) {
	if (util::assigned(cls)) {
		return (static_cast<app::PWebRequest>(cls))->postIteratorHandler(	cls,
																			kind,
																			key,
																			filename,
																			content_type,
																			transfer_encoding,
																			data, off, size );
	}
	return MHD_NO;
}


namespace app {


TWebRequest::TWebRequest(struct MHD_Connection *connection, TWebSessionMap& sessions, std::mutex& sessionMtx, std::mutex& requestMtx, const size_t sessionDelta, const TWebConfig& config) {
	prime();
	setName("TWebServer (c) db Application [SVN" + std::string(SVN_REV) + "]");
	setSessionDelta(sessionDelta);
	this->sessionMtx = &sessionMtx;
	this->requestMtx = &requestMtx;
	this->sessions = &sessions;
	this->authType = config.auth;
	this->secure = config.useHttps;
	this->realm = config.realm;
	this->debug = config.debug;
	prepare(connection);
}

TWebRequest::~TWebRequest() {
	release();
}


void TWebRequest::prime() {
	authenticated = false;
	secure = false;
	ranged = false;
	multipart = false;
	xmlRequest = false;
	upgradeRequest = false;
	urlEncoded = false;
	hasModifiedIf = false;
	zipAllowed = false;
	zipContent = false;
	contentSize = 0;
	parsedFile = nil;
	parsedBuffer = nil;
	virtualFile = nil;
	htmlPostBuffer = nil;
	currentFile = nil;
	postProcessor = nil;
	postDataIterator = nil;
	callbackBuffer = nil;
	callbackSize = 0;
	connection = nil;
	response = nil;
	session = nil;
	validRange = false;
	validMultipart = false;
	validXMLRequest = false;
	validUpgradeRequest = false;
	validUrlEncoded = false;
	validContentSize = false;
	validContentType = false;
	validModifiedIf = false;
	validZipAllowed = false;
	validZipContent = false;
	httpStatusCode = MHD_HTTP_OK;
	transferMode = WTM_DEFAULT;
	cookieMode = WCM_STRICT;
	postMode = WPM_DEFAULT;
	finalized = false;
	sessionDelta = 0;
	refC = 0;
}

void TWebRequest::clear() {
	contentType.clear();
	cookies.clear();
	params.clear();
	values.clear();
	ranges.clear();
	currentInode.close();
	release();
	prime();
}


void TWebRequest::setApplicationValues(const util::TVariantValues& values) {
	this->values = values;
}

void TWebRequest::setConnection(struct MHD_Connection *value) {
	connection = value;
}

void TWebRequest::prepare(struct MHD_Connection *connection) {
	refC = 1;
	setTimeStamp();
	setConnection(connection);
	findSession(connection);
	readConnectionValues(connection);
}

void TWebRequest::initialize(struct MHD_Connection *connection, const size_t sessionDelta) {
	clear();
	setSessionDelta(sessionDelta);
	prepare(connection);
}


void TWebRequest::finalize() {
	int rc = 0;
	if (!finalized) {

		// Decrement reference counters only once
		finalized = true;

		// Session no longer used for current request
		decSessionRefCount();

		// Decrement file buffer reference count
		decBufferRefCount();

		// Set new time stamp and decrement request reference count
		rc = decRequestRefCount();

	}
	if (rc <= 0) {
		// Release all ressources
		release();
	}
}


void TWebRequest::release() {
	// Free MHD post processor object and request data buffers
	deletePostProcessor();
	deleteOutputBuffer();
	deleteVirtualFileBuffer();
	deleteResponseRessources();
}


void TWebRequest::deleteOutputBuffer() {
	outputBuffer.clear();
}

void TWebRequest::deleteVirtualFileBuffer() {
	if (util::assigned(virtualFile)) {
		virtualFile->finalize(htmlPostBuffer);
		virtualFile = nil;
	}
}

void TWebRequest::deleteResponseRessources() {
	// Destroy local response resources
	if (util::assigned(response)) {
		MHD_destroy_response(response);
		response = nil;
	}
}

EWebCookieMode TWebRequest::getCookieMode(const std::string& path) {
	return path.empty() ? cookieMode : WCM_LAX;
}

MHD_Result TWebRequest::addSessionCookie(struct MHD_Response *response, const util::TTimePart age, const std::string& path) {
	if (!session->sid.empty()) {
		EWebCookieMode mode = WCM_STRICT; // Do NOT send session cookie to other domains...
		std::string cookie;
		if (age > 0) {
			util::TTimePart time = util::now() + age;
			std::string date = util::RFC1123DateTimeToStr(time);
			if (path.empty()) {
				cookie = util::csnprintf("%=%; expires=%; max-age=%; HttpOnly", SESSION_COOKIE, session->sid, path, date, age);
			} else {
				cookie = util::csnprintf("%=%; path=%; expires=%; max-age=%; HttpOnly", SESSION_COOKIE, session->sid, path, date, age);
			}
		} else {
			if (path.empty()) {
				cookie = util::csnprintf("%=%; HttpOnly", SESSION_COOKIE, session->sid, path);
			} else {
				cookie = util::csnprintf("%=%; path=%; HttpOnly", SESSION_COOKIE, session->sid, path);
			}
		}
		return addResponseCookie(response, cookie, mode);
	}
	return MHD_NO;
}

MHD_Result TWebRequest::addUserCookie(struct MHD_Response *response, const int level, const std::string& path) {
	if (!session->sid.empty()) {
		EWebCookieMode mode = WCM_STRICT; // Do NOT send user level cookie to other domains...
		std::string cookie;
		if (path.empty()) {
			cookie = util::csnprintf("%=%", USER_COOKIE, level);
		} else {
			cookie = util::csnprintf("%=%; path=%", USER_COOKIE, level, path);
		}
		return addResponseCookie(response, cookie, mode);
	}
	return MHD_NO;
}

MHD_Result TWebRequest::addApplicationCookie(struct MHD_Response *response, const std::string& name, const std::string& value, const std::string& path) {
	if (!session->sid.empty() && !name.empty() && !value.empty()) {
		EWebCookieMode mode = getCookieMode(path);
		std::string cookie;
		if (path.empty()) {
			cookie = util::csnprintf("%=%", name, value);
		} else {
			cookie = util::csnprintf("%=%; path=%", name, value, path);
		}
		return addResponseCookie(response, cookie, mode);
	}
	return MHD_NO;
}

MHD_Result TWebRequest::addLanguageCookie(struct MHD_Response *response, const std::string& path) {
	if (!session->sid.empty()) {
		// Add language cookie: e.g. language="de-DE"
		EWebCookieMode mode = getCookieMode(path);
		std::string cookie;
		const std::string& language = syslocale.asISO639();
		if (path.empty()) {
			cookie = util::csnprintf("%=%", LANGUAGE_COOKIE, language);
		} else {
			cookie = util::csnprintf("%=%; path=%", LANGUAGE_COOKIE, language, path);
		}
		return addResponseCookie(response, cookie, mode);
	}
	return MHD_NO;
}

MHD_Result TWebRequest::addResponseCookie(struct MHD_Response *response, const std::string& cookie, const EWebCookieMode mode) {
	if (!cookie.empty() && util::assigned(response)) {
		std::string value;
		value.reserve(cookie.size() + 30);
		value = cookie;
		switch (mode) {
			case WCM_STRICT:
				value += "; SameSite=Strict";
				break;
			case WCM_LAX:
				value += "; SameSite=Lax";
				break;
			case WCM_NONE:
				value += "; SameSite=None";
				break;
		}
		if (secure) {
			// Cookie will be transferred via HTTPS
			value += "; Secure";
		}
		return MHD_add_response_header(response, MHD_HTTP_HEADER_SET_COOKIE, value.c_str());
	}
	return MHD_NO;
}

MHD_Result TWebRequest::connectionCookieReader(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
	if (util::assigned(key) && util::assigned(value)) {
		if (strcmp(key, SESSION_COOKIE) == 0) {
			std::string s = value;
			if (!s.empty())
				cookies.push_back(s);
		}
	}
	return MHD_YES;
}


MHD_Result TWebRequest::connectionSessionFinder(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
	MHD_Result retVal = MHD_YES;
	if (!util::assigned(session)) {
		if (util::assigned(key) && util::assigned(value)) {
			std::string name(key);
			if (debug) std::cout << "TWebRequest::connectionSessionFinder() Found cookie = " << name << std::endl;
			if (strcmp(key, SESSION_COOKIE) == 0) {
				std::string uuid(value);
				if (debug) std::cout << "TWebRequest::connectionSessionFinder() Found session cookie = " << uuid << std::endl;
				if (util::isValidUUID(uuid)) {
					cookies.push_back(uuid);
					TWebSessionMap::const_iterator it = sessions->find(uuid);
					if (it != sessions->end()) {
						if (util::assigned(it->second)) {
							session = it->second;
							retVal = MHD_NO; // Session found, abort iteration
							if (debug) std::cout << "TWebRequest::connectionSessionFinder() Valid session cookie = " << uuid << std::endl;
						}
					}
				}
			}
		}
	} else {
		retVal = MHD_NO;
		if (debug) std::cout << "TWebRequest::connectionSessionFinder() Session " << session->sid << " already assigned. " << std::endl;
	}
	return retVal;
}


void TWebRequest::findSessionValue(struct MHD_Connection *connection) {
	session = nil;
	cookies.clear();
	MHD_get_connection_values(connection, MHD_COOKIE_KIND, connectionSessionFinderDispatcher, this); // @suppress("Invalid arguments")
}


PWebSession TWebRequest::findSession(struct MHD_Connection *connection) {
	// Lock access to sessions map for writing
	std::lock_guard<std::mutex> lock(*sessionMtx);
	if (debug) std::cout << app::yellow << "TWebRequest::findSession() Find session..." << app::reset << std::endl;
	findSessionValue(connection);

	// Reuse anonimous session
	//  - unused
	//  - no active post process
	//  - usage count < 2
	//  - older than x seconds
	if (!util::assigned(session)) {

		// Get client address
		std::string addr;
		if (util::assigned(connection)) {
			if (util::assigned(connection->addr)) {
				addr = inet::inetAddrToStr(connection->addr);
			}
		}

		// Use minimal session age of 2 seconds
		util::TTimePart dt = std::max((util::TTimePart)sessionDelta, (util::TTimePart)2);
		util::TTimePart ts = util::now() - dt;

		TWebSessionMap::const_iterator it = sessions->begin();
		while (it != sessions->end()) {
			PWebSession o = it->second;
			if (util::assigned(o)) {
				// Check if session idle and not used since given time
				// --> Also check for same client IP to prevent idle sessions assigned to a different client
				if (!o->busy() && o->refC <= 0 /*&& o->useC < 2*/ && addr == o->client && o->timestamp < ts) {
					o->reset();
					session = o;
					break;
				}
			}
			it++;
		}
	}

	// Create new session
	if (!util::assigned(session)) {
		// Check for session cookie given by client
		// --> Take over session ID of first cookie from client request
		std::string sid;
		if (!cookies.empty())
			sid = cookies[util::pred(cookies.size())];
		session = new TWebSession(sid);
		sessions->insert(TWebSessionItem(session->sid, session));
		if (debug) {
			std::cout << app::red << "TWebRequest::findSession() Create new session cookie = " << session->sid << app::reset << std::endl;
		}
	}

	// Set default session values
	session->setConnectionValues(connection);

	// Set timestamp and reference counter
	session->setTimeStamp();
	session->refC++;
	session->useC++;

	return session;
}


int TWebRequest::decSessionRefCount() {
	std::lock_guard<std::mutex> lock(*sessionMtx);
	if (util::assigned(session)) {
		session->setTimeStamp();
		if (session->refC)
			--session->refC;
		return session->refC;
	}
	return 0;
}


int TWebRequest::decRequestRefCount() {
	std::lock_guard<std::mutex> lock(*requestMtx);
	setTimeStamp();
	if (refC)
		--refC;
	return refC;
}

int TWebRequest::decBufferRefCount() {
	if (util::assigned(parsedFile)) {
		if (parsedFile->hasParser()) {
			return parsedFile->getParser()->decBufferRefCount();
		}
	}
	return 0;
}


void TWebRequest::readConnectionValues(struct MHD_Connection *connection) {
	if (util::assigned(session)) {
		if (!session->valuesRead) {
			session->values.clear();
			MHD_get_connection_values(connection, MHD_HEADER_KIND, connectionValueReaderDispatcher, this); // @suppress("Invalid arguments")
			session->valuesRead = true;
		}
	}
}


std::string TWebRequest::getConnectionValue(const std::string key) const {
	if (!session->values.empty()) {
		TValueMap::const_iterator it = session->values.find(key);
		if (it != session->values.end())
			return it->second;
	}
	return std::string();
}


void TWebRequest::readUriArguments(struct MHD_Connection *connection) {
	if (params.empty()) {
		MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentReaderDispatcher, this); // @suppress("Invalid arguments")
	}
}


MHD_Result TWebRequest::connectionValueReader(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
	if (util::assigned(key) && util::assigned(value)) {
		size_t length = strnlen(key, HTTP_MAX_TOKEN_LENGTH);
		if (length > 0 && length < HTTP_MAX_TOKEN_LENGTH) {
			session->values.insert(TValueMapItem(std::string(key, length), std::string(value)));
			if (0 == strcmp(key, USER_AGENT)) {
				session->userAgent = guessUserAgent(value);
				session->setSessionValue("SESSION_USER_AGENT", userAgentToStr(session->userAgent));
			}
		}
	}
	return MHD_YES;
}


MHD_Result TWebRequest::uriArgumentReader(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
	if (util::assigned(key) && util::assigned(value)) {
		size_t length = strnlen(key, HTTP_MAX_TOKEN_LENGTH);
		if (length > 0) {
			params.add(std::string(key, length), std::string(value));
		}
	}
	return MHD_YES;
}


std::string TWebRequest::getContentEncoding(struct MHD_Connection *connection) const {
	std::string s = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_ENCODING);
	if (!s.empty()) {
		std::string::size_type p = s.find_first_of(';');
		if (p != std::string::npos)
			s.erase(p, std::string::npos);
		if (!s.empty())
			std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	}
	return s;
}

std::string TWebRequest::getWebSocketKey(struct MHD_Connection *connection) const {
	return util::trim(getHeaderValue(connection, MHD_HEADER_KIND, HTTP_WEB_SOCKET_KEY));
}

std::string TWebRequest::getWebSocketProtocol(struct MHD_Connection *connection) const {
	return util::trim(getHeaderValue(connection, MHD_HEADER_KIND, HTTP_WEB_SOCKET_PROTOCOL));
}

std::string TWebRequest::getWebSocketVersion(struct MHD_Connection *connection) const {
	return util::trim(getHeaderValue(connection, MHD_HEADER_KIND, HTTP_WEB_SOCKET_VERSION));
}

std::string TWebRequest::getContentType(struct MHD_Connection *connection) const {
	std::string s = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
	if (!s.empty()) {
		std::string::size_type p = s.find_first_of(';');
		if (p != std::string::npos)
			s.erase(p, std::string::npos);
		if (!s.empty())
			std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	}
	return s;
}

int64_t TWebRequest::getContentLength(struct MHD_Connection *connection) const {
	int64_t retVal = 0;
	const char *const p = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH);
	if (util::assigned(p)) {
		char* q;
		errno = EXIT_SUCCESS;
		retVal = strtol(p, &q, 0);
		if (errno != EXIT_SUCCESS || p == q)
			retVal = 0;
	}
	return retVal;
}

bool TWebRequest::isMultipartMessage(struct MHD_Connection *connection) const {
	bool retVal = false;
	const char* value = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
	if (util::assigned(value)) {
		retVal = (0 == strncasecmp(value, MULTIPART_FORM_DATA, MULTIPART_FORM_DATA_SIZE));
	}
	return retVal;
}

bool TWebRequest::isRangedRequest(struct MHD_Connection *connection) const {
	bool retVal = false;
	const char* value = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_RANGE);
	if (util::assigned(value)) {
		size_t length = strnlen(value, HTTP_MAX_RANGE_VALUE_LENGTH);
		if (length > 0 && length < HTTP_MAX_RANGE_VALUE_LENGTH) {
			ranges = util::trim(std::string(value, length));
			retVal = !ranges.empty();
		}
	}
	return retVal;
}

bool TWebRequest::isUrlEncodedMessage(struct MHD_Connection *connection) const {
	bool retVal = false;
	const char* value = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
	if (util::assigned(value)) {
		retVal = (0 == strncasecmp(value, ENCODING_FORM_URLENCODED, ENCODING_FORM_URLENCODED_SIZE));
	}
	return retVal;
}


bool TWebRequest::isXMLHttpRequest(struct MHD_Connection *connection) const {
	bool retVal = false;
	const char* value = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, XML_HTTP_REQUEST_HEADER);
	if (util::assigned(value)) {
		retVal = (0 == strncasecmp(value, XML_HTTP_REQUEST, XML_HTTP_REQUEST_SIZE));
	}
	return retVal;
}


bool TWebRequest::isUpgradeRequest(struct MHD_Connection *connection) const {
	bool retVal = false;

	// Check for common upgrade request
	// ================================
	//  GET /index.html HTTP/1.1
	//  Host: www.example.com
	//  Connection: upgrade
	//  Upgrade: websocket

	// Check header "Connection: Upgrade"
	const char* value = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONNECTION);
	if (util::assigned(value)) {
		if (util::assigned(strcasestr(value, UPGRADE_HTTP_REQUEST))) {

			// Check header "Upgrade: websocket"
			value = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_UPGRADE);
			if (util::assigned(value)) {
				retVal = (0 == strncasecmp(value, UPGRADE_HTTP_PROTOCOL, UPGRADE_HTTP_PROTOCOL_SIZE));
			}
		}
	}
	return retVal;
}


bool TWebRequest::hasModifiedIfHeader(struct MHD_Connection *connection) const {
	return util::assigned(MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_IF_MODIFIED_SINCE));
}


bool TWebRequest::isZipAllowed(struct MHD_Connection *connection) const {
	std::string accept = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_ACCEPT_ENCODING);
	return (accept.find("gzip") != std::string::npos);
}

bool TWebRequest::isZippedContent(struct MHD_Connection *connection) const {
	std::string zipped = getContentEncoding(connection);
	return (zipped.find("gzip") != std::string::npos);
}

bool TWebRequest::isMultipartMessage() const {
	if (!validMultipart && util::assigned(connection)) {
		multipart = isMultipartMessage(connection);
		validMultipart = true;
	}
	return multipart;
}

bool TWebRequest::isRangedRequest() const {
	if (!validRange && util::assigned(connection)) {
		ranged = isRangedRequest(connection);
		validRange = true;
	}
	return ranged;
}

bool TWebRequest::isXMLHttpRequest() const {
	if (!validXMLRequest && util::assigned(connection)) {
		xmlRequest = isXMLHttpRequest(connection);
		validXMLRequest = true;
	}
	return xmlRequest;
}

bool TWebRequest::isUpgradeRequest() const {
	if (!validUpgradeRequest && util::assigned(connection)) {
		upgradeRequest = isUpgradeRequest(connection);
		validUpgradeRequest = true;
	}
	return upgradeRequest;
}

bool TWebRequest::isUrlEncodedMessage() const {
	if (!validUrlEncoded && util::assigned(connection)) {
		urlEncoded = isUrlEncodedMessage(connection);
		validUrlEncoded = true;
	}
	return urlEncoded;
}


bool TWebRequest::hasModifiedIfHeader() const {
	if (!validModifiedIf && util::assigned(connection)) {
		hasModifiedIf = hasModifiedIfHeader(connection);
		validModifiedIf = true;
	}
	return hasModifiedIf;
}


bool TWebRequest::isZippedContent() const {
	if (!validZipContent && util::assigned(connection)) {
		zipContent = isZippedContent(connection);
		validZipContent = true;
	}
	return zipContent;
}


bool TWebRequest::isZipAllowed() const {
	if (!validZipAllowed && util::assigned(connection)) {
		zipAllowed = isZipAllowed(connection);
		validZipAllowed = true;
	}
	return zipAllowed;
}

size_t TWebRequest::getContentLength() const {
	if (!validContentSize && util::assigned(connection)) {
		contentSize = getContentLength(connection);
		validContentSize = true;
	}
	return contentSize;
}

std::string TWebRequest::getContentType() const {
	if (!validContentType && util::assigned(connection)) {
		contentType = getContentType(connection);
		validContentType = true;
	}
	return contentType;
}

std::string TWebRequest::getRangeValue() const {
	if (isRangedRequest())
		return ranges;
	return std::string();
}


std::string TWebRequest::getHeaderValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, const std::string& key) const {
	if (!key.empty())
		return getHeaderValue(connection, kind, key.c_str());
	return std::string();
}


std::string TWebRequest::getHeaderValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, const char* key) const {
	const char* value = MHD_lookup_connection_value(connection, kind, key);
	if (util::assigned(value))
		return std::string(value);
	return std::string();
}


MHD_Result TWebRequest::createPostProcessor(struct MHD_Connection *connection, const std::string& url, const EHttpMethod method, const size_t size) {
	if (isMultipartMessage())
		getContentLength();
	if (!hasPostProcessor()) {
		postProcessor = MHD_create_post_processor(connection, ITERATOR_BUFFER_SIZE, postIteratorDispatcher, this); // @suppress("Invalid arguments")
		if (util::assigned(postProcessor)) {
			// Use default MHD post processor handling for standard file upload
			postMode = WPM_HTML;
		} else {
			// Handle other content like JSON, XML or whatever was uploaded...
			// --> Store data in post data buffer with given content size
			size_t bufsize, length;
			bufsize = length = getContentLength();
			if (size > length)
				bufsize = size;
			if (bufsize <= 0)
				bufsize = 1024;
			postBuffer.clear();
			postBuffer.reserve(bufsize, false);
			postMode = WPM_DATA;

			// Store request information to handle data afterwards
			buildPostArgumentList(connection, method, url);
		}
	}
	return MHD_YES;
}


void TWebRequest::deletePostProcessor() {
	if (hasPostProcessor()) {
	    MHD_destroy_post_processor(postProcessor);
	}
	postProcessor = nil;
	postBuffer.clear();
}


MHD_Result TWebRequest::executePostProcess(const char *upload_data, size_t *upload_data_size) {
	if (WPM_HTML == postMode) {
		if (hasPostProcessor()) {
			if (util::assigned(session) && isMultipartMessage())
				session->upload.contentSize = getContentLength();
			return MHD_post_process(postProcessor, upload_data, *upload_data_size);
		}
	}
	if (WPM_DATA == postMode) {
		if (*upload_data_size > 0) {
			postBuffer.append(upload_data, *upload_data_size);
			return MHD_YES;
		}
	}
	return MHD_NO;
}

void TWebRequest::finalizePostData() {
	if (WPM_DATA == postMode) {
		postBuffer.clear();
		postMode = WPM_DEFAULT;
	}
}

std::string TWebRequest::getUserName(struct MHD_Connection *connection) {
	std::string user;
	char *puser = MHD_digest_auth_get_username(connection);
	if (util::assigned(puser)) {
		try {
			user = puser;
		} catch (...) {};
		MHD_free(puser);
	}
	return user;
}

int TWebRequest::authUserCheck(struct MHD_Connection *connection, const TCredential& user, long int timeout) {
	if (!user.username.empty() && !user.realm.empty()) {
		if (authType == HAT_DIGEST_MD5) {
			if (!user.digestMD5.empty()) {
				unsigned char digest[MHD_MD5_DIGEST_SIZE];
				if (util::TBinaryConvert::hexToBin(user.digestMD5, digest, MHD_MD5_DIGEST_SIZE))
					return MHD_digest_auth_check_digest2(connection, user.realm.c_str(), user.username.c_str(), digest, MHD_MD5_DIGEST_SIZE, timeout, MHD_DIGEST_ALG_MD5);
			}
		} else if (authType == HAT_DIGEST_SHA256) {
			if (!user.digestSHA256.empty()) {
				unsigned char digest[SHA256_DIGEST_SIZE];
				if (util::TBinaryConvert::hexToBin(user.digestSHA256, digest, SHA256_DIGEST_SIZE))
					return MHD_digest_auth_check_digest2(connection, user.realm.c_str(), user.username.c_str(), digest, SHA256_DIGEST_SIZE, timeout, MHD_DIGEST_ALG_SHA256);
			}
		}
	}
	return MHD_NO;
}

bool TWebRequest::authenticate(struct MHD_Connection *connection, const TCredentialMap& users, long int timeout, int defaultlevel, int& error) {
	bool debugger = debug;
	//debugger = true;

	// Is current request already authenticated?
	if (authenticated)
		return true;

	// timeout == 0 --> session valid for ever
	bool toCheck = timeout > 0;

	// Is sessions is already authenticated?
	if (util::assigned(session)) {
		if (session->authenticated) {
			if (toCheck) {
				util::TTimePart now = util::now();
				if ((now - session->timeout) > timeout) {
					// Session expired
					if (debugger) std::cout << "TWebRequest::authenticate() Session no longer valid." << std::endl;
					logoffSessionUser();
				} else {
					// Session still in use
					session->timeout = now;
					if (debugger) std::cout << "TWebRequest::authenticate(0) Session still authenticated." << std::endl;
					return true;
				}
			} else {
				// Valid until doomsday
				if (debugger) std::cout << "TWebRequest::authenticate(1) Session still authenticated." << std::endl;
				return true;
			}
		}
	}

	// Set digest timeout to some valid value:
	// If timeout set, nonce is valid for 90% of session timeout time
	// If not set, assume 1 day
	timeout = ( toCheck ? (timeout * 9 / 10) : (60 * 60 * 24) );
	error = MHD_HTTP_FORBIDDEN;
	bool retVal = false;
	bool logoff = false;

	// Check for forced user log off
	if (util::assigned(session)) {
		if (session->logoff) {
			if (debugger) std::cout << "TWebRequest::authenticate() Logoff current user." << std::endl;
			session->logoff = false;
			logoff = true;
		}
	}

	// Get user name from client connection
	if (debugger) {
		const std::string username = getUserName(connection);
		std::cout << "TWebRequest::authenticate() Connection username = " << username << std::endl;
		std::cout << "TWebRequest::authenticate() Session Username    = " << (util::assigned(session) ? session->username : std::string("none")) << std::endl;
	}

	// Has user logged off?
	if (!logoff) {
		bool fallback = true;
		const std::string username = getUserName(connection);
		if (!username.empty()) {

			TCredentialMap::const_iterator it = users.find(username);
			if (it != users.end()) {
				const TCredential user = it->second;

				// Check user credentials
				// @ret = MHD_YES --> access granted
				// @ret = MHD_NO --> access denied
				// @ret = MHD_INVALID_NONCE --> handshake failed, nonce invalid
				error = authUserCheck(connection, user, timeout);
				retVal = (error == MHD_YES);

				// Set error to "Forbidden"
				if (error == MHD_NO)
					error = MHD_HTTP_FORBIDDEN;

				// Store login in session
				if (retVal) {
					if (loginUserName(user.username, user.password, user.level, false)) {
						if (debugger) std::cout << "TWebRequest::authenticate(1) Authenticated given user." << std::endl;
						fallback = false;
					}
				}
			}
		}

		if (fallback && defaultlevel > 0) {
			if (loginUserName("anonimous", "*", defaultlevel, true)) {
				if (debugger) std::cout << "TWebRequest::authenticate(2) Fallback after authentication failure." << std::endl;
				error = MHD_YES;
				retVal = true;
			}
		}

	} else {
		if (util::assigned(session)) {

			// Change to default anonimous user
			if (defaultlevel > 0 && session->username != "anonimous") {
				if (loginUserName("anonimous", "*", defaultlevel, true)) {
					if (debugger) std::cout << "TWebRequest::authenticate(3) Log on anonimous user." << std::endl;
					error = MHD_YES;
					retVal = true;
				}
			} else {
				// Session logged off
				if (debugger) std::cout << "TWebRequest::authenticate(4) Clear user data." << std::endl;
				logoffSessionUser();
			}
		}
	}

	authenticated = retVal;
	return authenticated;
}

bool TWebRequest::authenticate(const int userlevel) {

	// Is current request already authenticated?
	if (authenticated)
		return true;

	// Set default values for given user level
	authenticated = loginUserName("default", "*", userlevel, true);

	return authenticated;
}


bool TWebRequest::loginUserName(const std::string& username, const std::string& password, const int userlevel, const bool overwrite) {
	bool debugger = debug;
	bool authenticated = false;
	if (util::assigned(session)) {
		if (!session->authenticated || overwrite) {
			app::TLockGuard<app::TMutex> lock(session->userMtx);
			session->authenticated = true;
			session->username = username;
			session->password = password;
			session->userlevel = userlevel;
			session->valuesRead = false;
			session->setTimeOut();
			authenticated = session->authenticated;
		}
	}
	if (authenticated) {
		// Store authentication values
		session->setUserValues(username, userlevel, authenticated);
		if (debugger) std::cout << "TWebRequest::loginUserName() Login user <" << username << ">" << std::endl;
	}
	return authenticated;
}


void TWebRequest::logoffSessionUser() {
	bool debugger = debug;
	bool logoff = false;
	//debugger = true;
	if (util::assigned(session)) {
		// Session expired
		app::TLockGuard<app::TMutex> lock(session->userMtx);
		session->userlevel = 0;
		session->authenticated = false;
		session->valuesRead = false;
		logoff = true;

		// Do NOT (!) clear user name, still needed to compare against anonimous user name
		// session->username.clear();
		// session->password.clear();
	}
	if (logoff) {
		session->clearUserValues();
		if (debugger) std::cout << "TWebRequest::logoffSessionUser() Logged off session user <" << session->username << ">" << std::endl;
	}
}


MHD_Result TWebRequest::sendResponseFromFile(struct MHD_Connection *connection, EHttpMethod method, const EWebTransferMode mode,
		const bool useCaching, const util::PFile file, int64_t& send, int& error) {
	MHD_Result retVal = MHD_NO;
	const char* data = nil;;
	bool zipped = false;
	bool processed = false;
	bool caching = useCaching;
	bool persistent = true;
	std::string mime;
	size_t size = 0;
	bool debugger = debug;
	//debugger = true;

	if (util::assigned(file)) {

		// Get mime type from file properties
		mime = file->getMime();

		if (debugger)
			std::cout << "sendResponseFromFile[Start](" << file->getName() << ") Is CGI = " << file->isCGI() << ", Error = " << error << std::endl;

		// Check for fixed AJAX response file
		if (0 == util::strncasecmp(file->getURL(), AJAX_RESPONSE_FILE, file->getURL().size())) {
			if (debugger)
				std::cout << "Disable caching for AJAX response file \"" << file->getURL() << "\"" << std::endl;
			caching = false;
		}

		// Check for CGI (Common Gateway Interface)
		// --> execute script or binary file and return output as HTTP response
		// --> CGI file need execution rights for current user!
		if (file->isCGI()) {
			// Persistent buffer
			outputBuffer.clear();
			int result;

			// Read URI parameter for "value"
			readUriArguments(connection);
			if (debugger) {
				std::cout << "URI arguments for \"" << file->getURL() << "\"" << std::endl;
				params.debugOutput("Argument", "  ");
				std::cout << std::endl;
			}

			// Build CGI environment
			util::TEnvironment env;
			buildCGIEnvironment(connection, method, *file, params.asText(), env);

			if (debugger)
				std::cout << "sendResponseFromFile[CGI](" << file->getName() << ")" << std::endl;

			// Execute CGI script
			try {
				bool r = util::executeCommand(file->getFile(), params, file->getPath(), env, outputBuffer, result, 10);
				if (r && outputBuffer.size() > 0) {
					char* p;
					size_t n;

					// Use local (persistent) buffer, no caching!
					data = p = outputBuffer.data();
					size = n = outputBuffer.size();
					caching = false;
					persistent = false;
					
					// Cut output buffer, if Content-Type header set
					if (lookupMimeType(p, n, mime)) {
						data = p;
						size = n;
					}

				} else {
					// No output from CGI script!
					error = MHD_HTTP_NO_CONTENT;
					if (debugger)
						std::cout << "    sendResponseFromFile[CGI](" << file->getName() << ") --> No content returned!" << std::endl;
				}
			} catch (const std::exception& e)	{
				std::string sExcept = e.what();
				std::string sName = "TWebRequest::executeCommand(" + file->getName() + ")";
				std::string sText = "Exception in " + sName + "\n" + sExcept + "\n";
				// TODO Exception logging...
				//sysdat.obj.exceptionLog->write(sText);
				std::cout << sText << std::endl;
				error = MHD_HTTP_INTERNAL_SERVER_ERROR;
			} catch (...)	{
				std::string sName = "TWebRequest::executeCommand(" + file->getName() + ")";
				std::string sText = "Exception in " + sName;
				// TODO Exception logging...
				//sysdat.obj.exceptionLog->write(sText);
				std::cout << sText << std::endl;
				error = MHD_HTTP_INTERNAL_SERVER_ERROR;
			}

			// Set found in any case even output missing from CGI or if execution failed!
			// --> prevent looking further for other possibilities
			// Also set global buffer to be freed afterwards
			processed = true;
		}

		// Use cache handling via entity tag
		if (!processed && useCaching && !file->hasToken()) {
			std::string ETag = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_IF_NONE_MATCH);
			if (!ETag.empty()) {
				// Check if header request is for the same file:
				// Ignore ETag if file has parser token values
				// --> File content may have been changed!
				if (debugger)
					std::cout << "sendResponseFromFile[ETag](" << file->getName() << ") --> Entity tag from client = <" << ETag << ">" << std::endl;
				if (ETag == file->getETag()) {
					if (util::isMemberOf(method, HTTP_GET, HTTP_POST)) {
						if (debugger) {
							std::cout << "    Entity tag from client for <" << file->getName() << "> fits file tag." << std::endl;
							std::cout << "        Use cached file by entity tag [HTTP_NOT_MODIFIED/304]" << std::endl;
						}
						error = MHD_HTTP_NOT_MODIFIED;
					} else {
						// See http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
						// --> 14.26 If-None-Match: For all other request methods, the server MUST respond with a status of 412 (Precondition Failed)
						error = MHD_HTTP_PRECONDITION_FAILED;
					}
					processed = true;
				}
			} else
				if (debugger)
					std::cout << "sendResponseFromFile[ETag](" << file->getName() << ") --> No Entity tag from client!" << std::endl;
		}

		// Check preconditions and look for "If-Modified-Since" header in request
		if (!processed && useCaching && !file->hasToken() && hasModifiedIfHeader()) {
			std::string modified = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_IF_MODIFIED_SINCE);
			if (!modified.empty()) {
				// Check if file has been modified after given UTC timestamp
				if (debugger)
					std::cout << "sendResponseFromFile[Modified](" << file->getName() << ") file time = " << file->getTime().asRFC1123() << " --> 'If-Modified-Since' header = <" << modified << ">" << std::endl;
				util::TTimePart modTime = util::RFC1123ToDateTime(modified);
				if (file->getTime() <= modTime) {
					if (debugger) {
						std::cout << "    Modifiy file time for <" << file->getName() << "> is older or equal to requested client timestamp." << std::endl;
						std::cout << "        Use cached file by timestamp [HTTP_NOT_MODIFIED/304]" << std::endl;
					}
					error = MHD_HTTP_NOT_MODIFIED;
					processed = true;
				}
			} else
				if (debugger)
					std::cout << "sendResponseFromFile[Modified](" << file->getName() << ") --> No 'If-Modified-Since' header from client!" << std::endl;
		}

		// Use parser buffer if file has token
		// Disable caching because buffer might have changed by parser...
		if (!processed && file->hasToken()) {
			// File has token --> send parser buffer
			// Buffer is rebuild if needed by getParserData()
			util::PParserBuffer o = file->getParserData();
			data = o->buffer;
			size = o->size;
			caching = false;
			processed = true;
			parsedBuffer = o;
			if (debugger)
				std::cout << "sendResponseFromFile[Token](" << file->getName() << ") Parser buffer size = " << size << std::endl;
		}

		// Use compressed buffer if present and client accepts it
		if (!processed && !file->hasToken() && file->isZipped()) {
			if (isZipAllowed()) {
				// File has no token --> send zipped buffer
				data = file->getZippedData();
				size = file->getZippedSize();
				zipped = true;
				processed = true;
				if (debugger)
					std::cout << "sendResponseFromFile[GZip](" << file->getName() << ") Zipped buffer size = " << size << std::endl;
			}
		}

		// Use plain file content buffer
		if (!processed) {
			// File has no token --> send file buffer
			data = file->getData();
			size = file->getSize();
			if (debugger)
				std::cout << "sendResponseFromFile[Identity](" << file->getName() << ") File buffer size = " << size << std::endl;
		}

		send += size;
		parsedFile = file;

		// Send response (body data may be empty)
		util::TVariantValues headers;
		retVal = sendResponseFromBuffer(connection, method, mode, data, size, headers, persistent, caching, zipped, mime, error);
		if (debugger)
			std::cout << "sendResponseFromFile[End](" << file->getName() << ") Result = " << retVal << ", Error = " << error << std::endl;
	}
	return retVal;
}


MHD_Result TWebRequest::sendResponseFromVirtualFile(struct MHD_Connection *connection, EHttpMethod method, const EWebTransferMode mode,
		const bool useCaching, const app::PWebLink link, const std::string& url, int64_t& send, int& error) {
	MHD_Result retVal = MHD_NO;
	const void* data = nil;;
	bool persistent = true;
	bool failed = false;
	size_t size = 0;
	htmlPostBuffer = nil;
	bool debugger = debug;
	//debugger = true;

	if (util::assigned(link) && util::assigned(session)) {
		util::TVariantValues headers;

		// Increment usage count
		link->incRequested();

		// Set caching and ZIP properties
		bool zipped = isZipAllowed();
		bool cached = (link->useCache()) ? true : useCaching;
		std::string mime = link->getMime();

		// Read all URI arguments
		readUriArguments(connection);
		buildUriArgumentList(connection, method, link, url);
		if (debugger) {
			std::cout << "URI arguments for \"" << link->getURL() << "\"" << std::endl;
			params.debugOutput("Argument", "  ");
			std::cout << std::endl;
		}

		if (debugger)
			std::cout << "sendResponseFromVirtualFile[Start] \"" << link->getURL() << "\" Error = " << error << std::endl;

		// Is content encoding "gzip" allowed?
		if (debug) {
			if (isZipAllowed())
				std::cout << "sendResponseFromVirtualFile[Zip] \"" << link->getURL() << "\" --> Zip is allowed!" << std::endl;
			else
				std::cout << "sendResponseFromVirtualFile[Zip] \"" << link->getURL() << "\" --> Zip is NOT allowed!" << std::endl;
		}

		// Get data from request of virtual link
		try {
			{
				// Lock session matrix value access
				app::TReadWriteGuard<app::TReadWriteLock> lock(session->matrixLck, RWL_READ);
				htmlPostBuffer = link->getData(data, size, params, session->getSessionValuesWithNolock(), headers, zipped, cached, error, true);
			}
			if (util::assigned(htmlPostBuffer)) {
				if (debugger)
					std::cout << "sendResponseFromVirtualFile[Data] Data for link \"" << link->getURL() << "\" received, size = " << size << std::endl;
			} else {
				if (debugger)
					std::cout << "sendResponseFromVirtualFile[Data] No data for link \"" << link->getURL() << "\" received." << std::endl;
				error = MHD_HTTP_NOT_FOUND;
				failed = true;
			}
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::cout << "sendResponseFromVirtualFile[Data] Exception \"" << sExcept << "\"" << std::endl;
			error = MHD_HTTP_INTERNAL_SERVER_ERROR;
			failed = true;
		} catch (...)	{
			std::cout << "sendResponseFromVirtualFile[Data] Unknown exception" << std::endl;
			error = MHD_HTTP_INTERNAL_SERVER_ERROR;
			failed = true;
		}

		// Set data properties and check for empty JSON table data
		send += size;
		if (!util::assigned(htmlPostBuffer)) {
			// Send empty JSON table if JSON data was requested
			// --> detected by mime type
			if (link->isJSON() && !failed) {
				data = util::JSON_EMPTY_TABLE.c_str();
				size = util::JSON_EMPTY_TABLE.size();
			} else {
				data = nil;
				size = 0;
			}
		} else {
			// Virtual file buffer send
			virtualFile = link;
		}

		// Mime type may empty for virtual folders
		// --> Derive mime type from current requested URL
		if (mime.empty()) {
			std::string ext = util::fileExt(url);
			if (!ext.empty()) {
				mime = util::getMimeType(ext);
			}
		}

		// Add parameters
		addUriArgumentList(mime, zipped, cached);

		// Send response (body data may be empty)
		retVal = sendResponseFromBuffer(connection, method, mode, data, size, headers, persistent, cached, zipped, mime, error);

	}

	return retVal;
}


MHD_Result TWebRequest::sendResponseFromDirectory(struct MHD_Connection *connection, EHttpMethod method, const bool useCaching,
		const app::TWebDirectory& directory, const util::TFile& file, int64_t& send, int& error) {
	MHD_Result retVal = MHD_NO;
	bool cached = false;
	bool processed = false;
	const char* data = nil;
	size_t size = 0;
	bool debugger = debug;
	//debugger = true;

	// Increment usage count
	directory.incRequested();

	// Get mime type from file properties
	std::string mime = file.getMime();
	bool ranged = isRangedRequest();;

	if (debugger) {
		std::cout << "sendResponseFromDirectory[Start](" << file.getName() << ") Error = " << error << std::endl;
		if (ranged)
			std::cout << "sendResponseFromDirectory[Ranged](" << file.getName() << ") Ranges = " << getRangeValue() << std::endl;
	}

	// [1] Check for CGI (Common Gateway Interface)
	// --> execute script or binary file and return output as HTTP response
	if (!processed && file.isCGI() && directory.execCGI()) {

		// Persistent buffer
		outputBuffer.clear();
		int result;

		// Read URI parameter for "value"
		readUriArguments(connection);
		if (debugger) {
			std::cout << "URI arguments for \"" << file.getURL() << "\"" << std::endl;
			params.debugOutput("Argument", "  ");
			std::cout << std::endl;
		}

		// Build CGI environment
		util::TEnvironment env;
		buildCGIEnvironment(connection, method, file, params.asText(), env);

		if (debugger)
			std::cout << "sendResponseFromDirectory[CGI](" << file.getName() << ")" << std::endl;

		// Execute CGI script
		try {
			bool r = util::executeCommand(file.getFile(), params, file.getPath(), env, outputBuffer, result, 10);
			if (r && outputBuffer.size() > 0) {
				char* p;
				size_t n;

				// Use local (persistent) buffer, no caching!
				data = p = outputBuffer.data();
				size = n = outputBuffer.size();

				// Cut output buffer, if Content-Type header set
				if (lookupMimeType(p, n, mime)) {
					data = p;
					size = n;
				}

			} else {
				// No output from CGI script!
				error = MHD_HTTP_NO_CONTENT;
				if (debugger)
					std::cout << "    sendResponseFromDirectory[CGI](" << file.getName() << ") --> No content returned!" << std::endl;
			}

			// Send output of CGI execution as response
			if (debugger)
				std::cout << "sendResponseFromDirectory[CGI](" << file.getName() << ") Error = " << error << std::endl;
			util::TVariantValues headers;
			retVal = sendResponseFromBuffer(connection, method, WTM_ASYNC, data, size, headers, false, false, false, mime, error);

		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::string sName = "TWebRequest::executeCommand(" + file.getName() + ")";
			std::string sText = "Exception in " + sName + "\n" + sExcept + "\n";
			// TODO Exception logging...
			//sysdat.obj.exceptionLog->write(sText);
			std::cout << sText << std::endl;
			error = MHD_HTTP_INTERNAL_SERVER_ERROR;
		} catch (...)	{
			std::string sName = "TWebRequest::executeCommand(" + file.getName() + ")";
			std::string sText = "Exception in " + sName;
			// TODO Exception logging...
			//sysdat.obj.exceptionLog->write(sText);
			std::cout << sText << std::endl;
			error = MHD_HTTP_INTERNAL_SERVER_ERROR;
		}

		// Set found in any case even output missing from CGI or if execution failed!
		// --> prevent looking further for other possibilities
		// Also set global buffer to be freed afterwards
		send += outputBuffer.size();
		processed = true;

	} // if (!processed && file.isCGI() && directory.execCGI())

	// Reset pointer to current file on leaving current scope (on next terminating "}")
	// --> Sender may destroy or modify object after calling this method!
	// Used in buildResponseHeader() for header information
	util::TPointerGuard<util::TFile> pg(&currentFile);
	currentFile = &file;

	// [2] Check to send cached response for file if requested by client
	if (!processed) {
		// Use cache handling via entity tag
		if (useCaching) {
			std::string ETag = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_IF_NONE_MATCH);
			if (!ETag.empty()) {
				// Check if header request is for the same file:
				// Ignore ETag if file has parser token values
				// --> File content may have been changed!
				if (debugger)
					std::cout << "sendResponseFromDirectory[ETag](" << file.getName() << ") --> Entity tag from client = <" << ETag << ">" << std::endl;
				if (ETag == file.getETag()) {
					if (util::isMemberOf(method, HTTP_GET, HTTP_POST)) {
						if (debugger) {
							std::cout << "    Entity tag from client for <" << file.getName() << "> fits file tag." << std::endl;
							std::cout << "        Use cached file by entity tag [HTTP_NOT_MODIFIED/304]" << std::endl;
						}
						cached = true;
						error = MHD_HTTP_NOT_MODIFIED;
					} else {
						// See http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
						// --> 14.26 If-None-Match: For all other request methods, the server MUST respond with a status of 412 (Precondition Failed)
						cached = true;
						error = MHD_HTTP_PRECONDITION_FAILED;
					}
				}
			} else {
				if (debugger)
					std::cout << "sendResponseFromDirectory[ETag](" << file.getName() << ") --> No Entity tag from client!" << std::endl;
			}
		}

		// Check preconditions and look for "If-Modified-Since" header in request
		if (!cached && useCaching && hasModifiedIfHeader()) {
			std::string modified = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_IF_MODIFIED_SINCE);
			if (!modified.empty()) {
				// Check if file has been modified after given UTC timestamp
				if (debugger)
					std::cout << "sendResponseFromDirectory[Modified](" << file.getName() << ") file time = " << file.getTime().asRFC1123() << " --> 'If-Modified-Since' header = <" << modified << ">" << std::endl;
				util::TTimePart modTime = util::RFC1123ToDateTime(modified);
				if (file.getTime() <= modTime) {
					if (debugger) {
						std::cout << "    Modifiy file time for <" << file.getName() << "> is older or equal to requested client timestamp." << std::endl;
						std::cout << "        Use cached file by timestamp [HTTP_NOT_MODIFIED/304]" << std::endl;
					}
					cached = true;
					error = MHD_HTTP_NOT_MODIFIED;
				}
			} else {
				if (debugger)
					std::cout << "sendResponseFromDirectory[Modified](" << file.getName() << ") --> No 'If-Modified-Since' header from client!" << std::endl;
			}
		}

		// Check if cached response requested
		if (cached) {
			// Send cached response (body data is empty)
			if (debug)
				std::cout << "sendResponseFromDirectory[Cache](" << file.getName() << ") Error = " << error << std::endl;
			util::TVariantValues headers;
			retVal = sendResponseFromBuffer(connection, method, WTM_SYNC, nil, 0, headers, false, useCaching, false, mime, error);
			processed = true;
		}

	} // if (!processed)

	// [3] Check for JPG scaling on non ranged requests
	if (!processed && !ranged && file.isJPG() && (directory.scaleJPG() > 0)) {
		try {
			std::string fileName = file.getFile();
			int modified = 0;

			// Load and scale JPG file
			util::TJpeg jpg(fileName);
			jpg.loadFromFile();
			if (jpg.hasImage()) {

				// Read Exif orientation property from file
				if (directory.useExif()) {
					int orientation;
					if (jpg.realign(orientation))
						++modified;
					if (debugger)
						std::cout << "sendResponseFromDirectory[JPEG](" << file.getName() << ") Orientation = " << orientation << std::endl;
				}

				// Scale picture if needed to given height
				size_t dimension = directory.scaleJPG();
				util::TRGBSize ratio = (util::TRGBSize)dimension * (util::TRGBSize)100 / jpg.height();
				if ((ratio < (util::TRGBSize)90) || (ratio > (util::TRGBSize)110)) {
					if (dimension < jpg.height()) {
						jpg.setScalingMethod(util::ESM_DITHER);
						if (jpg.resizeY(dimension)) {
							jpg.contrast();
						}
					} else {
						jpg.setScalingMethod(util::ESM_BILINEAR);
						if (jpg.resizeY(dimension)) {
							jpg.blur(0.9);
							jpg.contrast();
						}
					}
					++modified;
				}

				// Send rotated and/or scaled image
				if (modified > 0) {
					char* image = nil;
					util::TArrayBufferGuard<char> bg(&image);
					jpg.encode(image, size);
					if (size > 0) {
						if (debugger)
							std::cout << "sendResponseFromDirectory[JPEG](" << file.getName() << ", ratio = " << ratio << "%) Error = " << error << std::endl;
						util::TVariantValues headers;
						retVal = sendResponseFromBuffer(connection, method, WTM_SYNC, image, size, headers, false, useCaching, false, mime, error);
						send += size;
						processed = true;
					}
				}

			}
		} catch (const std::exception& e)	{
			std::string sExcept = e.what();
			std::cout << "sendResponseFromDirectory[JPEG] Exception \"" << sExcept << "\"" << std::endl;
			error = MHD_HTTP_INTERNAL_SERVER_ERROR;
		} catch (...)	{
			std::cout << "sendResponseFromDirectory[JPEG] Unknown exception." << std::endl;
			error = MHD_HTTP_INTERNAL_SERVER_ERROR;
		}
	}

	// [4] Send plain file response from inode
	if (!processed) {
		if (debugger)
			std::cout << "sendResponseFromDirectory[Inode](" << file.getName() << ") Error = " << error << std::endl;
		retVal = sendResponseFromInode(connection, method, file, useCaching, send, error);
		processed = true;
	}

	return retVal;
}


size_t TWebRequest::buildUriArgumentList(struct MHD_Connection *connection, EHttpMethod method, const app::PWebLink link, const std::string& url) {
	// Add standard parameters to URI query
	params.add(URI_CLIENT_ADDRESS, inet::inetAddrToStr(connection->addr));
	params.add(URI_REQUEST_METHOD, httpMethodToStr(method));
	params.add(URI_REQUEST_ACTION, "URL-DATA");
	params.add(URI_REQUEST_LINK, link->getURL());
	params.add(URI_REQUEST_URL, url);
	return params.size();
}

size_t TWebRequest::buildPostArgumentList(struct MHD_Connection *connection, EHttpMethod method, const std::string& url) {
	// Add standard parameters to URI query
	params.add(URI_CLIENT_ADDRESS, inet::inetAddrToStr(connection->addr));
	params.add(URI_REQUEST_METHOD, httpMethodToStr(method));
	params.add(URI_REQUEST_ACTION, "POST-DATA");
	params.add(URI_REQUEST_URL, url);
	return params.size();
}

size_t TWebRequest::addUriArgumentList(const std::string& mime, const bool zipped, const bool cached)
{
	// Add standard parameters to URI query
	if (!mime.empty()) params.add(URI_MIME_TYPE, mime);
	params.add(URI_RESPONSE_ZIPPED, zipped);
	params.add(URI_RESPONSE_CACHED, cached);
	return params.size();
}


MHD_Result TWebRequest::sendResponseFromInode(struct MHD_Connection *connection, EHttpMethod method, const util::TFile& file,
		const bool caching, int64_t& send, int& error) {
	// Remember http status code
	httpStatusCode = error;
	int stale = (error == MHD_INVALID_NONCE) ? MHD_YES : MHD_NO;

	// Create web server response for given file btw. inode
	MHD_Result retVal = createResponseFromInode(connection, response, method, file, caching, send, error);
	if (retVal == MHD_YES) {
		switch (error) {
			case MHD_INVALID_NONCE:
			case MHD_HTTP_FORBIDDEN:
				if (authType == HAT_DIGEST_SHA256) {
					retVal = MHD_queue_auth_fail_response2 (connection, realm.c_str() , OPAQUE_STRING.c_str(), response, stale, MHD_DIGEST_ALG_SHA256);
				} else if (authType == HAT_DIGEST_MD5) {
					retVal = MHD_queue_auth_fail_response2 (connection, realm.c_str() , OPAQUE_STRING.c_str(), response, stale, MHD_DIGEST_ALG_MD5);
				} else {
					retVal = MHD_NO;
				}
				break;
			default:
				retVal = MHD_queue_response(connection, error, response);
				break;
		}
	}

	return retVal;
}


MHD_Result TWebRequest::sendResponseFromBuffer(struct MHD_Connection *connection, EHttpMethod method, EWebTransferMode mode,
		const void *const buffer, const size_t size, const util::TVariantValues& headers, const bool persist, const bool caching, const bool zipped, const std::string& mime, int& error) {
	if (debug)
		std::cout << "sendResponseFromBuffer[Start] Size = " << size << ", Error = " << error << std::endl;

	// Force synchronous mode for empty body
	EWebTransferMode wtm = mode;
	if (wtm == WTM_ASYNC) {
		if (!util::assigned(buffer) || (size <= 0))
			wtm = WTM_SYNC;
	}
	
	// Remember http status code
	httpStatusCode = error;
	int stale = (error == MHD_INVALID_NONCE) ? MHD_YES : MHD_NO;

	// Disable caching on empty body
	// if not "not modified" header
	bool cache = caching;
	if (cache && error != MHD_HTTP_NOT_MODIFIED) {
		if (!util::assigned(buffer) || (size <= 0))
			cache = false;
	}

	// Create web server response from given data buffer
	MHD_Result retVal = createResponseFromBuffer(connection, response, method, wtm, buffer, size, headers, persist, cache, zipped, mime, error);
	if (retVal == MHD_YES) {
		std::string method;
		switch (error) {
			case MHD_INVALID_NONCE:
			case MHD_HTTP_FORBIDDEN:
				if (authType == HAT_DIGEST_SHA256) {
					if (debug) method = "MHD_queue_auth_fail_response2::SHA256";
					retVal = MHD_queue_auth_fail_response2 (connection, realm.c_str() , OPAQUE_STRING.c_str(), response, stale, MHD_DIGEST_ALG_SHA256);
				} else if (authType == HAT_DIGEST_MD5) {
					if (debug) method = "MHD_queue_auth_fail_response2::MD5";
					retVal = MHD_queue_auth_fail_response2 (connection, realm.c_str() , OPAQUE_STRING.c_str(), response, stale, MHD_DIGEST_ALG_MD5);
				} else {
					if (debug) method = "None";
					retVal = MHD_NO;
				}
				break;
			default:
				if (debug) method = "MHD_queue_response";
				retVal = MHD_queue_response(connection, error, response);
				break;
		}
		if (debug)
			std::cout << "sendResponseFromBuffer[Queue] Method \"" + method + "\", result = " << retVal << std::endl;
	}

	if (debug)
		std::cout << "sendResponseFromBuffer[End] Result = " << retVal << std::endl;
	return retVal;
}


MHD_Result TWebRequest::sendUpgradeResponse(struct MHD_Connection *connection, MHD_UpgradeHandler method, TWebServer* owner) {
	MHD_Result retVal = MHD_NO;
	if (util::assigned(method) && util::assigned(owner)) {

		// Create response for upgrade via connection dispatcher callback
		response = MHD_create_response_for_upgrade(method, owner);

		// Get web socket key from client and calculate SHA1 hash as Base64 for response
		// See https://tools.ietf.org/html/rfc6455
		std::string key = getWebSocketKey(connection);
		if (!key.empty()) {
			std::string value = key + HTTP_WEB_SOCKET_GUID;
			util::TSHA1 sha1;
			sha1.setFormat(util::ERT_BASE64);
			std::string header = sha1.getSHA1(value);
			if (!header.empty()) {
				MHD_add_response_header (response, HTTP_WEB_SOCKET_ACCEPT, header.c_str());
			}

			// Add websocker version if present in request header
			std::string version = getWebSocketVersion(connection);
			if (!version.empty()) {
				// If version set PING <--> PONG must be implemented, otherwise Firefox will drop the connection after PING timed out!
				MHD_add_response_header (response, HTTP_WEB_SOCKET_VERSION, version.c_str());
			}

			// Add "chat" potokoll if present in request header
			std::string proto = getWebSocketProtocol(connection);
			if (!proto.empty()) {
				MHD_add_response_header (response, HTTP_WEB_SOCKET_PROTOCOL, "chat");
			}

			// Add response upgrade header
			MHD_add_response_header (response, MHD_HTTP_HEADER_UPGRADE, "websocket");

			// Send "header only" response, no data to cleanup afterwards...
			retVal = MHD_queue_response (connection, MHD_HTTP_SWITCHING_PROTOCOLS, response);
		}
	}
	return retVal;
}


MHD_Result TWebRequest::createResponseFromBuffer(struct MHD_Connection *connection, struct MHD_Response *& response, EHttpMethod method, const EWebTransferMode mode,
		const void *const buffer, const size_t size, const util::TVariantValues& headers, const bool persist, const bool caching, const bool zipped, const std::string& mime, int& error) {
	MHD_Result retVal = MHD_YES;
	if (debug)
		std::cout << "createResponseFromBuffer[Start] Size = " << size << ", Error = " << error << std::endl;

	// Volatile buffer can only be transfered synchronous
	// --> Force MHD to copy content buffer!
	MHD_ResponseMemoryMode rmm;
	if (persist) {
		// Application holds persistent data buffer
		transferMode = mode;
		rmm = MHD_RESPMEM_PERSISTENT;
	} else {
		// MHD copies volatile (non persistent) data buffer
		transferMode = WTM_SYNC;
		rmm = MHD_RESPMEM_MUST_COPY;
	}

	// Check for header request
	if (method != HTTP_HEAD && method != HTTP_OPTIONS && size > 0 && util::assigned(buffer)) {

		// Check for synchronous or callback response
		switch (transferMode) {
			case WTM_SYNC:
				callbackBuffer = nil;
				callbackSize = 0;
				response = MHD_create_response_from_buffer(size, (void*)buffer, rmm);
				if (debug)
					std::cout << "createResponseFromBuffer[Sync] Response = " << util::assigned(response) << std::endl;
				break;

			case WTM_ASYNC:
				callbackBuffer = (char*)buffer;
				callbackSize = size;
				response = MHD_create_response_from_callback (size,
															  RESPONSE_BLOCK_SIZE,
															  &contentReaderCallbackDispatcher,
															  this,
															  &contentReaderFreeCallbackDispatcher);
				if (debug)
					std::cout << "createResponseFromBuffer[Async] Response = " << util::assigned(response) << std::endl;
				break;
		}

	} else {
		// HEAD request: Prepare header only response with expected content size
		// Body must be empty, see http://www.w3.org/Protocols/rfc2616/rfc2616-sec9.html
		if (method == HTTP_HEAD || method == HTTP_OPTIONS) {
			error = MHD_HTTP_NO_CONTENT;
		}
		callbackBuffer = nil;
		callbackSize = 0;
		transferMode = WTM_SYNC;
		response = MHD_create_response_from_buffer(0, nil, MHD_RESPMEM_PERSISTENT);
		if (debug)
			std::cout << "createResponseFromBuffer[Buffer] Response = " << util::assigned(response) << std::endl;

	}

	// Check for valid response
	if (!util::assigned(response)) {
		error = MHD_HTTP_INTERNAL_SERVER_ERROR;
		retVal = MHD_NO;
	}

	// Check which file to use for header information
	util::TFile const * file = nil;
	if (util::assigned(parsedFile))
		file = parsedFile;
	else if (util::assigned(currentFile))
		file = currentFile;

	// Add header information and session cookie
	if (retVal == MHD_YES)
		retVal = buildResponseHeader(connection, response, file, virtualFile, size, headers, method, caching, zipped, mime, error);

	if (debug)
		std::cout << "createResponseFromBuffer[End] Result = " << retVal << std::endl;
	return retVal;
}


MHD_Result TWebRequest::createResponseFromInode(struct MHD_Connection *connection, struct MHD_Response *& response, EHttpMethod method,
		const util::TFile& file, const bool caching, int64_t& send, int& error) {
	MHD_Result retVal = MHD_YES;
	PWebRange range = nil;
	size_t size = 0;
	bool ok = true;
	bool changed = false;

	// Check for ranged request
	if (isRangedRequest()) {

		// Get first range from request header
		std::string ranges = getRangeValue();
		if (currentInode.parseRanges(ranges)) {
			// Respond to range request with 206 = MHD_HTTP_PARTIAL_CONTENT
			error = MHD_HTTP_PARTIAL_CONTENT;
		}

		// Multipart ranges are not supported for now...
		if (currentInode.isMultipart()) {
			// Alternative: Return whole ressouce with MHD_HTTP_OK
			error = MHD_HTTP_RANGE_NOT_SATISFIABLE;
			retVal = MHD_NO;
			ok = false;
		}

		// Check for valid range request
		if (ok && currentInode.hasRanges()) {
			range = currentInode.getActiveRange();
			if (util::assigned(range)) {
				size_t last = util::pred(file.getSize());

				// Check current range
				if (range->start > last) ok = false;
				if (ok && range->end != std::string::npos) {
					if (range->end > last) ok = false;
					if (range->start >= range->end) ok = false;
				}

				// Range checks failed!
				if (!ok) {
					error = MHD_HTTP_RANGE_NOT_SATISFIABLE;
					retVal = MHD_NO;
					ok = false;
				}

				// Check for non partial content if ressource had changed
				if (ok) {
					std::string header = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_IF_RANGE);
					if (!header.empty()) {
						std::string time = file.getTime().asRFC1123();
						std::string etag = file.getETag();
						if (0 != util::strcasecmp(header, etag) && 0 != util::strcasecmp(header, time)) {
						// if (header != etag && header != time) {
							// Return full ressource
							range->start = 0;
							range->end = last;
							error = MHD_HTTP_OK;
							changed = true;
						}
					}
				}

				// Reduce open range request ("0-" or "5436-") to some handy size
				if (ok && !changed) {
					size_t offs = std::min(FULL_RANGE_MAX_SIZE, std::max(FULL_RANGE_MIN_SIZE, last / 32));
					if (file.isMime(MP3_MIME_TYPE))
						offs = offs * 2 / 3; // MP3 files are smaller...
					if (range->isOpenRange()) {
						// Set end for calculated chunk size
						range->end = std::min(range->start + offs, last);
					} else {
						// Do not send full range to end of file even if requested
						if (range->end == last && range->start >= 0) {
							if (range->end > range->start) {
								size_t diff = range->end - range->start;
								if (diff > offs) {
									range->end = std::min(range->start + offs, last);
								}
							}
						}
					}
				}

			}
		}

	}

	// Send inode data
	if (ok) {

		// Check for header request
		if (util::isMemberOf(method, HTTP_GET, HTTP_POST)) {

			// Open file for further processing of direct data transfer
			currentInode.open(file.getFile());
			if (currentInode.isOpen()) {

				// Send asynchronous file response for range or native file
				if (util::assigned(range)) {
					size_t start = range->start;
					size_t end = start;
					if (range->end == std::string::npos) {
						end = util::pred(file.getSize());
					} else {
						end = std::min(range->end, util::pred(file.getSize()));
					}
					size = end - start + 1;
				} else {
					size = file.getSize();
				}
				response = MHD_create_response_from_callback (	size,
																RESPONSE_BLOCK_SIZE,
																&inodeReaderCallbackDispatcher,
																this,
																&inodeReaderFreeCallbackDispatcher );
			} else {
				error = MHD_HTTP_NOT_FOUND;
				retVal = MHD_NO;
			}

		} else {
			if (method == HTTP_HEAD || method == HTTP_OPTIONS) {
				// HEAD request: Prepare header only response with expected content size
				// Body must be empty, see http://www.w3.org/Protocols/rfc2616/rfc2616-sec9.html
				if (method == HTTP_PUT || method == HTTP_OPTIONS) {
					error = MHD_HTTP_NO_CONTENT;
				}
				callbackBuffer = nil;
				callbackSize = 0;
				transferMode = WTM_SYNC;
				size = 0;
				response = MHD_create_response_from_buffer(size, nil, MHD_RESPMEM_PERSISTENT);
			} else {
				error = MHD_HTTP_METHOD_NOT_ALLOWED;
				retVal = MHD_NO;
			}
		}

	}

	// Check for valid response
	if (!util::assigned(response) && MHD_YES == retVal) {
		error = MHD_HTTP_INTERNAL_SERVER_ERROR;
		retVal = MHD_NO;
	}

	// Add header information and session cookie
	if (retVal == MHD_YES) {
		util::TVariantValues headers;
		retVal = buildResponseHeader(connection, response, &file, nil, size, headers, method, caching, false, file.getMime(), error);
		send += size;
	}

	if (debug)
		std::cout << "createResponseFromInode[Build](" << file.getName() << ") Size = " << size << " Error = " << error << std::endl;

	return retVal;

}


MHD_Result TWebRequest::buildResponseHeader(struct MHD_Connection *connection, struct MHD_Response *& response,
		const util::TFile* file, const app::TWebLink* link, const size_t size, const util::TVariantValues& headers,
		const EHttpMethod method, const bool caching, const bool zipped, const std::string& mime, int error) {
	MHD_Result retVal = MHD_YES;
	bool addCachingHeaders = true;
	util::TTimePart t, now = util::now();
	std::string s, date = util::RFC1123DateTimeToStr(now);
	if (debug)
		std::cout << "buildResponseHeader[Start] File = " << util::assigned(file) << ", Link = " << util::assigned(link) << ", Error = " << error << std::endl;

	// Add header information and session cookie
	// See http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
	if (util::assigned(response)) {

		// Do not add cookies on OPTIONS request
		if (method != HTTP_OPTIONS) {

			// Add session cookie for client request identification
			if (retVal == MHD_YES) {
				// Set expire time if caching disabled
				// util::TTimePart age = caching ? 0 : COOKIE_EXPIRE_TIME;
				util::TTimePart age = COOKIE_MAX_AGE;
				retVal = addSessionCookie(response, age);
			}

			// Add session cookie for user athentication level
			if (retVal == MHD_YES) {
				int level = 0;
				if (session->authenticated)
					level = session->userlevel;
				retVal = addUserCookie(response, level);
			}

			// Add language cookie: e.g. language="de-DE"
			if (retVal == MHD_YES) {
				retVal = addLanguageCookie(response);
			}

			// Add application defined cookies
			if (retVal == MHD_YES) {
				util::TVariantValues cookies;
				if (session->getCookieValues(cookies)) {
					retVal = addApplicationCookie(response, APPLICATION_COOKIE, std::to_string((size_u)cookies.size()));
					if (retVal == MHD_YES) {
						for (size_t i=0; i<cookies.size(); ++i) {
							const std::string& name = cookies.variant(i).name();
							const std::string& value = cookies.value(i).asString();
							retVal = addApplicationCookie(response, name, value);
							if (retVal != MHD_YES)
								break;
						}
					}
				} else {
					retVal = addApplicationCookie(response, APPLICATION_COOKIE, "0");
				}
			}

			// Add application headers to response
			if (!headers.empty()) {
				util::TVariantList::const_iterator it = headers.begin();
				while (it != headers.end()) {
					const std::string& key = (*it)->name();
					const std::string& value = (*it)->value().asString();
					addResponseHeader(response, key, value);
					++it;
				}
			}

		}

		// Add allowed options (all suitable methods for now...)
		if (retVal == MHD_YES && method == HTTP_OPTIONS) {
			std::string options = util::cprintf("%s, %s, %s, %s, %s, %s, %s, %s",
					MHD_HTTP_METHOD_OPTIONS,
					MHD_HTTP_METHOD_GET,
					MHD_HTTP_METHOD_HEAD,
					MHD_HTTP_METHOD_POST,
					MHD_HTTP_METHOD_PUT,
					MHD_HTTP_METHOD_DELETE,
					MHD_HTTP_METHOD_PATCH,
					MHD_HTTP_METHOD_SUBSCRIBE);
			retVal = addResponseHeader(response, MHD_HTTP_HEADER_ACCEPT, options);
		}

		// Add current timestamp
		if (retVal == MHD_YES) {
			retVal = addResponseHeader(response, MHD_HTTP_HEADER_DATE, date);
		}

		// Add ranges headers
		if (retVal == MHD_YES) {
			if (currentInode.isOpen()) {
				// Allow range requests for native inode response
				s = "bytes";
			} else {
				s = "none";
			}
			retVal = addResponseHeader(response, MHD_HTTP_HEADER_ACCEPT_RANGES, s);
			if (error == MHD_HTTP_RANGE_NOT_SATISFIABLE) {
				// Return erroneous content range
				// e.g. "Content-Range: */0" or "Content-Range: */230"
				s = "bytes */" + std::to_string((size_u)currentInode.getPosition());
				retVal = addResponseHeader(response, MHD_HTTP_HEADER_CONTENT_RANGE, s);
			} else {
				// Return requested content range
				// e.g. "Content-Range: 0-1000/2300"
				if (currentInode.isOpen()) {
					PWebRange o = currentInode.getActiveRange();
					if (util::assigned(o)) {
						if (o->isFullRange()) {
							s = util::csnprintf("bytes 0-%/%", (size - 1), currentInode.getSize());
						} else {
							size_t start = o->start != std::string::npos ? o->start : 0;
							size_t end = o->end != std::string::npos ? o->end : (currentInode.getSize() - 1);
							s = util::csnprintf("bytes %-%/%", start, end, currentInode.getSize());
						}
						retVal = addResponseHeader(response, MHD_HTTP_HEADER_CONTENT_RANGE, s);
					}
				}
			}
		}

		// For some reason Firefox intermittent drops zipped images when "Content-Type" is set to "image/..."
		if (retVal == MHD_YES) {
			bool image = util::strcasestr(mime, "image");
			if (currentInode.isMultipart() || !(WUA_FIREFOX == session->userAgent && image && zipped)) {
				s = mime;
				bool multimedia = image || util::strcasestr(mime, "audio") || util::strcasestr(mime, "video");
				if (!multimedia && currentInode.isOpen()) {
					// Add charset to non multimedia plain inode response
					s += "; charset=UTF-8";
				}
				if (currentInode.isMultipart()) {
					s += "; boundary=" + currentInode.getBoundary();
				}
				retVal = addResponseHeader(response, MHD_HTTP_HEADER_CONTENT_TYPE, s);
			}
		}

		// Set "Content-Encoding" to appropriate type, only "gzip" supported
		if (retVal == MHD_YES && zipped && size) {
			s = "gzip";
			retVal = addResponseHeader(response, MHD_HTTP_HEADER_CONTENT_ENCODING, s);
		}

		// Send "Vary" header for "Accept-Encoding" if encoding might be changed between requests
		// and server has requested "Accept-Encoding", same procedure for "Mofified-If-Sinze" et al.
		// http://tools.ietf.org/html/rfc7231#section-7.1.4
		// https://www.fastly.com/blog/best-practices-for-using-the-vary-header
		// https://blogs.msdn.microsoft.com/ieinternals/2009/06/17/vary-with-care/
		if (retVal == MHD_YES) {
			std::string header, key;

			// Check for dynamically changing content
			bool vary = hasDynamicContent(file, link);
			if (vary) {
				// Use "Vary: Accept-Encoding" for zipped content only
				s = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_ACCEPT_ENCODING);
				if (!s.empty()) {
					if (zipped) {
						header = MHD_HTTP_HEADER_ACCEPT_ENCODING;
						key = MHD_HTTP_HEADER_ACCEPT_ENCODING "; w=\"gzip\"";
					} else {
						key = MHD_HTTP_HEADER_ACCEPT_ENCODING "; w=\"identity\"";
					}
				}
				s = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_IF_MODIFIED_SINCE);
				if (!s.empty()) {
					if (!header.empty()) header += ", ";
					header += MHD_HTTP_HEADER_IF_MODIFIED_SINCE;
					if (!key.empty()) key += ", ";
					key += MHD_HTTP_HEADER_IF_MODIFIED_SINCE;
				}
				s = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_IF_NONE_MATCH);
				if (!s.empty()) {
					if (!header.empty()) header += ", ";
					header += MHD_HTTP_HEADER_IF_NONE_MATCH;
					if (!key.empty()) key += ", ";
					key += MHD_HTTP_HEADER_IF_NONE_MATCH;
				}
				s = getHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_LAST_MODIFIED);
				if (!s.empty()) {
					if (!header.empty()) header += ", ";
					header += MHD_HTTP_HEADER_LAST_MODIFIED;
					if (!key.empty()) key += ", ";
					key += MHD_HTTP_HEADER_IF_NONE_MATCH;
				}
				if (!header.empty())
					retVal = addResponseHeader(response, MHD_HTTP_HEADER_VARY, header);
				if (!key.empty())
					retVal = addResponseHeader(response, "Key", key);

			}
		}

		//
		// See https://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html (14.29 Last-Modified)
		//
		// The exact meaning of this header field depends on the implementation of the origin server and the nature of the original resource.
		// For files, it may be just the file system last-modified time. For entities with dynamically included parts,
		// it may be the most recent of the set of last-modify times for its component parts. For database gateways,
		// it may be the last-update time stamp of the record. For virtual objects, it may be the last time the internal state changed.
		//
		// An origin server MUST NOT send a Last-Modified date which is later than the server's time of message origination.
		// In such cases, where the resource's last modification would indicate some time in the future,
		// the server MUST replace that date with the message origination date.
		//
		// An origin server SHOULD obtain the Last-Modified value of the entity
		// as close as possible to the time that it generates the Date value of its response.
		// This allows a recipient to make an accurate assessment of the entity's modification time,
		// especially if the entity changes near the time that the response is generated.
		//
		// HTTP/1.1 servers SHOULD send Last-Modified whenever feasible.
		//
		if (retVal == MHD_YES) { // && hasModifiedIfHeader()) {
			s.clear();
			if (s.empty() && util::assigned(file)) {
				s = (file->hasToken() || file->isCGI()) ? date : file->getTime().asRFC1123();
			}
			if (s.empty() && util::assigned(link))
				s = util::RFC1123DateTimeToStr(link->getTimeStamp());
			if (s.empty())
				s = date;
			retVal = addResponseHeader(response, MHD_HTTP_HEADER_LAST_MODIFIED, s);
		}

		// Allow chunk content transfer handled internally by MHD!
		// See http://www.jmarshall.com/easy/http/#http1.1c2
		// if (retVal == MHD_YES && size) {
		//	 s = "identity";
        //     retVal = addResponseHeader(response, MHD_HTTP_HEADER_TRANSFER_ENCODING, s);
		// }

		// Set "Content_Length" to the size of the transfer buffer, no header sizes taken in account (?)
		// ATTENTION: libmicrohhtpd will reject MHD_HTTP_HEADER_CONTENT_LENGTH up from Version 0.9.60
		if (MHD_VERSION < 0x00096100 && retVal == MHD_YES && size) {
			s = util::cprintf("%ld" , size);
			retVal = addResponseHeader(response, MHD_HTTP_HEADER_CONTENT_LENGTH, s);
		}

		// Send information about sender...
		if (retVal == MHD_YES) {
			retVal = addResponseHeader(response, MHD_HTTP_HEADER_SERVER, name);
		}

		// Set "Validator Header Fields" as needed
		if (addCachingHeaders) {

			if (caching) {

				if (retVal == MHD_YES && assigned(file)) {
					s = file->getETag();
					retVal = addResponseHeader(response, MHD_HTTP_HEADER_ETAG, s);
				}

				if (retVal == MHD_YES) {
					s = util::cprintf("public, max-age=%d", HEADER_EXPIRE_TIME);
					retVal = addResponseHeader(response, MHD_HTTP_HEADER_CACHE_CONTROL, s);
				}

				if (retVal == MHD_YES) {
					if (mime == HTML_MIME_TYPE || mime == CSS_MIME_TYPE || mime == JAVA_MIME_TYPE)
						t = now + HEADER_EXPIRE_TIME; // 1 day (standard value)
					else
						t = now + 30 * HEADER_EXPIRE_TIME; // 1 month
					s = util::RFC1123DateTimeToStr(t);
					retVal = addResponseHeader(response, MHD_HTTP_HEADER_EXPIRES, s);
				}

			} else { // if (caching)

				if (retVal == MHD_YES) {
					s = createETag(file, link);
					retVal = addResponseHeader(response, MHD_HTTP_HEADER_ETAG, s);
				}

				if (retVal == MHD_YES) {
					s = "private, no-cache, no-store, must-revalidate, proxy-revalidate, max-age=0";
					retVal = addResponseHeader(response, MHD_HTTP_HEADER_CACHE_CONTROL, s);
				}

				if (retVal == MHD_YES) {
					t = now - (365 * HEADER_EXPIRE_TIME); // Expired one year ago
					s = util::RFC1123DateTimeToStr(t);
					retVal = addResponseHeader(response, MHD_HTTP_HEADER_EXPIRES, s);
				}

			} // if (caching)

		} // if (addCachingHeaders)

	} // if (util::assigned(response))

	if (debug)
		std::cout << "buildResponseHeader[End] Result = " << retVal << std::endl;
	return retVal;
}


inline MHD_Result TWebRequest::addResponseHeader(struct MHD_Response *& response, const std::string& header, const std::string& value) {
	MHD_Result r = MHD_NO;
	if (!header.empty()) {
		return addResponseHeader(response, header.c_str(), value);
	}
	return r;
}

inline MHD_Result TWebRequest::addResponseHeader(struct MHD_Response *& response, const char *header, const std::string& value) {
	MHD_Result r = MHD_NO;
	if (util::assigned(header) && !value.empty()) {
		if (debug)
			std::cout << "  addResponseHeader(" << header << ": " << value << ")" << std::endl;
		r = MHD_add_response_header(response, header, value.c_str());
		if (debug && MHD_YES != r) {
			if (debug)
				std::cout << "  addResponseHeader(" << header << ": " << value << ") failed." << std::endl;
		}
	}
	return r;
}


char convertEnvString(char c) {
	if (c == '-')
		return '_';
	return ::toupper(c);
}

size_t TWebRequest::buildCGIEnvironment(struct MHD_Connection *connection, EHttpMethod method, const util::TFile& file, const std::string& param, util::TEnvironment& env) {
	// http://tools.ietf.org/html/draft-robinson-www-interface-00
	env.addEntry("GATEWAY_INTERFACE", "CGI/1.1");

	// Add request method (GET or POST)
	env.addEntry("REQUEST_METHOD", httpMethodToStr(method));

	// Add environment variables
	env.addEntry("REMOTE_ADDR", inet::inetAddrToStr(connection->addr));

	// Add file/script information
	env.addEntry("SCRIPT_NAME", file.getName());
	env.addEntry("PATH_INFO", file.getPath());

	// Add query string
	// See http://www.jmarshall.com/easy/cgi/
	// For GET submissions, it's in the environment variable QUERY_STRING.
    // For POST submissions, read it from STDIN.
	// The exact number of bytes to read is in the environment variable CONTENT_LENGTH.
	env.addEntry("QUERY_STRING", param);
	env.addEntry("CONTENT_LENGTH", std::to_string((size_u)param.size()));

	// Add connection values
	std::string s;
	TValueMap::const_iterator it = session->values.begin();
	while (it != session->values.end()) {
		s = it->first;
		std::transform(s.begin(), s.end(), s.begin(), convertEnvString);
		env.addEntry("HTTP_" + s, it->second);
		++it;
	}

	// Add URI parameter list
	if (!params.empty()) {
		for (size_t i=0; i<params.size(); ++i) {
			s = params.name(i);
			if (!s.empty()) {
				std::transform(s.begin(), s.end(), s.begin(), convertEnvString);
				env.addEntry("URI_" + s, params.value(i).asString());
			}
		}
	}

	if (debug)
		env.debugOutput();

	return env.size();
}


std::string TWebRequest::createETag(const util::TFile* file, const app::TWebLink* link, bool upper) {
	/*
	 * Example for entity tag: inode-date-size 9a06fe-5545c363-186a
	 */
	const char* fmt = (upper) ? "%lX-%lX-%lX" : "%lx-%lx-%lx";

	// 1. Return entity tag derived from file and buffer data if possible
	if (util::assigned(file) ) {
		if (util::assigned(parsedBuffer)) {
			// File loaded to buffer
			return util::cprintf(fmt, file->getInode(), parsedBuffer->timestamp, parsedBuffer->size);
		} else {
			// Native inode file transfer
			return util::cprintf(fmt, file->getInode(), file->getTime().time(), file->getSize());
		}
	}

	// 2. Return entity tag derived from virtual file link properties
	if (util::assigned(link) && util::assigned(htmlPostBuffer)) {
		return util::cprintf(fmt, link->getHash(), htmlPostBuffer->getTimeStamp(), htmlPostBuffer->getSize());
	}

	// 3. Return random entity tag
	return randomETag(upper);

}


std::string TWebRequest::randomETag(bool upper)
{
	// Random entity tag
	char retVal[25]; // 6 + 8 + 4 Byte + 2 times '-' = 20 + 5 reserve
	char *p = retVal;
	int off = upper ? 55 : 87;
	int i;

	/* Data 1 - 6 characters */
	for(i = 0; i < 6; i++, p++)
		((*p = (rand() % 16)) < 10) ? *p += 48 : *p += off;

	/* Data 2 - 8 characters */
	*p++ = '-';
	for(i = 0; i < 8; i++, p++)
		((*p = (rand() % 16)) < 10) ? *p += 48 : *p += off;

	/* Data 3 - 4 characters */
	*p++ = '-';
	for(i = 0; i < 4; i++, p++)
		((*p = (rand() % 16)) < 10) ? *p += 48 : *p += off;

	*p = '\0';

	return std::string(retVal, 20);
}


bool TWebRequest::hasDynamicContent(const util::TFile* file, const app::TWebLink* link) {
	bool vary = false;

	// Check for dynamically changing content
	if (util::assigned(link)) {
		vary = true;
	}
	if (!vary && util::assigned(file)) {
		vary = file->hasToken() || file->isCGI();
	}
	if (!vary && util::assigned(htmlPostBuffer)) {
		vary = htmlPostBuffer->isValid();
	}
	if (!vary && util::assigned(parsedBuffer)) {
		vary = parsedBuffer->isValid();
	}

	return vary;
}


ssize_t TWebRequest::contentReaderCallback(void *cls, uint64_t pos, char *buf, size_t max) {
	ssize_t retVal = (ssize_t)MHD_CONTENT_READER_END_WITH_ERROR;
	if (util::assigned(callbackBuffer)) {
		retVal = (ssize_t)MHD_CONTENT_READER_END_OF_STREAM;
		if (pos < callbackSize) {
			if ((pos + max) > callbackSize)
				max = callbackSize - pos;
			void *p = callbackBuffer + pos;
			if (max > 0) {
				memcpy(buf, p, max);
				retVal = (ssize_t)max;
			}
		}
	}
	return retVal;
}


void TWebRequest::contentReaderFreeCallback(void *cls) {
	callbackBuffer = nil;
	callbackSize = 0;
}


ssize_t TWebRequest::inodeReaderCallback(void *cls, uint64_t pos, char *buf, size_t max) {
	ssize_t retVal = currentInode.read(buf, pos, max);
	if (retVal > 0)
		return retVal;
	return (ssize_t)MHD_CONTENT_READER_END_WITH_ERROR;
}


void TWebRequest::inodeReaderFreeCallback(void *cls) {
	currentInode.close();
	if (debug)
		std::cout << "TWebRequest::inodeReaderFreeCallback() : File closed." << std::endl;
}


MHD_Result TWebRequest::postIteratorHandler (void *cls,
									  enum MHD_ValueKind kind,
									  const char *key,
									  const char *filename,
									  const char *content_type,
									  const char *transfer_encoding,
									  const char *data, uint64_t off, size_t size) {
	MHD_Result retVal = MHD_YES;
	bool found = false;
	size_t len = 0;
	if (util::assigned(key)) {
		len = strnlen(key, HTTP_MAX_TOKEN_LENGTH);
	}
	if (util::assigned(data) && util::assigned(key) && len > 0 && size > 0) {

		if (util::assigned(session) && !multipart) {

			// Add every key/value pair to session
			session->setSessionValue(std::string(key, len), std::string(data, size));

			// Web form based authentication
			if (off == 0 && !session->authenticated) {
				std::string username, password;
				bool authenticated = false;

				// Read username and store in session
				if (strcmp(key, USERNAME_IDENT) == 0) {
					username = std::string(data, size);
					found = true;
				}

				// Read password and store in session
				if (strcmp(key, PASSWORD_IDENT) == 0) {
					password = std::string(data, size);
					found = true;
				}

				// Set session values
				if (found) {
					app::TLockGuard<app::TMutex> lock(session->userMtx);
					session->username = username;
					session->password = password;
					authenticated = session->authenticated = !(session->username.empty() || session->password.empty());
				}

				// TODO Check credentials...
				if (found) {
					session->setUserValues(username, 0, authenticated);
				}

			} // if (off == 0 && !session->authenticated)

		} // if (util::assigned(session) && !multipart)

		// Forward iterator callback to calling webserver
		if (util::assigned(postDataIterator) && !found) {
			// std::cout << " ----> TWebRequest::postIteratorHandler() Key = " << util::charToStr(key, "NULL") << ", filename = " << util::charToStr(filename, "NULL") << ", content_type = " << util::charToStr(content_type, "NULL") << ", transfer_encoding = " << util::charToStr(transfer_encoding, "NULL") << std::endl;
			retVal = postDataIterator(this, kind, key, len, filename, content_type, transfer_encoding, data, off, size);
		}

	} // if (util::assigned(data) && util::assigned(key) && len > 0 && size > 0)

	return retVal;
}

std::string TWebRequest::getSessionValue(const std::string& key) const {
	if (util::assigned(session)) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(session->matrixLck, RWL_READ);
		size_t idx = session->matrix.find(key);
		return session->matrix[idx].asString();
	}
	return "";
}

const size_t MIME_SEARCH_DEPTH = 50;

bool TWebRequest::lookupMimeType(char *& data, size_t& size, std::string& mime) {
	// Set mime type to TEXT_MIME_TYPE for all content other than HTML
	mime = TEXT_MIME_TYPE;

	if (size > 0) {
		size_t depth = std::min(size, MIME_SEARCH_DEPTH);

		char* p = util::strncasestr(data, MHD_HTTP_HEADER_CONTENT_TYPE, depth);
		if (util::assigned(p)) {

			bool valid = false;
			int state = 0;
			size_t i = 0;
			char* q;

			// Buffer must be \0 termiated!
			for ( ; i < depth && util::assigned(p); ++p, ++i) {
				switch(state) {
					case 0:
						// Wait for ":" in "Content-type: text/html\n"
						if (*p == ':') {
							q = util::succ(p);
							state = 10;
						}
						break;

					case 10:
						// Check for separator (some kind of consistency check...)
						if (*p == '/') {
							valid = true;
						}
						// Wait for first line feed in "Content-type: text/html\n"
						if (*p == '\n') {
							if (valid && (p > q)) {
								// Copy content-type string from data
								std::string s(q, p - q);
								util::trim(s);
								if (!s.empty()) {
									std::transform(s.begin(), s.end(), s.begin(), ::tolower);
									mime = s;
									if (debug)
										std::cout << "TWebRequest::parseCGIResponseBuffer() mime = " << mime << std::endl;
								}
							}
							// Check for data after "Content-type" header
							// --> Return pointer to first char after "Content-type: text/html\n"
							//     and set new buffer size
							if (p > data) {
								size_t offs = p - data;
								if (size > offs) {
									size = size - offs - 1;
									data = p + 1;
								}
							}
							return true;
						}
						break;

					default:
						break;
				}

			}
		}
	}
	return false;
}



TInode::TInode() {
	prime();
}

TInode::~TInode() {
	close();
}

void TInode::prime() {
	inode = nil;
	position = 0;
	size = 0;
	index = -1;
}

void TInode::clear() {
	prime();
	boundary.clear();
	util::clearObjectList(ranges);
}

bool TInode::open(const std::string& fileName) {
	if (!fileName.empty()) {
		size_t r = util::fileSize(fileName);
		if (r > 0) {
			inode = fopen(fileName.c_str(), "rb");
			if (util::assigned(inode)) {
				size = r;
				boundary = randomMultipartBoundary();
				return true;
			}
		}
	}
	return false;
}

void TInode::close() {
	if (isOpen()) {
		fclose(inode);
		clear();
	}
}

bool TInode::seek(const size_t pos) {
	if (isOpen()) {
		if (position != pos) {
			if (EXIT_SUCCESS == fseek(inode, pos, SEEK_SET)) {
				position = pos;
				return true;
			}
		} else {
			return true;
		}
	}
	return false;
}

ssize_t TInode::read(void* buffer, const size_t size) {
	if (isOpen()) {
		ssize_t r = (ssize_t)fread(buffer, 1, size, inode);
		if (r > 0) {
			position += (size_t)r;
			return r;
		}
	}
	return (ssize_t)0;
}

ssize_t TInode::read(void* buffer, const size_t position, const size_t size) {
	if (isOpen()) {
		size_t offset = position;
		PWebRange range = getActiveRange();
		if (util::assigned(range)) {
			if (range->start > 0)
				offset += range->start;
		}
		if (seek(offset))
			return read(buffer, size);
	}
	return (ssize_t)0;
}

std::string TInode::randomMultipartBoundary(const bool hyphen) const {
	//                     123456789-123456789-123456789-123456789-123456789-123456789-12
	static char chars[] = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	// Create a string of length from 20 to 30 chars plus hyphens and fill it with "-"
	size_t start = hyphen ? (size_t)rand() % 5 + 8 : 0;
	size_t count = start + (size_t)rand() % 11 + 20;
	std::string boundary(count, '-');
	for (size_t i = start; i < count; i++) {
		boundary.replace(i, 1, 1, chars[(size_t)rand() % 62]);
	}
	return boundary;
}


PWebRange TInode::getActiveRange() const {
	if (index >= 0 && index < (ssize_t)ranges.size()) {
		return ranges[index];
	}
	return nil;
}

bool TInode::parseRanges(const std::string header) {
	std::string item = "bytes=";
	if (0 == util::strncasecmp(header, item, item.size())) {
		std::string csv = header.substr(item.size(), std::string::npos);
		util::TStringList list;
		list.assign(csv, ',');
		if (!list.empty()) {
			list.compress();
			for (size_t i=0; i<list.size(); ++i) {
				const std::string& line = list[i];
				size_t pos = line.find('-');
				if (std::string::npos != pos) {
					std::string start = line.substr(0, pos);
					std::string end = line.substr(pos+1, std::string::npos);
					PWebRange o = new TWebRange;
					o->start = start.empty() ? 0 : util::strToInt(start);
					o->end = end.empty() ? std::string::npos : util::strToInt(end);
					// std::cout << "TInode::parseRanges() Range = \"" << line << "\" Start = \"" << start << "\" End = \"" << end << "\" --> (" << o->start << "," << o->end << ")" << std::endl;
					ranges.push_back(o);
				}
			}
		}
	}
	if (hasRanges()) {
		index = 0;
		return true;
	}
	return false;
}

} /* namespace app */
