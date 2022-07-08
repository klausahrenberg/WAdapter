#ifndef W_SETTINGS_H
#define W_SETTINGS_H

#include "EEPROM.h"
#include "WLog.h"
#include "WProperty.h"

const byte FLAG_OPTIONS_NETWORK = 0x60;
const byte FLAG_OPTIONS_NETWORK_FORCE_AP = 0x65;
const int EEPROM_SIZE = 1024; // SPI_FLASH_SEC_SIZE;

struct WSettingItem {
	WProperty* value;
	bool networkSetting;
	WSettingItem* next = nullptr;
};

class WSettings {
public:
	WSettings(WLog* log, byte appSettingsFlag = 0x68) {
		this->log = log;
		this->appSettingsFlag = appSettingsFlag;
		this->addingNetworkSettings = true;
		this->address = 2;
		this->readingFirstTime = true;
		EEPROM.begin(EEPROM_SIZE);
		this->networkByte = EEPROM.read(0);
		this->existsSettingsApplication = (EEPROM.read(1) == this->appSettingsFlag);
	}

	void endReadingFirstTime() {
		if (isReadingFirstTime()) {
			EEPROM.end();
			this->readingFirstTime = false;
		}
	}

	bool isReadingFirstTime() {
		return readingFirstTime;
	}

	void save() {
		this->saveEEPROM(FLAG_OPTIONS_NETWORK);
	}

	void forceAPNextStart() {
		this->saveEEPROM(FLAG_OPTIONS_NETWORK_FORCE_AP);
	}

	void resetAll() {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.write(0, 0x00);
		EEPROM.write(1, 0x00);
		EEPROM.commit();
		EEPROM.end();
	}

	bool existsSetting(const char* id) {
		return (getSetting(id) != nullptr);
	}

	bool existsNetworkSettings() {
		return ((this->networkByte == FLAG_OPTIONS_NETWORK) || (forceNetworkAccessPoint()));
	}

	bool forceNetworkAccessPoint() {
		return (this->networkByte == FLAG_OPTIONS_NETWORK_FORCE_AP);
	}

	WProperty* getSetting(const char* id) {
		WSettingItem* settingItem = firstSetting;
		while (settingItem != nullptr) {
			WProperty* setting = settingItem->value;
			if (strcmp(id, setting->getId()) == 0) {
				return setting;
			}
			settingItem = settingItem->next;
		}
		return nullptr;
	}

	bool exists(WProperty* property) {
		WSettingItem* settingItem = firstSetting;
		while (settingItem != nullptr) {
			if (settingItem->value == property) {
				return true;
			}
			settingItem = settingItem->next;
		}
		return false;
	}

	void add(WProperty* property) {
		if (!exists(property)) {
			WSettingItem* settingItem = addSetting(property);
			if ((isReadingFirstTime()) &&
			    (((settingItem->networkSetting) && (this->existsNetworkSettings())) ||
			     ((!settingItem->networkSetting) && (this->existsSettingsApplication)))) {
				/*
				Serial.print("Read prop: ");
				Serial.print(property->getTitle());
				Serial.print(" / address: ");
				Serial.print(address);
				Serial.print(" / length: ");
				Serial.print(getLengthInEEPROM(property));
				Serial.print(" / value: ");
				*/
				switch (property->getType()) {
				case BOOLEAN: {
					property->setBoolean(EEPROM.read(address) == 0xFF);
					//Serial.println(property->getBoolean());
					break;
				}
				case DOUBLE: {
					double d;
					EEPROM.get(address, d);
					property->setDouble(d);
					//Serial.println(property->getDouble());
					break;
				}
				case SHORT: {
					short value = 0;
					EEPROM.get(address, value);
					property->setShort(value);
					//Serial.println(property->getShort());
					break;
				}
				case INTEGER: {
					int value = 0;
					EEPROM.get(address, value);
					property->setInteger(value);
					//Serial.println(property->getInteger());
					break;
				}
				case UNSIGNED_LONG: {
					unsigned long value = 0;
					EEPROM.get(address, value);
				  property->setUnsignedLong(value);
					//Serial.println(property->getUnsignedLong());
					break;
				}
				case BYTE: {
					int b2 = EEPROM.read(address);
					property->setByte(b2);
					break;
				}
				case BYTE_ARRAY: {
					//Serial.println("[]");
					const byte* ba = readByteArray(address);
					property->setByteArray(ba);
					delete ba;
					break;
				}
				case STRING: {
					const char* rs = readString(address);
					property->setString(rs);
					//Serial.println(property->c_str());
					delete rs;
					//break;
				}
				}
				address += this->getLengthInEEPROM(property);
			}
		}
	}

