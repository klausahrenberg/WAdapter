#ifndef W_SWITCH_H_
#define W_SWITCH_H_

#include "WGpio.h"

#define SWITCH_PRESSED_PEGEL HIGH

const unsigned long SWITCH_SENSITIVENESS = 200;
const unsigned long BUTTON_SENSITIVENESS = 20;
const unsigned long SWICTH_LONG_PRESS_DURATION = 5000;

class WSwitch : public WGpio {
 public:
  WSwitch(WGpioType gpioType = GPIO_TYPE_BUTTON, int switchPin = NO_PIN, bool inverted = false, IWExpander* expander = nullptr)
      : WGpio(gpioType, switchPin, (inverted ? INPUT_PULLUP : INPUT), expander) {
    _startTime = 0;
    _longPressStartTime = 0;
    _inverted = inverted;
    _triggerProperty = nullptr;
    if (this->isInitialized()) {
      _state = readInput(switchPin);
      _lastState = _state;
    }
  }

  void loop(unsigned long now) {
    if (this->isInitialized()) {
      // 1. Eliminate flickering input
      bool stateChanged = false;
      bool newState = readInput(pin());
      bool expectedPegel = (_type == GPIO_TYPE_SWITCH ? !_state : getOnLevel());
      unsigned long sensitiveness = (_type == GPIO_TYPE_SWITCH ? SWITCH_SENSITIVENESS : BUTTON_SENSITIVENESS);

      if ((newState != _lastState) && (_startTime == 0)) {
        _startTime = now;
      } else if ((newState == _state) && (_startTime > 0)) {
        _startTime = now;
      } else if ((newState != _state) && (now - _startTime >= sensitiveness)) {
        stateChanged = true;
      }

      _lastState = newState;
      // 2. If state really changed, now switch logic
      if (stateChanged) {
        _state = !_state;
        _startTime = 0;
        if (_state == expectedPegel) {
          if (_type != GPIO_TYPE_SWITCH) {
            // Button handling
            if (!_supportLongPress) {
              // Button
              handleButtonOrSwitchPressed();
              _longPressStartTime = 0;
            } else {
              _longPressStartTime = now;
            }
          } else {
            // Switch handling
            handleButtonOrSwitchPressed();
            _longPressStartTime = 0;
          }
        } else {
          if (_type == GPIO_TYPE_SWITCH) {
            // Switch handling
            handleButtonOrSwitchPressed();
          } else if ((_supportLongPress) && (_longPressStartTime > 0) && (now - _longPressStartTime < SWICTH_LONG_PRESS_DURATION)) {
            // Long press button was released before long press time
            handleButtonOrSwitchPressed();
          }
          _longPressStartTime = 0;
        }
      } else {
        // 3. If state not changed, reset trigger and handle long press buttons
        setTriggerValue(false);
        // Long press time is up
        if ((_supportLongPress) && (_longPressStartTime > 0) && (now - _longPressStartTime >= SWICTH_LONG_PRESS_DURATION)) {
          handleLongButtonPressed();
          _longPressStartTime = 0;
        }
      }
    }
  }

  bool isLongPressing() {
    return ((_supportLongPress) && (_longPressStartTime > 0));
  }

  void handleButtonOrSwitchPressed() {
    setTriggerValue(true);
    if (property() != nullptr) {
      property()->asBool(!property()->asBool());
    }
    if (_onPressed) {
      _onPressed();
    }
  }

  void handleLongButtonPressed() {
    if (_onLongPressed) {
      _onLongPressed();
    }
  }

  void setOnPressed(THandlerFunction onPressed) {
    _onPressed = onPressed;
  }

  void setOnLongPressed(THandlerFunction onLongPressed) {
    _onLongPressed = onLongPressed;
  }

  void setTriggerProperty(WProperty* triggerProperty) {
    if (_triggerProperty != triggerProperty) {
      _triggerProperty = triggerProperty;
    }
  }

  bool hasTriggerProperty() {
    return (_triggerProperty != nullptr);
  }

 protected:
  bool _inverted;

  byte getOnLevel() {
    return (!_inverted ? SWITCH_PRESSED_PEGEL : !SWITCH_PRESSED_PEGEL);
  }

  byte getOffLevel() {
    return !getOnLevel();
  }

 private:
  THandlerFunction _onPressed;
  THandlerFunction _onLongPressed;
  bool _state, _lastState;
  bool _supportLongPress = false;
  unsigned long _startTime, _longPressStartTime;
  WProperty* _triggerProperty;

  void setTriggerValue(bool triggered) {
    if (_triggerProperty != nullptr) {
      _triggerProperty->asBool(triggered);
    }
  }
};

#endif
