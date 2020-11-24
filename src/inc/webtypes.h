/*
 * webtypes.h
 *
 *  Created on: 27.03.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef WEBTYPES_H_
#define WEBTYPES_H_

#include "gcc.h"
#include "timer.h"
#include "array.h"
#include "detach.h"
#include "random.h"
#include "sockets.h"
#include "nullptr.h"
#include "fileutils.h"
#include "datetime.h"
#include "webtoken.h"
#include "webconsts.h"
#include "semaphores.h"
#include "stringutils.h"
#include "credentialtypes.h"
#include "microhttpd/microhttpd.h"
#include "microhttpd/internal.h"

namespace app {

class TWebServer;
class TWebRequest;
struct CWebData;
struct CWebConfig;
struct CWebSettings;
struct CWebAction;
struct CWebActionItem;
struct CWebSession;
struct CWebSessionInfo;
struct CWebRequestInfo;
struct CWebRange;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PWebServer = TWebServer*;

using TWebData = CWebData;
using TWebConfig = CWebConfig;
using TWebSettings = CWebSettings;
using TWebAction = CWebAction;
using PWebAction = TWebAction*;
using TWebActionItem = CWebActionItem;
using PWebActionItem = TWebActionItem*;
using TWebActionMap = std::map<std::string, app::PWebActionItem>;
using TWebActionMapItem = std::pair<std::string, app::PWebActionItem>;
using TWebActionList = std::vector<app::PWebAction>;
using TActionThread = TDataThread<app::TWebAction>;
using PActionThread = TActionThread*;
using TWebRange = CWebRange;
using PWebRange = TWebRange*;
using TWebRangeList = std::vector<app::PWebRange>;
using TWebSession = CWebSession;
using PWebSession = CWebSession*;
using TWebSessionInfo = CWebSessionInfo;
using PWebSessionInfo = CWebSessionInfo*;
using TWebRequestInfo = CWebRequestInfo;
using PWebRequestInfo = CWebRequestInfo*;
using TWebSessionList = std::vector<app::PWebSession>;
using TWebSessionMap = std::map<std::string, app::PWebSession>;
using TWebSessionItem = std::pair<std::string, app::PWebSession>;
using PWebSessionMap = TWebSessionMap*;
using PWebRequest = TWebRequest*;
using TWebRequestList = std::vector<app::PWebRequest>;
using TWebSessionInfoList = std::vector<app::PWebSessionInfo>;
using TWebRequestInfoList = std::vector<app::PWebRequestInfo>;

using TWebActionHandler = std::function<void(const std::string&, const std::string&, const util::TVariantValues&, const util::TVariantValues&, int&)>;
using TWebPrepareHandler = std::function<void(const std::string&, const util::TVariantValues&, util::TVariantValues&, bool&)>;
using TCredentialCallback = std::function<bool(const std::string&, const std::string&)>;
using TFileUploadEvent = std::function<void(const app::TWebServer&, const util::TVariantValues&, const std::string&, const size_t&)>;
using TStatisticsEvent = std::function<void(const app::TWebServer&, const app::TWebData&)>;

using TPrepareHandlerList = std::vector<app::TWebPrepareHandler>;
using TFileUploadEventList = std::vector<app::TFileUploadEvent>;
using TStatisticsEventList = std::vector<app::TStatisticsEvent>;

#else

typedef TWebServer* PWebServer;

typedef CWebData TWebData;
typedef CWebConfig TWebConfig;
typedef CWebSettings TWebSettings;
typedef CWebAction TWebAction;
typedef TWebAction* PWebAction;
typedef CWebActionItem TWebActionItem;
typedef TWebActionItem* PWebActionItem;
typedef std::map<std::string, app::PWebActionItem> TWebActionMap;
typedef std::pair<std::string, app::PWebActionItem> TWebActionMapItem;
typedef std::vector<app::PWebAction> TWebActionList;
typedef TDataThread<app::TWebAction> TActionThread;
typedef TActionThread* 	PActionThread;
typedef CWebRange TWebRange;
typedef TWebRange* PWebRange;
typedef std::vector<app::PWebRange> TWebRangeList;
typedef CWebSession TWebSession;
typedef CWebSession* PWebSession;
typedef CWebSessionInfo TWebSessionInfo;
typedef CWebSessionInfo* PWebSessionInfo;
typedef CWebRequestInfo TWebRequestInfo;
typedef CWebRequestInfo* PWebRequestInfo;
typedef std::vector<app::PWebSession> TWebSessionList;
typedef std::map<std::string, PWebSession> TWebSessionMap;
typedef std::pair<std::string, PWebSession> TWebSessionItem;
typedef TWebSessionMap* PWebSessionMap;
typedef TWebRequest* PWebRequest;
typedef std::vector<app::PWebRequest> TWebRequestList;
typedef std::vector<app::PWebSessionInfo> TWebSessionInfoList;
typedef std::vector<app::PWebRequestInfo> TWebRequestInfoList;

typedef std::function<void(const std::string&, const std::string&, const util::TNamedVariants&, const util::TNamedVariants&, int&)> TWebActionHandler;
typedef std::function<void(const std::string&, const util::TNamedVariants&, util::TNamedVariants&, bool&)> TWebPrepareHandler;
typedef std::function<bool(const std::string&, const std::string&)> TCredentialCallback;
typedef std::function<void(const app::TWebServer&, const std::string&, const size_t&)> TFileUploadEvent;

typedef std::vector<app::TWebPrepareHandler> TPrepareHandlerList;
typedef std::vector<app::TFileUploadEvent> TFileUploadEventList;
typedef std::function<void(const app::TWebServer&, const app::TWebData&)> TStatisticsEvent;

typedef std::vector<app::TWebPrepareHandler> TPrepareHandlerList;
typedef std::vector<app::TFileUploadEvent> TFileUploadEventList;
typedef std::vector<app::TStatisticsEvent> TStatisticsEventList;

#endif


#ifndef MHD_HTTP_METHOD_SUBSCRIBE
#  define MHD_HTTP_METHOD_SUBSCRIBE "SUBSCRIBE" // UPnP method
#endif
#ifndef MHD_HTTP_METHOD_PATCH
#  define MHD_HTTP_METHOD_PATCH "PATCH" // JSON patch/update method
#endif


enum EWebTransferMode {
	WTM_SYNC,  // Transfer complete buffer
	WTM_ASYNC, // Transfer buffer by callback
	WTM_DEFAULT = WTM_SYNC
};

enum EWebActionMode {
	WAM_SYNC,  // Execute POST action immediately
	WAM_ASYNC  // Execute POST action asynchronously in thread
};

enum EWebPostMode {
	WPM_HTML,  // Use default MHD post process handling to parse data (file upload or posted URL data)
	WPM_DATA,  // Store posted data in buffer for later use
	WPM_DEFAULT = WPM_HTML
};

enum EWebCookieMode {
	WCM_STRICT,
	WCM_LAX,
	WCM_NONE,
	WCM_DEFAULT = WCM_STRICT
};

enum EWebTransactState {
	WTS_IDLE,        // No POST/UPLOAD in progress
	WTS_PROGRESS,    // Executing POST/UPLOAD action
	WTS_INTERRUPTED, // POST/UPLOAD transaction interrupted
	WTS_FINISHED     // POST/UPLOAD transaction finished
};

enum EHttpMethod {
	HTTP_UNKNOWN,
	HTTP_CONNECT,
	HTTP_DELETE,
	HTTP_GET,
	HTTP_HEAD,
	HTTP_OPTIONS,
	HTTP_PATCH,
	HTTP_POST,
	HTTP_PUT,
	HTTP_TRACE,
	HTTP_SUBSCRIBE
};

enum EHttpAuthType {
	HAT_DIGEST_NONE,
	HAT_DIGEST_MD5,
	HAT_DIGEST_SHA256,
	HAT_DIGEST_SHA512,
	HAT_DIGEST_DEFAULT = HAT_DIGEST_MD5
};
#ifdef STL_HAS_TEMPLATE_ALIAS
using TWebAuthMap = std::map<EHttpAuthType, std::string>;
#else
typedef std::map<EHttpAuthType, std::string> TWebAuthMap;
#endif


enum EWebLogVerbosity {
	WLV_ERROR   = 0,
	WLV_WARNING = 1,
	WLV_INFO    = 2
};

enum EWebUserAgent {
	WUA_UNKNOWN  = 0,
	WUA_MSIE     = 1,
	WUA_MSEDGE   = 2,
	WUA_MSCHROME = 3,
	WUA_FIREFOX  = 4,
	WUA_CHROMIUM = 5,
	WUA_SAFARI   = 6,
	WUA_NETSCAPE = 7,
	WUA_OPERA    = 8,
	WUA_CURL     = 9,
	WUA_APPLICATION = 10
};

enum EWebStatusCode
{
	// Information 1xx
	WSC_Continue                      = MHD_HTTP_CONTINUE,
	WSC_SwitchingProtocols            = MHD_HTTP_SWITCHING_PROTOCOLS,
	WSC_Processing                    = MHD_HTTP_PROCESSING,
	WSC_ConnectionTimedOut            = 103,

	// Success 2xx
	WSC_OK                            = MHD_HTTP_OK,
	WSC_Created                       = MHD_HTTP_CREATED,
	WSC_Accepted                      = MHD_HTTP_ACCEPTED,
	WSC_NonAuthoritativeInformation   = MHD_HTTP_NON_AUTHORITATIVE_INFORMATION,
	WSC_NoContent                     = MHD_HTTP_NO_CONTENT,
	WSC_ResetContent                  = MHD_HTTP_RESET_CONTENT,
	WSC_PartialContent                = MHD_HTTP_PARTIAL_CONTENT,
	WSC_MultiStatus                   = MHD_HTTP_MULTI_STATUS,

	// Redirects 3xx
	WSC_MultipleChoices               = MHD_HTTP_MULTIPLE_CHOICES,
	WSC_MovedPermanently              = MHD_HTTP_MOVED_PERMANENTLY,
	WSC_Found                         = MHD_HTTP_FOUND,
	WSC_SeeOther                      = MHD_HTTP_SEE_OTHER,
	WSC_NotModified                   = MHD_HTTP_NOT_MODIFIED,
	WSC_UseProxy                      = MHD_HTTP_USE_PROXY,
	WSC_SwitchProxy                   = MHD_HTTP_SWITCH_PROXY,
	WSC_TemporaryRedirect             = MHD_HTTP_TEMPORARY_REDIRECT,

	// Client errors 4xx
	WSC_BadRequest                    = MHD_HTTP_BAD_REQUEST,
	WSC_Unauthorized                  = MHD_HTTP_UNAUTHORIZED,
	WSC_PaymentRequired               = MHD_HTTP_PAYMENT_REQUIRED,
	WSC_Forbidden                     = MHD_HTTP_FORBIDDEN,
	WSC_NotFound                      = MHD_HTTP_NOT_FOUND,
	WSC_MethodNotAllowed              = MHD_HTTP_METHOD_NOT_ALLOWED,

	// Deprecated (see microhttpd.h)
	WSC_NotAcceptable                 = MHD_HTTP_NOT_ACCEPTABLE,
	WSC_ProxyAuthenticationRequired   = MHD_HTTP_PROXY_AUTHENTICATION_REQUIRED,
	WSC_RequestTimeout                = MHD_HTTP_REQUEST_TIMEOUT,
	WSC_Conflict                      = MHD_HTTP_CONFLICT,
	WSC_Gone                          = MHD_HTTP_GONE,
	WSC_LengthRequired                = MHD_HTTP_LENGTH_REQUIRED,
	WSC_PreconditionFailed            = MHD_HTTP_PRECONDITION_FAILED,

	// MHD 0.9.55
	// WSC_RequestEntityTooLarge         = MHD_HTTP_REQUEST_ENTITY_TOO_LARGE,
	// WSC_RequestURITooLong             = MHD_HTTP_REQUEST_URI_TOO_LONG,
	WSC_RequestEntityTooLarge         = MHD_HTTP_PAYLOAD_TOO_LARGE,
	WSC_RequestURITooLong             = MHD_HTTP_URI_TOO_LONG,

	WSC_UnsupportedMediaType          = MHD_HTTP_UNSUPPORTED_MEDIA_TYPE,

	// MHD 0.9.55
	// WSC_RequestedRangeNotSatisfiable  = MHD_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE,
	WSC_RequestedRangeNotSatisfiable  = MHD_HTTP_RANGE_NOT_SATISFIABLE,

	WSC_ExpectationFailed             = MHD_HTTP_EXPECTATION_FAILED,
	WSC_ImATeapot                     = 418,
	WSC_TooManyConnections            = 421,
	WSC_UnprocessableEntity           = MHD_HTTP_UNPROCESSABLE_ENTITY,
	WSC_Locked                        = MHD_HTTP_LOCKED,
	WSC_FailedDependency              = MHD_HTTP_FAILED_DEPENDENCY,

	// MHD 0.9.65
	// WSC_UnorderedCollection           = MHD_HTTP_UNORDERED_COLLECTION,
	WSC_UpgradeRequired               = MHD_HTTP_UPGRADE_REQUIRED,

	// MHD 0.9.65
	// WSC_NoRespose                     = MHD_HTTP_NO_RESPONSE,
	WSC_RetryWith                     = MHD_HTTP_RETRY_WITH,
	WSC_BlockedByParentalControls     = MHD_HTTP_BLOCKED_BY_WINDOWS_PARENTAL_CONTROLS,
	WSC_UnavailForLegaReason          = MHD_HTTP_UNAVAILABLE_FOR_LEGAL_REASONS,

	// Server errors 5xx
	WSC_InternalServerError           = MHD_HTTP_INTERNAL_SERVER_ERROR,
	WSC_NotImplemented                = MHD_HTTP_NOT_IMPLEMENTED,
	WSC_BadGateway                    = MHD_HTTP_BAD_GATEWAY,
	WSC_ServiceUnavailable            = MHD_HTTP_SERVICE_UNAVAILABLE,
	WSC_GatewayTimeout                = MHD_HTTP_GATEWAY_TIMEOUT,
	WSC_HTTPVersionNotSupported       = MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED,
	WSC_VariantAlsoNegotiates         = MHD_HTTP_VARIANT_ALSO_NEGOTIATES,
	WSC_InsufficientStorage           = MHD_HTTP_INSUFFICIENT_STORAGE,
	WSC_BandwidthLimitExceeded        = MHD_HTTP_BANDWIDTH_LIMIT_EXCEEDED,
	WSC_NotExtended                   = MHD_HTTP_NOT_EXTENDED
};
#ifdef STL_HAS_TEMPLATE_ALIAS
using TWebStatusMap = std::map<EWebStatusCode, std::string>;
#else
typedef std::map<EWebStatusCode, std::string> TWebStatusMap;
#endif



struct CWebData {
	bool init;

	time_t startTime;
	int64_t bytesServed;
	int connectionsV4;
	int connectionsV6;

	int64_t requestQueue;
	int64_t maxRequestQueue;

	int64_t actionQueue;
	int64_t maxActionQueue;
	
	int64_t requestCount;
	int64_t lastRequestCount;
	int64_t maxRequestCount;
	int deltaRequestCount;

	int64_t virtualCount;
	int64_t lastVirtualCount;
	int64_t maxVirtualCount;
	int deltaVirtualCount;

	int64_t sessionCount;
	int64_t maxSessionCount;

	int64_t socketCount;
	int64_t maxSocketCount;

	int64_t threadCount;
	int64_t threadLimit;

	PWebToken wtUpTime;
	PWebToken wtWebRefreshTimer;
	PWebToken wtDisplayRefreshTimer;
	PWebToken wtWebRefreshInterval;
	PWebToken wtDisplayRefreshInterval;
	PWebToken wtRequestCount;
	PWebToken wtVirtualCount;
	PWebToken wtSessionCount;
	PWebToken wtSocketCount;
	PWebToken wtBytesServed;
	PWebToken wtRequestQueue;
	PWebToken wtActionQueue;
	PWebToken wtConnections;
	PWebToken wtCurrentMemory;
	PWebToken wtStartMemory;
	PWebToken wtPeakMemory;
	PWebToken wtCpuAffinity;
	PWebToken wtCpuCores;
	PWebToken wtUserCount;
	PWebToken wtLicense;

	CWebData() {
		init = false;

		startTime = util::now();
		bytesServed = 0;
		requestQueue = 0;
		maxRequestQueue = 0;
		actionQueue = 0;
		maxActionQueue = 0;
		connectionsV4 = 0;
		connectionsV6 = 0;

		requestCount = 0;
		lastRequestCount = 0;
		maxRequestCount = 0;
		deltaRequestCount = 0;

		virtualCount = 0;
		lastVirtualCount = 0;
		maxVirtualCount = 0;
		deltaVirtualCount = 0;

		sessionCount = 0;
		maxSessionCount = 0;

		socketCount = 0;
		maxSocketCount = 0;

		threadCount = 0;
		threadLimit = 0;

		wtUpTime = nil;
		wtWebRefreshTimer = nil;
		wtDisplayRefreshTimer = nil;
		wtWebRefreshInterval = nil;
		wtDisplayRefreshInterval = nil;
		wtRequestCount = nil;
		wtSessionCount = nil;
		wtVirtualCount = nil;
		wtSocketCount = nil;
		wtBytesServed = nil;
		wtRequestQueue = nil;
		wtActionQueue = nil;
		wtConnections = nil;
		wtCurrentMemory = nil;
		wtStartMemory = nil;
		wtPeakMemory = nil;
		wtCpuAffinity = nil;
		wtCpuCores = nil;
		wtUserCount = nil;
		wtLicense = nil;
	}
};


struct CWebConfig {
	bool init;
	int verbosity;
	bool debug;
	bool enabled;
	bool useHttps;
	bool caching;
	bool minimize;
	bool threaded;
	int defaultUserLevel;
	unsigned int port;
	EHttpAuthType auth;
	TCredentialMap users;
	std::string credentials;
	std::string realm;
	int sessionExpired;
	std::string greeterURL;
	std::string documentRoot;
	std::string uploadFolder;
	std::string sessionStore;
	std::string certFile;
	std::string keyFile;
	std::string dhFile;
	std::string tlsCipherPriority;
	TTimerDelay sessionDeleteDelay;
	TTimerDelay requestDeleteDelay;
	TTimerDelay vfsDataDeleteDelay;
	TTimerDelay rejectedDeleteAge;
	int sessionDeleteAge;
	int requestDeleteAge;
	int maxSessionCount;
	util::TStringList indexPages;
	util::TStringList allowedList;
	bool allowFromAll;
	bool allowManifestFiles;
	std::string tokenDelimiter;
	std::string tokenHeader;
	std::string tokenFooter;
	int refreshInterval;
	int refreshTimer;
	std::string restRoot;
	bool disableVfsGZip;
	int maxConnectionsPerCPU;
	bool allowWebSockets;
	bool usePortraitMode;

	CWebConfig() {
		init = false;
		verbosity = 1;
		debug = false;
		enabled = true;
		useHttps = false;
		//useAuth = false;
		caching = true;
		minimize = false;
		threaded = true;
		allowFromAll = false;
		allowManifestFiles = true;
		defaultUserLevel = 0;
		auth = HAT_DIGEST_NONE;
		realm = AUTH_REALM;
		credentials = "admin:12345";
		sessionExpired = 0; // 0 sec --> disabled!
		greeterURL = GREETER_URL;
		port = 8099;
		documentRoot = "";
		uploadFolder = "";
		sessionStore = "";
		certFile = "builtin";
		keyFile = "builtin";
		dhFile = "builtin";
#if GNU_TLS_VERSION >= 30000
		tlsCipherPriority = CRV_TLS_PRIORITY;
#elif GNU_TLS_VERSION >= 21200
		tlsCipherPriority = STD_TLS_PRIORITY; // EXT_TLS_PRIORITY;
#else
		tlsCipherPriority = STD_TLS_PRIORITY;
#endif
		indexPages.clear();
		allowedList.clear();
		tokenHeader = "[[";
		tokenFooter = "]]";
		tokenDelimiter = tokenHeader + "X" + tokenFooter;
		sessionDeleteDelay = 90000;    // 90 seconds
		sessionDeleteAge   = 86400000; // 24 hours
		requestDeleteDelay = 15000;    // 15 seconds
		requestDeleteAge   = 30000;    // 30 seconds
		vfsDataDeleteDelay = 1000;     //  1 second
		rejectedDeleteAge  = sessionDeleteDelay * 90 / 100;
		maxSessionCount = 512;
		refreshInterval = 5;
		refreshTimer = 2000;
		restRoot = "/rest/";
		disableVfsGZip = false;
		maxConnectionsPerCPU = CONNECTIONS_PER_CPU;
		allowWebSockets = true;
		usePortraitMode = false;
	}
};

struct CWebSettings {
	int verbosity;
	int refreshInterval;
	int refreshTimer;
	bool allowWebSockets;

	CWebSettings& operator= (const CWebSettings& value) {
		verbosity = value.verbosity;
		refreshInterval = value.refreshInterval;
		refreshTimer = value.refreshTimer;
		allowWebSockets = value.allowWebSockets;
		return *this;
	}

	CWebSettings& operator= (const CWebConfig& value) {
		verbosity = value.verbosity;
		refreshInterval = value.refreshInterval;
		refreshTimer = value.refreshTimer;
		allowWebSockets = value.allowWebSockets;
		return *this;
	}

	CWebSettings() {
		verbosity = 0;
		refreshInterval = 0;
		refreshTimer = 0;
		allowWebSockets = false;
	}
};


struct CWebAction {
	bool running;
	std::string key;
	std::string value;
	util::TVariantValues params;
	util::TVariantValues session;
	app::TWebActionHandler handler;

	void prime() {
		handler = nil;
		running = false;
	}
	void reset() {
		params.clear();
		session.clear();
		running = false;
	}
	void clear() {
		prime();
		reset();
	}

	CWebAction() {
		prime();
	}
};


struct CWebRange {
	size_t start;
	size_t end;

	void prime() {
		start = 0;
		end = 0;
	}
	void clear() {
		prime();
	}

	bool isOpenRange() const {
		return end == std::string::npos;
	}

	bool isFullRange() const {
		return start == 0 && isOpenRange();
	}

	bool isValid() const {
		if (start != std::string::npos && end != std::string::npos)
			if (start >= 0 && end > 0)
				if (end > start)
					return true;
		return false;
	}

	CWebRange() {
		prime();
	}
};


struct CWebActionItem {
	TWebActionHandler action;
	EWebActionMode mode;
	std::string URL;
	util::TTimePart timestamp;
	size_t requested;
	size_t tag;
	
	void prime() {
		action = nil;
		mode = WAM_SYNC;
		timestamp = util::now();
		requested = 0;
		tag = 0;
	}

	void clear() {
		prime();
		URL.clear();
	}
	
	void incRequested() {
		requested++;
		timestamp = util::now();
	}

	CWebActionItem() {
		prime();
	}
};


struct CRequestData {
	const char* data;
	size_t size;
	bool found;
	bool zipped;
	bool caching;
	bool persistent;
	std::string mime;
	util::PFile file;
	util::PParserBuffer parser;

	void clear() {
		if (util::assigned(data))
			delete[] data;
		data = nil;
		size = 0;
		found = false;
		zipped = false;
		caching = true;
		persistent = true;
		mime.clear();
		file = nil;
		parser = nil;
	}

	CRequestData() {
		clear();
	}
};



/*
 * Stores temporary information during file upload
 */
