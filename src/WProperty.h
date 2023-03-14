#ifndef W_PROPERTY_H
#define W_PROPERTY_H

#include <Arduino.h>
#include "WJson.h"
#include "WList.h"
#include "WValue.h"
#include "WOutput.h"
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

const char* UNIT_CELSIUS = "degree celsius";
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

enum WPropertyVisibility { ALL, NONE, MQTT, WEBTHING };

class WProperty {
 public:
  typedef std::function<void(WProperty* property)> TOnPropertyChange;  

  WProperty(const char* id, const char* title, WPropertyType type,
            const char* atType) {
    initialize(id, title, type, atType);
  }

  ~WProperty() {
    delete _id;
    delete _title;
    if ((_type == STRING) && (_value.string)) {
      delete[] _value.string;
    }
    if ((_type == BYTE_ARRAY) && (_value.asByteArray)) {
      delete[] _value.asByteArray;
    }
  }

  void setOnValueRequest(TOnPropertyChange onValueRequest) {
    _onValueRequest = onValueRequest;
  }

  void setOnChange(TOnPropertyChange onChange) { _onChange = onChange; }

  void setDeviceNotification(TOnPropertyChange deviceNotification) {
    _deviceNotification = deviceNotification;
  }

  const char* id() { return _id; }

  const char* title() { return _title; }

  WPropertyType type() { return _type; }

