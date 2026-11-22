#ifndef W_I2C_H
#define W_I2C_H

#include <Wire.h>

#include "WGpio.h"

const static char WC_SDA[] PROGMEM = "sda";
const static char WC_SCL[] PROGMEM = "scl";

class WI2C : public WGpio {
 public:
  WI2C(WGpioType gpioType, byte address, int sda, int scl, int interrupt, TwoWire* i2cPort = &Wire)
      : WGpio(gpioType, interrupt, INPUT_PULLUP) {
    _i2cPort = i2cPort;
    _address = address;
    _sdaPin->asByte(sda < 0 ? NO_PIN : sda);
    _sclPin->asByte(scl < 0 ? NO_PIN : scl);
    _beginPort();
  }

  virtual ~WI2C() {
    if (_sdaPin) delete _sdaPin;
    if (_sclPin) delete _sclPin;
  }

  virtual void registerSettings() {
    WGpio::registerSettings();
    SETTINGS->add(_sdaPin, nullptr);
    SETTINGS->add(_sclPin, nullptr);
    _beginPort();
  }

  virtual void fromJson(WList<WValue>* list) {
    WGpio::fromJson(list);
    list->ifExistsId(WC_SDA, [this](WValue* v) { _sdaPin->asByte(v->asByte()); });
    list->ifExistsId(WC_SCL, [this](WValue* v) { _sclPin->asByte(v->asByte()); });
    _beginPort();
  }

  virtual void toJson(WJson* json) {
    WGpio::toJson(json);
    json->propertyValue(WC_SDA, _sdaPin);
    json->propertyValue(WC_SCL, _sclPin);
  }

  byte address() {
    return _address;
  }

  virtual void loop(unsigned long now) {
    WGpio::loop(now);
  }

 protected:
  TwoWire* _i2cPort;
  byte _address;
  WValue* _sdaPin = new WValue((byte)NO_PIN);
  WValue* _sclPin = new WValue((byte)NO_PIN);

  bool isInitialized() {
    return ((_sdaPin->asByte() != NO_PIN) && (_sclPin->asByte() != NO_PIN));
  }

  void _beginPort() {
    if (isInitialized()) {
      _i2cPort->begin(_sdaPin->asByte(), _sclPin->asByte());
    }
  }

 private:
};

#endif
