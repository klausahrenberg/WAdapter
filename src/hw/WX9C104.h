#ifndef W_X9C104_H
#define W_X9C104_H

#include "WNetwork.h"

#define X9C10X_UP HIGH
#define X9C10X_DOWN LOW
#define X9C10X_DELAY_MICROS 3
#define X9C10X_MAXPOT 99

class WMCP444x : public WGpio {
 public:
  WMCP444x(int8_t cs, int8_t inc, int8_t u_d) : WGpio(GPIO_TYPE_X9C104, cs, OUTPUT, nullptr) {
    _inc = inc;
    _u_d = u_d;
    writeOutput(_inc, HIGH);
    writeOutput(_u_d, HIGH);
    mode(_inc, OUTPUT);
    mode(_u_d, OUTPUT);
    delayMicroseconds(500);
  }

  uint8_t resistance() { return _value; }

  void resistance(uint8_t value) {
    if (value > X9C10X_MAXPOT) {
      value = X9C10X_MAXPOT;
    }

    //  force to nearest end position first to minimize number of steps.
    if (value > _value) {
      _move(X9C10X_UP, value - _value);
    } else if (value < _value) {
      _move(X9C10X_DOWN, _value - value);
    }
    _value = value;
  }

 protected:

  void _move(uint8_t direction, uint8_t steps) {
    writeOutput(_u_d, direction);
    delayMicroseconds(3);  // Tdi  (page 5)

    //  _pulsePin starts default HIGH
    writeOutput(pin(), LOW);
    while (steps--) {
      writeOutput(_inc, HIGH);
      #if X9C10X_DELAY_MICROS > 0
        delayMicroseconds(X9C10X_DELAY_MICROS);
      #endif

      writeOutput(_inc, LOW);
      #if X9C10X_DELAY_MICROS > 0
        delayMicroseconds(X9C10X_DELAY_MICROS);
      #endif
    }
    //  _pulsePin == LOW, (No Store, page 7)
    writeOutput(pin(), HIGH);
    // reset _pulsePin to default.
    writeOutput(_inc, HIGH);
  }

 private:
  uint8_t _value = 0;
  byte _inc, _u_d;
};

#endif