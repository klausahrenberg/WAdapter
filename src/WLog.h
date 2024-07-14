#ifndef _W_LOG_H
#define _W_LOG_H

#include <inttypes.h>
#include <stdarg.h>
#include "Arduino.h"

#define LOG_LEVEL_SILENT 0
#define LOG_LEVEL_ERROR  1
#define LOG_LEVEL_DEBUG  2
#define LOG_LEVEL_NOTICE 3

const char* LOG_LEVEL_STRING_ERROR = "error";
const char* LOG_LEVEL_STRING_DEBUG = "debug";
const char* LOG_LEVEL_STRING_NOTICE = "notice";

/**
 * Logging is a helper class to output informations over
 * RS232. If you know log4j or log4net, this logging class
 * is more or less similar ;-) <br>
 * Different loglevels can be used to extend or reduce output
 * All methods are able to handle any number of output parameters.
 * All methods print out a formated string (like printf).<br>
 * To reduce output and program size, reduce loglevel.
 *
 * Output format string can contain below wildcards. Every wildcard
 * must be start with percent sign (\%)
 *
 * ---- Wildcards
 *
 * %s	replace with an string (char*)
 * %c	replace with an character
 * %d	replace with an integer value
 * %l	replace with an long value
 * %x	replace and convert integer value into hex
 * %X	like %x but combine with 0x123AB
 * %b	replace and convert integer value into binary
 * %B	like %x but combine with 0b10100011
 * %t	replace and convert boolean value into "t" or "f"
 * %T	like %t but convert into "true" or "false"
 *
 * ---- Loglevels
 *
 * 0 - LOG_LEVEL_SILENT     no output
 * 1 - LOG_LEVEL_ERROR      all errors
 * 2 - LOG_LEVEL_DEBUG      debug messages
 * 3 - LOG_LEVEL_NOTICE     notices
 */

class WLog {
public:

	WLog() {
		this->output = nullptr;
	}

	void setOutput(Print *output, int maxLevel, bool showLevel, bool printLineBreak) {
		this->output = output;
		this->maxLevel = maxLevel;
		this->showLevel = showLevel;
		this->printLineBreak = printLineBreak;
	}

	template<class T, typename ... Args> void error(T msg, Args ... args) {
		printLevel(LOG_LEVEL_ERROR, msg, args...);
	}

	template<class T, typename ... Args> void debug(T msg, Args ...args) {
		printLevel(LOG_LEVEL_DEBUG, msg, args...);
	}

	template<class T, typename ... Args> void notice(T msg, Args ...args) {
		printLevel(LOG_LEVEL_NOTICE, msg, args...);
	}

	template<class T> void printLevel(int level, T msg, ...) {
		if ((output != nullptr) && (level <= maxLevel)) {
			if (this->showLevel) {
				this->output->print(getLevelString(level));
				this->output->print(": ");
			}
			va_list args;
			va_start(args, msg);
			print(msg, args);
			va_end(args);
		}
	}

	const char* getLevelString(int level) {
		switch (level) {
			case LOG_LEVEL_ERROR : return LOG_LEVEL_STRING_ERROR;
			case LOG_LEVEL_DEBUG : return LOG_LEVEL_STRING_DEBUG;
			case LOG_LEVEL_NOTICE : return LOG_LEVEL_STRING_NOTICE;
		}
		return "";
	}

private:
	Print* output;
	byte maxLevel;
	bool showLevel;
	bool printLineBreak;

	void print(const char *format, va_list args) {
		for (; *format != 0; ++format) {
			if (*format == '%') {
				++format;
				printFormat(*format, &args);
			} else {
				this->output->print(*format);
			}
		}
		if (this->printLineBreak) this->output->println();
	}

	void print(const __FlashStringHelper *format, va_list args) {
		PGM_P p = reinterpret_cast<PGM_P>(format);
		char c = pgm_read_byte(p++);
		for(;c != 0; c = pgm_read_byte(p++)) {
			if (c == '%') {
				c = pgm_read_byte(p++);
				printFormat(c, &args);
			} else {
				this->output->print(c);
			}
		}
		if (this->printLineBreak) this->output->println();
	}

	void printFormat(const char format, va_list *args) {
		if (format == '%') {
			this->output->print(format);
		} else if (format == 's') {
			register char *s = (char *)va_arg(*args, int);
			this->output->print(s);
		} else if (format == 'S') {
			register __FlashStringHelper *s = (__FlashStringHelper *)va_arg(*args, int);
			this->output->print(s);
		} else if (format == 'd' || format == 'i') {
			this->output->print(va_arg(*args, int), DEC);
		} else if (format == 'D' || format == 'F') {
			this->output->print(va_arg(*args, double));
		} else if (format == 'x') {
			this->output->print(va_arg(*args, int), HEX);
		} else if (format == 'X') {
			this->output->print("0x");
			this->output->print(va_arg(*args, int), HEX);
		} else if (format == 'b') {
			this->output->print(va_arg(*args, int), BIN);
		} else if (format == 'B') {
			this->output->print("0b");
			this->output->print(va_arg(*args, int), BIN);
		} else if (format == 'l') {
			this->output->print(va_arg(*args, long), DEC);
		} else if (format == 'c') {
			this->output->print((char) va_arg(*args, int));
		} else if (format == 't') {
			if (va_arg(*args, int) == 1) {
				this->output->print("T");
			} else {
				this->output->print("F");
			}
		} else if (format == 'T') {
			if (va_arg(*args, int) == 1) {
				this->output->print(F("true"));
			} else {
				this->output->print(F("false"));
			}
		}
	}

};

WLog* LOG = new WLog();

#endif
