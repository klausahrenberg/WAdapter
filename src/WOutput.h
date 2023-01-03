#ifndef W_OUTPUT_H
#define W_OUTPUT_H

const int NO_PIN = -1;
const int NO_MODE = -1;

class WProperty;

class WOutput {
 public:
  WOutput(int pin) {
    _pin = pin;
    _id = nullptr;
    _isOn = false;
    if (_pin != NO_PIN) {
      pinMode(_pin, OUTPUT);
    }
  }

  bool isOn() { return _isOn; }

  void setOn(bool isOn) {
    if (isOn != _isOn) {
      _isOn = isOn;
      this->onChanged();      
    }
  }  

  void on() { setOn(true); }

  void off() { setOn(false); }

  void toggle() { setOn(isOn() ? false : true); }

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

  virtual void handleChangedProperty(WValue value) {    
    this->setOn(value.asBoolean);
  }

  virtual void loop(unsigned long now) {}

 protected:
  virtual bool isInitialized() { return (_pin != NO_PIN); }

  int pin() { return _pin; }  

  virtual void onChanged() {};  

 private:
  int _pin;
  bool _isOn;
  char* _id;
};

#endif
