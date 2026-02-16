#ifndef W_INPUT_H_
#define W_INPUT_H_

#include "WGpio.h"

const unsigned long INPUT_SENSITIVENESS = 20;

class WInput : public WGpio {
 public:
  typedef std::function<void(bool state)> TStateChangeFunction;
  WInput(int inputPin = NO_PIN, bool inverted = false, IWExpander* expander = nullptr)
      : WGpio(GPIO_TYPE_STATE, inputPin, (inverted ? INPUT_PULLUP : INPUT), expander) {
    this->inverted(inverted);
    if (this->isInitialized()) {
      _state = readInput(inputPin);
      _lastState = _state;
    }
  }

  static WInput* create(IWGpioRegister* device, int inputPin = NO_PIN, bool inverted = false, IWExpander* expander = nullptr) {
    WInput* i = new WInput(inputPin, inverted, expander);
    device->registerGpio(i);
    return i;
  }

  void loop(unsigned long now) {
    if (this->isInitialized()) {
      // 1. Eliminate flickering input
      bool newState = readInput(pin());
      if (newState != _state) {
        if (_startTime == 0) {
          _startTime = now;
        } else if ((_startTime > 0) && (now - _startTime >= (_stateOf(newState) ? _highDelay : _lowDelay))) {
          _state = newState;
          _startTime = 0;
          handleStateChange();
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

  void handleStateChange() {
    bool s = state();
    if (property() != nullptr) {
      property()->readOnly(false);
      property()->asBool(s);
      property()->readOnly(true);
    }
    if (_onStateChange) {
      _onStateChange(s);
    }
  }

  bool state() {
    return _stateOf(_state);
  }

  void setOnStateChange(TStateChangeFunction onStateChange) {
    _onStateChange = onStateChange;
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

  u_int16_t highDelay() { return _highDelay; };

  WInput* highDelay(u_int16_t highDelay) {
    _highDelay = highDelay;
    return this;
  };

  u_int16_t lowDelay() { return _lowDelay; };

  WInput* lowDelay(u_int16_t lowDelay) {
    _lowDelay = lowDelay;
    return this;
  };

 protected:
  WValue* _config = new WValue((byte)0b00000000);

  byte _onLevel() { return (!inverted() ? HIGH : LOW); }

 private:
  TStateChangeFunction _onStateChange;
  bool _state, _lastState;
  unsigned long _startTime = 0;
  u_int16_t _highDelay = 20;
  u_int16_t _lowDelay = 20;

  bool _stateOf(bool state) {
    return (inverted() ? !state : state);
  }
};

#endif
