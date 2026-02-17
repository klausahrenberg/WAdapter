#ifndef W_2812_LED_H
#define W_2812_LED_H

#include "Adafruit_NeoPixel.h"
#include "WGpio.h"

#define COLOR_DEFAULT 0x200000
const static char WRGB_NUMBER_OF_LEDS[] PROGMEM = "leds";
const int COUNT_LED_PROGRAMS = 3;
const float PI180 = 0.01745329;
const neoPixelType LED_TYPE_WS2812 = NEO_GRB + NEO_KHZ800;
const neoPixelType LED_TYPE_PL9823 = NEO_RGB + NEO_KHZ800;

class W2812Led : public WGpio {
 public:
  typedef std::function<uint32_t()> TColorPicker;
  W2812Led(WGpioType gpioType = GPIO_TYPE_RGB_WS2812, int ledPin = NO_PIN, byte numberOfLeds = 0) : WGpio(gpioType, ledPin) {                
    _programStatusCounter = 0;
    _lastUpdate = 0;
    _color = new WColorProperty("Color", 255, 0, 0);
    _colors = new uint32_t[numberOfLeds];
    _conditions = new TColorPicker[numberOfLeds];
    for (byte b = 0; b < numberOfLeds; b++) {
      _colors[b] = COLOR_DEFAULT;
      _conditions[b] = nullptr;
    }  
    // network->getSettings()->add(this->color);
    _brightness = new WRangeProperty("Brightness", WDataType::INTEGER, WValue::ofInt(10), WValue::ofInt(255), TYPE_LEVEL_PROPERTY);
    _brightness->asInt(160);
    //network->getSettings()->add(this->brightness);
    _brightness->addListener([this]() { _strip->setBrightness(_brightness->asInt()); });
    this->numberOfLeds(numberOfLeds);    
  }  

  static W2812Led* create(IWGpioRegister* device, WGpioType gpioType = GPIO_TYPE_RGB_WS2812, int ledPin = NO_PIN, byte numberOfLeds = 0) {
    W2812Led* leds = new W2812Led(gpioType, ledPin, numberOfLeds);
    device->registerGpio(leds);
    return leds;
  }

  virtual ~W2812Led() {
    delete _numberOfLeds;
    delete[] _colors;
    if (_strip) delete _strip;
  }

  void onChanged() {
    if (!this->isOn()) {
      for (int i = 0; i < _strip->numPixels(); i++) {
        _strip->setPixelColor(i, _strip->Color(0, 0, 0));
      }
      _strip->show();
    }
  }

  byte numberOfLeds() { return _numberOfLeds->asByte(); }

  W2812Led* numberOfLeds(byte numberOfLeds) {
    if (numberOfLeds != _numberOfLeds->asByte()) {
      _numberOfLeds->asByte(numberOfLeds);
      _onChange();
    }
    return this;
  }

  byte countModes() { return COUNT_LED_PROGRAMS;}

  const char* modeTitle(byte index) {
    switch (index) {
      case 2: return "Rainbow";
      case 1: return "Cycling";
      default: return "Fixed Color";
    }
  }

  byte rgbMode() { return _rgbMode->asByte();}

  void setRgbMode(byte rgbMode) {
    if (rgbMode >= COUNT_LED_PROGRAMS) {
      rgbMode = 0;
    }
    if (_rgbMode->asByte() != rgbMode) {
      _rgbMode->asByte(rgbMode);
      _programStatusCounter = 0;
      if (_settingsRegistered) SETTINGS->save();
    }
  }

  virtual void setRgbModeByTitle(const char* title) {
    for (byte b = 0; b < countModes(); b++) {
      if (strcmp(modeTitle(b), title) == 0) {
        setRgbMode(b);
        break;
      }
    }    
  }

  WColorProperty* color() { return _color; }

  WRangeProperty* brightness() { return _brightness; }

  /*void pixelColor(uint16_t ledNumber, uint8_t red, uint8_t green, uint8_t blue, bool updateImmediatly = true) {
    _strip->setPixelColor(ledNumber, red, green, blue);
    if (updateImmediatly) _strip->show();
  }*/

  void pixelColor(uint16_t ledNumber, uint32_t color, bool updateImmediatly = true) {
    _colors[ledNumber] = color;    
    if (updateImmediatly) _strip->show();
  }  

  void show() {
    bool b = isOn();
    for (byte i = 0; i < _numberOfLeds->asByte(); i++) _strip->setPixelColor(i, (b ? _colors[i] : 0x000000));     
    _strip->show(); 
  }

  virtual void registerSettings() {
    WGpio::registerSettings();
    SETTINGS->add(_numberOfLeds, nullptr);   
    SETTINGS->add(_rgbMode, nullptr);
    _onChange(); 
  }

  virtual void fromJson(WList<WValue>* list) {
    WGpio::fromJson(list);
    WValue* v = list->getById(WRGB_NUMBER_OF_LEDS);
    numberOfLeds(v != nullptr ? v->asByte() : numberOfLeds());
  }

  virtual void toJson(WJson* json) {
    WGpio::toJson(json);    
    json->propertyByte(WRGB_NUMBER_OF_LEDS, numberOfLeds());
  }

  W2812Led* conditio(byte index, TColorPicker condition) {
    _conditions[index] = condition;
    return this;
  }

  void loop(unsigned long now) {
    WGpio::loop(now);
    if ((_needsUpdate) || (isOn())) {
      for (byte i = 0; i < _numberOfLeds->asByte(); i++) {
        if (_conditions[i]) {
          uint32_t newColor = _conditions[i]();
          _needsUpdate = _needsUpdate || (newColor != _colors[i]);
          _colors[i] = newColor;
        }
      }
      if (_needsUpdate) _strip->show();
      _needsUpdate = false;

      
      /*if (now - _lastUpdate > 200) {
        switch (rgbMode()) {
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
      }*/
    }
  }

 protected:
  void _updateOn() {
    _needsUpdate = true;
    show();
  };

  virtual bool isInitialized() { return ((WGpio::isInitialized()) && (_numberOfLeds->asByte() > 0)); }
  
  virtual void _onChange() {
    if (_strip != nullptr) {
      delete _strip;
      _strip = nullptr;
    }
    if (isInitialized()) {
      _strip = new Adafruit_NeoPixel(numberOfLeds(), pin(), (type() == GPIO_TYPE_RGB_WS2812 ? LED_TYPE_WS2812 : LED_TYPE_PL9823));    
      _strip->begin();  // INITIALIZE NeoPixel strip object (REQUIRED)    
      _strip->show();   // Turn OFF all pixels ASAP
      _strip->setBrightness(_brightness->asInt());  // Set BRIGHTNESS to about 1/5 (max = 255)    
    }
  }    

 private:
  WValue* _numberOfLeds = new WValue(WDataType::BYTE);  
  WValue* _rgbMode = new WValue((byte) 2);
  Adafruit_NeoPixel* _strip = nullptr;  
  int _programStatusCounter;
  WColorProperty* _color;
  WRangeProperty* _brightness;
  unsigned long _lastUpdate;
  bool _needsUpdate = false;
  uint32_t* _colors;
  TColorPicker* _conditions;

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
