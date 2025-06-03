#ifndef W_MCP444x_H
#define W_MCP444x_H

#include "WI2C.h"

#define WMCP444x_ADRESS 0x2F
#define DEFAULT_WIPER_VALUE 0x80  // Default to the wipers in midrange

// meory addresses (all shifted 4 bits left)
// For all the Wipers 0x100 = Full scale, 0x80 = mid scale, 0x0 = Zero scale
#define MCP4461_VW0 0x00
#define MCP4461_VW1 0x10
#define MCP4461_NVW0 0x20
#define MCP4461_NVW1 0x30
// TCON0: D8: Reserved D7:R1HW D6: R1A D5:R1W D4:R1B D3:R0HW D2:R0A D1:R0W D0: R0B
#define MCP4461_TCON0 0x40
// STATUS: D8:D7: Reserved D6: WL3 D5:WL2 D4:EEWA D3:WL1 D2:WL0 D1:Reserved D0: WP
#define MCP4461_STATUS 0x50
#define MCP4461_VW2 0x60
#define MCP4461_VW3 0x70
#define MCP4461_NVW2 0x80
#define MCP4461_NVW3 0x90
// TCON0: D8: Reserved D7:R3HW D6: R3A D5:R3W D4:R3B D3:R2HW D2:R2A D1:R2W D0: R2B
#define MCP4461_TCON1 0xA0

// control commands
#define MCP4461_WRITE 0x0
#define MCP4461_INCREMENT 0x4  // 01 left shift by 2
#define MCP4461_DECREMENT 0x8  // 10 left shift by 2
#define MCP4461_READ 0xC       // 11 left shift by 2

class WMCP444x : public WI2C {
 public:
  WMCP444x(int sda = 21, int scl = 22, byte address = WMCP444x_ADRESS, TwoWire* i2cPort = &Wire)
      : WI2C(GPIO_TYPE_MCP444x, address, sda, scl, NO_PIN, i2cPort) {
  }

  // void begin() {}
  // void setMCP4461Address(uint8_t) {}

  bool begin() {
    Wire.beginTransmission(address());
    byte error = Wire.endTransmission();
    return (error == 0);
  }

  void write(uint8_t wiper, uint8_t value) {
    Wire.beginTransmission(address());
    Wire.write(wiper); // write to address 0 
    Wire.write(value);
    Wire.endTransmission();   
  }

  void setVolatileWiper(uint8_t wiper, uint16_t wiper_value) {
    uint16_t value = wiper_value;
    if (value > 0xFF) value = 0x100;
    uint8_t d_byte = (uint8_t)value;
    uint8_t c_byte;
    if (value > 0xFF)
      c_byte = 0x1;  // the 8th data bit is 1
    else
      c_byte = 0;
    switch (wiper) {
      case 0:
        c_byte |= MCP4461_VW0;
        break;
      case 1:
        c_byte |= MCP4461_VW1;
        break;
      case 2:
        c_byte |= MCP4461_VW2;
        break;
      case 3:
        c_byte |= MCP4461_VW3;
        break;
      default:
        break;  // not a valid wiper
    }
    c_byte |= MCP4461_WRITE;
    // send command byte
    Wire.beginTransmission(address());
    Wire.write(c_byte);
    Wire.write(d_byte);
    Wire.endTransmission();  // do not release bus
  }

  void setNonVolatileWiper(uint8_t wiper, uint16_t wiper_value) {
    uint16_t value = wiper_value;
    if (value > 0xFF) value = 0x100;
    uint8_t d_byte = (uint8_t)value;
    uint8_t c_byte;
    if (value > 0xFF)
      c_byte = 0x1;  // the 8th data bit is 1
    else
      c_byte = 0;
    switch (wiper) {
      case 0:
        c_byte |= MCP4461_NVW0;
        break;
      case 1:
        c_byte |= MCP4461_NVW1;
        break;
      case 2:
        c_byte |= MCP4461_NVW2;
        break;
      case 3:
        c_byte |= MCP4461_NVW3;
        break;
      default:
        break;  // not a valid wiper
    }
    c_byte |= MCP4461_WRITE;
    // send command byte
    Wire.beginTransmission(address());
    Wire.write(c_byte);
    Wire.write(d_byte);
    Wire.endTransmission();  // do not release bus
    delay(20);               // allow the write to complete (this is wasteful - better to check if the write has completed)
  }

