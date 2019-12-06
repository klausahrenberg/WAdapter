#ifndef W_SETTINGS_H
#define W_SETTINGS_H

#include "EEPROM.h"
#include "WLog.h"
#include "WProperty.h"

const byte STORED_FLAG = 0x16;
const int EEPROM_SIZE = 512;

class WSettingItem {
public:
	WProperty* value;
	int address;
	WSettingItem* next = nullptr;
};

class WSettings {
public:
	WSettings(WLog* log) {
		this->log = log;
		EEPROM.begin(EEPROM_SIZE);
		this->_existsSettings = (EEPROM.read(0) == STORED_FLAG);
		EEPROM.end();
	}

	void save() {
		EEPROM.begin(EEPROM_SIZE);
		WSettingItem* settingItem = firstSetting;
		while (settingItem != nullptr) {
			WProperty* setting = settingItem->value;
			switch (setting->getType()){
			case BOOLEAN:
				//log->notice(F("Save boolean to EEPROM: id='%s'; value=%T"), setting->getId(), setting->getBoolean());
				EEPROM.write(settingItem->address, (setting->getBoolean() ? 0xFF : 0x00));
				break;
			case BYTE:
				EEPROM.write(settingItem->address, setting->getByte());
				break;
			case INTEGER:
				byte low, high;
				low = (setting->getInteger() & 0xFF);
				high = ((setting->getInteger()>>8) & 0xFF);
				EEPROM.write(settingItem->address, low);
				EEPROM.write(settingItem->address + 1, high);
				break;
			case DOUBLE:
				EEPROM.put(settingItem->address, setting->getDouble());
				break;
			case STRING:
				writeString(settingItem->address, setting->getLength(), setting->c_str());
				break;
			}
			settingItem = settingItem->next;
		}
		//1. Byte - settingsStored flag
		EEPROM.write(0, STORED_FLAG);
		EEPROM.commit();
		EEPROM.end();
	}



	bool existsSetting(String id) {
		return (getSetting(id) != nullptr);
	}

	bool existsSettings() {
		return this->_existsSettings;
	}

	WProperty* getSetting(String id) {
		WSettingItem* settingItem = firstSetting;
		while (settingItem != nullptr) {
			WProperty* setting = settingItem->value;
			if (id.equals(setting->getId())) {
				return setting;
			}
			settingItem = settingItem->next;
		}
		return nullptr;
	}

	void add(WProperty* property) {
		if (getSetting(property->getId()) == nullptr) {
			WSettingItem* settingItem = addSetting(property);
			if (existsSettings()) {
				EEPROM.begin(EEPROM_SIZE);
				switch (property->getType()) {
				case BOOLEAN:
					property->setBoolean(EEPROM.read(settingItem->address) == 0xFF);
					break;
				case DOUBLE:
					double d;
					EEPROM.get(settingItem->address, d);
					property->setDouble(d);
					break;
				case INTEGER:
					byte low, high;
					low = EEPROM.read(settingItem->address);
					high = EEPROM.read(settingItem->address + 1);
					property->setInteger(low + ((high << 8)&0xFF00));
					break;
				case BYTE:
					property->setByte(EEPROM.read(settingItem->address));
					break;
				case STRING:
					String rs = readString(settingItem->address, property->getLength());
					property->setString(rs.c_str());
					break;
				}
				EEPROM.end();
			}
		}
	}

