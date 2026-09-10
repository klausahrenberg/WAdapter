#ifndef WNetworkPages_h
#define WNetworkPages_h

#include "WebApp.h"

class WNetworkPage : public WebPage {
 public:
  WNetworkPage() : WebPage() {
  }

  virtual ~WNetworkPage() {
  }

  /** The same cards and rows the control panel is drawn with. */
  virtual WebControl* createControls() {
    WebControl* form = new WebForm(WC_WIFI, nullptr);
    form->add((new WebCard(PSTR("Device")))
                  ->addRow(PSTR("Id"), new WebInput(WC_ID, SETTINGS->getString(WC_ID), 16)));
    form->add((new WebCard(PSTR("Wifi"), PSTR("2.4G only")))
                  ->addRow(PSTR("SSID"), new WebInput(WC_SSID, SETTINGS->getString(WC_SSID), 32))
                  ->addRow(PSTR("Password"), new WebInput(WC_PASSWORD, SETTINGS->getString(WC_PASSWORD), 32, true)));
    form->add((new WebCard(PSTR("MQTT")))
                  ->addRow(PSTR("Server"), new WebInput(WC_MQTT_SERVER, SETTINGS->getString(WC_MQTT_SERVER), 32))
                  ->addRow(PSTR("Port"), new WebInput(WC_MQTT_PORT, SETTINGS->getString(WC_MQTT_PORT), 4))
                  ->addRow(PSTR("User"), new WebInput(WC_MQTT_USER, SETTINGS->getString(WC_MQTT_USER), 16))
                  ->addRow(PSTR("Password"), new WebInput(WC_MQTT_PASSWORD, SETTINGS->getString(WC_MQTT_PASSWORD), 32, true)));
    form->add(new WebSubmitButton(WC_SAVE_CONFIGURATION));
    return form;
  }

  virtual WFormResponse submitForm(WList<WValue>* args) {
    LOG->debug("submit %d", args->size());
    SETTINGS->setString(WC_ID, args->getById(WC_ID)->asString());
    SETTINGS->setString(WC_SSID, args->getById(WC_SSID)->asString());
    SETTINGS->setString(WC_PASSWORD, args->getById(WC_PASSWORD)->asString());
    SETTINGS->setString(WC_MQTT_SERVER, args->getById(WC_MQTT_SERVER)->asString());
    SETTINGS->setString(WC_MQTT_PORT, args->getById(WC_MQTT_PORT)->asString());
    SETTINGS->setString(WC_MQTT_USER, args->getById(WC_MQTT_USER)->asString());
    SETTINGS->setString(WC_MQTT_PASSWORD, args->getById(WC_MQTT_PASSWORD)->asString());
    SETTINGS->save();
    // delay(300);
    return WFormResponse(FO_RESTART, PSTR("Settings saved. If MQTT activated, subscribe to topic 'devices/#' at your broker."));
  }

 protected:
};

class WResetPage : public WebPage {
 public:
  WResetPage(WNetwork* network) : WebPage() {
  }

  virtual ~WResetPage() {
  }

  /** The same cards and rows the control panel is drawn with. */
  virtual WebControl* createControls() {
    WebControl* form = new WebForm(WC_RESET, nullptr);
    form->add((new WebCard(PSTR("Restart")))
                  ->addRow(PSTR("Restart device"), (new WebButton(PSTR("Restart")))->onClickSubmit("0"))
                  ->addRow(PSTR("Access point mode"), (new WebButton(PSTR("Restart")))->onClickSubmit("1")));
    form->add((new WebCard(PSTR("Settings")))
                  ->addRow(PSTR("Reset all settings"), (new WebButton(PSTR("Reset")))->danger()->onClickSubmit("2"))
                  ->addNote(PSTR("Wifi and MQTT settings are lost, the device comes back as an access point.")));
    return form;
  }

  virtual WFormResponse submitForm(WList<WValue>* args) {
    const char* v = args->getById(WC_VALUE)->asString();
    switch (v[0]) {
      case '0':
        return WFormResponse(FO_RESTART, PSTR("Restart was caused by web interface"));
      case '1':
        return WFormResponse(FO_FORCE_AP, PSTR("Restart device in AccessPoint mode"));
      case '2':
        return WFormResponse(FO_RESET_ALL, PSTR("All settings are resetted, device restarts"));
      default:
        return WFormResponse(FO_NONE);
    }
    return WFormResponse(FO_RESTART, PSTR("Settings saved. If MQTT activated, subscribe to topic 'devices/#' at your broker."));
  }

 private:
};

class WRestartPage : public WebPage {
 public:
  WRestartPage(const char* restartMessage) : WebPage() {
    _restartMessage = restartMessage;
  }

  virtual ~WRestartPage() {
  }

