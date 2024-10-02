#ifndef W_SWITCH_H_
#define W_SWITCH_H_

#include "WGpio.h"

#define BIT_CONFIG_SUPPORT_LONG_PRESS 6

const static char WC_LONG_PRESS[] PROGMEM = "longPress";

const unsigned long SWITCH_SENSITIVENESS = 200;
const unsigned long BUTTON_SENSITIVENESS = 20;
const unsigned long SWICTH_LONG_PRESS_DURATION = 5000;

class WSwitch : public WGpio {
 public:
  WSwitch(WGpioType gpioType = GPIO_TYPE_BUTTON, int switchPin = NO_PIN, bool inverted = false, IWExpander* expander = nullptr)
      : WGpio(gpioType, switchPin, (inverted ? INPUT_PULLUP : INPUT), expander) {
    _startTime = 0;
    this->inverted(inverted);
    _triggerProperty = nullptr;
    if (this->isInitialized()) {
      _state = readInput(switchPin);
      _lastState = _state;
      Serial.print("state is ");
      Serial.println(_state);
    } else {
      Serial.print("ni state is ");
      Serial.println(_state);
    }
  }

  void loop(unsigned long now) {
    if (this->isInitialized()) {
      // 1. Eliminate flickering input
      bool stateChanged = false;
      bool newState = readInput(pin());
      /*if (pin() == 0) {
      Serial.print(pin());
      Serial.print(" / ");
      Serial.print(newState);
      Serial.print(" / ");
      Serial.println(digitalRead(pin()));
      }*/
      

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
        if (pin() == 0) {
        

if (_type == GPIO_TYPE_SWITCH) {
        Serial.println("type is switch");
      } else {
        Serial.println("type is button");
      }
      Serial.print("!state is ");
        Serial.println(!_state);
        Serial.print("onLevel is ");
        Serial.println(getOnLevel());

        }
        _state = !_state;
        _startTime = 0;

        if (_state == expectedPegel) {
          Serial.println("a");
          if (_type != GPIO_TYPE_SWITCH) {
            // Button handling
            Serial.println("b");
            if (!supportLongPress()) {
              // Button
              Serial.println("c");
              handleButtonOrSwitchPressed();
              _longPressStartTime = 0;
            } else {
              Serial.println("d");
              _longPressStartTime = now;
            }
          } else {
            Serial.println("e");
            // Switch handling
            handleButtonOrSwitchPressed();
            _longPressStartTime = 0;
          }
        } else {
          Serial.println("f");
          if (_type == GPIO_TYPE_SWITCH) {
            Serial.println("g");
            // Switch handling
            handleButtonOrSwitchPressed();
          } else if ((supportLongPress()) && (_longPressStartTime > 0) && (now - _longPressStartTime < SWICTH_LONG_PRESS_DURATION)) {
            Serial.println("h");
            // Long press button was released before long press time
            handleButtonOrSwitchPressed();
          } else {
            Serial.println("i");
          }
          _longPressStartTime = 0;
        }
      } else {
        // 3. If state not changed, reset trigger and handle long press buttons
        setTriggerValue(false);
        // Long press time is up
        if ((supportLongPress()) && (_longPressStartTime > 0) && (now - _longPressStartTime >= SWICTH_LONG_PRESS_DURATION)) {
          handleLongButtonPressed();
          _longPressStartTime = 0;
        }
      }
    }
  }

  bool inverted() { return bitRead(_config->asByte(), BIT_CONFIG_INVERTED); }

  WSwitch* inverted(bool inverted) {
    _config->asBit(BIT_CONFIG_INVERTED, inverted);
    _onChange();
    return this;
  }

  bool supportLongPress() { return ((_longPressStartTime > 0) && (bitRead(_config->asByte(), BIT_CONFIG_SUPPORT_LONG_PRESS))); }

  WSwitch* supportLongPress(bool supportLongPress) {
    _config->asBit(BIT_CONFIG_SUPPORT_LONG_PRESS, supportLongPress);
    _onChange();
    return this;
  }

  void handleButtonOrSwitchPressed() {
    Serial.println("handleButtonOrSwitchPressed");
    setTriggerValue(true);
    if (property() != nullptr) {
      property()->asBool(!property()->asBool());
    }
    if (_onPressed) {
      _onPressed();
    }
  }

  void handleLongButtonPressed() {
    Serial.println("handle Long...");
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

  virtual void registerSettings() {
    WGpio::registerSettings();
    SETTINGS->add(_config, nullptr);   
    _onChange(); 
  }

  virtual void fromJson(WList<WValue>* list) {
    WGpio::fromJson(list);
    WValue* v = list->getById(WC_INVERTED);
    inverted(v != nullptr ? v->asBool() : false);    
    v = list->getById(WC_LONG_PRESS);
    supportLongPress(v != nullptr ? v->asBool() : false);    
  }

  virtual void toJson(WJson* json) {
    WGpio::toJson(json);    
    json->propertyBoolean(WC_INVERTED, inverted());
    json->propertyBoolean(WC_LONG_PRESS, supportLongPress());
  }

 protected:
  WValue* _config = new WValue((byte) 0b00000000);  

  byte getOnLevel() {
    return (!inverted() ? HIGH : LOW);
  }

  byte getOffLevel() {
    return !getOnLevel();
  }

 private:
  THandlerFunction _onPressed;
  THandlerFunction _onLongPressed;
  bool _state, _lastState;
  unsigned long _startTime;
  unsigned long _longPressStartTime = 4000;
  WProperty* _triggerProperty;

  void setTriggerValue(bool triggered) {
    if (_triggerProperty != nullptr) {
      _triggerProperty->asBool(triggered);
    }
  }
};

#endif
