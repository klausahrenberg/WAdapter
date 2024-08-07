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

class WLed : public WOutput, public IWStorable {
 public:
  WLed(int ledPin, IWExpander* expander = nullptr) : WOutput(ledPin, OUTPUT, expander) {
    _blinkMillis = 0;    
    _inverted = false;
    if (this->isInitialized()) {
      writeOutput(ledPin, getOffLevel());
    }
    _blinkOn = false;
    _lastBlinkOn = 0;
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
          writeOutput(pin(), _blinkOn ? getOnLevel() : getOffLevel());          
        }
      } else {
        writeOutput(pin(), getOnLevel());
      }
    } else {
      // switchoff
      writeOutput(pin(), getOffLevel());
    }
  }

  bool inverted() { return _inverted; }

  void setInverted(bool inverted) { _inverted = inverted; }

 protected:
  byte getOnLevel() { return (!_inverted ? LED_ON : LED_OFF); }

  byte getOffLevel() { return !getOnLevel(); }

  virtual void _updateOn() {
    WOutput::_updateOn();

	}

 private:
  bool _blinkOn, _inverted;
  unsigned long _blinkMillis, _lastBlinkOn;
};

#endif
