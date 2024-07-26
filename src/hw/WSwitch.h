#ifndef W_SWITCH_H_
#define W_SWITCH_H_

#include "WInput.h"

#define MODE_BUTTON 0
#define MODE_BUTTON_LONG_PRESS 1
#define MODE_SWITCH 2
#define SWITCH_PRESSED_PEGEL HIGH

const unsigned long SWITCH_SENSITIVENESS = 200;
const unsigned long BUTTON_SENSITIVENESS = 20;
const unsigned long SWICTH_LONG_PRESS_DURATION = 5000;

class WSwitch : public WInput {
 public:
  WSwitch(int switchPin, byte mode, bool inverted = false, IWExpander* expander = nullptr)
      : WInput(switchPin, (inverted ? INPUT_PULLUP : INPUT), expander) {
    _startTime = 0;
    _longPressStartTime = 0;
    _inverted = inverted;
    _triggerProperty = nullptr;
    _mode = mode;
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
      bool expectedPegel = (_mode == MODE_SWITCH ? !_state : getOnLevel());
      unsigned long sensitiveness = (_mode == MODE_SWITCH ? SWITCH_SENSITIVENESS : BUTTON_SENSITIVENESS);

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
          if (_mode != MODE_SWITCH) {
            // Button handling
            if (_mode == MODE_BUTTON) {
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
          if (_mode == MODE_SWITCH) {
            // Switch handling
            handleButtonOrSwitchPressed();
          } else if ((_mode == MODE_BUTTON_LONG_PRESS) && (_longPressStartTime > 0) && (now - _longPressStartTime < SWICTH_LONG_PRESS_DURATION)) {
            // Long press button was released before long press time
            handleButtonOrSwitchPressed();
          }
          _longPressStartTime = 0;
        }
      } else {
        // 3. If state not changed, reset trigger and handle long press buttons
        setTriggerValue(false);
        // Long press time is up
        if ((_mode == MODE_BUTTON_LONG_PRESS) && (_longPressStartTime > 0) && (now - _longPressStartTime >= SWICTH_LONG_PRESS_DURATION)) {
          handleLongButtonPressed();
          _longPressStartTime = 0;
        }
      }
    }
  }

  bool isLongPressing() {
    return ((_mode == MODE_BUTTON_LONG_PRESS) && (_longPressStartTime > 0));
  }

  void handleButtonOrSwitchPressed() {
    setTriggerValue(true);
    if (property() != nullptr) {
      property()->value()->asBool(!property()->value()->asBool());
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
  byte _mode;
  bool _state, _lastState;
  unsigned long _startTime, _longPressStartTime;
  WProperty* _triggerProperty;

  void setTriggerValue(bool triggered) {
    if (_triggerProperty != nullptr) {
      _triggerProperty->value()->asBool(triggered);
    }
  }
};

#endif
