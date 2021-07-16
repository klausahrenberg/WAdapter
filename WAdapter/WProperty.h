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
const char* UNIT_PERCENT = "percent";

enum WPropertyType {
	BOOLEAN, DOUBLE, SHORT, INTEGER, UNSIGNED_LONG, BYTE, STRING, BYTE_ARRAY
};

enum WPropertyVisibility {
	ALL, NONE, MQTT, WEBTHING
};

union WPropertyValue {
	bool asBoolean;
	double asDouble;
	short asShort;
	int asInteger;
	unsigned long asUnsignedLong;
	byte asByte;
	char* string;
	byte* asByteArray;
};

class WProperty {
public:
	static WProperty* createIntegerProperty(const char* id, const char* title) {
		return new WProperty(id, title, INTEGER, "");
	}

	static WProperty* createStringProperty(const char* id, const char* title) {
		return new WProperty(id, title, STRING, "");
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

	static WProperty* createShortProperty(const char* id, const char* title) {
		return new WProperty(id, title, SHORT, "");
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
		initialize(id, title, type, atType);
	}

	~WProperty() {
		delete this->id;
		delete this->title;
		if ((type == STRING) && (this->value.string)) {
		  delete[] this->value.string;
		}
		if ((type == BYTE_ARRAY) && (this->value.asByteArray)) {
			delete[] this->value.asByteArray;
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
		switch (type) {
		case STRING:
			return (this->valueNull ? 0 : strlen(value.string));
			/*this->length = length;
			value.string = new char[length + 1];
			value.string[0] = '\0';
			break;*/
		case DOUBLE:
			return sizeof(double);
		case SHORT:
			return sizeof(short);
		case INTEGER:
			return sizeof(int);
		case UNSIGNED_LONG:
			return sizeof(unsigned long);
		case BYTE:
		case BOOLEAN:
			return 1;
		case BYTE_ARRAY:
			return (this->valueNull ? 0 : sizeof(value.asByteArray));
			/*this->length = length;
			value.asByteArray = new byte[length];
			for (byte i = 0; i < length; i++) {
				value.asByteArray[i] = 0;
			}*/
		}
		return 0;
	}

	void setType(WPropertyType type) {
		this->type = type;
	}

	const char* getAtType() {
		return atType;
	}

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
				value.toLowerCase();
				setBoolean(value.equals("true"));
				return true;
			}
			case DOUBLE: {
				setDouble(value.toDouble());
				return true;
			}
			case SHORT: {
				setShort(value.toInt());
				return true;
			}
			case INTEGER: {
				setInteger(value.toInt());
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
			case STRING: {
				setString(value.c_str());
				return true;
			}
			case BYTE_ARRAY: {
				//tbi not implemented yet
				return false;
			}
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

	short getShort() {
		requestValue();
		return (!this->valueNull ? this->value.asShort : 0);
	}

	void setShort(short newValue) {
		if (type != SHORT) {
			return;
		}
		bool changed = ((this->valueNull) || (this->value.asShort != newValue));
		if (changed) {
			WPropertyValue valueB;
			valueB.asShort = newValue;
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

	bool isIntegerBetween(int lowerLimit, int upperLimit) {
		return ((!this->valueNull) && (this->value.asInteger >= lowerLimit) && (this->value.asInteger < upperLimit));
	}

	bool equalsShort(short number) {
		return ((!this->valueNull) && (this->value.asShort == number));
	}

	bool equalsString(const char* toCompare) {
		return ((!this->valueNull) && (strcmp(this->value.string, toCompare) == 0));
	}

	bool equalsUnsignedLong(unsigned long number) {
		return ((!this->valueNull) && (this->value.asUnsignedLong == number));
	}

	bool isUnsignedLongBetween(unsigned long lowerLimit, unsigned long upperLimit) {
		return ((!this->valueNull) && (this->value.asUnsignedLong >= lowerLimit) && (this->value.asUnsignedLong < upperLimit));
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

	byte* getByteArray() {
		return value.asByteArray;
	}

	bool setByteArray(const byte* newValue) {
		if (type != BYTE_ARRAY) {
			return false;
		}
		byte newLength = sizeof(newValue);
		bool changed = ((this->valueNull) || (newLength != (sizeof(value.asByteArray))));
		if ((!this->valueNull) && (newLength != (sizeof(value.asByteArray)))) {
			free(value.asByteArray);
			value.asByteArray = (byte *) malloc(newLength);
		}
		value.asByteArray = (byte *) malloc(sizeof(newValue));
		for (int i = 0; i < newLength; i++) {
			changed = ((changed) || (value.asByteArray[i] != newValue[i]));
			value.asByteArray[i] = newValue[i];
		}
		if (changed) {
			this->valueNull = false;
			this->changed = true;
			valueChanged();
			notify();
		}
		return changed;
	}

	byte getByteArrayValue(byte index) {
		return value.asByteArray[index];
	}

	bool getByteArrayBitValue(byte byteIndex, byte bitIndex) {
		return bitRead(getByteArrayValue(byteIndex), bitIndex);
	}

	WPropertyValue getValue() {
	    return this->value;
	}

	bool setString(const char* newValue) {
		if (type != STRING) {
			return false;
		}
		bool changed = ((this->valueNull) || (strcmp(value.string, newValue) != 0));
		if ((changed) && (newValue != nullptr) && (this->hasEnum())) {
			//proceed only at valid enums
			changed = (getEnumIndex(this, newValue) != 0xFF);
		}
		if (changed) {
			if (!this->valueNull) {
				free(value.string);
			}

			if (newValue != nullptr) {
				int l = strlen(newValue);
				value.string = (char *) malloc(l + 1);
				strncpy(value.string, newValue, l);
				value.string[l] = '\0';
				this->valueNull = false;
			} else {
				value.string = nullptr;
				//value.string[0] = '\0';
				this->valueNull = true;
			}
			this->changed = true;
			valueChanged();
			notify();
		}
		return changed;
	}

	bool setByteArrayValue(byte index, byte newValue) {
		if (type != BYTE_ARRAY) {
			return false;
		}
		bool changed = ((this->valueNull) || (value.asByteArray[index] != newValue));
		if (changed) {
			value.asByteArray[index] = newValue;
			this->valueNull = false;
			this->changed = true;
			valueChanged();
			notify();
		}
		return changed;
	}

	bool setByteArrayBitValue(byte byteIndex, byte bitIndex, bool bitValue) {
		if (type != BYTE_ARRAY) {
			return false;
		}
		byte v = getByteArrayValue(byteIndex);
		if (bitValue) {
			bitSet(v, bitIndex);
		} else {
			bitClear(v, bitIndex);
		}
		return setByteArrayValue(byteIndex, v);
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
		case SHORT:
			json->propertyShort(memberName, getShort());
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
		case BYTE_ARRAY:
			//tbi
			json->propertyByteArray(memberName, getLength(), value.asByteArray);
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
		case SHORT:
		case INTEGER:
		case UNSIGNED_LONG:
		case BYTE:
			json->propertyString("type", "number");
			break;
		case BYTE_ARRAY:
				json->propertyString("type", "object");
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
				case SHORT:
					json->numberShort(propE->getShort());
					break;
				case INTEGER:
					json->numberInteger(propE->getInteger());
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
		WProperty* valueE = new WProperty("", "", this->type, "");
		valueE->setBoolean(enumBoolean);
		this->addEnum(valueE);
	}

	void addEnumNumber(double enumNumber) {
		if (type != DOUBLE) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, "");
		valueE->setDouble(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumInteger(int enumNumber) {
		if (type != INTEGER) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, "");
		valueE->setInteger(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumShort(short enumNumber) {
		if (type != SHORT) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, "");
		valueE->setShort(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumUnsignedLong(unsigned long enumNumber) {
		if (type != UNSIGNED_LONG) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, "");
		valueE->setUnsignedLong(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumByte(byte enumByte) {
		if (type != BYTE) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, "");
		valueE->setByte(enumByte);
		this->addEnum(valueE);
	}

	void addEnumString(const char* enumString) {
		if (type != STRING) {
			return;
		}
		WProperty* valueE = new WProperty("", "", this->type, "");
		valueE->setString(enumString);
		this->addEnum(valueE);
	}

	byte getEnumIndex() {
		return getEnumIndex(this, this->getValue().string);
	}

	static byte getEnumIndex(WProperty* property, const char* enumString) {
		if ((property->getType() != STRING) || (!property->hasEnum())) {
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

	void clearEnums() {
		WProperty* nextEnum = nullptr;
		while (firstEnum != nullptr) {
			WProperty* nextEnum = firstEnum->next;
			delete firstEnum;
			firstEnum = nextEnum;
		}
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

	void setVisibilityMqtt(bool value) {
		if ((value) && (visibility != MQTT) && (visibility != ALL)) {
			setVisibility(visibility == WEBTHING ? ALL : MQTT);
		} else if ((!value) && (visibility != NONE) && (visibility != WEBTHING)) {
			setVisibility(visibility == ALL ? WEBTHING : NONE);
		}
	}

	void setVisibilityWebthing(bool value) {
		if ((value) && (visibility != WEBTHING) && (visibility != ALL)) {
			setVisibility(visibility == MQTT ? ALL : WEBTHING);
		} else if ((!value) && (visibility != NONE) && (visibility != MQTT)) {
			setVisibility(visibility == ALL ? MQTT : NONE);
		}
	}

	bool isVisible(WPropertyVisibility visibility) {
		return ((this->visibility == ALL) || (this->visibility == visibility));
	}

	void setId(const char* id) {
		delete this->id;
		this->id = new char[strlen(id) + 1];
		strcpy(this->id, id);
	}

protected:
	const char* atType;

	void initialize(const char* id, const char* title, WPropertyType type, const char* atType) {
		this->id = new char[strlen(id) + 1];
		strcpy(this->id, id);
		this->title = new char[strlen(title) + 1];
		strcpy(this->title, title);
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
		this->next = nullptr;
		this->firstEnum = nullptr;
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
	char* id;
	char* title;
	WPropertyType type;
	WPropertyVisibility visibility;
	bool supportingMqtt;
	bool supportingWebthing;
	bool readOnly;
	const char* unit;
	double multipleOf;
	TOnPropertyChange onChange;
	TOnPropertyChange onValueRequest;
	TOnPropertyChange deviceNotification;
	WPropertyValue value = {false};
	bool valueNull;
	bool changed;
	bool requested;
	bool valueRequesting;
	bool notifying;

	WProperty* firstEnum;

	void notify() {
		if (!valueRequesting) {
			notifying = true;
			if (onChange) {
				onChange(this);
			}
			if (deviceNotification) {
				deviceNotification(this);
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
