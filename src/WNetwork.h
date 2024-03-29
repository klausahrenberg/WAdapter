#ifndef W_NETWORK_H
#define W_NETWORK_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#ifdef ESP8266
#include <ESP8266mDNS.h>
#include <Updater.h>
#define U_PART U_FS
#elif ESP32 
#include <ESPmDNS.h>
#include <Update.h>
#define U_PART U_SPIFFS
#endif
#include <DNSServer.h>
#include <StreamString.h>
#include <PubSubClient.h>
#include "WDevice.h"
#include "WHtmlPages.h"
#include "WJsonParser.h"
#include "WLed.h"
#include "WList.h"
#include "WLog.h"
#include "WPage.h"
#include "WSettings.h"
#include "WStringStream.h"
#include "WiFiClient.h"

#define SIZE_JSON_PACKET 1280
#define NO_LED -1
#define ESP_MAX_PUT_BODY_SIZE 512
#define WIFI_RECONNECTION 50000
#define WIFI_RECONNECTION_TRYS 3
const char *CONFIG_PASSWORD = "12345678";
const char *APPLICATION_JSON = "application/json";
const char *TEXT_HTML = "text/html";
const char *TEXT_PLAIN = "text/plain";
const char *DEFAULT_TOPIC_STATE = "properties";
const char *DEFAULT_TOPIC_SET = "set";
const char *SLASH = "/";

#ifdef ESP8266
WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
#endif

class WNetwork {
 public:
  typedef std::function<void()> THandlerFunction;

  WNetwork(bool debugging, String applicationName, String firmwareVersion,
           int statusLedPin, byte appSettingsFlag,
           Print *debuggingOutput = &Serial) {
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
#ifdef ESP8266
    WiFi.encryptionType(ENC_TYPE_CCMP);
    WiFi.setOutputPower(20.5);
    WiFi.setPhyMode(WiFiPhyMode::WIFI_PHY_MODE_11N);
#endif
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);

    _applicationName = applicationName;
    _firmwareVersion = firmwareVersion;
    _webServer = nullptr;
    _dnsApServer = nullptr;
    _wifiClient = new WiFiClient();
    _debugging = debugging;
    _devices = new WList<WDevice>();
    _pages = new WList<WPage>();
    _wlog = new WLog();
    this->setDebuggingOutput(debuggingOutput);
    _updateRunning = false;
    _restartFlag = "";
    _deepSleepFlag = nullptr;
    _waitForWifiConnection = false;
    _startupTime = millis();
    _mqttClient = nullptr;
    _settings = new WSettings(_wlog, appSettingsFlag);
    _loadNetworkSettings();
    _lastMqttConnect = _lastWifiConnect = 0;
    _initialMqttSent = false;
    _lastWillEnabled = true;
    _wifiConnectTrys = 0;

