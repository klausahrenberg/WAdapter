#ifndef _WJSON_H__
#define _WJSON_H__

#include "Arduino.h"

class WJson {
 public:
  WJson(Print* stream, IWJsonable* obj = nullptr) {
    _stream = stream;
    if (obj) propertyObject(nullptr, obj);
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

  /*
  // Gibt "name":"<text>" aus. Der Text entsteht per printf-Format mit Parametern:
  //   propertyString("id", "ESP-%06x", WUtils::getChipId());
  //   propertyString("sdk", "%s", ESP.getSdkVersion());
  // Ablauf: Laenge per vsnprintf(nullptr, 0, ...) ermitteln, Puffer anlegen, formatieren,
  // dann mit JSON-Escaping (", \\, Steuerzeichen) in den Stream schreiben. Kann kein Puffer
  // angelegt werden, wird null ausgegeben. Das Ergebnis ist immer ein gueltiger JSON-String.
  // Hinweise: Format muss im RAM liegen (kein F()/PROGMEM). Fremde Texte immer als Parameter
  // uebergeben ("%s", text), nie als Format. Zahlen/Booleans ohne Anfuehrungszeichen:
  // propertyValue / propertyBoolean.
  __attribute__((format(printf, 3, 4)))
  WJson& propertyString(const char* name, const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    va_list copy;
    va_copy(copy, arg);
    int len = vsnprintf(nullptr, 0, format, copy);
    va_end(copy);
    char* buffer = (len >= 0) ? new (std::nothrow) char[len + 1] : nullptr;
    if (!buffer) {
      va_end(arg);
      return propertyNull(name);
    }
    vsnprintf(buffer, len + 1, format, arg);
    va_end(arg);
    _ifSeparator();
    _separatorAlreadyCalled = true;
    memberName(name);
    _stream->print(WC_QUOTE);
    WValue::string(_stream, buffer, nullptr);
    _stream->print(WC_QUOTE);
    _separatorAlreadyCalled = false;
    delete[] buffer;
    return *this;
  }
  */

  WJson& propertyObject(const char* name, IWJsonable* value) {
    _ifSeparator();
    _separatorAlreadyCalled = true;
    memberName(name);
    if (!_separatorAlreadyCalled)
      _ifSeparator();
    beginObject();
    value->toJson(this);
    endObject();
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
