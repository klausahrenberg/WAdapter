#ifndef W_UTILS_H
#define W_UTILS_H

#include "WValue.h"

const static char WC_GPIO[] PROGMEM = "gpio";
const static char WC_ID[] PROGMEM = "id";
const static char WC_INVERTED[] PROGMEM = "inverted";
const static char WC_ITEMS[] PROGMEM = "items";
const static char WC_LINK_STATE[] PROGMEM = "linkstate";
const static char WC_MODE_ID[] PROGMEM = "modeId";
const static char WC_MODE_TITLE[] PROGMEM = "modeTitle";
const static char WC_TITLE[] PROGMEM = "title";
const static char WC_TYPE[] PROGMEM = "type";

const static char* APPLICATION = nullptr;
const static char* VERSION = nullptr;
static byte FLAG_SETTINGS = 0x23;
static bool DEBUG = true;

class WUtils {
 public:
  static uint32_t getChipId() {
#ifdef ARDUINO_ARCH_ESP8266
    uint32_t ci = ESP.getChipId();
#elif ARDUINO_ARCH_ESP32
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    uint32_t ci = (macAddressTrunc >> 40);
#endif
    /*char* textToWrite =  new char[16];
    sprintf(textToWrite,"%lu", ci);
    return textToWrite;*/
    return ci;
  }
  
};

class WJson;

class IWJsonable {
 public:
  virtual ~IWJsonable() {
  }

  virtual void registerSettings();
  virtual void fromJson(WList<WValue>* list);
  virtual void toJson(WJson* json);
};

class UUID {
 public:
  static WValue randomUUID() {
    char _buffer[37];
    randomSeed(millis());
    uint32_t ar[4];
    for (int i = 0; i < 4; i++) {
      ar[i] = random();
    }

    //  process 16 bytes build up the char array.
    for (int i = 0, j = 0; i < 16; i++) {
      //  multiples of 4 between 8 and 20 get a -.
      //  note we are processing 2 digits in one loop.
      if ((i & 0x1) == 0) {
        if ((4 <= i) && (i <= 10)) {
          _buffer[j++] = '-';
        }
      }

      //  process one byte at the time instead of a nibble
      uint8_t nr = i / 4;
      uint8_t xx = ar[nr];
      uint8_t ch = xx & 0x0F;
      _buffer[j++] = (ch < 10) ? '0' + ch : ('a' - 10) + ch;

      ch = (xx >> 4) & 0x0F;
      ar[nr] >>= 8;
      _buffer[j++] = (ch < 10) ? '0' + ch : ('a' - 10) + ch;
    }

    _buffer[36] = 0;
    return WValue((char*)&_buffer);
  }
};

#endif