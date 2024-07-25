#ifndef WNetworkPages_h
#define WNetworkPages_h

#include "WPage.h"

class WRootPage : public WPage {
 public:
  WRootPage(WList<WPageItem>* customPages) : WPage() {
    _customPages = customPages;
  }

  virtual ~WRootPage() {

  }  

  virtual void createControls(WebControl* parentNode) {    
    WebControl* div = new WebControl(WC_DIV, WC_CLASS, WC_WHITE_BOX, nullptr);    
    parentNode->add(div); 
    _customPages->forEach([this, div](WPageItem* pageItem, const char* id) {
      if (pageItem->showInMainMenu) {
        div->add(new WebDiv((new WebButton(pageItem->title))->onClickNavigateTo(id)));      
      }
    });
  }
 private:
  WList<WPageItem>* _customPages;

};

class WNetworkPage : public WPage {
 public: 
  WNetworkPage() : WPage() {
    
  }

  virtual ~WNetworkPage() {

  }  

  virtual void createControls(WebControl* parentNode) {    

    WebControl* form = new WebForm(WC_WIFI, nullptr);
    parentNode->add(form);
    //network
    form->add((new WebTextField(WC_ID, PSTR("Id"), SETTINGS->getString(WC_ID), 16)));
    form->add(new WebTextField(WC_SSID, PSTR("Wifi SSID (only 2.4G)"), SETTINGS->getString(WC_SSID), 32));
    form->add(new WebTextField(WC_PASSWORD, PSTR("Wifi password"), SETTINGS->getString(WC_PASSWORD), 32, true));
    //mqtt
    form->add((new WebTextField(WC_MQTT_SERVER, PSTR("MQTT Server"), SETTINGS->getString(WC_MQTT_SERVER), 32)));
    form->add((new WebTextField(WC_MQTT_PORT, PSTR("MQTT Port"), SETTINGS->getString(WC_MQTT_PORT), 4)));
    form->add(new WebTextField(WC_MQTT_USER, PSTR("MQTT User"), SETTINGS->getString(WC_MQTT_USER), 16));
    form->add(new WebTextField(WC_MQTT_PASSWORD, PSTR("MQTT password"), SETTINGS->getString(WC_MQTT_PASSWORD), 32, true));

    form->add((new WebSubmitButton(PSTR("Save configuration"))));
  }  

  virtual WFormResponse* submitForm(WList<WValue>* args) {
    SETTINGS->setString(WC_ID, args->getById(WC_ID)->asString());
    SETTINGS->setString(WC_SSID, args->getById(WC_SSID)->asString());
    SETTINGS->setString(WC_PASSWORD, args->getById(WC_PASSWORD)->asString());
    SETTINGS->setString(WC_MQTT_SERVER, args->getById(WC_MQTT_SERVER)->asString());
    SETTINGS->setString(WC_MQTT_PORT, args->getById(WC_MQTT_PORT)->asString());
    SETTINGS->setString(WC_MQTT_USER, args->getById(WC_MQTT_USER)->asString());
    SETTINGS->setString(WC_MQTT_PASSWORD, args->getById(WC_MQTT_PASSWORD)->asString());
    SETTINGS->save();
    //delay(300);
    return new WFormResponse(FO_RESTART, PSTR("Settings saved. If MQTT activated, subscribe to topic 'devices/#' at your broker."));   
  }

 protected:

};

class WResetPage : public WPage {
 public: 
  WResetPage(WNetwork* network) : WPage() {
    
  }

  virtual ~WResetPage() {

  }  

  virtual void createControls(WebControl* parentNode) {
    WebControl* div = new WebControl(WC_DIV, WC_CLASS, WC_WHITE_BOX, nullptr);
    parentNode->add(div);
    div->add(new WebDiv((new WebButton(PSTR("Restart")))->onClickSendValue("0")));    
    div->add(new WebDiv((new WebButton(PSTR("Restart in AccessPoint mode")))->onClickSendValue("1")));
    div->add(new WebDiv((new WebButton(PSTR("Reset all settings")))->onClickSendValue("2")));
    div->add(new WebDiv((new WebButton(WC_BACK_TO_MAINMENU))->onClickNavigateTo(WC_CONFIG)));
  }