struct CFileUpload {
	app::EWebTransactState state;
	time_t timestamp;
	int errnum;

	size_t contentSize;
	size_t fileSize;
	size_t dataSize;

	std::string fileName;
	util::TFile file;

	// Getter/Setter
	bool busy() {
		return (state != WTS_IDLE);
	}

	void setTimeStamp() {
		timestamp = util::now();
	}

	void finalize() {
		file.release();
		clear();
	}
	void clear() {
		state = WTS_IDLE;
		errnum = EXIT_SUCCESS;
		fileName.clear();
		contentSize = 0;
		fileSize = 0;
		dataSize = 0;
		setTimeStamp();
	}

	CFileUpload() {
		clear();
	}
};


/*
 * Stores temporary information during post process
 */
struct CPostProcess {
	// Current key/value data
	std::string key;
	std::string data;

	// Primary key/value data
	std::string query;
	std::string value;

	util::TVariantValues params;
	app::EWebTransactState state;

	// Getter/Setter
	bool busy() {
		return (state != WTS_IDLE);
	}

	bool empty() {
		return (query.empty() || value.empty());
	}
	bool valid() {
		return !(query.empty() || value.empty());
	}

	// Data methods
	void ident(const char* key, size_t len) {
		if (util::assigned(key) && len) {
			this->key = std::string(key, len);
		}
	}
	void assign(const char* data, size_t size) {
		if (util::assigned(data) && size) {
			this->data = std::string(data, size);
		}
	}
	void append(const char* data, size_t size) {
		if (util::assigned(data) && size) {
			this->data.append(data, size);
		}
	}

