#ifndef W_PCF8575_h
#define W_PCF8575_h

#include "../WI2C.h"

#define READ_ELAPSED_TIME 10

class WPCF8575 : public WI2C, public IWExpander {
 public:
  WPCF8575(byte address, int sda = 21, int scl = 22, TwoWire* i2cPort = &Wire)
      : WI2C(GPIO_TYPE_PCF8575, address, sda, scl, NO_PIN, i2cPort) {}

  static WPCF8575* create(IWGpioRegister* device, byte address, int sda = 21, int scl = 22, TwoWire* i2cPort = &Wire) {
    WPCF8575* exp = new WPCF8575(address, sda, scl, i2cPort);
    device->registerGpio(exp);
    return exp;
  }

  virtual void loop(unsigned long now) {
    WI2C::loop(now);
    if (_started && (now - _lastReadMillis >= READ_ELAPSED_TIME)) {
      _i2cPort->requestFrom(_address, (uint8_t)2);
      _lastReadMillis = millis();
      if (_i2cPort->available()) {        
        uint16_t currentInputs = _i2cPort->read();
        currentInputs |= _i2cPort->read() << 8;
        // Input-Pins behalten ihren aktuellen Zustand
        //alt: _byteBuffered = (uint16_t) currentInputs;
        _byteBuffered = (_writeByteBuffered & _writeMode) | (currentInputs & _readMode);
      }
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
    } else {
      LOG->debug(F("IO expander, missing in-/outputs"));
    }
    _lastReadMillis = millis();
    _started = this->isLastTransmissionSuccess();
    return _started;
  }

  bool isLastTransmissionSuccess() {
    return (_transmissionStatus == 0);
  }

  virtual void mode(uint8_t pin, uint8_t mode) {
    // bitRead(_resetInitial, 6);
    if (mode == OUTPUT) {
      _writeMode = _writeMode | bit(pin);
      _writeModeUp = _writeModeUp | bit(pin);
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
    return (_started ? bitRead(_byteBuffered, pin) : false);
  }

  /*
  alt:
  virtual void writeOutput(uint8_t pin, bool value) {
    Serial.println("writeOutput: ");
    if (_started) {
      _i2cPort->beginTransmission(_address);
      if (value == HIGH) {
        _writeByteBuffered = _writeByteBuffered | bit(pin);
        _byteBuffered = _writeByteBuffered | bit(pin);
      } else {
        _writeByteBuffered = _writeByteBuffered & ~bit(pin);
        _byteBuffered = _writeByteBuffered & ~bit(pin);
      }
      _byteBuffered = (_writeByteBuffered & _writeMode) | (_resetInitial & _readMode);
      _i2cPort->write((uint8_t)_byteBuffered);
      _i2cPort->write((uint8_t)(_byteBuffered >> 8));
      _byteBuffered =
          (_writeByteBuffered & _writeMode) | (_initialBuffer & _readMode);
      _transmissionStatus = _i2cPort->endTransmission();
      Serial.print("Write: ");
      Serial.print(_byteBuffered, BIN);
      Serial.println();
    }
  }
  */

  virtual void writeOutput(uint8_t pin, bool value) {
    if (_started) {
      // 1. Aktuelle Input-Werte lesen (optional, aber empfohlen)
      _i2cPort->requestFrom(_address, (uint8_t)2);
      if (_i2cPort->available() >= 2) {
        uint16_t currentInputs = _i2cPort->read();
        currentInputs |= _i2cPort->read() << 8;
        // Input-Pins behalten ihren aktuellen Zustand
        _byteBuffered = (_writeByteBuffered & _writeMode) | (currentInputs & _readMode);
      }

      // 2. Output-Pin setzen/clearen
      if (value == HIGH) {
        _writeByteBuffered |= bit(pin);
      } else {
        _writeByteBuffered &= ~bit(pin);
      }

      // 3. Neuen Gesamtzustand berechnen
      // Outputs: _writeByteBuffered, Inputs: aktuelle Werte (oder initial bei Fehler)
      uint16_t newState = (_writeByteBuffered & _writeMode) | (_byteBuffered & _readMode);

      // 4. An PCF8575 senden
      _i2cPort->beginTransmission(_address);
      _i2cPort->write((uint8_t)newState);
      _i2cPort->write((uint8_t)(newState >> 8));
      _transmissionStatus = _i2cPort->endTransmission();

      // 5. Puffern für spätere reads
      _byteBuffered = newState;

      Serial.print("Write pin ");
      Serial.print(pin);
      Serial.print(" = ");
      Serial.print(value);
      Serial.print(" State: 0b");
      Serial.println(newState, BIN);
    }
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
  bool _started = false;
};

#endif