#ifndef W_PROPERTY_H
#define W_PROPERTY_H

#include <Arduino.h>

#include <list>

#include "WJson.h"
#include "WList.h"
#include "WStringStream.h"
#include "WValue.h"

// for reference see https://iot.mozilla.org/schemas
const char* TYPE_COLOR_PROPERTY = "ColorProperty";
const char* TYPE_FAN_MODE_PROPERTY = "FanModeProperty";
const char* TYPE_HEATING_COOLING_PROPERTY = "HeatingCoolingProperty";
const char* TYPE_LEVEL_PROPERTY = "LevelProperty";
const char* TYPE_BRIGHTNESS_PROPERTY = "BrightnessProperty";
const char* TYPE_ON_OFF_PROPERTY = "OnOffProperty";
const char* TYPE_OPEN_PROPERTY = "OpenProperty";
const char* TYPE_TARGET_TEMPERATURE_PROPERTY = "TargetTemperatureProperty";
const char* TYPE_THERMOSTAT_MODE_PROPERTY = "ThermostatModeProperty";
const char* TYPE_TEMPERATURE_PROPERTY = "TemperatureProperty";
const char* TYPE_HUMIDITY_PROPERTY = "HumidityProperty";
const char* TYPE_PUSHED_PROPERTY = "PushedProperty";

const static char UNIT_CELSIUS[] PROGMEM = "degree celsius";
const char* UNIT_PERCENT = "percent";

const char* VALUE_OFF = "off";
const char* VALUE_HEATING = "heating";
const char* VALUE_COOLING = "cooling";

enum WPropertyType {
  BOOLEAN,
  DOUBLE,
  SHORT,
  INTEGER,
  UNSIGNED_LONG,
  BYTE,
  STRING,
  BYTE_ARRAY
};

enum WPropertyVisibility { ALL,
                           NONE,
                           MQTT,
                           WEBTHING };

typedef std::function<void()> TOnPropertyChange;

class WProperty {
 public:
  WProperty(const char* title, WPropertyType type, const char* atType = "") {
    initialize(title, type, atType);
  }

  virtual ~WProperty() {
    if (_title) delete _title;
    if (_unit) delete _unit;
    if ((_type == STRING) && (_value.string)) {
      delete[] _value.string;
    }
    if ((_type == BYTE_ARRAY) && (_value.asByteArray)) {
      delete[] _value.asByteArray;
    }
  }

  void onValueRequest(TOnPropertyChange onValueRequest) { _onValueRequest = onValueRequest; }

  void addListener(TOnPropertyChange onChange) { _listeners.push_back(onChange); }

  void deviceNotification(TOnPropertyChange deviceNotification) { _deviceNotification = deviceNotification; }

  const char* title() { return _title; }

  byte length() {
    switch (_type) {
      case STRING:
        return (_valueNull ? 0 : strlen(_value.string));
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
        return (_valueNull ? 0 : byteArrayLength());
    }
    return 0;
  }

  WPropertyType type() { return _type; }

  void type(WPropertyType type) { _type = type; }

  const char* atType() { return _atType; }

  bool isNull() { return (_valueNull); }

  void setNull() { _valueNull = true; }

  bool requested() { return (_requested); }

  void requested(bool requested) {
    _requested = ((requested) && (!isNull()));
  }

  bool changed() { return (_changed); }

  void changed(bool changed) { _changed = changed; }

  virtual bool parse(const char* value) {
    if ((!readOnly()) && (value != nullptr)) {
      String v = String(value);
      switch (_type) {
        case BOOLEAN: {
          v.toLowerCase();
          asBool(v.equals("true"));
          return true;
        }
        case DOUBLE: {
          asDouble(v.toDouble());
          return true;
        }
        case SHORT: {
          asShort(v.toInt());
          return true;
        }
        case INTEGER: {
          asInt(v.toInt());
          return true;
        }
        case UNSIGNED_LONG: {
          asUnsignedLong(v.toInt());
          return true;
        }
        case BYTE: {
          asByte(v.toInt());
          return true;
        }
        case STRING: {
          asString(value);
          return true;
        }
        case BYTE_ARRAY: {
          // tbi not implemented yet
          return false;
        }
      }
    }
    return false;
  }

