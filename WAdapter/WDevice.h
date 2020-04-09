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
#include "WPage.h"

const char* DEVICE_TYPE_ON_OFF_SWITCH = "OnOffSwitch";
const char* DEVICE_TYPE_LIGHT = "Light";
const char* DEVICE_TYPE_TEMPERATURE_SENSOR = "TemperatureSensor";
const char* DEVICE_TYPE_THERMOSTAT = "Thermostat";
const char* DEVICE_TYPE_LOG = "Log";
const char* DEVICE_TYPE_TEXT_DISPLAY = "TextDisplay";

class WNetwork;

class WDevice {
public:
	WDevice(WNetwork* network, const char* id, const char* name, const char* rootname, const char* type) {
		this->network = network;
		this->id = id;
		this->name = name;
		size_t size=strlen(rootname)+strlen(name) +1+1;
		this->fullname=(char*)malloc(size);
		snprintf_P(this->fullname, size, "%s_%s", rootname, name);
		this->type = type;
		this->visibility = ALL;
		//this->webSocket = nullptr;
		this->providingConfigPage = true;
		this->configNeedsReboot = true;
		this->mainDevice = true;
		this->lastStateNotify = 0;
		this->stateNotifyInterval = 300000;
		this->mqttRetain = false;
		this->mqttSendChangedValues = false;
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


	const char* getFullName() {
		return fullname;
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

	void addPage(WPage *Page) {
		if (lastPage == nullptr) {
			firstPage = Page;
			lastPage = Page;
		} else {
			lastPage->next = Page;
			lastPage = Page;
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
				property->toJsonValue(json);
			}
			property = property->next;
		}
	}

	virtual void toJsonStructure(WJson* json, const char* deviceHRef, WPropertyVisibility visibility) {
		json->beginObject();
		json->propertyString("name", this->getFullName());
		String result(deviceHRef);
		result.concat("/things/");
		result.concat(this->getId());
		json->propertyString("href", result.c_str());
		json->propertyString("@context", "https://iot.mozilla.org/schemas");
		json->propertyString("title",  this->getFullName());
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
    virtual bool isConfigNeedsReboot() {
    	return configNeedsReboot;
    }

    virtual void printConfigPage(WStringStream* page) {

    }

    virtual void saveConfigPage(ESP8266WebServer* webServer) {

    }

    virtual void bindWebServerCalls(ESP8266WebServer* webServer) {

    }

    virtual void handleUnknownMqttCallback(String completeTopic, String partialTopic, String payload, unsigned int length) {

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

	
	virtual void sendLog(int level, const char * message) {
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

	bool isMainDevice() {
		return mainDevice;
	}
	void setMqttRetain(bool val){
		this->mqttRetain = val;
	}
	bool isMqttRetain(){
		return this->mqttRetain;
	}
	void setMqttSendChangedValues(bool val){
		this->mqttSendChangedValues = val;
	}
	bool isMqttSendChangedValues(){
		return this->mqttSendChangedValues;
	}

    WDevice* next = nullptr;
    //WebSocketsServer* webSocket;
	WProperty* firstProperty = nullptr;
	WProperty* lastProperty = nullptr;
	WPage* firstPage= nullptr;
	WPage* lastPage= nullptr;
    WPin* firstPin = nullptr;
    WPin* lastPin = nullptr;
    unsigned long lastStateNotify;
    int stateNotifyInterval;
protected:
    WNetwork* network;
    WLed* statusLed = nullptr;
    bool providingConfigPage;
	bool configNeedsReboot;
    bool mainDevice;
    WPropertyVisibility visibility;
	bool mqttRetain;
	bool mqttSendChangedValues;

private:
	const char* id;
	const char* name;
	char* fullname;
	const char* type;

	void onPropertyChange() {
		this->lastStateNotify = 0;
	}

};

#endif
