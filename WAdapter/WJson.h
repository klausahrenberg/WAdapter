#ifndef _WJSON_H__
#define _WJSON_H__

#include "Arduino.h"

const static char BBEGIN = '[';
const static char BEND = ']';
const static char COMMA = ',';
const static char DPOINT = ':';
const static char SBEGIN = '{';
const static char SEND = '}';
const static char QUOTE = '\"';

class WJson {
public:
	WJson(Print* stream) {
		this->stream = stream;
	}

	WJson& beginObject() {
		if (!separatorAlreadyCalled) {
			ifSeparator();
			separatorAlreadyCalled = true;
		}
		stream->print(SBEGIN);
		firstElement = true;
		return *this;
	}

	WJson& beginObject(String name) {
		if (!separatorAlreadyCalled) {
			ifSeparator();
			separatorAlreadyCalled = true;
		}
		memberName(name);
		stream->print(SBEGIN);
		firstElement = true;
		return *this;
	}

	WJson& memberName(String name) {
		string(name);
		stream->print(DPOINT);
		return *this;
	}

	WJson& memberName(char *name) {
		string(name);
		stream->print(DPOINT);
		return *this;
	}

	WJson& separator() {
		stream->print(COMMA);
		return *this;

	}

	WJson& endObject() {
		stream->print(SEND);
		return *this;
	}

	WJson& beginArray() {
		if (!separatorAlreadyCalled) {
			ifSeparator();
		}
		firstElement = true;
		stream->print(BBEGIN);
		return *this;
	}

	WJson& beginArray(String name) {

		if (!separatorAlreadyCalled) {
			ifSeparator();
			separatorAlreadyCalled = true;
		}
		firstElement = true;
		memberName(name);
		separatorAlreadyCalled = false;
		stream->print(BBEGIN);
		return *this;
	}

	WJson& endArray() {
		stream->print(BEND);
		return *this;
	}

	WJson& property(String name, char *value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		string(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, String value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		string(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, int value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		number(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, unsigned int value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		number(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, long value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		number(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, unsigned long value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		number(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, short value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		number(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, unsigned short value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		number(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, byte value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		number(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, double value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		number(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& property(String name, bool value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		boolean(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& string(char *text) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(QUOTE);
		stream->print(text);
		stream->print(QUOTE);
		return *this;
	}

	WJson& string(String text) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(QUOTE);
		stream->print(text);
		stream->print(QUOTE);
		return *this;
	}

	WJson& number(int number) {
		if(!separatorAlreadyCalled)
		ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& number(unsigned int number) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& number(long number) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& number(unsigned long number) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& number(short number) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& number(unsigned short number) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& number(byte number) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& number(double number) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(number);
		return *this;
	}

	WJson& null() {
		ifSeparator();
		stream->print("null");
		return *this;
	}

	WJson& boolean(bool value) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(value ? "true" : "false");
		return *this;
	}

	int operator[](char*);

private:
	Print* stream;
	bool firstElement = true;
	bool separatorAlreadyCalled = false;

	void ifSeparator() {
		if (firstElement) {
			firstElement = false;
		} else {
			separator();
		}
	}

};

#endif
