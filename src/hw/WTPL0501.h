#ifndef W_TPL0501_H
#define W_TPL0501_H

#include <SPI.h>

class WTPL0501: public WOutput {
public:
    WTPL0501(int8_t SCLK, int8_t DIN, int8_t CS) : WOutput(CS) {
      tpl_spi = new SPIClass(VSPI);
      tpl_spi->begin(SCLK, -1, DIN, CS);
      //pinMode(tpl_spi->pinSS(), OUTPUT);
    }

    void resistance(uint8_t value) {
      tpl_spi->beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE3));
      digitalWrite(tpl_spi->pinSS(), LOW); //pull SS slow to prep other end for transfer
      tpl_spi->transfer(value);
      digitalWrite(tpl_spi->pinSS(), HIGH); //pull ss high to signify end of data transfer
      tpl_spi->endTransaction();
    }

private:
    static const int spiClockSpeed = 100000; // should be less than 25M
    SPIClass* tpl_spi = nullptr;

};

#endif