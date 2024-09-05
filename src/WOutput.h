#ifndef W_OUTPUT_H
#define W_OUTPUT_H

#include "WSettings.h"
#include "WProperty.h"
#include "IWExpander.h"

#define NO_PIN 0xFF
#define NO_MODE NO_PIN

#define BIT_CONFIG_INVERTED 5
#define BIT_CONFIG_LINKSTATE 6

//class WProperty;

class WOutput : public IWJsonable {
 public:
  WOutput(byte pin = NO_PIN, byte mode = OUTPUT, IWExpander* expander = nullptr) {    
    _mode = mode;
    _expander = expander;
    _isOn = false;
    this->pin(pin);
  }

  virtual ~WOutput() {
    if (_pin) delete _pin;
  }

  bool isOn() { return (_on != nullptr ? _on->asBool() : _isOn); }

  void setOn(bool isOn) {
    if ((_on == nullptr) && (isOn != _isOn)) {
      _isOn = isOn;
      _updateOn();      
    }
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

  virtual void registerSettings() {
    Serial.println("add pin");
    SETTINGS->add(_pin, nullptr); 
    pin(_pin->asByte());
  }  

  virtual void fromJson(WList<WValue>* list) {
    list->ifExistsId(WC_GPIO, [this] (WValue* v) { this->pin(v->asByte()); });
  }

  virtual void toJson(WJson* json) {
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
};

class WGroup : public WOutput {
 public:
  WGroup() : WOutput(NO_PIN, NO_MODE, nullptr) {
  }

  virtual ~WGroup() {
    if (_id) delete _id;
    if (_title) delete _title;
    if (_items) delete _items;
  }

  virtual void registerSettings() {
    //don't call super class
    SETTINGS->add(_id, nullptr);
    SETTINGS->add(_title, nullptr);
  }  

  virtual void fromJson(WList<WValue>* list) {
    //don't call super class
    list->ifExistsId(WC_ID, [this] (WValue* v) { this->_id->asString(v->asString()); });
    list->ifExistsId(WC_TITLE, [this] (WValue* v) { this->_title->asString(v->asString()); });
  }

  virtual void toJson(WJson* json) {
    //don't call super class
    json->property(WC_ID, _id);
    json->property(WC_TITLE, _title);
    if (_items != nullptr) {
      json->beginArray();
      _items->forEach([json] (int index, WOutput* item, const char* id) {
        json->beginObject(); 
        item->toJson(json); 
        json->endObject();
      });
      json->endArray();
    }
  }

  virtual void addItem(WOutput* output, const char* id) {
    if (_items == nullptr) _items = new WList<WOutput>();
    _items->add(output, id);
  }

  WValue* id() { return _id; }

  WValue* title() { return _title; }

 protected:
  WValue* _id = new WValue(STRING);
  WValue* _title = new WValue(STRING);
  WList<WOutput>* _items = nullptr;

};

class WMode : public WGroup {
 public:
  WMode() : WGroup() {
  }

  virtual ~WMode() {
    if (_modeId) delete _modeId;
    if (_modeTitle) delete _modeTitle;
  }

  virtual void registerSettings() {
    WGroup::registerSettings();
    SETTINGS->add(_modeId, nullptr);
    SETTINGS->add(_modeTitle, nullptr);
  }  

  virtual void fromJson(WList<WValue>* list) {
    WGroup::fromJson(list);
    list->ifExistsId(WC_MODE_ID, [this] (WValue* v) { this->_modeId->asString(v->asString()); });
    list->ifExistsId(WC_MODE_TITLE, [this] (WValue* v) { this->_modeTitle->asString(v->asString()); });
  }

  WValue* modeId() { return _modeId; }

  WValue* modeTitle() { return _modeTitle; }

 protected:
  WValue* _modeId = new WValue(STRING);
  WValue* _modeTitle = new WValue(STRING); 
  
};




#endif
