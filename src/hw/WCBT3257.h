#ifndef W_CBT3257_h
#define W_CBT3257_h

#include "../WGpio.h"

class WCBT3257 : public WGpio {
 public:
  typedef std::function<byte()> TIndexCondition;
  WCBT3257(byte pinEnable = NO_PIN, byte pinSelect = NO_PIN, IWExpander* expander = nullptr)
      : WGpio(GPIO_TYPE_LED, pinEnable, OUTPUT, expander) {
    _pinSelect = pinSelect;
    if (_pinSelect != NO_PIN) mode(_pinSelect, OUTPUT);
    index(0);
  }

  static WCBT3257* create(IWGpioRegister* device, byte pinEnable = NO_PIN, byte pinSelect = NO_PIN, IWExpander* expander = nullptr) {
    WCBT3257* multiplexer = new WCBT3257(pinEnable, pinSelect, expander);
    device->registerGpio(multiplexer);
    return multiplexer;
  }

  WCBT3257* indexCondition(TIndexCondition indexCondition) {
    _indexCondition = indexCondition;
    return this;
  }

  virtual void loop(unsigned long now) {
    if (_indexCondition) {
      index(_indexCondition());
    }
    WGpio::loop(now);
  }

  bool index() { return _index; }

  void index(bool index) {
    if (_index != index) {      
      _index = index;
      _updateOn();
    }
  }

 protected:
  virtual bool _isInitialized() {
    return ((WGpio::_isInitialized()) && (_pinSelect != NO_PIN));
  }

  virtual void _updateOn() {
    WGpio::_updateOn();
    if (_isInitialized()) {
      writeOutput(pin(), HIGH);
      if (isOn()) {
        writeOutput(_pinSelect, _index);    
        writeOutput(pin(), LOW);
      }
    }
  };

 private:
  byte _pinSelect;
  bool _index;
  TIndexCondition _indexCondition = nullptr;
};

#endif