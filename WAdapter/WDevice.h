#ifndef W_DEVICE_H
#define W_DEVICE_H

#include <ESPAsyncWebServer.h>
#include "WProperty.h"
#include "WLevelProperty.h"
#include "WLevelIntProperty.h"
#include "WColorProperty.h"
#include "WLed.h"

const char* DEVICE_TYPE_BINARY_SENSOR = "BinarySensor";
const char* DEVICE_TYPE_DOOR_SENSOR = "DoorSensor";
const char* DEVICE_TYPE_ON_OFF_SWITCH = "OnOffSwitch";
const char* DEVICE_TYPE_LIGHT = "Light";
const char* DEVICE_TYPE_TEMPERATURE_SENSOR = "TemperatureSensor";
const char* DEVICE_TYPE_THERMOSTAT = "Thermostat";
const char* DEVICE_TYPE_TEXT_DISPLAY = "TextDisplay";
const char* DEVICE_TYPE_MULTI_LEVEL_SENSOR = "MultiLevelSensor";
const char* DEVICE_TYPE_MULTI_LEVEL_SWITCH = "MultiLevelSwitch";

class WNetwork;

class WDevice {
public:

	WDevice(WNetwork* network, const char* id, const char* name, const char* type) {
		this->network = network;
		this->id = id;
		this->name = name;
		this->type = type;
		this->visibility = ALL;
		this->lastStateNotify = 0;
		this->stateNotifyInterval = 300000;
		this->mainDevice = true;
		this->lastStateWaitForResponse = false;
	}

	~WDevice() {
		//if (webSocket) delete webSocket;
	}

	const char* getId() {
		return id;
	}

	const char* getName() {
		return name;
	}

	const char* getType() {
		return type;
	}

	void addProperty(WProperty* property) {
		property->setDeviceNotification(std::bind(&WDevice::onPropertyChange, this));
		if (lastProperty == nullptr) {
			firstProperty = property;
			lastProperty = property;
		} else {
			lastProperty->next = property;
			lastProperty = property;
		}
	}

	void addPin(WPin* pin) {
		if (lastPin == nullptr) {
			firstPin = pin;
			lastPin = pin;
		} else {
			lastPin->next = pin;
			lastPin = pin;
		}
	}

	WProperty* getPropertyById(const char* propertyId) {
		WProperty* property = this->firstProperty;
		while (property != nullptr) {
			if (strcmp(property->getId(), propertyId) == 0) {
				return property;
			}
			property = property->next;
		}
		return nullptr;
	}

	virtual void toJsonValues(WJson* json, WPropertyVisibility visibility) {
		WProperty* property = this->firstProperty;
		while (property != nullptr) {
			if (property->isVisible(visibility)) {
				property->toJsonValue(json, false);
			}
			property->setUnChanged();
			property = property->next;
		}
	}

	virtual void toJsonStructure(WJson* json, const char* deviceHRef, WPropertyVisibility visibility) {
		json->beginObject();
		json->propertyString("id", this->getId());
		json->propertyString("title", this->getName());
		String result(deviceHRef);
		result.concat("/things/");
		result.concat(this->getId());
		json->propertyString("href", result.c_str());
		json->propertyString("@context", "https://iot.mozilla.org/schemas");
		//type
		json->beginArray("@type");
		json->string(getType());
		json->endArray();
		//properties
		json->beginObject("properties");
		WProperty* property = this->firstProperty;
		while (property != nullptr) {
			if (property->isVisible(visibility)) {
				property->toJsonStructure(json, property->getId(), result.c_str());
			}
			property = property->next;
		}
		json->endObject();
		json->endObject();
	}

    virtual void loop(unsigned long now) {
    	if (statusLed != nullptr) {
    		statusLed->loop(now);
    	}
    	/*if (webSocket != nullptr) {
    		webSocket->loop();
    	}*/
    	WPin* pin = this->firstPin;
    	while (pin != nullptr) {
    		pin->loop(now);
    		pin = pin->next;
    	}
    }

    virtual void bindWebServerCalls(AsyncWebServer* webServer) {
    }

    virtual void handleUnknownMqttCallback(bool getState, String completeTopic, String partialTopic, char *payload, unsigned int length) {

    }

    virtual WLed* getStatusLed() {
    	return this->statusLed;
    }

    virtual bool isDeviceStateComplete() {
        return true;
    }

		virtual bool sendCompleteDeviceState() {
			  return true;
		}

    virtual void on() {

    }

    virtual bool off() {

    }

    virtual bool areAllPropertiesRequested() {
    	WProperty* property = this->firstProperty;
    	while (property != nullptr) {
    		if (!property->isRequested()) {
    			return false;
    		}
    		property = property->next;
    	}
    	return true;
    }

    WPropertyVisibility getVisibility() {
    	return visibility;
    }

	void setVisibility(WPropertyVisibility visibility) {
		this->visibility = visibility;
	}

	bool isVisible(WPropertyVisibility visibility) {
		return ((this->visibility == ALL) || (this->visibility == visibility));
	}

	bool isMainDevice() {
		return mainDevice;
	}

	WDevice* next = nullptr;
  //WebSocketsServer* webSocket;
  WProperty* firstProperty = nullptr;
  WProperty* lastProperty = nullptr;
  WPin* firstPin = nullptr;
  WPin* lastPin = nullptr;
  bool lastStateWaitForResponse;
	unsigned long lastStateNotify;
	unsigned long stateNotifyInterval;
protected:
  WNetwork* network;
  WLed* statusLed = nullptr;
  bool mainDevice;
  WPropertyVisibility visibility;

private:
	const char* id;
	const char* name;
	const char* type;

	void onPropertyChange() {
		this->lastStateNotify = 0;
	}

};

#endif
