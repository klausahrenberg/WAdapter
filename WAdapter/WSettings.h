#ifndef W_SETTINGS_H
#define W_SETTINGS_H

#include "EEPROM.h"
#include "WLog.h"
#include "WProperty.h"

const byte STORED_FLAG_OLDLOW = 0x59;
const byte STORED_FLAG_OLDHIGH = 0x63;
const byte STORED_FLAG = 0xF0;
const int EEPROM_SIZE = 512;
const int ADDRESS_COMPAT_MAXSTARTADDRESS = 128; // 64 IDX +32  SSID + 32 PSK

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
		uint8_t epromStored = EEPROM.read(0);
		this->_existsSettingsCompat = (epromStored >= STORED_FLAG_OLDLOW && epromStored <= STORED_FLAG_OLDHIGH);
		this->_existsSettings = (epromStored == STORED_FLAG || this->_existsSettingsCompat);
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
	
	bool existsSettingsCompat() {
		return this->_existsSettingsCompat;
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
			if (existsSettings() && (!existsSettingsCompat() || settingItem->address <= ADDRESS_COMPAT_MAXSTARTADDRESS)) {
				if (settingItem->address + property->getLength() > EEPROM_SIZE){
					log->error(F("Cannot add EPROM property. Size too small, Adrress=%d, Id='%s', size=%d, MEM: %d"),
					settingItem->address, property->getId(), property->getLength(), EEPROM_SIZE);
					return;
				}
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
				case LONG: {
					long four = EEPROM.read(settingItem->address);
					long three = EEPROM.read(settingItem->address + 1);
					long two = EEPROM.read(settingItem->address + 2);
					long one = EEPROM.read(settingItem->address + 3);
					long value = ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
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
		if (settingItem->address + setting->getLength() > EEPROM_SIZE){
			log->error(F("Cannot save to EPROM. Size too small, Adrress=%d, Id='%s', size=%d, MEM: %d"),
				settingItem->address, setting->getId(), setting->getLength(), EEPROM_SIZE);
			return;
		}
		switch (setting->getType()){
		case BOOLEAN:
			//log->notice(F("Save boolean to EEPROM: id='%s'; value=%d"), setting->getId(), setting->getBoolean());
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
		case LONG:
			byte l1, l2, l3, l4;
			l4 = (setting->getLong() & 0xFF);
			l3 = ((setting->getLong() >> 8) & 0xFF);
			l2 = ((setting->getLong() >> 16) & 0xFF);
			l1 = ((setting->getLong() >> 24) & 0xFF);
			EEPROM.write(settingItem->address, l4);
			EEPROM.write(settingItem->address + 1, l3);
			EEPROM.write(settingItem->address + 2, l2);
			EEPROM.write(settingItem->address + 3, l1);
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
	bool _existsSettingsCompat;
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
