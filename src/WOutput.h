#ifndef W_OUTPUT_H
#define W_OUTPUT_H

#include "WSettings.h"
#include "WProperty.h"
#include "IWExpander.h"

#define NO_PIN 0xFF
#define NO_MODE NO_PIN

#define BIT_CONFIG_INVERTED 5
#define BIT_CONFIG_LINKSTATE 6

enum WGpioType {
  //Group  
  GPIO_TYPE_GROUP,
  //Mode
  GPIO_TYPE_MODE,
  //Outputs
  GPIO_TYPE_LED, GPIO_TYPE_RELAY,
  GPIO_TYPE_RGB_LED, GPIO_TYPE_DIMMER,
  //Inputs
  GPIO_TYPE_BUTTON, GPIO_TYPE_SWITCH,  
  GPIO_TYPE_TEMP_SENSOR,
  //NONE
  GPIO_TYPE_UNKNOWN = 0xFF
};

const char S_GPIO_TYPE_LED[] PROGMEM = "led";
const char S_GPIO_TYPE_RELAY[] PROGMEM = "relay";
const char S_GPIO_TYPE_BUTTON[] PROGMEM = "button";
const char S_GPIO_TYPE_SWITCH[] PROGMEM = "switch";
const char S_GPIO_TYPE_MODE[] PROGMEM = "mode";
const char S_GPIO_TYPE_RGB_LED[] PROGMEM = "rgb";
const char S_GPIO_TYPE_GROUP[] PROGMEM = "group";
const char S_GPIO_TYPE_TEMP_SENSOR[] PROGMEM = "temp";
const char S_GPIO_TYPE_DIMMER[] PROGMEM = "dimmer";
const char* const S_GPIO_TYPE[] PROGMEM = { S_GPIO_TYPE_GROUP, S_GPIO_TYPE_MODE,
                                            S_GPIO_TYPE_LED, S_GPIO_TYPE_RELAY, S_GPIO_TYPE_RGB_LED, S_GPIO_TYPE_DIMMER, 
                                            S_GPIO_TYPE_BUTTON, S_GPIO_TYPE_SWITCH, S_GPIO_TYPE_TEMP_SENSOR };

class WOutput : public IWJsonable {
 public:
  WOutput(WGpioType type = GPIO_TYPE_UNKNOWN, byte pin = NO_PIN, byte mode = OUTPUT, IWExpander* expander = nullptr) {    
    _type = type;
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

  int pin() { return _pin->asByte(); }  

  virtual WOutput* pin(byte pin) {
    bool changed = (_pin->asByte() != pin); 
    _pin->asByte(pin);
    if ((_pin->asByte() != NO_PIN) && (_mode != NO_MODE)) {
      _pinMode(_pin->asByte(), _mode);      
    }
    if (changed) {  
      _onChange();
    }
    return this; 
  }  

  virtual void registerSettings() {
    SETTINGS->add(_pin, nullptr); 
    pin(_pin->asByte());
  }  

  virtual void fromJson(WList<WValue>* list) {
    list->ifExistsId(WC_GPIO, [this] (WValue* v) { this->pin(v->asByte()); });
  }

  virtual void toJson(WJson* json) {
    if (_type != GPIO_TYPE_UNKNOWN) json->propertyString(WC_TYPE, S_GPIO_TYPE[_type], nullptr);
    if (pin() != NO_PIN) {
      json->propertyByte(WC_GPIO, pin());
    } else {
      json->propertyNull(WC_GPIO);
    }
  }

  WGpioType type() { return _type; }

  bool isGroupOrMode() { return ((_type == GPIO_TYPE_GROUP) || (_type == GPIO_TYPE_MODE)); }
  bool isOutput() { return ((_type >= GPIO_TYPE_LED) && (_type < GPIO_TYPE_BUTTON)); }
  bool isInput() { return ((_type >= GPIO_TYPE_BUTTON) && (_type < GPIO_TYPE_UNKNOWN)); }

 protected:
  WGpioType _type;
  WProperty* _on = nullptr;
  IWExpander* _expander;

  virtual bool isInitialized() { return (pin() != NO_PIN); }  

  void _pinMode(uint8_t pin, uint8_t mode) {
    (_expander == nullptr ? pinMode(pin, mode) : _expander->mode(pin, mode));
  }

  virtual void _updateOn() {}

  virtual void _onChange() {}

 private:    
  WValue* _pin = new WValue((byte) NO_PIN);
  byte _mode = NO_MODE;
  bool _isOn;
};

class WGroup : public WOutput {
 public:
  WGroup() : WOutput(GPIO_TYPE_GROUP) {
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
    Serial.println("add item in group");
    if (_items == nullptr) _items = new WList<WOutput>();
    _items->add(output, id);
  }

  WValue* id() { return _id; }

  WValue* title() { return _title; }

 protected:
  WValue* _id = new WValue(STRING);
  WValue* _title = new WValue(STRING);
  WList<WOutput>* _items = nullptr;

  virtual void _updateOn() {
    Serial.println("ipdateOn");
    if (_items != nullptr) {
      Serial.println(_items->size());
      _items->forEach([this] (int index, WOutput* output, const char* id) { 
        Serial.println(output->pin());
        output->setOn(this->isOn()); } );
    }
  }

};

class WMode : public WGroup {
 public:
  WMode() : WGroup() {
    _type = GPIO_TYPE_MODE;
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

class WLed;

#endif
