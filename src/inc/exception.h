/*
 * exception.h
 *
 *  Created on: 30.08.2014
 *      Author: Dirk Brinkmeier
 */

#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <errno.h>
#include <string>
#include <exception>
#include <string.h>
#include <locale.h>
#include "classes.h"
#include "stringtemplates.h"

namespace util {

class TBaseException : public std::exception {
private:
	mutable locale_t locale;

protected:
	std::string m_text;
	std::string getSysErrorMessage(int errnum) const {
		locale = newlocale(LC_MESSAGES_MASK, "POSIX", (locale_t)0);
		char c[128];
		char* p = nullptr;
		if (locale == (locale_t)0) {
			p = strerror_r(errnum, c, 128);
		} else {
			p = strerror_l(errnum, locale);
		}
		if (nullptr != p)
			return std::string(p);
		return "Getting error text via strerror_r(" + std::to_string((size_s)errnum) + ") failed.";
	}

public:
	explicit TBaseException (const std::string& text) : m_text(text) {
		locale = (locale_t)0;
	}
	virtual ~TBaseException () throw() {
		if (locale != (locale_t)0) {
			freelocale(locale);
		}
	}
	const char* what() const throw() {
		return m_text.empty() ? "" : m_text.c_str();
	}
};


class app_error : public TBaseException {
public:
	explicit app_error (const std::string& text) : TBaseException(text) {}
	virtual ~app_error () throw() {}
};


class app_error_fmt : public TBaseException {
public:
	template<typename value_t, typename... variadic_t>
	explicit app_error_fmt (const std::string& text, const value_t value, variadic_t... args) : TBaseException("") {
		m_text = util::csnprintf(text, value, std::forward<variadic_t>(args)...);
	}
	virtual ~app_error_fmt () throw() {}
};


class sys_error : public TBaseException {
private:
	int m_errnum;
	mutable std::string m_str;

public:
	explicit sys_error (const std::string& text, int errnum = errno) : TBaseException(text), m_errnum(errnum) {}
	virtual ~sys_error () throw() {}
	const char* what() const throw() {
		m_str = m_text + " [" + getSysErrorMessage(m_errnum) + " (" + std::to_string((size_s)m_errnum) + ")]";
		return m_str.c_str();
	}
};


class sys_error_fmt : public TBaseException {
private:
	int m_errnum;
	mutable std::string m_str;

public:
	template<typename value_t, typename... variadic_t>
	explicit sys_error_fmt (const std::string& text, const value_t value, variadic_t... args) : TBaseException("") {
		m_errnum = errno;
		m_text = util::csnprintf(text, value, std::forward<variadic_t>(args)...);
	}
	virtual ~sys_error_fmt () throw() {}
	const char* what() const throw() {
		m_str = m_text + " [" + getSysErrorMessage(m_errnum) + " (" + std::to_string((size_s)m_errnum) + ")]";
		return m_str.c_str();
	}
};

} /* namespace util */

#endif /* EXCEPTION_H_ */
