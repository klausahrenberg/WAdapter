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
#include <PubSubClient.h>
#include <StreamString.h>

#include "WDevice.h"
#include "WJsonParser.h"
#include "WList.h"
#include "WLog.h"
#include "WSettings.h"
#include "WStringStream.h"
#include "WiFiClient.h"
#include "html/WNetworkPages.h"
#include "html/WPage.h"
#include "hw/WLed.h"

#define SIZE_JSON_PACKET 1280
#define NO_LED -1
#define ESP_MAX_PUT_BODY_SIZE 512
#define WIFI_RECONNECTION 50000
#define WIFI_RECONNECTION_TRYS 3
const char *CONFIG_PASSWORD = "12345678";
const char *APPLICATION_JSON = "application/json";
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

  WNetwork(int statusLedPin, Print *debuggingOutput = &Serial) {
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

    _webServer = nullptr;
    _dnsApServer = nullptr;
    _wifiClient = new WiFiClient();
    _devices = new WList<WDevice>();
    _pages = new WList<WPageItem>();

    this->setDebuggingOutput(debuggingOutput);
    _updateRunning = false;
    _deepSleepFlag = nullptr;
    _waitForWifiConnection = false;
    _startupTime = millis();
    _mqttClient = nullptr;
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
    LOG->debug(F("firmware: %s"), VERSION);
    this->addCustomPage(WC_CONFIG, [this]() { return new WRootPage(_pages); }, false);
    this->addCustomPage(WC_WIFI, [this]() { return new WNetworkPage(); });
    this->addCustomPage(WC_FIRMWARE, [this]() { return new WFirmwarePage(); });
    this->addCustomPage(WC_INFO, [this]() { return new WInfoPage((millis() - _startupTime) / 1000 / 60); });
    this->addCustomPage(WC_RESET, [this]() { return new WResetPage(this); });
  }

  void setStatusLedPin(int statusLedPin, bool statusLedOnIfConnected) {
    this->setStatusLed(
        (statusLedPin != NO_LED ? new WLed(statusLedPin) : nullptr),
        statusLedOnIfConnected);
  }

  void setStatusLed(WLed *statusLed, bool statusLedOnIfConnected) {
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
    LOG->notice(F("Station connected, IP: %s Host Name is '%s'"), this->getDeviceIp().toString().c_str(), _hostname);
    // Connect, if webThing supported and Wifi is connected as client
    if ((_supportsWebServer) && (isWifiConnected())) {
      this->startWebServer();
    }
    _wifiConnectTrys = 0;
    _notify(false);
  }

  void onDisconnected() {
    if (!isSoftAP()) {
      LOG->notice("Station disconnected");
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
    SETTINGS->endReadingFirstTime();
    bool result = true;
    if (!isWebServerRunning()) {
      if (WiFi.status() != WL_CONNECTED) {
        if ((!SETTINGS->existsNetworkSettings()) ||
            (SETTINGS->forceNetworkAccessPoint()) || (strcmp(getSsid(), "") == 0) ||
            (_wifiConnectTrys == WIFI_RECONNECTION_TRYS)) {
          // Create own AP
          String apSsid = this->apSsid();
          LOG->notice(F("Start AccessPoint for configuration. SSID '%s'; password '%s'"), apSsid.c_str(), this->apPassword().c_str());
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
          LOG->notice("Connecting to '%s': %d. try", getSsid(), _wifiConnectTrys);
#ifdef ESP8266
          // Workaround: if disconnect is not called, WIFI connection fails
          // after first startup
          WiFi.disconnect();
          WiFi.hostname(_hostname);
#elif ESP32
          // Workaround: WiFi.setHostName now only works if: - You call it before calling WiFi.mode(WIFI_STA)
          // and ensure that the mode is not WIFI_STA already before calling WiFi.setHostName (by first calling WiFi.mode(WIFI_MODE_NULL)
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
      _devices->forEach([this, now](WDevice *device, const char *id) {
        device->loop(now);
        if ((this->isMqttConnected()) && (this->isSupportingMqtt()) &&
            ((device->lastStateNotify() == 0) ||
             ((device->stateNotifyInterval() > 0) &&
              (now > device->lastStateNotify()) &&
              (now - device->lastStateNotify() >
               device->stateNotifyInterval()))) &&
            (device->isDeviceStateComplete())) {
          LOG->notice(F("Notify interval is up -> Device state changed... %d"), device->lastStateNotify());
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
    if (_restartFlag) {
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
        LOG->notice(F("Go to deep sleep. Bye..."));
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
          LOG->error(F("deepsleep with GPIO not supported bye ESP8266"));
#endif
        } else if (_deepSleepFlag->deepSleepMode() == DEEP_SLEEP_GPIO_LOW) {
#ifdef ESP32
          esp_sleep_enable_ext0_wakeup((gpio_num_t)_deepSleepFlag->deepSleepGPIO(), LOW);
          esp_deep_sleep_start();
#elif ESP8266
          LOG->error(F("deepsleep with GPIO not supported bye ESP8266"));
#endif
        } else {
          LOG->notice(F("Going to deep sleep failed. Seconds or GPIO missing..."));
        }
      }
    }
    return result;
  }

  ~WNetwork() {}

  void setDebuggingOutput(Print *output) {
    _debuggingOutput = output;
    LOG->setOutput(output, (DEBUG ? LOG_LEVEL_NOTICE : LOG_LEVEL_SILENT), true, true);
  }

  void setOnNotify(THandlerFunction onNotify) { _onNotify = onNotify; }

  void setOnConfigurationFinished(THandlerFunction onConfigurationFinished) {
    _onConfigurationFinished = onConfigurationFinished;
  }

  bool publishMqtt(const char *topic, WStringStream *response, bool retained = false) {
    LOG->notice(F("MQTT... '%s'; %s"), topic, response->c_str());
    if (isMqttConnected()) {
      if (_mqttClient->publish(topic, response->c_str(), retained)) {
        LOG->notice(F("MQTT sent. Topic: '%s'"), topic);
        return true;
      } else {
        LOG->notice(F("Sending MQTT message failed, rc=%d"), _mqttClient->state());
        this->disconnectMqtt();
        return false;
      }
    } else {
      if (strcmp(mqttServer(), "") != 0) {
        LOG->notice(F("Can't send MQTT. Not connected to server: %s"), mqttServer());
      }
      return false;
    }
    LOG->notice(F("publish MQTT mystery... "));
  }

  bool publishMqtt(const char *topic, const char *key, const char *value) {
    if ((this->isMqttConnected()) && (this->isSupportingMqtt())) {
      WStringStream *response = getResponseStream();
      WJson json(response);
      json.beginObject();
      json.propertyString(key, value, nullptr);
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
      _pages->forEach([this](WPageItem *pageItem, const char *id) { WPage::bind(_webServer, id, pageItem); });
      _webServer->on("/events",
                     HTTP_POST,
                     std::bind(&WNetwork::_handleHttpEvent, this, std::placeholders::_1),
                     std::bind(&WNetwork::_handleHttpProgressEvent, this,
                               std::placeholders::_1, std::placeholders::_2,
                               std::placeholders::_3, std::placeholders::_4,
                               std::placeholders::_5, std::placeholders::_6),
                     std::bind(&WNetwork::_handleHttpFinishEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
      // WebThings
      if ((aDeviceNeedsWebThings) && (this->isWifiConnected())) {
        // Make the thing discoverable
        String mdnsName = String(_hostname);
        if (MDNS.begin(mdnsName.c_str())) {
          MDNS.addService("http", "tcp", 80);
          MDNS.addServiceTxt("http", "tcp", "url", "http://" + mdnsName + SLASH);
          MDNS.addServiceTxt("http", "tcp", "webthing", "true");
          LOG->notice(F("MDNS responder for Webthings started at '%s'"), _hostname);
        }
        _webServer->on(SLASH, HTTP_GET, std::bind(&WNetwork::_sendDevicesStructure, this, std::placeholders::_1));
        _devices->forEach([this](WDevice *device, const char *id) { _bindWebServerCalls(device); });
      }
      // Start http server
      _webServer->begin();
      LOG->notice(F("webServer started."));
      _notify(true);
    }
  }

  void stopWebServer() {
    if ((isWebServerRunning()) && (!_supportsWebServer) && (!_updateRunning)) {
      LOG->notice(F("Close web configuration."));
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

  bool isSupportingMqtt() { return ((_supportingMqtt->asBool()) && (strcmp(mqttServer(), "") != 0)); }

  bool isInitialMqttSent() { return _initialMqttSent; }

  bool isLastWillEnabled() { return _lastWillEnabled; }

  void setLastWillEnabled(bool lastWillEnabled) {
    _lastWillEnabled = lastWillEnabled;
  }

  const char *getIdx() { return _idx->c_str(); }

  const char *getHostName() { return _hostname; }

  const char *getSsid() { return _ssid->c_str(); }

  const char *getPassword() { return SETTINGS->getString(WC_PASSWORD); }

  const char *mqttServer() { return SETTINGS->getString(WC_MQTT_SERVER); }

  const char *mqttPort() { return SETTINGS->getString(WC_MQTT_PORT); }

  const char *mqttBaseTopic() { return _mqttBaseTopic->c_str(); }

  const char *mqttSetTopic() { return _mqttSetTopic->c_str(); }

  const char *mqttStateTopic() { return _mqttStateTopic->c_str(); }

  const char *mqttUser() { return SETTINGS->getString(WC_MQTT_USER); }

  const char *mqttPassword() { return SETTINGS->getString(WC_MQTT_PASSWORD); }

  void addDevice(WDevice *device) {
    _devices->add(device);
    _bindWebServerCalls(device);
  }

  void addCustomPage(const char *id, WPageInitializer initializer, bool showInMainMenu = true) {
    _pages->add(new WPageItem(initializer, showInMainMenu), id);
  }

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
    LOG->printLevel(level, msg, args...);
    if ((isMqttConnected()) && ((level == LOG_LEVEL_ERROR) || (level == LOG_LEVEL_NOTICE) || (DEBUG))) {
      WStringStream *response = getResponseStream();
      WJson json(response);
      json.beginObject();
      json.memberName(LOG->getLevelString(level));
      response->print(QUOTE);
      LOG->setOutput(response, level, false, false);
      LOG->printLevel(level, msg, args...);
      this->setDebuggingOutput(_debuggingOutput);
      response->print(QUOTE);
      json.endObject();
      publishMqtt(_mqttBaseTopic->c_str(), response);
    }
  }

  void restart() { _restart(nullptr, "Restart..."); }

  String apSsid() { return _getClientName(false); }
  String apPassword() { return CONFIG_PASSWORD; }

  WList<WPageItem> *pageItems() { return _pages; }

 private:
  WList<WDevice> *_devices;
  WList<WPageItem> *_pages;
  THandlerFunction _onNotify;
  THandlerFunction _onConfigurationFinished;
  bool _updateRunning;
  bool _restartFlag = false;
  DNSServer *_dnsApServer;
  AsyncWebServer *_webServer;
  int _networkState;
  WProperty *_supportingMqtt;
  bool _supportsWebServer;
  WProperty *_ssid;
  WProperty *_idx;
  char *_hostname;
  WProperty *_mqttBaseTopic;
  WProperty *_mqttSetTopic;
  WProperty *_mqttStateTopic;
  WiFiClient *_wifiClient;
  PubSubClient *_mqttClient;
  unsigned long _lastMqttConnect, _lastWifiConnect, _lastWifiConnectFirstTry;
  byte _wifiConnectTrys;
  WStringStream *_responseStream = nullptr;
  WLed *_statusLed;
  bool _statusLedOnIfConnected;
  // WSettings *_settings;
  WDevice *_deepSleepFlag;
  unsigned long _startupTime;
  char _body_data[ESP_MAX_PUT_BODY_SIZE];
  bool _b_has_body_data = false;
  Print *_debuggingOutput;
  bool _initialMqttSent;
  bool _lastWillEnabled;
  bool _waitForWifiConnection;

  bool _aDeviceNeedsWebThings() {
    bool result = false;
    WIterator<WDevice> *it_d = _devices->iterator();
    while ((!result) && (it_d->hasNext())) {
      WDevice *d = it_d->next();
      result = d->needsWebThings();
    }
    return result;
  }

  void _handleDeviceStateChange(WDevice *device, bool complete) {
    LOG->notice(F("Device state changed -> send device state for device '%s'"), device->id());
    String topic = String(mqttBaseTopic()) + SLASH + String(device->id()) + SLASH + String(mqttStateTopic());
    _mqttSendDeviceState(topic, device, complete);
  }

  void _mqttSendDeviceState(String topic, WDevice *device, bool complete) {
    if ((this->isMqttConnected()) && (isSupportingMqtt()) && (device->isDeviceStateComplete())) {
      LOG->notice(F("Send actual device state via MQTT"));

      if (device->sendCompleteDeviceState()) {
        // Send all properties of device in one json structure
        WStringStream *response = getResponseStream();
        WJson json(response);
        json.beginObject();
        if (device->isMainDevice()) {
          json.propertyString("idx", getIdx(), nullptr);
          json.propertyString("ip", getDeviceIp().toString().c_str(), nullptr);
          if (this->isLastWillEnabled()) {
            json.propertyBoolean("alive", true);
          }
          json.propertyString("firmware", VERSION, nullptr);
        }
        device->toJsonValues(&json, MQTT);
        json.endObject();

        _mqttClient->publish(topic.c_str(), (const uint8_t *)response->c_str(),
                             response->length(), true);
        _initialMqttSent = true;
      } else {
        // Send every changed property only
        device->properties()->forEach(
            [this, complete, topic](WProperty *property, const char *id) {
              if ((complete) || (property->changed())) {
                if (property->isVisible(MQTT)) {
                  WStringStream *response = getResponseStream();
                  WJson json(response);
                  property->toJsonValue(&json);
                  _mqttClient->publish(
                      String(topic + SLASH + String(id)).c_str(),
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
    LOG->notice(F("Received MQTT callback. topic: '%s'; payload: '%s'; length: %d"), ptopic, (char *)payload, length);
    String baseT = String(mqttBaseTopic());
    String stateT = String(mqttStateTopic());
    String setT = String(mqttSetTopic());

    String cTopic = String(ptopic);
    if (cTopic.startsWith(baseT)) {
      String topic = cTopic.substring(baseT.length() + 1);
      LOG->notice(F("Topic short '%s'"), topic.c_str());
      // Next is device id
      int i = topic.indexOf(SLASH);
      if (i > -1) {
        String deviceId = topic.substring(0, i);
        LOG->notice(F("look for device id '%s'"), deviceId.c_str());
        WDevice *device = _getDeviceById(deviceId.c_str());
        if (device != nullptr) {
          topic = topic.substring(i + 1);
          if (topic.startsWith(stateT)) {
            if (length == 0) {
              // State request
              topic = topic.substring(stateT.length() + 1);
              if (topic.equals("")) {
                // send all propertiesBase
                LOG->notice(F("Send complete device state..."));
                // Empty payload for topic 'properties' -> send device state
                _mqttSendDeviceState(String(ptopic), device, true);
              } else {
                WProperty *property = device->getPropertyById(topic.c_str());
                if (property != nullptr) {
                  if (property->isVisible(MQTT)) {
                    LOG->notice(F("Send state of property '%s'"),
                                topic);
                    WStringStream *response = getResponseStream();
                    WJson json(response);
                    property->toJsonValue(&json);
                    _mqttClient->publish(String(baseT + SLASH + deviceId + SLASH + stateT + SLASH + topic).c_str(), response->c_str(), true);
                  }
                } else {
                  device->handleUnknownMqttCallback(true, ptopic, topic, (char *)payload, length);
                }
              }
            }
          } else if (topic.startsWith(setT)) {
            if (length > 0) {
              // Set request
              topic = topic.substring(setT.length() + 1);
              if (topic.equals("")) {
                // set all properties
                LOG->notice(F("Try to set several properties for device %s"), device->id());
                WJsonParser *parser = new WJsonParser();
                if (parser->parse((char *)payload, device) == nullptr) {
                  LOG->notice(F("No properties updated for device %s"), device->id());
                } else {
                  LOG->notice(F("One or more properties updated for device %s"), device->id());
                }
                delete parser;
              } else {
                // Try to find property and set single value
                WProperty *property = device->getPropertyById(topic.c_str());
                if (property != nullptr) {
                  if (property->isVisible(MQTT)) {
                    // Set Property
                    LOG->notice(F("Try to set property %s for device %s"), topic, device->id());
                    if (!property->parse((char *)payload)) {
                      LOG->notice(F("Property not updated."));
                    } else {
                      LOG->notice(F("Property updated."));
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
          enableWebServer(String((char *)payload).equals(WC_TRUE));
        }
      }
    }
  }

  bool _mqttReconnect() {
    if (this->isSupportingMqtt()) {
      LOG->notice(F("Connect to MQTT server: %s; user: '%s'; password: '%s'; clientName: '%s'"),
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
          json.propertyString("idx", getIdx(), nullptr);
          json.propertyString("ip", getDeviceIp().toString().c_str(), nullptr);
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
        LOG->notice(F("Connected to MQTT server."));
        //  Send device structure and status
        _mqttClient->subscribe("devices/#");
        _devices->forEach([this](WDevice *device, const char *id) {
          String topic("devices/");
          topic.concat(device->id());
          WStringStream *response = getResponseStream();
          WJson json(response);
          json.beginObject();
          json.propertyString("url", "http://", getDeviceIp().toString().c_str(), "/things/", device->id(), nullptr);
          json.propertyString("stateTopic", mqttBaseTopic(), SLASH, device->id(), SLASH, mqttStateTopic(), nullptr);
          json.propertyString("setTopic", mqttBaseTopic(), SLASH, device->id(), SLASH, mqttSetTopic(), nullptr);
          json.endObject();
          _mqttClient->publish(topic.c_str(), response->c_str(), false);
        });
        _mqttClient->unsubscribe("devices/#");
        // Subscribe to device specific topic
        _mqttClient->subscribe(String(String(mqttBaseTopic()) + "/#").c_str());
        _notify(false);
        return true;
      } else {
        LOG->notice(F("Connection to MQTT server failed, rc=%d"), _mqttClient->state());
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
      _devices->forEach([this](WDevice *device, const char *id) { _handleDeviceStateChange(device, false); });
    }
    if (_onNotify) {
      _onNotify();
    }
  }

  void _handleHttpEvent(AsyncWebServerRequest *request) {
    LOG->debug("handle form action ... simple");    
    WStringList* args = new WStringList();
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter *p = request->getParam(i);
      LOG->debug("..POST[%s]: %s", p->name().c_str(), p->value().c_str());
      args->add(p->value().c_str(), p->name().c_str());
    }
    LOG->debug("handle form action ... b"); 
    _handleHttpEventArgs(request, args);         
    LOG->debug("handle form action ... c"); 
    delete args;    
    LOG->debug("handle form action ... d"); 
  }

  void _handleHttpFinishEvent(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    LOG->debug("handle form action ... complicated");
    WStringStream* ss = getResponseStream();
    for (size_t i = 0; i < len; i++) {
      ss->write(data[i]);
    }
    LOG->debug("Received post data: '%s'", ss->c_str());    
    WStringList* args = WJsonParser::asList(ss->c_str());
    _handleHttpEventArgs(request, args);   
    delete args;
  }

  void _handleHttpEventArgs(AsyncWebServerRequest *request, WStringList* args) {
    WPageItem *pi = _pages->getById(args->getById(WC_FORM));
    if (pi != nullptr) {
      WPage* p = pi->initializer();
      WFormResponse* result = p->submitForm(args);
      switch (result->operation) {
        case FO_RESTART : {
          _restart(request, result->message);
          break;
        }          
        case FO_FORCE_AP : {
          SETTINGS->forceAPNextStart();
          _restart(request, result->message);
          break;
        }
        case FO_RESET_ALL : {
          SETTINGS->resetAll();
          _restart(request, result->message);
          break;
        }
        default :
          request->send(200);
      }
      delete result;        
    } else {
      request->send(404);
    }     
  }

  String _getClientName(bool lowerCase) {
    String result = (APPLICATION == nullptr ? "ESP" : String(APPLICATION));
    result.replace(" ", "-");
    if (lowerCase) {
      result.replace("-", "");
      result.toLowerCase();
    }
    String chipId = String(WUtils::getChipId());
    int resLength = result.length() + chipId.length() + 1 - 32;
    if (resLength > 0) {
      result.substring(0, 32 - resLength);
    }
    return result + "_" + chipId;
  }

  void _handleHttpProgressEvent(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    LOG->debug("handle update progress...");    
    // Start firmwareUpdate
    _updateRunning = true;
    // Close existing MQTT connections
    this->disconnectMqtt();
    // Start update
    if (!index) {
      LOG->notice(F("Update starting: %s"), filename.c_str());
      size_t content_len = request->contentLength();
      int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
#ifdef ESP8266
      Update.runAsync(true);
      if (!Update.begin(content_len, cmd)) {
#elif ESP32
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
#endif
        LOG->debug(F("Can't start update"));
      }
    }
    // Upload running
    if (len) {
      if (Update.write(data, len) != len) {
        LOG->debug(F("Can't upload file"));
      }
    }
    // Upload finished
    if (final) {
      if (!Update.end(true)) {
        LOG->debug(F("Can't finish update"));
      }
    }
  }

  void _restart(AsyncWebServerRequest *request, const char *reasonMessage) {
    _restartFlag = true;
    if (request != nullptr) {
      request->client()->setNoDelay(true);
      WRestartPage *rp = new WRestartPage(reasonMessage);
      AsyncResponseStream *stream = request->beginResponseStream(WC_TEXT_HTML);
      rp->toString(stream);
      request->send(stream);
    }
  }

  void _loadNetworkSettings() {
    _idx = SETTINGS->setNetworkString(WC_ID, _getClientName(true).c_str());
    _hostname = new char[strlen_P(_idx->c_str()) + 1];
    strcpy_P(_hostname, _idx->c_str());
    for (int i = 0; i < strlen(_hostname); i++) {
      if ((_hostname[i] == '.') || (_hostname[i] == ' ')) {
        _hostname[i] = '-';
      }
    }
    _ssid = SETTINGS->setNetworkString(WC_SSID, "");
    SETTINGS->setNetworkString(WC_PASSWORD, "");
    _supportsWebServer = true;
    _supportingMqtt = SETTINGS->setNetworkBoolean("supportingMqtt", true);
    SETTINGS->setNetworkString(WC_MQTT_SERVER, "");
    SETTINGS->setNetworkString(WC_MQTT_PORT, "1883");
    SETTINGS->setNetworkString(WC_MQTT_USER, "");
    SETTINGS->setNetworkString(WC_MQTT_PASSWORD, "");
    _mqttBaseTopic = SETTINGS->setNetworkString("mqttTopic", getIdx());
    _mqttStateTopic = SETTINGS->setNetworkString("mqttStateTopic", DEFAULT_TOPIC_STATE);
    _mqttSetTopic = SETTINGS->setNetworkString("mqttSetTopic", DEFAULT_TOPIC_SET);
    if (SETTINGS->existsNetworkSettings()) {
      if (strcmp(mqttBaseTopic(), "") == 0) {
        _mqttBaseTopic->asString(_getClientName(true).c_str());
      }
      if ((isSupportingMqtt()) && (_mqttClient != nullptr)) {
        this->disconnectMqtt();
      }
      LOG->debug(F("SSID: '%s'; MQTT enabled: %T; MQTT server: '%s'; MQTT port: %s; WebServer started: %T"),
                 getSsid(), isSupportingMqtt(), mqttServer(), mqttPort(), isWebServerRunning());
    } else {
      LOG->notice(F("Network settings are missing."));
    }
  }

  void _handleUnknown(AsyncWebServerRequest *request) {
    if (!isUpdateRunning()) {
      request->send(404);
    }
  }

  void _sendDevicesStructure(AsyncWebServerRequest *request) {
    if (!isUpdateRunning()) {
      LOG->notice(F("Send description for all devices... "));
      AsyncResponseStream *response = request->beginResponseStream(APPLICATION_JSON);
      WJson *json = new WJson(response);
      json->beginArray();
      _devices->forEach([this, json](WDevice *device, const char *id) {
        if (device->isVisible(WEBTHING)) {
          LOG->notice(F("Send description for device %s "), device->id());
          device->toJsonStructure(json, "", WEBTHING);
        }
      });
      json->endArray();
      request->send(response);
    }
  }

  void _sendDeviceStructure(AsyncWebServerRequest *request, WDevice *&device) {
    if (!isUpdateRunning()) {
      LOG->notice(F("Send description for device: %s"), device->id());
      AsyncResponseStream *response = request->beginResponseStream(APPLICATION_JSON);
      WJson json(response);
      device->toJsonStructure(&json, "", WEBTHING);
      request->send(response);
    }
  }

  void _sendDeviceValues(AsyncWebServerRequest *request, WDevice *&device) {
    if (!isUpdateRunning()) {
      LOG->notice(F("Send all properties for device: "), device->id());
      AsyncResponseStream *response = request->beginResponseStream(APPLICATION_JSON);
      WJson json(response);
      json.beginObject();
      if (device->isMainDevice()) {
        json.propertyString("idx", getIdx(), nullptr);
        json.propertyString("ip", getDeviceIp().toString().c_str(), nullptr);
        json.propertyString("firmware", VERSION, nullptr);
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
      LOG->notice(F("Set property value:"));
      if (!_b_has_body_data) {
        request->send(422);
        return;
      }
      WJsonParser parser;
      WProperty *property = parser.parse(_body_data, device);
      if (property != nullptr) {
        // response new value
        LOG->notice(F("Set property value: %s (web request) %s"), property->title(), _body_data);
        AsyncResponseStream *response = request->beginResponseStream(APPLICATION_JSON);
        WJson json(response);
        json.beginObject();
        property->toJsonValue(&json /*tbi id, id!!!*/);
        json.endObject();
        request->send(response);
      } else {
        // unable to parse json
        LOG->notice(F("unable to parse json: %s"), _body_data);
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
      json.propertyString("error", msg, nullptr);
      json.propertyInteger("status", status);
      json.endObject();
      request->send(response);
    }
  }

  void _bindWebServerCalls(WDevice *device) {
    if (this->isWebServerRunning()) {
      LOG->notice(F("Bind webServer calls for device %s"), device->id());
      String deviceBase("/things/");
      deviceBase.concat(device->id());
      device->properties()->forEach([this, device,
                                     deviceBase](WProperty *property, const char *id) {
        if (property->isVisible(WEBTHING)) {
          String propertyBase = deviceBase + "/properties/" + id;
          _webServer->on(propertyBase.c_str(), HTTP_GET, std::bind(&WNetwork::_getPropertyValue, this, std::placeholders::_1, property));
          _webServer->on(propertyBase.c_str(), HTTP_PUT, std::bind(&WNetwork::_setPropertyValue, this, std::placeholders::_1, device),
                         NULL, std::bind(&WNetwork::_handleBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        }
      });
      String propertiesBase = deviceBase + "/properties";
      _webServer->on(propertiesBase.c_str(), HTTP_GET, std::bind(&WNetwork::_sendDeviceValues, this, std::placeholders::_1, device));
      _webServer->on(deviceBase.c_str(), HTTP_GET, std::bind(&WNetwork::_sendDeviceStructure, this, std::placeholders::_1, device));
      device->bindWebServerCalls(_webServer);
    }
  }

  WDevice *_getDeviceById(const char *deviceId) {
    return _devices->getIf([deviceId](WDevice *d) { return (strcmp(d->id(), deviceId) == 0); });
  }
};

#endif