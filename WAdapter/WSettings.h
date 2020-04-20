#ifndef W_SETTINGS_H
#define W_SETTINGS_H

#include "EEPROM.h"
#include "WLog.h"
#include "WProperty.h"

const byte STORED_FLAG = 0x63;
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
				case UNSIGNED_LONG: {
					unsigned long value = getUnsignedLong(EEPROM.read(settingItem->address), EEPROM.read(settingItem->address + 1),
					                                      EEPROM.read(settingItem->address + 2), EEPROM.read(settingItem->address + 3));
				  property->setUnsignedLong(value);
					break;
				}
				case LONG: {
					long value = getUnsignedLong(EEPROM.read(settingItem->address), EEPROM.read(settingItem->address + 1),
					                             EEPROM.read(settingItem->address + 2), EEPROM.read(settingItem->address + 3));
					property->setLong(value);
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

	long getLong(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getLong() : 0);
	}

	WProperty* setLong(const char* id, long value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, LONG);
			setting->setLong(value);
			add(setting);
		} else {
			setting->setLong(value);
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
			setting = new WProperty(id, id, UNSIGNED_LONG);
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
		case UNSIGNED_LONG:
			byte ulValues[4];
			getUnsignedLongBytes(setting->getUnsignedLong(), ulValues);
			EEPROM.write(settingItem->address, ulValues[3]);
			EEPROM.write(settingItem->address + 1, ulValues[2]);
			EEPROM.write(settingItem->address + 2, ulValues[1]);
			EEPROM.write(settingItem->address + 3, ulValues[0]);
			break;
		case LONG:
			byte bValues[4];
			getLongBytes(setting->getLong(), bValues);
			EEPROM.write(settingItem->address, bValues[3]);
			EEPROM.write(settingItem->address + 1, bValues[2]);
			EEPROM.write(settingItem->address + 2, bValues[1]);
			EEPROM.write(settingItem->address + 3, bValues[0]);
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
