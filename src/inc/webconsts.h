/*
 * webconsts.h
 *
 *  Created on: 26.01.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef WEBCONSTS_H_
#define WEBCONSTS_H_

#include "microhttpd/microhttpd.h"
#include "version.h"
#include "gcc.h"

/**
 * Current version of the library.
 * 0x01093001 = 1.9.30-1.
 */
#ifdef MHD_VERSION
#  define MHD_MAJOR   (((((MHD_VERSION & 0xFF000000) >> 24) / 16) * 10) + (((MHD_VERSION & 0xFF000000) >> 24) % 16))
#  define MHD_MINOR   (((((MHD_VERSION & 0x00FF0000) >> 16) / 16) * 10) + (((MHD_VERSION & 0x00FF0000) >> 16) % 16))
#  define MHD_PATCH   (((((MHD_VERSION & 0x0000FF00) >> 8 ) / 16) * 10) + (((MHD_VERSION & 0x0000FF00) >> 8 ) % 16))
#  define MHD_RELEASE (((( MHD_VERSION & 0x000000FF)        / 16) * 10) + (( MHD_VERSION & 0x000000FF)        % 16))
#endif

#ifndef STR_CONST_LEN
#  define STR_CONST_LEN(str) ((size_t)(sizeof(str) / sizeof(str[0]) - 1))
#endif

