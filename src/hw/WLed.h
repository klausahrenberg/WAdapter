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

  virtual ~WLed() {
    if (_config) delete _config;
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

  bool inverted() { return bitRead(_config->asByte(), BIT_CONFIG_INVERTED); }

  WLed* inverted(bool inverted) { 
    byte b = _config->asByte();
    bitWrite(b, BIT_CONFIG_INVERTED, inverted);
    _config->asByte(b);
    return this;
  }

  bool linkState() { return bitRead(_config->asByte(), BIT_CONFIG_LINKSTATE); }

  virtual void loadFromStore() {
    WOutput::loadFromStore();
    SETTINGS->add(_config, nullptr);    
  }

 protected:
  byte getOnLevel() { return (!inverted() ? LED_ON : LED_OFF); }

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
  WValue* _config = new WValue((byte) 0b00000000);
  bool _blinkOn = false;
  //bool _inverted = false;
  //bool _linkState = false;
  unsigned long _blinkMillis = 0;
  unsigned long _lastBlinkOn = 0;
};

#endif
