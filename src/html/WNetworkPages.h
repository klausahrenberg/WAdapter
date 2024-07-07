#ifndef WNetworkPages_h
#define WNetworkPages_h

#include "WPage.h"

class WRootPage : public WPage {
 public:
  WRootPage(WNetwork* network, WList<WPageItem>* customPages) : WPage() {
    _customPages = customPages;
  }

  ~WRootPage() {

  }  

  virtual void createControls(WebControl* parentNode) {
    WebControl* div = new WebControl(WC_DIV, WC_CLASS, WC_WHITE_BOX, nullptr);
    parentNode->add(div);
    div->add(new WebDiv((new WebButton(PSTR("wb"), PSTR("Configure network")))->onClickNavigateTo(PSTR("wifi2"))));
    _customPages->forEach([this, div](WPageItem* pageItem, const char* id) {
      if (pageItem->showInMainMenu) {
        div->add(new WebDiv((new WebButton(PSTR("wb"), id))->onClickNavigateTo(id)));      
      }
    });
    div->add(new WebDiv((new WebButton(PSTR("wb"), PSTR("Update firmware")))->onClickNavigateTo(PSTR("firmware"))));
    div->add(new WebDiv((new WebButton(PSTR("wb"), PSTR("Info")))->onClickNavigateTo(PSTR("info2"))));
    div->add(new WebDiv((new WebButton(PSTR("wb"), PSTR("Restart options")))->onClickNavigateTo(PSTR("reset"))));
  }
 private:
  WList<WPageItem>* _customPages;

};



class WNetworkPage : public WPage {
 public: 
  WNetworkPage(WSettings* settings) : WPage() {
    _settings = settings;
  }

  ~WNetworkPage() {

  }  

  virtual void createControls(WebControl* parentNode) {    

    WebControl* form = new WebForm(WC_WIFI, nullptr);
    parentNode->add(form);
    //network
    form->add((new WebTextField(WC_ID, PSTR("Id"), _settings->getString(WC_ID), 16)));
    form->add(new WebTextField(WC_SSID, PSTR("Wifi SSID (only 2.4G)"), _settings->getString(WC_SSID), 32));
    form->add(new WebTextField(WC_PASSWORD, PSTR("Wifi password"), _settings->getString(WC_PASSWORD), 32, true));
    //mqtt
    form->add((new WebTextField(WC_MQTT_SERVER, PSTR("MQTT Server"), _settings->getString(WC_MQTT_SERVER), 32)));
    form->add((new WebTextField(WC_MQTT_PORT, PSTR("MQTT Port"), _settings->getString(WC_MQTT_PORT), 4)));
    form->add(new WebTextField(WC_MQTT_USER, PSTR("MQTT User"), _settings->getString(WC_MQTT_USER), 16));
    form->add(new WebTextField(WC_MQTT_PASSWORD, PSTR("MQTT password"), _settings->getString(WC_MQTT_PASSWORD), 32, true));

    form->add(new WebSubmitButton(PSTR("Save configuration")));
    /*
      // advanced mqtt options
      page->printf(HTTP_CHECKBOX_OPTION, "sa", "sa", "", "tg()", "Advanced MQTT options");
      // HTTP_DIV(page, "ga");
      page->printf(HTTP_TEXT_FIELD, "MQTT Topic:", "mt", "32", mqttBaseTopic());
      page->printf(HTTP_TEXT_FIELD, "Topic for state requests:", "mtg", "16", mqttStateTopic());
      page->printf(HTTP_TEXT_FIELD, "Topic for setting values:", "mts", "16", mqttSetTopic());
      // HTTP_DIV_END(page);
      page->printf(HTTP_TOGGLE_FUNCTION_SCRIPT, "tg()", "sa", "ga", "gb");
      page->print(FPSTR(HTTP_CONFIG_SAVE_BUTTON));
      page->print(FPSTR(HTTP_BODY_END));
      request->send(page);*/
  }  