  virtual WebControl* createControls() {
    return (new WebCard(PSTR("Restart")))
                        ->addMessage(_restartMessage)
                        ->addNote(PSTR("The device reboots, the page is there again in a moment."));
  }

 private:
  const char* _restartMessage;
};

/** Bytes as they are written in every card of the firmware and the info page. */
static String _bytes(unsigned long value) { return String(value) + F(" bytes"); }

class WFirmwarePage : public WebPage {
 public:
  /** The same cards and rows the control panel is drawn with. */
  
  virtual WebControl* createControls() {
    WebDiv* result = new WebDiv();
    //the new firmware must fit into getFreeSketchSpace(): at esp8266 that's
    //what is left beside the running sketch, at esp32 the size of the next ota
    //partition. Assume the new firmware is up to 10% larger than the current one.
    unsigned long available = ESP.getFreeSketchSpace();
    bool enoughSpace = (available >= (ESP.getSketchSize() * 11 / 10));

    WebCard* installed = new WebCard(PSTR("Installed"), VERSION);
    if (APPLICATION != nullptr) installed->addRow(PSTR("Application"), APPLICATION);
    installed->addRow(PSTR("Current sketch size"), _bytes(ESP.getSketchSize()).c_str());
    installed->addRow(PSTR("Space for an update"), _bytes(available).c_str());

    if (enoughSpace) {
      WebControl* form = new WebForm(WC_FIRMWARE, nullptr);
      form->param(WC_ENCTYPE, WC_MULTIPART_FORM_DATA);
      result->add(form);
      form->add(installed);
      form->add((new WebCard(PSTR("Update")))
                    ->addContent(new WebInputFile("update", PSTR("Choose a firmware file (.bin)")))
                    ->addNote((String(F("The file can be up to ")) + available +
                               F(" bytes. The device restarts when the update is done."))
                                  .c_str()));
      form->add(new WebSubmitButton(PSTR("Install firmware")));
    } else {
      result->add(installed);
      result->add((new WebCard(PSTR("Update")))
                          ->addMessage(PSTR("There is not enough space left for an update over the air. "
                                            "The firmware has to be written over the serial port.")));
    }
    return result;
  }

  virtual WFormResponse submitForm(WList<WValue>* args) {
    LOG->debug("Update finished.");
    SETTINGS->save();
    if (Update.hasError()) {
#ifdef ARDUINO_ARCH_ESP8266
      LOG->debug("Error %s", Update.getErrorString().c_str());
#else
      LOG->debug("Error %s", Update.errorString());
#endif
    }
    return WFormResponse(FO_RESTART, (Update.hasError() ? PSTR("Some error during update") : PSTR("Update successful")));
  }
};

class WInfoPage : public WebPage {
 public:
  WInfoPage(unsigned long running) : WebPage() {
    _running = running;
  }

  virtual ~WInfoPage() {
  }

  /** The same cards and rows the control panel is drawn with. */
  virtual WebControl* createControls() {
    WebDiv* result = new WebDiv();

    WebCard* device = new WebCard(PSTR("Device"));
    result->add(device);
#ifdef ARDUINO_ARCH_ESP8266
    device->addRow(PSTR("Chip"), PSTR("ESP8266"));
#elif ARDUINO_ARCH_ESP32
    device->addRow(PSTR("Chip"), PSTR("ESP 32"));
#endif
    device->addRow(PSTR("Chip ID"), String(WUtils::getChipId()).c_str());
    device->addRow(PSTR("IDE flash size"), _bytes(ESP.getFlashChipSize()).c_str());
#ifdef ARDUINO_ARCH_ESP8266
    device->addRow(PSTR("Real flash size"), _bytes(ESP.getFlashChipRealSize()).c_str());
#endif
    device->addRow(PSTR("IP address"), deviceIp().c_str());
    device->addRow(PSTR("MAC address"), WiFi.macAddress().c_str());
    device->addRow(PSTR("Running since"), (String(_running) + F(" minutes")).c_str());

    WebCard* firmware = new WebCard(PSTR("Firmware"));
    result->add(firmware);
    firmware->addRow(PSTR("Current sketch size"), _bytes(ESP.getSketchSize()).c_str());
    firmware->addRow(PSTR("Available sketch size"), _bytes(ESP.getFreeSketchSpace()).c_str());

    WebCard* memory = new WebCard(PSTR("Memory"));
    result->add(memory);
    memory->addRow(PSTR("Free heap size"), _bytes(ESP.getFreeHeap()).c_str());
#ifdef ARDUINO_ARCH_ESP8266
    memory->addRow(PSTR("Largest heap block"), _bytes(ESP.getMaxFreeBlockSize()).c_str());
#endif
    return result;
  }

 private:
  unsigned long _running;
};

#endif