	WProperty* registerBoolean(const char* id, bool defaultValue) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, BOOLEAN);
			//setting->length = 1;
			WSettingItem* settingItem = addSetting(setting);
			if (existsSettings()) {
				EEPROM.begin(EEPROM_SIZE);
				setting->setBoolean(EEPROM.read(settingItem->address) == 0xFF);
				EEPROM.end();
			} else {
				setting->setBoolean(defaultValue);
			}
		}
		return setting;
	}

	bool getBoolean(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getBoolean() : false);
	}

	void setBoolean(const char* id, bool value) {
		WProperty* setting = registerBoolean(id, value);
		setting->setBoolean(value);
	}

	WProperty* registerByte(const char* id, byte defaultValue) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, BYTE);
			//setting->length = 1;
			WSettingItem* settingItem = addSetting(setting);
			if (existsSettings()) {
				EEPROM.begin(EEPROM_SIZE);
				setting->setByte(EEPROM.read(settingItem->address));
				EEPROM.end();
			} else {
				setting->setByte(defaultValue);
			}
		}
		return setting;
	}

	byte getByte(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getByte() : 0x00);
	}

	void setByte(const char* id, byte value) {
		WProperty* setting = registerByte(id, value);
		setting->setByte(value);
	}

	WProperty* registerInteger(const char* id, int defaultValue) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, INTEGER);
			//setting->length = 2;
			WSettingItem* settingItem = addSetting(setting);
			if (existsSettings()) {
				EEPROM.begin(EEPROM_SIZE);
				byte low, high;
				low = EEPROM.read(settingItem->address);
				high = EEPROM.read(settingItem->address + 1);
				setting->setInteger(low + ((high << 8)&0xFF00));
				EEPROM.end();
			} else {
				setting->setInteger(defaultValue);
			}
		}
		return setting;
	}

	int getInteger(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getInteger() : 0);
	}

	void setInteger(const char* id, int value) {
		WProperty* setting = registerInteger(id, value);
		setting->setInteger(value);
	}

	WProperty* registerDouble(const char* id, double defaultValue) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, DOUBLE);
			WSettingItem* settingItem = addSetting(setting);
			if (existsSettings()) {
				EEPROM.begin(EEPROM_SIZE);
				double d;
				EEPROM.get(settingItem->address, d);
				EEPROM.end();
				setting->setDouble(d);
			} else {
				setting->setDouble(defaultValue);
			}
		}
		return setting;
	}

	double getDouble(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getDouble() : 0.0);
	}

	void setDouble(const char* id, double value) {
		WProperty* setting = registerDouble(id, value);
		setting->setDouble(value);
	}

	WProperty* registerString(const char* id, byte length, const char* defaultValue) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, STRING, length);
			//setting->length = length + 1;
			WSettingItem* settingItem = addSetting(setting);
			if (existsSettings()) {
				EEPROM.begin(EEPROM_SIZE);
				const char* rs = readString(settingItem->address, setting->getLength());
				setting->setString(rs);
				EEPROM.end();
			} else {
				setting->setString(defaultValue);
			}
		}
		return setting;
	}

	const char* getString(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->c_str() : "");
	}

	void setString(const char* id, const char* value) {
		WProperty* setting = registerString(id, 32, value);
		setting->setString(value);
	}

protected:
	WSettingItem* addSetting(WProperty* setting) {
		WSettingItem* settingItem = new WSettingItem();
		settingItem->value = setting;
		if (this->lastSetting == nullptr) {
			settingItem->address = 1;
			this->firstSetting = settingItem;
			this->lastSetting = settingItem;
		} else {
			settingItem->address = this->lastSetting->address + this->lastSetting->value->getLength();
			this->lastSetting->next = settingItem;
			this->lastSetting = settingItem;
		}
		return settingItem;
	}
private:
	WLog* log;
	bool _existsSettings;
	WSettingItem* firstSetting = nullptr;
	WSettingItem* lastSetting = nullptr;

	const char* readString(int address, int length) {
		char* data = new char[length]; //Max 100 Bytes
		for (int i = 0; i < length; i++) {
			byte k = EEPROM.read(address + i);
			data[i] = k;
			if (k == '\0') {
				break;
			}
		}
		return data;
	}

	void writeString(int address, int length, const char* value) {
		int size = strlen(value);
		if (size + 1 >= length) {
			size = length - 1;
		}
		for (int i = 0; i < size; i++) {
			EEPROM.write(address + i, value[i]);
		}
		EEPROM.write(address + size, '\0');
	}

};

#endif
