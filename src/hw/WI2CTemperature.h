#ifndef W_I2C_TEMPERATURE_H
#define W_I2C_TEMPERATURE_H

#include "../WI2C.h"

#define ERROR_NONE 0
#define ERROR_I2C_TIMEOUT 1
#define ERROR_BAD_CRC	2
#define ERROR_I2C_REQUEST 3

const static char WC_HUMIDITY_ID[] PROGMEM = "humidityId";
const static char WC_HUMIDITY_TITLE[] PROGMEM = "humidityTitle";

class WI2CTemperature: public WI2C {
public:
	WI2CTemperature(WGpioType gpioType, byte address, int sda, int scl, TwoWire* i2cPort = &Wire)
			: WI2C(gpioType, address, sda, scl, NO_PIN, i2cPort) {				
		_humidity = nullptr;		
		_lastError = ERROR_NONE;
	}

	virtual ~WI2CTemperature() {
		if (_id) delete _id;
		if (_title) delete _title;
		if (_humidityId) delete _humidityId;
		if (_humidityTitle) delete _humidityTitle;
	}

	virtual void registerSettings() {
		WI2C::registerSettings();
		SETTINGS->add(_id, nullptr);
		SETTINGS->add(_title, nullptr);
		SETTINGS->add(_humidityId, nullptr);
		SETTINGS->add(_humidityTitle, nullptr);
	}

	virtual void fromJson(WList<WValue>* list) {
		WI2C::fromJson(list);
		list->ifExistsId(WC_ID, [this](WValue* v) { _id->asString(v->asString()); });
		list->ifExistsId(WC_TITLE, [this](WValue* v) { _title->asString(v->asString()); });
		list->ifExistsId(WC_HUMIDITY_ID, [this](WValue* v) { _humidityId->asString(v->asString()); });
		list->ifExistsId(WC_HUMIDITY_TITLE, [this](WValue* v) { _humidityTitle->asString(v->asString()); });
	}

	virtual void toJson(WJson* json) {
		WI2C::toJson(json);
		json->property(WC_ID, _id);
		json->property(WC_TITLE, _title);
		json->property(WC_HUMIDITY_ID, _humidityId);
		json->property(WC_HUMIDITY_TITLE, _humidityTitle);
	}

	WValue* id() { return _id; }
	WValue* title() { return _title; }
	WValue* humidityId() { return _humidityId; }
	WValue* humidityTitle() { return _humidityTitle; }

	virtual void loop(unsigned long now) {
		WI2C::loop(now);	
	}
	

	WProperty* humidity() { return _humidity; }

  void setHumidity(WProperty* humidity) {
    if (_humidity != humidity) {
      _humidity = humidity;
      this->loop(millis());
    }
  }

  bool hasHumidity() { return (_humidity != nullptr); }

	bool hasError() { return (_lastError != ERROR_NONE); }

	byte lastError() { return _lastError; }	

protected:
	WValue* _id = new WValue(WDataType::STRING);
	WValue* _title = new WValue(WDataType::STRING);
	WValue* _humidityId = new WValue(WDataType::STRING);
	WValue* _humidityTitle = new WValue(WDataType::STRING);
	WProperty* _humidity;	
	byte _lastError;

private:
	

};

#endif
