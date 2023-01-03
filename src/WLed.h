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
    this->blinkMillis = 0;    
    _inverted = false;
    if (this->isInitialized()) {
      digitalWrite(this->pin(), getOffLevel());
    }
  }  

  void onChanged() {
    this->blinkOn = false;
    this->lastBlinkOn = 0;
  };

  void setOn(bool ledOn, int blinkMillis) {
    if ((this->isOn()) && (this->blinkMillis != blinkMillis)) {
      WOutput::setOn(false);
    }
    this->blinkMillis = blinkMillis;
    WOutput::setOn(ledOn);
  }

  bool isBlinking() { return (this->blinkMillis > 0); }

  void loop(unsigned long now) {
    if (isOn()) {
      if (isBlinking()) {
        if ((lastBlinkOn == 0) || (now - lastBlinkOn > this->blinkMillis)) {
          blinkOn = !blinkOn;
          lastBlinkOn = now;
          digitalWrite(this->pin(), blinkOn ? getOnLevel() : getOffLevel());
        }
      } else {
        digitalWrite(this->pin(), getOnLevel());
      }
    } else {
      // switchoff
      digitalWrite(this->pin(), getOffLevel());
    }
    /*if ((this->isInitialized()) && (getProperty() != nullptr)) {
            digitalWrite(this->getPin(), getProperty()->getBoolean() ? LED_ON :
    LED_OFF);
    }*/
  }

  bool inverted() { return _inverted; }

  void setInverted(bool inverted) { _inverted = inverted; }

 protected:
  byte getOnLevel() { return (!_inverted ? LED_ON : LED_OFF); }

  byte getOffLevel() { return !getOnLevel(); }

 private:
  bool blinkOn, _inverted;
  unsigned long blinkMillis, lastBlinkOn;
};

#endif
