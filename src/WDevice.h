#ifndef W_DEVICE_H
#define W_DEVICE_H

#include "WList.h"
#include "WGpio.h"
#include "hw/WLed.h"
#include "hw/WRelay.h"
#include "hw/WSwitch.h"
#include "hw/WPCF8575.h"
#include "hw/W2812.h"

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

  WProperty* getPropertyById(const char* propertyId) { return _properties->getById(propertyId); }

  virtual void toJsonValues(WJson* json, WPropertyVisibility visibility) {        
    _properties->forEach([this, json, visibility](int index, WProperty* property, const char* id) {    
      if (property->isVisible(visibility)) {
        property->toJsonValue(json, id);
      }
      property->changed(false);      
    });    
  }

  virtual void toJsonStructure(WJson* json, const char* deviceHRef, WPropertyVisibility visibility) {    
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
    if (_gpios != nullptr) {
      _gpios->forEach([this, now] (int index, WGpio* gpio, const char* id) { if (gpio->isInput()) gpio->loop(now); } );
      _gpios->forEach([this, now] (int index, WGpio* gpio, const char* id) { if (gpio->isOutput()) gpio->loop(now); } );
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

  WList<WProperty>* properties() { return _properties; }  

  virtual WDeepSleepMode deepSleepMode() { return DEEP_SLEEP_NONE; }

  virtual int deepSleepSeconds() { return 0; }

  virtual int deepSleepGPIO() { return NO_PIN; }

  WNetwork* network() { return _network; }

  unsigned long lastStateNotify() { return _lastStateNotify; }

  void lastStateNotify(unsigned long lastStateNotify) { _lastStateNotify = lastStateNotify; }

  unsigned long stateNotifyInterval() { return _stateNotifyInterval; }
  
  void stateNotifyInterval(unsigned long stateNotifyInterval) { _stateNotifyInterval = stateNotifyInterval; }

  bool needsWebThings() {
    return (_properties->getIf([] (WProperty* p) { return ((p->visibility() == WEBTHING) || (p->visibility() == ALL));}) != nullptr);
  }  

  WRelay* relay(int relayPin, bool inverted = false, IWExpander* expander = nullptr) {
    WRelay* relay = new WRelay(relayPin, inverted, expander);
    _addGpio(relay);
    return relay;
  }

  WSwitch* button(WGpioType gpioType = GPIO_TYPE_BUTTON, int switchPin = NO_PIN, bool inverted = false, IWExpander* expander = nullptr) {
    WSwitch* button = new WSwitch(gpioType, switchPin, inverted, expander);
    _addGpio(button);
    return button;
  }

  WPCF8575* expander(byte address, int sda = 21, int scl = 22, TwoWire* i2cPort = &Wire) {
    WPCF8575* exp = new WPCF8575(address, sda, scl, i2cPort);
    _addGpio(exp);
    return exp;
  }

  W2812Led* ledStripe(WGpioType gpioType = GPIO_TYPE_RGB_WS2812, int ledPin = NO_PIN, byte numberOfLeds = 0) {
    W2812Led* leds = new W2812Led(gpioType, ledPin, numberOfLeds);
    _addGpio(leds);
    return leds;
  }

 protected:  

  void _addGpio(WGpio* output) {
    if (_gpios == nullptr) {
      _gpios = new WList<WGpio>();
    }
    _gpios->add(output);
  }

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
  WList<WGpio>* _gpios = nullptr;  

  void onPropertyChange() { _lastStateNotify = 0; }
};

#endif