  virtual const char* submitForm(WList<const char>* args) {
    Serial.println("submit form");
    _settings->setString(WC_ID, args->getById(WC_ID));
    _settings->setString(WC_SSID, args->getById(WC_SSID));
    _settings->setString(WC_PASSWORD, args->getById(WC_PASSWORD));
    _settings->setString(WC_MQTT_SERVER, args->getById(WC_MQTT_SERVER));
    _settings->setString(WC_MQTT_PORT, args->getById(WC_MQTT_PORT));
    _settings->setString(WC_MQTT_USER, args->getById(WC_MQTT_USER));
    _settings->setString(WC_MQTT_PASSWORD, args->getById(WC_MQTT_PASSWORD));
    _settings->save();
		delay(300);
    return PSTR("Settings saved. If MQTT activated, subscribe to topic 'devices/#' at your broker.");   
    /*
			this->supportingMqtt->setBoolean(webServer->arg("sa") == HTTP_TRUE);
			this->mqttBaseTopic->setString(webServer->arg("mt").c_str());
			String subTopic = webServer->arg("mtg");
			if (subTopic.startsWith(SLASH)) subTopic.substring(1);
			if (subTopic.endsWith(SLASH)) subTopic.substring(0, subTopic.length() - 1);
			if (subTopic.equals("")) subTopic = DEFAULT_TOPIC_STATE;
			this->mqttStateTopic->setString(subTopic.c_str());
			subTopic = webServer->arg("mts");
			if (subTopic.startsWith(SLASH)) subTopic.substring(1);
			if (subTopic.endsWith(SLASH)) subTopic.substring(0, subTopic.length() - 1);
			if (subTopic.equals("")) subTopic = DEFAULT_TOPIC_SET;
			this->mqttSetTopic->setString(subTopic.c_str());
			settings->save();
			delay(300);
			this->restart("Settings saved. If MQTT activated, subscribe to topic 'devices/#' at your broker.");*/
  }

 protected:
  WSettings* _settings; 
};

class WResetPage : public WPage {
 public: 
  WResetPage(WNetwork* network) : WPage() {
    
  }

  ~WResetPage() {

  }  

  virtual void createControls(WebControl* parentNode) {
    WebControl* div = new WebControl(WC_DIV, WC_CLASS, WC_WHITE_BOX, nullptr);
    parentNode->add(div);

    div->add(new WebDiv((new WebButton(PSTR("w4StEi18X6"), PSTR("Reboot")))->onClick([](){Serial.println("should reboot now...");})));
    div->add(new WebDiv((new WebButton(PSTR("w4StEi18X7"), PSTR("Restart in AccessPoint mode")))->onClickNavigateTo(PSTR("firmware"))));
    div->add(new WebDiv((new WebButton(PSTR("w4StEi18X8"), PSTR("Reset all settings")))->onClickNavigateTo(PSTR("firmware"))));
    div->add(new WebDiv((new WebButton(PSTR("wb"), PSTR("Cancel")))->onClickNavigateTo(PSTR("config"))));

/*
page->printf(HTTP_HEAD_BEGIN, "Restart options");
      page->print(FPSTR(HTTP_STYLE));
      page->print(FPSTR(HTTP_HEAD_END));
      _printHttpCaption(page);
      page->printf(HTTP_BUTTON, "w4StEi18X6", "post", "Reboot");
      //HTTP_DIV(page);
      //HTTP_DIV_END(page);      
      page->printf(HTTP_BUTTON_ALERT, "w4StEi18X7", "post", "Restart in AccessPoint mode");
      page->printf(HTTP_BUTTON_ALERT, "w4StEi18X8", "post", "Reset all settings");
      //HTTP_DIV(page);
      //HTTP_DIV_END(page);
      page->printf(HTTP_BUTTON, "config", "get", "Cancel");
      page->print(FPSTR(HTTP_BODY_END));
      request->send(page);
*/

  }
 private:
  
};

class WRestartPage : public WPage {
 public: 
  WRestartPage(const char* restartMessage) : WPage() {
    _restartMessage = restartMessage;
  }

  ~WRestartPage() {

  }  

