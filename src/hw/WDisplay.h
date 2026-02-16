#ifndef W_DISPLAY_H
#define W_DISPLAY_H

#include "WGpio.h"
#include "U8g2lib.h"

class WDevice;

class WDisplay : public WGpio {
 public:
  WDisplay(byte scl, byte sda, byte cs, byte dc, byte rst) {
    _display = new U8G2_SH1122_256X64_2_4W_SW_SPI(U8G2_R0, scl, sda, cs, dc, rst);

  }

  static WDisplay* create(IWGpioRegister* device, byte scl, byte sda, byte cs, byte dc, byte rst) {
    WDisplay* d = new WDisplay(scl, sda, cs, dc, rst);
    device->registerGpio(d);
    return d;
  }

  void begin() {
    _display->begin();
  }

  void show(const char* value) {
    _display->firstPage();
    do {
      _display->clearBuffer();
      _display->setContrast(2);
      _display->setFont(u8g2_font_fur20_tf);
      // u8g2.setFont(u8g2_font_luIS18_tr);
      // u8g2.setFont(u8g2_font_lubI18_te);
      // u8g2.setFont(u8g2_font_ncenB08_tr);
      _display->drawStr(10, 35, value);  // 0 left, 0 top bottom appx 100
    } while (_display->nextPage());
  }

 protected:

 private:
  U8G2_SH1122_256X64_2_4W_SW_SPI* _display;
}; 


#endif