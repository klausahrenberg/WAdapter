#ifndef W_VALUE_H
#define W_VALUE_H

#include <Arduino.h>

union WValue {
  bool asBoolean;
  double asDouble;
  short asShort;
  int asInteger;
  unsigned long asUnsignedLong;
  byte asByte;
  char* string;
  byte* asByteArray;
};

#endif