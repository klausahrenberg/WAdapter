#ifndef W_MCP4x000_H
#define W_MCP4x000_H

#include <SPI.h>

#include "WNetwork.h"

//  see page 18 datasheet
#define MCP_POT_IGNORE_CMD   0b00000000
#define MCP_POT_WRITE_CMD    0b00010000
#define MCP_POT_SHUTDOWN_CMD 0b00100000
#define MCP_POT_NONE_CMD     0b00110000

class WMcp4x000 : public WGpio {
 public:
  WMcp4x000(int8_t SCLK, int8_t DIN, int8_t CS) : WGpio(GPIO_TYPE_MCP4x000, CS, OUTPUT, nullptr) {
    _spi = new SPIClass(VSPI);
    _sclk = SCLK;
    _din = DIN;
    digitalWrite(pin(), HIGH);
    mode(_sclk, OUTPUT);
    mode(_din, OUTPUT);
    begin();
  }

  void begin() {
    _spi->begin(_sclk, -1, _din, pin());
    delay(100);
  }

  uint8_t resistance() { return _value; }

  void resistance(uint8_t value) {
    _value = value;
    _updateDevice(2, value, MCP_POT_WRITE_CMD);
  }

 protected:
  void _updateDevice(uint8_t pm, uint8_t value, uint8_t cmd) {
    uint8_t command = cmd;
    if (pm == 0) command |= 0b01;  //  01
    if (pm == 1) command |= 0b10;  //  10
    if (pm == 2) command |= 0b11;  //  11 => both potentiometers
    //  otherwise ignore
    digitalWrite(pin(), LOW);
    delay(100);

    _spi->beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0));
    digitalWrite(_spi->pinSS(), LOW);
    _spi->transfer(command);
    _spi->transfer(value);
    digitalWrite(_spi->pinSS(), HIGH);
    _spi->endTransaction();

    digitalWrite(pin(), HIGH);
    delay(100);
  }

 private:
  static const int spiClockSpeed = 100000;  // should be less than 25M
  SPIClass* _spi = nullptr;
  uint8_t _value = 0;
  byte _sclk, _din;
};

#endif

