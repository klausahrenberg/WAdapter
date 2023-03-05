#ifndef W_COLOR_PROPERTY_H
#define W_COLOR_PROPERTY_H

#include "WProperty.h"
#include "WStringStream.h"

class WColorProperty : public WProperty {
 public:
  WColorProperty(const char* id, const char* title, byte red, byte green, byte blue)
      : WProperty(id, title, STRING, TYPE_COLOR_PROPERTY) {
    _red = red;
    _green = green;
    _blue = blue;      
    setRGBString();
    //this->setRGB(red, green, blue);
    _changeValue = false;
  }

  byte red() { return _red; }

  byte green() { return _green; }

  byte blue() { return _blue; }

  void setRGB(byte red, byte green, byte blue) {      
    if ((_red != red) || (_green != green) || (_blue != blue)) {
      _red = red;
      _green = green;
      _blue = blue;      
      setRGBString();
    }
  }

  void setRGBString() {
    WStringStream result(7);
    result.print("#");
    char buffer[3];    
    itoa(_red, buffer, 16);    
    if (_red < 0x10) result.print("0");
    result.print(buffer);
    itoa(_green, buffer, 16);
    if (_green < 0x10) result.print("0");
    result.print(buffer);
    itoa(_blue, buffer, 16);
    if (_blue < 0x10) result.print("0");
    result.print(buffer);
    _changeValue = true;
    setString(result.c_str());
    _changeValue = false;
  }

  void parseRGBString() {
    char buffer[3];
    buffer[2] = '\0';
    buffer[0] = c_str()[1];
    buffer[1] = c_str()[2];
    _red = strtol(buffer, NULL, 16);
    buffer[0] = c_str()[3];
    buffer[1] = c_str()[4];
    _green = strtol(buffer, NULL, 16);
    buffer[0] = c_str()[5];
    buffer[1] = c_str()[6];
    _blue = strtol(buffer, NULL, 16);
  }

  bool parse(String value) {
    if ((!isReadOnly()) && (value != nullptr)) {
      if ((value.startsWith("#")) && (value.length() == 7)) {
        setString(value.c_str());
        return true;
      } else if ((value.startsWith("rgb(")) && (value.endsWith(")"))) {
        value = value.substring(4, value.length() - 1);
        int theComma;
        // red
        byte red = 0;
        if ((theComma = value.indexOf(",")) > -1) {
          red = value.substring(0, theComma).toInt();
          value = value.substring(theComma + 1);
        }
        // green
        byte green = 0;
        if ((theComma = value.indexOf(",")) > -1) {
          green = value.substring(0, theComma).toInt();
          value = value.substring(theComma + 1);
        }
        // blue
        byte blue = value.toInt();
        setRGB(red, green, blue);
      }
    }
    return false;
  }

 protected:
  virtual void valueChanged() {
    if (!_changeValue) {
      parseRGBString();
    }
  }

 private:
  bool _changeValue;
  byte _red, _green, _blue;
};

#endif
