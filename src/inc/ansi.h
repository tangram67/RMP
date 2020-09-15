/*
 * ansi.h
 *
 *  Created on: 06.06.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef ANSI_H_
#define ANSI_H_

#include <iostream>
#include "nullptr.h"

//
// Usage:
// std::cout << app::yellow << "setType(" << type << ") <" << getTypeAsString() << ">")) << app::reset << std::endl;
//
// Or (discouraged!)
// std::cout << BOLD(YELLOW("setType(" << type << ") <" << getTypeAsString() << ">")) << std::endl;
//

#define CRESET   "\x1B[0m"
#define CRED     "\x1B[31m"
#define CGREEN   "\x1B[32m"
#define CYELLOW  "\x1B[33m"
#define CBLUE    "\x1B[34m"
#define CMAGENTA "\x1B[35m"
#define CCYAN    "\x1B[36m"
#define CWHITE   "\x1B[37m"
#define CBOLD    "\x1B[1m"
#define CUNDL    "\x1B[4m"

#define RED(x)     CRED x CRESET
#define GREEN(x)   CGREEN x CRESET
#define YELLOW(x)  CYELLOW x CRESET
#define BLUE(x)    CBLUE x CRESET
#define MAGENTA(x) CMAGENTA x CRESET
#define CYANN(x)   CCYAN x CRESET
#define WHITE(x)   CWHITE x CRESET

#define BOLD(x) CBOLD x CRESET
#define UNDL(x) CUNDL x CRESET


namespace app {


enum EAnsiColors {
	EAC_RED,
	EAC_GREEN,
	EAC_YELLOW,
	EAC_BLUE,
	EAC_MAGENTA,
	EAC_CYAN,
	EAC_WHITE,
	EAC_RESET
};

enum EAnsiStyles {
	EAS_STANDARD,
	EAS_BOLD,
	EAS_UNDERLINED
};


class TAnsi
{
private:
	bool enabled;
	bool colored;
	EAnsiColors color;
	EAnsiStyles style;
	std::string fst;
	std::string fmt;

	void prime() {
		colored = hasColorTerminal();
		enabled = true;
	}

	void setAnsiColor(const EAnsiColors color) {
		this->color = color;
		switch (color) {
			case EAC_RED:
				fmt = fst + CRED;
				break;
			case EAC_GREEN:
				fmt = fst + CGREEN;
				break;
			case EAC_YELLOW:
				fmt = fst + CYELLOW;
				break;
			case EAC_BLUE:
				fmt = fst + CBLUE;
				break;
			case EAC_MAGENTA:
				fmt = fst + CMAGENTA;
				break;
			case EAC_CYAN:
				fmt = fst + CCYAN;
				break;
			case EAC_WHITE:
				fmt = fst + CWHITE;
				break;
			default:
				fmt = CRESET;
				break;
		}
	}

	void setAnsiStyle(const EAnsiStyles style) {
		this->style = style;
		switch (style) {
			case EAS_STANDARD:
				fst = "";
				break;
			case EAS_BOLD:
				fst = CBOLD;
				break;
			case EAS_UNDERLINED:
				fst = CUNDL;
				break;
		}
		setAnsiColor(color);
	}

	std::string getEnvironmentVariable(const std::string& key) {
		std::string var;
		if (!key.empty()) {
			char* p = getenv(key.c_str());
			if (p != nil)
				var = std::string(p);
		}
		return var;
	}

	bool hasColorTerminal() {
		std::string env = getEnvironmentVariable("TERM");
		if (!env.empty()) {
			if (std::string::npos != env.find("xterm"))
				return true;
		}
		env = getEnvironmentVariable("COLORTERM");
		if (!env.empty()) {
			return true;
		}
		return false;
	}


public:
	void enable() { enabled = true; }
	void disable() { enabled = false; }
	void setColor(const EAnsiColors color) { setAnsiColor(color); };
	void setStyle(const EAnsiStyles style) { setAnsiStyle(style); };
	std::string getAnsi() const { return fmt; };
	bool hasColor() const { return colored; };
	bool isEnabled() const { return enabled; };
	void operator() (const EAnsiColors color) { setColor(color); };
	void clear() {
		color = EAC_RESET;
		setAnsiStyle(EAS_BOLD);
	}

	friend std::istream& operator >> (std::istream& is, TAnsi& o);
	friend std::ostream& operator << (std::ostream& os, const TAnsi& o);

	TAnsi() {
		prime();
		clear();
	};
	TAnsi(const EAnsiColors color) {
		prime();
		this->color = color;
		setAnsiStyle(EAS_BOLD);
	};
	virtual ~TAnsi() = default;
};


static TAnsi ansi;
static TAnsi red(EAC_RED);
static TAnsi green(EAC_GREEN);
static TAnsi yellow(EAC_YELLOW);
static TAnsi blue(EAC_BLUE);
static TAnsi magenta(EAC_MAGENTA);
static TAnsi cyan(EAC_CYAN);
static TAnsi white(EAC_WHITE);
static TAnsi reset(EAC_RESET);


inline std::istream& operator >> (std::istream& is, TAnsi& o)
{
	// is >> o; => not const!
    return is;
}

inline std::ostream& operator << (std::ostream& os, const TAnsi& o)
{
	if (o.hasColor() && o.isEnabled())
		os << o.getAnsi();
	return os;
}

} // namespace app

#endif /* ANSI_H_ */
