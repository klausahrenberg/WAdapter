#ifndef W_INPUT_H
#define W_INPUT_H

#include "WOutput.h"

class WInput {
 public:
  typedef std::function<void()> THandlerFunction;
  WInput(int pin, byte mode) {
    _pin = pin;
    if ((_pin != NO_PIN) && ((mode == INPUT) || (mode == INPUT_PULLUP))) {
      pinMode(_pin, mode);
    }
    this->property = nullptr;
  }

  WProperty* getProperty() { return property; }

  void setProperty(WProperty* property) {
    if (this->property != property) {
      this->property = property;
      this->loop(millis());
    }
  }

  bool hasProperty() { return (this->property != nullptr); }

  virtual void loop(unsigned long now) {}

 protected:
  virtual bool isInitialized() { return (_pin != NO_PIN); }

  int pin() { return _pin; }

 private:
  int _pin;
  WProperty* property;
};

#endif