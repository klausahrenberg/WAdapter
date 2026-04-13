#ifndef W_INPUT_H_
#define W_INPUT_H_

#include "WGpio.h"

const unsigned long INPUT_SENSITIVENESS = 20;

class WInput : public WGpio {
 public:
  WInput(int inputPin = NO_PIN, bool inverted = false, IWExpander* expander = nullptr)
      : WGpio(GPIO_TYPE_STATE, inputPin, (inverted ? INPUT_PULLUP : INPUT), expander) {
    this->inverted(inverted);
    if (_isInitialized()) {
      on(readInput(inputPin) == _onLevel());
    }
  }

  static WInput* create(IWGpioRegister* device, int inputPin = NO_PIN, bool inverted = false, IWExpander* expander = nullptr) {
    WInput* i = new WInput(inputPin, inverted, expander);
    device->registerGpio(i);
    return i;
  }

  void loop(unsigned long now) {
    if (_isInitialized()) {
      // 1. Eliminate flickering input
      bool newOn = (readInput(pin()) == _onLevel());
      if (newOn != isOn()) {
        if (_startTime == 0) {
          _startTime = now;
        } else if ((_startTime > 0) && (now - _startTime >= (newOn ? _onDelay : _offDelay))) {
          _startTime = 0;
          on(newOn);
        }
      } else {
        _startTime = 0;
      }
    }
  }

  bool inverted() { return bitRead(_config->asByte(), BIT_CONFIG_INVERTED); }

  WInput* inverted(bool inverted) {
    _config->asBit(BIT_CONFIG_INVERTED, inverted);
    _onChange();
    return this;
  }

  virtual void registerSettings() {
    WGpio::registerSettings();
    SETTINGS->add(_config, nullptr);
    _onChange();
  }

  virtual void fromJson(WList<WValue>* list) {
    WGpio::fromJson(list);
    WValue* v = list->getById(WC_INVERTED);
    inverted(v != nullptr ? v->asBool() : false);
  }

  virtual void toJson(WJson* json) {
    WGpio::toJson(json);
    json->propertyBoolean(WC_INVERTED, inverted());
  }

  u_int16_t onDelay() { return _onDelay; };

  WInput* onDelay(u_int16_t onDelay) {
    _onDelay = onDelay;
    return this;
  };

  u_int16_t offDelay() { return _offDelay; };

  WInput* offDelay(u_int16_t offDelay) {
    _offDelay = offDelay;
    return this;
  };

 protected:
  WValue* _config = new WValue((byte)0b00000000);

  byte _onLevel() { return (!inverted() ? HIGH : LOW); }

  byte _offLevel() { return !_onLevel(); }

  virtual void _updateOn() {
    if (property() != nullptr) {
      property()->readOnly(false);
      property()->asBool(isOn());
      property()->readOnly(true);
    }
  }  

 private:
  unsigned long _startTime = 0;
  u_int16_t _onDelay = 20;
  u_int16_t _offDelay = 20;

};

#endif
