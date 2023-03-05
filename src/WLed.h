#ifndef W_LED_H
#define W_LED_H

#include "WOutput.h"

#ifdef ESP8266
const byte LED_ON = LOW;
const byte LED_OFF = HIGH;
#elif ESP32
const byte LED_ON = HIGH;
const byte LED_OFF = LOW;
#endif

class WLed : public WOutput {
 public:
  WLed(int ledPin) : WOutput(ledPin) {
    _blinkMillis = 0;    
    _inverted = false;
    if (this->isInitialized()) {
      digitalWrite(this->pin(), getOffLevel());
    }
  }  

  void onChanged() {
    _blinkOn = false;
    _lastBlinkOn = 0;
  };

  void setOn(bool ledOn, int blinkMillis) {
    if ((this->isOn()) && (_blinkMillis != blinkMillis)) {
      WOutput::setOn(false);
    }
    _blinkMillis = blinkMillis;
    WOutput::setOn(ledOn);
  }

  bool isBlinking() { return (_blinkMillis > 0); }

  void loop(unsigned long now) {
    if (isOn()) {
      if (isBlinking()) {
        if ((_lastBlinkOn == 0) || (now - _lastBlinkOn > _blinkMillis)) {
          _blinkOn = !_blinkOn;
          _lastBlinkOn = now;
          digitalWrite(this->pin(), _blinkOn ? getOnLevel() : getOffLevel());
        }
      } else {
        digitalWrite(this->pin(), getOnLevel());
      }
    } else {
      // switchoff
      digitalWrite(this->pin(), getOffLevel());
    }
  }

  bool inverted() { return _inverted; }

  void setInverted(bool inverted) { _inverted = inverted; }

 protected:
  byte getOnLevel() { return (!_inverted ? LED_ON : LED_OFF); }

  byte getOffLevel() { return !getOnLevel(); }

 private:
  bool _blinkOn, _inverted;
  unsigned long _blinkMillis, _lastBlinkOn;
};

#endif
