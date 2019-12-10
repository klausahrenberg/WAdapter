#ifndef W_DEVICE_H
#define W_DEVICE_H

#include "ESP8266WebServer.h"
#include "WProperty.h"
#include "WLevelProperty.h"
#include "WOnOffProperty.h"
#include "WStringProperty.h"
#include "WIntegerProperty.h"
#include "WLevelIntProperty.h"
#include "WLongProperty.h"
#include "WTemperatureProperty.h"
#include "WTargetTemperatureProperty.h"
#include "WColorProperty.h"
#include "WLed.h"

const char* DEVICE_TYPE_ON_OFF_SWITCH = "OnOffSwitch";
const char* DEVICE_TYPE_LIGHT = "Light";
const char* DEVICE_TYPE_TEMPERATURE_SENSOR = "TemperatureSensor";
const char* DEVICE_TYPE_THERMOSTAT = "Thermostat";
const char* DEVICE_TYPE_TEXT_DISPLAY = "TextDisplay";

class WNetwork;

class WDevice {
public:
	WDevice(WNetwork* network, const char* id, const char* name, const char* type) {
		this->network = network;
		this->id = id;
		this->name = name;
		this->type = type;
		this->visibility = ALL;
		//this->webSocket = nullptr;
		this->providingConfigPage = true;
		this->lastStateNotify = 0;
		this->stateNotifyInterval = 300000;
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

	virtual void toJsonStructure(WJson* json, const char* deviceHRef, WPropertyVisibility visibility) {
		json->beginObject();
		json->propertyString("name", this->getName());
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

    virtual bool isProvidingConfigPage() {
    	return providingConfigPage;
    }

    virtual void printConfigPage(WStringStream* page) {
    }

    virtual void saveConfigPage(ESP8266WebServer* webServer) {

    }

    virtual void bindWebServerCalls(ESP8266WebServer* webServer) {
    }

    virtual void handleUnknownMqttCallback(String completeTopic, String partialTopic, char *payload, unsigned int length) {

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

    /*WebSocketsServer* getWebSocket() {
    	return webSocket;
    }

    void setWebSocket(WebSocketsServer* webSocket) {
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
    //WebSocketsServer* webSocket;
    WProperty* firstProperty = nullptr;
    WProperty* lastProperty = nullptr;
    WPin* firstPin = nullptr;
    WPin* lastPin = nullptr;
    unsigned long lastStateNotify;
    bool lastStateWaitForResponse;
    int stateNotifyInterval;
protected:
    WNetwork* network;
    WLed* statusLed = nullptr;
    bool providingConfigPage;
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
