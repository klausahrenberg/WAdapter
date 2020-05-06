#ifndef W_PROPERTY_H
#define W_PROPERTY_H

#include <Arduino.h>
#include "WJson.h"

//for reference see https://iot.mozilla.org/schemas
const char* TYPE_COLOR_PROPERTY = "ColorProperty";
const char* TYPE_FAN_MODE_PROPERTY = "FanModeProperty";
const char* TYPE_HEATING_COOLING_PROPERTY = "HeatingCoolingProperty";
const char* TYPE_LEVEL_PROPERTY = "LevelProperty";
const char* TYPE_ON_OFF_PROPERTY = "OnOffProperty";
const char* TYPE_OPEN_PROPERTY = "OpenProperty";
const char* TYPE_TARGET_TEMPERATURE_PROPERTY = "TargetTemperatureProperty";
const char* TYPE_THERMOSTAT_MODE_PROPERTY = "ThermostatModeProperty";
const char* TYPE_TEMPERATURE_PROPERTY = "TemperatureProperty";

const char* UNIT_CELSIUS = "degree celsius";

enum WPropertyType {
	BOOLEAN, DOUBLE, INTEGER, LONG, UNSIGNED_LONG, BYTE, STRING
};

enum WPropertyVisibility {
	ALL, NONE, MQTT, WEBTHING
};

union WPropertyValue {
	bool asBoolean;
	double asDouble;
	int asInteger;
	long asLong;
	unsigned long asUnsignedLong;
	byte asByte;
	char* string;
};

class WProperty {
public:
	static WProperty* createIntegerProperty(const char* id, const char* title) {
		return new WProperty(id, title, INTEGER, "");
	}

	static WProperty* createStringProperty(const char* id, const char* title, byte length) {
		return new WProperty(id, title, STRING, length, "");
	}

	static WProperty* createByteProperty(const char* id, const char* title) {
		return new WProperty(id, title, BYTE, "");
	}

	static WProperty* createDoubleProperty(const char* id, const char* title) {
		return new WProperty(id, title, DOUBLE, "");
	}

	static WProperty* createUnsignedLongProperty(const char* id, const char* title) {
		return new WProperty(id, title, UNSIGNED_LONG, "");
	}

	static WProperty* createLongProperty(const char* id, const char* title) {
		return new WProperty(id, title, LONG, "");
	}

	static WProperty* createBooleanProperty(const char* id, const char* title) {
		return new WProperty(id, title, BOOLEAN, "");
	}

	static WProperty* createTargetTemperatureProperty(const char* id, const char* title) {
		WProperty* p = new WProperty(id, title, DOUBLE, TYPE_TARGET_TEMPERATURE_PROPERTY);
		p->setUnit(UNIT_CELSIUS);
		return p;
	}

	static WProperty* createTemperatureProperty(const char* id, const char* title) {
		WProperty* p = new WProperty(id, title, DOUBLE, TYPE_TEMPERATURE_PROPERTY);
		p->setUnit(UNIT_CELSIUS);
		return p;
	}

	static WProperty* createOnOffProperty(const char* id, const char* title) {
		return new WProperty(id, title, BOOLEAN, TYPE_ON_OFF_PROPERTY);
	}

	typedef std::function<void(WProperty* property)> TOnPropertyChange;

	WProperty(const char* id, const char* title, WPropertyType type, const char* atType) {
		initialize(id, title, type, (type == STRING ? 32 : 0), atType);
	}

	WProperty(const char* id, const char* title, WPropertyType type, byte length, const char* atType) {
		initialize(id, title, type, length, atType);
	}

	~WProperty() {
		delete this->id;
		delete this->title;
		if (this->unit) {
			delete this->unit;
		}
		if (this->atType) {
			delete this->atType;
		}
		if(this->value.string) {
		    delete[] this->value.string;
		}
	}

	void setOnValueRequest(TOnPropertyChange onValueRequest) {
		this->onValueRequest = onValueRequest;
	}

	void setOnChange(TOnPropertyChange onChange) {
		this->onChange = onChange;
	}

	void setDeviceNotification(TOnPropertyChange deviceNotification) {
		this->deviceNotification = deviceNotification;
	}

	void setSettingsNotification(TOnPropertyChange settingsNotification) {
		this->settingsNotification = settingsNotification;
	}

	const char* getId() {
		return id;
	}

	const char* getTitle() {
		return title;
	}

	WPropertyType getType() {
		return type;
	}

	byte getLength() {
		return length;
	}

	void setType(WPropertyType type) {
		this->type = type;
	}

	const char* getAtType() {
		return atType;
	}

