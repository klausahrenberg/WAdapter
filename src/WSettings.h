#ifndef W_SETTINGS_H
#define W_SETTINGS_H

#include "EEPROM.h"
#include "WLog.h"
#include "WList.h"
#include "WProps.h"

const byte FLAG_OPTIONS_NETWORK = 0x61;
const byte FLAG_OPTIONS_NETWORK_FORCE_AP = 0x65;
const int EEPROM_SIZE = 1024; // SPI_FLASH_SEC_SIZE;

class WSettings {
public:
	WSettings(WLog* log, byte appSettingsFlag = 0x68) {
		_items = new WList<WProperty>();
		_log = log;
		_appSettingsFlag = appSettingsFlag;		
		_address = 2;
		_readingFirstTime = true;
		EEPROM.begin(EEPROM_SIZE);
		_networkByte = EEPROM.read(0);
		_existsSettingsApplication = (EEPROM.read(1) == _appSettingsFlag);
	}

	void endReadingFirstTime() {
		if (isReadingFirstTime()) {
			EEPROM.end();
			_readingFirstTime = false;
		}
	}

	bool isReadingFirstTime() {
		return _readingFirstTime;
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
		return ((_networkByte == FLAG_OPTIONS_NETWORK) || (forceNetworkAccessPoint()));
	}

	bool forceNetworkAccessPoint() {
		return (_networkByte == FLAG_OPTIONS_NETWORK_FORCE_AP);
	}

	WProperty* getSetting(const char* id) {
		return _items->getIf([this, id](WProperty* p){return p->equalsId(id);});
	}

	bool exists(WProperty* property) {
		return _items->exists(property);
	}

	void add(WProperty* property) {
		this->add(property, false);
	}	

	void insert(WProperty* property, int index) {
		this->add(property, index, false);
	}	

	void add(WProperty* property, bool networkSetting) {
		add(property, _items->size(), networkSetting);
	}	

	void add(WProperty* property, int index, bool networkSetting) {	
		if (!exists(property)) {
			_items->insert(property, index);									
			if ((isReadingFirstTime()) &&
			    (((networkSetting) && (this->existsNetworkSettings())) ||
			     ((!networkSetting) && (_existsSettingsApplication)))) {
				switch (property->type()) {
				case BOOLEAN: {
					property->setBoolean(EEPROM.read(_address) == 0xFF);
					break;
				}
				case DOUBLE: {
					double d;
					EEPROM.get(_address, d);
					property->setDouble(d);
					break;
				}
				case SHORT: {
					short value = 0;
					EEPROM.get(_address, value);
					property->setShort(value);
					break;
				}
				case INTEGER: {
					int value = 0;
					EEPROM.get(_address, value);
					property->setInteger(value);
					break;
				}
				case UNSIGNED_LONG: {
					unsigned long value = 0;
					EEPROM.get(_address, value);
				  property->setUnsignedLong(value);
					break;
				}
				case BYTE: {
					int b2 = EEPROM.read(_address);
					property->setByte(b2);
					break;
				}
				case BYTE_ARRAY: {
					const byte* ba = readByteArray(_address);
					property->setByteArray(readByteArrayLength(_address), ba);
					delete ba;
					break;
				}
				case STRING: {
					const char* rs = readString(_address);
					property->setString(rs);
					delete rs;
					break;
				}
				}
				_address += this->getLengthInEEPROM(property);
			}
		}
	}

	void remove(const char* id) {
		_items->removeIf([this, id](WProperty* p){ return p->equalsId(id);});		
	}

	byte size() { return _items->size(); };

	bool getBoolean(const char* id) {
		WProperty* setting = getSetting(id);
		return (setting != nullptr ? setting->getBoolean() : false);
	}

	WProperty* setBoolean(const char* id, bool value) {
		return this->setBoolean(id, value, false);
	}

	WProperty* setNetworkBoolean(const char* id, bool value) {
		return this->setBoolean(id, value, true);
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
			setting = WProps::createByteProperty(id, id);
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
			setting = WProps::createIntegerProperty(id, id);
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
			setting = WProps::createShortProperty(id, id);
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
			setting = WProps::createUnsignedLongProperty(id, id);
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
			setting = WProps::createDoubleProperty(id, id);
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
		return this->setString(id, value, false);
	}

	WProperty* setNetworkString(const char* id, const char* value) {
		return this->setString(id, value, true);
	}

	WProperty* setByteArray(const char* id, byte length, const byte* value) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = new WProperty(id, id, BYTE_ARRAY, "");
			setting->setByteArray(length, value);
			setting->setVisibility(NONE);
			add(setting);
		} else {
			setting->setByteArray(length, value);
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

protected:

	WProperty* setBoolean(const char* id, bool value, bool networkSetting) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProps::createBooleanProperty(id, id);
			setting->setBoolean(value);
			add(setting, networkSetting);
		} else {
			setting->setBoolean(value);
		}
		return setting;
	}

	WProperty* setString(const char* id, const char* value, bool networkSetting) {
		WProperty* setting = getSetting(id);
		if (setting == nullptr) {
			setting = WProps::createStringProperty(id, id);
			setting->setString(value);
			add(setting, networkSetting);
		} else  {
			setting->setString(value);
		}
		return setting;
	}

	void save(WProperty* setting) {
		switch (setting->type()){
		case BOOLEAN: {
			EEPROM.write(_address, (setting->getBoolean() ? 0xFF : 0x00));
			break;
		}
		case BYTE: {
			EEPROM.write(_address, setting->getByte());
			break;
		}
		case SHORT: {
			EEPROM.put(_address, setting->getShort());
			break;
		}
		case INTEGER: {
			EEPROM.put(_address, setting->getInteger());
			break;
		}
		case UNSIGNED_LONG: {
			EEPROM.put(_address, setting->getUnsignedLong());
			break;
		}
		case DOUBLE: {
			EEPROM.put(_address, setting->getDouble());
			break;
		}
		case BYTE_ARRAY: {
			writeByteArray(_address, setting->getByteArrayLength(), setting->getByteArray());
			break;
		}
		case STRING: {						
			writeString(_address, setting->c_str());			
			break;
		}
		}
		_address += this->getLengthInEEPROM(setting);
	}

private:
	WLog* _log;
	bool _existsSettingsApplication;
	int _networkByte;
	byte _appSettingsFlag;
	WList<WProperty>* _items;
	int _address;
	bool _readingFirstTime;

	byte getLengthInEEPROM(WProperty* setting) {
		switch (setting->type()){
			case BYTE_ARRAY:
			case STRING:
			  return (setting->getLength() + 1);
		}
		return setting->getLength();
	}

	const byte readByteArrayLength(int address) {
		return EEPROM.read(address);
	}	

	const byte* readByteArray(int address) {
		byte length = EEPROM.read(address);
		byte* data = new byte[length];
		for (int i = 1; i <= length; i++) {
			byte k = EEPROM.read(address + i);
			data[i - 1] = k;
		}
		return data;
	}

	void writeByteArray(int address, byte length, const byte* value) {		
		EEPROM.write(address, length);
		for (int i = 1; i <= length; i++) {
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
		_address = 2;
		_items->forEach([this](WProperty* setting){	
			this->save(setting);
		});
		//1. Byte - settingsStored flag
		EEPROM.write(0, networkSettingsFlag);
		EEPROM.write(1, _appSettingsFlag);		
		EEPROM.commit();
		EEPROM.end();
	}

};

#endif