  bool asBool() {
    _requestValue();
    return (!_valueNull ? _value.asBool : false);
  }

  WProperty* asBool(bool newValue) {
    if (_type == BOOLEAN) {
      bool changed = ((_valueNull) || (_value.asBool != newValue));
      if (changed) {
        WValue valueB;
        valueB.asBool = newValue;
        this->value(valueB);
      }
    }
    return this;
  }

  void toggleBool() {
    if (_type == BOOLEAN) {      
      asBool(!asBool());
    }
  }

  double asDouble() {
    _requestValue();
    return (!_valueNull ? _value.asDouble : 0.0);
  }

  static bool isEqual(double a, double b, double precision) {
    double diff = a - b;
    return ((diff < precision) && (-diff < precision));
  }

  WProperty* asDouble(double newValue) {
    if (_type == DOUBLE) {
      bool changed = ((_valueNull) || (!isEqual(_value.asDouble, newValue, 0.01)));
      if (changed) {
        WValue valueB;
        valueB.asDouble = newValue;
        this->value(valueB);
      }
    }
    return this;
  }

  bool equalsDouble(double number) {
    return ((!_valueNull) &&
            (isEqual(_value.asDouble, number, 0.01)));
  }

  int asInt() {
    _requestValue();
    return (!_valueNull ? _value.asInt : 0);
  }

  WProperty* asInt(int newValue) {
    if (_type == INTEGER) {
      bool changed = ((_valueNull) || (_value.asInt != newValue));
      if (changed) {
        WValue valueB;
        valueB.asInt = newValue;
        this->value(valueB);
      }
    }
    return this;
  }

  short asShort() {
    _requestValue();
    return (!_valueNull ? _value.asShort : 0);
  }

  WProperty* asShort(short newValue) {
    if (_type == SHORT) {
      bool changed = ((_valueNull) || (_value.asShort != newValue));
      if (changed) {
        WValue valueB;
        valueB.asShort = newValue;
        this->value(valueB);
      }
    }
    return this;
  }

  unsigned long asUnsignedLong() {
    _requestValue();
    return (!_valueNull ? _value.asUnsignedLong : 0);
  }

  WProperty* asUnsignedLong(unsigned long newValue) {
    if (_type == UNSIGNED_LONG) {      
      bool changed = ((_valueNull) || (_value.asUnsignedLong != newValue));
      if (changed) {
        WValue valueB;
        valueB.asUnsignedLong = newValue;
        this->value(valueB);
      }
    }
    return this;
  }

  bool equalsInteger(int number) {
    return ((!_valueNull) && (_value.asInt == number));
  }

  bool isIntegerBetween(int lowerLimit, int upperLimit) {
    return ((!_valueNull) && (_value.asInt >= lowerLimit) &&
            (_value.asInt < upperLimit));
  }

  bool equalsShort(short number) {
    return ((!_valueNull) && (_value.asShort == number));
  }

  bool equalsString(const char* toCompare) {
    return ((!_valueNull) && (strcmp(_value.string, toCompare) == 0));
  }

  bool isStringEmpty() {
    return ((isNull()) || (equalsString("")));
  }

  bool equalsUnsignedLong(unsigned long number) {
    return ((!_valueNull) && (_value.asUnsignedLong == number));
  }

  bool isUnsignedLongBetween(unsigned long lowerLimit,
                             unsigned long upperLimit) {
    return ((!_valueNull) && (_value.asUnsignedLong >= lowerLimit) &&
            (_value.asUnsignedLong < upperLimit));
  }

  byte asByte() {
    _requestValue();
    return (!_valueNull ? _value.asByte : 0x00);
  }

  WProperty* asByte(byte newValue) {
    if (_type == BYTE) {      
      bool changed = ((_valueNull) || (_value.asByte != newValue));
      if (changed) {
        WValue valueB;
        valueB.asByte = newValue;
        this->value(valueB);
      }
    }
    return this;
  }

  bool equalsByte(byte number) {
    return ((!_valueNull) && (_value.asByte == number));
  }

