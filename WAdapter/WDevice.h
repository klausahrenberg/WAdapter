#ifndef W_DEVICE_H
#define W_DEVICE_H

#include "ESPAsyncWebServer.h"
#include "WProperty.h"
#include "WLevelProperty.h"
#include "WOnOffProperty.h"
#include "WStringProperty.h"
#include "WTemperatureProperty.h"
#include "WLed.h"

const String DEVICE_TYPE_ON_OFF_SWITCH = "OnOffSwitch";
const String DEVICE_TYPE_LIGHT = "Light";
const String DEVICE_TYPE_TEMPERATURE_SENSOR = "TemperatureSensor";

class WDevice {
public:
	WDevice(bool debug, String id, String name, String type) {
		this->debug = debug;
		this->id = id;
		this->name = name;
		this->type = type;
		this->providingConfigPage = false;
		this->lastStateNotify = 0;
		this->stateNotifyInterval = 300000;
	}

	~WDevice() {
		if (webSocket) delete webSocket;
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

	virtual void toJson(JsonObject& json) {
		WProperty* property = this->firstProperty;
		while (property != nullptr) {
			property->toJson(json);
			property = property->next;
		}
	}

    virtual String structToJson(JsonObject& json, String deviceHRef) {
    	String result = deviceHRef + "/things/" + this->getId();
    	json["name"] = this->getName();
    	json["href"] = result;
    	json["@context"] = "https://iot.mozilla.org/schemas";

    	JsonArray typeJson = json.createNestedArray("@type");
    	typeJson.add(getType());
    	/*char** type = device->getType();
    	while ((*type) != nullptr) {
    		typeJson.add(*type);
    		type++;
    	}*/

    	JsonObject props = json.createNestedObject("properties");

		WProperty* property = this->firstProperty;
    	while (property != nullptr) {
    		if (property->isSupportingWebthing()) {
    			JsonObject prop = props.createNestedObject(property->getId());
    			JsonObject& refProp = prop;
    			property->structToJson(refProp, result, "href");
    		}
    		property = property->next;
    	}
    	return result;
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

    virtual void saveConfigPage(AsyncWebServerRequest *request) {

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

    AsyncWebSocket* getWebSocket() {
    	return webSocket;
    }

    void setWebSocket(AsyncWebSocket* webSocket) {
    	this->webSocket = webSocket;
    }

    WDevice* next = nullptr;
    WProperty* firstProperty = nullptr;
    WProperty* lastProperty = nullptr;
    WPin* firstPin = nullptr;
    WPin* lastPin = nullptr;
    unsigned long lastStateNotify;
    int stateNotifyInterval;
protected:
    WLed* statusLed = nullptr;
    bool providingConfigPage;

    void log(String debugMessage) {
    	if (debug) {
    		Serial.println(debugMessage);
    	}
    }

private:
    bool debug;
	String id;
	String name;
	String type;
	AsyncWebSocket* webSocket = nullptr;

	void notify() {
		log("notified about change at a property");
		this->lastStateNotify = 0;
	}

};

#endif
