#ifndef W_VALUE_H
#define W_VALUE_H

#include "WStringStream.h"

enum class WDataType {
  BOOLEAN,
  DOUBLE,
  SHORT,
  UNSIGNED_SHORT,
  INTEGER,
  UNSIGNED_LONG,
  BYTE,
  STRING,
  BYTE_ARRAY,
  LIST
};

const static char WC_FALSE[] PROGMEM = "false";
const static char WC_TRUE[] PROGMEM = "true";
const static char WC__BASE[] PROGMEM = " =<>/\"{}()[],:'";
char WC_SPACE = WC__BASE[0];
char WC_EQUAL = WC__BASE[1];
char WC_SMALLER = WC__BASE[2];
char WC_GREATER = WC__BASE[3];
char WC_SLASH = WC__BASE[4];
char WC_QUOTE = WC__BASE[5];
char WC_SBEGIN = WC__BASE[6];
char WC_SEND = WC__BASE[7];
char WC_BBEGIN = WC__BASE[8];
char WC_BEND = WC__BASE[9];
char WC_RBEGIN = WC__BASE[10];
char WC_REND = WC__BASE[11];
char WC_COMMA = WC__BASE[12];
char WC_DPOINT = WC__BASE[13];
char WC_QUOTE2 = WC__BASE[14];

// using WOnValueChange = std::function<void()>;
typedef std::function<void()> WOnValueChange;

struct WValue {
 public:
  WValue(WDataType type = WDataType::BOOLEAN) {
    _type = type;
  }

  WValue(const char* string) {
    _type = WDataType::STRING;
    asString(string);
  }

  WValue(WList<WValue>* list) {
    _type = WDataType::LIST;
    asList(list);
  }

  WValue(double value) {
    _type = WDataType::DOUBLE;
    asDouble(value);
  }

  WValue(int value) {
    _type = WDataType::INTEGER;
    asInt(value);
  }

  WValue(uint32_t value) {
    _type = WDataType::UNSIGNED_LONG;
    asUnsignedLong(value);
  }

  WValue(short value) {
    _type = WDataType::SHORT;
    asShort(value);
  }

  WValue(uint16_t value) {
    _type = WDataType::UNSIGNED_SHORT;
    asUnsignedShort(value);
  }

  WValue(byte value) {
    _type = WDataType::BYTE;
    asByte(value);
  }

  WValue(bool value) {
    _type = WDataType::BOOLEAN;
    asBool(value);
  }

  WValue(byte length, const byte* ba) {
    _type = WDataType::BYTE_ARRAY;
    asByteArray(length, ba);
  }

  virtual ~WValue() {
    if ((_type == WDataType::STRING) && (!_isNull)) delete[] _asString;
    if ((_type == WDataType::BYTE_ARRAY) && (!_isNull)) free(_asByteArray);
    if ((_type == WDataType::LIST) && (!_isNull)) delete _asList;
    if (_toString) delete[] _toString;
  }

  WDataType type() { return _type; }

  bool isNull() { return _isNull; }

  bool asBool() {
    if (!_isNull) {
      switch (_type) {
        case WDataType::BOOLEAN:
          return _asBool;
        case WDataType::STRING:
          return (strcmp_P(asString(), WC_TRUE) == 0);
      }
    }
    return false;
  }

  bool asBool(bool newValue) {
    bool changed = false;
    if (_type == WDataType::BOOLEAN) {
      changed = ((_isNull) || (_asBool != newValue));
      _asBool = newValue;
      _isNull = false;
    }
    return changed;
  }

  double asDouble() { return (!_isNull ? _asDouble : 0.0); }

  bool asDouble(double newValue) {
    bool changed = false;
    if (_type == WDataType::DOUBLE) {
      changed = ((_isNull) || (!isDoubleEqual(_asDouble, newValue, 0.01)));
      _asDouble = newValue;
      _isNull = false;
    }
    return changed;
  }

  short asShort() { return (!_isNull ? _asShort : 0); }

  bool asShort(short newValue) {
    bool changed = false;
    if (_type == WDataType::SHORT) {
      changed = ((_isNull) || (_asShort != newValue));
      _asShort = newValue;
      _isNull = false;
    }
    return changed;
  }

  uint16_t asUnsignedShort() { return (!_isNull ? _asUnsignedShort : 0); }

  bool asUnsignedShort(uint16_t newValue) {
    bool changed = false;
    if (_type == WDataType::UNSIGNED_SHORT) {
      changed = ((_isNull) || (_asUnsignedShort != newValue));
      _asUnsignedShort = newValue;
      _isNull = false;
    }
    return changed;
  }