  byte* asByteArray() {
    if (_type == BYTE_ARRAY) {
      byte length = byteArrayLength();
      if (length > 0) {
        byte* result = (byte*)malloc(length);
        for (int i = 0; i < length; i++) {
          result[i] = _value.asByteArray[i + 1];
        }
        return result;
      } else {
        return 0;
      }
    }
    return 0;
  }

  bool asByteArray(byte length, const byte* newValue) {
    if (_type == BYTE_ARRAY) {
     
      bool changed = ((_valueNull) || (length != (_value.asByteArray[0])));
      if ((!_valueNull) && (length != (_value.asByteArray[0]))) {
        free(_value.asByteArray);
      }
      _value.asByteArray = (byte*)malloc(length + 1);
      _value.asByteArray[0] = length;
      for (int i = 0; i < length; i++) {
        changed = ((changed) || (_value.asByteArray[i] != newValue[i]));
        _value.asByteArray[i + 1] = newValue[i];
      }
      if (changed) {
        _valueNull = false;
        _changed = true;
        valueChanged();
        _notify();
      }
      return changed;
    } else {
      return false;
    }
  }

  byte byteArrayLength() { return (!_valueNull ? _value.asByteArray[0] : 0); }

  byte byteArrayValue(byte index) { return _value.asByteArray[index + 1]; }

