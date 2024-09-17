#ifndef W_PROPERTY_H
#define W_PROPERTY_H

#include <Arduino.h>

#include <list>

#include "WJson.h"
#include "WList.h"
#include "WStringStream.h"

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

enum WPropertyVisibility { ALL,
                           NONE,
                           MQTT,
                           WEBTHING };

typedef std::function<void()> TOnPropertyChange;

class WProperty {
 public:
  WProperty(const char* title, WDataType type, const char* atType = "") {
    _initialize(title, type, atType);
  }

  virtual ~WProperty() {
    if (_value) delete _value;
    if (_title) delete _title;    
    if (_unit) delete _unit;
    if (_enums) delete _enums;
  }

  void onValueRequest(TOnPropertyChange onValueRequest) { _onValueRequest = onValueRequest; }

  void addListener(TOnPropertyChange onChange) { _listeners.push_back(onChange); }

  void deviceNotification(TOnPropertyChange deviceNotification) { _deviceNotification = deviceNotification; }

  const char* title() { return _title; }

  byte length() {
    return _value->length();
  }

  WDataType type() { return _value->type(); }  

  const char* atType() { return _atType; }

  bool isNull() { return _value->isNull(); }

  bool requested() { return (_requested); }

  void requested(bool requested) {
    _requested = ((requested) && (!isNull()));
  }

  bool changed() { return (_changed); }

  void changed(bool changed) { _changed = changed; }

  virtual bool parse(const char* value) {  
    if (!_readOnly) {
      _changed = _value->parse(value) || _changed;
      if (_changed) _notify();
      return _changed;
    } else {
      return false;
    }    
  }    

  //WValue* value() { return _value; }

  bool asBool() { _requestValue(); return _value->asBool(); }

  WProperty* asBool(bool value) {
    if (!_readOnly) {
      _changed = _value->asBool(value) || _changed;
      if (_changed) _notify();
    }
    return this;
  }  

  char* asString() { _requestValue(); return _value->asString(); }

  WProperty* asString(const char* value) {  
    if (!_readOnly) {
      _changed = _value->asString(value) || _changed;
      if (_changed) _notify();
    }
    return this;
  }  

  bool isStringEmpty() { _requestValue(); return _value->isStringEmpty(); }

  bool equalsString(const char* toCompare) { _requestValue(); return _value->equalsString(toCompare); }

  int asInt() { _requestValue(); return _value->asInt(); }

  WProperty* asInt(int value) {
    if (!_readOnly) {
      _changed = _value->asInt(value) || _changed;
      if (_changed) _notify();
    }
    return this;
  }

  double asDouble() { _requestValue(); return _value->asDouble(); }

  WProperty* asDouble(double value) {
    if (!_readOnly) {
      _changed = _value->asDouble(value) || _changed;
      if (_changed) _notify();
    }
    return this;
  }  

  byte* asByteArray() { _requestValue(); return _value->asByteArray(); }

  bool asByteArray(byte length, const byte* value) {
    if (!_readOnly) {
      _changed = _value->asByteArray(length, value) || _changed;
      if (_changed) _notify();
    }
    return this;
  }  

