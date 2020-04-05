#ifndef W_NETWORK_H
#define W_NETWORK_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#ifdef ESP8266
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <StreamString.h>
#include "WHtmlPages.h"
#include "WAdapterMqtt.h"
#include "WStringStream.h"
#include "WDevice.h"
#include "WLed.h"
#include "WSettings.h"
#include "WJsonParser.h"
#include "WLog.h"

#define SIZE_MQTT_PACKET 1024
#define SIZE_JSON_PACKET 2048
#define NO_LED -1
const char* CONFIG_PASSWORD = "12345678";
const char* APPLICATION_JSON = "application/json";
const char* TEXT_HTML = "text/html";
const char* TEXT_PLAIN = "text/plain";

const char* MQTT_CMND = "cmnd";
const char* MQTT_STAT = "stat";
const char* MQTT_TELE = "tele";

const char* FORM_PW_NOCHANGE = "___NOCHANGE___";

#ifdef DEBUG
const int maxApRunTimeMinutes = 2;
const int maxConnectFail = 3;
#else
const int maxApRunTimeMinutes = 5;
const int maxConnectFail = 10;
#endif

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
WiFiClient wifiClient;
WAdapterMqtt *mqttClient;

class WNetwork {
public:
	typedef std::function<void(void)> THandlerFunction;
	WNetwork(bool debug, String applicationName, String firmwareVersion,
			int statusLedPin) {

		/* https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.rst#reconnect */
		//Serial.printf("AC: %d, ARC: %d\n", WiFi.getAutoConnect(), WiFi.getAutoReconnect());
		WiFi.disconnect();
		WiFi.mode(WiFiMode::WIFI_STA);
		WiFi.encryptionType(wl_enc_type::ENC_TYPE_CCMP);
		WiFi.setOutputPower(20.5);
		WiFi.setPhyMode(WiFiPhyMode::WIFI_PHY_MODE_11N);
		WiFi.setAutoConnect(false);
		WiFi.setAutoReconnect(true);
		WiFi.persistent(false);
		this->applicationName = applicationName;
		this->firmwareVersion = firmwareVersion;
		this->webServer = nullptr;
		this->dnsApServer = nullptr;
		this->debug = debug;
		this->statusLedPin = statusLedPin;
		wlog = new WLog((this->debug ? LOG_LEVEL_TRACE : LOG_LEVEL_SILENT), LOG_LEVEL_SILENT, &Serial);
		wlog->setOnLogCommand([this](int level, const char * message) {
			if (this->networkLogActive) return;
			this->networkLogActive=true; /* avoid endless loop */
			WDevice *device = this->firstDevice;
			while (device != nullptr) {
				device->sendLog(level, message);
				device = device->next;
			}
			this->networkLogActive=false;
		});

		this->updateRunning = false;
		this->restartFlag = "";
		this->networkLogActive = false;
		this->connectFailCounter = 0;
		this->apStartedAt = 0;
		this->lastLoopLog = 0;
		this->mqttClient = nullptr;
		//this->webSocket = nullptr;
		settings = new WSettings(wlog);
		settingsFound = loadSettings();
		lastMqttConnect = lastWifiConnect = 0;
		gotIpEventHandler = WiFi.onStationModeGotIP(
				[this](const WiFiEventStationModeGotIP &event) {
					wlog->notice(F("WiFi: Station connected, IP: %s"), this->getDeviceIp().toString().c_str());
					this->connectFailCounter=0;
					//Connect, if webThing supported and Wifi is connected as client

					this->notify(false);
				});
		disconnectedEventHandler = WiFi.onStationModeDisconnected(
				[this](const WiFiEventStationModeDisconnected &event) {
					this->connectFailCounter++;
					wlog->notice(F("WiFi: Station disconnected (count: %d)"), this->connectFailCounter);
					this->disconnectMqtt();
					this->lastMqttConnect = 0;
					this->notify(false);
				});




		
		wlog->notice(F("firmware: %s"), firmwareVersion.c_str());

	}


