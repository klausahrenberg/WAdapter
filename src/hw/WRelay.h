#ifndef W_RELAY_H
#define W_RELAY_H

#include "WGpio.h"

class WRelay : public WGpio {
 public:
  WRelay(int relayPin, bool inverted = false, IWExpander* expander = nullptr) : WGpio(GPIO_TYPE_RELAY, relayPin, OUTPUT, expander) {
    this->inverted(inverted);
  }

  bool inverted() { return bitRead(_config->asByte(), BIT_CONFIG_INVERTED); }

  WRelay* inverted(bool inverted) {
    _config->asBit(BIT_CONFIG_INVERTED, inverted);
    _onChange();
    return this;
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
  }

  virtual void toJson(WJson* json) {
    WGpio::toJson(json);
    json->propertyBoolean(WC_INVERTED, inverted());
  }

 protected:
  void _updateOn() {
    writeOutput(this->pin(), isOn() ? getOnLevel() : getOffLevel());
  };

  byte getOnLevel() { return (!inverted() ? HIGH : LOW); }

  byte getOffLevel() { return !getOnLevel(); }

  virtual void _onChange() {
    if (isInitialized()) {
      writeOutput(pin(), getOffLevel());
    }
  }

 private:
  WValue* _config = new WValue((byte)0b00000000);
};

#endif
