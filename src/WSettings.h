#ifndef W_SETTINGS_H
#define W_SETTINGS_H

#include "EEPROM.h"
#include "WList.h"
#include "WLog.h"

const byte FLAG_OPTIONS_NETWORK = 0x62;
const byte FLAG_OPTIONS_NETWORK_FORCE_AP = 0x65;
const int EEPROM_SIZE = 1024;  // SPI_FLASH_SEC_SIZE;

class WSettings {
 public:
  WSettings() {
    _items = new WList<WValue>();
    _address = 2;
    _readingFirstTime = true;
    EEPROM.begin(EEPROM_SIZE);
    _networkByte = EEPROM.read(0);
    _existsSettingsApplication = (EEPROM.read(1) == FLAG_SETTINGS);
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

  void changeId(const char* id, const char* newId) { _items->changeId(id, newId); }

  WValue* getById(const char* id) { return _items->getById(id); }

  bool existsSetting(const char* id) { return (_items->getById(id) != nullptr); }

  bool existsNetworkSettings() {
    return ((_networkByte == FLAG_OPTIONS_NETWORK) ||
            (forceNetworkAccessPoint()));
  }

  bool forceNetworkAccessPoint() {
    return (_networkByte == FLAG_OPTIONS_NETWORK_FORCE_AP);
  }

  void add(WValue* value, const char* id) { this->add(value, id, false); }

  void insert(WValue* value, int index, const char* id) {
    add(value, index, id, false);
  }

  void add(WValue* value, const char* id, bool networkSetting) {
    add(value, _items->size(), id, networkSetting);
  }

  void add(WValue* value, int index, const char* id, bool networkSetting) {
    if (!_items->exists(value)) {
      _items->insert(value, index, id);
      // read stored values
      if ((isReadingFirstTime()) &&
          (((networkSetting) && (this->existsNetworkSettings())) ||
           ((!networkSetting) && (_existsSettingsApplication)))) {
        switch (value->type()) {
          case BOOLEAN: {
            value->asBool(EEPROM.read(_address) == 0xFF);
            break;
          }
          case DOUBLE: {
            double d;
            EEPROM.get(_address, d);
            value->asDouble(d);
            break;
          }
          case SHORT: {
            short s = 0;
            EEPROM.get(_address, s);
            value->asShort(s);
            break;
          }
          case INTEGER: {
            int i = 0;
            EEPROM.get(_address, i);
            value->asInt(i);
            break;
          }
          case UNSIGNED_LONG: {
            unsigned long l = 0;
            EEPROM.get(_address, l);
            value->asUnsignedLong(l);
            break;
          }
          case BYTE: {
            int b2 = EEPROM.read(_address);
            value->asByte(b2);
            break;
          }
          case BYTE_ARRAY: {
            const byte* ba = readByteArray(_address);
            value->asByteArray(readByteArrayLength(_address), ba);
            delete ba;
            break;
          }
          case STRING: {
            const char* rs = readString(_address);
            value->asString(rs);
            delete rs;
            break;
          }
        }
        _address += this->getLengthInEEPROM(value);
      }
    }
  }

  void removeAllAfter(const char* id) { _items->removeAllAfter(id); }

  void remove(const char* id) { _items->removeById(id); }

  byte size() { return _items->size(); };

  bool getBoolean(const char* id) {
    WValue* value = _items->getById(id);
    return (value != nullptr ? value->asBool() : false);
  }

  WValue* setBoolean(const char* id, bool value) {
    return this->setBoolean(id, value, false);
  }

  WValue* setNetworkBoolean(const char* id, bool value) {
    return this->setBoolean(id, value, true);
  }

  byte getByte(const char* id) {
    WValue* value = _items->getById(id);
    return (value != nullptr ? value->asByte() : 0x00);
  }

  WValue* setByte(const char* id, byte b, byte max = 0xFF) {
    WValue* value = _items->getById(id);
    if (value == nullptr) {
      value = new WValue(b);
      add(value, id);
      if ((max > 0) && (value->asByte() > max)) {
        value->asByte(max);
      }
    } else {
      value->asByte(b);
    }
    return value;
  }

  int getInteger(const char* id) {
    WValue* value = _items->getById(id);
    return (value != nullptr ? value->asInt() : 0);
  }

  WValue* setInteger(const char* id, int i) {
    WValue* value = _items->getById(id);
    if (value == nullptr) {
      value = new WValue(i);
      add(value, id);
    } else {
      value->asInt(i);
    }
    return value;
  }

  short getShort(const char* id) {
    WValue* value = _items->getById(id);
    return (value != nullptr ? value->asShort() : 0);
  }

  WValue* setShort(const char* id, short s) {
    WValue* value = _items->getById(id);
    if (value == nullptr) {
      value = new WValue(s);
      add(value, id);
    } else {
      value->asShort(s);
    }
    return value;
  }

  unsigned long getUnsignedLong(const char* id) {
    WValue* value = _items->getById(id);
    return (value != nullptr ? value->asUnsignedLong() : 0);
  }

  WValue* setUnsignedLong(const char* id, unsigned long ul) {
    WValue* value = _items->getById(id);
    if (value == nullptr) {
      value = new WValue(ul);
      add(value, id);
    } else {
      value->asUnsignedLong(ul);
    }
    return value;
  }

  double getDouble(const char* id) {
    WValue* value = _items->getById(id);
    return (value != nullptr ? value->asDouble() : 0.0);
  }

  WValue* setDouble(const char* id, double d) {
    WValue* value = _items->getById(id);
    if (value == nullptr) {
      value = new WValue(d);
      add(value, id);
    } else {
      value->asDouble(d);
    }
    return value;
  }

  const char* getString(const char* id) {    
    WValue* value = _items->getById(id);
    return (value != nullptr ? value->c_str() : "");
  }

  WValue* setString(const char* id, const char* value) {
    return this->setString(id, value, false);
  }

  WValue* setNetworkString(const char* id, const char* value) {
    return this->setString(id, value, true);
  }

  WValue* setByteArray(const char* id, byte length, const byte* ba) {
    WValue* value = _items->getById(id);
    if (value == nullptr) {
      value = new WValue(length, ba);      
      add(value, id);
    } else {
      value->asByteArray(length, ba);
    }
    return value;
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
  WValue* setBoolean(const char* id, bool b, bool networkSetting) {
    WValue* value = _items->getById(id);
    if (value == nullptr) {
      value = new WValue(b);
      add(value, id, networkSetting);
    } else {
      value->asBool(b);
    }
    return value;
  }

  WValue* setString(const char* id, const char* s, bool networkSetting) {        
    WValue* value = _items->getById(id);
    if (value == nullptr) {
      value = new WValue(s);
      add(value, id, networkSetting);
    } else {
      value->asString(s);
    }
    return value;
  }  

  void _save(int address, WValue* value) {
    switch (value->type()) {
      case BOOLEAN: {
        EEPROM.write(address, (value->asBool() ? 0xFF : 0x00));
        break;
      }
      case BYTE: {
        EEPROM.write(address, value->asByte());
        break;
      }
      case SHORT: {
        EEPROM.put(address, value->asShort());
        break;
      }
      case INTEGER: {
        EEPROM.put(address, value->asInt());
        break;
      }
      case UNSIGNED_LONG: {
        EEPROM.put(address, value->asUnsignedLong());
        break;
      }
      case DOUBLE: {
        EEPROM.put(address, value->asDouble());
        break;
      }
      case BYTE_ARRAY: {
        writeByteArray(address, value->length(),  value->asByteArray());
        break;
      }
      case STRING: {
        writeString(address, value->asString());
        break;
      }
    }    
  }

 private:
  bool _existsSettingsApplication;
  int _networkByte;
  WList<WValue>* _items;
  int _address;
  bool _readingFirstTime;

  byte getLengthInEEPROM(WValue* setting) {
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
    byte size = (value != nullptr ? strlen(value) : 0);
    EEPROM.write(address, size);
    for (int i = 1; i <= size; i++) {
      EEPROM.write(address + i, value[i - 1]);
    }
  }

  void _saveEEPROM(int networkSettingsFlag, WValue* specificSetting = nullptr) {
    EEPROM.begin(EEPROM_SIZE);
    _address = 2;
    _items->forEach([this, specificSetting](int index, WValue* setting, const char* id) { 
      if ((specificSetting == nullptr) || (specificSetting == setting)) {
				_save(_address, setting); 
			}
			_address += this->getLengthInEEPROM(setting);		
		});
    // 1. Byte - settingsStored flag
    EEPROM.write(0, networkSettingsFlag);
    EEPROM.write(1, FLAG_SETTINGS);
    EEPROM.commit();
    EEPROM.end();
  }
};

WSettings* SETTINGS;

#endif