	/*void setAtType(String atType) {
		this->atType = atType;
	}*/

	bool isNull() {
		return (this->valueNull);
	}

	bool isRequested() {
		return (this->requested);
	}

	void setRequested(bool requested) {
		if ((requested) && (!isNull())) {
			this->requested = true;
		} else {
			this->requested = false;
		}
	}

	void setNull() {
		this->valueNull = true;
	}

	bool isChanged() {
		return (this->changed);
	}

	void setUnChanged() {
		this->changed = false;
	}

	virtual bool parse(String value) {
		if ((!isReadOnly()) && (value != nullptr)) {
			switch (getType()) {
			case BOOLEAN: {
				setBoolean(value.equals("true"));
				return true;
			}
			case DOUBLE: {
				setDouble(value.toDouble());
				return true;
			}
			case INTEGER: {
				setInteger(value.toInt());
				return true;
			}
			case LONG: {
				setLong(value.toInt());
				return true;
			}
			case UNSIGNED_LONG: {
				setUnsignedLong(value.toInt());
				return true;
			}
			case BYTE: {
				setByte(value.toInt());
				return true;
			}
			case STRING:
				setString(value.c_str());
				return true;
			}
		}
		return false;
	}

	bool getBoolean() {
		requestValue();
		return (!this->valueNull ? this->value.asBoolean : false);
	}

	void setBoolean(bool newValue) {
		if (type != BOOLEAN) {
			return;
		}
		bool changed = ((this->valueNull) || (this->value.asBoolean != newValue));
		if (changed) {
			WPropertyValue valueB;
			valueB.asBoolean = newValue;
			this->setValue(valueB);
		}
	}

	void toggleBoolean() {
		if (type != BOOLEAN) {
			return;
		}
		setBoolean(!getBoolean());
	}

	double getDouble() {
		requestValue();
		return (!this->valueNull ? this->value.asDouble : 0.0);
	}

	static bool isEqual(double a, double b, double precision) {
        double diff = a - b;
        return ((diff < precision) && (-diff < precision));
    }

	void setDouble(double newValue) {
		if (type != DOUBLE) {
			return;
		}
		bool changed = ((this->valueNull) || (!isEqual(this->value.asDouble, newValue, 0.01)));
		if (changed) {
			WPropertyValue valueB;
			valueB.asDouble = newValue;
			this->setValue(valueB);
		}
	}

	bool equalsDouble(double number) {
		return ((!this->valueNull) && (isEqual(this->value.asDouble, number, 0.01)));
	}

	int getInteger() {
		requestValue();
		return (!this->valueNull ? this->value.asInteger : 0);
	}

	void setInteger(int newValue) {
		if (type != INTEGER) {
			return;
		}
		bool changed = ((this->valueNull) || (this->value.asInteger != newValue));
		if (changed) {
			WPropertyValue valueB;
			valueB.asInteger = newValue;
			this->setValue(valueB);
		}
	}

	long getLong() {
		requestValue();
		return (!this->valueNull ? this->value.asLong : 0);
	}

	void setLong(long newValue) {
		if (type != LONG) {
			return;
		}
		bool changed = ((this->valueNull) || (this->value.asLong != newValue));
		if (changed) {
			WPropertyValue valueB;
			valueB.asLong = newValue;
			this->setValue(valueB);
		}
	}

	unsigned long getUnsignedLong() {
		requestValue();
		return (!this->valueNull ? this->value.asUnsignedLong : 0);
	}

	void setUnsignedLong(unsigned long newValue) {
		if (type != UNSIGNED_LONG) {
			return;
		}
		bool changed = ((this->valueNull) || (this->value.asUnsignedLong != newValue));
		if (changed) {
			WPropertyValue valueB;
			valueB.asUnsignedLong = newValue;
			this->setValue(valueB);
		}
	}

	bool equalsInteger(int number) {
		return ((!this->valueNull) && (this->value.asInteger == number));
	}

	bool equalsLong(long number) {
		return ((!this->valueNull) && (this->value.asLong == number));
	}

	bool equalsString(const char* toCompare) {
		return ((!this->valueNull) && (strcmp(this->value.string, toCompare) == 0));
	}

	bool equalsUnsignedLong(unsigned long number) {
		return ((!this->valueNull) && (this->value.asUnsignedLong == number));
	}

	byte getByte() {
		requestValue();
		return (!this->valueNull ? this->value.asByte : 0x00);
	}

	void setByte(byte newValue) {
		if (type != BYTE) {
			return;
		}
		bool changed = ((this->valueNull) || (this->value.asByte != newValue));
		if (changed) {
			WPropertyValue valueB;
			valueB.asByte = newValue;
			this->setValue(valueB);
		}
	}

