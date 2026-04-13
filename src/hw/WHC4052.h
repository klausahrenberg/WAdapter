#ifndef W_HC4052_h
#define W_HC4052_h

#include "../WGpio.h"

class WHC4052 : public WGpio {
 public:
  typedef std::function<byte()> TIndexCondition;
  WHC4052(byte pinEnable = NO_PIN, byte pinS0 = NO_PIN, byte pinS1 = NO_PIN, IWExpander* expander = nullptr)
      : WGpio(GPIO_TYPE_LED, pinEnable, OUTPUT, expander) {
    _s0 = pinS0;
    _s1 = pinS1;
    if (_s0 != NO_PIN) mode(_s0, OUTPUT);
    if (_s1 != NO_PIN) mode(_s1, OUTPUT);
    index(0);
  }

  static WHC4052* create(IWGpioRegister* device, byte pinEnable = NO_PIN, byte pinS0 = NO_PIN, byte pinS1 = NO_PIN, IWExpander* expander = nullptr) {
    WHC4052* multiplexer = new WHC4052(pinEnable, pinS0, pinS1, expander);
    device->registerGpio(multiplexer);
    return multiplexer;
  }

  WHC4052* indexCondition(TIndexCondition indexCondition) {
    _indexCondition = indexCondition;
    return this;
  }

  virtual void loop(unsigned long now) {
    if (_indexCondition) {
      index(_indexCondition());
    }
    WGpio::loop(now);
  }

  void index(byte index) {
    if (_index != index) {      
      _index = min(index, (byte) 3);
      _updateOn();
    }
  }

  byte indexMappingValue(byte idx) const {
    if (idx >= 4) return 0;
    return (_indexMapping >> (idx * 2)) & 0b11;
  }
  
  WHC4052* indexMappingValue(byte idx, byte value) {
    if (idx >= 4) return this;
    byte shift = idx * 2;
    _indexMapping = (_indexMapping & ~(0b11 << shift)) | ((value & 0b11) << shift);
    _updateOn();  
    return this;
  }
  
  WHC4052* indexMapping(const byte mapping[4]) {
    for (byte i = 0; i < 4; i++) {
      indexMappingValue(i, mapping[i]);
    }
    return this;
  }

 protected:
  virtual bool _isInitialized() {
    return ((WGpio::_isInitialized()) && (_s0 != NO_PIN) && (_s1 != NO_PIN));
  }

  virtual void _updateOn() {
    WGpio::_updateOn();
    if (_isInitialized()) {
      writeOutput(pin(), HIGH);
      if (isOn()) {
        writeOutput(_s0, bitRead(_indexMapping, 2 * _index));
        writeOutput(_s1, bitRead(_indexMapping, 2 * _index + 1));
        writeOutput(pin(), LOW);
      }
    }
  };

 private:
  byte _s0;
  byte _s1;
  byte _indexMapping = 0b11100100;  // Index0:00, Index1:01, Index2:10, Index3:11;
  byte _index;
  TIndexCondition _indexCondition = nullptr;
};

#endif