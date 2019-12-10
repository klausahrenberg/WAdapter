#ifndef W_PROPERTY_H
#define W_PROPERTY_H

#include <Arduino.h>
#include "WJson.h"

enum WPropertyType {
	BOOLEAN, DOUBLE, INTEGER, LONG, BYTE, STRING
};

enum WPropertyVisibility {
	ALL, NONE, MQTT, WEBTHING
};

union WPropertyValue {
	bool asBoolean;
	double asDouble;
	int asInteger;
	unsigned long asLong;
	byte asByte;
	char* string;
};

class WProperty {
public:
	typedef std::function<void(WProperty* property)> TOnPropertyChange;

	WProperty(const char* id, const char* title, WPropertyType type) {
		initialize(id, title, type, (type == STRING ? 32 : 0));
	}

	WProperty(const char* id, const char* title, WPropertyType type, byte length) {
		initialize(id, title, type, length);
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

	bool isEqual(double a, double b, double precision) {
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
		return ((!this->valueNull) && (this->value.asDouble == number));
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

	unsigned long getLong() {
		requestValue();
		return (!this->valueNull ? this->value.asLong : 0);
	}

	void setLong(unsigned long newValue) {
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

	bool equalsInteger(int number) {
		return ((!this->valueNull) && (this->value.asInteger == number));
	}

	bool equalsString(const char* toCompare) {
		return ((!this->valueNull) && (strcmp(this->value.string, toCompare) == 0));
	}

	bool equalsLong(unsigned long number) {
		return ((!this->valueNull) && (this->value.asLong == number));
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

	void setString(const char* newValue) {
		if (type != STRING) {
			return;
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
			valueChanged();
			notify();
		}
	}

	/*void setString(String newValue) {
		if (type != STRING) {
			return;
		}
		int l = newValue.length();
		if (l > length) {
			l = length;
		}
		bool changed = ((this->valueNull) || (strcmp(value.string, newValue.c_str()) != 0));
		if (changed) {
			strncpy(value.string, newValue.c_str(), l);
			value.string[l] = '\0';
			this->valueNull = false;
			valueChanged();
			notify();
		}
	}*/

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

	virtual void toJsonValue(WJson* json) {
		switch (getType()) {
		case BOOLEAN:
			json->propertyBoolean(getId(), getBoolean());
			break;
		case DOUBLE:
			json->propertyDouble(getId(), getDouble());
			break;
		case INTEGER:
			json->propertyInteger(getId(), getInteger());
			break;
		case LONG:
			json->propertyLong(getId(), getLong());
			break;
		case BYTE:
			json->propertyByte(getId(), getByte());
			break;
		case STRING:
			json->propertyString(getId(), c_str());
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
		json->propertyString("href", deviceHRef, "/properties/", this->getId());
		json->endObject();
	}

	WProperty* next;

	void addEnumBoolean(bool enumBoolean) {
		if (type != BOOLEAN) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0);
		valueE->setBoolean(enumBoolean);
		this->addEnum(valueE);
	}

	void addEnumNumber(double enumNumber) {
		if (type != DOUBLE) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0);
		valueE->setDouble(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumInteger(int enumNumber) {
		if (type != INTEGER) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0);
		valueE->setInteger(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumLong(unsigned long enumNumber) {
		if (type != LONG) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0);
		valueE->setLong(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumByte(byte enumByte) {
		if (type != BYTE) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, 0);
		valueE->setByte(enumByte);
		this->addEnum(valueE);
	}

	void addEnumString(const char* enumString) {
		if (type != STRING) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, this->length - 1);
		valueE->setString(enumString);
		this->addEnum(valueE);
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

	void setAtType(const char* atType) {
		this->atType = atType;
	}

protected:
	const char* atType;

	void initialize(const char* id, const char* title, WPropertyType type, byte length) {
		this->id = id;
		this->title = title;
		this->type = type;
		this->visibility = ALL;
		this->supportingWebthing = true;
		this->valueNull = true;
		this->requested = false;
		this->valueRequesting = false;
		this->readOnly = false;
		this->atType = "";
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
	bool requested;
	bool valueRequesting;

	WProperty* firstEnum = nullptr;

	void notify() {
		if (!valueRequesting) {
			if (onChange) {
				onChange(this);
			}
			if (deviceNotification) {
				deviceNotification(this);
			}
			if (settingsNotification) {
				settingsNotification(this);
			}
		}
	}

	void requestValue() {
		if (onValueRequest) {
			valueRequesting = true;
			onValueRequest(this);
			valueRequesting = false;
		}
	}

};

#endif
