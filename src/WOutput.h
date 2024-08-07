#ifndef W_OUTPUT_H
#define W_OUTPUT_H

#include "WProperty.h"
#include "IWExpander.h"

const int NO_PIN = -1;
const int NO_MODE = -1;

class WProperty;

class WOutput {
 public:
  WOutput(int pin, uint8_t mode = OUTPUT, IWExpander* expander = nullptr) {
    _pin = pin;
    _expander = expander;
    _id = nullptr;
    _isOn = false;
    if ((_pin != NO_PIN) && (mode != NO_MODE)) {
      _pinMode(_pin, mode);      
    }
  }

  bool isOn() { return (_on != nullptr ? _on->asBool() : _isOn); }

  void setOn(bool isOn) {
    if ((_on == nullptr) && (isOn != _isOn)) {
      _isOn = isOn;
      _updateOn();      
    }
  }    

  const char* id() { return _id; }

  void setId(const char* id) {
    if (_id != nullptr) {
      delete _id;
      _id = nullptr;
    }  
    if (id != nullptr) {
      _id = new char[strlen(id) + 1];
      strcpy(_id, id);
    }  
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

 protected:
  WProperty* _on = nullptr;
  IWExpander* _expander;

  virtual bool isInitialized() { return (_pin != NO_PIN); }

  int pin() { return _pin; }  


  void _pinMode(uint8_t pin, uint8_t mode) {
    (_expander == nullptr ? pinMode(pin, mode) : _expander->mode(pin, mode));
  }

  virtual void _updateOn() {

  };

 private:  
  int _pin;
  bool _isOn;
  char* _id;
};

#endif