  byte byteArrayValue(byte index) { return _value->byteArrayValue(index); }

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
    switch (_value->type()) {
      case BOOLEAN:
        json->propertyBoolean(memberName, _value->asBool());
        break;
      case DOUBLE:
        json->propertyDouble(memberName, _value->asDouble());
        break;
      case INTEGER:
        json->propertyInteger(memberName, _value->asInt());
        break;
      case SHORT:
        json->propertyShort(memberName, _value->asShort());
        break;
      case UNSIGNED_LONG:
        json->propertyUnsignedLong(memberName, _value->asUnsignedLong());
        break;
      case BYTE:
        json->propertyByte(memberName, _value->asByte());
        break;
      case STRING:
        if (memberName != nullptr)
          json->propertyString(memberName, _value->asString(), nullptr);
        else
          json->onlyString(_value->asString());
        break;
      case BYTE_ARRAY:        
        json->propertyByteArray(memberName, length(), _value->asByteArray());
        break;
    }
    _requested = true;
  }

  virtual void toString(Print* stream) {
    _requestValue();
    _value->toString(stream);
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
    switch (_value->type()) {
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
    if (this->unit() != nullptr) {
      json->propertyString("unit", this->unit(), nullptr);
    }
    // multipleOf
    if (this->multipleOf() > 0.0) {
      json->propertyDouble("multipleOf", this->multipleOf());
    }
    // enum
    if (this->hasEnums()) {
      json->beginArray("enum");
      _enums->forEach([this, json](int index, WValue* propE, const char* id) {
        switch (_value->type()) {
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
            json->string(propE->asString(), nullptr);
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
    if (_value->type() != BOOLEAN) {
      return;
    }
    this->addEnum(new WValue(enumBoolean));
  }

  void addEnumNumber(double enumNumber) {
    if (_value->type() != DOUBLE) {
      return;
    }    
    this->addEnum(new WValue(enumNumber));
  }

  void addEnumInteger(int enumNumber) {
    if (_value->type() != INTEGER) {
      return;
    }
    this->addEnum(new WValue(enumNumber));
  }

  void addEnumShort(short enumNumber) {
    if (_value->type() != SHORT) {
      return;
    }
    this->addEnum(new WValue(enumNumber));
  }

  void addEnumUnsignedLong(unsigned long enumNumber) {
    if (_value->type() != UNSIGNED_LONG) {
      return;
    }
    this->addEnum(new WValue(enumNumber));
  }

  void addEnumByte(byte enumByte) {
    if (_value->type() != BYTE) {
      return;
    }
    this->addEnum(new WValue(enumByte));
  }

  void addEnumString(const char* enumString) {
    if (_value->type() != STRING) {
      return;
    }
    this->addEnum(new WValue(enumString));
  }

  byte enumIndex() { return enumIndex(this, _value->asString()); }

  static byte enumIndex(WProperty* property, const char* enumString) {
    if ((property->hasEnums()) && (enumString != nullptr) && (property->type() == STRING)) {
      WValue* en = property->_enums->getIf([property, enumString](WValue* en) {
        return (strcmp(en->asString(), enumString) == 0);
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
      WValue* en = property->_enums->get(enumIndex);
      return (en != nullptr ? en->asString() : nullptr);
    } else {
      return nullptr;
    }
  }

  void clearEnums() {
    if (this->hasEnums()) {
      _enums->clear();
    }
  }

  void addEnum(WValue* enumValue) {
    if (_enums == nullptr) {
      _enums = new WList<WValue>();
    }
    _enums->add(enumValue);
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
  WValue* _value; 
  bool _changed;

  void _initialize(const char* title, WDataType type, const char* atType) {
    if (title) {
      _title = new char[strlen_P(title) + 1];
      strcpy_P(_title, title);
    }
    _value = new WValue(type);
    _visibility = ALL;
    _supportingWebthing = true;    
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

  virtual void valueChanged() {}

  virtual void toJsonStructureAdditionalParameters(WJson* json) {}

 private:
  char* _title = nullptr;  
  WPropertyVisibility _visibility;
  bool _supportingMqtt;
  bool _supportingWebthing;
  bool _readOnly;
  char* _unit = nullptr;
  double _multipleOf;
  std::list<TOnPropertyChange> _listeners;
  TOnPropertyChange _onValueRequest;
  TOnPropertyChange _deviceNotification;   
  bool _requested;
  bool _valueRequesting;
  bool _notifying;
  WList<WValue>* _enums;

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
  WRangeProperty(const char* title, WDataType type, WValue minimum, WValue maximum, const char* atType = TYPE_LEVEL_PROPERTY)
      : WProperty(title, type, atType) {
    _min = minimum;
    _max = maximum;
  }

  ~WRangeProperty() {
  }

  double getMinAsDouble() {
    return _min.asDouble();
  }

  int getMinAsInteger() {
    return _min.asInt();
  }

  double getMaxAsDouble() {
    return _max.asDouble();
  }

  int getMaxAsInteger() {
    return _max.asInt();
  }

  byte getScaledToMax0xFF() {
    int v = 0;
    switch (this->type()) {
      case DOUBLE: {
        v = (int)round(_value->asDouble() * 0xFF / getMaxAsDouble());
        break;
      }
      case INTEGER: {
        v = _value->asInt() * 0xFF / getMaxAsInteger();
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
  WValue _min;
  WValue _max;
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
    buffer[0] = asString()[1];
    buffer[1] = asString()[2];
    _red = strtol(buffer, NULL, 16);
    buffer[0] = asString()[3];
    buffer[1] = asString()[4];
    _green = strtol(buffer, NULL, 16);
    buffer[0] = asString()[5];
    buffer[1] = asString()[6];
    _blue = strtol(buffer, NULL, 16);
  }

  bool parse(String value) {
    if ((!readOnly()) && (value != nullptr)) {
      if ((value.startsWith("#")) && (value.length() == 7)) {
        this->asString(value.c_str());
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