  int asInt() {
    if (!_isNull) {
      switch (_type) {
        case WDataType::INTEGER:
          return _asInt;
        case WDataType::STRING:
          return atoi(_asString);
      }
    }
    return 0;
  }

  bool asInt(int newValue) {
    bool changed = false;
    if (_type == WDataType::INTEGER) {
      changed = ((_isNull) || (_asInt != newValue));
      _asInt = newValue;
      _isNull = false;
    }
    return changed;
  }

  bool isIntegerBetween(int lowerLimit, int upperLimit) {
    return ((!_isNull) && (_asInt >= lowerLimit) && (_asInt < upperLimit));
  }

  unsigned long asUnsignedLong() { return (!_isNull ? _asUnsignedLong : 0); }

  bool asUnsignedLong(unsigned long newValue) {
    bool changed = false;
    if (_type == WDataType::UNSIGNED_LONG) {
      changed = ((_isNull) || (_asUnsignedLong != newValue));
      _asUnsignedLong = newValue;
      _isNull = false;
    }
    return changed;
  }

  bool isUnsignedLongBetween(unsigned long lowerLimit, unsigned long upperLimit) {
    return ((!_isNull) && (_asUnsignedLong >= lowerLimit) && (_asUnsignedLong < upperLimit));
  }

  byte asByte() {
    if (!_isNull) {
      switch (_type) {
        case WDataType::BYTE:
          return _asByte;
        case WDataType::STRING:
          return atoi(_asString);
      }
    }
    return 0x00;
  }

  bool asByte(byte newValue) {
    bool changed = false;
    if (_type == WDataType::BYTE) {
      changed = ((_isNull) || (_asByte != newValue));
      _asByte = newValue;
      _isNull = false;
    }
    return changed;
  }

  bool asBit(byte bit) {
    return bitRead(asByte(), bit);
  }

  bool asBit(byte bit, bool value) {
    bool oldValue = asBit(bit);
    if (_type == WDataType::BYTE) {
      bitWrite(_asByte, bit, value);
    }
    return (oldValue != value);
  }

  byte* asByteArray() {
    if ((_type == WDataType::BYTE_ARRAY) && (length() > 0)) {
      byte* result = (byte*)malloc(length());
      for (int i = 0; i < length(); i++) {
        result[i] = _asByteArray[i + 1];
      }
      return result;
    } else {
      return 0;
    }
  }

  bool asByteArray(byte length, const byte* newValue) {
    bool changed = false;
    if (_type == WDataType::BYTE_ARRAY) {
      changed = ((_isNull) || (length != this->length()));
      if ((!_isNull) && (length != this->length())) {
        free(_asByteArray);
      }
      _asByteArray = (byte*)malloc(length + 1);
      _asByteArray[0] = length;
      for (int i = 0; i < length; i++) {
        changed = ((changed) || (_asByteArray[i + 1] != newValue[i]));
        _asByteArray[i + 1] = newValue[i];
      }
      if (changed) {
        _isNull = false;
      }
    }
    return changed;
  }

  byte byteArrayValue(byte index) { return _asByteArray[index + 1]; }

  bool byteArrayValue(byte index, byte newValue) {
    bool changed = false;
    if (_type == WDataType::BYTE_ARRAY) {
      changed = ((_isNull) || (_asByteArray[index + 1] != newValue));
      _asByteArray[index + 1] = newValue;
    }
    return changed;
  }

  bool byteArrayBitValue(byte byteIndex, byte bitIndex) {
    return bitRead(byteArrayValue(byteIndex), bitIndex);
  }

  bool byteArrayBitValue(byte byteIndex, byte bitIndex, bool bitValue) {
    if (_type != WDataType::BYTE_ARRAY) {
      return false;
    }
    byte v = byteArrayValue(byteIndex);
    if (bitValue) {
      bitSet(v, bitIndex);
    } else {
      bitClear(v, bitIndex);
    }
    return byteArrayValue(byteIndex, v);
  }

  char* asString() {
    if ((!_isNull) && (_type == WDataType::LIST)) {
      return "<list>";
    }
    if ((_isNull) || (_type != WDataType::STRING)) {
      return "";
    }
    return _asString;
  }

