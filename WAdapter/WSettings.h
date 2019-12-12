#ifndef W_SETTINGS_H
#define W_SETTINGS_H

#include "EEPROM.h"
#include "WLog.h"
#include "WProperty.h"

const byte STORED_FLAG = 0x59;
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

	void save(WProperty* property) {
		WSettingItem* settingItem = firstSetting;
		while (settingItem != nullptr) {
			if (property == settingItem->value) {
				EEPROM.begin(EEPROM_SIZE);
				save(settingItem);
				EEPROM.commit();
				EEPROM.end();
			}
			settingItem = settingItem->next;
		}
	}

	void save() {
		EEPROM.begin(EEPROM_SIZE);
		WSettingItem* settingItem = firstSetting;
		while (settingItem != nullptr) {
			save(settingItem);
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
				case INTEGER: {
					byte low, high;
					low = EEPROM.read(settingItem->address);
					high = EEPROM.read(settingItem->address + 1);
					int value = (low + ((high << 8)&0xFF00));
					property->setInteger(value);
					break;
				}
				case BYTE:
					property->setByte(EEPROM.read(settingItem->address));
					break;
				case STRING:
					const char* rs = readString(settingItem->address, property->getLength());
					property->setString(rs);
					delete rs;
					break;
				}
				EEPROM.end();
			}
			property->setSettingsNotification([this](WProperty* property) {save(property);});
		}
	}

	bool getBoolean(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getBoolean() : false);
	}

	WProperty* setBoolean(const char* id, bool value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, BOOLEAN);
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
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, BYTE);
			setting->setByte(value);
			add(setting);
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
			setting = new WProperty(id, id, INTEGER);
			setting->setInteger(value);
			add(setting);
		} else {
			setting->setInteger(value);
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
			setting = new WProperty(id, id, DOUBLE);
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
		return setString(id, 32, value);
	}

	WProperty* setString(const char* id, byte length, const char* value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WStringProperty(id, id, length);
			setting->setString(value);
			add(setting);
		} else  {
			setting->setString(value);
		}
		return setting;
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

	void save(WSettingItem* settingItem) {
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

	}

private:
	WLog* log;
	bool _existsSettings;
	WSettingItem* firstSetting = nullptr;
	WSettingItem* lastSetting = nullptr;

	const char* readString(int address, int length) {
		char* data = new char[length+ 1]; //Max 100 Bytes
		int i = 0;
		for (i = 0; i <= length; i++) {
			if (i < length) {
				byte k = EEPROM.read(address + i);
				data[i] = k;
				if (k == '\0') {
					break;
				}
			} else {
				data[i] = '\0';
			}
		}
		return data;
	}

	void writeString(int address, int length, const char* value) {
		int size = strlen(value);
		if (size > length) {
			size = length;
		}
		for (int i = 0; i < size; i++) {
			EEPROM.write(address + i, value[i]);
		}
		if (size < length) {
			EEPROM.write(address + size, '\0');
		}
	}

};

#endif