  bool byteArrayValue(byte index, byte newValue) {
    if (_type != BYTE_ARRAY) {
      return false;
    }
    bool changed = ((_valueNull) || (_value.asByteArray[index + 1] != newValue));
    if (changed) {
      _value.asByteArray[index + 1] = newValue;
      _valueNull = false;
      _changed = true;
      valueChanged();
      _notify();
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

  WValue value() { return _value; }

  char* c_str() {
    _requestValue();
    return _value.string;
  }

  char* asString() {
    return c_str();
  }

  WProperty* asString(const char* newValue) {
    if (_type != STRING) {
      return this;
    }
    bool changed = ((_valueNull) || (strcmp(_value.string, newValue) != 0));
    if ((changed) && (newValue != nullptr) && (this->hasEnums())) {
      // proceed only at valid enums
      changed = (enumIndex(this, newValue) != 0xFF);
    }
    if (changed) {
      if (!_valueNull) {
        free(_value.string);
      }

      if (newValue != nullptr) {
        int l = strlen(newValue);
        _value.string = (char*)malloc(l + 1);
        strncpy(_value.string, newValue, l);
        _value.string[l] = '\0';
        _valueNull = false;
      } else {
        _value.string = nullptr;
        _valueNull = true;
      }
      _changed = true;
      valueChanged();
      _notify();
    }
    return this;
  }

  bool readOnly() { return _readOnly; }

  void readOnly(bool readOnly) { _readOnly = readOnly; }

  const char* unit() { return _unit; }

  WProperty* unit(const char* unit) { 
    if (_unit) delete _unit;
    _unit = new char[strlen_P(unit) + 1];
    strcpy_P(_unit, unit);    
    return this; 
  }

  double multipleOf() { return _multipleOf; }

  void multipleOf(double multipleOf) { _multipleOf = multipleOf; }

  virtual void toJsonValue(WJson* json, const char* memberName = nullptr) {
    _requestValue();
    switch (_type) {
      case BOOLEAN:
        json->propertyBoolean(memberName, asBool());
        break;
      case DOUBLE:
        json->propertyDouble(memberName, asDouble());
        break;
      case INTEGER:
        json->propertyInteger(memberName, asInt());
        break;
      case SHORT:
        json->propertyShort(memberName, asShort());
        break;
      case UNSIGNED_LONG:
        json->propertyUnsignedLong(memberName, asUnsignedLong());
        break;
      case BYTE:
        json->propertyByte(memberName, asByte());
        break;
      case STRING:
        if (memberName != nullptr)
          json->propertyString(memberName, c_str(), nullptr);
        else
          json->onlyString(c_str());
        break;
      case BYTE_ARRAY:        
        json->propertyByteArray(memberName, length(), asByteArray());
        break;
    }
    _requested = true;
  }

  virtual void toString(Print* stream) {
    _requestValue();
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
    }
    if (_unit) {
      stream->print(_unit);
    }
    _requested = true;
  }

  virtual void toJsonStructure(WJson* json, const char* memberName,
                               const char* deviceHRef) {
    json->beginObject(memberName);
    // title
    if (this->title() != "") {
      json->propertyString("title", title(), nullptr);
    }
    // type
    switch (_type) {
      case BOOLEAN:
        json->propertyString("type", "boolean", nullptr);
        break;
      case DOUBLE:
      case SHORT:
      case INTEGER:
      case UNSIGNED_LONG:
      case BYTE:
        json->propertyString("type", "number", nullptr);
        break;
      case BYTE_ARRAY:
        json->propertyString("type", "object", nullptr);
        break;
      default:
        json->propertyString("type", "string", nullptr);
        break;
    }
    // readOnly
    if (this->readOnly()) {
      json->propertyBoolean("readOnly", true);
    }
    // unit
    if (this->unit() != "") {
      json->propertyString("unit", this->unit(), nullptr);
    }
    // multipleOf
    if (this->multipleOf() > 0.0) {
      json->propertyDouble("multipleOf", this->multipleOf());
    }
    // enum
    if (this->hasEnums()) {
      json->beginArray("enum");
      _enums->forEach([this, json](WProperty* propE, const char* id) {
        switch (_type) {
          case BOOLEAN:
            json->boolean(propE->asBool());
            break;
          case DOUBLE:
            json->numberDouble(propE->asDouble());
            break;
          case SHORT:
            json->numberShort(propE->asShort());
            break;
          case INTEGER:
            json->numberInteger(propE->asInt());
            break;
          case UNSIGNED_LONG:
            json->numberUnsignedLong(propE->asUnsignedLong());
            break;
          case BYTE:
            json->numberByte(propE->asByte());
            break;
          case STRING:
            json->string(propE->c_str(), nullptr);
            break;
        }
      });
      json->endArray();
    }
    // aType
    if (this->atType() != "") {
      json->propertyString("@type", this->atType(), nullptr);
    }
    toJsonStructureAdditionalParameters(json);    
    json->propertyString("href", deviceHRef, "/properties/", memberName, nullptr);    
    json->endObject();
  }

  void addEnumBoolean(bool enumBoolean) {
    if (_type != BOOLEAN) {
      return;
    }
    WProperty* valueE = new WProperty("", _type, "");
    valueE->asBool(enumBoolean);
    this->addEnum(valueE);
  }

  void addEnumNumber(double enumNumber) {
    if (_type != DOUBLE) {
      return;
    }
    WProperty* valueE = new WProperty("", _type, "");
    valueE->asDouble(enumNumber);
    this->addEnum(valueE);
  }

  void addEnumInteger(int enumNumber) {
    if (_type != INTEGER) {
      return;
    }
    WProperty* valueE = new WProperty("", _type, "");
    valueE->asInt(enumNumber);
    this->addEnum(valueE);
  }

  void addEnumShort(short enumNumber) {
    if (_type != SHORT) {
      return;
    }
    WProperty* valueE = new WProperty("", _type, "");
    valueE->asShort(enumNumber);
    this->addEnum(valueE);
  }

  void addEnumUnsignedLong(unsigned long enumNumber) {
    if (_type != UNSIGNED_LONG) {
      return;
    }
    WProperty* valueE = new WProperty("", _type, "");
    valueE->asUnsignedLong(enumNumber);
    this->addEnum(valueE);
  }

  void addEnumByte(byte enumByte) {
    if (_type != BYTE) {
      return;
    }
    WProperty* valueE = new WProperty("", _type, "");
    valueE->asByte(enumByte);
    this->addEnum(valueE);
  }

  void addEnumString(const char* enumString) {
    if (_type != STRING) {
      return;
    }
    WProperty* valueE = new WProperty("", _type, "");
    valueE->asString(enumString);
    this->addEnum(valueE);
  }

  byte enumIndex() { return enumIndex(this, this->value().string); }

  static byte enumIndex(WProperty* property, const char* enumString) {
    if ((property->hasEnums()) && (enumString != nullptr) && (property->type() == STRING)) {
      WProperty* en = property->_enums->getIf([property, enumString](WProperty* en) {
        return (strcmp(en->value().string, enumString) == 0);
      });
      return (en != nullptr ? property->_enums->indexOf(en) : 0xFF);
    } else {
      return 0xFF;
    }

    if ((property->type() != STRING) || (!property->hasEnums())) {
      return 0xFF;
    }
  }

  const char* enumString(byte enumIndex) {
    return enumString(this, enumIndex);
  }

  static const char* enumString(WProperty* property, byte enumIndex) {
    if ((property->hasEnums()) && (property->type() == STRING)) {
      WProperty* en = property->_enums->get(enumIndex);
      return (en != nullptr ? en->value().string : nullptr);
    } else {
      return nullptr;
    }
  }

  void clearEnums() {
    if (this->hasEnums()) {
      _enums->clear();
    }
  }

  void addEnum(WProperty* propEnum) {
    if (_enums == nullptr) {
      _enums = new WList<WProperty>();
    }
    _enums->add(propEnum);
  }

  bool hasEnums() { return (_enums != nullptr); }

  int enumsCount() { return _enums->size(); }

  WPropertyVisibility visibility() { return _visibility; }

  void visibility(WPropertyVisibility visibility) {
    _visibility = visibility;
  }

  void visibility(bool mqtt, bool webthing) {
    if ((mqtt) && (webthing)) {
      visibility(ALL);
    } else if ((!mqtt) && (!webthing)) {
      visibility(NONE);
    } else if (mqtt) {
      visibility(MQTT);
    } else {
      visibility(WEBTHING);
    }
  }

  void visibilityMqtt(bool value) {
    if ((value) && (_visibility != MQTT) && (_visibility != ALL)) {
      visibility(_visibility == WEBTHING ? ALL : MQTT);
    } else if ((!value) && (_visibility != NONE) && (_visibility != WEBTHING)) {
      visibility(_visibility == ALL ? WEBTHING : NONE);
    }
  }

  void visibilityWebthing(bool value) {
    if ((value) && (_visibility != WEBTHING) && (_visibility != ALL)) {
      visibility(_visibility == MQTT ? ALL : WEBTHING);
    } else if ((!value) && (_visibility != NONE) && (_visibility != MQTT)) {
      visibility(_visibility == ALL ? MQTT : NONE);
    }
  }

  bool isVisible(WPropertyVisibility visibility) {
    return ((_visibility == ALL) || (_visibility == visibility));
  }

 protected:
  const char* _atType;

  void initialize(const char* title, WPropertyType type, const char* atType) {
    if (title) {
      _title = new char[strlen_P(title) + 1];
      strcpy_P(_title, title);
    }
    _type = type;
    _visibility = ALL;
    _supportingWebthing = true;
    _valueNull = true;
    _changed = true;
    _requested = false;
    _valueRequesting = false;
    _notifying = false;
    _readOnly = false;
    _atType = atType;    
    _multipleOf = 0.0;
    _deviceNotification = nullptr;
    _enums = nullptr;
  }

  void value(WValue value) {
    _value = value;
    _valueNull = false;
    _changed = true;
    valueChanged();
    _notify();
  }

  virtual void valueChanged() {}

  virtual void toJsonStructureAdditionalParameters(WJson* json) {}

 private:
  char* _title = nullptr;
  WPropertyType _type;
  WPropertyVisibility _visibility;
  bool _supportingMqtt;
  bool _supportingWebthing;
  bool _readOnly;
  char* _unit = nullptr;
  double _multipleOf;
  std::list<TOnPropertyChange> _listeners;
  TOnPropertyChange _onValueRequest;
  TOnPropertyChange _deviceNotification;
  WValue _value = {false};
  bool _valueNull;
  bool _changed;
  bool _requested;
  bool _valueRequesting;
  bool _notifying;

  WList<WProperty>* _enums;

  void _notify() {
    if (!_valueRequesting) {
      _notifying = true;
      if (!_listeners.empty()) {
        for (std::list<TOnPropertyChange>::iterator f = _listeners.begin(); f != _listeners.end(); ++f) {          
          f->operator()();
        }
      }
      if (_deviceNotification) {
        _deviceNotification();
      }
      _notifying = false;
    }
  }

  void _requestValue() {
    if ((!_notifying) && (_onValueRequest)) {
      _valueRequesting = true;
      _onValueRequest();
      _valueRequesting = false;
    }
  }
};

class WRangeProperty : public WProperty {
 public:
  WRangeProperty(const char* title, WPropertyType type, WValue minimum, WValue maximum, const char* atType = TYPE_LEVEL_PROPERTY)
      : WProperty(title, type, atType) {
    _min = minimum;
    _max = maximum;
  }

