#ifndef _WJSON_H__
#define _WJSON_H__

#include "Arduino.h"

class WJson {
 public:
  WJson(Print* stream) {
    _stream = stream;
  }

  ~WJson() {
    _stream = nullptr;
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
    _stream->print(WC_SBEGIN);
    _firstElement = true;
    return *this;
  }

  WJson& memberName(const char* name) {
    if (name != nullptr) {
      string(name, nullptr);
      _stream->print(WC_DPOINT);
    }
    return *this;
  }

  WJson& separator() {
    _stream->print(WC_COMMA);
    return *this;
  }

  WJson& endObject() {
    _stream->print(WC_SEND);
    return *this;
  }

  WJson& beginArray() {
    if (!_separatorAlreadyCalled) {
      _ifSeparator();
    }
    _firstElement = true;
    _stream->print(WC_RBEGIN);
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
    _stream->print(WC_RBEGIN);
    return *this;
  }

  WJson& endArray() {
    _stream->print(WC_REND);
    return *this;
  }

  WJson& property(const char* name, WValue* value) {
    if ((value != nullptr) && (!value->isNull())) {
      switch (value->type()) {
        case WDataType::STRING:
          if (!value->isStringEmpty()) {
            propertyString(name, value->asString(), nullptr);
          } else {
            propertyNull(name);
          }
          break;
        case WDataType::BOOLEAN:
          propertyBoolean(name, value->asBool());
          break;
        case WDataType::DOUBLE:
          propertyDouble(name, value->asDouble());
          break;
        case WDataType::SHORT:
          propertyShort(name, value->asShort());
          break;
        case WDataType::INTEGER:
          propertyInteger(name, value->asInt());
          break;
        case WDataType::UNSIGNED_LONG:
          propertyUnsignedLong(name, value->asUnsignedLong());
          break;
        case WDataType::BYTE:
          propertyByte(name, value->asByte());
          break;
        case WDataType::BYTE_ARRAY:
          propertyByteArray(name, value->length(), value->asByteArray());
          break;
        case WDataType::LIST:
          // tbi, not supported yet
          break;
      }
    } else {
      propertyNull(name);
    }
    return *this;
  }

  WJson& propertyNull(const char* name) {
    _ifSeparator();
    _separatorAlreadyCalled = true;
    memberName(name);
    null();
    _separatorAlreadyCalled = false;
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

  WJson& propertyString(const char* name, const char* value, ...) {
    _ifSeparator();
    _separatorAlreadyCalled = true;
    memberName(name);
    _stream->print(WC_QUOTE);
    va_list arg;
    va_start(arg, value);
    while (value) {
      WValue::string(_stream, value, nullptr);
      value = va_arg(arg, const char*);
    }
    va_end(arg);
    _stream->print(WC_QUOTE);
    _separatorAlreadyCalled = false;
    return *this;
  }

  WJson& string(const char* text, ...) {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    _stream->print(WC_QUOTE);
    va_list arg;
    va_start(arg, text);
    while (text) {
      WValue::string(_stream, text, nullptr);
      text = va_arg(arg, const char*);
    }
    va_end(arg);
    _stream->print(WC_QUOTE);
    return *this;
  }

  WJson& onlyString(const char* text1) {
    if (text1 != nullptr) _stream->print(text1);
    return *this;
  }

  WJson& numberInteger(int number) {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    _stream->print(number, DEC);
    return *this;
  }

  WJson& numberShort(short number) {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    WValue::numberShort(_stream, number);
    return *this;
  }

  WJson& numberUnsignedLong(unsigned long number) {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    WValue::numberUnsignedLong(_stream, number);
    return *this;
  }

  WJson& numberByte(byte number) {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    WValue::numberByte(_stream, number);
    return *this;
  }

  WJson& numberByteArray(byte length, byte* value) {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    WValue::numberByteArray(_stream, length, value);
    return *this;
  }

  WJson& numberDouble(double number) {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    WValue::numberDouble(_stream, number);
    return *this;
  }

  WJson& null() {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    _stream->print("null");
    return *this;
  }

  WJson& boolean(bool value) {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    WValue::boolean(_stream, value);
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
