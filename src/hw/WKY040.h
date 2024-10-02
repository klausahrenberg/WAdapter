#ifndef W_KY040_H
#define W_KY040_H

#include "WSwitch.h"

class WKY040;
static void _addJog(WKY040 *jog);
static void _jogEventHandler(void);

class WKY040 : public WSwitch {
 public:
  WKY040(int pinSwitch, int pinClk, int pinDt, bool inverted = false, IWExpander *expander = nullptr)
      : WSwitch(GPIO_TYPE_BUTTON, pinSwitch, true, expander) {        
    _pinClk = pinClk;
    _pinDt = pinDt;
    _pinMode(_pinClk, (inverted ? INPUT_PULLUP : INPUT));
    _pinMode(_pinDt, (inverted ? INPUT_PULLUP : INPUT));    
    _irqEventLeft = _irqEventRight = false;
    _useInterrupt = (expander == nullptr);
    supportLongPress(true);
    if (_useInterrupt) {
      _addJog(this);
      attachInterrupt(_pinClk, _jogEventHandler, (inverted ? FALLING : RISING));
    }
    _rotatingLeft = nullptr;
    _rotatingRight = nullptr;
    if (this->isInitialized()) {
      _lastClk = readInput(_pinClk);
    }    
  }

  void loop(unsigned long now) {
    WSwitch::loop(now);
    bool clk = (!_useInterrupt ? readInput(_pinClk) : getOffLevel());
    //Serial.println(readInput(_pinClk));
    if ((_irqEventLeft) || (_irqEventRight) || ((_lastClk == getOffLevel()) && (clk == getOnLevel()))) {      
      if (!_useInterrupt) {
        
        bool dt = readInput(_pinDt);
        _irqEventLeft = (dt == clk);
        _irqEventRight = !_irqEventLeft;
      }
      if (inverted()) {
        _irqEventLeft = !_irqEventLeft;
        _irqEventRight = !_irqEventRight;
      }
      
      Serial.print("jog changed at pin ");
      Serial.print(_pinClk);
      Serial.print(" rotating ");
      Serial.println(_irqEventLeft ? "left" : "right");

      this->handleButtonOrSwitchPressed();
      
      if (_rotatingLeft != nullptr) {
        _rotatingLeft->asBool(_irqEventLeft);
      }
      if (_rotatingRight != nullptr) {
        _rotatingRight->asBool(!_irqEventRight);
      };
    }
    _irqEventLeft = false;
    _irqEventRight = false;
    _lastClk = clk;
  }

  void rotatingLeft(WProperty *rotatingLeft) {
    _rotatingLeft = rotatingLeft;
  }

  void rotatingRight(WProperty *rotatingRight) {
    _rotatingRight = rotatingRight;
  }

  bool _useInterrupt;
  bool _irqEventLeft, _irqEventRight;
  int _pinClk, _pinDt;

  bool rotatedLeft() { return _irqEventLeft; }

  bool rotatedRight() { return _irqEventRight; }

 protected:
  bool isInitialized() { return (WSwitch::isInitialized()) && (_pinClk != NO_PIN) && (_pinDt != NO_PIN); }

 private:
  WProperty *_rotatingLeft;
  WProperty *_rotatingRight;
  bool _lastClk;
};

WList<WKY040> *_jogs = nullptr;

static void _addJog(WKY040 *jog) {
    if (_jogs == nullptr) {
        _jogs = new WList<WKY040>();
    }
    _jogs->add(jog);
}

static void _jogEventHandler(void) {
    if (_jogs != nullptr) {
        _jogs->forEach([](int index, WKY040 *jog, const char* id) { 
          bool clk = digitalRead(jog->_pinClk);
          jog->_irqEventLeft = (digitalRead(jog->_pinDt) == clk);
          jog->_irqEventRight = !jog->_irqEventLeft;          
        });
    }
}

#endif