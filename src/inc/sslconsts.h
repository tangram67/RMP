/*
 * sslconsts.h
 *
 *  Created on: 29.05.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef SSLCONSTS_H_
#define SSLCONSTS_H_

#include <string.h>

namespace util {

// RSA key with 1024 bits
// It is recommended to use 2048 external key file!
const char TLS_KEY[] =
		"-----BEGIN RSA PRIVATE KEY-----\n"
		"MIICXQIBAAKBgQC9hmYXYIRnmt/8AKMsEUs4OTXlsjal/Ie+KpgWtBaSDVN5zPLr\n"
		"YWyzm4mI/JBW4Sm34vIS3dyAl9vad6iLw3Wyd3rEEh5zC3IVfuixId95KNnnAKFA\n"
		"464eNuSEiSSi0MDsm/5K04JCqy9gWQgXN6DCLeq1K6bqHOSiH2gGWz2JuQIDAQAB\n"
		"AoGAW6b8BgAR57x45vgz8KKoWpcoHl1xmhGmX2tfw1Lxu02geb2IGBY0KCNmfo2N\n"
		"C8a1kwy3/jG2aaWGl37YTkaGyqL9KPb6ou2ZguxaVFW7SUnzGZvI8I0GziXPTE2v\n"
		"r2sdgiQApEEK+oHC3H4yp9gzKsi6zuLvE6ao+8YhF5asSBECQQDr0YrmUMdJVaGG\n"
		"5jzHRjFPnxm0v6GPFf1dgWHQtAknxtbFBh9r9AnYV56wdL3dGseomX3WrAIH3xUQ\n"
		"yzaiDeyNAkEAzb6hCdlNoyu5K3YP5tXOHmsZ0MBt42z6GLHuP8tj8kO73EMNzZSb\n"
		"WM4X/5O6dE5+ko0tsIO5/GmaeQ8Pz+ak3QJBAK+wK6WvpOmT7IWOXtWwC+jgBczN\n"
		"wFXT8jGJxRAyMWf7EeMzzpEk/Xi6vrWEJDfoTbvkrwYSnGi04QLkA030xbkCQGsI\n"
		"luJ2x+rxHh28B73A0MAGW6G72e8MjHc4aYeKme44yoxl3dJrUv26CcYN5lwHUdWP\n"
		"2IsRnDnx/kioS0OO64kCQQDdKhFpTsldYB+aF0lYZf2zMU+MONYpQ5uFGHIau8YL\n"
		"BWxA636tU0ZgY6ngbU+WQT1X7hA7wKH6tQ6BlwOnMkOq\n"
		"-----END RSA PRIVATE KEY-----\n";

static const size_t TLS_KEY_SIZE = strlen(TLS_KEY);


// Certificate with 1024 bits
// It is recommended to use 2048 external certificate!
const char TLS_CERT[] =
		"-----BEGIN CERTIFICATE-----\n"
		"MIICuzCCAiQCCQC3e2ZZbEVLijANBgkqhkiG9w0BAQUFADCBoTELMAkGA1UEBhMC\n"
		"REUxEDAOBgNVBAgTB0dFUk1BTlkxEDAOBgNVBAcTB0hFUkZPUkQxFzAVBgNVBAoT\n"
		"DkRCUklOS01FSUVSLkRFMQ8wDQYDVQQLEwZEQkhPTUUxGzAZBgNVBAMTElRMUy5E\n"
		"QklOVFJBTkVULkxBTjEnMCUGCSqGSIb3DQEJARYYV0VCTUFTVEVSQERCUklOS01F\n"
		"SUVSLkRFMB4XDTE2MDUyOTA3NDMxNFoXDTM2MDUyNDA3NDMxNFowgaExCzAJBgNV\n"
		"BAYTAkRFMRAwDgYDVQQIEwdHRVJNQU5ZMRAwDgYDVQQHEwdIRVJGT1JEMRcwFQYD\n"
		"VQQKEw5EQlJJTktNRUlFUi5ERTEPMA0GA1UECxMGREJIT01FMRswGQYDVQQDExJU\n"
		"TFMuREJJTlRSQU5FVC5MQU4xJzAlBgkqhkiG9w0BCQEWGFdFQk1BU1RFUkBEQlJJ\n"
		"TktNRUlFUi5ERTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAvYZmF2CEZ5rf\n"
		"/ACjLBFLODk15bI2pfyHviqYFrQWkg1Teczy62Fss5uJiPyQVuEpt+LyEt3cgJfb\n"
		"2neoi8N1snd6xBIecwtyFX7osSHfeSjZ5wChQOOuHjbkhIkkotDA7Jv+StOCQqsv\n"
		"YFkIFzegwi3qtSum6hzkoh9oBls9ibkCAwEAATANBgkqhkiG9w0BAQUFAAOBgQAh\n"
		"d8uneGeDgJFVu+CShI2IsH9CP8U8dEIJz/WxGz0VFjNrqOAGnHMPIfgCbXBxiBf5\n"
		"qfYaKtmBKalwob3R2QUkwoY3ljgZXnoL+KCuT9FqQImVNLujqMHzXMgbNMLbEJOY\n"
		"TvBizh698x9Rs7Hm+Cbm6eJa6Gj5XSXtnRyYfhX2dw==\n"
		"-----END CERTIFICATE-----\n";

static const size_t TLS_CERT_SIZE = strlen(TLS_CERT);


// Diffie-Hellman "prime" with 1024 bits
// It is strongly (!) recommended to use 2048 external DH parameters!
const char TLS_DH_PARAMS[] =
		"-----BEGIN DH PARAMETERS-----\n"
		"MIIBCAKCAQEA8aGhYzTWNzBHBiLmg9Ssy7ONrRNDhXP2H4xCJZf8YsLL8BtzmEkk\n"
		"t1WPwtke61HDXmFOJ5Z+FnGOUOj7He5LDRdmk1KVVunzA2agHMUlPs/utSaPAQrb\n"
		"vn7TeP45XjkPy2plNnIx87/Svv7cXMxYElzysH2SkF3Ku8gOe4moPnCGGiGA7Mcm\n"
		"GKJakmt3wcYC1xMAPyQ2AzBX3RyHWX9ZDiujc0NTXpvMjQ+tlwl9k1xXqhaj6rNF\n"
		"wyuLFMJIsx0UCJNTgx82/oHEdJZ7SSsZWANoPo0GbRJTulRPGGTRhwiKVOzSXYzt\n"
		"pAyk81QQUsOpVxMTmkZbWFLQ/2aDZ55jEwIBAg==\n"
		"-----END DH PARAMETERS-----\n";

static const size_t TLS_DH_PARAMS_SIZE = strlen(TLS_DH_PARAMS);


// aXXX ... Authentication
// eXXX ... Encryption
// kXXX ... Key exchange
const char TLS_PREFERRED_CIPHERS[] = "HIGH:!aNULL:!PSK:!SRP:!MD5:!RC4";
const char TLS_HIGH_SECURE_CIPHERS[] = "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4";


} /* namespace util */

#endif /* SSLCONSTS_H_ */
