#ifndef W_UTILS_H
#define W_UTILS_H

#include "Arduino.h"

const static char WC_BASE[] PROGMEM = " =<>/\"{}()[],";
const static char* APPLICATION = nullptr;
const static char* VERSION = nullptr;
static byte FLAG_SETTINGS = 0x23;
static bool DEBUG = true;

class WUtils {
 public: 
  static uint32_t getChipId() {
    #ifdef ESP8266
    uint32_t ci = ESP.getChipId();
    #elif ESP32
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    uint32_t ci = (macAddressTrunc >> 40);
    #endif
    /*char* textToWrite =  new char[16];
    sprintf(textToWrite,"%lu", ci);
    return textToWrite;*/
    return ci;
  }

  static void boolean(Print* stream, bool value) {		
		stream->print(value ? F("true") : F("false"));
  }  

  static void numberDouble(Print* stream, double number) {
    stream->print(number);
  }

  static void numberInteger(Print* stream, int number) {
    stream->print(number, DEC);
  }

  static void numberShort(Print* stream, short number) {
    stream->print(number, DEC);
  }  

  static void numberUnsignedLong(Print* stream, unsigned long number) {
    stream->print(number, DEC);
  }

  static void numberByte(Print* stream, byte number) {
    stream->print(number, DEC);
  }  

  static void string(Print* stream, const char* text, ...) {    
    va_list arg;        
    va_start(arg, text);    
    while (text) {
      stream->print(text);
      text = va_arg(arg, const char*);
    }
    va_end(arg);
  }

  static void numberByteArray(Print* stream, byte length, byte* value) {
    stream->print(WC_BASE[10]);
		for (byte i = 0; i < length; i++) {
			if (i != 0) stream->print(WC_BASE[12]);
			stream->print(value[i], DEC);
		}
		stream->print(WC_BASE[11]);
  }  

};

#endif