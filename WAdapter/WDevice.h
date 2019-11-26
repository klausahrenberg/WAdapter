#ifndef W_DEVICE_H
#define W_DEVICE_H

#include "ESP8266WebServer.h"
#include "WProperty.h"
#include "WLevelProperty.h"
#include "WOnOffProperty.h"
#include "WStringProperty.h"
#include "WIntegerProperty.h"
#include "WLongProperty.h"
#include "WTemperatureProperty.h"
#include "WTargetTemperatureProperty.h"
#include "WHeatingCoolingProperty.h"
#include "WLed.h"

const String DEVICE_TYPE_ON_OFF_SWITCH = "OnOffSwitch";
const String DEVICE_TYPE_LIGHT = "Light";
const String DEVICE_TYPE_TEMPERATURE_SENSOR = "TemperatureSensor";
const String DEVICE_TYPE_THERMOSTAT = "Thermostat";
const String DEVICE_TYPE_TEXT_DISPLAY = "TextDisplay";

class WNetwork;

class WDevice {
public:
	WDevice(WNetwork* network, String id, String name, String type) {
		this->network = network;
		this->id = id;
		this->name = name;
		this->type = type;
		this->visibility = ALL;
		this->providingConfigPage = true;
		this->lastStateNotify = 0;
		this->stateNotifyInterval = 300000;
	}

	~WDevice() {
		//if (webSocket) delete webSocket;
	}

	String getId() {
		return id;
	}

	String getName() {
		return name;
	}

	String getType() {
		return type;
	}

	void addProperty(WProperty* property) {
		property->setDeviceNotification(std::bind(&WDevice::notify, this));
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

	WProperty* getPropertyById(String propertyId) {
		WProperty* property = this->firstProperty;
		while (property != nullptr) {
			if (property->getId().equals(propertyId)) {
				return property;
			}
			property = property->next;
		}
		return nullptr;
	}

	virtual void toJsonValues(WJson* response, WPropertyVisibility visibility) {
		response->beginObject();
		WProperty* property = this->firstProperty;
		while (property != nullptr) {
			if (property->isVisible(visibility)) {
				property->toJsonValue(response);
				property = property->next;
			} else {
				property = property->next;
			}
		}
		response->endObject();
	}

	virtual void toJsonStructure(WJson* json, String deviceHRef, WPropertyVisibility visibility) {
		json->beginObject();
		json->property("name", this->getName());
		String result = deviceHRef + "/things/" + this->getId();
		json->property("href", result);
		json->property("@context", "https://iot.mozilla.org/schemas");
		//type
		json->beginArray("@type");
		json->string(getType());
		json->endArray();
		//properties
		json->beginObject("properties");
		WProperty* property = this->firstProperty;
		while (property != nullptr) {
			if (property->isVisible(visibility)) {
				property->toJsonStructure(json, result);
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
    	WPin* pin = this->firstPin;
    	while (pin != nullptr) {
    		pin->loop(now);
    		pin = pin->next;
    	}
    }

    virtual bool isProvidingConfigPage() {
    	return providingConfigPage;
    }

    virtual String getConfigPage() {
    	return "";
    }

    virtual void saveConfigPage(ESP8266WebServer* server) {

    }

    virtual WLed* getStatusLed() {
    	return this->statusLed;
    }

    virtual bool isDeviceStateComplete() {
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

    /*AsyncWebSocket* getWebSocket() {
    	return webSocket;
    }

    void setWebSocket(AsyncWebSocket* webSocket) {
    	this->webSocket = webSocket;
    }*/

    WPropertyVisibility getVisibility() {
    	return visibility;
    }

	void setVisibility(WPropertyVisibility visibility) {
		this->visibility = visibility;
	}

	bool isVisible(WPropertyVisibility visibility) {
		return ((this->visibility == ALL) || (this->visibility == visibility));
	}

    WDevice* next = nullptr;
    WProperty* firstProperty = nullptr;
    WProperty* lastProperty = nullptr;
    WPin* firstPin = nullptr;
    WPin* lastPin = nullptr;
    unsigned long lastStateNotify;
    int stateNotifyInterval;
protected:
    WNetwork* network;
    WLed* statusLed = nullptr;
    bool providingConfigPage;
    WPropertyVisibility visibility;

private:
	String id;
	String name;
	String type;
	//AsyncWebSocket* webSocket = nullptr;

	void notify() {
		this->lastStateNotify = 0;
	}

};

#endif
