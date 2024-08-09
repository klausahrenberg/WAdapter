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
  WLed(int ledPin, IWExpander* expander = nullptr) : WOutput(ledPin, OUTPUT, expander) {    
    
  }  

  void onChanged() {
    _blinkOn = false;
    _lastBlinkOn = 0;
  };

  void setOn(bool ledOn, int blinkMillis = 0) {
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

  WLed* inverted(bool inverted) { 
    _inverted = inverted; 
    return this;
  }

  bool linkState() { return _linkState; }

  virtual void loadFromStore() {
    WOutput::loadFromStore();
    WValue* config = SETTINGS->setByte(nullptr, NO_PIN);
    // Inverted
    inverted(bitRead(config->asByte(), BIT_CONFIG_INVERTED));
    // Linkstate
    _linkState = bitRead(config->asByte(), BIT_CONFIG_LINKSTATE);    
  }

 protected:
  byte getOnLevel() { return (!_inverted ? LED_ON : LED_OFF); }

  byte getOffLevel() { return !getOnLevel(); }

  virtual void _updateOn() {
    WOutput::_updateOn();

	}

  virtual void _pinChanged() {    
    if (isInitialized()) {
      writeOutput(pin(), getOffLevel());
    }
  } 

 private:
  bool _blinkOn = false;
  bool _inverted = false;
  bool _linkState = false;
  unsigned long _blinkMillis = 0;
  unsigned long _lastBlinkOn = 0;
};

#endif
