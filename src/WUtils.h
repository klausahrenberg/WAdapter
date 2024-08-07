#ifndef W_UTILS_H
#define W_UTILS_H

#include "Arduino.h"
#include "WStringStream.h"

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
const static char WC_FALSE[] PROGMEM = "false";
const static char WC_TRUE[] PROGMEM = "true";

const static char* APPLICATION = nullptr;
const static char* VERSION = nullptr;
static byte FLAG_SETTINGS = 0x23;
static bool DEBUG = true;

class WUtils {
 public:
  static uint32_t getChipId() {
#ifdef ESP8266
    uint32_t ci = ESP.getChipId();
#elif ESP32
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    uint32_t ci = (macAddressTrunc >> 40);
#endif
    /*char* textToWrite =  new char[16];
    sprintf(textToWrite,"%lu", ci);
    return textToWrite;*/
    return ci;
  }

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
      stream->printf_P(text);
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
};

enum WDataType {
  BOOLEAN,
  DOUBLE,
  SHORT,
  INTEGER,
  UNSIGNED_LONG,
  BYTE,
  STRING,
  BYTE_ARRAY,
  LIST
};

typedef std::function<void()> WOnValueChange;

struct WValue {
 public:  
  WValue(WDataType type = BOOLEAN) {
    _type = type;
  }

  WValue(const char* string) {
    _type = STRING;
    asString(string);
  }

  WValue(WList<WValue>* list) {
    _type = LIST;
    asList(list);
  }

  WValue(double value) { 
    _type = DOUBLE;
    asDouble(value); 
  }

  WValue(int value) { 
    _type = INTEGER;
    asInt(value); 
  }

  WValue(uint32_t value) {
    _type = INTEGER;
    asInt(value); 
  }

  WValue(short value) { 
    _type = SHORT;
    asShort(value); 
  }

  WValue(byte value) { 
    _type = BYTE;
    asByte(value); 
  }

  WValue(unsigned long value) { 
    _type = UNSIGNED_LONG;
    asUnsignedLong(value); 
  }

  WValue(bool value) { 
    _type = BOOLEAN;
    asBool(value); 
  }

  WValue(byte length, const byte* ba) {
    _type = BYTE_ARRAY;
    asByteArray(length, ba);
  }

  virtual ~WValue() {
    if ((_type == STRING) && (!_isNull)) delete _asString;
    if ((_type == BYTE_ARRAY) && (!_isNull)) delete _asByteArray;
    if ((_type == LIST) && (!_isNull)) delete _asList;
    if (_toString) delete _toString;
  }

  WDataType type() { return _type; }

  bool isNull() { return _isNull; }
  
  bool asBool() { return (!_isNull ? _asBool : false); }

  bool asBool(bool newValue) {
    bool changed = false;
    if (_type == BOOLEAN) {
      changed = ((_isNull) || (_asBool != newValue));
      _asBool = newValue;       
      _isNull = false; 
    }
    return changed;
  }

  double asDouble() { return (!_isNull ? _asDouble : 0.0); }

  bool asDouble(double newValue) {
    bool changed = false;
    if (_type == DOUBLE) {
      changed = ((_isNull) || (!isEqual(_asDouble, newValue, 0.01)));
      _asDouble = newValue;      
      _isNull = false;
    }
    return changed;
  }

  bool equalsDouble(double number) {
    return ((_type == DOUBLE) && (!_isNull) && (isEqual(_asDouble, number, 0.01)));
  }

  short asShort() { return (!_isNull ? _asShort : 0); }

  bool asShort(short newValue) {
    bool changed = false;
    if (_type == SHORT) {
      changed = ((_isNull) || (_asShort != newValue));
      _asShort = newValue;
      _isNull = false;
    }
    return changed;
  }

  bool equalsShort(short number) {
    return ((!_isNull) && (_asShort == number));
  }

  int asInt() { return (!_isNull ? _asInt : 0); }

  bool asInt(int newValue) {
    bool changed = false;
    if (_type == INTEGER) {
      bool changed = false;
      changed = ((_isNull) || (_asInt != newValue));
      _asInt = newValue;
      _isNull = false;
    }
    return changed;
  }

  bool equalsInteger(int number) {
    return ((!_isNull) && (_asInt == number));
  }

  bool isIntegerBetween(int lowerLimit, int upperLimit) {
    return ((!_isNull) && (_asInt >= lowerLimit) && (_asInt < upperLimit));
  }

  unsigned long asUnsignedLong() { return (!_isNull ? _asUnsignedLong : 0); }

  bool asUnsignedLong(unsigned long newValue) {
    bool changed = false;
    if (_type == UNSIGNED_LONG) {      
      changed = ((_isNull) || (_asUnsignedLong != newValue));
      _asUnsignedLong = newValue;
      _isNull = false;
    }
    return changed;
  }

  bool equalsUnsignedLong(unsigned long number) {
    return ((!_isNull) && (_asUnsignedLong == number));
  }

  bool isUnsignedLongBetween(unsigned long lowerLimit, unsigned long upperLimit) {
    return ((!_isNull) && (_asUnsignedLong >= lowerLimit) && (_asUnsignedLong < upperLimit));
  }

