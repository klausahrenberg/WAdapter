#ifndef W_NETWORK_H
#define W_NETWORK_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266mDNS.h>
#include <Updater.h>
#define U_PART U_FS
#elif ARDUINO_ARCH_ESP32
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFi.h>
#define U_PART U_FLASH
#endif
#include <DNSServer.h>
#include <StreamString.h>

#include "WDevice.h"
#include "WJsonParser.h"
#include "WList.h"
#include "WSettings.h"
#include "WStringStream.h"
#include "WiFiClient.h"
#include "html/WNetworkPages.h"
#ifndef W_NO_UI
#include "html/WThingsUiPage.h"
#endif
#include "html/WebApp.h"
#include "hw/WLed.h"

#define NO_LED 0xFF
#define ESP_MAX_PUT_BODY_SIZE 512
#define WIFI_RECONNECTION 50000
#define WIFI_RECONNECTION_TRYS 2
//upper limit for how long a background retry may occupy the shared radio while
//the configuration AccessPoint is open. it is only the safety net - a retry
//that fails is normally ended as soon as the disconnect event reports it
#define WIFI_RECONNECTION_AP_WINDOW 12000
const char* CONFIG_PASSWORD = "12345678";
const char* APPLICATION_JSON = "application/json";
const char* TEXT_PLAIN = "text/plain";
const char* DEFAULT_TOPIC_STATE = "properties";
const char* DEFAULT_TOPIC_SET = "set";
const char* SLASH = "/";

#ifdef ARDUINO_ARCH_ESP8266
WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
#endif

//a gateway needs one connection, every more takes heap from the pages
#define MAX_PROPERTY_PUSH_CLIENTS 2
#define SIZE_PROPERTY_STATUS 256

class WNetwork {
 public:
  typedef std::function<void()> THandlerFunction;

  WNetwork(int statusLedPin, Print* debuggingOutput = &Serial) {
    SETTINGS = new WSettings();
    WiFi.disconnect();
    // WiFi.mode(WIFI_STA);
#ifdef ARDUINO_ARCH_ESP8266
    WiFi.encryptionType(ENC_TYPE_CCMP);
    WiFi.setOutputPower(20.5);
    WiFi.setPhyMode(WiFiPhyMode::WIFI_PHY_MODE_11N);
#endif
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    _wifiClient = new WiFiClient();
    _webServer = nullptr;
    _dnsApServer = nullptr;
    _devices = new WList<WDevice>();

    this->setDebuggingOutput(debuggingOutput);
    _updateRunning = false;
    _deepSleepFlag = nullptr;
    _waitForWifiConnection = false;
    _startupTime = millis();
    _loadNetworkSettings();
    _lastWifiConnect = 0;
    _apStaRetryStarted = 0;
    _apStaRetryFailed = false;
    _wifiConnectTrys = 0;

#ifdef ARDUINO_ARCH_ESP8266
    gotIpEventHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP& event) { onGotIP(); });
    disconnectedEventHandler = WiFi.onStationModeDisconnected(
        [this](const WiFiEventStationModeDisconnected& event) {
          onDisconnected();
        });
#else
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) { onGotIP(); },
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) { onDisconnected(); },
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#endif
    _statusLed = nullptr;
    setStatusLedPin(statusLedPin, false);
    LOG->debug(F("firmware: %s"), VERSION);
#ifndef W_NO_UI
    this->addWebPage(WC_UI, [this]() { return new WThingsUiPage(_devices); }, PSTR("Control"));