	void prime() {
		state = WTS_IDLE;
	}
	void clear() {
		if (!key.empty())
			key.clear();
		if (!data.empty())
			data.clear();
		if (!query.empty())
			query.clear();
		if (!data.empty())
			data.clear();
		if (!value.empty())
			value.clear();
		if (!params.empty())
			params.clear();
		prime();
	}

	CPostProcess() {
		prime();
	}
};


/*
 * Store state for each user/session/browser.
 */
struct CWebSession {
	// Unique session ID
	std::string sid;

	// Reference counter giving the number of requests
	// currently using this session.
	int refC;
	int useC;

	// Time when this session was last active.
	util::TTimePart timestamp;

	// Calling user agent
	EWebUserAgent userAgent;

	// Map of browser connection values
	// NOT thread save, only to be modified during request handling!!!
	TValueMap values;
	bool valuesRead;

	// Delete flag
	bool deleted;

	// Map of session values posted by client
	mutable TReadWriteLock matrixLck;
	util::TVariantValues matrix;

	void clearSessionValues() {
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_WRITE);
		matrix.clear();
	}

	void setConnectionValues(struct MHD_Connection *connection) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_WRITE);
		size_t idx = matrix.find(SESSION_ID);
		if (app::nsizet == idx)
			matrix.add(SESSION_ID, sid);
		if (util::assigned(connection))
			matrix.add(SESSION_REMOTE_HOST, util::toupper(inet::inetAddrToStr(connection->addr)));
	}
	void setUserValues(const std::string& username, int userlevel , bool authenticated) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_WRITE);
		if (!username.empty())
			matrix.add(SESSION_USER_NAME, username);
		matrix.add(SESSION_USER_AUTH, authenticated);
		matrix.add(SESSION_USER_LEVEL, userlevel);
	}
	void clearUserValues() {
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_WRITE);
		matrix.add(SESSION_USER_NAME, "*");
		matrix.add(SESSION_USER_AUTH, false);
		matrix.add(SESSION_USER_LEVEL, 0);
	}
	void getDefaultValues(std::string& sid, std::string& username, std::string& host) const {
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_READ);
		sid = matrix.value(matrix.find(SESSION_ID, SESSION_ID_SIZE)).asString("00000000-0000-0000-0000-000000000000");
		host = matrix.value(matrix.find(SESSION_REMOTE_HOST, SESSION_REMOTE_HOST_SIZE)).asString("0.0.0.0");
		username = matrix.value(matrix.find(SESSION_USER_NAME, SESSION_USER_NAME_SIZE)).asString("unknown");
	}

	std::string getSessionValue(const char* key) const {
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_READ);
		return matrix[key].asString();
	}
	std::string getSessionValue(const std::string& key) const {
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_READ);
		return matrix[key].asString();
	}

	// Add key/value pairs to session
	template<typename value_t>
	inline void setSessionValue(const char* key, const value_t value) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_WRITE);
		matrix.add(key, value);
	}
	template<typename value_t>
	inline void setSessionValue(const std::string& key, const value_t value) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_WRITE);
		matrix.add(key, value);
	}

	bool getSessionValues(util::TVariantValues& values) {
		values.clear();
		app::TReadWriteGuard<app::TReadWriteLock> lock(matrixLck, RWL_READ);
		if (!matrix.empty()) {
			values = matrix;
		}
		return false;
	}
	util::TVariantValues& getSessionValuesWithNolock() {
		return matrix;
	}

	// Map of user defined cookie values posted by client
	mutable TReadWriteLock cookieLck;
	util::TVariantValues cookies;

	void clearCookieValues() {
		app::TReadWriteGuard<app::TReadWriteLock> lock(cookieLck, RWL_WRITE);
		cookies.clear();
	}

	void setCookieValue(const std::string& name, const std::string& value) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(cookieLck, RWL_WRITE);
		if (!name.empty() && !value.empty()) {
			cookies.add(name, value);
		}
	}
	void setCookieValues(const util::TVariantValues& cookies) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(cookieLck, RWL_WRITE);
		this->cookies = cookies;
	}
	void updateCookieValues(const util::TVariantValues& cookies) {
		app::TReadWriteGuard<app::TReadWriteLock> lock(cookieLck, RWL_WRITE);
		this->cookies.merge(cookies);
	}

	std::string getCookieValue(const std::string& name) const {
		app::TReadWriteGuard<app::TReadWriteLock> lock(cookieLck, RWL_READ);
		size_t idx = cookies.find(name);
		if (idx != app::nsizet)
			return cookies.value(idx).asString();
		return "";
	}
	bool getCookieValues(util::TVariantValues& cookies) const {
		cookies.clear();
		app::TReadWriteGuard<app::TReadWriteLock> lock(cookieLck, RWL_READ);
		if (!this->cookies.empty()) {
			cookies = this->cookies;
		}
		return !cookies.empty();
	}

	// User credentials
	app::TMutex userMtx;
	int userlevel;
	bool authenticated;
	std::string username;
	std::string password;
	time_t timeout;
	bool logoff;

	// File upload process
	CFileUpload upload;

	// Post data processing
	CPostProcess post;
	
	bool busy() {
		return (post.busy() || upload.busy());
	}
	bool idle() {
		return !busy();
	}

	void setTimeStamp() {
		timestamp = util::now();
	}
	void setTimeOut() {
		timeout = util::now();
	}

	void prime() {
		valuesRead = false;
		userAgent = WUA_UNKNOWN;
		authenticated = false;
		deleted = false;
		logoff = false;
		userlevel = 0;
		refC = 0;
		useC = 0;
		setTimeOut();
		setTimeStamp();
	}
	void reset() {
		// sid.clear();
		prime();
		release();
		username.clear();
		password.clear();
		upload.clear();
		post.clear();
	}
	void release() {
		values.clear();
		clearSessionValues();
		clearCookieValues();
	}
	void clear() {
		sid.clear();
		reset();
	}

	CWebSession(const std::string id = "") {
		prime();
		sid = util::isValidUUID(id) ? id : util::fastCreateUUID(true, false);
		setSessionValue(SESSION_ID, sid);
	}
	virtual ~CWebSession() {
		release();
	}
};


