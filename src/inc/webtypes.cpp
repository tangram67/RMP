/*
 * webtypes.cpp
 *
 *  Created on: 06.04.2015
 *      Author: Dirk Brinkmeier
 */

#include "webtypes.h"
#include "compare.h"
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace app;


TWebStatusMap fillStatusMessages() {
	TWebStatusMap map;
	map[WSC_Continue]                      = "Continue";
	map[WSC_SwitchingProtocols]            = "Switching Protocols";
	map[WSC_Processing]                    = "Processing";
	map[WSC_ConnectionTimedOut]            = "Connection timed out";
	map[WSC_OK]                            = "OK";
	map[WSC_Created]                       = "Created";
	map[WSC_Accepted]                      = "Accepted";
	map[WSC_NonAuthoritativeInformation]   = "Non-Authoritative Information";
	map[WSC_NoContent]                     = "No Content";
	map[WSC_ResetContent]                  = "Reset Content";
	map[WSC_PartialContent]                = "Partial Content";
	map[WSC_MultiStatus]                   = "Multi-Status";
	map[WSC_MultipleChoices]               = "Multiple Choices";
	map[WSC_MovedPermanently]              = "Moved Permanently";
	map[WSC_Found]                         = "Found";
	map[WSC_SeeOther]                      = "See Other";
	map[WSC_NotModified]                   = "Not Modified";
	map[WSC_UseProxy]                      = "Use Proxy";
	map[WSC_SwitchProxy]                   = "Switch Proxy";
	map[WSC_TemporaryRedirect]             = "Temporary Redirect";
	map[WSC_BadRequest]                    = "Bad Request";
	map[WSC_Unauthorized]                  = "Unauthorized";
	map[WSC_PaymentRequired]               = "Payment Required";
	map[WSC_Forbidden]                     = "Forbidden";
	map[WSC_NotFound]                      = "Not Found";
	map[WSC_MethodNotAllowed]              = "Method Not Allowed";
	map[WSC_NotAcceptable]                 = "Not Acceptable";
	map[WSC_ProxyAuthenticationRequired]   = "Proxy Authentication Required";
	map[WSC_RequestTimeout]                = "Request Time-out";
	map[WSC_Conflict]                      = "Conflict";
	map[WSC_Gone]                          = "Gone";
	map[WSC_LengthRequired]                = "Length Required";
	map[WSC_PreconditionFailed]            = "Precondition Failed";
	map[WSC_RequestEntityTooLarge]         = "Request Entity Too Large";
	map[WSC_RequestURITooLong]             = "Request-URI Too Long";
	map[WSC_UnsupportedMediaType]          = "Unsupported Media Type";
	map[WSC_RequestedRangeNotSatisfiable]  = "Requested range not satisfiable";
	map[WSC_ExpectationFailed]             = "Expectation Failed";
	map[WSC_ImATeapot]                     = "I'm a Teapot";
	map[WSC_TooManyConnections]            = "Too many connections";
	map[WSC_UnprocessableEntity]           = "Unprocessable Entity";
	map[WSC_Locked]                        = "Locked";
	map[WSC_FailedDependency]              = "Failed Dependency";
	map[WSC_UpgradeRequired]               = "Upgrade Required";
	map[WSC_InternalServerError]           = "Internal Server Error";
	map[WSC_NotImplemented]                = "Not Implemented";
	map[WSC_BadGateway]                    = "Bad Gateway";
	map[WSC_ServiceUnavailable]            = "Service Unavailable";
	map[WSC_GatewayTimeout]                = "Gateway Time-out";
	map[WSC_HTTPVersionNotSupported]       = "HTTP Version not supported";
	map[WSC_VariantAlsoNegotiates]         = "Variant Also Negotiates";
	map[WSC_InsufficientStorage]           = "Insufficient Storage";
	map[WSC_BandwidthLimitExceeded]        = "Bandwidth Limit Exceeded";
	map[WSC_NotExtended]                   = "Not Extended";
	return map;
}

TWebAuthMap fillWebAuthTypes() {
	TWebAuthMap map;
	map[HAT_DIGEST_NONE]   = "NONE";
	map[HAT_DIGEST_MD5]    = "MD5";
	map[HAT_DIGEST_SHA256] = "SHA256";
	return map;
}


TWebAuthMap webAuthList = fillWebAuthTypes();
TWebStatusMap webStatusList = fillStatusMessages();


EHttpAuthType app::getWebAuthType(const std::string& type) {
	if (!webAuthList.empty()) {
		TWebAuthMap::const_iterator it = webAuthList.begin();
		do {
			if (0 == util::strcasecmp(type, it->second))
				return it->first;
			it++;
		} while (it != webAuthList.end());
	}
	// No message found
	return HAT_DIGEST_NONE;
}