  void setVolatileWipers(uint16_t wiper_value) {
  }

  void setNonVolatileWipers(uint16_t) {}
  uint16_t getVolatileWiper(uint8_t) {}
  uint16_t getNonVolatileWiper(uint8_t) const {}

  uint16_t read_2(byte mem_addr) {
      uint16_t ret = 0;
      uint16_t c_byte = 0;
      c_byte |= MCP4461_STATUS;
      c_byte |= MCP4461_READ;
      // send command byte
      Serial.println("a");
      Wire.beginTransmission(address());
      Serial.println("b");
      Wire.write(c_byte);
      Serial.println("c");
      Wire.endTransmission(false);  // do not release bus
      Serial.println("d");
      Wire.requestFrom((uint8_t)address(), (uint8_t)2);
      Serial.println("e");
      // read the register
      int i = 0;
      while (Wire.available()) {
          Serial.println("f");
          ret |= Wire.read();
          if (i == 0) ret = ret << 8;
          i++;
      }
      Serial.print("g ");
      Serial.println(ret);
      return ret;
  }

  uint16_t read(uint8_t mem_addr) {
    // mem addr 0x00 - 0x0f ( 0-16 )
    if (mem_addr < 0x00 || mem_addr > 0x0F) {
      return 0x0FFF;  // return something that is out of the expected bounds to signify an error
    }
    byte cmd_byte = 0x0F, highbyte, lowbyte;
    cmd_byte = (mem_addr << 4) | B00001100;

    Wire.beginTransmission(address());
    Wire.write(cmd_byte);
    Wire.endTransmission();
    Wire.requestFrom(address(), 2);
    Wire.endTransmission();  // stop transmitting
    if (Wire.available()) {
      highbyte = Wire.read();  // high byte
      lowbyte = Wire.read();   // low byte
      Wire.flush();
    }
    uint16_t returnValue = 0;
    returnValue = (((uint16_t)highbyte << 8) | lowbyte) & 0x01FF;
    return returnValue;
  }

  /*uint8_t write(uint8_t mem_addr, uint16_t setValue)  // mem_addr is 00-0F, setvalue is 0-257
  {
      // if you set the volatile output register, the same value ispu tto the non-volatile register
      if (setValue < 0) {
          setValue = 0;
      }
      if (setValue > 257) {
          setValue = 257;
      }
      if (mem_addr < 0x00 || mem_addr > 0x0F) {
          return 0;
      }

      byte cmd_byte = 0x00, data_byte = 0x00;
      cmd_byte = ((mem_addr << 4) & B11110000) | (((setValue & 0x01FF) >> 8) & B00000011);  //  top 4 is now address   2 command   2 data (D9,D8)
      data_byte = lowByte(setValue);                                                        // is now D7-D0
      Wire.beginTransmission(address());                                                    // transmit to device this has the read byte ( LSB) set as 1
                                                                                            // device address                 0101110x
      Wire.write(cmd_byte);                                                                 // sends command byte             AAAACCDD
      Wire.write(data_byte);                                                                // sends potentiometer value byte DDDDDDDD  (D7-D0)
      Wire.endTransmission();                                                               // stop transmitting
      Wire.flush();
      delay(10);  // give unit time to apply the value to non volatile register
      uint16_t set_reading = read(mem_addr);
      if (set_reading == setValue) {
          return 1;  // it has accepted our setting ( EEPROM reflects what we set it to )
      }
      return 0;
  }*/

  

 protected:
 private:
  uint8_t _wiper;
  uint8_t _value;
};

#endif