/*
 * Store short info for user/session/browser.
 */
struct CWebSessionInfo {
	int refC;
	int useC;
	std::string sid;
	util::TTimePart timestamp;
	EWebUserAgent userAgent;
	bool authenticated;
	std::string username;
	std::string password;
	std::string remote;

	void prime() {
		refC = 0;
		useC = 0;
		timestamp = util::now();
		authenticated = false;
	}
	void clear() {
		prime();
		sid.clear();
		username.clear();
		password.clear();
		remote.clear();
	}

	CWebSessionInfo() {
		prime();
	}
};


/*
 * Store short info for RESTful API and virtual directories.
 */
struct CWebRequestInfo {
	std::string type;
	std::string url;
	size_t requested;
	util::TTimePart timestamp;
	size_t percent;

	void prime() {
		percent = 0;
		requested = 0;
		timestamp = util::now();
	}
	void clear() {
		prime();
		url.clear();
		type.clear();
	}

	CWebRequestInfo() {
		prime();
	}
};


class TWebOption : public util::TArray<MHD_OptionItem> {
private:
	typedef util::TArray<MHD_OptionItem> option_t;

public:
	void add(const MHD_OptionItem& item) {
		option_t::object_p o = new MHD_OptionItem;
		*o = item;
		option_t::add(o);
	}