  double getMinAsDouble() {
    return _min.asDouble;
  }

  int getMinAsInteger() {
    return _min.asInt;
  }

  double getMaxAsDouble() {
    return _max.asDouble;
  }

  int getMaxAsInteger() {
    return _max.asInt;
  }

  byte getScaledToMax0xFF() {
    int v = 0;
    switch (this->type()) {
      case DOUBLE: {
        v = (int)round(asDouble() * 0xFF / getMaxAsDouble());
        break;
      }
      case INTEGER: {
        v = asInt() * 0xFF / getMaxAsInteger();
        break;
      }
    }
    return (byte)v;
  }

  void toJsonStructureAdditionalParameters(WJson* json) {
    switch (this->type()) {
      case DOUBLE: {
        json->propertyDouble("minimum", getMinAsDouble());
        json->propertyDouble("maximum", getMaxAsDouble());
        break;
      }
      case INTEGER: {
        json->propertyInteger("minimum", getMinAsInteger());
        json->propertyInteger("maximum", getMaxAsInteger());
        break;
      }
    }
  }

 protected:
 private:
  WValue _min, _max;
};

class WColorProperty : public WProperty {
 public:
  WColorProperty(const char* title, byte red, byte green, byte blue)
      : WProperty(title, STRING, TYPE_COLOR_PROPERTY) {
    _red = red;
    _green = green;
    _blue = blue;
    setRGBString();
    // this->setRGB(red, green, blue);
    _changeValue = false;
  }

