#ifndef W_2812_LED_H
#define W_2812_LED_H

#include "Adafruit_NeoPixel.h"
#include "WProps.h"
#include "WOutput.h"

const int COUNT_LED_PROGRAMS = 3;
const float PI180 = 0.01745329;
const neoPixelType LED_TYPE_WS2812 = NEO_GRB + NEO_KHZ800;
const neoPixelType LED_TYPE_PL9823 = NEO_RGB + NEO_KHZ800;

class W2812Led : public WOutput {
 public:
  W2812Led(WNetwork* network, int ledPin, byte numberOfLeds, neoPixelType ledType = LED_TYPE_WS2812) : WOutput(ledPin) {        
    _network = network;
    _numberOfLeds = numberOfLeds;
    _ledProgram = 2;
    _programStatusCounter = 0;
    _lastUpdate = 0;
    _color = new WColorProperty("color", "Color", 255, 0, 0);
    // network->getSettings()->add(this->color);
    _brightness = WProps::createLevelIntProperty("brightness", "Brightness", 10, 255);
    _brightness->asInt(160);
    // network->getSettings()->add(this->brightness);
    _brightness->addListener([this]() {
      _strip->setBrightness(_brightness->asInt());
    });
    _strip = new Adafruit_NeoPixel(numberOfLeds, ledPin, ledType);
    //_strip = new Adafruit_NeoPixel(numberOfLeds, ledPin, NEO_GRBW + NEO_KHZ800);
    _strip->begin();  // INITIALIZE NeoPixel strip object (REQUIRED)    
    _strip->show();   // Turn OFF all pixels ASAP
    _strip->setBrightness(_brightness->asInt());  // Set BRIGHTNESS to about 1/5 (max = 255)    
  }  

  void onChanged() {
    if (!this->isOn()) {
      for (int i = 0; i < _strip->numPixels(); i++) {
        _strip->setPixelColor(i, _strip->Color(0, 0, 0));
      }
      _strip->show();
    }
  }

  byte countModes() { return COUNT_LED_PROGRAMS;}

  const char* modeTitle(byte index) {
    switch (index) {
      case 2: return "Rainbow";
      case 1: return "Cycling";
      default: return "Fixed Color";
    }
  }

  byte mode() { return _ledProgram;}

  void setMode(byte ledProgram) {
    if (ledProgram >= COUNT_LED_PROGRAMS) {
      ledProgram = 0;
    }
    if (_ledProgram != ledProgram) {
      _ledProgram = ledProgram;
      _programStatusCounter = 0;
    }
  }

  WColorProperty* color() { return _color; }

  WRangeProperty* brightness() { return _brightness; }

  void pixelColor(uint16_t ledNumber, uint8_t red, uint8_t green, uint8_t blue) {
    _strip->setPixelColor(ledNumber, red, green, blue);
    _strip->show();
  }

  void pixelColor(uint16_t ledNumber, uint32_t color) {
    _strip->setPixelColor(ledNumber, color);
    _strip->show();
  }  

  void loop(unsigned long now) {
    if (isOn()) {
      
      if (now - _lastUpdate > 200) {
        switch (_ledProgram) {
          case 0: {
            // Pulsing RGB color
            float t = sin((_programStatusCounter - 90) * PI180);
            t = (t + 1) / 2;
            uint32_t pulseColor = _strip->Color(
                round(_color->red() * (85 + round(170.0 * t)) / 255),
                round(_color->green() * (85 + round(170.0 * t)) / 255),
                round(_color->blue() * (85 + round(170.0 * t)) / 255));
            for (uint16_t rainbowI = 0; rainbowI < _strip->numPixels();
                 rainbowI++) {
              _strip->setPixelColor(rainbowI, pulseColor);
            }
            _strip->show();
            _programStatusCounter++;
            if (_programStatusCounter >= 360) {
              _programStatusCounter = 0;
            }
            break;
          }
          case 1: {
            // Rainbow program, all pixel the same color
            uint32_t wC = wheelColor((_programStatusCounter)&255);
            for (uint16_t rainbowI = 0; rainbowI < _strip->numPixels();
                 rainbowI++) {
              _strip->setPixelColor(rainbowI, wC);
            }
            _strip->show();
            _programStatusCounter++;
            if (_programStatusCounter == 256 * 5) {
              _programStatusCounter = 0;
            }
            break;
          }
          case 2: {
            // Rainbow program, different colors
            for (uint16_t rainbowI = 0; rainbowI < _strip->numPixels(); rainbowI++) {
              _strip->setPixelColor(rainbowI, wheelColor(((rainbowI * 256 / _strip->numPixels()) + _programStatusCounter) & 255));
            }
            _strip->show();
            _programStatusCounter++;
            if (_programStatusCounter == 256 * 5) {
              _programStatusCounter = 0;
            }
            break;
          }
        }
        _lastUpdate = now;
      }
    }
  }

 protected:
 private:
  WNetwork* _network;
  byte _numberOfLeds;
  byte _ledProgram;
  int _programStatusCounter;
  WColorProperty* _color;
  WRangeProperty* _brightness;
  unsigned long _lastUpdate;
  Adafruit_NeoPixel* _strip;

  uint32_t wheelColor(byte wheelPos) {
    byte c;
    if (wheelPos < 85) {
      c = wheelPos * 3;
      return _strip->Color(c, 255 - c, 0);
    } else if (wheelPos < 170) {
      // wheelPos -= 85;
      c = (wheelPos - 85) * 3;
      return _strip->Color(255 - c, 0, c);
    } else {
      // wheelPos -= 170;
      c = (wheelPos - 170) * 3;
      return _strip->Color(0, c, 255 - c);
    }
  }

  void whiteOverRainbow(int whiteSpeed, int whiteLength) {
    if (whiteLength >= _strip->numPixels()) whiteLength = _strip->numPixels() - 1;

    int head = whiteLength - 1;
    int tail = 0;
    int loops = 3;
    int loopNum = 0;
    uint32_t lastTime = millis();
    uint32_t firstPixelHue = 0;

    for (;;) {  // Repeat forever (or until a 'break' or 'return')
      for (int i = 0; i < _strip->numPixels();
           i++) {                            // For each pixel in strip...
        if (((i >= tail) && (i <= head)) ||  //  If between head & tail...
            ((tail > head) && ((i >= tail) || (i <= head)))) {
          _strip->setPixelColor(i, _strip->Color(0, 0, 0, 255));  // Set white
        } else {  // else set rainbow
          int pixelHue = firstPixelHue + (i * 65536L / _strip->numPixels());
          _strip->setPixelColor(i, _strip->gamma32(_strip->ColorHSV(pixelHue)));
        }
      }

      _strip->show();  // Update strip with new contents
      // There's no delay here, it just runs full-tilt until the timer and
      // counter combination below runs out.

      firstPixelHue += 40;  // Advance just a little along the color wheel

      if ((millis() - lastTime) > whiteSpeed) {  // Time to update head/tail?
        if (++head >= _strip->numPixels()) {      // Advance head, wrap around
          head = 0;
          if (++loopNum >= loops) return;
        }
        if (++tail >= _strip->numPixels()) {  // Advance tail, wrap around
          tail = 0;
        }
        lastTime = millis();  // Save time of last movement
      }
    }
  }
};

#endif
