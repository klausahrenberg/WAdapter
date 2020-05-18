#ifndef W_SETTINGS_H
#define W_SETTINGS_H

#include "EEPROM.h"
#include "WLog.h"
#include "WProperty.h"

const byte FLAG_OPTIONS_NETWORK = 0x64;
const byte FLAG_OPTIONS_APPLICATION = 0x68;
const int EEPROM_SIZE = 512;

class WSettingItem {
public:
	WProperty* value;
	int address;
	bool networkSetting;
	WSettingItem* next = nullptr;
};

class WSettings {
public:
	WSettings(WLog* log) {
		this->log = log;
		this->addingNetworkSettings = true;
		this->saveOnPropertyChanges = true;
		EEPROM.begin(EEPROM_SIZE);
		this->existsSettingsNetwork = (EEPROM.read(0) == FLAG_OPTIONS_NETWORK);
		this->existsSettingsApplication = (EEPROM.read(1) == FLAG_OPTIONS_APPLICATION);
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
		EEPROM.write(0, FLAG_OPTIONS_NETWORK);
		EEPROM.write(1, FLAG_OPTIONS_APPLICATION);
		EEPROM.commit();
		EEPROM.end();
	}

	bool existsSetting(String id) {
		return (getSetting(id) != nullptr);
	}

	bool existsNetworkSettings() {
		return this->existsSettingsNetwork;
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
			if (((settingItem->networkSetting) && (this->existsSettingsNetwork)) ||
			    ((!settingItem->networkSetting) && (this->existsSettingsApplication))) {
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
				case SHORT: {
					short value = 0;
					EEPROM.get(settingItem->address, value);
					property->setShort(value);
					break;
				}
				case INTEGER: {
					int value = 0;
					EEPROM.get(settingItem->address, value);
					property->setInteger(value);
					break;
				}
				case UNSIGNED_LONG: {
					unsigned long value = 0;
					EEPROM.get(settingItem->address, value);
				  property->setUnsignedLong(value);
					break;
				}
				case BYTE:
					property->setByte(EEPROM.read(settingItem->address));
					break;
				case BYTE_ARRAY:
					for (byte i = 0; i < property->getLength(); i++) {
						property->setByteArrayValue(i, EEPROM.read(settingItem->address + i));
					}
					break;
				case STRING:
					const char* rs = readString(settingItem->address, property->getLength());
					property->setString(rs);
					delete rs;
					break;
				}
				EEPROM.end();
			}
			property->setSettingsNotification([this](WProperty* property) {if (saveOnPropertyChanges) save(property);});
		}
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
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProperty::createByteProperty(id, id);
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
		return setString(id, 32, value);
	}

	WProperty* setString(const char* id, byte length, const char* value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProperty::createStringProperty(id, id, length);
			setting->setString(value);
			add(setting);
		} else  {
			setting->setString(value);
		}
		return setting;
	}

	WProperty* setByteArray(const char* id, byte length, const byte* value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, BYTE_ARRAY, length, "");
			for (byte i = 0; i < length; i++) {
				setting->setByteArrayValue(i, value[i]);
			}
			setting->setVisibility(NONE);
			add(setting);
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
  bool saveOnPropertyChanges;
protected:
	WSettingItem* addSetting(WProperty* setting) {
		WSettingItem* settingItem = new WSettingItem();
		settingItem->value = setting;
		settingItem->networkSetting = this->addingNetworkSettings;
		if (this->lastSetting == nullptr) {
			settingItem->address = 2;
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
		//log->notice(F("Save boolean to EEPROM: address=%d id='%s'; length=%d"), settingItem->address, setting->getId(), setting->getLength());
		switch (setting->getType()){
		case BOOLEAN:
			EEPROM.write(settingItem->address, (setting->getBoolean() ? 0xFF : 0x00));
			break;
		case BYTE:
			EEPROM.write(settingItem->address, setting->getByte());
			break;
		case SHORT:
			EEPROM.put(settingItem->address, setting->getShort());
			break;
		case INTEGER:
			EEPROM.put(settingItem->address, setting->getInteger());
			break;
		case UNSIGNED_LONG:
			EEPROM.put(settingItem->address, setting->getUnsignedLong());
			break;
		case DOUBLE:
			EEPROM.put(settingItem->address, setting->getDouble());
			break;
		case BYTE_ARRAY:
			for (byte i = 0; i < setting->getLength(); i++) {
				EEPROM.write(settingItem->address + i, setting->getByteArrayValue(i));
			}
			break;
		case STRING:
			writeString(settingItem->address, setting->getLength(), setting->c_str());
			break;
		}

	}

private:
	WLog* log;
	bool existsSettingsNetwork, existsSettingsApplication;
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