  virtual WFormResponse* submitForm(WList<WValue>* args) {
    const char* v = args->getById(WC_VALUE)->asString();
    switch (v[0]) {
      case '0' : return new WFormResponse(FO_RESTART, PSTR("Restart was caused by web interface"));        
      case '1' : return new WFormResponse(FO_FORCE_AP, PSTR("Restart device in AccessPoint mode"));        
      case '2' : return new WFormResponse(FO_RESET_ALL, PSTR("All settings are resetted, device restarts"));        
      default : return new WFormResponse(FO_NONE);
    }
    return new WFormResponse(FO_RESTART, PSTR("Settings saved. If MQTT activated, subscribe to topic 'devices/#' at your broker."));   
  }

 private:
  
};

class WRestartPage : public WPage {
 public: 
  WRestartPage(const char* restartMessage) : WPage() {
    _restartMessage = restartMessage;
  }

  virtual ~WRestartPage() {

  }  

  virtual void createControls(WebControl* parentNode) {
    WebControl* div = new WebControl(WC_DIV, WC_CLASS, WC_WHITE_BOX, nullptr);
    parentNode->add(div);    
    div->add(new WebLabel(_restartMessage));
    div->add(new WebLabel(PSTR("ESP reboots now...")));
    div->add(new WebDiv((new WebButton(PSTR("Back to configuration")))->onClickNavigateTo(WC_CONFIG)));
  }
 private:
  const char* _restartMessage;
  
};

class WFirmwarePage : public WPage {
 public:
  virtual void createControls(WebControl* parentNode) {    

    WebControl* form = new WebForm(WC_FIRMWARE, nullptr);
    form->addParam(WC_ENCTYPE, WC_MULTIPART_FORM_DATA);
    parentNode->add(form);    
    //network
    form->add(new WebInputFile("update"));

    form->add(new WebSubmitButton(PSTR("Save configuration")));
  
  }  

  virtual WFormResponse* submitForm(WList<WValue>* args) {
    LOG->debug("Update finished.");
    SETTINGS->save();
    return new WFormResponse(FO_RESTART, (Update.hasError() ? PSTR("Some error during update") : PSTR("Update successful")));
  }  

};  


class WInfoPage : public WPage {
 public: 
  WInfoPage(unsigned long running) : WPage() {    \
    _running = running;
  }

  virtual ~WInfoPage() {    
  }  

  virtual void createControls(WebControl* parentNode) {
    WebControl* div = new WebControl(WC_DIV, WC_CLASS, WC_WHITE_BOX, nullptr);
    parentNode->add(div);
    WList<WValue>* datas = new WList<WValue>();
#ifdef ESP8266    
    datas->add(new WValue("ESP8266"), PSTR("Chip"));      
#elif ESP32
    datas->add(WProps::createStringProperty()->asString("ESP 32"), PSTR("Chip"));      
#endif
    datas->add(new WValue(WUtils::getChipId()), PSTR("Chip ID"));      
    datas->add(new WValue(ESP.getFlashChipSize()), PSTR("IDE Flash Size"));      
    datas->add(new WValue(ESP.getFlashChipRealSize()), PSTR("Real Flash Size")); 
    //datas->add(WProps::create..., PSTR("IP address"));      
    //datas->add(WProps::createStringProperty()->asString(WiFi.macAddress()), PSTR("MAC address"));
    datas->add(new WValue(ESP.getSketchSize()), PSTR("Current sketch size"));      
    datas->add(new WValue(ESP.getFreeSketchSpace()), PSTR("Available sketch size"));      
    datas->add(new WValue(ESP.getFreeHeap()), PSTR("Free heap size"));    
#ifdef ESP8266            
    datas->add(new WValue(ESP.getMaxFreeBlockSize()), PSTR("Largest heap block"));        
#endif           
    datas->add(new WValue(_running)/*->unit(PSTR(" minutes"))*/, PSTR("Running since"));        

    div->add(new WebTable(datas));
    div->add(new WebDiv((new WebButton(WC_BACK_TO_MAINMENU))->onClickNavigateTo(WC_CONFIG)));
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
  unsigned long _running;
  
};

#endif