  bool asString(const char* newValue) {
    bool changed = false;
    if ((_type == WDataType::STRING) && ((!_isNull) || (newValue != nullptr))) {
      changed = (((_isNull) && (newValue != nullptr)) ||
                 ((!_isNull) && (newValue == nullptr)) ||
                 (strcmp_P(_asString, newValue) != 0));
      if (changed) {
        if (!_isNull) delete _asString;
        _isNull = (newValue == nullptr);
        if (!_isNull) {
          _asString = new char[strlen_P(newValue) + 1];
          strcpy_P(_asString, newValue);
        }
      }
    }
    return changed;
  }

  bool isStringEmpty() {
    return ((isNull()) || (strcmp_P(asString(), "") == 0));
  }

  WList<WValue>* asList() { return _asList; };

  bool asList(WList<WValue>* list) {
    bool changed = false;
    if (_type == WDataType::LIST) {
      changed = ((_isNull) || (_asList != list));
      _asList = list;
      _isNull = (_asList == nullptr);
    }
    return changed;
  }

  byte length() {
    switch (_type) {
      case WDataType::STRING:
        return ((!_isNull) && (_asString != nullptr) ? strlen(_asString) : 0);
      case WDataType::DOUBLE:
        return sizeof(double);
      case WDataType::SHORT:
        return sizeof(short);
      case WDataType::UNSIGNED_SHORT:
        return sizeof(uint16_t);
      case WDataType::INTEGER:
        return sizeof(int);
      case WDataType::UNSIGNED_LONG:
        return sizeof(unsigned long);
      case WDataType::BYTE:
      case WDataType::BOOLEAN:
        return 1;
      case WDataType::BYTE_ARRAY:
        return ((_asByteArray != nullptr) ? _asByteArray[0] : 0);
    }
    return 0;
  }

  virtual bool parse(const char* value) {
    String v = String(value);
    switch (_type) {
      case WDataType::BOOLEAN: {
        v.toLowerCase();
        return asBool(v.equals(WC_TRUE));
      }
      case WDataType::DOUBLE:
        return asDouble(v.toDouble());
      case WDataType::SHORT:
        return asShort(v.toInt());
      case WDataType::UNSIGNED_SHORT:
        return asUnsignedShort(v.toInt());
      case WDataType::INTEGER:
        return asInt(v.toInt());
      case WDataType::UNSIGNED_LONG:
        return asUnsignedLong(v.toInt());
      case WDataType::BYTE:
        return asByte(v.toInt());
      case WDataType::STRING:
        return asString(value);
      case WDataType::BYTE_ARRAY: /* tbi not implemented yet*/
        return false;
    }
    return false;
  }

  bool equals(WValue another) {
    bool result = ((_isNull) && (another.isNull()));
    if ((!result) && (!_isNull) && (!another.isNull())) {
      switch (_type) {
        case WDataType::BOOLEAN:
          return asBool() == another.asBool();
        case WDataType::DOUBLE:
          return (isDoubleEqual(asDouble(), another.asDouble(), 0.01));
        case WDataType::SHORT:
          return asShort() == another.asShort();
        case WDataType::UNSIGNED_SHORT:
          return asUnsignedShort() == another.asUnsignedShort();
        case WDataType::INTEGER:
          return asInt() == another.asInt();
        case WDataType::UNSIGNED_LONG:
          return asUnsignedLong() == another.asUnsignedLong();
        case WDataType::BYTE:
          return asByte() == another.asByte();
        case WDataType::STRING:
          return (strcmp_P(asString(), another.asString()) == 0);
        case WDataType::BYTE_ARRAY:
          // not implemented yet
          return false;
      }
    }
    return result;
  }

  bool equalOrLess(WValue another) {
    return ((lessThan(another)) || equals(another));
  }

  bool lessThan(WValue another) {
    if ((!_isNull) && (!another.isNull())) {
      switch (_type) {
        case WDataType::DOUBLE:
          return (isDoubleALessThanB(asDouble(), another.asDouble(), 0.01));
        case WDataType::SHORT:
          return asShort() < another.asShort();
        case WDataType::UNSIGNED_SHORT:
          return asUnsignedShort() < another.asUnsignedShort();
        case WDataType::INTEGER:
          return asInt() < another.asInt();
        case WDataType::UNSIGNED_LONG:
          return asUnsignedLong() < another.asUnsignedLong();
        case WDataType::BYTE:
          return asByte() < another.asByte();
        default:
          return false;
      }
    }
    return false;
  }

  bool moreThan(WValue another) {
    return ((!lessThan(another)) && (!equals(another)));
  }

  bool equalOrMore(WValue another) {
    return ((moreThan(another)) || equals(another));
  }

