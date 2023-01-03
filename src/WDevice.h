#ifndef W_DEVICE_H
#define W_DEVICE_H

#include <ESPAsyncWebServer.h>

#include "WList.h"
#include "WColorProperty.h"
#include "WInput.h"
#include "WOutput.h"
#include "WLed.h"
#include "WLevelIntProperty.h"
#include "WLevelProperty.h"
#include "WProperty.h"

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

class WNetwork;

class WDevice {
 public:
  WDevice(WNetwork* network, const char* id, const char* name, const char* type,
          const char* alternativeType = nullptr) {
    this->network = network;
    this->id = id;
    this->name = name;
    this->type = type;
    this->alternativeType = alternativeType;
    this->visibility = ALL;
    this->lastStateNotify = 0;
    this->stateNotifyInterval = 300000;
    this->mainDevice = true;
    this->lastStateWaitForResponse = false;
    _properties = new WList<WProperty>();
  }

  ~WDevice() {
    // if (webSocket) delete webSocket;
  }

  const char* getId() { return id; }

  const char* getName() { return name; }

  const char* getType() { return type; }

  void addProperty(WProperty* property) {
    property->setDeviceNotification(std::bind(&WDevice::onPropertyChange, this));
    _properties->add(property);
  }

  void addInput(WInput* input) {
    if (this->inputs == nullptr) {
      this->inputs = new WList<WInput>();
    }
    this->inputs->add(input);
  }

  void addOutput(WOutput* output) {
    if (this->outputs == nullptr) {
      this->outputs = new WList<WOutput>();
    }
    this->outputs->add(output);
  }

  WProperty* getPropertyById(const char* propertyId) {
    return _properties->getIf([this, propertyId](WProperty* p){return p->equalsId(propertyId);});
  }

  virtual void toJsonValues(WJson* json, WPropertyVisibility visibility) {
    _properties->forEach([this, json, visibility](WProperty* property) {    
      if (property->isVisible(visibility)) {
        property->toJsonValue(json, false);
      }
      property->setUnChanged();      
    });
  }

  virtual void toJsonStructure(WJson* json, const char* deviceHRef,
                               WPropertyVisibility visibility) {
    json->beginObject();
    json->propertyString("id", this->getId());
    json->propertyString("title", this->getName());
    String result(deviceHRef);
    result.concat("/things/");
    result.concat(this->getId());
    json->propertyString("href", result.c_str());
    json->propertyString("@context", "https://iot.mozilla.org/schemas");
    // type
    json->beginArray("@type");
    json->string(getType());
    if (alternativeType != nullptr) {
      json->string(alternativeType);
    }
    json->endArray();
    // properties
    json->beginObject("properties");
    _properties->forEach([this, json, result, visibility](WProperty* property) {        
      if (property->isVisible(visibility)) {
        property->toJsonStructure(json, property->getId(), result.c_str());
      }      
    });
    json->endObject();
    json->endObject();
  }

  virtual void loop(unsigned long now) {
    if (this->inputs != nullptr) {
      this->inputs->forEach([this, now](WInput* input){input->loop(now);});
    }
    if (this->outputs != nullptr) {
      this->outputs->forEach([this, now](WOutput* output){output->loop(now);});
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
    return (_properties->getIf([this](WProperty* p){return (!p->isRequested());}) == nullptr);
  }

  WPropertyVisibility getVisibility() { return visibility; }

  void setVisibility(WPropertyVisibility visibility) {
    this->visibility = visibility;
  }

  bool isVisible(WPropertyVisibility visibility) {
    return ((this->visibility == ALL) || (this->visibility == visibility));
  }

  bool isMainDevice() { return mainDevice; }

  WList<WProperty>* properties() {
    return _properties;
  }

  AsyncWebSocket* webSocket = nullptr;  
  WList<WInput>* inputs = nullptr;
  WList<WOutput>* outputs = nullptr;
  bool lastStateWaitForResponse;
  unsigned long lastStateNotify;
  unsigned long stateNotifyInterval;

 protected:
  WNetwork* network;
  bool mainDevice;
  WPropertyVisibility visibility;
  WList<WProperty>* _properties;

 private:
  const char* id;
  const char* name;
  const char* type;
  const char* alternativeType;

  void onPropertyChange() { this->lastStateNotify = 0; }
};

#endif