	//returns true, if no configuration mode and no own ap is opened
	bool loop(unsigned long now) {

#ifdef DEBUG
		if (this->lastLoopLog == 0 || now  > this->lastLoopLog + 2000){
			if (WiFi.status() != WL_CONNECTED) wlog->trace(F("WiFi: loop, status: %d, isSoftAP: %d, now: %d"), WiFi.status(), isSoftAP(), now);
			this->lastLoopLog=now;
		} 
#endif
		bool result = true;
		if ((!settingsFound)  || this->connectFailCounter >= maxConnectFail) {
			if (isSoftAP() ){
				// Switch back to Station Mode if config is available
				if (settingsFound && (now -  this->apStartedAt > maxApRunTimeMinutes * 60 * 1000)){
					wlog->warning(F("WiFi: AP ran %d minutes, trying now to use configured WLAN again, Wifi Status %d "), maxApRunTimeMinutes, WiFi.status());
					//this->stopWebServer();
					this->connectFailCounter=0;
					WiFi.softAPdisconnect();
					lastWifiConnect=0;
					apStartedAt=0;
					deleteDnsApServer();
				}
			} else {
				wlog->trace(F("Starting AP Mode"));
				if (WiFi.status() != WL_CONNECTED) {
					// even if currently disconnected - stop trying to connect
					//WiFi.disconnect();
					//Create own AP
					this->apStartedAt=millis();
					String apSsid = getClientName(false);
					wlog->notice(F("Start AccessPoint for configuration. SSID '%s'; password '%s'"), apSsid.c_str(), CONFIG_PASSWORD);
					dnsApServer = new DNSServer();
					WiFi.mode(WIFI_AP);
					WiFi.softAP(apSsid.c_str(), CONFIG_PASSWORD);
					dnsApServer->setErrorReplyCode(DNSReplyCode::NoError);
					dnsApServer->start(53, "*", WiFi.softAPIP());


				} else {
					wlog->notice(F("Start web server for configuration. IP %s"), this->getDeviceIp().toString().c_str());
				}
			}		
		}


		if (!isSoftAP() && (WiFi.status() != WL_CONNECTED)	&& getSsid() != "" && ((lastWifiConnect == 0) || (now - lastWifiConnect > 20 * 1000))) {
			wlog->notice(F("WiFi: Connecting to '%s', using Hostname '%s'"), getSsid(), getHostName().c_str());
			wlog->notice(F("WiFi: SSID/PSK/Hostname '%s'/'%s'/'%s' (strlen %d/%d/%d)"), getSsid(), getPassword(), getHostName().c_str(),
				strlen(getSsid()), strlen(getPassword()), strlen(getHostName().c_str()));
			//Workaround: if disconnect is not called, WIFI connection fails after first startup
			//WiFi.disconnect();
			WiFi.mode(WIFI_STA);
			WiFi.hostname(getHostName());
			WiFi.begin(getSsid(), getPassword());
			apStartedAt=0;
			wlog->notice(F("Wait"));
			WiFi.waitForConnectResult();
			wlog->notice(F("Wait DONE"));
			lastWifiConnect = now;
		}


		if (isWebServerRunning()){
			if (isSoftAP()) {
				dnsApServer->processNextRequest();
			}
			webServer->handleClient();
			/*if (webSocket != nullptr) {
				webSocket->loop();
			}*/
			result = ((!isSoftAP()) && (!isUpdateRunning()));
		}
		if (!isSoftAP()){
			//MQTT connection
			if ((isWifiConnected()) && (isSupportingMqtt())
					&& (!mqttClient->connected())
					&& ((lastMqttConnect == 0) || (now - lastMqttConnect > 300000))
					&& (strcmp(getMqttServer(), "") != 0)
					&& (strcmp(getMqttPort(), "") != 0)) {
				mqttReconnect();
				lastMqttConnect = now;
			}
			if ((!isUpdateRunning()) && (this->isMqttConnected())) {
				mqttClient->loop();
			}
		}

		//Loop led
		if (statusLed != nullptr) {
			statusLed->loop(now);
		}
		//Loop Devices
		WDevice *device = firstDevice;
		while (device != nullptr) {
			device->loop(now);
			if ((this->isMqttConnected()) && (this->isSupportingMqtt())
					&& ((device->lastStateNotify == 0)
							|| ((device->stateNotifyInterval > 0) && (now > device->lastStateNotify) &&
							    (now - device->lastStateNotify > device->stateNotifyInterval)))
					&& (device->isDeviceStateComplete())) {
				wlog->notice(F("Notify interval is up -> Device state changed..."));
				handleDeviceStateChange(device);
			}
			device = device->next;
		}
		//WebThingAdapter
		if ((!isUpdateRunning()) && (this->isSupportingWebThing()) && (isWifiConnected())) {
			MDNS.update();
		}
		//Restart required?
		if (!restartFlag.equals("")) {
			wlog->notice(F("Restart flag: '%s'"), restartFlag.c_str());
			this->updateRunning = false;
			stopWebServer();
			delay(1000);
			ESP.restart();
			delay(2000);
		}
		return result;
	}

	~WNetwork() {
		delete wlog;
	}

	WSettings* getSettings() {
		return this->settings;
	}

	void setOnNotify(THandlerFunction onNotify) {
		this->onNotify = onNotify;
	}

	void setOnConfigurationFinished(THandlerFunction onConfigurationFinished) {
		this->onConfigurationFinished = onConfigurationFinished;
	}

	bool publishMqtt(const char* topic, const char * message, bool retained=false) {
		wlog->verbose(F("MQTT... '%s'"), topic);
		wlog->verbose(F("MQTT  .. '%s'"), message);
		if (isMqttConnected()) {
			wlog->verbose(F("MQTT connected... "));
			if (mqttClient->publish(topic, message, retained)) {
				wlog->verbose(F("MQTT sent. Topic: '%s'"), topic);
				return true;
			} else {
				wlog->verbose(F("Sending MQTT message failed, rc=%d"), mqttClient->state());
				this->disconnectMqtt();
				return false;
			}
		} else {
			wlog->notice(F("MQTT not connected... "));
			if (strcmp(getMqttServer(), "") != 0) {
				wlog->verbose(F("Can't send MQTT. Not connected to server: %s"), getMqttServer());
			}
			return false;
		}
		wlog->warning(F("publish MQTT mystery... "));
	}

	bool publishMqtt(const char* topic, WStringStream* response, bool retained=false) {
		return publishMqtt(topic, response->c_str(), retained);
	}

	bool publishMqtt(const char* topic, const char* key, const char* value, const char* key2=nullptr, const char* value2=nullptr, bool retained=false) {
		if (this->isSupportingMqtt()) {
			WStringStream* response = getResponseStream();
			WJson json(response);
			json.beginObject();
			json.propertyString(key, value);
			if (key2!=nullptr && value2!=nullptr){
				json.propertyString(key2, value2);
			}
			json.endObject();
			return publishMqtt(topic, response, retained);
		} else {
			return false;
		}
	}