  const char* toString() {
    if (_type != WDataType::STRING) {
      if (!_toString) {
        WStringStream* ss = new WStringStream(20, false);
        this->toString(ss);
        _toString = ss->c_str();
        delete ss;
      }
      return _toString;
    } else {
      return asString();
    }
  }

  virtual void toString(Print* stream) {
    switch (_type) {
      case WDataType::BOOLEAN:
        WValue::boolean(stream, asBool());
        break;
      case WDataType::DOUBLE:
        WValue::numberDouble(stream, asDouble());
        break;
      case WDataType::INTEGER:
        WValue::numberInteger(stream, asInt());
        break;
      case WDataType::SHORT:
        WValue::numberShort(stream, asShort());
        break;
      case WDataType::UNSIGNED_SHORT:
        WValue::numberUnsignedShort(stream, asUnsignedShort());
        break;
      case WDataType::UNSIGNED_LONG:
        WValue::numberUnsignedLong(stream, asUnsignedLong());
        break;
      case WDataType::BYTE:
        WValue::numberByte(stream, asByte());
        break;
      case WDataType::STRING:
        WValue::string(stream, asString(), nullptr);
        break;
      case WDataType::BYTE_ARRAY:
        WValue::numberByteArray(stream, length(), asByteArray());
        break;
      case WDataType::LIST:
        stream->print(F("List: "));
        stream->print(asList()->size());
        stream->print(F(" items"));
        break;
      default:
        stream->print(F("n.a"));
    }
  }

  static bool isDoubleEqual(double a, double b, double precision) {
    double diff = a - b;
    return ((diff < precision) && (-diff < precision));
  }

  static bool isDoubleALessThanB(double a, double b, double precision) {
    double diff = b - a;
    return (diff >= precision);
  }

  static WValue empty(WDataType type) { return WValue(type); }

  static WValue ofString(const char* string) { return WValue(string); }

  static WValue ofPattern(const char* pattern, ...) {
    va_list args;
    va_start(args, pattern);
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), pattern, args);
    va_end(args);
    return WValue::ofString(buffer);
  }

  static WValue ofList(WList<WValue>* list) { return WValue(list); }

  static WValue ofDouble(double value) { return WValue(value); }

  static WValue ofInt(int value) { return WValue(value); }

  static WValue ofShort(short value) { return WValue(value); }

  static WValue ofUnsignedShort(uint16_t value) { return WValue(value); }

  static WValue ofByte(byte value) { return WValue(value); }

  static WValue ofUnsignedLong(unsigned long value) { return WValue(value); }

  static WValue ofBool(bool value) { return WValue(value); }

  static WValue ofByteArray(byte length, const byte* ba) { return WValue(length, ba); }

  static void boolean(Print* stream, bool value) {
    stream->print(value ? WC_TRUE : WC_FALSE);
  }

  static void numberDouble(Print* stream, double number) {
    stream->print(number);
  }

  static void numberInteger(Print* stream, int number) {
    stream->print(number, DEC);
  }

  static void numberShort(Print* stream, short number) {
    stream->print(number, DEC);
  }

  static void numberUnsignedShort(Print* stream, uint16_t number) {
    stream->print(number, DEC);
  }

  static void numberUnsignedLong(Print* stream, unsigned long number) {
    stream->print(number, DEC);
  }

  static void numberByte(Print* stream, byte number) {
    stream->print(number, DEC);
  }

  static void string(Print* stream, const char* text, ...) {
    va_list arg;
    va_start(arg, text);
    while (text) {
      int index = 0;
      while (text[index] != '\0') {
        switch (text[index]) {
          case '\n': {
            stream->print("\\n");
            break;
          }
          default:
            stream->print((char)text[index]);
        }
        index++;
      }
      // stream->printf_P(text);
      text = va_arg(arg, const char*);
    }
    va_end(arg);
  }

  static void numberByteArray(Print* stream, byte length, byte* value) {
    stream->print(WC_RBEGIN);
    for (byte i = 0; i < length; i++) {
      if (i != 0) stream->print(WC_COMMA);
      stream->print(value[i], DEC);
    }
    stream->print(WC_REND);
  }

 protected:
 private:
  WDataType _type;
  bool _isNull = true;
  const char* _toString = nullptr;
  union {
    bool _asBool;
    double _asDouble;
    short _asShort;
    uint16_t _asUnsignedShort;
    int _asInt;
    unsigned long _asUnsignedLong;
    byte _asByte;
    char* _asString;
    byte* _asByteArray;
    WList<WValue>* _asList;
  };
};

#endif