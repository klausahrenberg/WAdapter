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

  //void setInverted(bool inverted) { this->inverted = inverted; }

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
