/*
 * webclient.cpp
 *
 *  Created on: 16.07.2016
 *      Author: Dirk Brinkmeier
 */

#include "webclient.h"
#include "exception.h"
#include "globals.h"


static size_t writeFileDispatcher(void *buffer, size_t size, size_t count, void *ctx) {
	if (util::assigned(ctx))
		return (static_cast<app::PWebBaseClient>(ctx))->writeFileHandler(buffer, size, count);
	return 0;
}

static size_t writeReceiveDispatcher(void *buffer, size_t size, size_t count, void *ctx) {
	if (util::assigned(ctx))
		return (static_cast<app::PWebBaseClient>(ctx))->writeReceiveHandler(buffer, size, count);
	return 0;
}

static size_t writeDataDispatcher(void *buffer, size_t size, size_t count, void *ctx) {
	if (util::assigned(ctx))
		return (static_cast<app::PWebBaseClient>(ctx))->writeDataHandler(buffer, size, count);
	return 0;
}

static size_t writeStringDispatcher(void *buffer, size_t size, size_t count, void *ctx) {
	if (util::assigned(ctx))
		return (static_cast<app::PWebBaseClient>(ctx))->writeStringHandler(buffer, size, count);
	return 0;
}

static size_t writeHeaderDispatcher(char *buffer, size_t size, size_t count, void *ctx) {
	if (util::assigned(ctx))
		return (static_cast<app::PWebBaseClient>(ctx))->writeHeaderHandler(buffer, size, count);
	return 0;
}

static size_t writeNullDispatcher(void *buffer, size_t size, size_t count, void *ctx) {
	if (util::assigned(ctx))
		return (static_cast<app::PWebBaseClient>(ctx))->writeNullHandler(buffer, size, count);
	return 0;
}


