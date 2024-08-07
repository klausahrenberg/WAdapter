#ifndef W_DEVICE_H
#define W_DEVICE_H

#include "WList.h"
#include "WInput.h"
#include "WOutput.h"
#include "hw/WLed.h"
#include "WProps.h"

const char* DEVICE_TYPE_BINARY_SENSOR = "BinarySensor";
const char* DEVICE_TYPE_DOOR_SENSOR = "DoorSensor";
const char* DEVICE_TYPE_ON_OFF_SWITCH = "OnOffSwitch";
const char* DEVICE_TYPE_LIGHT = "Light";
const char* DEVICE_TYPE_TEMPERATURE_SENSOR = "TemperatureSensor";
const char* DEVICE_TYPE_THERMOSTAT = "Thermostat";
const char* DEVICE_TYPE_TEXT_DISPLAY = "TextDisplay";
const char* DEVICE_TYPE_MULTI_LEVEL_SENSOR = "MultiLevelSensor";
const char* DEVICE_TYPE_MULTI_LEVEL_SWITCH = "MultiLevelSwitch";
const char* DEVICE_TYPE_RADIO = "Radio";

enum WDeepSleepMode {
  DEEP_SLEEP_NONE,
  DEEP_SLEEP_SECONDS,
  DEEP_SLEEP_GPIO_LOW,
  DEEP_SLEEP_GPIO_HIGH
};

class WNetwork;

class WDevice {
 public:
  WDevice(WNetwork* network, const char* id, const char* title, const char* type,
          const char* alternativeType = nullptr) {
    _network = network;
    _id = id;
    _title = title;
    _type = type;
    _alternativeType = alternativeType;
    _visibility = ALL;
    _lastStateNotify = 0;
    _stateNotifyInterval = 300000;
    _mainDevice = true;
    _lastStateWaitForResponse = false;
    _properties = new WList<WProperty>();
  }

  const char* id() { return _id; }

  const char* title() { return _title; }

  const char* type() { return _type; }

  void addProperty(WProperty* property, const char* id) {
    property->deviceNotification(std::bind(&WDevice::onPropertyChange, this));
    _properties->add(property, id);
  }

  void addInput(WInput* input) {
    if (_inputs == nullptr) {
      _inputs = new WList<WInput>();
    }
    _inputs->add(input);
  }

  void addOutput(WOutput* output) {
    if (_outputs == nullptr) {
      _outputs = new WList<WOutput>();
    }
    _outputs->add(output);
  }

  WProperty* getPropertyById(const char* propertyId) { return _properties->getById(propertyId); }

  virtual void toJsonValues(WJson* json, WPropertyVisibility visibility) {    
    _properties->forEach([this, json, visibility](int index, WProperty* property, const char* id) {    
      if (property->isVisible(visibility)) {
        property->toJsonValue(json, id);
      }
      property->changed(false);      
    });
  }

  virtual void toJsonStructure(WJson* json, const char* deviceHRef,
                               WPropertyVisibility visibility) {
    json->beginObject();
    json->propertyString("id", this->id(), nullptr);
    json->propertyString("title", this->title(), nullptr);
    String result(deviceHRef);
    result.concat("/things/");
    result.concat(this->id());
    json->propertyString("href", result.c_str(), nullptr);
    json->propertyString("@context", "https://iot.mozilla.org/schemas", nullptr);
    // type
    json->beginArray("@type");
    json->string(type(), nullptr);
    if (_alternativeType != nullptr) {
      json->string(_alternativeType, nullptr);
    }
    json->endArray();
    // properties
    json->beginObject("properties");
    _properties->forEach([this, json, result, visibility](int index, WProperty* property, const char* id) {        
      if (property->isVisible(visibility)) {
        property->toJsonStructure(json, id, result.c_str());
      }      
    });
    json->endObject();
    json->endObject();
  }

  virtual void loop(unsigned long now) {
    if (_inputs != nullptr) {
      _inputs->forEach([this, now](int index, WInput* input, const char* id){input->loop(now);});
    }
    if (_outputs != nullptr) {
      _outputs->forEach([this, now](int index, WOutput* output, const char* id){output->loop(now);});
    }  
  }

  virtual void bindWebServerCalls(AsyncWebServer* webServer) {}

  virtual void handleUnknownMqttCallback(bool getState, String completeTopic,
                                         String partialTopic, char* payload,
                                         unsigned int length) {}

  virtual bool isDeviceStateComplete() { return true; }

  virtual bool sendCompleteDeviceState() { return true; }

  virtual void on() {}

  virtual bool off() { return false; }

  virtual bool areAllPropertiesRequested() {    
    return (_properties->getIf([this](WProperty* p){return (!p->requested());}) == nullptr);
  }

  WPropertyVisibility visibility() { return _visibility; }

  void setVisibility(WPropertyVisibility visibility) {
    _visibility = visibility;
  }

  bool isVisible(WPropertyVisibility visibility) {
    return ((_visibility == ALL) || (_visibility == visibility));
  }

  bool isMainDevice() { return _mainDevice; }

  void setMainDevice(bool mainDevice) { _mainDevice = mainDevice; }

  WList<WProperty>* properties() {
    return _properties;
  }  

  virtual WDeepSleepMode deepSleepMode() { return DEEP_SLEEP_NONE; }

  virtual int deepSleepSeconds() { return 0; }

  virtual int deepSleepGPIO() { return NO_PIN; }

  WNetwork* network() { return _network; }

  unsigned long lastStateNotify() { return _lastStateNotify; }

  void lastStateNotify(unsigned long lastStateNotify) { _lastStateNotify = lastStateNotify; }

  unsigned long stateNotifyInterval() { return _stateNotifyInterval; }
  
  void stateNotifyInterval(unsigned long stateNotifyInterval) { _stateNotifyInterval = stateNotifyInterval; }

  bool needsWebThings() {
    bool result = false;
    WIterator<WProperty>* it_p = _properties->iterator();
    while ((!result) && (it_p->hasNext())) {
      WProperty* p = it_p->next();
      result = ((p->visibility() == WEBTHING) || (p->visibility() == ALL));
    } 
    return result;
  }  

 protected:  

 private:
  WNetwork* _network;
  bool _mainDevice;
  WPropertyVisibility _visibility;
  WList<WProperty>* _properties;
  const char* _id;
  const char* _title;
  const char* _type;
  const char* _alternativeType;
  unsigned long _lastStateNotify;
  unsigned long _stateNotifyInterval;
  bool _lastStateWaitForResponse;
  WList<WInput>* _inputs = nullptr;
  WList<WOutput>* _outputs = nullptr;  

  void onPropertyChange() { _lastStateNotify = 0; }
};

#endif
