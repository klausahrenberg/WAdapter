#ifndef W_PCF8575_h
#define W_PCF8575_h

#include "../WI2C.h"

#define READ_ELAPSED_TIME 10

class WPCF8575 : public WI2C, public IWExpander {
 public:
  WPCF8575(byte address, int sda = 21, int scl = 22, TwoWire* i2cPort = &Wire)
      : WI2C(GPIO_TYPE_PCF8575, address, sda, scl, NO_PIN, i2cPort) {}

  virtual void loop(unsigned long now) {
    WI2C::loop(now);    
    _i2cPort->requestFrom(_address, (uint8_t)2);
    _lastReadMillis = millis();
    if (_i2cPort->available()) {
      
      uint16_t iInput = _i2cPort->read();
      iInput |= _i2cPort->read() << 8;
      //if ((_readModePullDown & iInput) > 0 or (_readModePullUp & ~iInput) > 0) {
        _byteBuffered = /*(_byteBuffered & ~_readMode) |*/ (uint16_t)iInput;
        
      //}
    }
  }

  bool begin() {
    _transmissionStatus = 4;
    if (_writeMode > 0 || _readMode > 0) {
      _i2cPort->beginTransmission(_address);
      _resetInitial = _writeModeUp | _readMode;
      _i2cPort->write((uint8_t)_resetInitial);
      _i2cPort->write((uint8_t)(_resetInitial >> 8));
      _initialBuffer = _writeModeUp | _readModePullUp;
      _byteBuffered = _initialBuffer;
      _writeByteBuffered = _writeModeUp;
      _transmissionStatus = _i2cPort->endTransmission();
    }
    // PCF8575::attachInterrupt();
    //  inizialize last read
    _lastReadMillis = millis();
    return this->isLastTransmissionSuccess();
  }

  bool isLastTransmissionSuccess() { return (_transmissionStatus == 0); }

  virtual void mode(uint8_t pin, uint8_t mode) {
    // void pinMode(uint8_t pin, uint8_t mode, uint8_t output_start = HIGH) {
    
    if (mode == OUTPUT) {
      _writeMode = _writeMode | bit(pin);
      // if (output_start == HIGH) {
      _writeModeUp = _writeModeUp | bit(pin);
      //}
      _readMode = _readMode & ~bit(pin);
      _readModePullDown = _readModePullDown & ~bit(pin);
      _readModePullUp = _readModePullUp & ~bit(pin);
    } else if (mode == INPUT) {
      _writeMode = _writeMode & ~bit(pin);
      _readMode = _readMode | bit(pin);
      _readModePullDown = _readModePullDown | bit(pin);
      _readModePullUp = _readModePullUp & ~bit(pin);
    } else if (mode == INPUT_PULLUP) {
      _writeMode = _writeMode & ~bit(pin);
      _readMode = _readMode | bit(pin);
      _readModePullDown = _readModePullDown & ~bit(pin);
      _readModePullUp = _readModePullUp | bit(pin);
    }
  }

  virtual bool readInput(uint8_t pin) {
    /*if (pin == 14) {
      Serial.print("r: ");
      Serial.println(bitRead(_byteBuffered, pin));
    } */ 
    return bitRead(_byteBuffered, pin);

    // uint8_t read(uint8_t pin, bool forceReadNow = false) {
    /*uint8_t value = (bit(pin) & _readModePullUp) ? HIGH : LOW;

    if ((((bit(pin) & (_readModePullDown & _byteBuffered)) > 0) or
         (bit(pin) & (_readModePullUp & ~_byteBuffered)) > 0)) {
      //Serial.println("a)");
      if ((bit(pin) & _byteBuffered) > 0) {
        value = HIGH;
      } else {
        value = LOW;
      }
    } else if ( (millis() > _lastReadMillis + READ_ELAPSED_TIME)) {
      //Serial.println("b)");
      _i2cPort->requestFrom(_address, (uint8_t)2);
      _lastReadMillis = millis();
      if (_i2cPort->available()) {
        uint16_t iInput = _i2cPort->read();
        iInput |= _i2cPort->read() << 8;
        if ((_readModePullDown & iInput) > 0 or (_readModePullUp & ~iInput) > 0) {
          _byteBuffered = (_byteBuffered & ~_readMode) | (uint16_t)iInput;
          if ((bit(pin) & _byteBuffered) > 0) {
            value = HIGH;
          } else {
            value = LOW;
          }
        }
      }
      
    }
    // If HIGH set to low to read buffer only one time
    if ((bit(pin) & _readModePullDown) and value == HIGH) {
      _byteBuffered = bit(pin) ^ _byteBuffered;
    } else if ((bit(pin) & _readModePullUp) and value == LOW) {
      _byteBuffered = bit(pin) ^ _byteBuffered;
    } else if (bit(pin) & _writeByteBuffered) {
      value = HIGH;
    }
    return value;*/
  }

  virtual void writeOutput(uint8_t pin, bool value) {
    // bool digitalWrite(uint8_t pin, uint8_t value) {
    _i2cPort->beginTransmission(_address);  // Begin the transmission to PCF8574
    if (value == HIGH) {
      _writeByteBuffered = _writeByteBuffered | bit(pin);
      _byteBuffered = _writeByteBuffered | bit(pin);
    } else {
      _writeByteBuffered = _writeByteBuffered & ~bit(pin);
      _byteBuffered = _writeByteBuffered & ~bit(pin);
    }
    _byteBuffered =
        (_writeByteBuffered & _writeMode) | (_resetInitial & _readMode);
    _i2cPort->write((uint8_t)_byteBuffered);
    _i2cPort->write((uint8_t)(_byteBuffered >> 8));
    _byteBuffered =
        (_writeByteBuffered & _writeMode) | (_initialBuffer & _readMode);
    _transmissionStatus = _i2cPort->endTransmission();
    // return this->isLastTransmissionSuccess();
  }

 protected:
 private:
  uint8_t _transmissionStatus = 0;
  uint16_t _writeMode = 0;
  uint16_t _readMode = 0;
  uint16_t _resetInitial = 0;
  uint16_t _initialBuffer = 0;
  uint16_t _writeModeUp = 0;
  uint16_t _readModePullUp = 0;
  uint16_t _readModePullDown = 0;
  uint16_t _byteBuffered = 0;
  unsigned long _lastReadMillis = 0;
  uint16_t _writeByteBuffered = 0;
};

#endif