namespace app {


static bool URLEngineInitialized = false;

TURLInit::TURLInit() {
	if (!app::URLEngineInitialized) {
		curl_global_init(CURL_GLOBAL_ALL);
		URLEngineInitialized = true;
	}
}

TURLInit::~TURLInit() {
	if (app::URLEngineInitialized) {
		curl_global_cleanup();
		URLEngineInitialized = false;
	}
}



TURLClient::TURLClient() {
	init();
}

TURLClient::~TURLClient() {
	finalize();
}

void TURLClient::init() {
	curl = nil;
	initialize();
}

void TURLClient::initialize() {
	if (!util::assigned(curl)) {
		curl = curl_easy_init();
		if (!util::assigned(curl))
			throw util::app_error("TURLClient::init() Calling curl_easy_init() failed.");
	}
}

void TURLClient::finalize() {
	if (util::assigned(curl))
		curl_easy_cleanup(curl);
	curl = nil;
}



TWebClient::TWebClient() {
	prime();
}

TWebClient::TWebClient(const std::string& url) {
	this->url = util::trim(url);
	prime();
}

TWebClient::~TWebClient() {
	cleanup();
}

void TWebClient::prime() {
	agent = "TWebClient/1.0 (dbApplication)";
	follow = false;
	overwrite = false;
	readSize = true;
	onReadMethod = nil;
	onDataMethod = nil;
	onTableMethod = nil;
	onVariantMethod = nil;
	onHeaderDataMethod = nil;
	httpVersion = EHP_DEFAULT;
	connectTimeout = 10L;
	transferTimeout = 0L;
	allowCompress = true;
	ignoreLength = false;
	bufferSize = 1024;
	chunkSize = 0;
	debug = false;
	headerList = nil;
	clear();
}

void TWebClient::clear() {
	aborted = false;
	terminated = false;
	processing = false;
	relativeSize = 0;
	currentSize = 0;
	contentSize = 0;
	rxHeaders.clear();
	buffer.clear();
	chunk.clear();
	data.clear();
	cleanup();
}

void TWebClient::cleanup() {
	// Free headers if set before
	if (util::assigned(headerList))
		curl_slist_free_all(headerList);
	headerList = nil;
	if (streamFile.isOpen()) {
		streamFile.close();
	}
}


void TWebClient::abort() {
	if (processing)
		aborted = true;
};

void TWebClient::terminate() {
	if (processing)
		terminated = true;
};


std::string TWebClient::errmsg() const {
	if (errval < CURL_LAST) {
		return errmsg(error());
	}
	// Error must be HTTP status code
	EWebStatusCode code = (EWebStatusCode)(errval - CURL_LAST);
	std::string status = getWebStatusMessage(code);
	return util::csnprintf("HTTP ERROR $ (%)", status, code);
};


void TWebClient::setCredentials(const std::string& credentials) {
	this->credentials = credentials;
	if (std::string::npos == credentials.find_first_of(":"))
		throw util::app_error_fmt("TWebClient::setCredentials() Malformed credentials $, missing \":\"", credentials);
}


size_t TWebClient::writeFileHandler(void *buffer, size_t size, size_t count) {
	size_t sz = 0;
	if (!isInterrupted() && file.isOpen()) {
		sz = size * count;
		if (sz > 0) {
			size_t r = file.write(buffer, sz);
			currentSize += r;
			onRead();
			return r;
		}
	}
	return sz;
}

size_t TWebClient::writeDataHandler(void *buffer, size_t size, size_t count) {
	size_t sz = 0;
	if (!isInterrupted()) {
		sz = size * count;
		if (sz > 0) {
			data.append(buffer, sz);
			currentSize += sz;
			onRead();
		}
	}	
	return sz;
}

size_t TWebClient::writeReceiveHandler(void *buffer, size_t size, size_t count) {
	size_t sz = 0;
	if (!isInterrupted()) {
		sz = size * count;
		if (sz > 0) {
			if (streamFile.isOpen()) {
				streamFile.write(buffer, sz);
			}
			currentSize += sz;
			if (chunkSize > 0) {
				if (sz < chunkSize) {
					chunk.append(buffer, sz);
					if (chunk.size() >= chunkSize) {
						onReceived(chunk.data(), chunk.size());
						chunk.clear();
					}
				} else {
					if (!chunk.empty()) {
						chunk.append(buffer, sz);
						onReceived(chunk.data(), chunk.size());
						chunk.clear();
					} else {
						onReceived(buffer, sz);
					}
				}
			} else {
				onReceived(buffer, sz);
			}
		}
	}
	return sz;
}

size_t TWebClient::writeStringHandler(void *buffer, size_t size, size_t count) {
	size_t sz = 0;
	if (!isInterrupted()) {
		sz = size * count;
		if (sz > 0) {
			this->buffer.append((char*)buffer, sz);
			currentSize += sz;
			onRead();
		}
	}	
	return sz;
}

size_t TWebClient::writeHeaderHandler(void *buffer, size_t size, size_t count) {
	size_t sz = 0;
	if (!isInterrupted()) {
		sz = size * count;
		if (sz > 0) {
			std::string header = util::trim(std::string((char*)buffer, sz));
			if (!header.empty()) {
				if (debug) {
					std::cout << "    TWebClient::writeHeaderHandler() Content = \"" \
								<< util::ellipsis(header, 65) \
									<< "\", Size = " << (sz) << std::endl;
				}
				std::string key, value;
				if (parseHeaderData(header, key, value)) {
					onHeaderData(key, value);
				}
			}
		}
	}
	return sz;
}

size_t TWebClient::writeNullHandler(void *buffer, size_t size, size_t count) {
	size_t sz = 0;
	if (!isInterrupted()) {
		sz = size * count;
		if (sz > 0 && debug) {
			std::string header = util::trim(std::string((char*)buffer, sz));
			if (!header.empty() && debug) {
				std::cout << "    TWebClient::writeNullHandler() Content = \"" \
							<< util::ellipsis(util::trim(std::string((char*)buffer, sz)), 65) \
								<< "\", Size = " << (sz) << std::endl;
			}
		}
	}
	return sz;
}


std::string TWebClient::errmsg(CURLcode error) const {
	return util::csnprintf("$ (%)", curl_easy_strerror(error), error);
}


bool TWebClient::execute(TURLClient& curl, TWebClientWriter callback, const bool requestHeaders) {
	if (!isValid())
		throw util::app_error("TWebClient::execute() failed: Missing URL.");

	// Enable event handler call during execution
	util::TBooleanGuard<bool> bg(processing);
	bg.set();

	// Set URL to download from
	errval = curl_easy_setopt(curl(), CURLOPT_URL, url.c_str());
	if (!success()) {
		throw util::app_error_fmt("TWebClient::execute() Setting URL failed for $ [%]", url, errmsg());
	}

	// Set custom headers
	if (!txHeaders.empty() && !util::assigned(headerList)) {
		bool first = true;
		headerList = nil;
		for (size_t i=0; i<txHeaders.size(); ++i) {
			const std::string& s = txHeaders.at(i);
			struct curl_slist * sl = nil;
			if (!s.empty()) {
				if (first) {
					sl = headerList = curl_slist_append(NULL, s.c_str());
					first = false;
				} else {
					sl = curl_slist_append(headerList, s.c_str());
				}
				if (!util::assigned(sl)) {
					throw util::app_error_fmt("TWebClient::execute() Setting custom header failed for $ <%;> [%]", url, s, errmsg());
				}
			}
		}
		if (util::assigned(headerList)) {
			errval = curl_easy_setopt(curl(), CURLOPT_HTTPHEADER, headerList);
			if (!success()) {
				throw util::app_error_fmt("TWebClient::execute() Setting custom headers failed for $ [%]", url, errmsg());
			}
		}
	}

	// Complete connection within given timeout in seconds
	if (connectTimeout > 0) {
		errval = curl_easy_setopt(curl(), CURLOPT_CONNECTTIMEOUT, connectTimeout);
		if (!success()) {
			throw util::app_error_fmt("TWebClient::execute() Setting connection timeout failed for $, timeout = % sec [%]", url, connectTimeout, errmsg());
		}
	}

	// Complete within given timeout in seconds
	if (transferTimeout > 0) {
		errval = curl_easy_setopt(curl(), CURLOPT_TIMEOUT, transferTimeout);
		if (!success()) {
			throw util::app_error_fmt("TWebClient::execute() Setting transfer timeout failed for $, timeout = % sec [%]", url, transferTimeout, errmsg());
		}
	}
	
	// Set HTTP protocol version
	if (httpVersion != EHP_DEFAULT) {
		switch (httpVersion) {
			case EHP_VERSION_1:
				curl_easy_setopt (curl(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
				break;
			case EHP_VERSION_2:
				curl_easy_setopt (curl(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
				break;
			case EHP_VERSION_2_TLS:
				curl_easy_setopt (curl(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
				break;
			default:
				break;
		}
	}	

	// Ignore (wrong) content length from server
	// !!! ENABLE WHEN STRICTLY NEEDE ONLY !!!
	if (ignoreLength) {
		errval = curl_easy_setopt(curl(), CURLOPT_IGNORE_CONTENT_LENGTH, 1L);
		if (!success()) {
			throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_IGNORE_CONTENT_LENGTH failed for $, timeout = % sec [%]", url, transferTimeout, errmsg());
		}
	}

	// Setup transfer data callback
	if (nil != callback) {
		errval = curl_easy_setopt(curl(), CURLOPT_WRITEFUNCTION, callback);
		if (!success()) {
			throw util::app_error_fmt("TWebClient::execute() Setting callback method failed for $ [%]", url, errmsg());
		}
	}
	errval = curl_easy_setopt(curl(), CURLOPT_WRITEDATA, this);
	if (!success()) {
		throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_WRITEDATA option failed for $ [%]", url, errmsg());
	}

	// Setup header data callback
	if (requestHeaders && nil != onHeaderDataMethod) {
		errval = curl_easy_setopt(curl(), CURLOPT_HEADERFUNCTION, writeHeaderDispatcher);
		if (!success()) {
			throw util::app_error_fmt("TWebClient::execute() Setting header callback method failed for $ [%]", url, errmsg());
		}
		errval = curl_easy_setopt(curl(), CURLOPT_HEADERDATA, this);
		if (!success()) {
			throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_HEADERDATA option failed for $ [%]", url, errmsg());
		}
	}

	// Set user credentials
	if (!credentials.empty()) {
		errval = curl_easy_setopt(curl(), CURLOPT_USERPWD, credentials.c_str());
		if (CURLE_OK != errval) {
			std::string cr = (credentials.empty()) ? "none" : credentials;
			throw util::app_error_fmt("TWebClient::execute() Setting credentials failed for $ [%]", cr, errmsg());
		}
	}

	// Trust all SSL/TLS connections...
	errval = curl_easy_setopt(curl(), CURLOPT_SSL_VERIFYPEER, 0L);
	if (!success()) {
		throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_SSL_VERIFYPEER option failed for $ [%]", url, errmsg());
	}
	errval = curl_easy_setopt(curl(), CURLOPT_SSL_VERIFYHOST, 0L);
	if (!success()) {
		throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_SSL_VERIFYHOST option failed for $ [%]", url, errmsg());
	}
	errval = curl_easy_setopt(curl(), CURLOPT_FOLLOWLOCATION, (follow ? 1L : 0L));
	if (!success()) {
		throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_FOLLOWLOCATION option failed for $ [%]", url, errmsg());
	}

	// Enable all supported built-in compressions
	errval = curl_easy_setopt(curl(), CURLOPT_ACCEPT_ENCODING, ""); //allowCompress ? "" : 0L);
	if (!success()) {
		throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_ACCEPT_ENCODING option failed for $ [%]", url, errmsg());
	}

	// Disable signal handlers
	errval = curl_easy_setopt(curl(), CURLOPT_NOSIGNAL, 1L);
	if (!success()) { // && CURLE_UNKNOWN_OPTION != errval) {
		throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_NOSIGNAL option failed for $ [%]", url, errmsg());
	}

	// Set application user agent name
	if (!agent.empty()) {
		errval = curl_easy_setopt(curl(), CURLOPT_USERAGENT, agent.c_str());
		if (!success()) {
			throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_USERAGENT option failed for $", agent, errmsg());
		}
	}

	// Be verbose?
	if (debug) {
		errval = curl_easy_setopt(curl(), CURLOPT_VERBOSE, 1L);
		if (!success()) {
			throw util::app_error_fmt("TWebClient::execute() Setting CURLOPT_VERBOSE option failed for $", agent, errmsg());
		}
	}

	// Execute prepared CURL action...
	errval = curl_easy_perform(curl());
	return success();
}


size_t TWebClient::getContentSize() {
	size_t r = 0;
	if (readSize && !ignoreLength) {
		double size = 0.0;
		TURLClient curl;
		errval = curl_easy_setopt(curl(), CURLOPT_HEADER, 1L);
		if (success()) {
			errval = curl_easy_setopt(curl(), CURLOPT_NOBODY, 1L);
			if (success()) {
				if (execute(curl, writeNullDispatcher, false)) {
					errval = curl_easy_getinfo(curl(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size);
					if (success()) {
						r = floor(size);
					}
				}
			}
		}
	}
	return r;
}

size_t TWebClient::alignToContentSize() {
	// Set data buffer to estimated size + 20%
	size_t size = bufferSize;
	if (!ignoreLength) {
		contentSize = getContentSize();
		if (contentSize > 0 && contentSize < 128000) {
			size = contentSize + 2 * contentSize / 10;
		}
	}
	return size;
}


EWebStatusCode TWebClient::getLastResponseCode(TURLClient& curl) {
    long code = WSC_NotFound;
    CURLcode r = curl_easy_getinfo(curl(), CURLINFO_RESPONSE_CODE, &code);
    if (CURLE_OK == r) {
    	return (EWebStatusCode)code;
    }
    return WSC_ImATeapot;
}


bool TWebClient::parseHeaderData(const std::string& header, std::string& key, std::string& value) {
	if (!header.empty()) {
		size_t pos = header.find_first_of(':');
		if (std::string::npos != pos) {
			key = util::trim(header.substr(0, pos));
			value = util::trim(header.substr(util::succ(pos)));
			if (!(key.empty() || value.empty())) {
				rxHeaders.add(key, value);
				return true;
			}
			value.clear();
			key.clear();
		}
	}
	return false;
}


bool TWebClient::readData(const std::string& url) {
	this->url = util::trim(url);
	baseName = util::fileExtName(this->url);
	return readData();
}

bool TWebClient::readData() {
	clear();

	// Set data buffer to estimated size + 20%
	size_t size = alignToContentSize();
	if (size > 0) {
		data.reserve(size, false);
	}

	// Retrieve data from given URL
	TURLClient curl;
	if (execute(curl, writeDataDispatcher))
		return hasData();

	return false;
}


bool TWebClient::readString(const std::string& url, std::string& str)  {
	this->url = util::trim(url);
	baseName = util::fileExtName(this->url);
	return readString(str);
}

bool TWebClient::readString(std::string& str) {
	str.clear();
	clear();

	// Set string buffer to estimated size + 20%
	size_t size = alignToContentSize();
	if (size > 0) {
		buffer.reserve(size);
	}

	// Retrieve string data from given URL
	TURLClient curl;
	if (execute(curl, writeStringDispatcher)) {
		if (util::TASCII::isValidUTF8MultiByteStr(buffer.data(), buffer.size())) {
			str.assign(buffer.data(), buffer.size());
			return !str.empty();
		}
	}

	return false;
}


bool TWebClient::readVariant(const std::string& url, util::TVariantValues& variants)  {
	this->url = util::trim(url);
	baseName = util::fileExtName(this->url);
	return readVariant(variants);
}

bool TWebClient::readVariant(util::TVariantValues& variants) {
	variants.clear();
	clear();

	// Set string buffer to estimated size + 20%
	size_t size = alignToContentSize();
	if (size > 0) {
		buffer.reserve(size);
	}

	// Retrieve string data from given URL
	TURLClient curl;
	if (execute(curl, writeStringDispatcher)) {
		if (util::TASCII::isValidUTF8MultiByteStr(buffer.data(), buffer.size())) {
			std::string raw(buffer.data(), buffer.size());
			if (!raw.empty()) {
				variants.parseJSON(raw);
				if (debug) {
					variants.debugOutput("TWebClient::readVariant()", "  ");
				}
				return !variants.empty();
			}
		}
	}

	return false;
}


bool TWebClient::readTable(const std::string& url, data::TTable& table)  {
	this->url = util::trim(url);
	baseName = util::fileExtName(this->url);
	return readTable(table);
}

bool TWebClient::readTable(data::TTable& table) {
	table.clear();
	clear();

	// Set string buffer to estimated size + 20%
	size_t size = alignToContentSize();
	if (size > 0) {
		buffer.reserve(size);
	}

	// Retrieve string data from given URL
	TURLClient curl;
	if (execute(curl, writeStringDispatcher)) {
		if (util::TASCII::isValidUTF8MultiByteStr(buffer.data(), buffer.size())) {
			std::string raw(buffer.data(), buffer.size());
			if (!raw.empty()) {
				table.parseJSON(raw);
				if (debug) {
					table.debugOutputData("  TWebClient::readTable()");
				}
				return !table.empty();
			}
		}
	}

	return false;
}


bool TWebClient::download(const std::string& url, const std::string& folder) {
	this->url = util::trim(url);
	baseName = util::fileExtName(this->url);
	fileName = util::validPath(folder) + baseName;
	return download();
}

bool TWebClient::download(const std::string& folder) {
	baseName = util::fileExtName(url);
	fileName = util::validPath(folder) + baseName;
	return download();
}

bool TWebClient::download() {
	clear();

	if (fileName.empty())
		throw util::app_error("TWebClient::getFile() failed: No filename given.");

	// Get unique file name
	if (overwrite) {
		if (util::fileExists(fileName)) {
			util::deleteFile(fileName);
		}
	} else {
		fileName = util::uniqueFileName(fileName);
	}

	// Is HTTP(S)?
	bool http = "HTTP" == util::toupper(url.substr(0,4));
	EWebStatusCode code = http ? WSC_NotFound : WSC_OK;

	// Create file
	size_t size = 0;
	file.assign(fileName);
	util::TFileGuard<util::TBaseFile> fg(file, O_WRONLY | O_CREAT);
	if (file.isOpen()) {

		// Read content size to resize new file
		size = bufferSize;
		contentSize = getContentSize();
		if (!ignoreLength) {
			contentSize = getContentSize();
			if (contentSize > 0 && contentSize < 1000000000) {
				size = contentSize;
			}
		}

		// Resize file to given/estimated content size
		if (size > 0) {
			file.resize(size);
		}

		// Retrieve file from given URL
		TURLClient curl;
		execute(curl, writeFileDispatcher);
		if (http && success()) {
			code = getLastResponseCode(curl);
			if (WSC_OK != code) {
				errval = (CURLcode)(CURL_LAST + code);
			}
		}

	} else {
		// Fake cURL error...
		errval = CURLE_FILE_COULDNT_READ_FILE;
	}

	// Check for successful file download
	if (success() && (currentSize > 0) && (WSC_OK == code)) {
		// Shrink file to fit downloaded size
		if (size > 0) {
			if (size > currentSize) {
				file.resize(currentSize);
			}
		}
		return true;
	}

	// Close file before deleting empty file
	if (file.isOpen()) {
		file.close();
	}
	if (util::fileExists(fileName)) {
		util::deleteFile(fileName);
	}

	return false;
}


bool TWebClient::receive(const std::string& url) {
	this->url = util::trim(url);
	baseName = util::fileExtName(this->url);
	return receive();
}

bool TWebClient::receive() {
	clear();

	// Deliver received data in minimal chunk sizes
	if (chunkSize > 0) {
		chunk.reserve(2 * chunkSize, false);
	}

	// Write copy of data to file
	if (!fileName.empty()) {
		util::deleteFile(fileName);
		streamFile.open(fileName, "w");
		if (!streamFile.isOpen())
			throw util::app_error_fmt("TWebClient::receive() Open file $ failed.", fileName);
	}

	// Retrieve data from given URL
	TURLClient curl;
	execute(curl, writeReceiveDispatcher);

	// Close file
	if (streamFile.isOpen()) {
		streamFile.close();
	}

	// Check for successful file download
	// --> ignore error on abort
	if ((isTerminated() || success()) && (currentSize > 0)) {
		if (!chunk.empty()) {
			// Send last chunk
			onReceived(chunk.data(), chunk.size());
			chunk.clear();
		}
		return true;
	}

	return false;
}


void TWebClient::onHeaderData(const std::string& key, const std::string& value) {
	if (nil != onHeaderDataMethod && processing) {
		onHeaderDataMethod(*this, key, value);
	}
}

void TWebClient::onRead() {
	if (nil != onReadMethod && processing) {
		size_t r = 0;
		if (contentSize > 0 && currentSize > 0) {
			r = currentSize * 100 / contentSize;
		}
		if ((r != relativeSize) || ((r == 0) && (contentSize == 0)) || (currentSize >= contentSize)) {
			try {
				// Execute data event method
				onReadMethod(*this, contentSize, currentSize, r);
			} catch (...) {}
			relativeSize = r;
		}
	}
}

void TWebClient::onReceived(const void *const data, const size_t size) {
	if (processing && util::assigned(data) && size > 0) {
		if (nil != onDataMethod) {
			try {
				// Execute data event method
				onDataMethod(*this, data, size);
			} catch (...) {}
		}
		if (nil != onVariantMethod) {
			try {
				util::TVariantValues variants;

				// JSON data should be UTF-8 by design!
				if (util::TASCII::isValidUTF8MultiByteStr(data, size)) {
					std::string raw((char*)data, size);
					if (!raw.empty()) {
						variants.parseJSON(raw);
						if (debug) {
							variants.debugOutput("TWebClient::onReceived()", "  ");
						}
					}
				}

				if (!variants.empty()) {
					try {
						// Execute data event method
						onVariantMethod(*this, variants);
					} catch (...) {}
				}
			} catch (...) {}
		}
		if (nil != onTableMethod) {
			try {
				data::TTable table;

				// JSON data should be UTF-8 by design!
				if (util::TASCII::isValidUTF8MultiByteStr(data, size)) {
					std::string raw((char*)data, size);
					if (!raw.empty()) {
						table.parseJSON(raw);
						if (debug) {
							table.debugOutputData("  TWebClient::onReceived()");
						}
					}
				}

				if (!table.empty()) {
					try {
						// Execute data event method
						onTableMethod(*this, table);
					} catch (...) {}
				}
			} catch (...) {}
		}
	}
}

} /* namespace app */
