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
#include "classes.h"
#include "sysutils.h"
#include "stringtemplates.h"

namespace util {

class TBaseException : public std::exception {
protected:
	std::string m_text;

public:
	explicit TBaseException (const std::string& text) : m_text(text) {}
	virtual ~TBaseException () throw() {}
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
		m_str = m_text + " [" + sysutil::getSysErrorMessage(m_errnum) + " (" + std::to_string((size_s)m_errnum) + ")]";
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
		m_str = m_text + " [" + sysutil::getSysErrorMessage(m_errnum) + " (" + std::to_string((size_s)m_errnum) + ")]";
		return m_str.c_str();
	}
};

} /* namespace util */

#endif /* EXCEPTION_H_ */
