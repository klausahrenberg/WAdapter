#ifndef W_KY040_H
#define W_KY040_H

#include "WSwitch.h"

int knobClk;
int knobDt;
bool knobRotatedLeft;
bool knobRotatedRight;

static void knobInterruptHandler(void) { 
  bool clk = digitalRead(knobClk);
  knobRotatedLeft = (digitalRead(knobDt) == clk);
  knobRotatedRight = !knobRotatedLeft;
}

class WKY040 : public WSwitch {
 public:
  WKY040(int pinSwitch, int pinClk, int pinDt)
      : WSwitch(pinSwitch, MODE_BUTTON_LONG_PRESS, true) {    
    knobClk = pinClk;
    knobDt = pinDt;
    pinMode(knobClk, INPUT);
    pinMode(knobDt, INPUT);    
    knobRotatedLeft = false;
    knobRotatedRight = false;
    attachInterrupt(knobClk, knobInterruptHandler, RISING);
    _rotatingLeft = nullptr;
    _rotatingRight = nullptr;
  }  

  void loop(unsigned long now) {
    WSwitch::loop(now);
    if (_rotatingLeft != nullptr) {
      _rotatingLeft->asBool(knobRotatedLeft);
    }
    if (_rotatingRight != nullptr) {
      _rotatingRight->asBool(knobRotatedRight);
    }  
    knobRotatedLeft = false;
    knobRotatedRight = false;
  }

  void setRotatingLeft(WProperty* rotatingLeft) {
		_rotatingLeft = rotatingLeft;
	}

  void setRotatingRight(WProperty* rotatingRight) {
		_rotatingRight = rotatingRight;
	}

 protected:
  bool isInitialized() { return (WSwitch::isInitialized()) && (knobClk != NO_PIN) && (knobDt != NO_PIN); }  

 private:
  WProperty* _rotatingLeft;
  WProperty* _rotatingRight;

};

#endif