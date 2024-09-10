#ifndef W_INPUT_H
#define W_INPUT_H

#include "WGpio.h"

class WInput {
 public:
  typedef std::function<void()> THandlerFunction;
  WInput(int pin, byte mode = INPUT, IWExpander* expander = nullptr) {
    _pin = pin;
    _expander = expander;
    if ((_pin != NO_PIN) && ((mode == INPUT) || (mode == INPUT_PULLUP))) {
      _pinMode(_pin, mode);
    }    
    _property = nullptr;
  }

  WProperty* property() { return _property; }

  void setProperty(WProperty* property) {
    if (_property != property) {
      _property = property;
      this->loop(millis());
    }
  }

  bool hasProperty() { return (_property != nullptr); }

  virtual void loop(unsigned long now) {}

  bool readInput(uint8_t pin) {
    return (_expander == nullptr ? digitalRead(pin) : _expander->readInput(pin));
  }    

 protected:
  virtual bool isInitialized() { return (_pin != NO_PIN); }

  int pin() { return _pin; }

  void _pinMode(uint8_t pin, uint8_t mode) {
    (_expander == nullptr ? pinMode(pin, mode) : _expander->mode(pin, mode));
  }

 private:
  int _pin;
  WProperty* _property;
  IWExpander* _expander;
};

#endif