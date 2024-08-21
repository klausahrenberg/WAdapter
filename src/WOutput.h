#ifndef W_OUTPUT_H
#define W_OUTPUT_H

#include "WSettings.h"
#include "WProperty.h"
#include "IWExpander.h"

#define NO_PIN 0xFF
#define NO_MODE NO_PIN

//class WProperty;

class WOutput : public IWStorable, public IWJsonable {
 public:
  WOutput(byte pin = NO_PIN, byte mode = OUTPUT, IWExpander* expander = nullptr) {    
    _mode = mode;
    _expander = expander;
    _id = nullptr;
    _isOn = false;
    this->pin(pin);
  }

  virtual ~WOutput() {
    if (_pin) delete _pin;
    if (_id) delete _id;
  }

  bool isOn() { return (_on != nullptr ? _on->asBool() : _isOn); }

  void setOn(bool isOn) {
    if ((_on == nullptr) && (isOn != _isOn)) {
      _isOn = isOn;
      _updateOn();      
    }
  }    

  const char* id() { return (_id != nullptr ? _id->asString() : nullptr); }

  WOutput* id(const char* id) {
    if ((_id == nullptr) && (id != nullptr)) _id = new WValue(STRING);
    _id->asString(id);
    return this;
  }

  bool equalsId(const char* id) {
    return ((id != nullptr) && (_id != nullptr) && (_id->equalsString(id)));
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
    this->_on = on; 
    _on->addListener([this]() { _updateOn();});    
	}  

  void writeOutput(uint8_t pin, bool value) {
    if (_expander == nullptr) {
      digitalWrite(pin, value);
    } else {
      _expander->writeOutput(pin, value);
    } 
  }

  int pin() { return _pin->asByte(); }  

  virtual WOutput* pin(byte pin) {
    bool changed = (_pin->asByte() != pin); 
    _pin->asByte(pin);
    if ((_pin->asByte() != NO_PIN) && (_mode != NO_MODE)) {
      _pinMode(_pin->asByte(), _mode);      
    }
    if (changed) {  
      _pinChanged();
    }
    return this; 
  }  

  virtual void loadFromStore() {
    Serial.println("add pin");
    SETTINGS->add(_pin, nullptr); 
    pin(_pin->asByte());
    if (_id == nullptr) _id = new WValue(STRING);
    SETTINGS->add(_id, nullptr);
  }  

  virtual void writeToStore() {
    
  }

  virtual void loadFromJson(WList<WValue>* list) {
    WValue* gpio = list->getById(WC_GPIO);
    Serial.print("id in json is ");
    Serial.println(gpio != nullptr ? gpio->asByte() : 23 );
    pin(gpio != nullptr ? gpio->asByte() : NO_PIN);
    //SETTINGS->setByte(nullptr, pin());
    WValue* idx = list->getById(WC_ID);
    Serial.print("id in json is ");
    Serial.println(idx != nullptr ? idx->asString() : "n.a." );
    
    id(idx != nullptr ? idx->asString() : nullptr);
    Serial.println(id() != nullptr ? id() : "n.a." );
    //SETTINGS->setString(nullptr, id());
  }

  virtual void toJson(WJson* json) {
    Serial.print("id is ");
    Serial.println(id() != nullptr ? id() : "n.a." );
    json->propertyString(WC_ID, id(), nullptr);
    if (pin() != NO_PIN) {
      json->propertyByte(WC_GPIO, pin());
    } else {
      json->propertyNull(WC_GPIO);
    }
  }

 protected:
  WProperty* _on = nullptr;
  IWExpander* _expander;

  virtual bool isInitialized() { return (pin() != NO_PIN); }  

  void _pinMode(uint8_t pin, uint8_t mode) {
    (_expander == nullptr ? pinMode(pin, mode) : _expander->mode(pin, mode));
  }

  virtual void _updateOn() {}

  virtual void _pinChanged() {}

 private:  
  WValue* _pin = new WValue((byte) NO_PIN);
  byte _mode = NO_MODE;
  bool _isOn;
  WValue* _id = nullptr;
};

#endif