	WProperty* remove(const char* id) {
		WSettingItem* prevItem = nullptr;
		WSettingItem* settingItem = firstSetting;
		while (settingItem != nullptr) {
			WProperty* setting = settingItem->value;
			if (strcmp(id, setting->getId()) == 0) {
				if (prevItem != nullptr) {
					prevItem->next = settingItem->next;
				}
				if (settingItem == this->firstSetting) {
					this->firstSetting = settingItem->next;
				}
				if (settingItem == this->lastSetting) {
					this->lastSetting = prevItem;
				}
				WProperty* result = settingItem->value;
				delete settingItem;
				return result;
			}
			prevItem = settingItem;
			settingItem = settingItem->next;
		}
		return nullptr;
	}

	bool getBoolean(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getBoolean() : false);
	}

	WProperty* setBoolean(const char* id, bool value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProperty::createBooleanProperty(id, id);
			setting->setBoolean(value);
			add(setting);
		} else {
			setting->setBoolean(value);
		}
		return setting;
	}

	byte getByte(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getByte() : 0x00);
	}

	WProperty* setByte(const char* id, byte value) {
		return setByte(id, value, 0);
	}

	WProperty* setByte(const char* id, byte value, byte max) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProperty::createByteProperty(id, id);
			setting->setByte(value);
			add(setting);
			if ((max > 0) && (setting->getByte() > max)) {
				setting->setByte(value);
			}
		} else {
			setting->setByte(value);
		}
		return setting;
	}

	int getInteger(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getInteger() : 0);
	}

	WProperty* setInteger(const char* id, int value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProperty::createIntegerProperty(id, id);
			setting->setInteger(value);
			add(setting);
		} else {
			setting->setInteger(value);
		}
		return setting;
	}

	short getShort(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getShort() : 0);
	}

	WProperty* setShort(const char* id, short value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProperty::createShortProperty(id, id);
			setting->setShort(value);
			add(setting);
		} else {
			setting->setShort(value);
		}
		return setting;
	}

	unsigned long getUnsignedLong(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getUnsignedLong() : 0);
	}

	WProperty* setUnsignedLong(const char* id, unsigned long value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProperty::createUnsignedLongProperty(id, id);
			setting->setUnsignedLong(value);
			add(setting);
		} else {
			setting->setUnsignedLong(value);
		}
		return setting;
	}

	double getDouble(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getDouble() : 0.0);
	}

	WProperty* setDouble(const char* id, double value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProperty::createDoubleProperty(id, id);
			setting->setDouble(value);
			add(setting);
		} else {
			setting->setDouble(value);
		}
		return setting;
	}

	const char* getString(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->c_str() : "");
	}

	WProperty* setString(const char* id, const char* value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProperty::createStringProperty(id, id);
			setting->setString(value);
			add(setting);
		} else  {
			setting->setString(value);
		}
		return setting;
	}

	WProperty* setByteArray(const char* id, const byte* value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, BYTE_ARRAY, "");
			setting->setByteArray(value);
			setting->setVisibility(NONE);
			add(setting);
		} else {
			setting->setByteArray(value);
		}
		return setting;
	}

	static unsigned long getUnsignedLong(byte l1, byte l2, byte l3, byte l4) {
		return getLong(l1, l2, l3, l4);
	}

	static long getLong(byte l1, byte l2, byte l3, byte l4) {
		long four = l4;
		long three = l3;
		long two = l2;
		long one = l1;
		long value = ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
		return value;
	}

	static void getUnsignedLongBytes(unsigned long value, byte* dest) {
		return getLongBytes(value, dest);
	}

	static void getLongBytes(long value, byte* dest) {
		byte l1, l2, l3, l4;
		dest[3] = (value & 0xFF);
		dest[2] = ((value >> 8) & 0xFF);
		dest[1] = ((value >> 16) & 0xFF);
		dest[0] = ((value >> 24) & 0xFF);
	}

	bool addingNetworkSettings;
