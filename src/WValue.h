#ifndef W_VALUE_H
#define W_VALUE_H

#include <Arduino.h>

union WValue { 
  bool asBool;
  double asDouble;
  short asShort;
  int asInt;
  unsigned long asUnsignedLong;
  byte asByte;
  char* string;
  byte* asByteArray;

  static WValue ofDouble(double d) {
    WValue r;
    r.asDouble = d;
    return r;
  }

  static WValue ofInt(int d) {
    WValue r;
    r.asInt = d;
    return r;
  }

};

#endif