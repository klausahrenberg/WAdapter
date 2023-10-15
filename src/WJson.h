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
		_stream = stream;
	}

	WJson& beginObject() {
		return beginObject("");
	}

	WJson& beginObject(const char* name) {
		if (!_separatorAlreadyCalled) {
			_ifSeparator();
			_separatorAlreadyCalled = true;
		}
		if (name != "") {
			memberName(name);
		}
		_stream->print(SBEGIN);
		_firstElement = true;
		return *this;
	}

	WJson& memberName(const char *name) {
		if (name != nullptr) {
			string(name);
			_stream->print(DPOINT);
		}
		return *this;
	}

	WJson& separator() {
		_stream->print(COMMA);
		return *this;
	}

	WJson& endObject() {
		_stream->print(SEND);
		return *this;
	}

	WJson& beginArray() {
		if (!_separatorAlreadyCalled) {
			_ifSeparator();
		}
		_firstElement = true;
		_stream->print(BBEGIN);
		return *this;
	}

	WJson& beginArray(const char* name) {

		if (!_separatorAlreadyCalled) {
			_ifSeparator();
			_separatorAlreadyCalled = true;
		}
		_firstElement = true;
		memberName(name);
		_separatorAlreadyCalled = false;
		_stream->print(BBEGIN);
		return *this;
	}

	WJson& endArray() {
		_stream->print(BEND);
		return *this;
	}

	WJson& propertyInteger(const char* name, int value) {
		_ifSeparator();
		_separatorAlreadyCalled = true;
		memberName(name);
		numberInteger(value);
		_separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyShort(const char* name, short value) {
		_ifSeparator();
		_separatorAlreadyCalled = true;
		memberName(name);
		numberShort(value);
		_separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyUnsignedLong(const char* name, unsigned long value) {
		_ifSeparator();
		_separatorAlreadyCalled = true;
		memberName(name);
		numberUnsignedLong(value);
		_separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyByte(const char* name, byte value) {
		_ifSeparator();
		_separatorAlreadyCalled = true;
		memberName(name);
		numberByte(value);
		_separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyByteArray(const char* name, byte length, byte* value) {
		_ifSeparator();
		_separatorAlreadyCalled = true;
		memberName(name);
		numberByteArray(length, value);
		_separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyDouble(const char* name, double value) {
		_ifSeparator();
		_separatorAlreadyCalled = true;
		memberName(name);
		numberDouble(value);
		_separatorAlreadyCalled = false;
		return *this;
	}

	WJson& propertyBoolean(const char* name, bool value) {
		_ifSeparator();
		_separatorAlreadyCalled = true;
		memberName(name);
		boolean(value);
		_separatorAlreadyCalled = false;
		return *this;
	}	

	WJson& propertyString(const char* name, const char *value1 = nullptr, const char *value2 = nullptr, const char *value3 = nullptr, const char *value4 = nullptr, const char *value5 = nullptr, const char *value6 = nullptr, const char *value7 = nullptr, const char *value8 = nullptr, const char *value9 = nullptr, const char *value10 = nullptr) {
		_ifSeparator();
		_separatorAlreadyCalled = true;
		memberName(name);
		string(value1, value2, value3, value4, value5, value6, value7, value8, value9, value10);
		_separatorAlreadyCalled = false;
		return *this;
	}	

	WJson& string(const char *text1 = nullptr, const char *text2 = nullptr, const char *text3 = nullptr, const char *text4 = nullptr, const char *text5 = nullptr, const char *text6 = nullptr, const char *text7 = nullptr, const char *text8 = nullptr, const char *text9 = nullptr, const char *text10 = nullptr) {
		if (!_separatorAlreadyCalled)
			_ifSeparator();
		_stream->print(QUOTE);
		if (text1 != nullptr) _stream->print(text1);
		if (text2 != nullptr) _stream->print(text2);
		if (text3 != nullptr) _stream->print(text3);
		if (text4 != nullptr) _stream->print(text4);
		if (text5 != nullptr) _stream->print(text5);
		if (text6 != nullptr) _stream->print(text6);
		if (text7 != nullptr) _stream->print(text7);
		if (text8 != nullptr) _stream->print(text8);
		if (text9 != nullptr) _stream->print(text9);
		if (text10 != nullptr) _stream->print(text10);
		_stream->print(QUOTE);
		return *this;
	}

	WJson& onlyString(const char *text1) {
		if (text1 != nullptr) _stream->print(text1);
		return *this;
	}

	WJson& numberInteger(int number) {
		if(!_separatorAlreadyCalled)
		_ifSeparator();
		_stream->print(number, DEC);
		return *this;
	}

	WJson& numberShort(short number) {
		if (!_separatorAlreadyCalled)
			_ifSeparator();
		_stream->print(number, DEC);
		return *this;
	}

	WJson& numberUnsignedLong(unsigned long number) {
		if (!_separatorAlreadyCalled)
			_ifSeparator();
		_stream->print(number, DEC);
		return *this;
	}

	WJson& numberByte(byte number) {
		if (!_separatorAlreadyCalled)
			_ifSeparator();
		_stream->print(number, DEC);
		return *this;
	}

	WJson& numberByteArray(byte length, byte* value) {
		if (!_separatorAlreadyCalled)
			_ifSeparator();
		_stream->print(BBEGIN);
		for (byte i = 0; i < length; i++) {
			if (i != 0) _stream->print(COMMA);
			_stream->print(value[i], DEC);
		}
		_stream->print(BEND);
		return *this;
	}

	WJson& numberDouble(double number) {
		if (!_separatorAlreadyCalled)
			_ifSeparator();
		_stream->print(number);
		return *this;
	}

	WJson& null() {
		_ifSeparator();
		_stream->print("null");
		return *this;
	}

	WJson& boolean(bool value) {
		if (!_separatorAlreadyCalled)
			_ifSeparator();
		_stream->print(value ? "true" : "false");
		return *this;
	}

private:
	Print* _stream;
	bool _firstElement = true;
	bool _separatorAlreadyCalled = false;

	void _ifSeparator() {
		if (_firstElement) {
			_firstElement = false;
		} else {
			separator();
		}
	}

};

#endif