#endif
    this->addWebPage(WC_WIFI, [this]() { return new WNetworkPage(); }, PSTR("Configure network"));
    this->addWebPage(WC_FIRMWARE, [this]() { return new WFirmwarePage(); }, PSTR("Firmware"));
    this->addWebPage(WC_INFO, [this]() { return new WInfoPage((millis() - _startupTime) / 1000 / 60); }, PSTR("Info"));
    this->addWebPage(WC_RESET, [this]() { return new WResetPage(this); }, PSTR("Reboot"));
  }

  ~WNetwork() {
    delete SETTINGS;
    delete _devices;
    if (_webServer) delete _webServer;
    if (_dnsApServer) delete _dnsApServer;
    if (_webApp) delete _webApp;
    if (_hostname) delete[] _hostname;
  }

  void setStatusLedPin(int statusLedPin, bool statusLedOnIfConnected) {
    this->setStatusLed(
        (statusLedPin != NO_LED ? new WLed(statusLedPin) : nullptr),
        statusLedOnIfConnected);
  }

  void setStatusLed(WLed* statusLed, bool statusLedOnIfConnected = false) {
    _statusLedOnIfConnected = statusLedOnIfConnected;
    _statusLed = statusLed;
    if (_statusLed != nullptr) {
      _statusLed->on(true, 500);
    }
    _updateLedState();
  }

  void onGotIP() {
    LOG->notice(F("Station connected, IP: %s Host Name is '%s'"), this->getDeviceIp().toString().c_str(), _hostname);
    _wifiConnectTrys = 0;
    _apStaRetryStarted = 0;
    _apStaRetryFailed = false;
    _notify(false);
  }

  void onDisconnected() {
    //a retry running next to the ap has failed: loop() frees the radio for the
    //ap right away instead of waiting out the whole window. the station is not
    //touched from inside the event itself
    if (_apStaRetryStarted != 0) {
      _apStaRetryFailed = true;
    }
    if (!isSoftAP()) {
      LOG->notice("Station disconnected");
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
    //an update in progress owns whichever webserver is currently answering
    //it - reconnecting in the background or tearing an ap down would disturb
    //an active upload on either interface, so the wifi/ap state is frozen for
    //the duration of the update and only re-evaluated once it is done
    if (!isUpdateRunning()) {
      if ((WiFi.status() == WL_CONNECTED) && (isSoftAP()) && (!SETTINGS->forceNetworkAccessPoint())) {
        //the configured network is reachable again: the ap opened as a fallback
        //for configuration has done its job and can close
        _stopAccessPoint();
      } else if (WiFi.status() != WL_CONNECTED) {
        bool hasSsid = (strcmp(getSsid(), "") != 0);
        //a retry that runs next to an open ap gets a limited time window: both
        //share one radio, so a station that keeps searching scans across all
        //channels and takes the ap off its own channel for as long as it runs.
        //once the window is up the station is silenced again until the next try
        if ((_apStaRetryStarted != 0) &&
            ((_apStaRetryFailed) ||
             (now - _apStaRetryStarted > WIFI_RECONNECTION_AP_WINDOW))) {
          _stopStationRetry(now);
        }
        //not connected: keep retrying in the background, at the usual interval,
        //whether or not an own ap is already open for configuration - that way
        //a returning router is picked up without a restart of the device
        if ((hasSsid) && (_apStaRetryStarted == 0) &&
            ((_lastWifiConnect == 0) || (now - _lastWifiConnect > WIFI_RECONNECTION)) &&
            (((isSoftAP()) && (_apStaRetry)) || (_wifiConnectTrys < WIFI_RECONNECTION_TRYS))) {
          if (isSoftAP()) {
            LOG->notice("Retrying '%s' while the configuration AccessPoint is open", getSsid());
          } else {
            _wifiConnectTrys++;
            LOG->notice("Connecting to '%s': %d. try", getSsid(), _wifiConnectTrys);
            if (_wifiConnectTrys == 1) {
              _lastWifiConnectFirstTry = now;
            }
          }
          _connectWifi(now);
        }
        //opens an own ap for configuration once reconnecting failed enough
        //times, or immediately if there is nothing configured to connect to
        if ((!isSoftAP()) &&
            ((!hasSsid) || (!SETTINGS->existsNetworkSettings()) ||
             (SETTINGS->forceNetworkAccessPoint()) || (_wifiConnectTrys >= WIFI_RECONNECTION_TRYS))) {
          _startAccessPoint();
        }
      }
    }
    if (isSoftAP()) {
      _dnsApServer->processNextRequest();
    }
    bool result = ((!isSoftAP()) && (!isUpdateRunning()));
    // WebServer
    if ((isWifiConnected()) && (_supportsWebServer)) {
      this->startWebServer();
    }
    for (AsyncWebSocket* ws : _webSockets) ws->cleanupClients(MAX_PROPERTY_PUSH_CLIENTS);
    if (!isUpdateRunning()) {
      if (_webApp != nullptr) {
        _webApp->loop(now);
      }
      // Loop led
      if (_statusLed != nullptr) {
        _statusLed->loop(now);
      }
      bool allStatesComplete = true;
      bool stateUpd = false;
      // Loop Devices
      _devices->forEach([this, now](int index, WDevice* device, const char* id) {
        device->loop(now);
      });

// WebThingAdapter
#ifdef ARDUINO_ARCH_ESP8266
      if ((!isUpdateRunning()) && (isWifiConnected())) {
        if (MDNS.isRunning()) {
          MDNS.update();
        } else if ((_mdnsWanted) && (now - _lastMdnsTry > 10000)) {
          //the responder did not come up (or went down), try it again
          _lastMdnsTry = now;
          _startMdns();
        }
      }
#endif
    }
    // Restart required?
    if (_restartFlag) {
      _updateRunning = false;
      if (_onConfigurationFinished) {
        _onConfigurationFinished();
      }
      delay(1000);
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
#ifdef ARDUINO_ARCH_ESP32
          esp_sleep_enable_ext0_wakeup((gpio_num_t)_deepSleepFlag->deepSleepGPIO(), HIGH);
          esp_deep_sleep_start();
#elif ARDUINO_ARCH_ESP8266
          LOG->error(F("deepsleep with GPIO not supported bye ESP8266"));
#endif
        } else if (_deepSleepFlag->deepSleepMode() == DEEP_SLEEP_GPIO_LOW) {
#ifdef ARDUINO_ARCH_ESP32
          esp_sleep_enable_ext0_wakeup((gpio_num_t)_deepSleepFlag->deepSleepGPIO(), LOW);
          esp_deep_sleep_start();
#elif ARDUINO_ARCH_ESP8266
          LOG->error(F("deepsleep with GPIO not supported bye ESP8266"));
#endif
        } else {
          LOG->notice(F("Going to deep sleep failed. Seconds or GPIO missing..."));
        }
      }
    }
    return result;
  }

  void setDebuggingOutput(Print* output) {
    _debuggingOutput = output;
    LOG->setOutput(output, (DEBUG ? LOG_LEVEL_NOTICE : LOG_LEVEL_SILENT), true, true);
  }

  void setOnNotify(THandlerFunction onNotify) { _onNotify = onNotify; }

  void setOnConfigurationFinished(THandlerFunction onConfigurationFinished) {
    _onConfigurationFinished = onConfigurationFinished;
  }

  // Creates a web server. If Wifi is not connected, then an own AP will be
  // created
  void startWebServer() {
    if (!isWebServerRunning()) {
      bool aDeviceNeedsWebThings = _aDeviceNeedsWebThings();

      _webServer = new AsyncWebServer(80);
      _webServer->onNotFound(std::bind(&WNetwork::_handleUnknown, this, std::placeholders::_1));
      if (_webApp != nullptr) {
        _webApp->bindWebServerCalls(_webServer);
      }
      if (WEB_SOCKETS != nullptr) {
        WEB_SOCKETS->begin();
        //_webServer->addHandler(WEB_SOCKETS);
      }
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
        _mdnsWanted = true;
        _startMdns();
        // root page sends json structure for webthing
        _webServer->on(SLASH, HTTP_GET, std::bind(&WNetwork::_sendDevicesStructure, this, std::placeholders::_1));
        _devices->forEach([this](int index, WDevice* device, const char* id) { _bindWebServerCalls(device); });
      } else {
        if (_webApp != nullptr) {
          _webApp->bindRootPage(_webServer);
        }
        //no web thing to serve, but a device may bring pages and files of its own
        _devices->forEach([this](int index, WDevice* device, const char* id) { device->bindWebServerCalls(_webServer); });
      }
      // Start http server
      _webServer->begin();
      LOG->notice(F("webServer started."));
      // update
      _notify(true);
    }
  }

  void stopWebServer() {
    if ((isWebServerRunning()) && (!_supportsWebServer) && (!_updateRunning)) {
      if (WEB_SOCKETS != nullptr) {
        WEB_SOCKETS->close();
        //_webServer->removeHandler(WEB_SOCKETS);
      }
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

  IPAddress getDeviceIp() {
    return (isSoftAP() ? WiFi.softAPIP() : WiFi.localIP());
  }

  //whether the station keeps retrying while an own ap is open. it shares the
  //one radio with the ap, so every retry makes the ap refuse associations for
  //as long as it scans - a device whose configuration ap has to be reliable
  //turns it off
  bool apStaRetry() { return _apStaRetry; }

  void setApStaRetry(bool apStaRetry) { _apStaRetry = apStaRetry; }

  bool supportsWebServer() { return _supportsWebServer; }

  void setSupportsWebServer(bool supportsWebServer) {
    _supportsWebServer = supportsWebServer;
  }

  const char* getIdx() { return _idx->asString(); }

  const char* getHostName() { return _hostname; }

  const char* getSsid() { return _ssid->asString(); }

  const char* getPassword() { return SETTINGS->getString(WC_PASSWORD); }

  void addDevice(WDevice* device) {
    _devices->add(device);
    _bindWebServerCalls(device);
  }

  void addWebPage(const char* id, WebPageInitializer initializer, const char* title, bool showInMainMenu = true) {
    if (_webApp == nullptr) _webApp = new WebApp();
    _webApp->addWebPage(id, initializer, title, showInMainMenu);
  }

  AsyncWebServer* webServer() { return _webServer; }

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
  }

  void restart() { _restartFlag = true; }

  //a device that is done with its work can ask for the deep sleep itself, so
  //sleeping is not bound to a mqtt publish any more. the device's own off()
  //still decides the moment, this only names who is asked
  void allowDeepSleep(WDevice* device) {
    if ((device != nullptr) && (device->deepSleepMode() != DEEP_SLEEP_NONE)) {
      _deepSleepFlag = device;
    }
  }

  String apSsid() { return _getClientName(false); }
  String apPassword() { return CONFIG_PASSWORD; }

  WebApp* webApp() { return _webApp; }

 private:
  WList<WDevice>* _devices;
  THandlerFunction _onNotify;
  THandlerFunction _onConfigurationFinished;
  bool _updateRunning;
  bool _restartFlag = false;
  DNSServer* _dnsApServer;
  AsyncWebServer* _webServer;
  std::list<AsyncWebSocket*> _webSockets;
  int _networkState;
  bool _supportsWebServer;
  WValue* _ssid;
  WValue* _idx;
  char* _hostname;
  WiFiClient* _wifiClient;
  unsigned long _lastWifiConnect, _lastWifiConnectFirstTry;
  //!= 0 while a background retry is running with the AccessPoint open
  unsigned long _apStaRetryStarted;
  //set from the disconnect event when that retry has failed
  bool _apStaRetryFailed;
  byte _wifiConnectTrys;
  bool _apStaRetry = true;
  WLed* _statusLed;
  bool _statusLedOnIfConnected;
  WDevice* _deepSleepFlag;
  unsigned long _startupTime;
  char _body_data[ESP_MAX_PUT_BODY_SIZE];
  bool _b_has_body_data = false;
  Print* _debuggingOutput;
  bool _waitForWifiConnection;
  //the hand over from the body handler of a post to its request handler, both
  //of which run for one and the same request - consumed and cleared there, so
  //nothing of one request is ever read by the next one
  WFormResponse _postResponse = WFormResponse(FO_NONE);
  //tells the request handler that the body handler has worked the post out
  //already, which FO_NONE alone cannot say - it also means 'nothing yet'
  bool _postHandled = false;
  WebApp* _webApp = nullptr;

  bool _mdnsWanted = false;
  unsigned long _lastMdnsTry = 0;

  void _startMdns() {
    String mdnsName = String(_hostname);
#ifdef ARDUINO_ARCH_ESP8266
    if (MDNS.begin(mdnsName.c_str())) {
      MDNS.addService(WC_HTTP, WC_TCP, 80);
      MDNS.addServiceTxt(WC_HTTP, WC_TCP, WC_URL, "http://" + mdnsName + SLASH);
      MDNS.addServiceTxt("http", "tcp", "webthing", "true");
      LOG->notice(F("MDNS responder for Webthings started at '%s'"), _hostname);
    } else {
      LOG->notice(F("MDNS responder could not be started"));
    }
#elif ARDUINO_ARCH_ESP32
    esp_log_level_set("mdns", ESP_LOG_NONE);
    esp_log_level_set("udp", ESP_LOG_NONE);
    if (MDNS.begin(mdnsName.c_str())) {
      MDNS.addService(WC_HTTP, WC_TCP, 80);
      MDNS.addServiceTxt(WC_HTTP, WC_TCP, WC_URL, "http://" + mdnsName + SLASH);
      MDNS.addServiceTxt("http", "tcp", "webthing", "true");
    }
#endif
  }

  bool _aDeviceNeedsWebThings() {
    return (_devices->getIf([](WDevice* d) { return d->needsWebThings(); }) != nullptr);
  }

  void _updateLedState() {
    if (_statusLed != nullptr) {
      if (isWifiConnected()) {
        // off
        _statusLed->on(_statusLedOnIfConnected, 0);
      } else if (isSoftAP()) {
        _statusLed->on(!_statusLedOnIfConnected, 0);
      } else {
        _statusLed->on(true, 500);
      }
    }
  }

  void _notify(bool sendState) {
    _updateLedState();
    if (_onNotify) {
      _onNotify();
    }
  }

  void _handleHttpEvent(AsyncWebServerRequest* request) {
    LOG->debug(F("Simple http event handling"));
    //where the post carries json, the body handler of this same request has
    //worked the form out before this one runs. what it left is picked up here
    //and the members are free again right away
    WFormResponse response = _postResponse;
    bool handledInBody = _postHandled;
    _postResponse = WFormResponse(FO_NONE);
    _postHandled = false;
    //set where handleHttpEventArgs has answered the request on its own
    bool answered = false;
    if (!handledInBody) {
      WList<WValue>* args = new WList<WValue>();
      int params = request->params();
      for (int i = 0; i < params; i++) {
        const AsyncWebParameter* p = request->getParam(i);
        LOG->debug("..POST[%s]: %s", p->name().c_str(), p->value().c_str());
        args->add(new WValue(p->value().c_str()), p->name().c_str());
      }
      response = _webApp->handleHttpEventArgs(request, args, &answered);
      delete args;
    }
    if (response.operation != FO_NONE) {
      AsyncResponseStream* stream = request->beginResponseStream(WC_TEXT_HTML, SIZE_RESPONSE_STREAM);
      WebPage* result = new WRestartPage(response.message == nullptr ? PSTR("Restart...") : response.message);
      if (_webApp != nullptr) {
        _webApp->toString(stream, result, PSTR("Restart"));
      } else {
        result->add(result->createControls());
        result->toString(stream);
      }
      request->client()->setNoDelay(true);
      request->send(stream);
      delete result;
      switch (response.operation) {
        case FO_RESTART: {
          restart();
          break;
        }
        case FO_FORCE_AP: {
          SETTINGS->forceAPNextStart();
          restart();
          break;
        }
        case FO_RESET_ALL: {
          SETTINGS->resetAll();
          restart();
          break;
        }
        //FO_NONE does not reach this, the block is only entered without it
        default:
          break;
      }
    } else if ((!handledInBody) && (!answered)) {
      //a page has taken the form without asking for a restart, and nobody has
      //answered the request yet - without this the browser would wait for an
      //answer that never comes
      request->send(200);
    }
  }

  void _handleHttpFinishEvent(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    LOG->debug(F("Advanced http event handling"));
    WStringStream* ss = createResponseStream();
    for (size_t i = 0; i < len; i++) {
      ss->write(data[i]);
    }
    WList<WValue>* args = WJsonParser::asMap(ss->c_str());
    bool answered = false;
    _postResponse = _webApp->handleHttpEventArgs(request, args, &answered);
    //the request handler runs after this one for the very same request: it is
    //told not to read the post a second time, which it would do without a form
    //name in hand and answer with a 404 on top of the answer given here
    _postHandled = true;
    delete ss;
    //a restart is answered with its page by the request handler, everything
    //else is settled here - unless handleHttpEventArgs has answered already
    if ((_postResponse.operation == FO_NONE) && (!answered)) request->send(200);
    delete args;
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

  void _handleHttpProgressEvent(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
    LOG->debug("handle update progress...");
    // Start firmwareUpdate
    _updateRunning = true;
    // Start update
    if (!index) {
      LOG->notice(F("Update starting: %s"), filename.c_str());
      size_t content_len = request->contentLength();
      int cmd = U_FLASH;
#ifdef ARDUINO_ARCH_ESP32
      if (filename.indexOf("spiffs") > -1 || filename.indexOf("littlefs") > -1) {
        cmd = U_SPIFFS;  // Für Dateisystem-Updates
      }

      // Wichtig: Für ESP32 muss die Größe korrekt sein
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
        LOG->debug(F("Can't start update"));
        Update.printError(Serial);
      }
#else
      // ESP8266 Code
      cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
      Update.runAsync(true);
      if (!Update.begin(content_len, cmd)) {
        LOG->debug(F("Can't start update"));
      }
#endif
    }
    // Upload running
    if (len) {
      if (Update.write(data, len) != len) {
        LOG->debug(F("Can't upload file"));
        Update.printError(Serial);
      }
    }
    // Upload finished
    if (final) {
      if (!Update.end(true)) {
        LOG->debug(F("Can't finish update"));
        Update.printError(Serial);
      }
    }
  }

  //the actual connection attempt, shared by the pre-ap tries and the
  //background retries that keep running while the ap stays open
  void _connectWifi(unsigned long now) {
    if (!isSoftAP()) {
      //the hostname only needs to be set once, before there is an own ap: on
      //esp32 that requires resetting the wifi mode first, which would drop
      //an own ap if it was already open
#ifdef ARDUINO_ARCH_ESP8266
      // Workaround: if disconnect is not called, WIFI connection fails
      // after first startup
      WiFi.disconnect();
      WiFi.hostname(_hostname);
#elif ARDUINO_ARCH_ESP32
      // Workaround: WiFi.setHostName now only works if: - You call it before calling WiFi.mode(WIFI_STA)
      // and ensure that the mode is not WIFI_STA already before calling WiFi.setHostName (by first calling WiFi.mode(WIFI_MODE_NULL)
      WiFi.mode(WIFI_MODE_NULL);
      WiFi.setHostname(_hostname);
#endif
    }
    WiFi.mode(isSoftAP() ? WIFI_AP_STA : WIFI_STA);
#ifdef ARDUINO_ARCH_ESP32
    //next to an open ap the station must not retry on its own: every driver
    //retry is another scan across all channels on the radio the ap needs, and
    //those retries come every few seconds. the retries are driven from loop()
    //instead, so they stay within a bounded window
    WiFi.setAutoReconnect(!isSoftAP());
#endif
    if (isSoftAP()) {
      _apStaRetryStarted = now;
      _apStaRetryFailed = false;
    }
    WiFi.begin(getSsid(), getPassword());
    //waiting here blocks everything including the ap's webserver, so it is only
    //done while there is no ap that has to stay responsive
    while ((_waitForWifiConnection) && (!isSoftAP()) && (WiFi.status() != WL_CONNECTED)) {
      delay(100);
      if (millis() - now >= 5000) {
        break;
      }
    }
    _lastWifiConnect = now;
  }

  //ends a background retry: disconnecting stops the station from searching, so
  //the radio belongs to the ap alone again until the next try is due
  void _stopStationRetry(unsigned long now) {
    _apStaRetryStarted = 0;
    _apStaRetryFailed = false;
    _lastWifiConnect = now;
    WiFi.disconnect(false);
  }

  // opens the own ap for configuration, next to the station that keeps
  // retrying the configured network in the background
  void _startAccessPoint() {
    String apSsid = this->apSsid();
    LOG->notice(F("Start AccessPoint for configuration. SSID '%s'; password '%s'"), apSsid.c_str(), this->apPassword().c_str());
    _dnsApServer = new DNSServer();
    WiFi.mode(WIFI_AP_STA);
    _apStaRetryStarted = 0;
    _apStaRetryFailed = false;
    _lastWifiConnect = millis();
    WiFi.softAP(apSsid.c_str(), this->apPassword().c_str());
    _dnsApServer->setErrorReplyCode(DNSReplyCode::NoError);
    _dnsApServer->start(53, "*", WiFi.softAPIP());
    this->startWebServer();
    //lets the display switch over to the ap screen with the qr code
    _notify(false);
  }

  // closes the own ap once the configured network is reachable again
  void _stopAccessPoint() {
    LOG->notice(F("Connected to '%s'. Closing the configuration AccessPoint."), getSsid());
    if (WEB_SOCKETS != nullptr) {
      WEB_SOCKETS->close();
    }
    _webServer->end();
    delete _webServer;
    _webServer = nullptr;
    _dnsApServer->stop();
    delete _dnsApServer;
    _dnsApServer = nullptr;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
#ifdef ARDUINO_ARCH_ESP32
    //no ap left to share the radio with: the station gets its own retries and
    //its default power saving back
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(true);
#endif
    _apStaRetryStarted = 0;
    _apStaRetryFailed = false;
    _notify(true);
  }

  void _loadNetworkSettings() {
    _idx = SETTINGS->setNetworkString(WC_ID, _getClientName(true).c_str());
    _hostname = new char[strlen_P(_idx->asString()) + 1];
    strcpy_P(_hostname, _idx->asString());
    for (int i = 0; i < strlen(_hostname); i++) {
      if ((_hostname[i] == '.') || (_hostname[i] == ' ')) {
        _hostname[i] = '-';
      }
    }
    _ssid = SETTINGS->setNetworkString(WC_SSID, "");
    SETTINGS->setNetworkString(WC_PASSWORD, "");
    _supportsWebServer = true;
    if (SETTINGS->existsNetworkSettings()) {
      if (strcmp(getIdx(), "") == 0) {
        _idx->asString(_getClientName(true).c_str());
      }
      LOG->debug(F("SSID: '%s'; WebServer started: %T"), getSsid(), isWebServerRunning());
    } else {
      LOG->notice(F("Network settings are missing"));
    }
  }

  void _handleUnknown(AsyncWebServerRequest* request) {
    if (!isUpdateRunning()) {
      if (isSoftAP()) {
        //Captive portal, typically '/check_network_status.txt' or '/canonical.html'
        //To keep it easy, redirect to network config in general.
        //WC_WIFI lies in the flash, so read it byte safe
        request->redirect("http://" + WiFi.softAPIP().toString() + SLASH + FPSTR(WC_WIFI) + SLASH);
      } else {
        request->send(404);
      }
    }
  }

  void _sendDevicesStructure(AsyncWebServerRequest* request) {
    if (!isUpdateRunning()) {
      //every client gets the description of the things, a browser as well: this
      //is bound as the root only where a device needs webthings and the station
      //is connected, and a gateway expects the description there and nothing
      //else. the control panel keeps its own url, '/ui', and stays in the menu
      LOG->notice(F("Send description for all devices... "));
      AsyncResponseStream* response = request->beginResponseStream(APPLICATION_JSON);
      WJson* json = new WJson(response);
      json->beginArray();
      _devices->forEach([this, json](int index, WDevice* device, const char* id) {
        if (device->isVisible(WEBTHING)) {
          LOG->notice(F("Send description for device %s "), device->id());
          device->toJsonStructure(json, "", WEBTHING);
        }
      });
      json->endArray();
      delete json;  // ← Hier fehlte das delete!
      request->send(response);
    }
  }

  void _sendDeviceStructure(AsyncWebServerRequest* request, WDevice*& device) {
    if (!isUpdateRunning()) {
      LOG->notice(F("Send description for device: %s"), device->id());
      AsyncResponseStream* response = request->beginResponseStream(APPLICATION_JSON);
      WJson* json = new WJson(response);
      device->toJsonStructure(json, "", WEBTHING);
      delete json;  // ← Sofort freigeben
      request->send(response);
    }
  }

  void _sendDeviceValues(AsyncWebServerRequest* request, WDevice*& device) {
    if (!isUpdateRunning()) {
      LOG->notice(F("Send all properties for device: "), device->id());
      AsyncResponseStream* response = request->beginResponseStream(APPLICATION_JSON);
      WJson* json = new WJson(response);
      json->beginObject();
      if (device->isMainDevice()) {
        json->propertyString("idx", getIdx(), nullptr);
        json->propertyString("ip", getDeviceIp().toString().c_str(), nullptr);
        json->propertyString("firmware", VERSION, nullptr);
      }
      device->toJsonValues(json, WEBTHING);
      json->endObject();
      delete json;
      request->send(response);
    }
  }

  void _getPropertyValue(AsyncWebServerRequest* request, WProperty* property, const char* propertyId) {
    if (!isUpdateRunning()) {
      AsyncResponseStream* response =
          request->beginResponseStream(APPLICATION_JSON);
      WJson* json = new WJson(response);
      json->beginObject();
      property->toJsonValue(json, propertyId);
      json->endObject();
      delete json;  // ← Sofort freigeben
      property->requested(true);
      request->send(response);
    }
  }

  void _handleBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!isUpdateRunning()) {
      if ((total >= ESP_MAX_PUT_BODY_SIZE) || (index + len >= ESP_MAX_PUT_BODY_SIZE)) {
        return;  // cannot store this size..
      }
      // copy to internal buffer
      memcpy(&_body_data[index], data, len);
      _body_data[index + len] = '\0';
      _b_has_body_data = true;
    }
  }

  void _setPropertyValue(AsyncWebServerRequest* request, WDevice* device) {
    if (!isUpdateRunning()) {
      if (!_b_has_body_data) {
        request->send(422);
        return;
      }
      WList<WValue>* properties = WJsonParser::asMap(_body_data);
      //the body is used up, a request without one must not find this one again
      _b_has_body_data = false;
      //a body that is no json at all gives no list
      if (properties == nullptr) {
        request->send(422);
        return;
      }
      LOG->debug("list items count: %d", properties->size());
      if (properties->size() > 0) {
        AsyncResponseStream* response = request->beginResponseStream(APPLICATION_JSON);
        WJson* json = new WJson(response);
        json->beginObject();
        for (int i = 0; i < properties->size(); i++) {
          WValue* value = properties->get(i);
          const char* id = properties->getId(i);
          WProperty* property = device->getPropertyById(id);
          if (property != nullptr) {
            LOG->notice(F("Set property '%s' to value '%s' (web request)"), id, value->toString());
            property->parse(value->asString());
            property->toJsonValue(json, id);
          }
        }
        json->endObject();
        delete json;
        request->send(response);
      } else {
        LOG->notice(F("unable to parse json: %s"), _body_data);
        _b_has_body_data = false;
        memset(_body_data, 0, sizeof(_body_data));
        request->send(500);
      }
      delete properties;
    }
  }

  /*void _sendErrorMsg(AsyncWebServerRequest* request, int status, const char* msg) {
    if (!isUpdateRunning()) {
      AsyncResponseStream* response =
          request->beginResponseStream(APPLICATION_JSON);
      WJson* json = new WJson(response);
      json->beginObject();
      json->propertyString("error", msg, nullptr);
      json->propertyInteger("status", status);
      json->endObject();
      delete json;  // ← Sofort freigeben
      request->send(response);
    }
  }*/

  void _bindWebServerCalls(WDevice* device) {
    if (this->isWebServerRunning()) {
      LOG->notice(F("Bind webServer calls for device %s"), device->id());
      String deviceBase("/things/");
      deviceBase.concat(device->id());
      device->properties()->forEach([this, device, deviceBase](int index, WProperty* property, const char* id) {
        if (property->isVisible(WEBTHING)) {
          String propertyBase = deviceBase + "/properties/" + id;
          _webServer->on(propertyBase.c_str(), HTTP_GET, std::bind(&WNetwork::_getPropertyValue, this, std::placeholders::_1, property, id));
          _webServer->on(propertyBase.c_str(), HTTP_PUT, std::bind(&WNetwork::_setPropertyValue, this, std::placeholders::_1, device),
                         NULL, std::bind(&WNetwork::_handleBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        }
      });
      _bindPropertyPush(device, deviceBase);
      String propertiesBase = deviceBase + "/properties";
      _webServer->on(propertiesBase.c_str(), HTTP_GET, std::bind(&WNetwork::_sendDeviceValues, this, std::placeholders::_1, device));
      _webServer->on(deviceBase.c_str(), HTTP_GET, std::bind(&WNetwork::_sendDeviceStructure, this, std::placeholders::_1, device));
      device->bindWebServerCalls(_webServer);
    }
  }

  // Web Thing standard push: ws://<device>/things/<id> sends every change of a
  // visible property as {"messageType":"propertyStatus","data":{"id":value}}
  void _bindPropertyPush(WDevice* device, const String& deviceBase) {
    if (!device->needsWebThings()) return;
    AsyncWebSocket* ws = new AsyncWebSocket(deviceBase.c_str());
    _webServer->addHandler(ws);
    _webSockets.push_back(ws);
    device->properties()->forEach([this, ws](int index, WProperty* property, const char* id) {
      if (property->isVisible(WEBTHING)) {
        property->addListener([this, ws, property, id]() { _pushPropertyStatus(ws, property, id); });
      }
    });
  }

  void _pushPropertyStatus(AsyncWebSocket* ws, WProperty* property, const char* id) {
    if ((ws->count() == 0) || (isUpdateRunning())) return;
    //a status message is short, a big buffer would only take heap the pages need
    WStringStream* message = createResponseStream(SIZE_PROPERTY_STATUS);
    if (message == nullptr) return;
    WJson* json = new (std::nothrow) WJson(message);
    if (json != nullptr) {
      json->beginObject();
      json->propertyString("messageType", "propertyStatus", nullptr);
      json->beginObject("data");
      property->toJsonValue(json, id);
      json->endObject();
      json->endObject();
      delete json;
      ws->textAll(message->c_str());
    }
    delete message;
  }

  WDevice* _getDeviceById(const char* deviceId) {
    return _devices->getIf([deviceId](WDevice* d) { return (strcmp(d->id(), deviceId) == 0); });
  }
};

#endif