protected:
	WSettingItem* addSetting(WProperty* setting) {
		WSettingItem* settingItem = new WSettingItem();
		settingItem->value = setting;
		settingItem->networkSetting = this->addingNetworkSettings;
		if (this->lastSetting == nullptr) {
			this->firstSetting = settingItem;
			this->lastSetting = settingItem;
		} else {
			this->lastSetting->next = settingItem;
			this->lastSetting = settingItem;
		}
		return settingItem;
	}

	void save(WSettingItem* settingItem) {
		WProperty* setting = settingItem->value;
		/*
		Serial.print("Save prop: ");
		Serial.print(setting->getTitle());
		Serial.print(" / address: ");
		Serial.print(address);
		Serial.print(" / length: ");
		Serial.print(getLengthInEEPROM(setting));
		Serial.print(" / value: ");
		//log->notice(F("Save boolean to EEPROM: address=%d id='%s'; length=%d"), settingItem->address, setting->getId(), setting->getLength());
		*/
		switch (setting->getType()){
		case BOOLEAN: {
			EEPROM.write(address, (setting->getBoolean() ? 0xFF : 0x00));
			//Serial.println(setting->getBoolean());
			break;
		}
		case BYTE: {
			EEPROM.write(address, setting->getByte());
			//Serial.println(setting->getByte());
			break;
		}
		case SHORT: {
			EEPROM.put(address, setting->getShort());
			//Serial.println(setting->getShort());
			break;
		}
		case INTEGER: {
			EEPROM.put(address, setting->getInteger());
			//Serial.println(setting->getInteger());
			break;
		}
		case UNSIGNED_LONG: {
			EEPROM.put(address, setting->getUnsignedLong());
			//Serial.println(setting->getUnsignedLong());
			break;
		}
		case DOUBLE: {
			EEPROM.put(address, setting->getDouble());
			//Serial.println(setting->getDouble());
			break;
		}
		case BYTE_ARRAY: {
			writeByteArray(address, setting->getByteArray());
			//Serial.println("[]");
			break;
		}
		case STRING: {
			writeString(address, setting->c_str());
			//Serial.println(setting->c_str());
			break;
		}
		}
		address += this->getLengthInEEPROM(setting);
	}

private:
	WLog* log;
	bool existsSettingsApplication;
	int networkByte;
	byte appSettingsFlag;
	WSettingItem* firstSetting = nullptr;
	WSettingItem* lastSetting = nullptr;
	int address;
	bool readingFirstTime;

	byte getLengthInEEPROM(WProperty* setting) {
		switch (setting->getType()){
			case BYTE_ARRAY:
			case STRING:
			  return (setting->getLength() + 1);
		}
		return setting->getLength();
	}

	const byte* readByteArray(int adress) {
		byte length = EEPROM.read(address);
		byte* data = new byte[length];
		for (int i = 1; i <= length; i++) {
			byte k = EEPROM.read(address + i);
			data[i - 1] = k;
		}
		return data;
	}

	void writeByteArray(int address, const byte* value) {
		byte size = sizeof(value);
		EEPROM.write(address, size);
		for (int i = 1; i <= size; i++) {
			EEPROM.write(address + i, value[i - 1]);
		}
	}

	const char* readString(int address) {
		byte length = EEPROM.read(address);
		char* data = new char[length + 1];
		for (int i = 1; i <= length; i++) {
			byte k = EEPROM.read(address + i);
			data[i - 1] = k;
		}
		data[length] = '\0';
		return data;
	}

	void writeString(int address, const char* value) {
		byte size = strlen(value);
		EEPROM.write(address, size);
		for (int i = 1; i <= size; i++) {
			EEPROM.write(address + i, value[i - 1]);
		}
	}

	void saveEEPROM(int networkSettingsFlag) {
		EEPROM.begin(EEPROM_SIZE);
		this->address = 2;
		WSettingItem* settingItem = firstSetting;
		while (settingItem != nullptr) {
			save(settingItem);
			settingItem = settingItem->next;
		}
		//1. Byte - settingsStored flag
		EEPROM.write(0, networkSettingsFlag);
		EEPROM.write(1, this->appSettingsFlag);
		EEPROM.commit();
		EEPROM.end();
	}

};

#endif
