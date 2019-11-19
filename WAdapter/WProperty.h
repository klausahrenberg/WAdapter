#ifndef W_PROPERTY_H
#define W_PROPERTY_H

#include <Arduino.h>

enum WPropertyType {
	BOOLEAN, DOUBLE, INTEGER, BYTE, STRING
};

union WPropertyValue {
	bool asBoolean;
	double asDouble;
	int asInteger;
	byte asByte;
	//String* asString;
	char* string;
};

class WProperty {
public:
	typedef std::function<void(WProperty* property)> TOnPropertyChange;

	WProperty(String id, String title, String description, WPropertyType type) {
		initialize(id, title, description, type, (type == STRING ? 32 : 0));
	}

	WProperty(String id, String title, String description, WPropertyType type, byte length) {
		initialize(id, title, description, type, length);
	}

	void setOnChange(TOnPropertyChange onChange) {
		this->onChange = onChange;
	}

	void setDeviceNotification(TOnPropertyChange deviceNotification) {
		this->deviceNotification = deviceNotification;
	}

	String getId() {
		return id;
	}

	void setId(String id) {
		this->id = id;
	}

	String getTitle() {
		return title;
	}

	void setTitle(String title) {
		this->title = title;
	}

	String getDescription() {
		return description;
	}

	void setDescription(String description) {
		this->description = description;
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

	String getAtType() {
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

	void setFromJson(JsonVariant value) {
		if ((!isReadOnly()) && (value != nullptr)) {
			switch (getType()) {
			case BOOLEAN: {
				setBoolean(value.as<bool>());
				break;
			}
			case DOUBLE: {
				setDouble(value.as<double>());
				break;
			}
			case INTEGER: {
				setInteger(value.as<int>());
				break;
			}
			case BYTE: {
				setByte(value.as<byte>());
				break;
			}
			case STRING:
				setString(value.as<String>());
				break;
			}
		}
	}

	bool getBoolean() {
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

	bool equalsInteger(int number) {
		return ((!this->valueNull) && (this->value.asInteger == number));
	}

	byte getByte() {
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

	String getString() {
		return (!this->valueNull ? String(value.string) : "");
	}

	WPropertyValue getValue() {
	    return this->value;
	}

	void setString(String newValue) {
		if (type != STRING) {
			return;
		}
		bool changed = ((this->valueNull) || (strcmp(value.string, newValue.c_str()) != 0));
		if (changed) {
			strcpy(value.string, newValue.c_str());
			this->valueNull = false;
			valueChanged();
			notify();
		}
	}

	bool isReadOnly() {
		return readOnly;
	}

	void setReadOnly(bool readOnly) {
		this->readOnly = readOnly;
	}

	String getUnit() {
		return unit;
	}

	void setUnit(String unit) {
		this->unit = unit;
	}

	double getMultipleOf() {
		return multipleOf;
	}

	void setMultipleOf(double multipleOf) {
		this->multipleOf = multipleOf;
	}

	virtual void toJson(JsonObject& json) {
		switch (getType()) {
		case BOOLEAN:
			json[getId()] = getBoolean();
			break;
		case DOUBLE:
			json[getId()] = getDouble();
			break;
		case INTEGER:
			json[getId()] = getInteger();
			break;
		case BYTE:
			json[getId()] = getByte();
			break;
		case STRING:
			json[getId()] = getString();
			break;
		}
	}

	virtual String structToJson(JsonObject& json, String deviceHRef, String nameHRef) {
		if (this->getTitle() != "") {
			json["title"] = this->getTitle();
		}
		if (this->getDescription() != "") {
			json["description"] = this->getDescription();
		}
	    switch (this->getType()) {
	    case BOOLEAN:
	    	json["type"] = "boolean";
	    	break;
	    case DOUBLE:
	    case INTEGER:
	    case BYTE:
	    	json["type"] = "number";
	    	break;
	    case STRING:
	    	json["type"] = "string";
	    	break;
	    }

	    if (this->isReadOnly()) {
	    	json["readOnly"] = true;
	    }

	    if (this->getUnit() != "") {
	    	json["unit"] = this->getUnit();
	    }

	    if (this->getMultipleOf() > 0.0) {
	    	json["multipleOf"] = this->getMultipleOf();
	    }

	    if (hasEnum()) {
	    	JsonArray propEnum = json.createNestedArray("enum");
	    	WProperty* propE = this->firstEnum;
	    	while (propE != nullptr) {
	    		switch (this->getType()) {
	    		case BOOLEAN:
	    			propEnum.add(propE->getBoolean());
	    		   	break;
	    		case DOUBLE:
	    			propEnum.add(propE->getDouble());
	    		  	break;
	    		case INTEGER:
	    			propEnum.add(propE->getInteger());
	    			break;
	    		case BYTE:
	    			propEnum.add(propE->getByte());
	    			break;
	    		case STRING:
	    			propEnum.add(propE->getString());
	    		  	break;
	    		}
	    		propE = propE->next;
	    	}
		}

	    if (this->getAtType() != "") {
	    	json["@type"] = this->getAtType();
	    }
	    String result = deviceHRef + "/properties/" + this->getId();
	    json[nameHRef] = result;
	    return result;
	}

	WProperty* next;

	void addEnumBoolean(bool enumBoolean) {
		if (type != BOOLEAN) {
			return;
		}
		WProperty* valueE = new WProperty("", "", "", this->type, 0);
		valueE->setBoolean(enumBoolean);
		this->addEnum(valueE);
	}

	void addEnumNumber(double enumNumber) {
		if (type != DOUBLE) {
			return;
		}
		WProperty* valueE = new WProperty("", "", "", this->type, 0);
		valueE->setDouble(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumInteger(int enumNumber) {
		if (type != INTEGER) {
			return;
		}
		WProperty* valueE = new WProperty("", "", "", this->type, 0);
		valueE->setInteger(enumNumber);
		this->addEnum(valueE);
	}

	void addEnumByte(byte enumByte) {
		if (type != BYTE) {
			return;
		}
		WProperty* valueE = new WProperty("", "", "", this->type, 0);
		valueE->setByte(enumByte);
		this->addEnum(valueE);
	}

	void addEnumString(String enumString) {
		if (type != STRING) {
			return;
		}
		WProperty* valueE = new WProperty("", "", "", this->type, this->length - 1);
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

	bool isSupportingMqtt() {
		return this->supportingMqtt;
	}

	void setSupportingMqtt(bool supportingMqtt) {
		this->supportingMqtt = supportingMqtt;
	}

	bool isSupportingWebthing() {
		return this->supportingWebthing;
	}

	void setSupportingWebthing(bool supportingWebthing) {
		this->supportingWebthing = supportingWebthing;
	}

protected:
	String atType;

	void initialize(String id, String title, String description, WPropertyType type, byte length) {
		this->id = id;
		this->title = title;
		this->description = description;
		this->type = type;
		this->supportingMqtt = true;
		this->supportingWebthing = true;
		this->valueNull = true;
		this->requested = false;
		this->readOnly = false;
		this->unit = "";
		this->multipleOf = 0.0;
		this->next = nullptr;;
		switch (type) {
		case STRING:
			this->length = length;
			value.string = new char[length + 1];
			break;
		case DOUBLE:
			this->length = sizeof(double);
			break;
		case INTEGER:
			this->length = 2;
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

private:
	String id;
	String title;
	String description;
	WPropertyType type;
	bool supportingMqtt;
	bool supportingWebthing;
	byte length;
	bool readOnly;
	String unit;
	double multipleOf;
	TOnPropertyChange onChange;
	TOnPropertyChange deviceNotification;
	WPropertyValue value = {false};
	bool valueNull;
	bool requested;

	WProperty* firstEnum = nullptr;

	void notify() {
		if (onChange) {
			onChange(this);
		}
		if (deviceNotification) {
			deviceNotification(this);
		}
	}
};

#endif
