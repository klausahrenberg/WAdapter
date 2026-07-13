#ifndef _WJSON_H__
#define _WJSON_H__

#include "Arduino.h"

class WJson {
 public:
  WJson(Print* stream) {
    _stream = stream;
  }

  ~WJson() {
    //_stream = nullptr;
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
        default:
          propertyValue(name, value);
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

  WJson& propertyBoolean(const char* name, bool value) {
    _ifSeparator();
    _separatorAlreadyCalled = true;
    memberName(name);
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    WValue::boolToString(_stream, value);  
    _separatorAlreadyCalled = false;
    return *this;
  }

  WJson& propertyValue(const char* name, WValue* value) {
    _ifSeparator();
    _separatorAlreadyCalled = true;
    memberName(name);
    pValue(value);
    _separatorAlreadyCalled = false;
    return *this;
  }

  WJson& pValue(WValue* value) {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    WValue::toString(_stream, value);
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

  WJson& null() {
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    _stream->print("null");
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