	bool equalsByte(byte number) {
		return ((!this->valueNull) && (this->value.asByte == number));
	}

	char* c_str() {
		requestValue();
		return value.string;
	}

	WPropertyValue getValue() {
	    return this->value;
	}

	bool setString(const char* newValue) {
		if (type != STRING) {
			return false;
		}
		bool changed = ((this->valueNull) || (strcmp(value.string, newValue) != 0));
		if (changed) {
			if (newValue != nullptr) {
				int l = strlen(newValue);
				if (l > length) {
					l = length;
				}
				strncpy(value.string, newValue, l);
				value.string[l] = '\0';
				this->valueNull = false;
			} else {
				value.string[0] = '\0';
				this->valueNull = true;
			}
			this->changed = true;
			valueChanged();
			notify();
		}
		return changed;
	}

	bool isReadOnly() {
		return readOnly;
	}

	void setReadOnly(bool readOnly) {
		this->readOnly = readOnly;
	}

	const char* getUnit() {
		return unit;
	}

	void setUnit(const char* unit) {
		this->unit = unit;
	}

	double getMultipleOf() {
		return multipleOf;
	}

	void setMultipleOf(double multipleOf) {
		this->multipleOf = multipleOf;
	}

	virtual void toJsonValue(WJson* json, bool onlyValue=false) {
		requestValue();
		const char* memberName = (onlyValue ? nullptr : getId());
		switch (getType()) {
		case BOOLEAN:
			json->propertyBoolean(memberName, getBoolean());
			break;
		case DOUBLE:
			json->propertyDouble(memberName, getDouble());
			break;
		case INTEGER:
			json->propertyInteger(memberName, getInteger());
			break;
		case LONG:
			json->propertyLong(memberName, getLong());
			break;
		case UNSIGNED_LONG:
			json->propertyUnsignedLong(memberName, getUnsignedLong());
			break;
		case BYTE:
			json->propertyByte(memberName, getByte());
			break;
		case STRING:
			json->propertyString(memberName, c_str());
			break;
		}
	}

	virtual void toJsonStructure(WJson* json, const char* memberName, const char* deviceHRef) {
		json->beginObject(memberName);
		//title
		if (this->getTitle() != "") {
			json->propertyString("title", getTitle());
		}
		//type
		switch (this->getType()) {
		case BOOLEAN:
			json->propertyString("type", "boolean");
			break;
		case DOUBLE:
		case INTEGER:
		case LONG:
		case UNSIGNED_LONG:
		case BYTE:
			json->propertyString("type", "number");
			break;
		default:
			json->propertyString("type", "string");
			break;
		}
		//readOnly
		if (this->isReadOnly()) {
			json->propertyBoolean("readOnly", true);
		}
		//unit
		if (this->getUnit() != "") {
			json->propertyString("unit", this->getUnit());
		}
		//multipleOf
		if (this->getMultipleOf() > 0.0) {
			json->propertyDouble("multipleOf", this->getMultipleOf());
		}
		//enum
		if (hasEnum()) {
			json->beginArray("enum");
			WProperty* propE = this->firstEnum;
			while (propE != nullptr) {
				switch (this->getType()) {
				case BOOLEAN:
					json->boolean(propE->getBoolean());
				   	break;
				case DOUBLE:
					json->numberDouble(propE->getDouble());
					break;
				case INTEGER:
					json->numberInteger(propE->getInteger());
					break;
				case LONG:
					json->numberLong(propE->getLong());
					break;
				case UNSIGNED_LONG:
					json->numberUnsignedLong(propE->getUnsignedLong());
					break;
				case BYTE:
					json->numberByte(propE->getByte());
					break;
				case STRING:
					json->string(propE->c_str());
				  	break;
				}
				propE = propE->next;
			}
			json->endArray();
		}
		//aType
		if (this->getAtType() != "") {
			json->propertyString("@type", this->getAtType());
		}
		toJsonStructureAdditionalParameters(json);
		//json->beginArray("links");
		//json->beginObject();
		json->propertyString("href", deviceHRef, "/properties/", this->getId());
		//json->endObject();
		//json->endArray();
		json->endObject();
	}

	WProperty* next;

