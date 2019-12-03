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
		return beginObject("");
	}

	WJson& beginObject(const char* name) {
		if (!separatorAlreadyCalled) {
			ifSeparator();
			separatorAlreadyCalled = true;
		}
		if (name != "") {
			memberName(name);
		}
		stream->print(SBEGIN);
		firstElement = true;
		return *this;
	}

	WJson& memberName(const char *name) {
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

	WJson& beginArray(const char* name) {

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

	WJson& propertyString(const char* name, const char *value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		string(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyString(const char* name, const char *value1, const char *value2) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		string(value1, value2);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyString(const char* name, const char *value1, const char *value2, const char *value3) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		string(value1, value2, value3);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyString(const char* name, const char *value1, const char *value2, const char *value3, const char *value4) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		string(value1, value2, value3, value4);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyInteger(const char* name, int value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		numberInteger(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyLong(const char* name, unsigned long value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		numberLong(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyByte(const char* name, byte value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		numberByte(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyDouble(const char* name, double value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		numberDouble(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyBoolean(const char* name, bool value) {
		ifSeparator();
		separatorAlreadyCalled = true;
		memberName(name);
		boolean(value);
		separatorAlreadyCalled = false;
		return *this;
	}

	WJson& string(const char *text) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(QUOTE);
		stream->print(text);
		stream->print(QUOTE);
		return *this;
	}

	WJson& string(const char *text1, const char *text2) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(QUOTE);
		stream->print(text1);
		stream->print(text2);
		stream->print(QUOTE);
		return *this;
	}

	WJson& string(const char *text1, const char *text2, const char *text3) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(QUOTE);
		stream->print(text1);
		stream->print(text2);
		stream->print(text3);
		stream->print(QUOTE);
		return *this;
	}

	WJson& string(const char *text1, const char *text2, const char *text3, const char *text4) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(QUOTE);
		stream->print(text1);
		stream->print(text2);
		stream->print(text3);
		stream->print(text4);
		stream->print(QUOTE);
		return *this;
	}

	WJson& numberInteger(int number) {
		if(!separatorAlreadyCalled)
		ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& numberLong(unsigned long number) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& numberByte(byte number) {
		if (!separatorAlreadyCalled)
			ifSeparator();
		stream->print(number, DEC);
		return *this;
	}

	WJson& numberDouble(double number) {
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
