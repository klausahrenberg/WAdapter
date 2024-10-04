#ifndef W_TPL0501_H
#define W_TPL0501_H

#include <SPI.h>

class WTPL0501: public WGpio {
public:
    WTPL0501(int8_t SCLK, int8_t DIN, int8_t CS) : WGpio(GPIO_TYPE_TPL0501, CS, OUTPUT, nullptr) {
      _spi = new SPIClass(VSPI);      
      _sclk = SCLK;
      _din = DIN;
      begin();
    }

    void begin() {
      _spi->begin(_sclk, -1, _din, pin());
      delay(100);
    }  

    uint8_t resistance() { return _value; }

    void resistance(uint8_t value) {
      _value = value;
      _spi->beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0));
      digitalWrite(_spi->pinSS(), LOW); //pull SS slow to prep other end for transfer
      
      _spi->transfer(value);
      digitalWrite(_spi->pinSS(), HIGH); //pull ss high to signify end of data transfer
      _spi->endTransaction();
    }

private:
    static const int spiClockSpeed = 100000; // should be less than 25M
    SPIClass* _spi = nullptr;
    uint8_t _value = 0;
    byte _sclk, _din;
};

#endif