	void addEnumBoolean(bool enumBoolean) {
		if (type != BOOLEAN) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0, "");
		valueE->setBoolean(enumBoolean);
		this->addEnum(valueE);
	}

	void addEnumNumber(double enumNumber) {
		if (type != DOUBLE) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0, "");
		valueE->setDouble(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumInteger(int enumNumber) {
		if (type != INTEGER) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0, "");
		valueE->setInteger(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumLong(long enumNumber) {
		if (type != LONG) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0, "");
		valueE->setLong(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumUnsignedLong(unsigned long enumNumber) {
		if (type != UNSIGNED_LONG) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0, "");
		valueE->setUnsignedLong(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumByte(byte enumByte) {
		if (type != BYTE) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0, "");
		valueE->setByte(enumByte);
		this->addEnum(valueE);
	}

	void addEnumString(const char* enumString) {
		if (type != STRING) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, this->length - 1, "");
		valueE->setString(enumString);
		this->addEnum(valueE);
	}

	byte getEnumIndex() {
		return getEnumIndex(this, this->getValue().string);
	}

	static byte getEnumIndex(WProperty* property, const char* enumString) {
		if ((property->getType() != STRING) || (property->isNull())) {
			return 0xFF;
		}
		byte result = 0xFF;
		WProperty* en = property->firstEnum;
		byte i = 0x00;
		while ((result == 0xFF) && (en != nullptr)) {
			if (strcmp(en->getValue().string, enumString) == 0) {
				result = i;
			}
			en = en->next;
			i++;
		}
		return result;
	}

	const char* getEnumString(byte enumIndex) {
		return getEnumString(this, enumIndex);
	}

	static const char* getEnumString(WProperty* property, byte enumIndex) {
		if (property->getType() != STRING) {
			return nullptr;
		}
		WProperty* en = property->firstEnum;
		byte i = 0x00;
		while ((i < enumIndex) && (en != nullptr)) {
			en = en->next;
			i++;
		}
		return (en != nullptr ? en->getValue().string : nullptr);
	}

	void addEnum(WProperty* propEnum) {
		WProperty* lastEnum = firstEnum;
		while ((lastEnum != nullptr) && (lastEnum->next != nullptr)) {
			lastEnum = lastEnum->next;
		}
		if (lastEnum != nullptr) {
			lastEnum->next = propEnum;
		} else {
			firstEnum = propEnum;
		}
	}

	bool hasEnum() {
		return (firstEnum != nullptr);
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

protected:
	const char* atType;

	void initialize(const char* id, const char* title, WPropertyType type, byte length, const char* atType) {
		this->id = id;
		this->title = title;
		this->type = type;
		this->visibility = ALL;
		this->supportingWebthing = true;
		this->valueNull = true;
		this->changed = true;
		this->requested = false;
		this->valueRequesting = false;
		this->notifying = false;
		this->readOnly = false;
		this->atType = atType;
		this->unit = "";
		this->multipleOf = 0.0;
		this->onChange = nullptr;
		this->deviceNotification = nullptr;
		this->settingsNotification = nullptr;
		this->next = nullptr;
		switch (type) {
		case STRING:
			this->length = length;
			value.string = new char[length + 1];
			value.string[0] = '\0';
			break;
		case DOUBLE:
			this->length = sizeof(double);
			break;
		case INTEGER:
			this->length = 2;
			break;
		case LONG:
		case UNSIGNED_LONG:
			this->length = 4;
			break;
		case BYTE:
		case BOOLEAN:
			this->length = 1;
			break;
		}
	}

	void setValue(WPropertyValue newValue) {
		this->value = newValue;
		this->valueNull = false;
		this->changed = true;
		valueChanged();
		notify();
	}

	virtual void valueChanged() {
	}

	virtual void toJsonStructureAdditionalParameters(WJson* json) {

	}

private:
	const char* id;
	const char* title;
	WPropertyType type;
	WPropertyVisibility visibility;
	bool supportingMqtt;
	bool supportingWebthing;
	byte length;
	bool readOnly;
	const char* unit;
	double multipleOf;
	TOnPropertyChange onChange;
	TOnPropertyChange onValueRequest;
	TOnPropertyChange deviceNotification;
	TOnPropertyChange settingsNotification;
	WPropertyValue value = {false};
	bool valueNull;
	bool changed;
	bool requested;
	bool valueRequesting;
	bool notifying;

	WProperty* firstEnum = nullptr;

	void notify() {
		if (!valueRequesting) {
			notifying = true;
			if (onChange) {
				onChange(this);
			}
			if (deviceNotification) {
				deviceNotification(this);
			}
			if (settingsNotification) {
				settingsNotification(this);
			}
			notifying = false;
		}
	}

	void requestValue() {
		if ((!notifying) && (onValueRequest)) {
			valueRequesting = true;
			onValueRequest(this);
			valueRequesting = false;
		}
	}

};

#endif
