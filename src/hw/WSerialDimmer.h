#ifndef W_SERIAL_DIMMER_H
#define W_SERIAL_DIMMER_H

#include "WGpio.h"

// Dimmer module with two channels, controlled over the serial port (9600 baud).
// Every change sends one frame, that always contains the levels of both channels:
// 0xFF 0x55 <changed channel 1|2> <level ch1> <level ch2> 0x05 0xDC 0x0A
// A level is 0x00 (off) up to 0xFF (full).
const static char WC_CHANNEL[] PROGMEM = "channel";
const unsigned long SERIAL_DIMMER_BAUD = 9600;

class WSerialDimmer : public WGpio {
 public:
  WSerialDimmer() : WGpio(GPIO_TYPE_SERIAL_DIMMER, NO_PIN, NO_MODE) {
  }

  virtual ~WSerialDimmer() {
    delete _channel;
    delete _id;
    delete _title;
  }

  virtual void registerSettings() {
    WGpio::registerSettings();
    SETTINGS->add(_channel, nullptr);
    SETTINGS->add(_id, nullptr);
    SETTINGS->add(_title, nullptr);
    _begin();
  }

  virtual void fromJson(WList<WValue>* list) {
    WGpio::fromJson(list);
    list->ifExistsId(WC_CHANNEL, [this](WValue* v) { channel(v->asByte()); });
    list->ifExistsId(WC_ID, [this](WValue* v) { _id->asString(v->asString()); });
    list->ifExistsId(WC_TITLE, [this](WValue* v) { _title->asString(v->asString()); });
    _begin();
  }

  virtual void toJson(WJson* json) {
    // no gpio, the serial port is used
    json->propertyString(WC_TYPE, S_GPIO_TYPE[_type], nullptr);
    json->propertyValue(WC_CHANNEL, _channel);
    json->property(WC_ID, _id);
    json->property(WC_TITLE, _title);
  }

  // 1 or 2
  byte channel() { return _channel->asByte(); }

  void channel(byte channel) { _channel->asByte(channel == 2 ? 2 : 1); }

  // a brightness property exists, if the dimmer has an id or a title
  bool isDimmable() { return ((!_id->isStringEmpty()) || (!_title->isStringEmpty())); }

  WValue* id() { return _id; }

  WValue* title() { return _title; }

  // brightness in percent
  byte level() { return _level; }

  void level(byte level) {
    _level = min(level, (byte)100);
    if (isOn()) _sendFrame();
  }

 protected:
  virtual void _updateOn() {
    _sendFrame();
  }

 private:
  WValue* _channel = new WValue((byte)1);
  WValue* _id = new WValue(WDataType::STRING);
  WValue* _title = new WValue(WDataType::STRING);
  byte _level = 100;

  // the last sent level of both channels
  static byte* _outputs() {
    static byte outputs[2] = {0, 0};
    return outputs;
  }

  void _begin() {
    // the serial port belongs to the dimmer now, a log would disturb the frames
    Serial.begin(SERIAL_DIMMER_BAUD);
    setDebug(false);
    LOG->setOutput(&Serial, LOG_LEVEL_SILENT, true, true);
  }

  void _sendFrame() {
    byte* outputs = _outputs();
    outputs[channel() - 1] = (isOn() ? (isDimmable() ? (byte)((unsigned int)_level * 0xFF / 100) : 0xFF) : 0x00);
    const byte frame[] = {0xFF, 0x55, channel(), outputs[0], outputs[1], 0x05, 0xDC, 0x0A};
    Serial.write(frame, sizeof(frame));
  }
};

#endif
