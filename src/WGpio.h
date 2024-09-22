#ifndef W_GPIO_H
#define W_GPIO_H

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
  GPIO_TYPE_RGB_WS2812, GPIO_TYPE_RGB_PL9823, GPIO_TYPE_RGB_SINGLE,
  GPIO_TYPE_PWM, GPIO_TYPE_SERIAL_DIMMER,
  GPIO_TYPE_TPL0501,
  //Inputs
  GPIO_TYPE_BUTTON, GPIO_TYPE_SWITCH,  
  GPIO_TYPE_HTU21, GPIO_TYPE_SHT30, GPIO_TYPE_KY013,
  //In- and Output
  GPIO_TYPE_PCF8575,  
  //NONE
  GPIO_TYPE_UNKNOWN = 0xFF
};

const char S_GPIO_TYPE_LED[] PROGMEM = "led";
const char S_GPIO_TYPE_RELAY[] PROGMEM = "relay";
const char S_GPIO_TYPE_BUTTON[] PROGMEM = "button";
const char S_GPIO_TYPE_SWITCH[] PROGMEM = "switch";
const char S_GPIO_TYPE_MODE[] PROGMEM = "mode";
const char S_GPIO_TYPE_RGB_WS2812[] PROGMEM = "ws2812";
const char S_GPIO_TYPE_RGB_PL9823[] PROGMEM = "pl9823";
const char S_GPIO_TYPE_RGB_SINGLE[] PROGMEM = "rgb";
const char S_GPIO_TYPE_GROUP[] PROGMEM = "group";
const char S_GPIO_TYPE_HTU21[] PROGMEM = "htu21";
const char S_GPIO_TYPE_SHT30[] PROGMEM = "sht30";
const char S_GPIO_TYPE_KY013[] PROGMEM = "ky013";
const char S_GPIO_TYPE_PWM[] PROGMEM = "pwm";
const char S_GPIO_TYPE_SERIAL_DIMMER[] PROGMEM = "serialdimmer";
const char S_GPIO_TYPE_TPL0501[] PROGMEM = "tpl0501";
const char S_GPIO_TYPE_PCF8575[] PROGMEM = "pcf8575";
const char* const S_GPIO_TYPE[] PROGMEM = { S_GPIO_TYPE_GROUP, S_GPIO_TYPE_MODE,
                                            S_GPIO_TYPE_LED, S_GPIO_TYPE_RELAY, S_GPIO_TYPE_RGB_WS2812, S_GPIO_TYPE_RGB_PL9823, S_GPIO_TYPE_RGB_SINGLE,
                                            S_GPIO_TYPE_PWM, S_GPIO_TYPE_SERIAL_DIMMER, S_GPIO_TYPE_TPL0501,                                           
                                            S_GPIO_TYPE_BUTTON, S_GPIO_TYPE_SWITCH, S_GPIO_TYPE_HTU21, S_GPIO_TYPE_SHT30, S_GPIO_TYPE_KY013,
                                            S_GPIO_TYPE_PCF8575 };

class WGpio : public IWJsonable {
 public:
  typedef std::function<void()> THandlerFunction;
  WGpio(WGpioType type = GPIO_TYPE_UNKNOWN, byte pin = NO_PIN, byte mode = OUTPUT, IWExpander* expander = nullptr) {    
    _type = type;
    _mode = mode;
    _expander = expander;
    _isOn = false;
    this->pin(pin);
  }

  virtual ~WGpio() {
    if (_pin) delete _pin;
  }

  bool isOn() { return (_property != nullptr ? _property->asBool() : _isOn); }

  void setOn(bool isOn) {
    if ((_property == nullptr) && (isOn != _isOn)) {
      _isOn = isOn;
      _updateOn();      
    }
  }    

  virtual void loop(unsigned long now) {}

  WProperty* property() { return _property; }

	void property(WProperty* property) { 
    _property = property; 
    if (isInput()) {
      this->loop(millis());
    } else {
      _property->addListener([this]() { _updateOn();});
    }      
	}  

  bool hasProperty() { return (_property != nullptr); }

  bool readInput(uint8_t pin) {
    return (_expander == nullptr ? digitalRead(pin) : _expander->readInput(pin));
  } 

  void writeOutput(uint8_t pin, bool value) {
    if (_expander == nullptr) {
      digitalWrite(pin, value);
    } else {
      _expander->writeOutput(pin, value);
    } 
  }

  int pin() { return _pin->asByte(); }  

