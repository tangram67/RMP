/*
 * ssl.cpp
 *
 *  Created on: 27.05.2016
 *      Author: Dirk Brinkmeier
 */

#include <iostream>
#include <openssl/ssl.h>
#include "exception.h"
#include "sysutils.h"
#include "fileutils.h"
#include "stringutils.h"
#include "encoding.h"
#include "ansi.h"
#include "ssl.h"
#include "gcc.h"

namespace util {

static bool SSLEngineInitialized = false;
static int SSLCallbackIndex = -1;


std::string getOpenSSLErrorMessage() {
	std::string s;
	BIO *bio = BIO_new(BIO_s_mem());
	ERR_print_errors(bio);
	char *buffer = nil;
	size_t size = BIO_get_mem_data(bio, &buffer);
	if (size)
		s.assign(buffer, size);
	BIO_free(bio);
	return util::trim(s);
}


std::string getOpenSSLErrorCode(int code) {
	std::string s = "SSL_ERROR_UNKNOWN";
	switch (code) {
	case SSL_ERROR_NONE:
		s = "SSL_ERROR_NONE";
		break;
	case SSL_ERROR_SSL:
		s = "SSL_ERROR_SSL";
		break;
	case SSL_ERROR_WANT_READ:
		s = "SSL_ERROR_WANT_READ";
		break;
	case SSL_ERROR_WANT_WRITE:
		s = "SSL_ERROR_WANT_WRITE";
		break;
	case SSL_ERROR_WANT_X509_LOOKUP:
		s = "SSL_ERROR_WANT_X509_LOOKUP";
		break;
	case SSL_ERROR_SYSCALL:
		s = "SSL_ERROR_SYSCALL";
		break;
	case SSL_ERROR_ZERO_RETURN:
		s = "SSL_ERROR_ZERO_RETURN";
		break;
	case SSL_ERROR_WANT_CONNECT:
		s = "SSL_ERROR_WANT_CONNECT";
		break;
	case SSL_ERROR_WANT_ACCEPT:
		s = "SSL_ERROR_WANT_ACCEPT";
		break;
	default:
		break;
	}
	return s;
}



TSSLInit::TSSLInit() {
	if (!util::SSLEngineInitialized) {
		SSL_load_error_strings();
		OpenSSL_add_all_digests();
		OpenSSL_add_ssl_algorithms();
		SSLCallbackIndex = SSL_get_ex_new_index(0, (void*)"TSSLConnection::callback::index", NULL, NULL, NULL);
		index = SSLCallbackIndex;
		util::SSLEngineInitialized = true;
	} else {
		index = SSLCallbackIndex;
	}	
}

TSSLInit::~TSSLInit() {
	if (util::SSLEngineInitialized) {
		EVP_cleanup();
		util::SSLEngineInitialized = false;
	}
}



TSSLBuffer::TSSLBuffer(const char* buffer, const size_t size) {
	init(buffer, size);
}

TSSLBuffer::~TSSLBuffer() {
	clear();
}


void TSSLBuffer::init(const char* buffer, const size_t size) {
	bio = BIO_new_mem_buf((void*)buffer, (int)size);
	if (!util::assigned(bio))
		throw util::app_error("TSSLBuffer::init() failed : " + util::quote(getOpenSSLErrorMessage()));
}

void TSSLBuffer::clear() {
	if (util::assigned(bio))
		BIO_free(bio);
	bio = nil;
}



TSSLCertificate::TSSLCertificate() {
	cert = nil;
}

TSSLCertificate::TSSLCertificate(TSSLConnection& connection) {
	init(connection);
}

TSSLCertificate::~TSSLCertificate() {
	clear();
}


void TSSLCertificate::init(TSSLConnection& connection) {
	cert = SSL_get_peer_certificate(connection());
}

void TSSLCertificate::clear() {
	if (util::assigned(cert))
		X509_free(cert);
	cert = nil;
}

void TSSLCertificate::invalidate() {
	subject.clear();
	issuer.clear();
}

void TSSLCertificate::assign(TSSLConnection& connection) {
	clear();
	init(connection);
}

const std::string& TSSLCertificate::getSubject() {
	if (isValid()) {
		if (subject.empty()) {
			char * p = X509_NAME_oneline(X509_get_subject_name(cert), nil, 0);
			subject = util::charToStr(p, "<none>");
			if (util::assigned(p))
				OPENSSL_free(p);
		}
		return subject;
	}
	return subject = "";
}

const std::string& TSSLCertificate::getIssuer() {
	if (isValid()) {
		if (issuer.empty() && isValid()) {
			char * p = X509_NAME_oneline(X509_get_issuer_name(cert), nil, 0);
			issuer = util::charToStr(p, "<none>");
			if (util::assigned(p))
				OPENSSL_free(p);
		}
		return issuer;
	}
	return issuer = "";
}


TSSLContext::TSSLContext() {
	prime();
}

TSSLContext::TSSLContext(const bool debug) {
	prime();
	this->debug = debug;
}

TSSLContext::TSSLContext(const EContextType type) {
	prime();
	init(type);
}

TSSLContext::~TSSLContext() {
	clear();
}

void TSSLContext::prime() {
	ctx = nil;
	crt  = nil;
	rsa = nil;
	dh = nil;
	ecdh = nil;
	type = ECT_UNKNOWN;
	debug = false;
}

void TSSLContext::init(const EContextType type) {
	if (debug) std::cout << "TSSLConnection::init() called." << std::endl;
	SSL_const SSL_METHOD *method = nil;
	switch(type) {
		case ECT_SERVER:
			if (debug) std::cout << "TSSLConnection::init() client method." << std::endl;
			method = SSLv23_server_method();
			break;
		case ECT_CLIENT:
			if (debug) std::cout << "TSSLConnection::init() server method." << std::endl;
			method = SSLv23_client_method();
			break;
		default:
			break;
	}

	if (util::assigned(method)) {

		// Create new context
		ctx = SSL_CTX_new(method);

		if (!util::assigned(ctx))
			throw util::app_error("TSSLContext::init()::SSL_CTX_new() failed : " + util::quote(getOpenSSLErrorMessage()));

		// Setting up new context was successful
		this->type = type;

	} else
		throw util::app_error("TSSLContext::init()::SSLv23_method() failed : " + util::quote(getOpenSSLErrorMessage()));

}

long TSSLContext::mode(long mode) {
	if (mode > 0) {
		long m = SSL_CTX_get_mode(ctx);
		m |= mode;
		return SSL_CTX_set_mode(ctx, m);
	}
	return SSL_CTX_get_mode(ctx);
}

long TSSLContext::option(long option) {
	if (option > 0) {
		long o = SSL_CTX_get_options(ctx);
		o |= option;
		return SSL_CTX_set_options(ctx, o);
	}
	return SSL_CTX_get_options(ctx);
}

#if defined SSL_HAS_PROTO_VERSION
bool TSSLContext::version(int version) {
	return (1 == SSL_CTX_set_min_proto_version(ctx, version));
}
#endif

bool TSSLContext::ciphers(const char* names) {
	return (1 == SSL_CTX_set_cipher_list(ctx, names));
}

void TSSLContext::clear() {
	if (util::assigned(ctx))
		SSL_CTX_free(ctx);
	if (util::assigned(crt))
		X509_free(crt);
	if (util::assigned(rsa))
		RSA_free(rsa);
	if (util::assigned(dh))
		DH_free(dh);
	if (util::assigned(ecdh))
		EC_KEY_free(ecdh);
	ctx = nil;
	crt = nil;
	rsa = nil;
	dh = nil;
	ecdh = nil;
}

void TSSLContext::initialize(const EContextType type) {
	if (debug) std::cout << "TSSLConnection::initialize() called." << std::endl;
	if (!util::assigned(ctx) && type != ECT_UNKNOWN)
		init(type);
}

void TSSLContext::setDebug(const bool debug) {
	this->debug = debug;
	if (debug) std::cout << "TSSLContext::setDebug() Debugging enabled." << std::endl;
};

bool TSSLContext::useEllipticCurveDiffieHellmannParameter() {
	if (!util::assigned(ecdh) && util::assigned(ctx)) {
#if defined SSL_HAS_ECDH_AUTO
		return (1 == SSL_CTX_set_ecdh_auto(ctx, 1));
#else
		ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
		if (util::assigned(ecdh)) {
			if (1 == SSL_CTX_set_tmp_ecdh(ctx, ecdh))
				return true;
			EC_KEY_free(ecdh);
			ecdh = nil;
		}
#endif
	}
	return util::assigned(ecdh);
}

bool TSSLContext::useDiffieHellmannParameter(const char* param, const size_t size) {
	if (!util::assigned(dh) && util::assigned(ctx) && util::assigned(param) && size > 0) {
		TSSLBuffer bio(param, size);
		dh = PEM_read_bio_DHparams(bio(), NULL, 0, NULL);
		if (util::assigned(dh)) {
			if (1 == SSL_CTX_set_tmp_dh(ctx, dh))
				return true;
			DH_free(dh);
			dh = nil;
		}
	}
	return util::assigned(dh);
}

bool TSSLContext::useCertificate(const char* cert, const size_t size) {
	if (!util::assigned(crt) && util::assigned(ctx) && util::assigned(cert) && size > 0) {
		TSSLBuffer bio(cert, size);
		crt = PEM_read_bio_X509(bio(), NULL, 0, NULL);
		if (util::assigned(crt)) {
			if (1 == SSL_CTX_use_certificate(ctx, crt))
				return true;
			 X509_free(crt);
			 crt = nil;
		}
	}
	return util::assigned(crt);
}

bool TSSLContext::usePrivateKey(const char* key, const size_t size) {
	if (!util::assigned(rsa) && util::assigned(ctx) && util::assigned(key) && size > 0) {
		TSSLBuffer bio(key, size);
		rsa = PEM_read_bio_RSAPrivateKey(bio(), NULL, 0, NULL);
		if (util::assigned(rsa)) {
			if (1 == SSL_CTX_use_RSAPrivateKey(ctx, rsa))
				return true;
			 RSA_free(rsa);
			 rsa = nil;
		}
	}
	return util::assigned(rsa);
}

bool TSSLContext::loadDiffieHellmannParameter(const std::string& fileName) {
	if (util::assigned(ctx) && !fileName.empty()) {
		dh = nil;
		TStdioFile file;
		file.open(fileName, "r");
		if (file.isOpen())
			dh = PEM_read_DHparams(file(), NULL, 0, NULL);
		if (util::assigned(dh)) {
			if (1 == SSL_CTX_set_tmp_dh(ctx, dh))
				return true;
			DH_free(dh);
			dh = nil;
		}
	}
	return false;
}

bool TSSLContext::loadCertificate(const std::string& fileName) {
	if (util::assigned(ctx) && !fileName.empty())
		return (1 == SSL_CTX_use_certificate_file(ctx, fileName.c_str(), SSL_FILETYPE_PEM));
	return false;
}

bool TSSLContext::loadPrivateKey(const std::string& fileName) {
	if (util::assigned(ctx) && !fileName.empty())
		return (1 == SSL_CTX_use_PrivateKey_file(ctx, fileName.c_str(), SSL_FILETYPE_PEM));
	return false;
}

bool TSSLContext::checkPrivateKey() {
	return (1 == SSL_CTX_check_private_key(ctx));
}

// See https://wiki.openssl.org/index.php/OpenSSL_1.1.0_Changes
#ifndef SSL_HAS_ACCESS_GETTERS
void DH_get0_pqg(const DH *dh, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g) {
	if (p != NULL)
		*p = dh->p;
	if (q != NULL)
		*q = dh->q;
	if (g != NULL)
		*g = dh->g;
}
#endif

int TSSLContext::checkDiffieHellmanParams(std::string& message) {
	int retVal = EXIT_ERROR;
	if (util::assigned(dh)) {

		int r, codes = 0;
		r = DH_check(dh, &codes);
		if (1 == r) {
			retVal = EXIT_SUCCESS;

			const BIGNUM *p = NULL;
			const BIGNUM *g = NULL;
			DH_get0_pqg(dh, &p, NULL, &g);
			if (p && g) {
				if(BN_is_word(g, DH_GENERATOR_2)) {
					long residue = BN_mod_word(p, 24);
					if(residue == 11 || residue == 23) {
						codes &= ~DH_NOT_SUITABLE_GENERATOR;
					}
				}
			}

			if (codes & DH_UNABLE_TO_CHECK_GENERATOR) {
				retVal = -DH_UNABLE_TO_CHECK_GENERATOR;
				message = "Failed to test generator.";
			}

			if (codes & DH_NOT_SUITABLE_GENERATOR) {
				retVal = -DH_NOT_SUITABLE_GENERATOR;
				message = "Generator is not a suitable.";
			}

			if (codes & DH_CHECK_P_NOT_PRIME) {
				retVal = -DH_CHECK_P_NOT_PRIME;
				message = "Parameter is not prime.";
			}

			if (codes & DH_CHECK_P_NOT_SAFE_PRIME) {
				retVal = -DH_CHECK_P_NOT_SAFE_PRIME;
				message = "Parameter is not a save prime.";
			}

		} else {
			message = "DH_check() failed.";
		}

	} else {
		message = "DH parameter not initialized.";
	}

	if (debug && retVal != EXIT_SUCCESS) std::cout << "TSSLConnection::checkDiffieHellmanParams() failed: " << message << std::endl;
	return retVal;
}

bool TSSLContext::verify(const EContextType type, TSSLVerifyCallback callback) {
	bool retVal = false;
	int option = SSL_VERIFY_PEER;
	if (util::assigned(ctx)) {
		switch(type) {
			case ECT_SERVER:
				//SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
				SSL_CTX_set_verify(ctx, option |= SSL_VERIFY_CLIENT_ONCE, callback);
				SSL_CTX_set_verify_depth(ctx, 4);
				retVal = true;
				break;
			case ECT_CLIENT:
				SSL_CTX_set_verify(ctx, option, callback);
				retVal = true;
				break;
			default:
				break;
		}
	}
	return retVal;
}


TSSLConnection::TSSLConnection() {
	prime();
}

TSSLConnection::TSSLConnection(PSSLContext context) {
	prime();
	init(context);
}

TSSLConnection::~TSSLConnection() {
	clear();
}

void TSSLConnection::prime() {
	owner = nil;
	ssl = nil;
	hnd = -1;
	errval = SSL_ERROR_NONE;
	recval = SSL_ERROR_NONE;
	sndval = SSL_ERROR_NONE;
	sysval = EXIT_SUCCESS;
	negotiated = false;
}

void TSSLConnection::clear() {
	close();
	if (util::assigned(ssl)) {
		SSL_free(ssl);
	}
	hnd = -1;
	ssl = nil;
	owner = nil;
}

bool TSSLConnection::reset() {
	if (debug) std::cout << "TSSLConnection::reset() called." << std::endl;
	errno = EXIT_SUCCESS;
	int r = SSL_clear(ssl);
	sysval = errno;
	errval = errcode(r);
	return (1 == r);
}

bool TSSLConnection::assign(const app::THandle socket) {
	hnd = -1;
	errno = EXIT_SUCCESS;
	int r = SSL_set_fd(ssl, socket);
	sysval = errno;
	errval = errcode(r);
	if (1 == r) {
		hnd = socket;
		return true;
	}
	return false;
}

bool TSSLConnection::setCaller(void * data, const int index) const {
	errno = EXIT_SUCCESS;
	int r = SSL_set_ex_data(ssl, index, data);
	sysval = errno;
	errval = errcode(r);
	return (1 == r);
}

void * TSSLConnection::getCaller(const int index) const {
	void * retVal = nil;
	return (1 == SSL_set_ex_data(ssl, index, retVal)) ? retVal : nil;
}

void TSSLConnection::setDebug(const bool debug) {
	this->debug = debug;
	if (debug) std::cout << "TSSLConnection::setDebug() Debugging enabled." << std::endl;
};

void TSSLConnection::close() {
	if (hnd > 0) {
		if (debug) std::cout << "TSSLConnection::close() called for socket <" << hnd << ">" << std::endl;
		executer(SSL_shutdown, "TSSLConnection::close()");
	}
	setNegotiated(false);
	hnd = -1;
}

void TSSLConnection::init(PSSLContext context) {
	owner = nil;
	if (util::assigned(context)) {
		ssl = SSL_new(context->context());
		if (!util::assigned(ssl))
			throw util::app_error("TSSLConnection::init() failed : " + util::quote(getOpenSSLErrorMessage()));
		owner = context;
	} else
		throw util::app_error("TSSLConnection::init() failed : Invalid context.");
}

void TSSLConnection::initialize(PSSLContext context) {
	if (!util::assigned(ssl) && util::assigned(context))
		init(context);
}

bool TSSLConnection::ciphers(const std::string& names) {
	if (!names.empty())
		return (1 == SSL_set_cipher_list(ssl, names.c_str()));
	return false;
}

int TSSLConnection::errcode(const int retVal) const {
	return SSL_get_error(ssl, retVal);
}

std::string TSSLConnection::errmsg() const {
	std::string msg, errcode = getOpenSSLErrorCode(error());
	if (errval == SSL_ERROR_SYSCALL)
		msg = sysutil::getSysErrorMessage(syserr());
	else
		msg = getOpenSSLErrorMessage();
	if (msg.empty())
		return errcode;
	return errcode + ": " + msg;
}

int TSSLConnection::executer(const TSSLExecuter method, const std::string& name) {
	int r;
	int cnt = 0;
	TTimePart start = util::now();
	bool timeout = false;
	bool first = false;

	// Read as long as no further retry requested
	do {
		if (!first)	first = true;
		else util::wait(10);

		// Execute native SSL function
		errno = EXIT_SUCCESS;
		errval = SSL_ERROR_NONE;
		r = (method)(ssl);
		sysval = errno;
		if (r <= 0)
			errval = errcode(r);

		// Debug counter output every 10 calls
		if (debug) {
			++cnt;
			if ((cnt % 10) == 0)
				std::cout << app::yellow << name << " called (" << errval << "/" << cnt << ")" << app::reset << std::endl;
		}

		// Timeout expired?
		if ((now() - start) > 2)
			timeout = true;

	} while (!timeout && (SSL_ERROR_WANT_READ == errval || SSL_ERROR_WANT_WRITE == errval));

	// Create system error "No Data" on timeout
	if (timeout && (SSL_ERROR_WANT_READ == errval || SSL_ERROR_WANT_WRITE == errval)) {
		r = errval = SSL_ERROR_SYSCALL;
		sysval = errno = ENODATA;
	}

	return r;
}

bool TSSLConnection::accept() {
	bool retVal = false;
	if (ECT_SERVER == getType()) {
		retVal = (1 == executer(SSL_accept, "TSSLConnection::accept()"));
		setNegotiated(retVal);
	}
	return retVal;
}

bool TSSLConnection::connect() {
	bool retVal = false;
	if (ECT_CLIENT == getType()) {
		retVal = (1 == executer(SSL_connect, "TSSLConnection::connect()"));
		setNegotiated(retVal);
	}
	return retVal;
}

EContextType TSSLConnection::getType() const {
	if (util::assigned(owner))
		return owner->getType();
	return ECT_UNKNOWN;
}

ssize_t TSSLConnection::receive(void * const data, size_t const size) const {
    // Check for negotiated connection
	if (!isNegotiated()) {
		errno = sysval = EXIT_SUCCESS;
		errval = recval = SSL_ERROR_NONE;
		if (debug) std::cout << "TSSLConnection::receive() ignored for <" << hnd << ">" << std::endl;
		return (ssize_t)0;
	}

	int r;

	// Read as long as no further read requested
	do {
		errno = EXIT_SUCCESS;
		errval = recval = SSL_ERROR_NONE;
		r = SSL_read(ssl, data, size);
		sysval = errno;
		if (r <= 0)
			errval = recval = errcode(r);
		if (debug) std::cout << "TSSLConnection::receive() called for socket <" << hnd << "> result = " << r << ", errval = " << errval << std::endl;
	} while (SSL_ERROR_WANT_READ == errval || SSL_ERROR_WANT_WRITE == errval);
	if (debug) std::cout << "TSSLConnection::receive() finished for socket <" << hnd << "> result = " << r << ", errval = " << errval << std::endl;

	// Underlying socket closed?
	if (r <= 0) {
		if (SSL_ERROR_ZERO_RETURN == errval) {
			errno = sysval = EXIT_SUCCESS;
			return (ssize_t)0;
		}
	}

	return (ssize_t)r;
}

ssize_t TSSLConnection::send(void const * const data, size_t const size) const {
    // Check for negotiated connection
	if (!isNegotiated()) {
		errno = sysval = EXIT_SUCCESS;
		errval = sndval = SSL_ERROR_NONE;
		std::cout << "TSSLConnection::send() ignored for socket <" << hnd << ">" << std::endl;
		return (ssize_t)0;
	}

	char const *p = (char const *)data;
    char const *const q = (char const *)data + size;
    int r;

    // Call SSL_write() as long as all data written
    while (p < q) {

		// Write as long as no further write requested
		do {
			errno = EXIT_SUCCESS;
			errval = recval = SSL_ERROR_NONE;
			r = SSL_write(ssl, p, (size_t)(q - p));
			sysval = errno;
			if (r <= 0)
				errval = sndval = errcode(r);
			if (debug) std::cout << "TSSLConnection::send() called for socket <" << hnd << "> result = " << r << ", errval = " << errval << std::endl;
		} while (SSL_ERROR_WANT_READ == errval || SSL_ERROR_WANT_WRITE == errval);
		if (debug) std::cout << "TSSLConnection::send() finished for socket <" << hnd << "> result = " << r << ", errval = " << errval << std::endl;
		
    	// Underlying socket closed?
   		if (r <= 0) {
   			if (SSL_ERROR_ZERO_RETURN == errval) {
   				errno = sysval = EXIT_SUCCESS;
   				return (ssize_t)0;
   			}

   	   		// Write failed
   			return (ssize_t)r;
   		}

    	p += (size_t)r;
    }

    // Buffer has been fully written
    // Possible overflow on result value (ssize_t)size !!!
    return (ssize_t)size;
}

void TSSLConnection::showCertificates(const std::string& name) {
	if (!cert.isValid())
		cert.assign(*this);
	if (cert.isValid()) {
		std::cout << "  Subject : " << cert.getSubject() << std::endl;
		std::cout << "  Issuer  : " << cert.getIssuer() << std::endl;
	} else {
		if (name.empty()) std::cout << "No certificate found." << std::endl;
		else std::cout << "No certificate found for \"" << name << "\"" << std::endl;
	}
}

const std::string& TSSLConnection::getSubject() {
	if (!cert.isValid())
		cert.assign(*this);
	if (cert.isValid())
		return cert.getSubject();
	return empty;
}

const std::string& TSSLConnection::getIssuer() {
	if (!cert.isValid())
		cert.assign(*this);
	if (cert.isValid())
		return cert.getIssuer();
	return empty;
}




TDigest::TDigest(EDigestType type) {
	init();
	setType(type);
	setFormat(ERT_HEX_SHORT_NCASE);
}

TDigest::~TDigest() {
	destroy();
}

void TDigest::init() {
	mdtype = nil;
	name = "UNKNOWN";
	mdctx = EVP_MD_CTX_create();
	buffer.resize(4 * MAX_MD_SIZE + 1, false);
	result = new UINT8[MAX_MD_SIZE + 1];
	result[MAX_MD_SIZE] = '\0';
	resultSize = 0;
	state = EUS_NONE;
}

void TDigest::destroy() {
	if (util::assigned(mdctx))
		EVP_MD_CTX_destroy(mdctx);
	if (util::assigned(result))
		delete[] result;
	mdctx = nil;
	result = nil;
	resultSize = 0;
}

void TDigest::clear() {
	state = EUS_NONE;
	resultSize = 0;
}

const char* TDigest::getDigestName() {
	for (size_t i=0; DIGEST_NAMES[i].name; i++) {
		if (DIGEST_NAMES[i].type == digestType) {
			return DIGEST_NAMES[i].name;
		}
	}
	return nil;
}

void TDigest::setType (const EDigestType type) {
	digestType = type;

	const char* name = getDigestName();
	if (!util::assigned(name))
		throw util::app_error("TDigest::setType() failed : Invalid digest type.");

	this->name = util::charToStr(name);

	mdtype = EVP_get_digestbyname(name);
	if (!util::assigned(mdtype))
		throw util::app_error("TDigest::setType() failed : Invalid digest name \"" + std::string(name) + "\"");
}

void TDigest::setFormat (const EReportType format) {
	reportType = format;
}


void TDigest::initialize() {
	if (state != EUS_NONE && state != EUS_FINAL)
		throw util::app_error("TDigest::initialize() failed : Wrong calling sequence.");

	resultSize = 0;
	if (1 != EVP_DigestInit_ex(mdctx, mdtype, NULL))
		throw util::app_error("TDigest::initialize()::EVP_DigestInit_ex() failed.");

	state = EUS_INIT;
}

void TDigest::finalize() {
	if (state != EUS_UPDATE)
		throw util::app_error("TDigest::finalize() failed : Wrong calling sequence.");

	unsigned int len;
	if (1 != EVP_DigestFinal_ex(mdctx, result, &len))
		throw util::app_error("TDigest::finalize()::EVP_DigestFinal_ex() failed.");

	resultSize = static_cast<size_t>(len);
	if (!(resultSize > 0))
		throw util::app_error("TDigest::finalize() failed : Invalid digest, calculation failed.");

	state = EUS_FINAL;
}

void TDigest::update(void const * const data, const size_t size) {
	if (state != EUS_INIT && state != EUS_UPDATE)
		throw util::app_error("TDigest::update() failed : Out of sequence.");

	if (1 != EVP_DigestUpdate(mdctx, data, size))
		throw util::app_error("TDigest::update()::EVP_DigestUpdate() failed.");

	state = EUS_UPDATE;
}

void TDigest::calculate(void const * const data, const size_t size) {
	initialize();
	update(data, size);
	finalize();
}

bool TDigest::format(char* output, size_t& size) const
{
	size = 0;

	if(!util::assigned(output))
		return false;

	if (!(resultSize > 0))
		return false;

	char tmp[5];

	if ((reportType == ERT_HEX) ||
		(reportType == ERT_HEX_SHORT) ||
		(reportType == ERT_HEX_NCASE) ||
		(reportType == ERT_HEX_SHORT_NCASE) )
	{
		const char* fmt1;
		const char* fmtN;

		switch (reportType) {
			case ERT_HEX_SHORT_NCASE:
				fmt1 = "%02x";
				fmtN = "%02x";
				break;
			case ERT_HEX_NCASE:
				fmt1 = "%02x";
				fmtN = " %02x";
				break;
			case ERT_HEX_SHORT:
				fmt1 = "%02X";
				fmtN = "%02X";
				break;
			case ERT_HEX:
			default:
				fmt1 = "%02X";
				fmtN = " %02X";
				break;
		}

		size += snprintf(tmp, 4, fmt1, result[0]);
		strcpy(output, tmp);

		for(size_t i = 1; i < resultSize; ++i) {
			size += snprintf(tmp, 4, fmtN, result[i]);
			strcat(output, tmp);
		}

	} else if (reportType == ERT_DIGIT)
	{
		const char* fmt1 = "%u";
		const char* fmtN = " %u";

		size += snprintf(tmp, 4, fmt1, result[0]);
		strcpy(output, tmp);

		for(size_t i = 1; i < resultSize; ++i) {
			size += snprintf(tmp, 4, fmtN, result[i]);
			strcat(output, tmp);
		}
	} else if (reportType == ERT_BASE64)
	{
		// To avoid double string copies use TBase64::encode() directly...
		std::string base64;
		util::TBase64::encode((const char*)result, resultSize, base64);
		if (!base64.empty()) {
			size = base64.size();
			strcpy(output, base64.c_str());
		}
	} else
		return false;

	return true;
}

std::string TDigest::report() {
	size_t size;
	if (reportType == ERT_BASE64)
		return util::TBase64::encode((const char*)result, resultSize);
	if (format(buffer.data(), size))
		return std::string(buffer.data(), size);
	return "";
}

bool TDigest::compare(const TDigest& value) const {
	if (!util::assigned(result))
		return false;
	if (!util::assigned(value.result))
		return false;

	if (digestType != value.digestType)
		return false;

	if (resultSize != value.resultSize)
		return false;

	const UINT8* p = result;
	const UINT8* q = value.result;
	for (size_t i=0; i<resultSize; i++, p++, q++) {
		if (*p != *q) {
			return false;
		}
	}

	return true;
}

std::string TDigest::getDigest(void const * const data, const size_t size) {
	clear();
	calculate(data, size);
	return report();
}

std::string TDigest::getDigest() {
	if (isValid()) {
		return report();
	}
	return "";
}

void TDigest::onFileChunkRead(void * const data, const size_t size) {
	update(data, size);
}

std::string TDigest::getDigest(const std::string& fileName) {
	clear();
	TBaseFile o(fileName);
	if (o.exists()) {
		o.bindChunkReader(&TDigest::onFileChunkRead, this);
		initialize();
		o.chunkedRead();
		finalize();
		return report();
	}
	return "";
}


} /* namespace util */