  byte asByte() { return (!_isNull ? _asByte : 0x00); }

  bool asByte(byte newValue) {
    bool changed = false;
    if (_type == BYTE) {
      changed = ((_isNull) || (_asByte != newValue));
      _asByte = newValue;
      _isNull = false;
    }
    return changed;
  }

  bool equalsByte(byte number) {
    return ((!_isNull) && (_asByte == number));
  }

  byte* asByteArray() {
    if ((_type == BYTE_ARRAY) && (length() > 0)) {
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
    if (_type == BYTE_ARRAY) {     
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
    if (_type == BYTE_ARRAY) {      
      changed = ((_isNull) || (_asByteArray[index + 1] != newValue));
      _asByteArray[index + 1] = newValue;            
    }
    return changed;
  }

  bool byteArrayBitValue(byte byteIndex, byte bitIndex) {
    return bitRead(byteArrayValue(byteIndex), bitIndex);
  }

  bool byteArrayBitValue(byte byteIndex, byte bitIndex, bool bitValue) {
    if (_type != BYTE_ARRAY) {
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

  char* c_str() { return _asString; }

  char* asString() { return _asString; }

  bool asString(const char* newValue) {
    bool changed = false;
    if (_type == STRING) {
      changed = ((_isNull) || (strcmp_P(_asString, newValue) != 0));      
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

  bool equalsString(const char* toCompare) {
    return ((!_isNull) && (strcmp(_asString, toCompare) == 0));
  }

  bool isStringEmpty() {
    return ((isNull()) || (equalsString("")));
  }
  
  WList<WValue>* asList() { return _asList; };

  bool asList(WList<WValue>* list) {
    bool changed = false;
    if (_type == LIST) {      
      changed = ((_isNull) || (_asList != list));
      _asList = list;
      _isNull = (_asList == nullptr);
    }    
    return changed;
  }

  byte length() {
    switch (_type) {
      case STRING:
        return ((_asString != nullptr) ? strlen(_asString) : 0);
      case DOUBLE:
        return sizeof(double);
      case SHORT:
        return sizeof(short);
      case INTEGER:
        return sizeof(int);
      case UNSIGNED_LONG:
        return sizeof(unsigned long);
      case BYTE:
      case BOOLEAN:
        return 1;
      case BYTE_ARRAY:
        return ((_asByteArray != nullptr) ? _asByteArray[0] : 0);
    }
    return 0;
  }

  virtual bool parse(const char* value) {
    String v = String(value);
    switch (_type) {
      case BOOLEAN: {
        v.toLowerCase();
        return asBool(v.equals(WC_TRUE));
      }
      case DOUBLE: return asDouble(v.toDouble());
      case SHORT: return asShort(v.toInt());
      case INTEGER: return asInt(v.toInt());
      case UNSIGNED_LONG: return asUnsignedLong(v.toInt());
      case BYTE: return asByte(v.toInt());
      case STRING: return asString(value);
      case BYTE_ARRAY: /* tbi not implemented yet*/ return false;      
    }
    return false;
  }

  // byte byteArrayLength() { return (!_valueNull ? _value.asByteArray[0] : 0); }

  const char* toString() {
    if (_type != STRING) {      
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
      case BOOLEAN:
        WUtils::boolean(stream, asBool());
        break;
      case DOUBLE:
        WUtils::numberDouble(stream, asDouble());
        break;
      case INTEGER:
        WUtils::numberInteger(stream, asInt());
        break;
      case SHORT:
        WUtils::numberShort(stream, asShort());
        break;
      case UNSIGNED_LONG:
        WUtils::numberUnsignedLong(stream, asUnsignedLong());
        break;
      case BYTE:
        WUtils::numberByte(stream, asByte());
        break;
      case STRING:
        WUtils::string(stream, c_str(), nullptr);
        break;
      case BYTE_ARRAY:   
        WUtils::numberByteArray(stream, length(), asByteArray());
        break;
      case LIST:
        stream->print(F("List: "));
        stream->print(asList()->size());
        stream->print(F(" items"));
        break;  
      default:
        stream->print(F("n.a"));  
    }
  }

  static bool isEqual(double a, double b, double precision) {
    double diff = a - b;
    return ((diff < precision) && (-diff < precision));
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

  static WValue ofByte(byte value) { return WValue(value); }

  static WValue ofUnsignedLong(unsigned long value) { return WValue(value); }

  static WValue ofBool(bool value) { return WValue(value); }

  static WValue ofByteArray(byte length, const byte* ba) { return WValue(length, ba); }

 protected:  

 private:
  WDataType _type;
  bool _isNull = true;
  const char* _toString = nullptr;
  union {
    bool _asBool;
    double _asDouble;
    short _asShort;
    int _asInt;
    unsigned long _asUnsignedLong;
    byte _asByte;
    char* _asString;
    byte* _asByteArray;
    WList<WValue>* _asList;
  };
};

class IWStorable {
public:
  //virtual void mode(uint8_t pin, uint8_t mode);
  //virtual bool readInput(uint8_t pin);
  //virtual void writeOutput(uint8_t pin, bool value);
}; 

#endif