	// Creates a web server
	void startWebServer() {
		wlog->trace(F("Starting Webserver"));

		if (this->isSupportingMqtt()) {
			this->mqttClient = new WAdapterMqtt(debug, wifiClient, SIZE_JSON_PACKET);
			mqttClient->setCallback(std::bind(&WNetwork::mqttCallback, this,
										std::placeholders::_1, std::placeholders::_2,
										std::placeholders::_3));
		}
		
		if (this->statusLedPin != NO_LED) {
			statusLed = new WLed(statusLedPin);
			statusLed->setOn(true, 500);
		} else {
			statusLed = nullptr;
		}

		this->webServer = new ESP8266WebServer(80);

		webServer->onNotFound(std::bind(&WNetwork::handleUnknown, this));
		//if ((WiFi.status() != WL_CONNECTED) || (!this->isSupportingWebThing())) {
		webServer->on("/", HTTP_GET, std::bind(&WNetwork::handleHttpRootRequest, this));
		webServer->on("/config", HTTP_GET, std::bind(&WNetwork::handleHttpRootRequest, this));
		webServer->on("/generate_204", HTTP_GET, std::bind(&WNetwork::handleHttpRootRequest, this));
		//webServer->on("/generate_204", HTTP_GET, std::bind(&WNetwork::handleCaptivePortal, this));

		WDevice *device = this->firstDevice;
		while (device != nullptr) {
			String did("/");
			did.concat(device->getId());
			String deviceConfiguration("/saveConfiguration");
			deviceConfiguration.concat(device->getId());
			if (device->isProvidingConfigPage()) {
				webServer->on(did, HTTP_GET, std::bind(&WNetwork::handleHttpDeviceConfiguration, this, device));
				webServer->on(deviceConfiguration.c_str(), HTTP_GET, std::bind(&WNetwork::handleHttpSaveDeviceConfiguration, this, device));
			}
			WPage *subpage = device->firstPage;
			while (subpage != nullptr) {
				did.concat("_");
				did.concat(subpage->getId());
				deviceConfiguration.concat("_");
				deviceConfiguration.concat(subpage->getId());
				webServer->on(did, HTTP_GET, std::bind(&WNetwork::handleHttpDevicePage, this, device, subpage));
				webServer->on(deviceConfiguration.c_str(), HTTP_GET, std::bind(&WNetwork::handleHttpDevicePageSubmitted, this, device, subpage));
				subpage = subpage->next;
			}
			device = device->next;
		}
		webServer->on("/wifi", HTTP_GET,
				std::bind(&WNetwork::handleHttpNetworkConfiguration, this));
		webServer->on("/saveConfiguration", HTTP_GET,
				std::bind(&WNetwork::handleHttpSaveConfiguration, this));
		webServer->on("/info", HTTP_GET,
				std::bind(&WNetwork::handleHttpInfo, this));
		webServer->on("/reset", HTTP_ANY,
				std::bind(&WNetwork::handleHttpReset, this));

		//firmware update
		webServer->on("/firmware", HTTP_GET,
				std::bind(&WNetwork::handleHttpFirmwareUpdate, this));
		webServer->on("/firmware", HTTP_POST,
				std::bind(&WNetwork::handleHttpFirmwareUpdateFinished, this),
				std::bind(&WNetwork::handleHttpFirmwareUpdateProgress, this));

		//WebThings
		if ((this->isSupportingWebThing()) && (this->isWifiConnected())) {
			//Make the thing discoverable
			//String mdnsName = getHostName() + ".local";
			String mdnsName = this->getDeviceIp().toString();
			if (MDNS.begin(mdnsName)) {
				MDNS.addService("http", "tcp", 80);
				MDNS.addServiceTxt("http", "tcp", "url", "http://" + mdnsName + "/things");
				MDNS.addServiceTxt("http", "tcp", "webthing", "true");
				wlog->notice(F("MDNS responder started at %s"), mdnsName.c_str());
			}
			webServer->on("/things", HTTP_GET, std::bind(&WNetwork::sendDevicesStructure, this));
			webServer->on("/things/", HTTP_GET, std::bind(&WNetwork::sendDevicesStructure, this));
			WDevice *device = this->firstDevice;
			while (device != nullptr) {
				bindWebServerCallsNetwork(device);
				device = device->next;
			}
		}


		//Start http server
		wlog->notice(F("webServer prepared."));

		webServer->begin();
		this->notify(true);
		return;
	}

	void stopWebServer() {
		if (this->updateRunning) return;
		if ((isWebServerRunning())) {
			wlog->notice(F("stopWebServer"));
			delay(100);
			webServer->stop();
			this->notify(true);
		}
		deleteDnsApServer();
		disconnectMqtt();
		wlog->notice(F("kill"));
		if (webServer!=nullptr){
			delete webServer;
			webServer = nullptr;
		}
		if (onConfigurationFinished) {
			onConfigurationFinished();
		}
	}

	bool isWebServerRunning() {
		return (webServer != nullptr);
	}

	bool isUpdateRunning() {
		return this->updateRunning;
	}

	bool isDebug() {
		return this->debug;
	}

	bool isSoftAP() {
		return ((isWebServerRunning()) && (dnsApServer != nullptr));
	}

	bool isWifiConnected() {
		return ((!isSoftAP()) && (!isUpdateRunning())
				&& (WiFi.status() == WL_CONNECTED));
	}

	bool isMqttConnected() {
		return ((this->isSupportingMqtt()) && (this->mqttClient != nullptr)
				&& (this->mqttClient->connected()));
	}

	void disconnectMqtt() {
		if (this->mqttClient != nullptr) {
			this->mqttClient->disconnect();
		}
	}
	void deleteDnsApServer(){
		if (this->dnsApServer){
			this->dnsApServer->stop();
			delete dnsApServer;
			this->dnsApServer = nullptr;
		}
	}

	IPAddress getDeviceIp() {
		return (isSoftAP() ? WiFi.softAPIP() : WiFi.localIP());
	}

	bool isSupportingWebThing() {
		return true;
	}

	bool isSupportingMqtt() {
		return this->supportingMqtt->getBoolean();
	}

	const char* getIdx() {
		return this->idx->c_str();
	}

	const char* getSsid() {
		return this->ssid->c_str();
	}

	const char* getPassword() {
		return settings->getString("password");
	}

	const char* getMqttServer() {
		return settings->getString("mqttServer");
	}

	const char* getMqttPort() {
		return settings->getString("mqttPort");
	}

	const char* getMqttTopic() {
		return this->mqttTopic->c_str();
	}

