#ifndef W_INPUT_H
#define W_INPUT_H

#include "WOutput.h"

class WInput {
 public:
  WInput(int switchPin, byte mode) {
    this->pin = pin;
    if ((this->pin != NO_PIN) && ((mode == INPUT) || (mode == INPUT_PULLUP))) {
      pinMode(this->pin, mode);
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
  virtual bool isInitialized() { return (pin != NO_PIN); }

  int getPin() { return pin; }

 private:
  int pin;
  WProperty* property;
};

#endif