  virtual WGpio* pin(byte pin) {
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
    _settingsRegistered = true;
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
  bool isOutput() { return (((_type >= GPIO_TYPE_LED) && (_type < GPIO_TYPE_BUTTON)) || ((_type >= GPIO_TYPE_PCF8575) && (_type < GPIO_TYPE_UNKNOWN))); }
  bool isInput() { return ((_type >= GPIO_TYPE_BUTTON) && (_type < GPIO_TYPE_UNKNOWN)); }

 protected:
  WGpioType _type;
  WProperty* _property = nullptr;
  IWExpander* _expander;
  bool _settingsRegistered = false;

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

class WGroup : public WGpio {
 public:
  WGroup() : WGpio(GPIO_TYPE_GROUP) {
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
    if (_type != GPIO_TYPE_UNKNOWN) json->propertyString(WC_TYPE, S_GPIO_TYPE[_type], nullptr);
    json->property(WC_ID, _id);
    json->property(WC_TITLE, _title);
    _toJsonListOrMap(json);
  }

  virtual void addItem(WGpio* output, const char* id) {    
    if (_items == nullptr) _items = new WList<WGpio>();
    _items->add(output, id);
  }

  WValue* id() { return _id; }

  WValue* title() { return _title; }

 protected:
  WValue* _id = new WValue(STRING);
  WValue* _title = new WValue(STRING);
  WList<WGpio>* _items = nullptr;

  virtual void _updateOn() {
    if (_items != nullptr) {      
      _items->forEach([this] (int index, WGpio* output, const char* id) { output->setOn(this->isOn()); } ); 
    }
  }

  virtual void _toJsonListOrMap(WJson* json) {
    if (_items != nullptr) {
      json->beginArray(WC_ITEMS);
      _items->forEach([json] (int index, WGpio* item, const char* id) {
        json->beginObject(); 
        item->toJson(json); 
        json->endObject();
      });
      json->endArray();
    }
  }  

};

class WMode : public WGroup {
 public:
  WMode(bool storeLastState = true) : WGroup() {    
    _type = GPIO_TYPE_MODE;
    if (storeLastState) {
      _state = new WValue((byte) 0);
    }
  }

  virtual ~WMode() {
    if (_modeId) delete _modeId;
    if (_modeTitle) delete _modeTitle;
    if (_state) delete _state;
  }

  virtual void registerSettings() {
    WGroup::registerSettings();
    SETTINGS->add(_modeId, nullptr);
    SETTINGS->add(_modeTitle, nullptr);
    if (_state != nullptr) {
      SETTINGS->add(_state, nullptr);
    }
  }  

  virtual void fromJson(WList<WValue>* list) {
    WGroup::fromJson(list);
    list->ifExistsId(WC_MODE_ID, [this] (WValue* v) { this->_modeId->asString(v->asString()); });
    list->ifExistsId(WC_MODE_TITLE, [this] (WValue* v) { this->_modeTitle->asString(v->asString()); });
  }

  WValue* modeId() { return _modeId; }

  WValue* modeTitle() { return _modeTitle; }

  WProperty* modeProp() { return _modeProp; }

  void modeProp(WProperty* modeProp) { 
    _modeProp = modeProp; 
    if ((_items) && (_modeProp)) {
      _items->forEach([this] (int index, WGpio* gpio, const char* id) { this->_modeProp->addEnumString(id); } );
      if (_items->size() > 0) {
        byte index = 0;
        if (_state != nullptr) {
          index = min(_state->asByte(), (byte) (_items->size() - 1));
        }
        _modeProp->asString(_items->getId(index));        
      }
    }
    _modeProp->addListener([this]() { _updateOn();}); 
	}

 protected:  
  WProperty* _modeProp = nullptr;
  WValue* _modeId = new WValue(STRING);
  WValue* _modeTitle = new WValue(STRING); 
  WValue* _state = nullptr;

  virtual void _updateOn() {
    if (_items != nullptr) { 
      //switch all off     
      _items->forEach([this] (int index, WGpio* output, const char* id) { output->setOn(false); } );
      if ((this->isOn()) && (_modeProp != nullptr) && (!_modeProp->isStringEmpty())) {
        _items->ifExistsId(_modeProp->asString(), [this] (WGpio* gpio) { 
          gpio->setOn(true); 
          if (this->_state != nullptr) {
            this->_state->asByte(this->_items->indexOfId(this->_modeProp->asString()));
            SETTINGS->save();
          }
        });        
      } 
    }
  }

  virtual void _toJsonListOrMap(WJson* json) {
    if (_items != nullptr) {
      json->beginObject(WC_ITEMS);
      json->beginArray(WC_ITEMS);
      _items->forEach([json] (int index, WGpio* item, const char* id) {
        json->beginObject(id); 
        item->toJson(json); 
        json->endObject();
      });
      json->endObject();
    }
  }
  
};

class WLed;

#endif
