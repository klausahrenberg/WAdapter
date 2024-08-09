#ifndef W_RELAY_H
#define W_RELAY_H

#include "WOutput.h"

class WRelay : public WOutput {
 public:
  WRelay(int relayPin, bool inverted) : WOutput(relayPin) {
    _inverted = inverted;
    if (this->isInitialized()) {
      digitalWrite(this->pin(), getOffLevel());
    }
  }  

  void loop(unsigned long now) {}

  bool inverted() { return _inverted; }

  WRelay* inverted(bool inverted) { 
    _inverted = inverted; 
    return this;
  }

  virtual void loadFromStore() {
    WOutput::loadFromStore();
    WValue* config = SETTINGS->setByte(nullptr, NO_PIN);
    // Inverted
    inverted(bitRead(config->asByte(), BIT_CONFIG_INVERTED));    
  }

 protected:
  void _updateOn() {
    digitalWrite(this->pin(), isOn() ? getOnLevel() : getOffLevel());
  };

  byte getOnLevel() { return (!_inverted ? HIGH : LOW); }

  byte getOffLevel() { return !getOnLevel(); }

 private:
  bool _inverted;
};

#endif