  virtual void createControls(WebControl* parentNode) {
    WebControl* div = new WebControl(WC_DIV, WC_CLASS, WC_WHITE_BOX, nullptr);
    parentNode->add(div);
    WebControl* label = new WebControl(WC_LABEL, nullptr);
    label->content(_restartMessage);
    div->add(label);
    WebControl* label2 = new WebControl(WC_LABEL, nullptr);
    label2->content(PSTR("ESP reboots now..."));
    div->add(label2);
    div->add(new WebDiv((new WebButton(PSTR("wb"), PSTR("Back to configuration")))->onClickNavigateTo(WC_CONFIG)));
  }
 private:
  const char* _restartMessage;
  
};

class WInfoPage : public WPage {
 public: 
  WInfoPage() : WPage() {
    
  }

  ~WInfoPage() {

  }  

  virtual void createControls(WebControl* parentNode) {
    WebControl* div = new WebControl(WC_DIV, WC_CLASS, WC_WHITE_BOX, nullptr);
    parentNode->add(div);
    
    WebControl* label2 = new WebControl(WC_LABEL, nullptr);
    label2->content(PSTR("ESP reboots now..."));
    div->add(label2);
    

    WList<const char>* datas = new WList<const char>();
#ifdef ESP8266
    datas->add("ESP 8266", "Chip:");      
#elif ESP32
    datas->add(PSTR("ESP 32"), PSTR("Chip:"));
#endif
    datas->add(WUtils::getChipId(), "Chip ID:");      

    div->add(new WebTable(datas));
    div->add(new WebDiv((new WebButton(PSTR("wb"), PSTR("Back to configuration")))->onClickNavigateTo(WC_CONFIG)));
    /*
    if (isWebServerRunning()) {
      AsyncResponseStream *page = request->beginResponseStream(WC_TEXT_HTML);
      page->printf(HTTP_HEAD_BEGIN, "Info");
      page->print(FPSTR(HTTP_STYLE));
      page->print(FPSTR(HTTP_HEAD_END));
      _printHttpCaption(page);
      page->print(F("<table>"));
      page->print(F("<tr><th>Chip:</th><td>"));
#ifdef ESP8266
      page->print(F("ESP 8266"));
#elif ESP32
      page->print(F("ESP 32"));
#endif
      page->print(F("</td></tr>"));
      page->print(F("<tr><th>Chip ID:</th><td>"));
      page->print(getChipId());
      page->print(F("</td></tr>"));
      page->print(F("<tr><th>IDE Flash Size:</th><td>"));
      page->print(ESP.getFlashChipSize());
      page->print(F("</td></tr>"));
#ifdef ESP8266
      page->print(F("<tr><th>Real Flash Size:</th><td>"));
      page->print(ESP.getFlashChipRealSize());
      page->print(F("</td></tr>"));
#endif
      page->print(F("<tr><th>IP address:</th><td>"));
      page->print(this->getDeviceIp().toString());
      page->print(F("</td></tr>"));
      page->print(F("<tr><th>MAC address:</th><td>"));
      page->print(WiFi.macAddress());
      page->print(F("</td></tr>"));

      page->print(F("<tr><th>Current sketch size:</th><td>"));
      page->print(ESP.getSketchSize());
      page->print(F("</td></tr>"));
      page->print(F("<tr><th>Available sketch size:</th><td>"));
      page->print(ESP.getFreeSketchSpace());
      page->print(F("</td></tr>"));

      page->print(F("<tr><th>Free heap size:</th><td>"));
      page->print(ESP.getFreeHeap());
      page->print(F("</td></tr>"));
#ifdef ESP8266
      page->print(F("<tr><th>Largest free heap block:</th><td>"));
      page->print(ESP.getMaxFreeBlockSize());
      page->print(F("</td></tr>"));
      page->print(F("<tr><th>Heap fragmentation:</th><td>"));
      page->print(ESP.getHeapFragmentation());
      page->print(F(" %</td></tr>"));
#endif
      page->print(F("<tr><th>Running since:</th><td>"));
      page->print(((millis() - _startupTime) / 1000 / 60));
      page->print(F(" minutes</td></tr>"));
      page->print(F("</table>"));
      page->printf(HTTP_BUTTON, "config", "get", "Main menu");
      page->print(FPSTR(HTTP_BODY_END));
      request->send(page);
    */
  }
 private:
  
};

#endif