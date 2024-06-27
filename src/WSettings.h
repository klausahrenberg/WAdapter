#ifndef W_SETTINGS_H
#define W_SETTINGS_H

#include "EEPROM.h"
#include "WList.h"
#include "WLog.h"
#include "WProps.h"

const byte FLAG_OPTIONS_NETWORK = 0x61;
const byte FLAG_OPTIONS_NETWORK_FORCE_AP = 0x65;
const int EEPROM_SIZE = 1024;  // SPI_FLASH_SEC_SIZE;

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

  bool isReadingFirstTime() { return _readingFirstTime; }

  void save() { _saveEEPROM(FLAG_OPTIONS_NETWORK); }

  void forceAPNextStart() { _saveEEPROM(FLAG_OPTIONS_NETWORK_FORCE_AP); }

  void resetAll() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(0, 0x00);
    EEPROM.write(1, 0x00);
    EEPROM.commit();
    EEPROM.end();
  }

  bool existsSetting(const char* id) { return (getSetting(id) != nullptr); }

  bool existsNetworkSettings() {
    return ((_networkByte == FLAG_OPTIONS_NETWORK) ||
            (forceNetworkAccessPoint()));
  }

  bool forceNetworkAccessPoint() {
    return (_networkByte == FLAG_OPTIONS_NETWORK_FORCE_AP);
  }

  WProperty* getSetting(const char* id) {
    return _items->getIf([this, id](WProperty* p) { return p->equalsId(id); });
  }

  bool exists(WProperty* property) { return _items->exists(property); }

  void add(WProperty* property) { this->add(property, false); }

  void insert(WProperty* property, int index) {
    this->add(property, index, false);
  }

  void add(WProperty* property, bool networkSetting) {
    add(property, _items->size(), networkSetting);
  }

  void add(WProperty* property, int index, bool networkSetting) {
    if (!exists(property)) {
      _items->insert(property, index);
      // read stored values
      if ((isReadingFirstTime()) &&
          (((networkSetting) && (this->existsNetworkSettings())) ||
           ((!networkSetting) && (_existsSettingsApplication)))) {
        switch (property->type()) {
          case BOOLEAN: {
            property->asBool(EEPROM.read(_address) == 0xFF);
            break;
          }
          case DOUBLE: {
            double d;
            EEPROM.get(_address, d);
            property->asDouble(d);
            break;
          }
          case SHORT: {
            short value = 0;
            EEPROM.get(_address, value);
            property->asShort(value);
            break;
          }
          case INTEGER: {
            int value = 0;
            EEPROM.get(_address, value);
            property->asInt(value);
            break;
          }
          case UNSIGNED_LONG: {
            unsigned long value = 0;
            EEPROM.get(_address, value);
            property->asUnsignedLong(value);
            break;
          }
          case BYTE: {
            int b2 = EEPROM.read(_address);
            property->asByte(b2);
            break;
          }
          case BYTE_ARRAY: {
            const byte* ba = readByteArray(_address);
            property->asByteArray(readByteArrayLength(_address), ba);
            delete ba;
            break;
          }
          case STRING: {
            const char* rs = readString(_address);
            property->asString(rs);
            delete rs;
            break;
          }
        }
        _address += this->getLengthInEEPROM(property);
      }
      // add listener to save user settings
      if (!networkSetting) {
        property->addListener([this, property]() {
          if (!_readingFirstTime) {
            _saveEEPROM(FLAG_OPTIONS_NETWORK, property);
          }
        });
      }
    }
  }

  void remove(const char* id) {
    _items->removeIf([this, id](WProperty* p) { return p->equalsId(id); });
  }

  byte size() { return _items->size(); };

  bool getBoolean(const char* id) {
    WProperty* setting = getSetting(id);
    return (setting != nullptr ? setting->asBool() : false);
  }

  WProperty* setBoolean(const char* id, bool value) {
    return this->setBoolean(id, value, false);
  }

  WProperty* setNetworkBoolean(const char* id, bool value) {
    return this->setBoolean(id, value, true);
  }

  byte getByte(const char* id) {
    WProperty* setting = getSetting(id);
    return (setting != nullptr ? setting->asByte() : 0x00);
  }

  WProperty* setByte(const char* id, byte value) {
    return setByte(id, value, 0);
  }

  WProperty* setByte(const char* id, byte value, byte max) {
    WProperty* setting = getSetting(id);
    if (setting == nullptr) {
      setting = WProps::createByteProperty(id, id);
      setting->asByte(value);
      add(setting);
      if ((max > 0) && (setting->asByte() > max)) {
        setting->asByte(value);
      }
    } else {
      setting->asByte(value);
    }
    return setting;
  }

  int getInteger(const char* id) {
    WProperty* setting = getSetting(id);
    return (setting != nullptr ? setting->asInt() : 0);
  }

  WProperty* setInteger(const char* id, int value) {
    WProperty* setting = getSetting(id);
    if (setting == nullptr) {
      setting = WProps::createIntegerProperty(id, id);
      setting->asInt(value);
      add(setting);
    } else {
      setting->asInt(value);
    }
    return setting;
  }

  short getShort(const char* id) {
    WProperty* setting = getSetting(id);
    return (setting != nullptr ? setting->asShort() : 0);
  }

  WProperty* setShort(const char* id, short value) {
    WProperty* setting = getSetting(id);
    if (setting == nullptr) {
      setting = WProps::createShortProperty(id, id);
      setting->asShort(value);
      add(setting);
    } else {
      setting->asShort(value);
    }
    return setting;
  }

  unsigned long getUnsignedLong(const char* id) {
    WProperty* setting = getSetting(id);
    return (setting != nullptr ? setting->asUnsignedLong() : 0);
  }

  WProperty* setUnsignedLong(const char* id, unsigned long value) {
    WProperty* setting = getSetting(id);
    if (setting == nullptr) {
      setting = WProps::createUnsignedLongProperty(id, id);
      setting->asUnsignedLong(value);
      add(setting);
    } else {
      setting->asUnsignedLong(value);
    }
    return setting;
  }

  double getDouble(const char* id) {
    WProperty* setting = getSetting(id);
    return (setting != nullptr ? setting->asDouble() : 0.0);
  }

  WProperty* setDouble(const char* id, double value) {
    WProperty* setting = getSetting(id);
    if (setting == nullptr) {
      setting = WProps::createDoubleProperty(id, id);
      setting->asDouble(value);
      add(setting);
    } else {
      setting->asDouble(value);
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
      setting->asByteArray(length, value);
      setting->visibility(NONE);
      add(setting);
    } else {
      setting->asByteArray(length, value);
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
    long value = ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) +
                 ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
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
      setting->asBool(value);
      add(setting, networkSetting);
    } else {
      setting->asBool(value);
    }
    return setting;
  }

  WProperty* setString(const char* id, const char* value, bool networkSetting) {
    WProperty* setting = getSetting(id);
    if (setting == nullptr) {
      setting = WProps::createStringProperty(id, id);
      setting->asString(value);
      add(setting, networkSetting);
    } else {
      setting->asString(value);
    }
    return setting;
  }

  void _save(int address, WProperty* setting) {
    switch (setting->type()) {
      case BOOLEAN: {
        EEPROM.write(address, (setting->asBool() ? 0xFF : 0x00));
        break;
      }
      case BYTE: {
        EEPROM.write(address, setting->asByte());
        break;
      }
      case SHORT: {
        EEPROM.put(address, setting->asShort());
        break;
      }
      case INTEGER: {
        EEPROM.put(address, setting->asInt());
        break;
      }
      case UNSIGNED_LONG: {
        EEPROM.put(address, setting->asUnsignedLong());
        break;
      }
      case DOUBLE: {
        EEPROM.put(address, setting->asDouble());
        break;
      }
      case BYTE_ARRAY: {
        writeByteArray(address, setting->byteArrayLength(),
                       setting->asByteArray());
        break;
      }
      case STRING: {
        writeString(address, setting->c_str());
        break;
      }
    }    
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
    switch (setting->type()) {
      case BYTE_ARRAY:
      case STRING:
        return (setting->length() + 1);
    }
    return setting->length();
  }

  const byte readByteArrayLength(int address) { return EEPROM.read(address); }

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

  void _saveEEPROM(int networkSettingsFlag, WProperty* specificSetting = nullptr) {
    EEPROM.begin(EEPROM_SIZE);
    _address = 2;
    _items->forEach([this, specificSetting](WProperty* setting) { 
			if ((specificSetting == nullptr) || (specificSetting == setting)) {
				_save(_address, setting); 
			}
			_address += this->getLengthInEEPROM(setting);		
		});
    // 1. Byte - settingsStored flag
    EEPROM.write(0, networkSettingsFlag);
    EEPROM.write(1, _appSettingsFlag);
    EEPROM.commit();
    EEPROM.end();
  }
};

#endif