std::string app::getWebAuthType(const EHttpAuthType type) {
	if (!webAuthList.empty()) {
		TWebAuthMap::const_iterator it = webAuthList.find(type);
		if (it != webAuthList.end())
			return it->second;
	}
	// No message found
	return std::to_string((size_s)type);
}


std::string app::getWebStatusMessage(const EWebStatusCode status) {
	if (!webStatusList.empty()) {
		TWebStatusMap::const_iterator it = webStatusList.find(status);
		if (it != webStatusList.end())
			return it->second;
	}
	// No message found
	return std::to_string((size_s)status);
}

EWebUserAgent app::guessUserAgent(const char *value) {
	// std::cout << "app::guessUserAgent() \"" << util::strToStr(value, "unknown") << std::endl;
	if (util::assigned(strstr(value, "Edge")))
		return WUA_MSEDGE;
	if (util::assigned(strstr(value, "Edg")))
		return WUA_MSCHROME;
	if (util::assigned(strstr(value, "Firefox")))
		return WUA_FIREFOX;
	if (util::assigned(strstr(value, "Chrome")))
		return WUA_CHROMIUM;
	if (util::assigned(strstr(value, "Safari")))
		return WUA_SAFARI;
	if (util::assigned(strstr(value, "Trident")))
		return WUA_MSIE;
	if (util::assigned(strstr(value, "MSIE")))
		return WUA_MSIE;
	if (util::assigned(strstr(value, "Opera")))
		return WUA_OPERA;
	if (util::assigned(strstr(value, "Netscape")))
		return WUA_NETSCAPE;
	if (util::assigned(strstr(value, "Navigator")))
		return WUA_NETSCAPE;
	if (util::assigned(strstr(value, "curl")))
		return WUA_CURL;
	if (util::assigned(strstr(value, "db Application")))
		return WUA_APPLICATION;
	return WUA_UNKNOWN;
}

std::string app::userAgentToStr(const EWebUserAgent agent) {
	std::string retVal = "Unknown user agent";
	switch (agent) {
		case WUA_MSEDGE:
			retVal = "Microsoft Edge Classic";
			break;
		case WUA_MSCHROME:
			retVal = "Microsoft Edge Chromium";
			break;
		case WUA_MSIE:
			retVal = "Microsoft Internet Explorer";
			break;
		case WUA_FIREFOX:
			retVal = "Mozilla Firefox";
			break;
		case WUA_CHROMIUM:
			retVal = "Google Chrome";
			break;
		case WUA_SAFARI:
			retVal = "Apple Safari";
			break;
		case WUA_OPERA:
			retVal = "Opera Browser";
			break;
		case WUA_NETSCAPE:
			retVal = "Netscape Navigator";
			break;
		case WUA_CURL:
			retVal = "cURL Client Toolkit";
			break;
		case WUA_APPLICATION:
			retVal = "db Application Web Client";
			break;
		default:
			break;
	}
	return retVal;
}

EHttpMethod app::getHttpMethod(const std::string& method) {
	return getHttpMethod(method.c_str());
}

EHttpMethod app::getHttpMethod(const char *const method) {
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_GET))
		return HTTP_GET;
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_POST))
		return HTTP_POST;
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_HEAD))
		return HTTP_HEAD;
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_PUT))
		return HTTP_PUT;
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_CONNECT))
		return HTTP_CONNECT;
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_DELETE))
		return HTTP_DELETE;
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_OPTIONS))
		return HTTP_OPTIONS;
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_PATCH))
		return HTTP_PATCH;
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_TRACE))
		return HTTP_TRACE;
	if (0 == strcasecmp(method, MHD_HTTP_METHOD_SUBSCRIBE))
		return HTTP_SUBSCRIBE;
	return HTTP_UNKNOWN;
}

std::string app::httpMethodToStr(const EHttpMethod method) {
	switch (method) {
		case HTTP_GET:		 return MHD_HTTP_METHOD_GET;
		case HTTP_POST:		 return MHD_HTTP_METHOD_POST;
		case HTTP_HEAD:		 return MHD_HTTP_METHOD_HEAD;
		case HTTP_PUT:		 return MHD_HTTP_METHOD_PUT;
		case HTTP_CONNECT:	 return MHD_HTTP_METHOD_CONNECT;
		case HTTP_DELETE:	 return MHD_HTTP_METHOD_DELETE;
		case HTTP_OPTIONS:	 return MHD_HTTP_METHOD_OPTIONS;
		case HTTP_PATCH:	 return MHD_HTTP_METHOD_PATCH;
		case HTTP_TRACE:	 return MHD_HTTP_METHOD_TRACE;
		case HTTP_SUBSCRIBE: return MHD_HTTP_METHOD_SUBSCRIBE;
		default:			 break;
	}
	return "Unknown method";
}