	const char* getMqttUser() {
		return settings->getString("mqttUser");
	}

	const char* getMqttPassword() {
		return settings->getString("mqttPassword");
	}

	void addDevice(WDevice *device) {
		if (statusLed == nullptr) {
			statusLed = device->getStatusLed();
			if (statusLed != nullptr) {
				statusLed->setOn(true, 500);
			}
		}
		if (this->lastDevice == nullptr) {
			this->firstDevice = device;
			this->lastDevice = device;
		} else {
			this->lastDevice->next = device;
			this->lastDevice = device;
		}

		/*ToDo
		AsyncWebSocket *webSocket = new AsyncWebSocket("/things/" + device->getId());
		device->setWebSocket(webSocket);
		*/
		// started at startWebserver()
		//bindWebServerCalls(device);
	}

	WLog* log() {
		return wlog;
	}

	WStringStream* getResponseStream() {
		if (responseStream == nullptr) {
			responseStream = new WStringStream(SIZE_JSON_PACKET);
		}
		responseStream->flush();
		return responseStream;
	}

private:
	ESP8266WebServer *webServer;
	WDevice *firstDevice = nullptr;
	WDevice *lastDevice = nullptr;
	WLog* wlog;
	THandlerFunction onNotify;
	THandlerFunction onConfigurationFinished;
	bool debug, updateRunning;
	String restartFlag;
	DNSServer *dnsApServer;
	int networkState;
	String applicationName;
	String firmwareVersion;
	const char* firmwareUpdateError;
	WProperty *supportingMqtt;
	WProperty *ssid;
	WProperty *idx;
	WProperty *mqttTopic;
	WAdapterMqtt *mqttClient;
	long lastMqttConnect, lastWifiConnect;
	WStringStream* responseStream = nullptr;
	WLed *statusLed;
	int statusLedPin;
	WSettings *settings;
	bool settingsFound;
	bool networkLogActive;
	int connectFailCounter;
	unsigned long apStartedAt;
	unsigned long lastLoopLog;

	void handleDeviceStateChange(WDevice *device) {
		String topic = String(getMqttTopic()) + "/" + MQTT_STAT + "/things/" + String(device->getId()) + "/properties";
		wlog->notice(F("Device state changed -> send device state... %s"), topic.c_str());
		mqttSendDeviceState(topic, device);
	}

	void mqttSendDeviceState(String topic, WDevice *device) {		
		if ((this->isMqttConnected()) && (isSupportingMqtt())){
			if (device->isDeviceStateComplete()) {
				wlog->notice(F("Send actual device state via MQTT %s"), topic.c_str());
				WStringStream* response = getResponseStream();
				WJson json(response);
				json.beginObject();
				if (device->isMainDevice()) {
					json.propertyString("idx", getIdx());
					json.propertyString("ip", getDeviceIp().toString().c_str());
					json.propertyString("firmware", firmwareVersion.c_str());
				}
				device->toJsonValues(&json, MQTT);
				json.endObject();
								
				if (mqttClient->publish(topic.c_str(), response->c_str(), device->mqttRetain)) {
					wlog->verbose(F("MQTT sent"));
				}
				device->lastStateNotify = millis();
			} else {
				wlog->warning(F("Not sending state via MQTT %s, deviceStateComplete=false"), topic.c_str());
			}
		}
	}

	void mqttCallback(char *ptopic, char *payload, unsigned int length) {
		String ptopicS=String(ptopic);
		String payloadS=String(payload);
		wlog->trace(F("Received MQTT callback: '%s'->'%s'"), ptopic, payload, strlen(ptopic), ptopicS.length());
		if (!ptopicS.startsWith(getMqttTopic())){
			wlog->notice(F("Ignoring, starts not with our topic '%s'"), getMqttTopic());
			return;
		}
		String fulltopic = ptopicS.substring(strlen(getMqttTopic()) + 1);
		String cmd = String(MQTT_CMND);
		if (fulltopic.startsWith(cmd)) {			
			String topic = String(fulltopic).substring(strlen(cmd.c_str()) + 1);
			if (topic.startsWith("things/")) {
				topic = topic.substring(String("things/").length());
				int i = topic.indexOf("/");
				if (i > -1) {
					String deviceId = topic.substring(0, i);					
					wlog->trace(F("look for device id '%s'"), deviceId.c_str());
					WDevice *device = this->getDeviceById(deviceId.c_str());
					if (device != nullptr) {
						String stat_topic = getMqttTopic() + (String)"/" + String(MQTT_STAT) + (String)"/things/" + String(deviceId) + (String)"/";
						topic = topic.substring(i + 1);	
						if (topic.startsWith("properties")) {							
							topic = topic.substring(String("properties").length() + 1);
							if (topic.equals("")) {
								if (length > 0) {
									//Check, if it's only response to a state before
									wlog->notice(F("Set several properties for device %s"), device->getId());
									WJsonParser* parser = new WJsonParser();
									if (parser->parse(payload, device) == nullptr) {
										wlog->warning(F("No properties updated for device %s"), device->getId());
									} else {
										wlog->trace(F("One or more properties updated for device %s"), device->getId());
									}
									delete parser;
								} else {
									wlog->notice(F("Empty payload for topic 'properties' -> send device state..."));
									//Empty payload for topic 'properties' ->  just send state (below)
								}


							} else {
								//There are still some more topics after properties
								//Try to find property with that topic and set single value
								WProperty* property = device->getPropertyById(topic.c_str());
								if ((property != nullptr) && (property->isVisible(MQTT))) {
									//Set Property
									wlog->notice(F("Set property '%s' for device %s"), property->getId(), device->getId(), payloadS.c_str());
									if (!property->parse(payloadS)) {
										wlog->warning(F("Property not updated."));
									} else {
										wlog->trace(F("Property updated."));
									}
								}
								// answer just with changed value
								//publishMqtt((stat_topic+topic).c_str(), property->c_str(), false);
							}			
							wlog->notice(F("Sending device State to %sproperties for device %s"), stat_topic.c_str(), device->getName());				
							mqttSendDeviceState(stat_topic+"properties", device);
						} else {
							//unknown, ask the device
							device->handleUnknownMqttCallback(stat_topic, topic, payloadS, length);
						}
					}
				}
			}
		}
	}