//       HARDWARE SPI
/*MCP_POT::MCP_POT(uint8_t select, uint8_t reset, uint8_t shutdown, __SPI_CLASS__ * mySPI)
{
  _pmCount  = 2;
  _select   = select;
  _reset    = reset;
  _shutdown = shutdown;
  _dataOut  = 255;
  _clock    = 255;
  _hwSPI    = true;
  _mySPI    = mySPI;
}


//       SOFTWARE SPI
MCP_POT::MCP_POT(uint8_t select, uint8_t reset, uint8_t shutdown, uint8_t dataOut, uint8_t clock)
{
  _pmCount  = 2;
  _select   = select;
  _reset    = reset;
  _shutdown = shutdown;
  _dataOut  = dataOut;
  _clock    = clock;
  _hwSPI    = false;
  _mySPI    = NULL;
}


void MCP_POT::begin(uint8_t value)
{

}


void MCP_POT::reset(uint8_t value)
{
  digitalWrite(_reset, LOW);
  digitalWrite(_reset, HIGH);
  setValue(value);  //  set all to same value.
}



/////////////////////////////////////////////////////////////////////////////
//
//  SET VALUE
//
bool MCP_POT::setValue(uint8_t value)
{
  _value[0] = value;
  _value[1] = value;
  updateDevice(2, value, MCP_POT_WRITE_CMD);
  return true;
}


bool MCP_POT::setValue(uint8_t pm, uint8_t value)
{
  if (pm >= _pmCount) return false;
  _value[pm] = value;
  updateDevice(pm, value, MCP_POT_WRITE_CMD);
  return true;
}


uint8_t MCP_POT::getValue(uint8_t pm)
{
  if (pm >= _pmCount) return 0;
  return _value[pm];
}


/////////////////////////////////////////////////////////////////////////////
//
//  OHM - wrappers
//
void MCP_POT::setMaxOhm(uint32_t maxOhm)
{
  _maxOhm = maxOhm;
}

uint32_t MCP_POT::getMaxOhm()
{
  return _maxOhm;
}

void MCP_POT::setOhm(uint8_t pm, uint32_t ohm)
{
  setValue(pm, round(ohm * 255.0 / _maxOhm));
}

uint32_t MCP_POT::getOhm(uint8_t pm)
{
  return round(getValue(pm) * (_maxOhm / 255.0));
}


/////////////////////////////////////////////////////////////////////////////
//
//  OTHER
//
uint8_t MCP_POT::pmCount()
{
  return _pmCount;
}


void MCP_POT::powerOn()
{
  digitalWrite(_shutdown, HIGH);
}


void MCP_POT::powerOff()
{
  digitalWrite(_shutdown, LOW);
}


bool MCP_POT::isPowerOn()
{
  return digitalRead(_shutdown) == HIGH;
}


/////////////////////////////////////////////////////////////////////////////
//
//  SPI
//
void MCP_POT::setSPIspeed(uint32_t speed)
{
  _SPIspeed = speed;
  _spi_settings = SPISettings(_SPIspeed, MSBFIRST, SPI_MODE0);
}


uint32_t MCP_POT::getSPIspeed()
{
  return _SPIspeed;
}


void MCP_POT::setSWSPIdelay(uint16_t del)
{
  _swSPIdelay = del;
}


uint16_t MCP_POT::getSWSPIdelay()
{
  return _swSPIdelay;
}


bool MCP_POT::usesHWSPI()
{
  return _hwSPI;
}


/////////////////////////////////////////////////////////////////////////////
//
//  PROTECTED
//
void MCP_POT::updateDevice(uint8_t pm, uint8_t value, uint8_t cmd)
{
  uint8_t command = cmd;
  if (pm == 0) command |= 1;   //  01
  if (pm == 1) command |= 2;   //  10
  if (pm == 2) command |= 3;   //  11 => both potentiometers
  //  otherwise ignore
  digitalWrite(_select, LOW);
  if (_hwSPI)
  {
    _mySPI->beginTransaction(_spi_settings);
    _mySPI->transfer(command);
    _mySPI->transfer(value);
    _mySPI->endTransaction();
  }
  else      //  Software SPI
  {
    swSPI_transfer(command);
    swSPI_transfer(value);
  }
  digitalWrite(_select, HIGH);
}


//  MSBFIRST
void  MCP_POT::swSPI_transfer(uint8_t val)
{
  //  split _swSPIdelay in equal dLow and dHigh
  //  dLow should be longer one when _swSPIdelay = odd.
  uint16_t dHigh = _swSPIdelay / 2;
  uint16_t dLow  = _swSPIdelay - dHigh;

  uint8_t clk = _clock;
  uint8_t dao = _dataOut;
  //  MSBFIRST
  for (uint8_t mask = 0x80; mask; mask >>= 1)
  {
    digitalWrite(dao,(val & mask));
    digitalWrite(clk, HIGH);
    if (dHigh > 0) delayMicroseconds(dHigh);
    digitalWrite(clk, LOW);
    if (dLow > 0) delayMicroseconds(dLow);
  }
}


////////////////////////////////////////////////////////////////////////////
//
//  DERIVED CLASSES MCP41000 SERIES
//
MCP41010::MCP41010(uint8_t select, uint8_t reset, uint8_t shutdown, __SPI_CLASS__ * mySPI)
        :MCP_POT(select, reset, shutdown, mySPI)
{
  _pmCount = 1;
  _maxOhm  = 10000;
}

MCP41010::MCP41010(uint8_t select, uint8_t reset, uint8_t shutdown, uint8_t dataOut, uint8_t clock)
        :MCP_POT(select, reset, shutdown, dataOut, clock)
{
  _pmCount = 1;
  _maxOhm  = 10000;
}

MCP41050::MCP41050(uint8_t select, uint8_t reset, uint8_t shutdown, __SPI_CLASS__ * mySPI)
        :MCP_POT(select, reset, shutdown, mySPI)
{
  _pmCount = 1;
  _maxOhm  = 50000;
}

MCP41050::MCP41050(uint8_t select, uint8_t reset, uint8_t shutdown, uint8_t dataOut, uint8_t clock)
        :MCP_POT(select, reset, shutdown, dataOut, clock)
{
  _pmCount = 1;
  _maxOhm  = 10000;
}

MCP41100::MCP41100(uint8_t select, uint8_t reset, uint8_t shutdown, __SPI_CLASS__ * mySPI)
        :MCP_POT(select, reset, shutdown, mySPI)
{
  _pmCount = 1;
  _maxOhm  = 100000;
}

MCP41100::MCP41100(uint8_t select, uint8_t reset, uint8_t shutdown, uint8_t dataOut, uint8_t clock)
        :MCP_POT(select, reset, shutdown, dataOut, clock)
{
  _pmCount = 1;
  _maxOhm  = 100000;
}


////////////////////////////////////////////////////////////////////////////
//
//  DERIVED CLASSES MCP42000 SERIES
//
MCP42010::MCP42010(uint8_t select, uint8_t reset, uint8_t shutdown, __SPI_CLASS__ * mySPI)
        :MCP_POT(select, reset, shutdown, mySPI)
{
  _pmCount = 2;
  _maxOhm  = 10000;
}

MCP42010::MCP42010(uint8_t select, uint8_t reset, uint8_t shutdown, uint8_t dataOut, uint8_t clock)
        :MCP_POT(select, reset, shutdown, dataOut, clock)
{
  _pmCount = 2;
  _maxOhm  = 10000;
}

MCP42050::MCP42050(uint8_t select, uint8_t reset, uint8_t shutdown, __SPI_CLASS__ * mySPI)
        :MCP_POT(select, reset, shutdown, mySPI)
{
  _pmCount = 2;
  _maxOhm  = 50000;
}

MCP42050::MCP42050(uint8_t select, uint8_t reset, uint8_t shutdown, uint8_t dataOut, uint8_t clock)
        :MCP_POT(select, reset, shutdown, dataOut, clock)
{
  _pmCount = 2;
  _maxOhm  = 50000;
}

MCP42100::MCP42100(uint8_t select, uint8_t reset, uint8_t shutdown, __SPI_CLASS__ * mySPI)
        :MCP_POT(select, reset, shutdown, mySPI)
{
  _pmCount = 2;
  _maxOhm  = 100000;
}

MCP42100::MCP42100(uint8_t select, uint8_t reset, uint8_t shutdown, uint8_t dataOut, uint8_t clock)
        :MCP_POT(select, reset, shutdown, dataOut, clock)
{
  _pmCount = 2;
  _maxOhm  = 100000;
}
*/
