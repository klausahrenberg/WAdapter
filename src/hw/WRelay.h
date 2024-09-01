#ifndef W_RELAY_H
#define W_RELAY_H

#include "WOutput.h"

class WRelay : public WOutput {
 public:
  WRelay(int relayPin, bool inverted = false) : WOutput(relayPin) {
    this->inverted(inverted);
    if (this->isInitialized()) {
      digitalWrite(this->pin(), getOffLevel());
    }
  }  

  void loop(unsigned long now) {}

  bool inverted() { return bitRead(_config->asByte(), BIT_CONFIG_INVERTED); }

  WRelay* inverted(bool inverted) {
    _config->asBit(BIT_CONFIG_INVERTED, inverted);
    return this;
  }

  virtual void registerSettings() {
    WOutput::registerSettings();
    SETTINGS->add(_config, nullptr);    
  }

  virtual void fromJson(WList<WValue>* list) {
    WOutput::fromJson(list);
    WValue* v = list->getById(WC_INVERTED);
    inverted(v != nullptr ? v->asBool() : false);    
  }

  virtual void toJson(WJson* json) {
    WOutput::toJson(json);    
    json->propertyBoolean(WC_INVERTED, inverted());
  }

 protected:
  void _updateOn() {
    digitalWrite(this->pin(), isOn() ? getOnLevel() : getOffLevel());
  };

  byte getOnLevel() { return (!inverted() ? HIGH : LOW); }

  byte getOffLevel() { return !getOnLevel(); }

 private:
  WValue* _config = new WValue((byte) 0b00000000);

};

#endif