  byte red() { return _red; }

  byte green() { return _green; }

  byte blue() { return _blue; }

  void setRGB(byte red, byte green, byte blue) {
    if ((_red != red) || (_green != green) || (_blue != blue)) {
      _red = red;
      _green = green;
      _blue = blue;
      setRGBString();
    }
  }

  void setRGBString() {
    WStringStream result(7);
    result.print("#");
    char buffer[3];
    itoa(_red, buffer, 16);
    if (_red < 0x10) result.print("0");
    result.print(buffer);
    itoa(_green, buffer, 16);
    if (_green < 0x10) result.print("0");
    result.print(buffer);
    itoa(_blue, buffer, 16);
    if (_blue < 0x10) result.print("0");
    result.print(buffer);
    _changeValue = true;
    asString(result.c_str());
    _changeValue = false;
  }

  void parseRGBString() {
    char buffer[3];
    buffer[2] = '\0';
    buffer[0] = c_str()[1];
    buffer[1] = c_str()[2];
    _red = strtol(buffer, NULL, 16);
    buffer[0] = c_str()[3];
    buffer[1] = c_str()[4];
    _green = strtol(buffer, NULL, 16);
    buffer[0] = c_str()[5];
    buffer[1] = c_str()[6];
    _blue = strtol(buffer, NULL, 16);
  }

  bool parse(String value) {
    if ((!readOnly()) && (value != nullptr)) {
      if ((value.startsWith("#")) && (value.length() == 7)) {
        asString(value.c_str());
        return true;
      } else if ((value.startsWith("rgb(")) && (value.endsWith(")"))) {
        value = value.substring(4, value.length() - 1);
        int theComma;
        // red
        byte red = 0;
        if ((theComma = value.indexOf(",")) > -1) {
          red = value.substring(0, theComma).toInt();
          value = value.substring(theComma + 1);
        }
        // green
        byte green = 0;
        if ((theComma = value.indexOf(",")) > -1) {
          green = value.substring(0, theComma).toInt();
          value = value.substring(theComma + 1);
        }
        // blue
        byte blue = value.toInt();
        setRGB(red, green, blue);
      }
    }
    return false;
  }

 protected:
  virtual void valueChanged() {
    if (!_changeValue) {
      parseRGBString();
    }
  }

 private:
  bool _changeValue;
  byte _red, _green, _blue;
};

#endif