	void add(enum MHD_OPTION option, intptr_t value, void *ptr_value) {
		option_t::object_p o = new MHD_OptionItem;
		o->option = option;
		o->value = value;
		o->ptr_value = ptr_value;
		option_t::add(o);
	}

	void terminate() {
		add(MHD_OPTION_END, 0, NULL);
	}

	void debugOutput() {
		MHD_OptionItem* p = array();
		for (size_t i=0; i<size(); ++i) {
			std::cout << i << ". Option = " << p[i].option << ", Value = " << p[i].value << "/" \
					<< (void*)p[i].value << ", Pointer = " << p[i].ptr_value << std::endl;
		}
		std::cout << std::endl;
	}

	TWebOption() : TArray(true) {
	}
};

struct CWebSocketRequest {
  struct MHD_UpgradeResponseHandle *urh;
  char *data;
  size_t size;
  THandle socket;
  TWebRequest* owner;

  void clear() {
	  urh = nil;
	  data = nil;
	  size = 0;
	  socket = INVALID_HANDLE_VALUE;
	  owner = nil;
  }

  CWebSocketRequest() {
	  clear();
  }
};


std::string getWebAuthType(const EHttpAuthType type);
EHttpAuthType getWebAuthType(const std::string& type);
std::string getWebStatusMessage(const EWebStatusCode status);
EWebUserAgent guessUserAgent(const char *value);
std::string userAgentToStr(const EWebUserAgent agent);
EHttpMethod getHttpMethod(const std::string& method);
EHttpMethod getHttpMethod(const char *const method);
std::string httpMethodToStr(const EHttpMethod method);

} /* namespace app */

#endif /* WEBTYPES_H_ */