	bool mqttReconnect() {
		if (this->isSupportingMqtt()) {
			wlog->notice(F("Connect to MQTT server: %s; user: '%s'; password: '%s'; clientName: '%s'"),
					   getMqttServer(), getMqttUser(), getMqttPassword(), getClientName(true).c_str());
			// Attempt to connect
			this->mqttClient->setServer(getMqttServer(), String(getMqttPort()).toInt());
			if (mqttClient->connect(getClientName(true).c_str(),
					getMqttUser(), //(mqttUser != "" ? mqttUser.c_str() : NULL),
					getMqttPassword())) { //(mqttPassword != "" ? mqttPassword.c_str() : NULL))) {
				wlog->notice(F("Connected to MQTT server."));
				//Send device structure and status
				mqttClient->subscribe("devices/#");

				WDevice *device = this->firstDevice;
				while (device != nullptr) {
					String topic("devices/");
					topic.concat(device->getId());
					WStringStream* response = getResponseStream();
					WJson json(response);
					json.beginObject();
					json.propertyString("url", "http://", getDeviceIp().toString().c_str(), "/things/", device->getId());
					json.propertyString("ip", getDeviceIp().toString().c_str());
					json.propertyString("topic", ((String)getMqttTopic()+"/"+MQTT_STAT+"/things/"+device->getId()).c_str());
					json.endObject();
					mqttClient->publish(topic.c_str(), response->c_str());
					device = device->next;
				}
				mqttClient->unsubscribe("devices/#");
				//Subscribe to device specific topic
				String subscribeTopic=String(String(getMqttTopic()) + "/" + String(MQTT_CMND) + "/#");
				wlog->notice(F("Subscribing to Topic %s"),subscribeTopic.c_str());
				mqttClient->subscribe(subscribeTopic.c_str());
				notify(false);
				return true;
			} else {
				wlog->notice(F("Connection to MQTT server failed, rc=%d"), mqttClient->state());
				notify(false);
				return false;
			}
		}
	}

	void notify(bool sendState) {
		if (statusLed != nullptr) {
			if (isWifiConnected()) {
				//statusLed->setOn(false);
			} else if (isSoftAP()) {
				//statusLed->setOn(true, 0);
			} else {
				//statusLed->setOn(true, 500);
			}
		}
		if (sendState) {
			WDevice *device = this->firstDevice;
			while (device != nullptr) {
				handleDeviceStateChange(device);
				device = device->next;
			}
		}
		if (onNotify) {
			onNotify();
		}
	}

