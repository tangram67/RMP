/*
 * ssl.h
 *
 *  Created on: 27.05.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef SSL_H_
#define SSL_H_

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include "ssltypes.h"
#include "gcc.h"


#ifdef SSL_HAS_CONST_DECALRATION
# define SSL_const const
#else
# define SSL_const
#endif


#ifdef STL_HAS_TEMPLATE_ALIAS
using TSSLExecuter = int (*) (SSL*);
using TSSLVerifyCallback = int(*)(int, X509_STORE_CTX *);
#else
typedef int (*TSSLExecuter) (SSL*);
typedef int(*TSSLVerifyCallback)(int, X509_STORE_CTX *);
#endif


namespace util {

std::string getOpenSSLErrorMessage();
std::string getOpenSSLErrorCode(int code);


class TSSLInit {
	int index;

public:
	int getIndex() const { return index; };

	TSSLInit();
	~TSSLInit();
};


class TSSLBuffer {
private:
	BIO * bio;
	void init(const char* buffer, const size_t size);
	void clear();

public:
	BIO * pointer() const { return bio; };
	BIO * operator () () const { return pointer(); };

	TSSLBuffer(const char* buffer, const size_t size);
	~TSSLBuffer();
};


class TSSLCertificate {
private:
	X509 * cert;
	std::string subject;
	std::string issuer;

	void init(TSSLConnection& connection);
	void clear();

public:
	bool isValid() const { return util::assigned(cert); };
	X509 * certificate() const { return cert; };
	X509 * operator () () const { return certificate(); };

	void invalidate();
	const std::string& getSubject();
	const std::string& getIssuer();

	void assign(TSSLConnection& connection);

	TSSLCertificate();
	TSSLCertificate(TSSLConnection& connection);
	~TSSLCertificate();
};


class TSSLContext {
private:
	SSL_CTX * ctx;
	X509 * crt;
	RSA * rsa;
	DH * dh;
	EC_KEY *ecdh;
	EContextType type;
	bool debug;

	void init(const EContextType type);
	void prime();
	void clear();

public:
	bool isValid() const { return util::assigned(ctx); };
	bool hasEllipticCurveParameter() const { return util::assigned(ecdh); };
	bool hasDiffieHellmannParameter() const { return util::assigned(dh); };

	EContextType getType() const { return type; };
	bool useEllipticCurveDiffieHellmannParameter();
	bool useDiffieHellmannParameter(const char* param, const size_t size);
	bool useCertificate(const char* cert, const size_t size);
	bool usePrivateKey(const char* key, const size_t size);
	bool loadDiffieHellmannParameter(const std::string& fileName);
	bool loadCertificate(const std::string& fileName);
	bool loadPrivateKey(const std::string& fileName);
	bool checkPrivateKey();
	int checkDiffieHellmanParams(std::string& message);

#ifdef SSL_HAS_PROTO_VERSION
	bool version(int version);
#endif
	long mode(long mode = 0);
	long option(long option = 0);
	bool ciphers(const char* names);
	bool verify(const EContextType type, TSSLVerifyCallback callback = nil);
	void setDebug(const bool debug);

	SSL_CTX * context() const { return ctx; };
	SSL_CTX * operator () () const { return context(); };

	void initialize(const EContextType type);

	TSSLContext();
	TSSLContext(const bool debug);
	TSSLContext(const EContextType type);
	~TSSLContext();
};


class TSSLConnection {
private:
	SSL * ssl;
	PSSLContext owner;
	app::THandle hnd;
	mutable int errval;
	mutable int recval;
	mutable int sndval;
	mutable int sysval;
	bool negotiated;
	TSSLCertificate cert;
	std::string empty;
	bool debug;

	void init(PSSLContext context);
	void prime();
	void clear();
	int errcode(const int retVal) const;
	int executer(const TSSLExecuter method, const std::string& name);

public:
	bool isValid() const { return util::assigned(ssl); };

	EContextType getType() const;
	app::THandle handle() const { return hnd; };
	SSL * connection() const { return ssl; };
	SSL * operator () () const { return connection(); };

	bool assign(const app::THandle socket);
	bool accept();
	bool connect();
	bool reset();
	void close();

	bool setCaller(void * data, const int index) const;
	void * getCaller(const int index) const;
	void setDebug(const bool debug);

	bool isNegotiated() const { return negotiated; };
	void setNegotiated(const bool value) { negotiated = value; };

	int error() const { return errval; };
	int recerr() const { return recval; };
	int snderr() const { return sndval; };
	int syserr() const { return sysval; };
	std::string errmsg() const;

	void showCertificates(const std::string& name = "");
	const std::string& getSubject();
	const std::string& getIssuer();

	ssize_t receive(void * const data, size_t const size) const;
	ssize_t send(void const * const data, size_t const size) const;

	void initialize(PSSLContext context);
	bool ciphers(const std::string& names);

	TSSLConnection();
	TSSLConnection(PSSLContext context);
	~TSSLConnection();
};


class TDigest {
private:
	enum EUpdateState { EUS_NONE, EUS_INIT, EUS_UPDATE, EUS_FINAL };

#ifdef STL_HAS_TEMPLATE_ALIAS
	using UINT8 = unsigned char;
	using PUINT8 = UINT8*;
#else
	typedef unsigned char UINT8;
	typedef UINT8* PUINT8;
#endif
#ifdef STL_HAS_CONSTEXPR
	static constexpr size_t MAX_MD_SIZE = EDS_SHA512;
#else
	static const size_t MAX_MD_SIZE = EDS_SHA512;
#endif

	util::TBuffer buffer;
	const EVP_MD* mdtype;
	EVP_MD_CTX* mdctx;
	PUINT8 result;
	size_t resultSize;

	EDigestType digestType;
	EReportType reportType;
	EUpdateState state;
	std::string name;
	void init();
	void clear();
	void destroy();
	bool compare(const TDigest& value) const;
	void calculate(void const * const data, const size_t size);
	bool format(char* output, size_t& size) const;
	std::string report();
	const char* getDigestName();
	void onFileChunkRead(void * const data, const size_t size);

public:
	bool isValid () const { return (resultSize > 0); };
	size_t getSize () const { return resultSize; };
	void setType (const EDigestType type);
	EDigestType getType () const { return digestType; };
	std::string getTypeAsString () const { return name; };
	void setFormat (const EReportType format);
	EReportType getFormat () const { return reportType; };
	std::string getDigest(void const * const data, const size_t size);
	std::string getDigest(const std::string& fileName);
	std::string getDigest();
	inline std::string operator () (const char * data, size_t size) { return getDigest(data, size); };
	inline std::string operator () (const std::string& data) { if (!data.empty()) return getDigest(data.c_str(), data.size()); return std::string(); };
	inline bool operator == (const TDigest& value) const { return compare(value); };
	inline bool operator != (const TDigest& value) const { return !compare(value); };

	void initialize();
	void finalize();
	void update(void const * const data, const size_t size);

	TDigest(EDigestType type = EDT_SHA1);
	virtual ~TDigest();
};

static TSSLInit SSLInit;
static TDigest MD5(EDT_MD5);
static TDigest SHA1(EDT_SHA1);
static TDigest SHA224(EDT_SHA224);
static TDigest SHA256(EDT_SHA256);
static TDigest SHA384(EDT_SHA384);
static TDigest SHA512(EDT_SHA512);

} /* namespace util */

#endif /* SSL_H_ */