    if (this->isSupportingMqtt()) {
      WiFiClient *wifiClient = new WiFiClient();
      _mqttClient = new PubSubClient(*wifiClient);
      _mqttClient->setBufferSize(SIZE_JSON_PACKET);
      _mqttClient->setCallback(std::bind(&WNetwork::_mqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }
#ifdef ESP8266
    gotIpEventHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP &event) { onGotIP(); });
    disconnectedEventHandler = WiFi.onStationModeDisconnected(
        [this](const WiFiEventStationModeDisconnected &event) {
          onDisconnected();
        });
#elif ESP32
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) { onGotIP(); },
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) { onDisconnected(); },
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#endif
    _statusLed = nullptr;
    setStatusLedPin(statusLedPin, false);
    _wlog->debug(F("firmware: %s"), firmwareVersion.c_str());
  }

  void setStatusLedPin(int statusLedPin, bool statusLedOnIfConnected) {
    this->setStatusLed(
        (statusLedPin != NO_LED ? new WLed(statusLedPin) : nullptr),
        statusLedOnIfConnected);
  }

  void setStatusLed(WLed* statusLed, bool statusLedOnIfConnected) {
    _statusLedOnIfConnected = statusLedOnIfConnected;
    if (_statusLed != nullptr) {
      delete _statusLed;
    }
    _statusLed = nullptr;
    if (statusLed != nullptr) {
      _statusLed = statusLed;
      _statusLed->setOn(true, 500);
    }
    _updateLedState();
  }

  void onGotIP() {
    _wlog->notice(F("Station connected, IP: %s Host Name is '%s'"), this->getDeviceIp().toString().c_str(), _hostname);
    // Connect, if webThing supported and Wifi is connected as client
    if ((_supportsWebServer) && (isWifiConnected())) {
      this->startWebServer();
    }
    _wifiConnectTrys = 0;
    _notify(false);
  }

  void onDisconnected() {
    if (!isSoftAP()) {
      _wlog->notice("Station disconnected");
      this->disconnectMqtt();
      _lastMqttConnect = 0;
      this->stopWebServer();
      _notify(false);
    }
  }

  void startConfiguration() {
    if (WiFi.status() != WL_CONNECTED) {
      _wifiConnectTrys = WIFI_RECONNECTION_TRYS;
    }
  }

  // returns true, if no configuration mode and no own ap is opened
  bool loop(unsigned long now) {
    _settings->endReadingFirstTime();
    bool result = true;
    if (!isWebServerRunning()) {
      if (WiFi.status() != WL_CONNECTED) {        
        if ((!_settings->existsNetworkSettings()) ||
            (_settings->forceNetworkAccessPoint()) || (strcmp(getSsid(), "") == 0) ||
            (_wifiConnectTrys == WIFI_RECONNECTION_TRYS)) {
          // Create own AP
          String apSsid = this->apSsid();
          _wlog->notice(F("Start AccessPoint for configuration. SSID '%s'; password '%s'"), apSsid.c_str(), this->apPassword().c_str());
          _dnsApServer = new DNSServer();
          WiFi.setAutoReconnect(false);
          WiFi.softAP(apSsid.c_str(), this->apPassword().c_str());
          _dnsApServer->setErrorReplyCode(DNSReplyCode::NoError);
          _dnsApServer->start(53, "*", WiFi.softAPIP());          
          this->startWebServer();
        } else if ((_wifiConnectTrys < WIFI_RECONNECTION_TRYS) &&
                   ((_lastWifiConnect == 0) ||
                    (now - _lastWifiConnect > WIFI_RECONNECTION))) {
          _wifiConnectTrys++;
          _wlog->notice("Connecting to '%s': %d. try", getSsid(), _wifiConnectTrys);
#ifdef ESP8266
          // Workaround: if disconnect is not called, WIFI connection fails
          // after first startup
          WiFi.disconnect();
          WiFi.hostname(_hostname);
#elif ESP32          
          //Workaround: WiFi.setHostName now only works if: - You call it before calling WiFi.mode(WIFI_STA)
          //and ensure that the mode is not WIFI_STA already before calling WiFi.setHostName (by first calling WiFi.mode(WIFI_MODE_NULL)
          WiFi.mode(WIFI_MODE_NULL);
          WiFi.setHostname(_hostname);
          WiFi.mode(WIFI_STA);
#endif
          WiFi.begin(getSsid(), getPassword());

          while ((_waitForWifiConnection) && (WiFi.status() != WL_CONNECTED)) {
            delay(100);
            if (millis() - now >= 5000) {
              break;
            }
          }
          if (_wifiConnectTrys == 1) {
            _lastWifiConnectFirstTry = now;
          }
          _lastWifiConnect = now;
        }
      }

    } else {
      if (isSoftAP()) {
        _dnsApServer->processNextRequest();
      }
      // webServer->handleClient();
      result = ((!isSoftAP()) && (!isUpdateRunning()));
    }
    // MQTT connection
    if ((isWifiConnected()) && (isSupportingMqtt()) &&
        (!_mqttClient->connected()) &&
        ((_lastMqttConnect == 0) || (now - _lastMqttConnect > 300000)) &&
        (strcmp(mqttServer(), "") != 0) &&
        (strcmp(mqttPort(), "") != 0)) {
      _mqttReconnect();
      _lastMqttConnect = now;
    }
    if (!isUpdateRunning()) {
      if ((!isUpdateRunning()) && (this->isMqttConnected())) {
        _mqttClient->loop();
      }
      // Loop led
      if (_statusLed != nullptr) {
        _statusLed->loop(now);
      }
      bool allStatesComplete = true;
      bool stateUpd = false;
      // Loop Devices
      _devices->forEach([this, now](WDevice *device) {
        device->loop(now);
        if ((this->isMqttConnected()) && (this->isSupportingMqtt()) &&
            ((device->lastStateNotify() == 0) ||
             ((device->stateNotifyInterval() > 0) &&
              (now > device->lastStateNotify()) &&
              (now - device->lastStateNotify() >
               device->stateNotifyInterval()))) &&
            (device->isDeviceStateComplete())) {
          _wlog->notice(F("Notify interval is up -> Device state changed... %d"), device->lastStateNotify());
          _handleDeviceStateChange(device, (device->lastStateNotify() != 0));
        }
      });
// WebThingAdapter
#ifdef ESP8266
      if ((!isUpdateRunning()) && (MDNS.isRunning()) && (isWifiConnected())) {        
        MDNS.update();
      }
#endif
    }
    // Restart required?
    if (!_restartFlag.equals("")) {
      _updateRunning = false;
      delay(1000);
      if (_onConfigurationFinished) {
        _onConfigurationFinished();
      }
      stopWebServer();
      ESP.restart();
      delay(2000);
    } else if (_deepSleepFlag != nullptr) {
      if (_deepSleepFlag->off()) {
        // Deep Sleep
        _wlog->notice(F("Go to deep sleep. Bye..."));
        _updateRunning = false;
        stopWebServer();
        delay(500);
        if (_deepSleepFlag->deepSleepSeconds() > 0) {
          ESP.deepSleep(_deepSleepFlag->deepSleepSeconds() * 1000 * 1000);
        } else if (_deepSleepFlag->deepSleepMode() == DEEP_SLEEP_GPIO_HIGH) {
#ifdef ESP32
          esp_sleep_enable_ext0_wakeup((gpio_num_t)_deepSleepFlag->deepSleepGPIO(), HIGH);
          esp_deep_sleep_start();
#elif ESP8266
          _wlog->error(F("deepsleep with GPIO not supported bye ESP8266"));
#endif
        } else if (_deepSleepFlag->deepSleepMode() == DEEP_SLEEP_GPIO_LOW) {
#ifdef ESP32
          esp_sleep_enable_ext0_wakeup((gpio_num_t)_deepSleepFlag->deepSleepGPIO(), LOW);
          esp_deep_sleep_start();
#elif ESP8266
          _wlog->error(F("deepsleep with GPIO not supported bye ESP8266"));
#endif
        } else {
          _wlog->notice(F("Going to deep sleep failed. Seconds or GPIO missing..."));
        }
      }
    }
    return result;
  }

  ~WNetwork() { delete _wlog; }

  WSettings* settings() { return _settings; }

  void setDebuggingOutput(Print* output) {
    _debuggingOutput = output;
    _wlog->setOutput(output, (_debugging ? LOG_LEVEL_NOTICE : LOG_LEVEL_SILENT), true, true);
  }

  void setOnNotify(THandlerFunction onNotify) { _onNotify = onNotify; }

  void setOnConfigurationFinished(THandlerFunction onConfigurationFinished) {
    _onConfigurationFinished = onConfigurationFinished;
  }

  bool publishMqtt(const char* topic, WStringStream* response, bool retained = false) {
    _wlog->notice(F("MQTT... '%s'; %s"), topic, response->c_str());
    if (isMqttConnected()) {
      if (_mqttClient->publish(topic, response->c_str(), retained)) {
        _wlog->notice(F("MQTT sent. Topic: '%s'"), topic);
        return true;
      } else {
        _wlog->notice(F("Sending MQTT message failed, rc=%d"), _mqttClient->state());
        this->disconnectMqtt();
        return false;
      }
    } else {      
      if (strcmp(mqttServer(), "") != 0) {
        _wlog->notice(F("Can't send MQTT. Not connected to server: %s"), mqttServer());
      }
      return false;
    }
    _wlog->notice(F("publish MQTT mystery... "));
  }

  bool publishMqtt(const char *topic, const char *key, const char *value) {
    if ((this->isMqttConnected()) && (this->isSupportingMqtt())) {
      WStringStream *response = getResponseStream();
      WJson json(response);
      json.beginObject();
      json.propertyString(key, value);
      json.endObject();
      return publishMqtt(topic, response);
    } else {
      return false;
    }
  }

  // Creates a web server. If Wifi is not connected, then an own AP will be
  // created
  void startWebServer() {
    if (!isWebServerRunning()) {
      bool aDeviceNeedsWebThings = _aDeviceNeedsWebThings();
      _webServer = new AsyncWebServer(80);
      _webServer->onNotFound(std::bind(&WNetwork::_handleUnknown, this, std::placeholders::_1));
      if ((WiFi.status() != WL_CONNECTED) || (!aDeviceNeedsWebThings)) {
        _webServer->on(SLASH, HTTP_GET, std::bind(&WNetwork::_handleHttpRootRequest, this, std::placeholders::_1));
      }
      _webServer->on("/config", HTTP_GET, std::bind(&WNetwork::_handleHttpRootRequest, this, std::placeholders::_1));
      _pages->forEach([this](WPage *page) {
        String did(SLASH);
        did.concat(page->getId());
        _webServer->on(did.c_str(), HTTP_ANY, std::bind(&WNetwork::_handleHttpCustomPage, this, std::placeholders::_1, page));
        if (page->hasSubmittedPage()) {
          String dis("/submit");
          dis.concat(page->getId());
          _webServer->on(dis.c_str(), HTTP_ANY, std::bind(&WNetwork::_handleHttpSubmittedCustomPage, this, std::placeholders::_1, page));
        }
      });
      _webServer->on("/wifi", HTTP_GET, std::bind(&WNetwork::_handleHttpNetworkConfiguration, this, std::placeholders::_1));
      _webServer->on("/submitnetwork", HTTP_GET, std::bind(&WNetwork::_handleHttpSaveConfiguration, this, std::placeholders::_1));
      _webServer->on("/info", HTTP_GET, std::bind(&WNetwork::_handleHttpInfo, this, std::placeholders::_1));
      _webServer->on("/reset", HTTP_GET, std::bind(&WNetwork::_handleHttpReset, this, std::placeholders::_1));
      _webServer->on("/w4StEi18X6", HTTP_ANY, std::bind(&WNetwork::_handleHttpDoReset, this, std::placeholders::_1));
      _webServer->on("/w4StEi18X7", HTTP_ANY, std::bind(&WNetwork::_handleHttpResetNetwork, this, std::placeholders::_1));
      _webServer->on("/w4StEi18X8", HTTP_ANY, std::bind(&WNetwork::_handleHttpResetAll, this, std::placeholders::_1));
      // firmware update
      _webServer->on("/firmware", HTTP_GET, std::bind(&WNetwork::_handleHttpFirmwareUpdate, this, std::placeholders::_1));
      _webServer->on("/firmware", HTTP_POST, std::bind(&WNetwork::_handleHttpFirmwareUpdateFinished, this, std::placeholders::_1),
                    std::bind(&WNetwork::_handleHttpFirmwareUpdateProgress, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4,
                              std::placeholders::_5, std::placeholders::_6));
      // WebThings      
      if ((aDeviceNeedsWebThings) && (this->isWifiConnected())) {
        // Make the thing discoverable
        String mdnsName = String(_hostname);
        if (MDNS.begin(mdnsName.c_str())) {
          MDNS.addService("http", "tcp", 80);
          MDNS.addServiceTxt("http", "tcp", "url", "http://" + mdnsName + SLASH);
          MDNS.addServiceTxt("http", "tcp", "webthing", "true");
          _wlog->notice(F("MDNS responder for Webthings started at '%s'"), _hostname);
        }
        _webServer->on(SLASH, HTTP_GET, std::bind(&WNetwork::_sendDevicesStructure, this, std::placeholders::_1));
        _devices->forEach([this](WDevice *device) { _bindWebServerCalls(device); });
      }
      // Start http server
      _webServer->begin();
      _wlog->notice(F("webServer started."));
      _notify(true);
    }
  }

  void stopWebServer() {
    if ((isWebServerRunning()) && (!_supportsWebServer) && (!_updateRunning)) {
      _wlog->notice(F("Close web configuration."));
      delay(100);
      _webServer->end();
      _webServer = nullptr;
      _notify(true);
    }
  }

  void enableWebServer(bool startWebServer) {
    if (startWebServer) {
      this->startWebServer();
    } else {
      this->stopWebServer();
    }
  }

  bool isWebServerRunning() { return (_webServer != nullptr); }

  bool isUpdateRunning() { return _updateRunning; }

  bool isSoftAP() {
    return ((isWebServerRunning()) && (_dnsApServer != nullptr));
  }

  bool isWifiConnected() {
    return ((!isSoftAP()) && (!isUpdateRunning()) && (WiFi.status() == WL_CONNECTED));
  }

  bool isMqttConnected() {
    return ((this->isSupportingMqtt()) && (_mqttClient != nullptr) && (_mqttClient->connected()));
  }

  void disconnectMqtt() {
    if (_mqttClient != nullptr) {
      _mqttClient->disconnect();
    }
  }

  IPAddress getDeviceIp() {
    return (isSoftAP() ? WiFi.softAPIP() : WiFi.localIP());
  }

  bool supportsWebServer() { return _supportsWebServer; }

  void setSupportsWebServer(bool supportsWebServer) {
    _supportsWebServer = supportsWebServer;
  }

  bool isSupportingMqtt() { return _supportingMqtt->asBool(); }

  bool isInitialMqttSent() { return _initialMqttSent; }

  bool isLastWillEnabled() { return _lastWillEnabled; }

  void setLastWillEnabled(bool lastWillEnabled) {
    _lastWillEnabled = lastWillEnabled;
  }

  const char *getIdx() { return _idx->c_str(); }

  const char *getHostName() { return _hostname; }

  const char *getSsid() { return _ssid->c_str(); }

  const char *getPassword() { return _settings->getString("password"); }

  const char *mqttServer() { return _settings->getString("mqttServer"); }

  const char *mqttPort() { return _settings->getString("mqttPort"); }

  const char* mqttBaseTopic() { return _mqttBaseTopic->c_str(); }

  const char *mqttSetTopic() { return _mqttSetTopic->c_str(); }

  const char *mqttStateTopic() { return _mqttStateTopic->c_str(); }

  const char *mqttUser() { return _settings->getString("mqttUser"); }

  const char *mqttPassword() { return _settings->getString("mqttPassword"); }

  void addDevice(WDevice *device) {
    _devices->add(device);
    _bindWebServerCalls(device);
  }

  void addCustomPage(WPage *page) { _pages->add(page); }

  AsyncWebServer *getWebServer() { return _webServer; }

  WStringStream *getResponseStream() {
    if (_responseStream == nullptr) {
      _responseStream = new WStringStream(SIZE_JSON_PACKET);
    }
    _responseStream->flush();
    return _responseStream;
  }

  template <class T, typename... Args>
  void error(T msg, Args... args) {
    logLevel(LOG_LEVEL_ERROR, msg, args...);
  }

  template <class T, typename... Args>
  void debug(T msg, Args... args) {
    logLevel(LOG_LEVEL_DEBUG, msg, args...);
  }

  template <class T, typename... Args>
  void notice(T msg, Args... args) {
    logLevel(LOG_LEVEL_NOTICE, msg, args...);
  }

  template <class T, typename... Args>
  void logLevel(int level, T msg, Args... args) {
    _wlog->printLevel(level, msg, args...);
    if ((isMqttConnected()) && ((level == LOG_LEVEL_ERROR) || (level == LOG_LEVEL_NOTICE) || (_debugging))) {
      WStringStream *response = getResponseStream();
      WJson json(response);
      json.beginObject();
      json.memberName(_wlog->getLevelString(level));
      response->print(QUOTE);
      _wlog->setOutput(response, level, false, false);
      _wlog->printLevel(level, msg, args...);
      this->setDebuggingOutput(_debuggingOutput);
      response->print(QUOTE);
      json.endObject();
      publishMqtt(_mqttBaseTopic->c_str(), response);
    }
  }

  bool isDebugging() { return _debugging; }

  void restart() { _restart(nullptr, "Restart..."); }

  String apSsid() { return _getClientName(false); }
  String apPassword() { return CONFIG_PASSWORD; }

 private:
  WLog* _wlog;
  WList<WDevice> *_devices;
  WList<WPage> *_pages;
  THandlerFunction _onNotify;
  THandlerFunction _onConfigurationFinished;
  bool _debugging, _updateRunning;
  String _restartFlag;
  DNSServer* _dnsApServer;
  AsyncWebServer* _webServer;
  int _networkState;
  String _applicationName;
  String _firmwareVersion;
  const char* _firmwareUpdateError;
  WProperty* _supportingMqtt;
  bool _supportsWebServer;
  WProperty* _ssid;
  WProperty* _idx;
  char* _hostname;
  WProperty* _mqttBaseTopic;
  WProperty* _mqttSetTopic;
  WProperty* _mqttStateTopic;
  WiFiClient* _wifiClient;
  PubSubClient* _mqttClient;
  unsigned long _lastMqttConnect, _lastWifiConnect, _lastWifiConnectFirstTry;
  byte _wifiConnectTrys;
  WStringStream* _responseStream = nullptr;
  WLed* _statusLed;
  bool _statusLedOnIfConnected;
  WSettings* _settings;
  WDevice* _deepSleepFlag;
  unsigned long _startupTime;
  char _body_data[ESP_MAX_PUT_BODY_SIZE];
  bool _b_has_body_data = false;
  Print* _debuggingOutput;
  bool _initialMqttSent;
  bool _lastWillEnabled;
  bool _waitForWifiConnection;

  bool _aDeviceNeedsWebThings() {
    bool result = false;
    WIterator<WDevice>* it_d = _devices->iterator();
    while ((!result) && (it_d->hasNext())) {      
      WDevice* d = it_d->next();
      result = d->needsWebThings();
    } 
    return result;
  }

  void _handleDeviceStateChange(WDevice *device, bool complete) {
    _wlog->notice(F("Device state changed -> send device state for device '%s'"), device->id());
    String topic = String(mqttBaseTopic()) + SLASH + String(device->id()) + SLASH + String(mqttStateTopic());
    _mqttSendDeviceState(topic, device, complete);
  }

  void _mqttSendDeviceState(String topic, WDevice *device, bool complete) {
    if ((this->isMqttConnected()) && (isSupportingMqtt()) && (device->isDeviceStateComplete())) {
      _wlog->notice(F("Send actual device state via MQTT"));

      if (device->sendCompleteDeviceState()) {
        // Send all properties of device in one json structure
        WStringStream *response = getResponseStream();
        WJson json(response);
        json.beginObject();
        if (device->isMainDevice()) {
          json.propertyString("idx", getIdx());
          json.propertyString("ip", getDeviceIp().toString().c_str());
          if (this->isLastWillEnabled()) {
            json.propertyBoolean("alive", true);
          }
          json.propertyString("firmware", _firmwareVersion.c_str());
        }
        device->toJsonValues(&json, MQTT);
        json.endObject();

        _mqttClient->publish(topic.c_str(), (const uint8_t *)response->c_str(),
                            response->length(), true);
        _initialMqttSent = true;
      } else {
        // Send every changed property only
        device->properties()->forEach(
            [this, complete, topic](WProperty *property) {
              if ((complete) || (property->changed())) {
                if (property->isVisible(MQTT)) {
                  WStringStream *response = getResponseStream();
                  WJson json(response);
                  property->toJsonValue(&json, true);
                  _mqttClient->publish(
                      String(topic + SLASH + String(property->id())).c_str(),
                      response->c_str(), true);
                }
                property->changed(false);
              }
            });
        if (complete) {
          _initialMqttSent = true;
        }
      }      
      device->lastStateNotify(millis());
      if ((device->deepSleepMode() != DEEP_SLEEP_NONE) &&
          ((!_supportsWebServer) ||
           (device->areAllPropertiesRequested()))) {
        _deepSleepFlag = device;
      }      
    }
  }

  void _mqttCallback(char *ptopic, uint8_t *payload, unsigned int length) {
    payload[length] = '\0';
    _wlog->notice(F("Received MQTT callback. topic: '%s'; payload: '%s'; length: %d"), ptopic, (char *)payload, length);
    String baseT = String(mqttBaseTopic());
    String stateT = String(mqttStateTopic());
    String setT = String(mqttSetTopic());

    String cTopic = String(ptopic);
    if (cTopic.startsWith(baseT)) {
      String topic = cTopic.substring(baseT.length() + 1);
      _wlog->notice(F("Topic short '%s'"), topic.c_str());
      // Next is device id
      int i = topic.indexOf(SLASH);
      if (i > -1) {
        String deviceId = topic.substring(0, i);
        _wlog->notice(F("look for device id '%s'"), deviceId.c_str());
        WDevice *device = _getDeviceById(deviceId.c_str());
        if (device != nullptr) {
          topic = topic.substring(i + 1);
          if (topic.startsWith(stateT)) {
            if (length == 0) {
              // State request
              topic = topic.substring(stateT.length() + 1);
              if (topic.equals("")) {
                // send all propertiesBase
                _wlog->notice(F("Send complete device state..."));
                // Empty payload for topic 'properties' -> send device state
                _mqttSendDeviceState(String(ptopic), device, true);
              } else {
                WProperty *property = device->getPropertyById(topic.c_str());
                if (property != nullptr) {
                  if (property->isVisible(MQTT)) {
                    _wlog->notice(F("Send state of property '%s'"),
                                 property->id());
                    WStringStream *response = getResponseStream();
                    WJson json(response);
                    property->toJsonValue(&json, true);
                    _mqttClient->publish(String(baseT + SLASH + deviceId + SLASH + stateT + SLASH + String(property->id())).c_str(), response->c_str(), true);
                  }
                } else {
                  device->handleUnknownMqttCallback(true, ptopic, topic, (char*)payload, length);
                }
              }
            }
          } else if (topic.startsWith(setT)) {
            if (length > 0) {
              // Set request
              topic = topic.substring(setT.length() + 1);
              if (topic.equals("")) {
                // set all properties
                _wlog->notice(F("Try to set several properties for device %s"), device->id());
                WJsonParser *parser = new WJsonParser();
                if (parser->parse((char *)payload, device) == nullptr) {
                  _wlog->notice(F("No properties updated for device %s"), device->id());
                } else {
                  _wlog->notice(F("One or more properties updated for device %s"), device->id());
                }
                delete parser;
              } else {
                // Try to find property and set single value
                WProperty *property = device->getPropertyById(topic.c_str());
                if (property != nullptr) {
                  if (property->isVisible(MQTT)) {
                    // Set Property
                    _wlog->notice(F("Try to set property %s for device %s"), property->id(), device->id());
                    if (!property->parse((char *)payload)) {
                      _wlog->notice(F("Property not updated."));
                    } else {
                      _wlog->notice(F("Property updated."));
                    }
                  }
                } else {
                  device->handleUnknownMqttCallback(false, ptopic, topic, (char *)payload, length);
                }
              }
            }
          } else {
            // unknown, ask the device
            // device->handleUnknownMqttCallback(ptopic, topic, payload,
            // length);
          }
        } else if (deviceId.equals("webServer")) {
          enableWebServer(String((char *)payload).equals(HTTP_TRUE));
        }
      }
    }
  }

  bool _mqttReconnect() {
    if (this->isSupportingMqtt()) {
      _wlog->notice(F("Connect to MQTT server: %s; user: '%s'; password: '%s'; clientName: '%s'"),
                   mqttServer(), mqttUser(), mqttPassword(), _getClientName(true).c_str());
      // Attempt to connect
      _mqttClient->setServer(mqttServer(), String(mqttPort()).toInt());
      bool connected = false;
      // Create last will message
      if (this->isLastWillEnabled()) {
        String lastWillTopic = String(mqttBaseTopic());
        lastWillTopic.concat(SLASH);
        WStringStream *lastWillMessage = getResponseStream();
        WJson json(lastWillMessage);
        WDevice *device = _devices->getIf([this](WDevice *d) { return (d->isMainDevice()); });
        if (device != nullptr) {
          lastWillTopic.concat(device->id());
          lastWillTopic.concat(SLASH);
          lastWillTopic.concat(mqttStateTopic());
          json.beginObject();
          json.propertyString("idx", getIdx());
          json.propertyString("ip", getDeviceIp().toString().c_str());
          json.propertyBoolean("alive", false);
          json.endObject();
        }
        connected = (_mqttClient->connect(
            _getClientName(true).c_str(),
            mqttUser(),
            mqttPassword(), lastWillTopic.c_str(), 0, true,
            lastWillMessage->c_str()));
      } else {
        connected = (_mqttClient->connect(
            _getClientName(true).c_str(),
            mqttUser(),
            mqttPassword()));
      }

      if (connected) { 
        _wlog->notice(F("Connected to MQTT server."));        
        //  Send device structure and status
        _mqttClient->subscribe("devices/#");
        _devices->forEach([this](WDevice *device) {
          String topic("devices/");
          topic.concat(device->id());
          WStringStream *response = getResponseStream();
          WJson json(response);
          json.beginObject();
          json.propertyString("url", "http://", getDeviceIp().toString().c_str(), "/things/", device->id());
          json.propertyString("stateTopic", mqttBaseTopic(), SLASH, device->id(), SLASH, mqttStateTopic());
          json.propertyString("setTopic", mqttBaseTopic(), SLASH, device->id(), SLASH, mqttSetTopic());
          json.endObject();
          _mqttClient->publish(topic.c_str(), response->c_str(), false);
        });
        _mqttClient->unsubscribe("devices/#");
        // Subscribe to device specific topic
        _mqttClient->subscribe(String(String(mqttBaseTopic()) + "/#").c_str());
        _notify(false);
        return true;
      } else {
        _wlog->notice(F("Connection to MQTT server failed, rc=%d"), _mqttClient->state());
        this->startWebServer();
        _notify(false);
        return false;
      }
      _initialMqttSent = false;
    }
    return false;
  }

  void _updateLedState() {
    if (_statusLed != nullptr) {
      if (isWifiConnected()) {
        // off
        _statusLed->setOn(_statusLedOnIfConnected, 0);
      } else if (isSoftAP()) {
        _statusLed->setOn(!_statusLedOnIfConnected, 0);
      } else {
        _statusLed->setOn(true, 500);
      }
    }
  }

  void _notify(bool sendState) {
    _updateLedState();
    if (sendState) {
      _devices->forEach([this](WDevice *device) { _handleDeviceStateChange(device, false); });
    }
    if (_onNotify) {
      _onNotify();
    }
  }

  void _handleHttpRootRequest(AsyncWebServerRequest *request) {
    if (isWebServerRunning()) {
      if (_restartFlag.equals("")) {
        AsyncResponseStream *page = request->beginResponseStream(TEXT_HTML);
        page->printf(HTTP_HEAD_BEGIN, _applicationName.c_str());
        page->print(FPSTR(HTTP_STYLE));
        page->print(FPSTR(HTTP_HEAD_END));
        _printHttpCaption(page);
        page->printf(HTTP_BUTTON, "wifi", "get", "Configure network");
        _pages->forEach([this, page](WPage *customPage) {
          if (customPage->isShowInMainMenu()) {
            page->printf(HTTP_BUTTON, customPage->getId(), "get",
                         customPage->getTitle());
          }
        });
        page->printf(HTTP_BUTTON, "firmware", "get", "Update firmware");
        page->printf(HTTP_BUTTON, "info", "get", "Info");
        page->printf(HTTP_BUTTON, "reset", "get", "Restart options");
        page->print(FPSTR(HTTP_BODY_END));
        request->send(page);
      } else {
        AsyncResponseStream *page = request->beginResponseStream(TEXT_HTML);
        page->printf(HTTP_HEAD_BEGIN, "Info");
        page->print(FPSTR(HTTP_STYLE));
        page->print("<meta http-equiv=\"refresh\" content=\"10\">");
        page->print(FPSTR(HTTP_HEAD_END));
        page->print(_restartFlag);
        page->print("<br><br>");
        page->print("Module will reset in a few seconds...");
        page->print(FPSTR(HTTP_BODY_END));
        request->send(page);
      }
    }
  }

  void _handleHttpCustomPage(AsyncWebServerRequest *request, WPage *customPage) {
    if (isWebServerRunning()) {
      AsyncResponseStream *response = request->beginResponseStream(TEXT_HTML, 6100U);
      response->printf(HTTP_HEAD_BEGIN, customPage->getTitle());
      response->print(FPSTR(HTTP_STYLE));
      response->print(FPSTR(HTTP_HEAD_END));
      _printHttpCaption(response);
      customPage->printPage(request, response);
      response->print(FPSTR(HTTP_BODY_END));
      request->send(response);
    }
  }

  void _handleHttpNetworkConfiguration(AsyncWebServerRequest *request) {
    if (isWebServerRunning()) {
      _wlog->notice(F("Network config page"));
      AsyncResponseStream *page = request->beginResponseStream(TEXT_HTML);
      page->printf(HTTP_HEAD_BEGIN, "Network Configuration");
      page->print(FPSTR(HTTP_STYLE));
      page->print(FPSTR(HTTP_HEAD_END));
      _printHttpCaption(page);
      page->printf(HTTP_CONFIG_PAGE_BEGIN, "network");
      page->printf(HTTP_TOGGLE_GROUP_STYLE, "ga", HTTP_NONE, "gb", HTTP_NONE);
      page->printf(HTTP_TEXT_FIELD, "Identifier (idx):", "i", "16", getIdx());
      page->printf(HTTP_TEXT_FIELD, "Wifi ssid (only 2.4G):", "s", "32", getSsid());
      page->printf(HTTP_PASSWORD_FIELD, "Wifi password:", "p", "32", getPassword());
      // mqtt
      page->printf(HTTP_TEXT_FIELD, "MQTT Server:", "ms", "32", mqttServer());
      page->printf(HTTP_TEXT_FIELD, "MQTT Port:", "mo", "4", mqttPort());
      page->printf(HTTP_TEXT_FIELD, "MQTT User:", "mu", "16", mqttUser());
      page->printf(HTTP_PASSWORD_FIELD, "MQTT Password:", "mp", "32", mqttPassword());
      // advanced mqtt options
      page->printf(HTTP_CHECKBOX_OPTION, "sa", "sa", "", "tg()", "Advanced MQTT options");
      page->printf(HTTP_DIV_ID_BEGIN, "ga");
      page->printf(HTTP_TEXT_FIELD, "MQTT Topic:", "mt", "32", mqttBaseTopic());
      page->printf(HTTP_TEXT_FIELD, "Topic for state requests:", "mtg", "16", mqttStateTopic());
      page->printf(HTTP_TEXT_FIELD, "Topic for setting values:", "mts", "16", mqttSetTopic());
      page->print(FPSTR(HTTP_DIV_END));
      page->printf(HTTP_TOGGLE_FUNCTION_SCRIPT, "tg()", "sa", "ga", "gb");
      page->print(FPSTR(HTTP_CONFIG_SAVE_BUTTON));
      page->print(FPSTR(HTTP_BODY_END));
      request->send(page);
    }
  }

  void _handleHttpSaveConfiguration(AsyncWebServerRequest *request) {
    String mbt = request->arg("mt");
    bool equalsOldIdx = _idx->equalsString(mbt.c_str());
    String itx = request->arg("i");

    _idx->asString(itx.c_str());
    _ssid->asString(request->arg("s").c_str());
    _settings->setString("password", request->arg("p").c_str());
    _supportingMqtt->asBool(true);  
    _settings->setString("mqttServer", request->arg("ms").c_str());
    String mqtt_port = request->arg("mo");
    _settings->setString("mqttPort",
                         (mqtt_port != "" ? mqtt_port.c_str() : "1883"));
    _settings->setString("mqttUser", request->arg("mu").c_str());
    _settings->setString("mqttPassword", request->arg("mp").c_str());
    // advanced mqtt options
    _mqttBaseTopic->asString(equalsOldIdx ? itx.c_str() : mbt.c_str());
    String subTopic = request->arg("mtg");
    if (subTopic.startsWith(SLASH)) subTopic.substring(1);
    if (subTopic.endsWith(SLASH)) subTopic.substring(0, subTopic.length() - 1);
    if (subTopic.equals("")) subTopic = DEFAULT_TOPIC_STATE;
    _mqttStateTopic->asString(subTopic.c_str());
    subTopic = request->arg("mts");
    if (subTopic.startsWith(SLASH)) subTopic.substring(1);
    if (subTopic.endsWith(SLASH)) subTopic.substring(0, subTopic.length() - 1);
    if (subTopic.equals("")) subTopic = DEFAULT_TOPIC_SET;
    _mqttSetTopic->asString(subTopic.c_str());
    _settings->save();
    _restart(request, "Settings saved. Subscribe to topic 'devices/#' at your broker to get device information.");
  }

  void _handleHttpSubmittedCustomPage(AsyncWebServerRequest *request,
                                     WPage *customPage) {
    if (customPage->hasSubmittedPage()) {
      _wlog->notice(F("Save custom page: %s"), customPage->getId());
      WStringStream *page = new WStringStream(1024);
      customPage->submittedPage(request, page);
      if (customPage->getTargetAfterSubmitting() != nullptr) {
        _handleHttpCustomPage(request, customPage->getTargetAfterSubmitting());
      } else {
        _settings->save();
        _restart(request, (strlen(page->c_str()) == 0 ? "Settings saved." : page->c_str()));
      }
      delete page;
    }
  }

  void _handleHttpInfo(AsyncWebServerRequest *request) {
    if (isWebServerRunning()) {
      AsyncResponseStream *page = request->beginResponseStream(TEXT_HTML);
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
    }
  }

  void _handleHttpReset(AsyncWebServerRequest *request) {
    if (isWebServerRunning()) {
      AsyncResponseStream *page = request->beginResponseStream(TEXT_HTML);
      page->printf(HTTP_HEAD_BEGIN, "Restart options");
      page->print(FPSTR(HTTP_STYLE));
      page->print(FPSTR(HTTP_HEAD_END));
      _printHttpCaption(page);
      page->printf(HTTP_BUTTON, "w4StEi18X6", "post", "Reboot");
      page->print(FPSTR(HTTP_DIV_BEGIN));
      page->print(FPSTR(HTTP_DIV_END));
      page->printf(HTTP_BUTTON_ALERT, "w4StEi18X7", "post", "Restart in AccessPoint mode");
      page->printf(HTTP_BUTTON_ALERT, "w4StEi18X8", "post", "Reset all settings");
      page->print(FPSTR(HTTP_DIV_BEGIN));
      page->print(FPSTR(HTTP_DIV_END));
      page->printf(HTTP_BUTTON, "config", "get", "Cancel");
      page->print(FPSTR(HTTP_BODY_END));
      request->send(page);
    }
  }

  void _handleHttpDoReset(AsyncWebServerRequest *request) {
    if (isWebServerRunning()) {
      _restart(request, "Resetting was caused manually by web interface. ");
    }
  }

  void _handleHttpResetNetwork(AsyncWebServerRequest *request) {
    if (isWebServerRunning()) {
      _settings->forceAPNextStart();
      WStringStream *response = getResponseStream();
      response->print(F("Restart device in AccessPoint mode.<br>"));
      response->print(F("Connect to WLAN AP '"));
      response->print(_getClientName(false).c_str());
      response->print(F("' for configuration."));
      _restart(request, response->c_str());
    }
  }

  void _handleHttpResetAll(AsyncWebServerRequest *request) {
    if (isWebServerRunning()) {
      _settings->resetAll();
      WStringStream *response = getResponseStream();
      response->print(F("All settings are resetted, device restarts.<br>"));
      response->print(F("Connect to WLAN AP '"));
      response->print(_getClientName(false).c_str());
      response->print(F("' for configuration."));
      _restart(request, response->c_str());
    }
  }

  void _printHttpCaption(Print *page) {
    page->print(F("<h2>"));
    page->print(_applicationName);
    page->print(F("</h2><h3>Idx: "));
    page->print(getIdx());
    page->print(F("</h3><h3>Rev: "));
    page->print(_firmwareVersion);
    page->print(_debugging ? " (debug)" : "");
    page->print(F("</h3>"));
  }

  String _getClientName(bool lowerCase) {
    String result = (_applicationName.equals("") ? "ESP" : String(_applicationName));
    result.replace(" ", "-");
    if (lowerCase) {
      result.replace("-", "");
      result.toLowerCase();
    }
    String chipId = String(getChipId());
    int resLength = result.length() + chipId.length() + 1 - 32;
    if (resLength > 0) {
      result.substring(0, 32 - resLength);
    }
    return result + "_" + chipId;
  }

  uint32_t getChipId() {
#ifdef ESP8266
    return ESP.getChipId();
#elif ESP32
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    return (macAddressTrunc >> 40);
#endif
  }

  void _handleHttpFirmwareUpdate(AsyncWebServerRequest *request) {
    if (isWebServerRunning()) {
      AsyncResponseStream *page = request->beginResponseStream(TEXT_HTML);
      page->printf(HTTP_HEAD_BEGIN, "Firmware update");
      page->print(FPSTR(HTTP_STYLE));
      page->print(FPSTR(HTTP_HEAD_END));
      _printHttpCaption(page);
      page->print(FPSTR(HTTP_FORM_FIRMWARE));
      page->printf(HTTP_BUTTON, "config", "get", "Cancel");
      page->print(FPSTR(HTTP_BODY_END));
      request->send(page);
    }
  }

  void _handleHttpFirmwareUpdateFinished(AsyncWebServerRequest *request) {
    _settings->save();
    if (Update.hasError()) {
      _restart(request, _firmwareUpdateError);
    } else {
      _restart(request, "Update successful.");
    }
  }

  void _handleHttpFirmwareUpdateProgress(AsyncWebServerRequest *request,
                                        String filename, size_t index,
                                        uint8_t *data, size_t len, bool final) {
    // Start firmwareUpdate
    _updateRunning = true;
    // Close existing MQTT connections
    this->disconnectMqtt();
    // Start update
    if (!index) {
      _wlog->notice(F("Update starting: %s"), filename.c_str());
      size_t content_len = request->contentLength();
      int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
#ifdef ESP8266
      Update.runAsync(true);
      if (!Update.begin(content_len, cmd)) {
#elif ESP32
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
#endif
        _setFirmwareUpdateError("Can't start update. ");
      }
    }
    // Upload running
    if (len) {
      if (Update.write(data, len) != len) {
        _setFirmwareUpdateError("Can't upload file. ");
      }
    }
    // Upload finished
    if (final) {
      if (Update.end(true)) {
        _wlog->notice(F("Update completed. "));
      } else {
        _setFirmwareUpdateError("Can't finish update. ");
      }
    }
  }

  const char* _getFirmwareUpdateErrorMessage() {
    switch (Update.getError()) {
      case UPDATE_ERROR_OK:
        return "No Error";
      case UPDATE_ERROR_WRITE:
        return "Flash Write Failed";
      case UPDATE_ERROR_ERASE:
        return "Flash Erase Failed";
      case UPDATE_ERROR_READ:
        return "Flash Read Failed";
      case UPDATE_ERROR_SPACE:
        return "Not Enough Space";
      case UPDATE_ERROR_SIZE:
        return "Bad Size Given";
      case UPDATE_ERROR_STREAM:
        return "Stream Read Timeout";
      case UPDATE_ERROR_MD5:
        return "MD5 Failed: ";
#ifdef ESP8266
      case UPDATE_ERROR_SIGN:
        return "Signature verification failed";
      case UPDATE_ERROR_FLASH_CONFIG:
        return "Flash config wrong.";
      case UPDATE_ERROR_NEW_FLASH_CONFIG:
        return "New Flash config wrong.";
      case UPDATE_ERROR_MAGIC_BYTE:
        return "Magic byte is wrong, not 0xE9";
      case UPDATE_ERROR_BOOTSTRAP:
        return "Invalid bootstrapping state, reset ESP8266 before updating";
#endif
      default:
        return "UNKNOWN";
    }
  }

  void _setFirmwareUpdateError(String msg) {
    _firmwareUpdateError = _getFirmwareUpdateErrorMessage();
    String s = msg + _firmwareUpdateError;
    _wlog->notice(s.c_str());
  }

  void _restart(AsyncWebServerRequest *request, const char *reasonMessage) {
    _restartFlag = reasonMessage;
    if (request != nullptr) {
      request->client()->setNoDelay(true);
      AsyncResponseStream *page = request->beginResponseStream(TEXT_HTML);
      page->printf(HTTP_HEAD_BEGIN, reasonMessage);
      page->print(FPSTR(HTTP_STYLE));
      page->print(FPSTR(HTTP_HEAD_END));
      _printHttpCaption(page);
      page->printf(HTTP_SAVED, reasonMessage);
      page->print(FPSTR(HTTP_BODY_END));
      request->send(page);
    }
  }

  void _loadNetworkSettings() {
    _idx = _settings->setNetworkString("idx", _getClientName(true).c_str());
    _hostname = new char[strlen(_idx->c_str()) + 1];
    strcpy(_hostname, _idx->c_str());
    for (int i = 0; i < strlen(_hostname); i++) {
      if ((_hostname[i] == '.') || (_hostname[i] == ' ')) {
        _hostname[i] = '-';
      }
    }
    _ssid = _settings->setNetworkString("ssid", "");
    _settings->setNetworkString("password", "");
    _supportsWebServer = true;
    _supportingMqtt = _settings->setNetworkBoolean("supportingMqtt", true);
    _settings->setNetworkString("mqttServer", "");
    _settings->setNetworkString("mqttPort", "1883");
    _settings->setNetworkString("mqttUser", "");
    _settings->setNetworkString("mqttPassword", "");
    _mqttBaseTopic = _settings->setNetworkString("mqttTopic", getIdx());
    _mqttStateTopic = _settings->setNetworkString("mqttStateTopic", DEFAULT_TOPIC_STATE);
    _mqttSetTopic = _settings->setNetworkString("mqttSetTopic", DEFAULT_TOPIC_SET);
    if (_settings->existsNetworkSettings()) {
      if (strcmp(mqttBaseTopic(), "") == 0) {
        _mqttBaseTopic->asString(_getClientName(true).c_str());
      }
      if ((isSupportingMqtt()) && (_mqttClient != nullptr)) {
        this->disconnectMqtt();
      }
      _wlog->debug(F("SSID: '%s'; MQTT enabled: %T; MQTT server: '%s'; MQTT port: %s; WebServer started: %T"),
                  getSsid(), isSupportingMqtt(), mqttServer(), mqttPort(), isWebServerRunning());
    } else {
      _wlog->notice(F("Network settings are missing."));
    }
  }

  void _handleUnknown(AsyncWebServerRequest *request) {
    if (!isUpdateRunning()) {
      request->send(404);
    }
  }

  void _sendDevicesStructure(AsyncWebServerRequest *request) {
    if (!isUpdateRunning()) {
      _wlog->notice(F("Send description for all devices... "));
      AsyncResponseStream *response = request->beginResponseStream(APPLICATION_JSON);
      WJson *json = new WJson(response);
      json->beginArray();
      _devices->forEach([this, json](WDevice *device) {
        if (device->isVisible(WEBTHING)) {
          _wlog->notice(F("Send description for device %s "), device->id());
          device->toJsonStructure(json, "", WEBTHING);
        }
      });
      json->endArray();
      request->send(response);
    }
  }

  void _sendDeviceStructure(AsyncWebServerRequest *request, WDevice *&device) {
    if (!isUpdateRunning()) {
      _wlog->notice(F("Send description for device: %s"), device->id());
      AsyncResponseStream *response = request->beginResponseStream(APPLICATION_JSON);
      WJson json(response);
      device->toJsonStructure(&json, "", WEBTHING);
      request->send(response);
    }
  }

  void _sendDeviceValues(AsyncWebServerRequest *request, WDevice *&device) {
    if (!isUpdateRunning()) {
      _wlog->notice(F("Send all properties for device: "), device->id());
      AsyncResponseStream *response = request->beginResponseStream(APPLICATION_JSON);
      WJson json(response);
      json.beginObject();
      if (device->isMainDevice()) {
        json.propertyString("idx", getIdx());
        json.propertyString("ip", getDeviceIp().toString().c_str());
        json.propertyString("firmware", _firmwareVersion.c_str());
      }
      device->toJsonValues(&json, WEBTHING);
      json.endObject();
      request->send(response);
    }
  }

  void _getPropertyValue(AsyncWebServerRequest *request, WProperty *property) {
    if (!isUpdateRunning()) {
      AsyncResponseStream *response =
          request->beginResponseStream(APPLICATION_JSON);
      WJson json(response);
      json.beginObject();
      property->toJsonValue(&json);
      json.endObject();
      property->requested(true);
      request->send(response);
    }
  }

  void _handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!isUpdateRunning()) {
      if ((total >= ESP_MAX_PUT_BODY_SIZE) || (index + len >= ESP_MAX_PUT_BODY_SIZE)) {
        return;  // cannot store this size..
      }
      // copy to internal buffer
      memcpy(&_body_data[index], data, len);
      _body_data[len] = '\0';
      _b_has_body_data = true;
    }
  }

  void _setPropertyValue(AsyncWebServerRequest *request, WDevice *device) {
    if (!isUpdateRunning()) {
      _wlog->notice(F("Set property value:"));
      if (!_b_has_body_data) {
        request->send(422);
        return;
      }
      WJsonParser parser;
      WProperty *property = parser.parse(_body_data, device);
      if (property != nullptr) {
        // response new value
        _wlog->notice(F("Set property value: %s (web request) %s"), property->id(), _body_data);
        AsyncResponseStream *response = request->beginResponseStream(APPLICATION_JSON);
        WJson json(response);
        json.beginObject();
        property->toJsonValue(&json);
        json.endObject();
        request->send(response);
      } else {
        // unable to parse json
        _wlog->notice(F("unable to parse json: %s"), _body_data);
        _b_has_body_data = false;
        memset(_body_data, 0, sizeof(_body_data));
        request->send(500);
      }
    }
  }

  void _sendErrorMsg(AsyncWebServerRequest *request, int status, const char *msg) {
    if (!isUpdateRunning()) {
      AsyncResponseStream *response =
          request->beginResponseStream(APPLICATION_JSON);
      WJson json(response);
      json.beginObject();
      json.propertyString("error", msg);
      json.propertyInteger("status", status);
      json.endObject();
      request->send(response);
    }
  }

  void _bindWebServerCalls(WDevice *device) {
    if (this->isWebServerRunning()) {
      _wlog->notice(F("Bind webServer calls for device %s"), device->id());
      String deviceBase("/things/");
      deviceBase.concat(device->id());
      device->properties()->forEach([this, device,
                                     deviceBase](WProperty *property) {
        if (property->isVisible(WEBTHING)) {
          String propertyBase = deviceBase + "/properties/" + property->id();
          _webServer->on(propertyBase.c_str(), HTTP_GET, std::bind(&WNetwork::_getPropertyValue, this, std::placeholders::_1, property));
          _webServer->on(propertyBase.c_str(), HTTP_PUT, std::bind(&WNetwork::_setPropertyValue, this, std::placeholders::_1, device),
              NULL, std::bind(&WNetwork::_handleBody, this, std::placeholders::_1,
                        std::placeholders::_2, std::placeholders::_3,
                        std::placeholders::_4, std::placeholders::_5));
        }
      });
      String propertiesBase = deviceBase + "/properties";
      _webServer->on(propertiesBase.c_str(), HTTP_GET, std::bind(&WNetwork::_sendDeviceValues, this, std::placeholders::_1, device));
      _webServer->on(deviceBase.c_str(), HTTP_GET, std::bind(&WNetwork::_sendDeviceStructure, this, std::placeholders::_1, device));
      device->bindWebServerCalls(_webServer);
    }
  }

  WDevice* _getDeviceById(const char *deviceId) {
    return _devices->getIf([deviceId](WDevice *d) { return (strcmp(d->id(), deviceId) == 0); });
  }
};

#endif