namespace app {

STATIC_CONST int DEFAULT_USER_LEVEL = 3;

STATIC_CONST char INDEX_PAGE[] = "index.html;index.htm;home.html;home.htm";
STATIC_CONST char GREETER_URL[] = "/login.html";
STATIC_CONST char DEFAULT_WEB_ACTION[] = "DEFAULT_WEB_ACTION";
STATIC_CONST char ALLOWED_LIST[] = "0.0.0.0;127.0.0.1;::1;fd66:1967:0:1;192.168.200;192.168.201";
STATIC_CONST char AJAX_RESPONSE_FILE[] = "/ajax/response.html";
STATIC_CONST char PARSER_FILES[] = \
		"bootstrap-table-na-DB.min.js;bootstrap-table-na-DB.js;" \
		"credentialmanager.min.js;credentialmanager.js;" \
		"credentialdialog.min.js;credentialdialog.js;" \
		"alertdialogs.min.js;alertdialogs.js;" \
		"credentials.min.js;credentials.js;" \
		"statistics.min.js;statistics.js;" \
		"mainmenu.min.js;mainmenu.js;" \
		"network.min.js;network.js;" \
		"sysconf.min.js;sysconf.js;" \
		"appconf.min.js;appconf.js";

STATIC_CONST char URI_CLIENT_ADDRESS[] = "_URI_CLIENT_ADDRESS_";
STATIC_CONST char URI_MIME_TYPE[] = "_URI_MIME_TYPE_";
STATIC_CONST char URI_REQUEST_URL[] = "_URI_REQUEST_URL_";
STATIC_CONST char URI_REQUEST_LINK[] = "_URI_REQUEST_LINK_";
STATIC_CONST char URI_REQUEST_METHOD[] = "_URI_REQUEST_METHOD_";
STATIC_CONST char URI_REQUEST_ACTION[] = "_URI_REQUEST_ACTION_";
STATIC_CONST char URI_RESPONSE_ZIPPED[] = "_URI_RESPONSE_ZIPPED_";
STATIC_CONST char URI_RESPONSE_CACHED[] = "_URI_RESPONSE_CAHCHED_";

STATIC_CONST char SESSION_ID[] = "SESSION_ID";
STATIC_CONST char SESSION_REMOTE_HOST[] = "REMOTE_HOST";
STATIC_CONST char SESSION_USER_NAME[] = "SESSION_USER_NAME";
STATIC_CONST char SESSION_USER_AUTH[] = "SESSION_USER_AUTH";
STATIC_CONST char SESSION_USER_LEVEL[] = "SESSION_USER_LEVEL";
STATIC_CONST char SESSION_USER_PASSWORD[] = "SESSION_USER_PASSWORD";

STATIC_CONST size_t SESSION_ID_SIZE = STR_CONST_LEN(SESSION_ID);
STATIC_CONST size_t SESSION_REMOTE_HOST_SIZE = STR_CONST_LEN(SESSION_REMOTE_HOST);
STATIC_CONST size_t SESSION_USER_NAME_SIZE = STR_CONST_LEN(SESSION_USER_NAME);
STATIC_CONST size_t SESSION_USER_PASSWORD_SIZE = STR_CONST_LEN(SESSION_USER_PASSWORD);

// ITERATOR_BUFFER_SIZE is also the chunk size on POSTed upload from client
STATIC_CONST size_t FULL_RANGE_MIN_SIZE = 128 * 1024;
STATIC_CONST size_t FULL_RANGE_MAX_SIZE = 312 * 1024;
STATIC_CONST size_t ITERATOR_BUFFER_SIZE = 8 * 1024;
STATIC_CONST size_t RESPONSE_BLOCK_SIZE = 16 * 1024;
STATIC_CONST size_t HEADER_EXPIRE_TIME = 60 * 60 * 24;

STATIC_CONST unsigned int CONNECTION_TIMEOUT = 60 * 60;
STATIC_CONST unsigned int CONNECTIONS_PER_CPU = 32;
STATIC_CONST app::TTimerDelay BUFFER_TIMER_DELAY = 1000;
STATIC_CONST app::TTimerDelay STATISTICS_TIMER_DELAY = 3000;

STATIC_CONST size_t COOKIE_EXPIRE_TIME = 3 * 60 * 60 * 24;
STATIC_CONST size_t COOKIE_MAX_AGE = 60 * 60 * 24 * 365 * 10;
STATIC_CONST char SESSION_COOKIE[] = "session-cookie";
STATIC_CONST char APPLICATION_COOKIE[] = "application-cookies";
STATIC_CONST char LANGUAGE_COOKIE[] = "system-language";
STATIC_CONST char USER_COOKIE[] = "user-level";
STATIC_CONST char USER_AGENT[] = "User-Agent";
STATIC_CONST char USERNAME_IDENT[] = "41F709FA-448A-8F72-1E64-739930F9D262";
STATIC_CONST char PASSWORD_IDENT[] = "C7B09D66-D4EE-CF5F-7D34-8F0242EAFD0B";

STATIC_CONST char AUTH_REALM[] = "dbApplications (c) 2015";
STATIC_CONST char MULTIPART_FORM_DATA[] = MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA;
STATIC_CONST size_t MULTIPART_FORM_DATA_SIZE = STR_CONST_LEN(MULTIPART_FORM_DATA);
STATIC_CONST char ENCODING_FORM_URLENCODED[] = MHD_HTTP_POST_ENCODING_FORM_URLENCODED;
STATIC_CONST size_t ENCODING_FORM_URLENCODED_SIZE = STR_CONST_LEN(ENCODING_FORM_URLENCODED);

STATIC_CONST char XML_HTTP_REQUEST[] = "XMLHttpRequest";
STATIC_CONST size_t XML_HTTP_REQUEST_SIZE = STR_CONST_LEN(XML_HTTP_REQUEST);
STATIC_CONST char XML_HTTP_REQUEST_HEADER[] = "X-Requested-With";
STATIC_CONST size_t XML_HTTP_REQUEST_HEADER_SIZE = STR_CONST_LEN(XML_HTTP_REQUEST_HEADER);

STATIC_CONST char UPGRADE_HTTP_REQUEST[] = "upgrade";
STATIC_CONST size_t UPGRADE_HTTP_REQUEST_SIZE = STR_CONST_LEN(UPGRADE_HTTP_REQUEST);

STATIC_CONST char UPGRADE_HTTP_PROTOCOL[] = "websocket";
STATIC_CONST size_t UPGRADE_HTTP_PROTOCOL_SIZE = STR_CONST_LEN(UPGRADE_HTTP_PROTOCOL);

STATIC_CONST char HTTP_WEB_SOCKET_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
STATIC_CONST size_t HTTP_WEB_SOCKET_GUID_SIZE = STR_CONST_LEN(HTTP_WEB_SOCKET_GUID);


STATIC_CONST char HTTP_WEB_SOCKET_KEY[] = "Sec-WebSocket-Key";
STATIC_CONST size_t HTTP_WEB_SOCKET_KEY_SIZE = STR_CONST_LEN(HTTP_WEB_SOCKET_KEY);

STATIC_CONST char HTTP_WEB_SOCKET_ACCEPT[] = "Sec-WebSocket-Accept";
STATIC_CONST size_t HTTP_WEB_SOCKET_ACCEPT_SIZE = STR_CONST_LEN(HTTP_WEB_SOCKET_ACCEPT);

STATIC_CONST char HTTP_WEB_SOCKET_PROTOCOL[] = "Sec-WebSocket-Protocol";
STATIC_CONST size_t HTTP_WEB_SOCKET_PROTOCOL_SIZE = STR_CONST_LEN(HTTP_WEB_SOCKET_PROTOCOL);

STATIC_CONST char HTTP_WEB_SOCKET_VERSION[] = "Sec-WebSocket-Version";
STATIC_CONST size_t HTTP_WEB_SOCKET_VERSION_SIZE = STR_CONST_LEN(HTTP_WEB_SOCKET_VERSION);


STATIC_CONST size_t HTTP_DIGEST_SIZE = 34;
STATIC_CONST size_t HTTP_NONCE_SIZE = 256;
STATIC_CONST size_t HTTP_MAX_TOKEN_LENGTH = 128;
STATIC_CONST size_t HTTP_MAX_RANGE_VALUE_LENGTH = 255;


STATIC_CONST char PAGE_JSON_OK[] =
		"{\n" \
		"  \"Error\" : 200,\n" \
		"  \"Status\" : \"Request accepted\",\n" \
		"  \"Method\" : \"%s\",\n" \
		"  \"URL\" : \"%s\",\n" \
		"  \"Client\" : \"%s\"\n" \
		"}";


STATIC_CONST char PAGE_HTTP_OK[] =
		"<!DOCTYPE html>\n" \
		"<html lang=\"en\"><head>\n" \
		"<meta charset=\"utf-8\">\n" \
		"<title>200 Request accepted</title></head>\n" \
		"<body><h1>200 Request accepted</h1>\n" \
		"<p>Request for URL \"%s\" was processed.</p><hr>\n" \
		"<address>db Application TWebServer (%s) GNU Libmicrohttpd (%s) Client address [%s]</address>\n" \
		"</body></html>\n";

STATIC_CONST char PAGE_HTTP_FORBIDDEN[] =
		"<!DOCTYPE html>\n" \
		"<html lang=\"en\"><head>\n" \
		"<meta charset=\"utf-8\">\n" \
		"<title>%d Forbidden</title></head>\n" \
		"<body><h1>%d Forbidden</h1>\n" \
		"<p>Access denied to URL \"%s\"</p><hr>\n" \
		"<address>db Application TWebServer (%s) GNU Libmicrohttpd (%s) Client address [%s]</address>\n" \
		"</body></html>\n";

STATIC_CONST char PAGE_HTTP_NOT_FOUND[] =
		"<!DOCTYPE html>\n" \
		"<html lang=\"en\"><head>\n" \
		"<meta charset=\"utf-8\">\n" \
		"<title>404 Not Found</title></head>\n" \
		"<body><h1>404 Not found</h1>\n" \
		"<p>The requested URL \"%s\" was not found on this server.</p><hr>\n" \
		"<address>db Application TWebServer (%s) GNU Libmicrohttpd (%s) Client address [%s]</address>\n" \
		"</body></html>\n";

STATIC_CONST char PAGE_HTTP_INTERNAL_SERVER_ERROR[] =
		"<!DOCTYPE html>\n" \
		"<html lang=\"en\"><head>\n" \
		"<meta charset=\"utf-8\">\n" \
		"<title>%d Internal error</title></head>\n" \
		"<body><h1>%d Internal error</h1>\n" \
		"<p>Internal server error (%d) \"%s\"</p><hr>\n" \
		"<address>db Application TWebServer (%s) GNU Libmicrohttpd (%s) Client address [%s]</address>\n" \
		"</body></html>\n";

STATIC_CONST char PAGE_HTTP_NOT_IMPLEMENTED[] =
		"<!DOCTYPE html>\n" \
		"<html lang=\"en\"><head>\n" \
		"<meta charset=\"utf-8\">" \
		"<title>%d Not implemented</title></head>\n" \
		"<body><h1>%d Not implemented</h1>\n" \
		"<p>The requested service or method \"%s\" is not implemented.</p><hr>\n" \
		"<address>db Application TWebServer (%s) GNU Libmicrohttpd (%s) Client address [%s]</address>\n" \
		"</body></html>\n";

STATIC_CONST char PAGE_HTTP_NOT_AVAILABLE[] =
		"<!DOCTYPE html>\n" \
		"<html lang=\"en\"><head>\n" \
		"<meta charset=\"utf-8\">" \
		"<title>%d Not available</title></head>\n" \
		"<body><h1>%d Service not available</h1>\n" \
		"<p>The requested service for URL \"%s\" is not available.</p><hr>\n" \
		"<address>db Application TWebServer (%s) GNU Libmicrohttpd (%s) Client address [%s]</address>\n" \
		"</body></html>\n";


// RSA key with 2048 bits
STATIC_CONST char HTTPS_KEY[] =
		"-----BEGIN RSA PRIVATE KEY-----\n"
		"MIIEowIBAAKCAQEAvVBvu7q/yqVSECUAYfAzyEeoFUu5/uehMS+30GlJEmJO/VS8\n"
		"Ig2oq9oxpyffCV7ILSmcK3d7hLwp+7Si9OkkFWWDsiHnn9D9YkETY1PmP7tY9GGk\n"
		"cPHkD4gO5/yUymlPOJ68eWJZwv/Z+Ac5/DIa2N3qz4OXIU/Eov9YdZc2QFOuwKnz\n"
		"AHKef8fphfbfQrKcszFvKaQLn8FOw5/tnfPvxttuJLtGiumGHK/xTUeOnUPr4EbP\n"
		"Is2C1Hf9QF4tdwbVZ5NSSmULWWcHiA5qRh2SgQ8qdYlNmdhWODXSyk9yCNmr5FNQ\n"
		"hcgSGCdQ7cltTSVRP8+mnWvq8MRJT87o1k6iaQIDAQABAoIBAH9Exfi0kR8QiNyl\n"
		"o14z9vvbgFngsMd2vFyusaoAPcmIIYYZIujZudzeMKcpHL3V5EjIQl7OUlFnlenL\n"
		"BAoVedaQijqEpIxCGTWmffw2eQG7Vw/jXIM5epIea7b1jKmOpl1wCVCpF6MKEWS2\n"
		"pvquTHIiriqXUlBoqc7STouu/h+7eae3yY7US7jDVPSBuu2A/3VUfWE5EMo4BEkP\n"
		"X6g7qWpcDKbLgVVbCpEzHc5MnDUkLKy9RsIwT6021yIBf5Ub38Hz7vtxfI1sIhVR\n"
		"lbfxyh/aozR+aoZQc/e83rCXv9qVpLrzHfqFyLrH6KD6QipiE9qJRVKW3Zpp35s7\n"
		"lYdfO4kCgYEA6a55yk0wRoN1SmdiZtR+T57K9oH2uSK9L3yTjjloOzB2puKlJBcE\n"
		"FE82OUSVcZ/73q8zveyKPXA07Nl0y7bMZLff0dWIDu7Z0FzJZaFiyVqd4ZEZdYeS\n"
		"JPJP0KyEb2H6VmnMfb1jefKqkd6V7cuYcvnFQzUDBo53iAixYdKQ8zcCgYEAz2Ut\n"
		"X8fIY6Win1pR3KSpNAXW46DdUSruleArULe4tEMP8Fhi5weBLbmj4sSYEUnWs2/i\n"
		"8okXZAoCisFL5VUb0E4n6n2Th4bGcL4sVDok7pVoTqBl2wrI2KNiiXJSaorDljpG\n"
		"WCvV6lyu/M73bBhd5QWYaPC2PZ1Tfr2qlEfBJ18CgYANMko0b3l7ce8MvZvj/LoJ\n"
		"WwlRNHOvbtPKO7nFfV5ygUEiGYiD6jzTvMluIH5kBUnfAHvmjNYdtBl5Cqq62l7e\n"
		"jTe5jNp7JWftiV/iOmPuxQxHcb9DUN2i8oApY6Sy+ZB+ksj2jNxyRY72X+CNpkK8\n"
		"s3g9XGAIXcFIUF1cDd0brwKBgFtNu7ATEBFudi2ZYbi1dRhGCdiklUqKkAbDbc5X\n"
		"U6VocLfq8X+sOh6bP58x1ZCm5TKR62PDHt0X2w6jEnqgAWKvRbtiFXTwzKQN1Q4v\n"
		"mtq+Q/F5g93u3YUiSNshzU7CUGDuvtFKWx6WNyNtKlgYUh3lXLe2YUS24m9FqLcm\n"
		"784ZAoGBAJrKv4okVCNc83/ilEc1i30mTOHCGJw/v/VynxXzYq6AIKiYg8g4AFZ7\n"
		"E3kF7sFtxAYsffnmS+eLYfqb10+ZQPbDMAM6i8xq15CgkgyQOpwt0dvR7MTHd0qX\n"
		"noCfiZSGHHR9sypokzIMRRwnCTTY5SG4lQekzwkUAFCVk7SSYX2s\n"
		"-----END RSA PRIVATE KEY-----\n";

// Certificate with 2048 bits
STATIC_CONST char HTTPS_CERT[] =
		"-----BEGIN CERTIFICATE-----\n"
		"MIID0TCCArkCFDsRcjDQ/WuzGCa7xBas6NUUccsjMA0GCSqGSIb3DQEBCwUAMIGk\n"
		"MQswCQYDVQQGEwJERTEMMAoGA1UECAwDTlJXMRAwDgYDVQQHDAdIRVJGT1JEMRUw\n"
		"EwYDVQQKDAxEYiBTT0xVVElPTlMxGDAWBgNVBAsMD0RCIEFQUExJQ0FUSU9OUzEb\n"
		"MBkGA1UEAwwSd3d3LmRicmlua21laWVyLmRlMScwJQYJKoZIhvcNAQkBFhh3ZWJt\n"
		"YXN0ZXJAZGJyaW5rbWVpZXIuZGUwHhcNMjAwNjIyMTgyODExWhcNNDAwNjE3MTgy\n"
		"ODExWjCBpDELMAkGA1UEBhMCREUxDDAKBgNVBAgMA05SVzEQMA4GA1UEBwwHSEVS\n"
		"Rk9SRDEVMBMGA1UECgwMRGIgU09MVVRJT05TMRgwFgYDVQQLDA9EQiBBUFBMSUNB\n"
		"VElPTlMxGzAZBgNVBAMMEnd3dy5kYnJpbmttZWllci5kZTEnMCUGCSqGSIb3DQEJ\n"
		"ARYYd2VibWFzdGVyQGRicmlua21laWVyLmRlMIIBIjANBgkqhkiG9w0BAQEFAAOC\n"
		"AQ8AMIIBCgKCAQEAvVBvu7q/yqVSECUAYfAzyEeoFUu5/uehMS+30GlJEmJO/VS8\n"
		"Ig2oq9oxpyffCV7ILSmcK3d7hLwp+7Si9OkkFWWDsiHnn9D9YkETY1PmP7tY9GGk\n"
		"cPHkD4gO5/yUymlPOJ68eWJZwv/Z+Ac5/DIa2N3qz4OXIU/Eov9YdZc2QFOuwKnz\n"
		"AHKef8fphfbfQrKcszFvKaQLn8FOw5/tnfPvxttuJLtGiumGHK/xTUeOnUPr4EbP\n"
		"Is2C1Hf9QF4tdwbVZ5NSSmULWWcHiA5qRh2SgQ8qdYlNmdhWODXSyk9yCNmr5FNQ\n"
		"hcgSGCdQ7cltTSVRP8+mnWvq8MRJT87o1k6iaQIDAQABMA0GCSqGSIb3DQEBCwUA\n"
		"A4IBAQCGwxTF12pGuMw2UytSN7HzDvknI1usYRJ3aAVSiv2+LtdSnX1CKXY90TTP\n"
		"lrvcviTFN7c14cK1XECMKYKlkEi5zLhV8T454RwPXZ51yIt6ChQ1gCobaPH6gaWG\n"
		"6VTZNFgaw9MGIC+3qgAT86jEMZiw9mKICSmBpY8VbOF3G0CkM2CnrOm1bQSfTir/\n"
		"/r0rWprhtny0VXaifsENI9+I4Eeu+axcYpXjWTMYJhIjDeG8EVkr4C/+pLfOw5FQ\n"
		"GrS8ZGdO6MaFjfA+S8XSkpfH8yPpnngiMlJvcf8QQnimEMunWpfuduTzDGwbzHI0\n"
		"l0IdsdoL40/a2W8/JIuV8f3fV+/S\n"
		"-----END CERTIFICATE-----\n";

// Diffie-Hellman prime with 2048 bits
STATIC_CONST char HTTPS_PARAMS[] =
		"-----BEGIN DH PARAMETERS-----\n"
		"MIIBCAKCAQEA8aGhYzTWNzBHBiLmg9Ssy7ONrRNDhXP2H4xCJZf8YsLL8BtzmEkk\n"
		"t1WPwtke61HDXmFOJ5Z+FnGOUOj7He5LDRdmk1KVVunzA2agHMUlPs/utSaPAQrb\n"
		"vn7TeP45XjkPy2plNnIx87/Svv7cXMxYElzysH2SkF3Ku8gOe4moPnCGGiGA7Mcm\n"
		"GKJakmt3wcYC1xMAPyQ2AzBX3RyHWX9ZDiujc0NTXpvMjQ+tlwl9k1xXqhaj6rNF\n"
		"wyuLFMJIsx0UCJNTgx82/oHEdJZ7SSsZWANoPo0GbRJTulRPGGTRhwiKVOzSXYzt\n"
		"pAyk81QQUsOpVxMTmkZbWFLQ/2aDZ55jEwIBAg==\n"
		"-----END DH PARAMETERS-----\n";


// Working for all gnutls versions and browsers, but has some regression to weak ciphers!
STATIC_CONST char STD_TLS_PRIORITY[] =
		"NORMAL:-MD5:-VERS-SSL3.0:+VERS-TLS1.1:+VERS-TLS1.0";

// Not working with MSIE, using STD_TLS_PRIORITY instead!
STATIC_CONST char EXT_TLS_PRIORITY[] =
		"NONE:+SHA512:+SHA384:+SHA256:+DHE-RSA:+DHE-PSK:+DHE-DSS:+AES-256-CBC:+AES-128-CBC:"
		"+3DES-CBC:+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0:+COMP-NULL:+SHA1:+SIGN-ALL";

// Curved DH not yet tested with gnutls >= 3.x.x
STATIC_CONST char CRV_TLS_PRIORITY[] =
		"NONE:+SHA512:+SHA384:+SHA256:+DHE-RSA:+DHE-PSK:+DHE-DSS:+AES-256-CBC:+AES-128-CBC:"
		"+3DES-CBC:+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0:+COMP-NULL:+ECDHE-RSA:+SHA1:+SIGN-ALL";


} /* namespace app */

#endif /* WEBCONSTS_H_ */
