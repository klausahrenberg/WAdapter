#ifndef W_OUTPUT_H
#define W_OUTPUT_H

#include "WSettings.h"
#include "WProperty.h"
#include "IWExpander.h"

#define NO_PIN 0xFF
#define NO_MODE NO_PIN

class WProperty;

class WOutput : public IWStorable {
 public:
  WOutput(byte pin = NO_PIN, byte mode = OUTPUT, IWExpander* expander = nullptr) {    
    _mode = mode;
    _expander = expander;
    _id = nullptr;
    _isOn = false;
    this->pin(pin);
  }

  bool isOn() { return (_on != nullptr ? _on->asBool() : _isOn); }

  void setOn(bool isOn) {
    if ((_on == nullptr) && (isOn != _isOn)) {
      _isOn = isOn;
      _updateOn();      
    }
  }    

  const char* id() { return _id; }

  WOutput* id(const char* id) {
    if (_id != nullptr) {
      delete _id;
      _id = nullptr;
    }  
    if (id != nullptr) {
      _id = new char[strlen(id) + 1];
      strcpy(_id, id);
    }  
    return this;
  }

  bool equalsId(const char* id) {
    return ((id != nullptr) && (_id != nullptr) && (strcmp(_id, id) == 0));
  }

  virtual void loop(unsigned long now) {}

  virtual byte countModes() { return 0;}

  virtual const char* modeTitle(byte index) { return ""; }

  virtual byte mode() { return 0;}

  virtual void setMode(byte index) {}

  virtual void setModeByTitle(const char* title) {
    for (byte b = 0; b < countModes(); b++) {
      if (strcmp(modeTitle(b), title) == 0) {
        setMode(b);
        break;
      }
    }    
  }

  WProperty* on() { return _on; }

	void on(WProperty* on) { 
		_on = on; 
		_on->addListener([this]() { _updateOn();});
	}  

  void writeOutput(uint8_t pin, bool value) {
    if (_expander == nullptr) {
      digitalWrite(pin, value);
    } else {
      _expander->writeOutput(pin, value);
    } 
  }

  int pin() { return _pin; }  

  virtual WOutput* pin(byte pin) { 
    if (_pin != pin) {
      _pin = pin;
      if ((_pin != NO_PIN) && (_mode != NO_MODE)) {
        _pinMode(_pin, _mode);      
      }
      _pinChanged();
    }
    return this; 
  }  

  virtual void loadFromStore() {
    Serial.println("load output parameters");   
    WValue* gpio = SETTINGS->setByte(nullptr, NO_PIN);
    Serial.println(gpio->asByte()); 
    pin(gpio->asByte());
    WValue* idx = SETTINGS->setString(nullptr, nullptr);
    Serial.println(idx->asString()); 
    id(idx->asString());
  }  

  virtual void writeToStore() {
    
  }

 protected:
  WProperty* _on = nullptr;
  IWExpander* _expander;

  virtual bool isInitialized() { return (_pin != NO_PIN); }  

  void _pinMode(uint8_t pin, uint8_t mode) {
    (_expander == nullptr ? pinMode(pin, mode) : _expander->mode(pin, mode));
  }

  virtual void _updateOn() {}

  virtual void _pinChanged() {}

 private:  
  byte _pin = NO_PIN;
  byte _mode = NO_MODE;
  bool _isOn;
  char* _id;
};

#endif
