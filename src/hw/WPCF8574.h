#ifndef W_PCF8574_H
#define W_PCF8574_H

#include "Wire.h"

#define P0 0
#define P1 1
#define P2 2
#define P3 3
#define P4 4
#define P5 5
#define P6 6
#define P7 7

class WPCF8574 {
 public:
  typedef std::function<void(byte pin, bool isRising)> THandlerFunction;
  WPCF8574(byte address, byte intPin) {
    this->i2cAddress = i2cAddress;
    this->intPin = intPin;
    lastUpdate = 0;
    lastRead = 0;
    pinMode(intPin, INPUT);
    Wire.begin();
    expanderSetInput(0xFF);
  }

  void loop() {
    long now = millis();
    if ((lastUpdate == 0) || (now - lastUpdate > 50)) {
      lastUpdate = now;

      byte newState = expanderRead();
      if (newState != lastRead) {
        byte tempState = lastRead;
        lastRead = newState;
        // notify every pin that goes high
        for (int i = 0; i < 8; i++) {
          bool newBit = readABit(lastRead, i);
          if (newBit != readABit(tempState, i)) {
            notify(i, newBit);
          }
        }
      }
    }
  }

  void onNotify(THandlerFunction fn) { _callback = fn; }
  bool digitalRead(byte inputNo) { return readABit(lastRead, inputNo); }

 private:
  byte i2cAddress, intPin, lastRead;
  THandlerFunction _callback;
  unsigned long lastUpdate;

  void notify(byte pin, bool isRising) {
    if (_callback) {
      _callback(pin, isRising);
    }
  }

  void expanderSetInput(byte dir) {
    Wire.beginTransmission(i2cAddress);
    Wire.write(dir);  // outputs high for input
    Wire.endTransmission();
  }

  byte expanderRead() {
    int _data = -1;
    Wire.requestFrom(i2cAddress, 1);
    if (Wire.available()) {
      _data = Wire.read();
    }
    return _data;
  }

  void expanderWrite(byte data) {
    Wire.beginTransmission(i2cAddress);
    Wire.write(data);
    Wire.endTransmission();
  }

  bool readABit(byte data, byte bit) {
    byte mask = 0b00000001;
    mask <<= bit;
    return ((data & mask) != 0x00);
  }
};

#endif
