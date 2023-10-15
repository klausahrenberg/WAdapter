#ifndef W_PROPS_H
#define W_PROPS_H

#include <Arduino.h>
#include "WProperty.h"

class WProps {
 public:
  static WProperty* createIntegerProperty(const char* id, const char* title) {
    return new WProperty(id, title, INTEGER, "");
  }

  static WProperty* createStringProperty(const char* id, const char* title) {
    return new WProperty(id, title, STRING, "");
  }

  static WProperty* createByteProperty(const char* id, const char* title) {
    return new WProperty(id, title, BYTE, "");
  }

  static WProperty* createDoubleProperty(const char* id, const char* title) {
    return new WProperty(id, title, DOUBLE, "");
  }

  static WProperty* createUnsignedLongProperty(const char* id,
                                               const char* title) {
    return new WProperty(id, title, UNSIGNED_LONG, "");
  }

  static WProperty* createShortProperty(const char* id, const char* title) {
    return new WProperty(id, title, SHORT, "");
  }

  static WProperty* createBooleanProperty(const char* id, const char* title) {
    return new WProperty(id, title, BOOLEAN, "");
  }

  static WRangeProperty* createTargetTemperatureProperty(const char* id, const char* title) {
    WRangeProperty* p = new WRangeProperty(id, title, DOUBLE, WValue::ofDouble(15.0), WValue::ofDouble(25.0), TYPE_TARGET_TEMPERATURE_PROPERTY);
    p->multipleOf(0.5);
    p->unit(UNIT_CELSIUS);
    return p;
  }  

  static WProperty* createTemperatureProperty(const char* id,
                                              const char* title) {
    WProperty* p = new WProperty(id, title, DOUBLE, TYPE_TEMPERATURE_PROPERTY);
    p->readOnly(true);
    p->unit(UNIT_CELSIUS);
    return p;
  }

  static WRangeProperty* createHumidityProperty(const char* id, const char* title) {
    WRangeProperty* p = new WRangeProperty(id, title, DOUBLE, WValue::ofDouble(0.0), WValue::ofDouble(100.0), TYPE_HUMIDITY_PROPERTY);
    p->readOnly(true);
    p->unit(UNIT_PERCENT);
    return p;
  }

  static WRangeProperty* createLevelProperty(const char* id, const char* title, double min, double max) {
    WRangeProperty* p = new WRangeProperty(id, title, DOUBLE, WValue::ofDouble(min), WValue::ofDouble(max), TYPE_LEVEL_PROPERTY);    
    return p;
  }

  static WRangeProperty* createLevelIntProperty(const char* id, const char* title, int min, int max) {
    WRangeProperty* p = new WRangeProperty(id, title, INTEGER, WValue::ofInt(min), WValue::ofInt(max), TYPE_LEVEL_PROPERTY);    
    return p;
  }

  static WRangeProperty* createBrightnessProperty(const char* id, const char* title, byte min = 0, byte max = 100) {
    WRangeProperty* p = new WRangeProperty(id, title, INTEGER, WValue::ofInt(min), WValue::ofInt(max), TYPE_BRIGHTNESS_PROPERTY);    
    return p;
  }

  static WColorProperty* createColorProperty(const char* id, const char* title, byte red, byte green, byte blue) {
    WColorProperty* cp = new WColorProperty(id, title, red, green, blue);
    return cp;
  }

  static WProperty* createOnOffProperty(const char* id, const char* title) {
    return new WProperty(id, title, BOOLEAN, TYPE_ON_OFF_PROPERTY);
  }

  static WProperty* createPushedProperty(const char* id, const char* title) {
    return new WProperty(id, title, BOOLEAN, TYPE_PUSHED_PROPERTY);
  }

  static WProperty* createHeatingCoolingProperty(const char* id, const char* title) {
    WProperty* p = new WProperty(id, title, STRING, TYPE_HEATING_COOLING_PROPERTY);
    p->readOnly(true);
		p->addEnumString(VALUE_OFF);
		p->addEnumString(VALUE_HEATING);
		p->addEnumString(VALUE_COOLING);
    return p;
  }

};

#endif