  byte getLength() {
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
        return (_valueNull ? 0 : sizeof(_value.asByteArray));
    }
    return 0;
  }

  void setType(WPropertyType type) { _type = type; }

  const char* atType() { return _atType; }

  bool isNull() { return (_valueNull); }

  bool isRequested() { return (_requested); }

  void setRequested(bool requested) {
    _requested = ((requested) && (!isNull()));
  }

  void setNull() { _valueNull = true; }

  bool isChanged() { return (_changed); }

  void setUnChanged() { _changed = false; }

  virtual bool parse(String value) {
    if ((!isReadOnly()) && (value != nullptr)) {
      switch (_type) {
        case BOOLEAN: {
          value.toLowerCase();
          setBoolean(value.equals("true"));
          return true;
        }
        case DOUBLE: {
          setDouble(value.toDouble());
          return true;
        }
        case SHORT: {
          setShort(value.toInt());
          return true;
        }
        case INTEGER: {
          setInteger(value.toInt());
          return true;
        }
        case UNSIGNED_LONG: {
          setUnsignedLong(value.toInt());
          return true;
        }
        case BYTE: {
          setByte(value.toInt());
          return true;
        }
        case STRING: {
          setString(value.c_str());
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

  bool getBoolean() {
    requestValue();
    return (!_valueNull ? _value.asBoolean : false);
  }

  void setBoolean(bool newValue) {
    if (_type != BOOLEAN) {
      return;
    }
    bool changed = ((_valueNull) || (_value.asBoolean != newValue));
    if (changed) {
      WValue valueB;
      valueB.asBoolean = newValue;
      this->setValue(valueB);
    }
  }

  void toggleBoolean() {
    if (_type != BOOLEAN) {
      return;
    }
    setBoolean(!getBoolean());
  }

  double getDouble() {
    requestValue();
    return (!_valueNull ? _value.asDouble : 0.0);
  }

  static bool isEqual(double a, double b, double precision) {
    double diff = a - b;
    return ((diff < precision) && (-diff < precision));
  }

  void setDouble(double newValue) {
    if (_type != DOUBLE) {
      return;
    }
    bool changed = ((_valueNull) || (!isEqual(_value.asDouble, newValue, 0.01)));
    if (changed) {
      WValue valueB;
      valueB.asDouble = newValue;
      this->setValue(valueB);
    }
  }

  bool equalsDouble(double number) {
    return ((!_valueNull) &&
            (isEqual(_value.asDouble, number, 0.01)));
  }

  int getInteger() {
    requestValue();
    return (!_valueNull ? _value.asInteger : 0);
  }

  void setInteger(int newValue) {
    if (_type != INTEGER) {
      return;
    }
    bool changed = ((_valueNull) || (_value.asInteger != newValue));
    if (changed) {
      WValue valueB;
      valueB.asInteger = newValue;
      this->setValue(valueB);
    }
  }

  short getShort() {
    requestValue();
    return (!_valueNull ? _value.asShort : 0);
  }

  void setShort(short newValue) {
    if (_type != SHORT) {
      return;
    }
    bool changed = ((_valueNull) || (_value.asShort != newValue));
    if (changed) {
      WValue valueB;
      valueB.asShort = newValue;
      this->setValue(valueB);
    }
  }

  unsigned long getUnsignedLong() {
    requestValue();
    return (!_valueNull ? _value.asUnsignedLong : 0);
  }

  void setUnsignedLong(unsigned long newValue) {
    if (_type != UNSIGNED_LONG) {
      return;
    }
    bool changed =
        ((_valueNull) || (_value.asUnsignedLong != newValue));
    if (changed) {
      WValue valueB;
      valueB.asUnsignedLong = newValue;
      this->setValue(valueB);
    }
  }

  bool equalsInteger(int number) {
    return ((!_valueNull) && (_value.asInteger == number));
  }

  bool isIntegerBetween(int lowerLimit, int upperLimit) {
    return ((!_valueNull) && (_value.asInteger >= lowerLimit) &&
            (_value.asInteger < upperLimit));
  }

  bool equalsShort(short number) {
    return ((!_valueNull) && (_value.asShort == number));
  }

  bool equalsId(const char* id) {
    return ((id != nullptr) && (_id != nullptr) && (strcmp(_id, id) == 0));
  }

  bool equalsString(const char* toCompare) {
    return ((!_valueNull) && (strcmp(_value.string, toCompare) == 0));
  }

  bool equalsUnsignedLong(unsigned long number) {
    return ((!_valueNull) && (_value.asUnsignedLong == number));
  }

  bool isUnsignedLongBetween(unsigned long lowerLimit,
                             unsigned long upperLimit) {
    return ((!_valueNull) && (_value.asUnsignedLong >= lowerLimit) &&
            (_value.asUnsignedLong < upperLimit));
  }

  byte getByte() {
    requestValue();
    return (!_valueNull ? _value.asByte : 0x00);
  }

  void setByte(byte newValue) {
    if (_type != BYTE) {
      return;
    }
    bool changed = ((_valueNull) || (_value.asByte != newValue));
    if (changed) {
      WValue valueB;
      valueB.asByte = newValue;
      this->setValue(valueB);
    }
  }

  bool equalsByte(byte number) {
    return ((!_valueNull) && (_value.asByte == number));
  }

  char* c_str() {
    requestValue();
    return _value.string;
  }

  byte* getByteArray() { return _value.asByteArray; }

  bool setByteArray(const byte* newValue) {
    if (_type != BYTE_ARRAY) {
      return false;
    }
    byte newLength = sizeof(newValue);
    bool changed =
        ((_valueNull) || (newLength != (sizeof(_value.asByteArray))));
    if ((!_valueNull) && (newLength != (sizeof(_value.asByteArray)))) {
      free(_value.asByteArray);
      _value.asByteArray = (byte*)malloc(newLength);
    }
    _value.asByteArray = (byte*)malloc(sizeof(newValue));
    for (int i = 0; i < newLength; i++) {
      changed = ((changed) || (_value.asByteArray[i] != newValue[i]));
      _value.asByteArray[i] = newValue[i];
    }
    if (changed) {
      _valueNull = false;
      _changed = true;
      valueChanged();
      notify();
    }
    return changed;
  }

  byte getByteArrayValue(byte index) { return _value.asByteArray[index]; }

  bool getByteArrayBitValue(byte byteIndex, byte bitIndex) {
    return bitRead(getByteArrayValue(byteIndex), bitIndex);
  }

  WValue getValue() { return _value; }

  bool setString(const char* newValue) {
    if (_type != STRING) {
      return false;
    }
    bool changed = ((_valueNull) || (strcmp(_value.string, newValue) != 0));
    if ((changed) && (newValue != nullptr) && (this->hasEnums())) {
      // proceed only at valid enums
      changed = (getEnumIndex(this, newValue) != 0xFF);
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
        // value.string[0] = '\0';
        _valueNull = true;
      }
      _changed = true;
      valueChanged();
      notify();
    }
    return changed;
  }

  bool setByteArrayValue(byte index, byte newValue) {
    if (_type != BYTE_ARRAY) {
      return false;
    }
    bool changed =
        ((_valueNull) || (_value.asByteArray[index] != newValue));
    if (changed) {
      _value.asByteArray[index] = newValue;
      _valueNull = false;
      _changed = true;
      valueChanged();
      notify();
    }
    return changed;
  }

  bool setByteArrayBitValue(byte byteIndex, byte bitIndex, bool bitValue) {
    if (_type != BYTE_ARRAY) {
      return false;
    }
    byte v = getByteArrayValue(byteIndex);
    if (bitValue) {
      bitSet(v, bitIndex);
    } else {
      bitClear(v, bitIndex);
    }
    return setByteArrayValue(byteIndex, v);
  }

  bool isReadOnly() { return _readOnly; }

  void setReadOnly(bool readOnly) { _readOnly = readOnly; }

  const char* getUnit() { return _unit; }

  void setUnit(const char* unit) { _unit = unit; }

  double getMultipleOf() { return _multipleOf; }

  void setMultipleOf(double multipleOf) { _multipleOf = multipleOf; }

  virtual void toJsonValue(WJson* json, bool onlyValue = false) {
    requestValue();
    const char* memberName = (onlyValue ? nullptr : id());
    switch (_type) {
      case BOOLEAN:
        json->propertyBoolean(memberName, getBoolean());
        break;
      case DOUBLE:
        json->propertyDouble(memberName, getDouble());
        break;
      case INTEGER:
        json->propertyInteger(memberName, getInteger());
        break;
      case SHORT:
        json->propertyShort(memberName, getShort());
        break;
      case UNSIGNED_LONG:
        json->propertyUnsignedLong(memberName, getUnsignedLong());
        break;
      case BYTE:
        json->propertyByte(memberName, getByte());
        break;
      case STRING:
        if (!onlyValue)
          json->propertyString(memberName, c_str());
        else
          json->onlyString(c_str());
        break;
      case BYTE_ARRAY:
        // tbi
        json->propertyByteArray(memberName, getLength(), _value.asByteArray);
        break;
    }
    _requested = true;
  }

  virtual void toJsonStructure(WJson* json, const char* memberName,
                               const char* deviceHRef) {
    json->beginObject(memberName);
    // title
    if (this->title() != "") {
      json->propertyString("title", title());
    }
    // type
    switch (_type) {
      case BOOLEAN:
        json->propertyString("type", "boolean");
        break;
      case DOUBLE:
      case SHORT:
      case INTEGER:
      case UNSIGNED_LONG:
      case BYTE:
        json->propertyString("type", "number");
        break;
      case BYTE_ARRAY:
        json->propertyString("type", "object");
        break;
      default:
        json->propertyString("type", "string");
        break;
    }
    // readOnly
    if (this->isReadOnly()) {
      json->propertyBoolean("readOnly", true);
    }
    // unit
    if (this->getUnit() != "") {
      json->propertyString("unit", this->getUnit());
    }
    // multipleOf
    if (this->getMultipleOf() > 0.0) {
      json->propertyDouble("multipleOf", this->getMultipleOf());
    }
    // enum
    if (this->hasEnums()) {
      json->beginArray("enum");
      _enums->forEach([this, json](WProperty* propE){
        switch (_type) {
          case BOOLEAN:
            json->boolean(propE->getBoolean());
            break;
          case DOUBLE:
            json->numberDouble(propE->getDouble());
            break;
          case SHORT:
            json->numberShort(propE->getShort());
            break;
          case INTEGER:
            json->numberInteger(propE->getInteger());
            break;
          case UNSIGNED_LONG:
            json->numberUnsignedLong(propE->getUnsignedLong());
            break;
          case BYTE:
            json->numberByte(propE->getByte());
            break;
          case STRING:
            json->string(propE->c_str());
            break;
        }
      });
      json->endArray();
    }
    // aType
    if (this->atType() != "") {
      json->propertyString("@type", this->atType());
    }
    toJsonStructureAdditionalParameters(json);
    // json->beginArray("links");
    // json->beginObject();
    json->propertyString("href", deviceHRef, "/properties/", id());
    // json->endObject();
    // json->endArray();
    json->endObject();
  }

  void addEnumBoolean(bool enumBoolean) {
    if (_type != BOOLEAN) {
      return;
    }
    WProperty* valueE = new WProperty("", "", _type, "");
    valueE->setBoolean(enumBoolean);
    this->addEnum(valueE);
  }

  void addEnumNumber(double enumNumber) {
    if (_type != DOUBLE) {
      return;
    }
    WProperty* valueE = new WProperty("", "", _type, "");
    valueE->setDouble(enumNumber);
    this->addEnum(valueE);
  }

  void addEnumInteger(int enumNumber) {
    if (_type != INTEGER) {
      return;
    }
    WProperty* valueE = new WProperty("", "", _type, "");
    valueE->setInteger(enumNumber);
    this->addEnum(valueE);
  }

  void addEnumShort(short enumNumber) {
    if (_type != SHORT) {
      return;
    }
    WProperty* valueE = new WProperty("", "", _type, "");
    valueE->setShort(enumNumber);
    this->addEnum(valueE);
  }

  void addEnumUnsignedLong(unsigned long enumNumber) {
    if (_type != UNSIGNED_LONG) {
      return;
    }
    WProperty* valueE = new WProperty("", "", _type, "");
    valueE->setUnsignedLong(enumNumber);
    this->addEnum(valueE);
  }

  void addEnumByte(byte enumByte) {
    if (_type != BYTE) {
      return;
    }
    WProperty* valueE = new WProperty("", "", _type, "");
    valueE->setByte(enumByte);
    this->addEnum(valueE);
  }

  void addEnumString(const char* enumString) {
    if (_type != STRING) {
      return;
    }
    WProperty* valueE = new WProperty("", "", _type, "");
    valueE->setString(enumString);
    this->addEnum(valueE);
  }

  byte getEnumIndex() { return getEnumIndex(this, this->getValue().string); }

  static byte getEnumIndex(WProperty* property, const char* enumString) {
    if ((property->hasEnums()) && (enumString != nullptr) && (property->type() == STRING)) {
      WProperty* en = property->_enums->getIf([property, enumString](WProperty* en) {
        return (strcmp(en->getValue().string, enumString) == 0);
      });
      return (en != nullptr ? property->_enums->indexOf(en) : 0xFF);
    } else {
      return 0xFF;    
    }  
    
    if ((property->type() != STRING) || (!property->hasEnums())) {
      return 0xFF;    
    }
  }

  const char* getEnumString(byte enumIndex) {
    return getEnumString(this, enumIndex);
  }

  static const char* getEnumString(WProperty* property, byte enumIndex) {
    if ((property->hasEnums()) && (property->type() == STRING)) {
      WProperty* en = property->_enums->get(enumIndex);
      return (en != nullptr ? en->getValue().string : nullptr);
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

  void addOutput(WOutput* output) {
    if (_outputs == nullptr) {
      _outputs = new WList<WOutput>();
    }
    _outputs->add(output);
  }

  bool hasOutputs() { return (_outputs != nullptr); }

  WList<WOutput>* outputs() {
    return _outputs;
  }

  WPropertyVisibility visibility() { return _visibility; }

  void setVisibility(WPropertyVisibility visibility) {
    _visibility = visibility;
  }

  void setVisibility(bool mqtt, bool webthing) {
    if ((mqtt) && (webthing)) {
      setVisibility(ALL);
    } else if ((!mqtt) && (!webthing)) {
      setVisibility(NONE);
    } else if (mqtt) {
      setVisibility(MQTT);
    } else {
      setVisibility(WEBTHING);
    } 
  }  

  void setVisibilityMqtt(bool value) {
    if ((value) && (_visibility != MQTT) && (_visibility != ALL)) {
      setVisibility(_visibility == WEBTHING ? ALL : MQTT);
    } else if ((!value) && (_visibility != NONE) && (_visibility != WEBTHING)) {
      setVisibility(_visibility == ALL ? WEBTHING : NONE);
    }
  }

  void setVisibilityWebthing(bool value) {
    if ((value) && (_visibility != WEBTHING) && (_visibility != ALL)) {
      setVisibility(_visibility == MQTT ? ALL : WEBTHING);
    } else if ((!value) && (_visibility != NONE) && (_visibility != MQTT)) {
      setVisibility(_visibility == ALL ? MQTT : NONE);
    }
  }

  bool isVisible(WPropertyVisibility visibility) {
    return ((_visibility == ALL) || (_visibility == visibility));
  }

  void setId(const char* id) {
    delete _id;
    _id = new char[strlen(id) + 1];
    strcpy(_id, id);
  }

 protected:
  const char* _atType;

  void initialize(const char* id, const char* title, WPropertyType type,
                  const char* atType) {
    _id = new char[strlen(id) + 1];
    strcpy(_id, id);
    _title = new char[strlen(title) + 1];
    strcpy(_title, title);
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
    _unit = "";
    _multipleOf = 0.0;
    _onChange = nullptr;
    _deviceNotification = nullptr;
    _enums = nullptr;
    _outputs = nullptr;
  }

  void setValue(WValue newValue) {
    _value = newValue;
    _valueNull = false;
    _changed = true;
    valueChanged();
    notify();
  }

  virtual void valueChanged() {}

  virtual void toJsonStructureAdditionalParameters(WJson* json) {}

 private:
  char* _id;
  char* _title;
  WPropertyType _type;
  WPropertyVisibility _visibility;
  bool _supportingMqtt;
  bool _supportingWebthing;
  bool _readOnly;
  const char* _unit;
  double _multipleOf;
  TOnPropertyChange _onChange;
  TOnPropertyChange _onValueRequest;
  TOnPropertyChange _deviceNotification;
  WValue _value = {false};
  bool _valueNull;
  bool _changed;
  bool _requested;
  bool _valueRequesting;
  bool _notifying;

  WList<WOutput>* _outputs;  
  WList<WProperty>* _enums;

  void notify() {
    if (!_valueRequesting) {
      _notifying = true;
      if (_onChange) {
        //Custom change handling
        _onChange(this);
      } else if (_outputs != nullptr) {
        //Let the output handle the change
        _outputs->forEach([this](WOutput* output){output->handleChangedProperty(_value);});
	    }
      if (_deviceNotification) {
        _deviceNotification(this);
      }
      _notifying = false;
    }
  }

  void requestValue() {
    if ((!_notifying) && (_onValueRequest)) {
      _valueRequesting = true;
      _onValueRequest(this);
      _valueRequesting = false;
    }
  }
};

class WRangeProperty: public WProperty {
public:
	WRangeProperty(const char* id, const char* title, WPropertyType type, WValue minimum, WValue maximum, const char* atType = TYPE_LEVEL_PROPERTY)
	: WProperty(id, title, type, atType) {    
		_min = minimum;
		_max = maximum;
	}

	double getMinAsDouble() {
		return _min.asDouble;
	}

  int getMinAsInteger() {
		return _min.asInteger;
	}

	double getMaxAsDouble() {
		return _max.asDouble;
	}

  int getMaxAsInteger() {
		return _max.asInteger;
	}

  byte getScaledToMax0xFF() {
		int v = 0;
    switch (this->type()) {
      case DOUBLE: {
        v = (int) round(getDouble() * 0xFF / getMaxAsDouble());
        break;
      }
      case INTEGER: {
        v = getInteger() * 0xFF / getMaxAsInteger();
        break;
      }
    }  
		return (byte) v;
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
  WColorProperty(const char* id, const char* title, byte red, byte green, byte blue)
      : WProperty(id, title, STRING, TYPE_COLOR_PROPERTY) {
    _red = red;
    _green = green;
    _blue = blue;      
    setRGBString();
    //this->setRGB(red, green, blue);
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
    setString(result.c_str());
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
    if ((!isReadOnly()) && (value != nullptr)) {
      if ((value.startsWith("#")) && (value.length() == 7)) {
        setString(value.c_str());
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