	void handleHttpRootRequest() {
		wlog->notice(F("handleHttpRootRequest"));
		if (isWebServerRunning()) {
			if (restartFlag.equals("")) {
				WStringStream* page = new WStringStream(2048);
				page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), applicationName.c_str());
				page->print(FPSTR(HTTP_SCRIPT));
				page->print(FPSTR(HTTP_STYLE));
				page->print(FPSTR(HTTP_HEAD_END));
				printHttpCaption(page);
				WDevice *device = firstDevice;
				page->printAndReplace(FPSTR(HTTP_BUTTON), "wifi", "get", "Configure network");
				while (device != nullptr) {
					if (device->isProvidingConfigPage()) {
						String s("Configure ");
						s.concat(device->getName());
						page->printAndReplace(FPSTR(HTTP_BUTTON), device->getId(), "get", s.c_str());
						//page->printAndReplace(FPSTR(HTTP_BUTTON_DEVICE), device->getId(), device->getName());
					}
					WPage *subpage = device->firstPage;
					while (subpage != nullptr) {
						String url =(String)device->getId()+"_"+(String)subpage->getId();
						page->printAndReplace(FPSTR(HTTP_BUTTON), url.c_str() , "get", subpage->getTitle());
						subpage = subpage->next;
					}
					device = device->next;
				}
				page->printAndReplace(FPSTR(HTTP_BUTTON), "firmware", "get", "Update firmware");
				page->printAndReplace(FPSTR(HTTP_BUTTON), "info", "get", "Info");
				page->printAndReplace(FPSTR(HTTP_BUTTON), "reset", "post", "Reboot");
				page->print(FPSTR(HTTP_BODY_END));
				webServer->send(200, TEXT_HTML, page->c_str());
				delete page;
			} else {
				WStringStream* page = new WStringStream(2048);
				page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Info");
				page->print(FPSTR(HTTP_SCRIPT));
				page->print(FPSTR(HTTP_STYLE));
				page->print("<meta http-equiv=\"refresh\" content=\"10\">");
				page->print(FPSTR(HTTP_HEAD_END));
				page->print(restartFlag);
				page->print("<br><br>");
				page->print("Module will reset in a few seconds...");
				page->print(FPSTR(HTTP_BODY_END));
				webServer->send(200, TEXT_HTML, page->c_str());
				delete page;
			}
		}
	}

	void handleHttpDeviceConfiguration(WDevice *&device) {
		if (isWebServerRunning()) {
			wlog->notice(F("Device config page"));
			WStringStream* page = new WStringStream(8*1024);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Device Configuration");
			page->print(FPSTR(HTTP_SCRIPT));
			page->print(FPSTR(HTTP_STYLE));
			page->print(FPSTR(HTTP_HEAD_END));
			printHttpCaption(page);
			device->printConfigPage(page);
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			delete page;
		}

	}
	void handleHttpDevicePage(WDevice *&device, WPage *&subpage) {
		if (isWebServerRunning()) {
			wlog->notice(F("Device subpage"));
			WStringStream* page = new WStringStream(8*1024);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), subpage->getTitle());
			page->print(FPSTR(HTTP_SCRIPT));
			page->print(FPSTR(HTTP_STYLE));
			page->print(FPSTR(HTTP_HEAD_END));
			printHttpCaption(page);
			subpage->printPage(webServer, page);
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			delete page;
		}
	}

	void handleHttpDevicePageSubmitted(WDevice *&device, WPage *&subpage) {
		if (isWebServerRunning()) {
			wlog->notice(F("handleHttpDevicePageSubmitted "), device->getId());
			webServer->client().setNoDelay(true);
			WStringStream* page = new WStringStream(2048);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), subpage->getTitle());
			page->print(FPSTR(HTTP_SCRIPT));
			page->print(FPSTR(HTTP_STYLE));
			page->print(FPSTR(HTTP_HEAD_END));
			printHttpCaption(page);
			subpage->submittedPage(webServer, page);

			page->print(FPSTR(HTTP_HOME_BUTTON));
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			settings->save();
			delete page;
			wlog->notice(F("handleHttpDevicePageSubmitted Done"));
		}
	}

	void handleHttpNetworkConfiguration() {
		if (isWebServerRunning()) {
			wlog->notice(F("Network config page"));
			WStringStream* page = new WStringStream(3072);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Network Configuration");
			page->print(FPSTR(HTTP_SCRIPT));
			page->print(FPSTR(HTTP_STYLE));
			page->print(FPSTR(HTTP_HEAD_END));
			printHttpCaption(page);
			page->printAndReplace(FPSTR(HTTP_CONFIG_PAGE_BEGIN), "");
			page->printAndReplace(FPSTR(HTTP_PAGE_CONFIGURATION_STYLE), (this->isSupportingMqtt() ? "block" : "none"));
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "Hostname/Idx:", "i", "32", getIdx());
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "Wifi ssid (only 2.4G):", "s", "32", getSsid());
			page->printAndReplace(FPSTR(HTTP_PASSWORD_FIELD), "Wifi password:", "p", "64", FORM_PW_NOCHANGE);
			//mqtt
			page->printAndReplace(FPSTR(HTTP_PAGE_CONFIGURATION_MQTT_OPTION), (this->isSupportingMqtt() ? "checked" : ""));
			page->print(FPSTR(HTTP_PAGE_CONFIGURATION_MQTT_BEGIN));
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "MQTT Server:", "ms", "32", getMqttServer());
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "MQTT Port:", "mo", "4", getMqttPort());
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "MQTT User:", "mu", "32", getMqttUser());
			page->printAndReplace(FPSTR(HTTP_PASSWORD_FIELD), "MQTT Password:", "mp", "64", FORM_PW_NOCHANGE);
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "Topic, e.g.'home/room':", "mt", "64", getMqttTopic());

			page->print(FPSTR(HTTP_PAGE_CONFIGURATION_MQTT_END));
			page->print(FPSTR(HTTP_CONFIG_SAVE_BUTTON));
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			delete page;
		}
	}

	void handleHttpSaveConfiguration() {
		if (isWebServerRunning()) {
			this->idx->setString(webServer->arg("i").c_str());
			this->ssid->setString(webServer->arg("s").c_str());
			settings->setString("password",  (webServer->arg("p").equals(FORM_PW_NOCHANGE) ? getPassword() : webServer->arg("p").c_str())) ;
			this->supportingMqtt->setBoolean(webServer->arg("mq") == "true");
			settings->setString("mqttServer", webServer->arg("ms").c_str());
			String mqtt_port = webServer->arg("mo");
			settings->setString("mqttPort", (mqtt_port != "" ? mqtt_port.c_str() : "1883"));
			settings->setString("mqttUser", webServer->arg("mu").c_str());
			settings->setString("mqttPassword",(webServer->arg("mp").equals(FORM_PW_NOCHANGE) ? getMqttPassword() : webServer->arg("mp").c_str()));
			this->mqttTopic->setString(webServer->arg("mt").c_str());
			settings->save();
			this->restart("Settings saved.");
		}
	}

	void handleHttpSaveDeviceConfiguration(WDevice *&device) {
		if (isWebServerRunning()) {
			wlog->notice(F("handleHttpSaveDeviceConfiguration "), device->getId());
			device->saveConfigPage(webServer);
			settings->save();
			if (device->isConfigNeedsReboot()){
				wlog->notice(F("Reboot "));
				delay(300);
				this->restart("Device settings saved.");
			} else {
				this->webReturnStatusPage("Device settings saved.", "");
			}
			wlog->notice(F("handleHttpSaveDeviceConfiguration Done"));
		}
	}

	void handleHttpInfo() {
		if (isWebServerRunning()) {
			WStringStream* page = new WStringStream(2048);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Info");
			page->print(FPSTR(HTTP_SCRIPT));
			page->print(FPSTR(HTTP_STYLE));
			page->print(FPSTR(HTTP_HEAD_END));
			printHttpCaption(page);
			page->print("<table>");
			page->print("<tr><th>Chip ID:</th><td>");
			page->print(ESP.getChipId());
			page->print("</td></tr>");
			page->print("<tr><th>Flash Chip ID:</th><td>");
			page->print(ESP.getFlashChipId());
			page->print("</td></tr>");
			page->print("<tr><th>IDE Flash Size:</th><td>");
			page->print(ESP.getFlashChipSize());
			page->print("</td></tr>");
			page->print("<tr><th>Real Flash Size:</th><td>");
			page->print(ESP.getFlashChipRealSize());
			page->print("</td></tr>");
			page->print("<tr><th>IP address:</th><td>");
			page->print(this->getDeviceIp().toString());
			page->print("</td></tr>");
			page->print("<tr><th>MAC address:</th><td>");
			page->print(WiFi.macAddress());
			page->print("</td></tr>");
			page->print("<tr><th>Current sketch size:</th><td>");
			page->print(ESP.getSketchSize());
			page->print("</td></tr>");
			page->print("<tr><th>Available sketch size:</th><td>");
			page->print(ESP.getFreeSketchSpace());
			page->print("</td></tr>");
			page->print("<tr><th>Free heap size:</th><td>");
			page->print(ESP.getFreeHeap());
			page->print("</td></tr>");
			page->print("<tr><th>Largest free heap block:</th><td>");
			page->print(ESP.getMaxFreeBlockSize());
			page->print("</td></tr>");
			page->print("<tr><th>Heap fragmentation:</th><td>");
			page->print(ESP.getHeapFragmentation());
			page->print(" %</td></tr>");
			page->print("</td></tr>");
			page->print("<tr><th>Uptime:</th><td>");
			unsigned long secs=millis()/1000;
			unsigned int days = secs / (60 * 60 * 24);
			secs -= days * (60 * 60 * 24);
			unsigned int hours = secs / (60 * 60);
			secs -= hours * (60 * 60);
			unsigned int minutes = secs / 60;
			page->printf_P("%dd, %dh, %dm, %ds",
			days, hours, minutes, secs);
			page->print("</td></tr>");
			page->print("</table>");
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			delete page;
		}
	}

	/** Handle the reset page */
	void handleHttpReset() {
		if (isWebServerRunning()) {
			this->restart("Resetting was caused manually by web interface. ");
		}
	}

	void printHttpCaption(WStringStream* page) {
		page->print("<h2>");
		page->print(applicationName);
		page->print(" ");
		page->print(firmwareVersion);
		page->print(debug ? " (debug)" : "");
		page->print("</h2>");
		if (getIdx()){
			page->print("<h3>");
			page->print(getIdx());
			page->print("</h3>");
		}

	}

	String getClientName(bool lowerCase) {
		String result = (applicationName.equals("") ? "ESP" : String(applicationName));
		result.replace(" ", "-");
		if (lowerCase) {
			result.replace("-", "");
			result.toLowerCase();
		}
		//result += "_";
		String chipId = String(ESP.getChipId());
		int resLength = result.length() + chipId.length() + 1 - 32;
		if (resLength > 0) {
			result.substring(0, 32 - resLength);
		}
		return result + "_" + chipId;
	}

	String getHostName() {
		String hostName = getIdx();
		hostName.replace(".", "-");
		hostName.replace(" ", "-");
		if (hostName.equals("")) {
			hostName = getClientName(false);
		}
		return hostName;
	}

	void handleHttpFirmwareUpdate() {
		if (isWebServerRunning()) {
			WStringStream* page = new WStringStream(2048);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Firmware update");
			page->print(FPSTR(HTTP_SCRIPT));
			page->print(FPSTR(HTTP_STYLE));
			page->print(FPSTR(HTTP_HEAD_END));
			printHttpCaption(page);
			page->print(FPSTR(HTTP_FORM_FIRMWARE));
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			delete page;
		}
	}

	void handleHttpFirmwareUpdateFinished() {
		if (isWebServerRunning()) {
			if (Update.hasError()) {
				this->restart(firmwareUpdateError);
			} else {
				this->restart("Update successful.");
			}
		}
	}

	void handleHttpFirmwareUpdateProgress() {
		if (isWebServerRunning()) {

			HTTPUpload& upload = webServer->upload();
			//Start firmwareUpdate
			this->updateRunning = true;
			//Close existing MQTT connections
			this->disconnectMqtt();

			if (upload.status == UPLOAD_FILE_START){
				firmwareUpdateError = "";
				unsigned long free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
				wlog->notice(F("Update starting: %s"), upload.filename.c_str());
				//Update.runAsync(true);
				if (!Update.begin(free_space)) {
					setFirmwareUpdateError("Can't start update (" + String(free_space) + "): ");
				}
			} else if (upload.status == UPLOAD_FILE_WRITE) {
			    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
			    	setFirmwareUpdateError("Can't upload file: ");
			    }
			} else if (upload.status == UPLOAD_FILE_END) {
			    if (Update.end(true)) { //true to set the size to the current progress
			    	wlog->notice(F("Update complete: "));
			    } else {
			    	setFirmwareUpdateError("Can't finish update: ");
			    }
			}
		}
	}

	const char* getFirmwareUpdateErrorMessage() {
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
		default:
			return "UNKNOWN";
		}
	}

	void setFirmwareUpdateError(String msg) {
		firmwareUpdateError = getFirmwareUpdateErrorMessage();
		String s = msg + firmwareUpdateError;
		wlog->notice(s.c_str());
	}

	void webReturnStatusPage(const char* reasonMessage1, const char* reasonMessage2) {
		webServer->client().setNoDelay(true);
		WStringStream* page = new WStringStream(2048);
		page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), reasonMessage1);
		page->print(FPSTR(HTTP_SCRIPT));
		page->print(FPSTR(HTTP_STYLE));
		page->print(FPSTR(HTTP_HEAD_END));
		printHttpCaption(page);
		page->printAndReplace(FPSTR(HTTP_SAVED), reasonMessage1, reasonMessage2);
		page->print(FPSTR(HTTP_HOME_BUTTON));
		page->print(FPSTR(HTTP_BODY_END));
		webServer->send(200, TEXT_HTML, page->c_str());
		delete page;
	}
	
	void restart(const char* reasonMessage) {
		this->restartFlag = reasonMessage;
		webReturnStatusPage(reasonMessage, "ESP reboots now...");
	}

	bool loadSettings() {
		this->idx = settings->setString("idx", 32, this->getClientName(true).c_str());
		this->ssid = settings->setString("ssid", 32, "");
		settings->setString("password", 64, "");
		this->supportingMqtt = settings->setBoolean("supportingMqtt", false);
		settings->setString("mqttServer", 32, "");
		settings->setString("mqttPort", 4, "1883");
		settings->setString("mqttUser", 32, "");
		settings->setString("mqttPassword", 64, "");
		this->mqttTopic = settings->setString("mqttTopic", 64, getIdx());
		bool settingsStored = settings->existsSettings();
		if (settingsStored) {
			if (getMqttTopic() == "") {
				this->mqttTopic->setString(this->getClientName(true).c_str());
			}
			if ((isSupportingMqtt()) && (this->mqttClient != nullptr)) {
				this->disconnectMqtt();
			}
			settingsStored = ((strcmp(getSsid(), "") != 0)
					&& (((isSupportingMqtt()) && (strcmp(getMqttServer(), "") != 0) && (strcmp(getMqttPort(), "") != 0)) || (isSupportingWebThing())));
			if (settingsStored) {
				wlog->notice(F("Settings loaded successfully:"));
			} else {
				wlog->notice(F("Settings are not complete:"));
			}
			wlog->notice(F("SSID: '%s'; MQTT enabled: %d; MQTT server: '%s'; MQTT port: %s; WebThings enabled: %d"),
								  getSsid(), isSupportingMqtt(), getMqttServer(), getMqttPort(), isSupportingWebThing());

		}
		EEPROM.end();
		return settingsStored;
	}

	void handleUnknown() {
		wlog->warning(F("Webserver: 404: '%s'"), this->webServer->uri().c_str());
		webServer->send(404, "text/plain", "404: Not found");
		webServer->client().stop();
	}


	void handleCaptivePortal() {
		wlog->warning(F("Webserver: captive Portal: '%s'"));
		webServer->send(200, "text/plain", "200: Servus");
	}

	void sendDevicesStructure() {
		wlog->notice(F("Send description for all devices... "));
		WStringStream* response = getResponseStream();
		WJson json(response);
		json.beginArray();
		WDevice *device = this->firstDevice;
		while (device != nullptr) {
			if (device->isVisible(WEBTHING)) {
				device->toJsonStructure(&json, "", WEBTHING);
			}
			device = device->next;
		}
		json.endArray();
		webServer->send(200, APPLICATION_JSON, response->c_str());
	}

	void sendDeviceStructure(WDevice *&device) {
		wlog->notice(F("Send description for device: %s"), device->getId());
		WStringStream* response = getResponseStream();
		WJson json(response);
		device->toJsonStructure(&json, "", WEBTHING);
		webServer->send(200, APPLICATION_JSON, response->c_str());
	}

	void sendDeviceValues(WDevice *&device) {
		wlog->notice(F("Send all properties for device: "), device->getId());
		WStringStream* response = getResponseStream();
		WJson json(response);
		json.beginObject();
		if (device->isMainDevice()) {
			json.propertyString("idx", getIdx());
			json.propertyString("ip", getDeviceIp().toString().c_str());
			json.propertyString("firmware", firmwareVersion.c_str());
		}
		device->toJsonValues(&json, WEBTHING);
		json.endObject();
		webServer->send(200, APPLICATION_JSON, response->c_str());
	}

	void getPropertyValue(WProperty *property) {
		WStringStream* response = getResponseStream();
		WJson json(response);
		json.beginObject();
		property->toJsonValue(&json);
		json.endObject();
		property->setRequested(true);
		wlog->notice(F("getPropertyValue %s"), response->c_str());
		webServer->send(200, APPLICATION_JSON, response->c_str());

	}

	void setPropertyValue(WDevice *device) {
		if (webServer->hasArg("plain") == false) {
			webServer->send(422);
			return;
		}
		WJsonParser parser;
		WProperty* property = parser.parse(webServer->arg("plain").c_str(), device);
		if (property != nullptr) {
			//response new value
			wlog->notice(F("Set property value: %s (web request) %s"), property->getId(), webServer->arg("plain").c_str());
			WStringStream* response = getResponseStream();
			WJson json(response);
			json.beginObject();
			property->toJsonValue(&json);
			json.endObject();
			webServer->send(200, APPLICATION_JSON, response->c_str());
		} else {
			// unable to parse json
			wlog->notice(F("unable to parse json: %s"), webServer->arg("plain").c_str());
			webServer->send(500);
		}
	}

	void sendErrorMsg(int status, const char *msg) {
		WStringStream* response = getResponseStream();
		WJson json(response);
		json.beginObject();
		json.propertyString("error", msg);
		json.propertyInteger("status", status);
		json.endObject();
		webServer->send(200, APPLICATION_JSON, response->c_str());
	}

	void bindWebServerCallsNetwork(WDevice *device) {
		wlog->notice(F("Bind webServer calls for device %s"), device->getId());
		String deviceBase("/things/");
		deviceBase.concat(device->getId());
		WProperty *property = device->firstProperty;
		while (property != nullptr) {
			if (property->isVisible(WEBTHING)) {
				String propertyBase = deviceBase + "/properties/" + property->getId();
				webServer->on(propertyBase.c_str(), HTTP_GET, std::bind(&WNetwork::getPropertyValue, this, property));
				webServer->on(propertyBase.c_str(), HTTP_PUT, std::bind(&WNetwork::setPropertyValue, this, device));
			}
			property = property->next;
		}
		String propertiesBase = deviceBase + "/properties";
		webServer->on(propertiesBase.c_str(), HTTP_GET,	std::bind(&WNetwork::sendDeviceValues, this, device));
		webServer->on(deviceBase.c_str(), HTTP_GET,	std::bind(&WNetwork::sendDeviceStructure, this, device));
		device->bindWebServerCalls(webServer);
	}

	WDevice* getDeviceById(const char* deviceId) {
		WDevice *device = this->firstDevice;
		while (device != nullptr) {
			if (strcmp(device->getId(), deviceId) == 0) {
				return device;
			}
			device = device->next;
		}
		return nullptr;
	}
};

#endif