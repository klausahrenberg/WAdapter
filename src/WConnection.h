#ifndef W_NETWORK_H
#define W_NETWORK_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
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
#include "WPage.h"

#define SIZE_JSON_PACKET 1280
#define SIZE_WEB_PAGE 5120
#define NO_LED -1
const char* CONFIG_PASSWORD = "12345678";
const char* APPLICATION_JSON = "application/json";
const char* TEXT_HTML = "text/html";
const char* TEXT_PLAIN = "text/plain";
const char* DEFAULT_TOPIC_STATE = "properties";
const char* DEFAULT_TOPIC_SET = "set";
const String SLASH = "/";

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
WiFiClient wifiClient;
WAdapterMqtt *mqttClient;

class WNetwork {
public:
	typedef std::function<void()> THandlerFunction;
	WNetwork(bool debugging, String applicationName, String firmwareVersion,
			bool startWebServerAutomaticly, int statusLedPin, byte appSettingsFlag) {
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
		this->startWebServerAutomaticly = startWebServerAutomaticly;
		this->webServer = nullptr;
		this->dnsApServer = nullptr;
		this->debugging = debugging;
		this->wlog = new WLog();
		this->setDebuggingOutput(&Serial);
		this->updateRunning = false;
		this->restartFlag = "";
		this->deepSleepFlag = nullptr;
		this->deepSleepSeconds = 0;
		this->startupTime = millis();
		settings = new WSettings(wlog, appSettingsFlag);
		settingsFound = loadSettings();
		this->mqttClient = nullptr;
		lastMqttConnect = lastWifiConnect = 0;
		gotIpEventHandler = WiFi.onStationModeGotIP(
				[this](const WiFiEventStationModeGotIP &event) {
					wlog->notice(F("Station connected, IP: %s"), this->getDeviceIp().toString().c_str());
					//Connect, if webThing supported and Wifi is connected as client
					if ((this->isSupportingWebThing()) && (isWifiConnected())) {
						this->startWebServer();
					}
					this->notify(false);
				});
		disconnectedEventHandler = WiFi.onStationModeDisconnected(
				[this](const WiFiEventStationModeDisconnected &event) {
					wlog->notice("Station disconnected");
					this->disconnectMqtt();
					this->lastMqttConnect = 0;
					this->notify(false);
				});
		if (this->isSupportingMqtt()) {
			this->mqttClient = new WAdapterMqtt(debugging, wifiClient, SIZE_JSON_PACKET);
			mqttClient->setCallback(std::bind(&WNetwork::mqttCallback, this,
										std::placeholders::_1, std::placeholders::_2,
										std::placeholders::_3));
		}
		if (statusLedPin != NO_LED) {
			statusLed = new WLed(statusLedPin);
			statusLed->setOn(true, 500);
		} else {
			statusLed = nullptr;
		}
		wlog->notice(F("firmware: %s"), firmwareVersion.c_str());
	}

	//returns true, if no configuration mode and no own ap is opened
	bool loop(unsigned long now) {
		bool result = true;
		bool waitForWifiConnection = (deepSleepSeconds > 0);
		if ((!settingsFound) && (startWebServerAutomaticly)) {
			this->startWebServer();
		}
		if (!isWebServerRunning()) {
			if (getSsid() != "") {
				//WiFi connection
				if ((WiFi.status() != WL_CONNECTED)	&& ((lastWifiConnect == 0) || (now - lastWifiConnect > 300000))) {
					wlog->notice("Connecting to '%s'", getSsid());
					//Workaround: if disconnect is not called, WIFI connection fails after first startup
					WiFi.disconnect();
					WiFi.hostname(getHostName());
					WiFi.begin(getSsid(), getPassword());
					while ((waitForWifiConnection) && (WiFi.status() != WL_CONNECTED)) {
						delay(100);
						if (millis() - now >= 5000) {
							break;
						}
					}
					//WiFi.waitForConnectResult();
					lastWifiConnect = now;
				}
			}
		} else {
			if (isSoftAP()) {
				dnsApServer->processNextRequest();
			}
			//webServer->handleClient();
			result = ((!isSoftAP()) && (!isUpdateRunning()));
		}
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
		//Loop led
		if (statusLed != nullptr) {
			statusLed->loop(now);
		}
		bool allStatesComplete = true;
		bool stateUpd = false;
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
				handleDeviceStateChange(device, (device->lastStateNotify != 0));
			}
			device = device->next;
		}
#ifdef ESP8266
		if ((!isUpdateRunning()) && (this->isSupportingWebThing()) && (isWifiConnected())) {
			MDNS.update();
		}
#endif
		//Restart required?
		if (!restartFlag.equals("")) {
			this->updateRunning = false;
			stopWebServer();
			delay(1000);
			ESP.restart();
			delay(2000);
		} else if (deepSleepFlag != nullptr) {
			if (deepSleepFlag->off()) {
				//Deep Sleep
				wlog->notice("Go to deep sleep. Bye...");
				this->updateRunning = false;
				stopWebServer();
				delay(500);
				ESP.deepSleep(deepSleepSeconds * 1000 * 1000);
			} else {
				deepSleepFlag = nullptr;
			}
		}
		return result;
	}

	~WNetwork() {
		delete wlog;
	}

	WSettings* getSettings() {
		return this->settings;
	}

	void setDebuggingOutput(Print* output) {
		this->wlog->setOutput(output, (debugging ? LOG_LEVEL_NOTICE : LOG_LEVEL_SILENT), true, true);
	}

	void setOnNotify(THandlerFunction onNotify) {
		this->onNotify = onNotify;
	}

	void setOnConfigurationFinished(THandlerFunction onConfigurationFinished) {
		this->onConfigurationFinished = onConfigurationFinished;
	}

	bool publishMqtt(const char* topic, WStringStream* response, bool retained=false) {
		wlog->notice(F("MQTT... '%s'; %s"), topic, response->c_str());
		if (isMqttConnected()) {
			if (mqttClient->publish(topic, response->c_str(), retained)) {
				wlog->notice(F("MQTT sent. Topic: '%s'"), topic);
				return true;
			} else {
				wlog->notice(F("Sending MQTT message failed, rc=%d"), mqttClient->state());
				this->disconnectMqtt();
				return false;
			}
		} else {
			if (strcmp(getMqttServer(), "") != 0) {
				wlog->notice(F("Can't send MQTT. Not connected to server: %s"), getMqttServer());
			}
			return false;
		}
		wlog->notice(F("publish MQTT mystery... "));
	}

	bool publishMqtt(const char* topic, const char* key, const char* value) {
		if ((this->isMqttConnected()) && (this->isSupportingMqtt())) {
			WStringStream* response = getResponseStream();
			WJson json(response);
			json.beginObject();
			json.propertyString(key, value);
			json.endObject();
			return publishMqtt(topic, response);
		} else {
			return false;
		}
	}

	// Creates a web server. If Wifi is not connected, then an own AP will be created
	void startWebServer() {
		if (!isWebServerRunning()) {
			String apSsid = getClientName(false);
			webServer = new AsyncWebServer(80);
			if (WiFi.status() != WL_CONNECTED) {
				//Create own AP
				wlog->notice(F("Start AccessPoint for configuration. SSID '%s'; password '%s'"), apSsid.c_str(), CONFIG_PASSWORD);
				dnsApServer = new DNSServer();
				WiFi.softAP(apSsid.c_str(), CONFIG_PASSWORD);
				dnsApServer->setErrorReplyCode(DNSReplyCode::NoError);
				dnsApServer->start(53, "*", WiFi.softAPIP());
			} else {
				wlog->notice(F("Start web server for configuration. IP %s"), this->getDeviceIp().toString().c_str());
			}
			webServer->onNotFound(std::bind(&WNetwork::handleUnknown, this));
			if ((WiFi.status() != WL_CONNECTED) || (!this->isSupportingWebThing())) {
				webServer->on(SLASH, HTTP_GET, std::bind(&WNetwork::handleHttpRootRequest, this));
			}
			webServer->on("/config", HTTP_GET, std::bind(&WNetwork::handleHttpRootRequest, this));
			WPage* page = this->firstPage;
			while (page != nullptr) {
				String did(SLASH);
				did.concat(page->getId());
				webServer->on(did, HTTP_GET, std::bind(&WNetwork::handleHttpCustomPage, this, page));
				String dis("/submit");
				dis.concat(page->getId());
				webServer->on(dis.c_str(), HTTP_GET, std::bind(&WNetwork::handleHttpSubmittedCustomPage, this, page));
				page = page->next;
			}
			webServer->on("/wifi", HTTP_GET,
					std::bind(&WNetwork::handleHttpNetworkConfiguration, this));
			webServer->on("/submitnetwork", HTTP_GET,
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
				String mdnsName = getHostName();
				//String mdnsName = this->getDeviceIp().toString();
				if (MDNS.begin(mdnsName)) {
					MDNS.addService("webthing", "tcp", 80);
			    MDNS.addServiceTxt("webthing", "tcp", "path", "/");

					//MDNS.addService("http", "tcp", 80);
					//MDNS.addServiceTxt("http", "tcp", "url", "http://" + mdnsName + SLASH);
					//MDNS.addServiceTxt("http", "tcp", "webthing", "true");
					wlog->notice(F("MDNS responder started at %s"), mdnsName.c_str());
				}
				webServer->on(SLASH, HTTP_GET, std::bind(&WNetwork::sendDevicesStructure, this));
				WDevice *device = this->firstDevice;
				while (device != nullptr) {
					bindWebServerCalls(device);
					device = device->next;
				}
			}
			//Start http server
			webServer->begin();
			wlog->notice(F("webServer started."));
			this->notify(true);
		}
	}

	void stopWebServer() {
		if ((isWebServerRunning()) && (!this->isSupportingWebThing()) && (!this->updateRunning)) {
			wlog->notice(F("Close web configuration."));
			delay(100);
			webServer->stop();
			webServer = nullptr;
			if (onConfigurationFinished) {
				onConfigurationFinished();
			}
			this->notify(true);
		}
	}

	void enableWebServer(bool startWebServer) {
		if (startWebServer) {
			this->startWebServer();
		} else {
			this->stopWebServer();
		}
	}

	bool isWebServerRunning() {
		return (webServer != nullptr);
	}

	bool isUpdateRunning() {
		return this->updateRunning;
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

	IPAddress getDeviceIp() {
		return (isSoftAP() ? WiFi.softAPIP() : WiFi.localIP());
	}

	bool isSupportingWebThing() {
		return this->supportingWebThing;
	}

	void setSupportingWebThing(bool supportingWebThing) {
		this->supportingWebThing = supportingWebThing;
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

	const char* getMqttBaseTopic() {
		return this->mqttBaseTopic->c_str();
	}

	const char* getMqttSetTopic() {
		return this->mqttSetTopic->c_str();
	}

	const char* getMqttStateTopic() {
		return this->mqttStateTopic->c_str();
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

		bindWebServerCalls(device);
	}

	void addCustomPage(WPage *Page) {
		if (lastPage == nullptr) {
			firstPage = Page;
			lastPage = Page;
		} else {
			lastPage->next = Page;
			lastPage = Page;
		}
	}

	void setDeepSleepSeconds(int dsp) {
		this->deepSleepSeconds = dsp;
	}

	WStringStream* getResponseStream() {
		if (responseStream == nullptr) {
			responseStream = new WStringStream(SIZE_JSON_PACKET);
		}
		responseStream->flush();
		return responseStream;
	}

	template<class T, typename ... Args> void error(T msg, Args ... args) {
		logLevel(LOG_LEVEL_ERROR, msg, args...);
	}

	template<class T, typename ... Args> void debug(T msg, Args ...args) {
		logLevel(LOG_LEVEL_DEBUG, msg, args...);
	}

	template<class T, typename ... Args> void notice(T msg, Args ...args) {
		logLevel(LOG_LEVEL_NOTICE, msg, args...);
	}

	template<class T, typename ... Args> void logLevel(int level, T msg, Args ...args) {
		wlog->printLevel(level, msg, args...);
		if ((isMqttConnected()) && ((level == LOG_LEVEL_ERROR) || (debugging))) {
			WStringStream* response = getResponseStream();
			WJson json(response);
			json.beginObject();
			json.memberName(this->wlog->getLevelString(level));
			response->print(QUOTE);
			this->wlog->setOutput(response, level, false, false);
			this->wlog->printLevel(level, msg, args...);
			this->setDebuggingOutput(&Serial);
			response->print(QUOTE);
			json.endObject();
			publishMqtt(mqttBaseTopic->c_str(), response);
		}
	}

	bool isDebugging() {
		return this->debugging;
	}

private:
	WLog* wlog;
	WDevice *firstDevice = nullptr;
	WDevice *lastDevice = nullptr;
	WPage* firstPage = nullptr;
	WPage* lastPage = nullptr;
	THandlerFunction onNotify;
	THandlerFunction onConfigurationFinished;
	bool debugging, updateRunning, startWebServerAutomaticly;
	String restartFlag;
	DNSServer *dnsApServer;
	AsyncWebServer *webServer;
	int networkState;
	String applicationName;
	String firmwareVersion;
	const char* firmwareUpdateError;
	WProperty *supportingMqtt;
	bool supportingWebThing;
	WProperty *ssid;
	WProperty *idx;
	WProperty *mqttBaseTopic;
	WProperty *mqttSetTopic;
	WProperty *mqttStateTopic;
	WAdapterMqtt *mqttClient;
	long lastMqttConnect, lastWifiConnect;
	WStringStream* responseStream = nullptr;
	WLed *statusLed;
	WSettings *settings;
	bool settingsFound;
	WDevice *deepSleepFlag;
	int deepSleepSeconds;
	unsigned long startupTime;


	/*void handleDeviceMessage(WDevice *device, const char* pTopic, const char* key, const char* value) {
		wlog->notice(F("Device failure -> send "), pTopic, "... ", key, F(": "), value);
		String topic = String(getMqttBaseTopic()) + SLASH + String(device->getId()) + SLASH + String(pTopic);
		publishMqtt(topic.c_str(), key, value);
	}*/

	void handleDeviceStateChange(WDevice *device, bool complete) {
		wlog->notice(F("Device state changed -> send device state..."));
		String topic = String(getMqttBaseTopic()) + SLASH + String(device->getId()) + SLASH + String(getMqttStateTopic());
		mqttSendDeviceState(topic, device, complete);
	}

	void mqttSendDeviceState(String topic, WDevice *device, bool complete) {
		if ((this->isMqttConnected()) && (isSupportingMqtt()) && (device->isDeviceStateComplete())) {
			wlog->notice(F("Send actual device state via MQTT"));

			if (device->sendCompleteDeviceState()) {
				//Send all properties of device in one json structure
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

				mqttClient->publish(topic.c_str(), response->c_str(), true);
			} else {
				//Send every changed property only
				WProperty* property = device->firstProperty;
				while (property != nullptr) {
					if ((complete) || (property->isChanged())) {
						if (property->isVisible(MQTT)) {
							WStringStream* response = getResponseStream();
							WJson json(response);
							property->toJsonValue(&json, true);
							mqttClient->publish(String(topic + SLASH + String(property->getId())).c_str(), response->c_str(), true);
						}
						property->setUnChanged();
					}
					property = property->next;
				}
			}

			device->lastStateNotify = millis();
			if ((deepSleepSeconds > 0)	&& ((!this->isSupportingWebThing())	|| (device->areAllPropertiesRequested()))) {
				deepSleepFlag = device;
			}
		}
	}

	void mqttCallback(char *ptopic, char *payload, unsigned int length) {
		wlog->notice(F("Received MQTT callback: %s/{%s}"), ptopic, payload);
		String baseT = String(getMqttBaseTopic());
		String stateT = String(getMqttStateTopic());
		String setT = String(getMqttSetTopic());

		String topic = String(ptopic).substring(baseT.length() + 1);
		wlog->notice(F("Topic short '%s'"), topic.c_str());
		//Next is device id
		int i = topic.indexOf(SLASH);
		if (i > -1) {
			String deviceId = topic.substring(0, i);
			wlog->notice(F("look for device id '%s'"), deviceId.c_str());
			WDevice *device = this->getDeviceById(deviceId.c_str());
			if (device != nullptr) {
				topic = topic.substring(i + 1);
				if (topic.startsWith(stateT)) {
					if (length == 0) {
						//State request
						topic = topic.substring(stateT.length() + 1);
						if (topic.equals("")) {
							//send all propertiesBase
							wlog->notice(F("Send complete device state..."));
							//Empty payload for topic 'properties' -> send device state
							mqttSendDeviceState(String(ptopic), device, true);
						} else {
							WProperty* property = device->getPropertyById(topic.c_str());
							if (property != nullptr) {
								if (property->isVisible(MQTT)) {
									wlog->notice(F("Send state of property: "), property->getId());
									WStringStream* response = getResponseStream();
									WJson json(response);
									property->toJsonValue(&json, true);
									mqttClient->publish(String(baseT + SLASH + deviceId + SLASH + stateT + SLASH + String(property->getId())).c_str(), response->c_str(), true);
								}
							} else {
								device->handleUnknownMqttCallback(true, ptopic, topic, payload, length);
							}
						}
					}
				}	else if (topic.startsWith(setT)) {
					if (length > 0) {
						//Set request
						topic = topic.substring(setT.length() + 1);
						if (topic.equals("")) {
							//set all properties
							wlog->notice(F("Try to set several properties for device %s"), device->getId());
							WJsonParser* parser = new WJsonParser();
							if (parser->parse(payload, device) == nullptr) {
								wlog->notice(F("No properties updated for device %s"), device->getId());
							} else {
								wlog->notice(F("One or more properties updated for device %s"), device->getId());
							}
						}	else {
							//Try to find property and set single value
							WProperty* property = device->getPropertyById(topic.c_str());
							if (property != nullptr) {
								if (property->isVisible(MQTT)) {
									//Set Property
									wlog->notice(F("Try to set property %s for device %s"), property->getId(), device->getId());
									if (!property->parse((char *) payload)) {
										wlog->notice(F("Property not updated."));
									} else {
										wlog->notice(F("Property updated."));
									}
								}
							} else {
								device->handleUnknownMqttCallback(false, ptopic, topic, payload, length);
							}
						}
					}
				} else {
					//unknown, ask the device
					//device->handleUnknownMqttCallback(ptopic, topic, payload, length);
				}
			}	else if (deviceId.equals("webServer")) {
				enableWebServer(String((char *) payload).equals(HTTP_TRUE));
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
				if (this->deepSleepSeconds == 0) {
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
						json.propertyString("stateTopic", getMqttBaseTopic(), SLASH.c_str(), device->getId(), SLASH.c_str(), getMqttStateTopic());
						json.propertyString("setTopic", getMqttBaseTopic(), SLASH.c_str(), device->getId(), SLASH.c_str(), getMqttSetTopic());
						json.endObject();

						mqttClient->publish(topic.c_str(), response->c_str(), false);
						device = device->next;
					}
					mqttClient->unsubscribe("devices/#");
				}
				//Subscribe to device specific topic
				mqttClient->subscribe(String(String(getMqttBaseTopic()) + "/#").c_str());
				notify(false);
				return true;
			} else {
				wlog->notice(F("Connection to MQTT server failed, rc=%d"), mqttClient->state());
				if (startWebServerAutomaticly) {
					this->startWebServer();
				}
				notify(false);
				return false;
			}
		}
	}

	void notify(bool sendState) {
		if (statusLed != nullptr) {
			if (isWifiConnected()) {
				statusLed->setOn(false);
			} else if (isSoftAP()) {
				statusLed->setOn(true, 0);
			} else {
				statusLed->setOn(true, 500);
			}
		}
		if (sendState) {
			WDevice *device = this->firstDevice;
			while (device != nullptr) {
				handleDeviceStateChange(device, false);
				device = device->next;
			}
		}
		if (onNotify) {
			onNotify();
		}
	}

	void handleHttpRootRequest() {
		if (isWebServerRunning()) {
			if (restartFlag.equals("")) {
				WStringStream* page = new WStringStream(SIZE_WEB_PAGE);
				page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), applicationName.c_str());
				page->print(FPSTR(HTTP_STYLE));
				page->print(FPSTR(HTTP_HEAD_END));
				printHttpCaption(page);
				page->printAndReplace(FPSTR(HTTP_BUTTON), "wifi", "get", "Configure network");
				WPage *customPage = firstPage;
				while (customPage != nullptr) {
					page->printAndReplace(FPSTR(HTTP_BUTTON), customPage->getId(), "get", customPage->getTitle());
					customPage = customPage->next;
				}
				page->printAndReplace(FPSTR(HTTP_BUTTON), "firmware", "get", "Update firmware");
				page->printAndReplace(FPSTR(HTTP_BUTTON), "info", "get", "Info");
				page->printAndReplace(FPSTR(HTTP_BUTTON), "reset", "post", "Reboot");
				page->print(FPSTR(HTTP_BODY_END));
				webServer->send(200, TEXT_HTML, page->c_str());
				delete page;
			} else {
				WStringStream* page = new WStringStream(SIZE_WEB_PAGE);
				page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Info");
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

	void handleHttpCustomPage(WPage *&customPage) {
		if (isWebServerRunning()) {
			wlog->notice(F("Custom page called: %s"), customPage->getId());
			WStringStream* page = new WStringStream(SIZE_WEB_PAGE);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), customPage->getTitle());
			page->print(FPSTR(HTTP_STYLE));
			page->print(FPSTR(HTTP_HEAD_END));
			printHttpCaption(page);
			customPage->printPage(webServer, page);
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			delete page;
		}

	}

	void handleHttpNetworkConfiguration() {
		if (isWebServerRunning()) {
			wlog->notice(F("Network config page"));
			WStringStream* page = new WStringStream(SIZE_WEB_PAGE);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Network Configuration");
			page->print(FPSTR(HTTP_STYLE));
			page->print(FPSTR(HTTP_HEAD_END));
			printHttpCaption(page);
			page->printAndReplace(FPSTR(HTTP_CONFIG_PAGE_BEGIN), "network");
			page->printAndReplace(FPSTR(HTTP_TOGGLE_GROUP_STYLE), "ga", (this->isSupportingMqtt() ? HTTP_BLOCK : HTTP_NONE), "gb", HTTP_NONE);
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "Idx:", "i", "16", getIdx());
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "Wifi ssid (only 2.4G):", "s", "32", getSsid());
			page->printAndReplace(FPSTR(HTTP_PASSWORD_FIELD), "Wifi password:", "p", "32", getPassword());
			//mqtt
			page->printAndReplace(FPSTR(HTTP_CHECKBOX_OPTION), "sa", "sa", (this->isSupportingMqtt() ? HTTP_CHECKED : ""), "tg()", "Support MQTT");
			page->printAndReplace(FPSTR(HTTP_DIV_ID_BEGIN), "ga");
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "MQTT Server:", "ms", "32", getMqttServer());
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "MQTT Port:", "mo", "4", getMqttPort());
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "MQTT User:", "mu", "16", getMqttUser());
			page->printAndReplace(FPSTR(HTTP_PASSWORD_FIELD), "MQTT Password:", "mp", "32", getMqttPassword());
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "MQTT Topic:", "mt", "32", getMqttBaseTopic());
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "Topic for state requests:", "mtg", "16", getMqttStateTopic());
			page->printAndReplace(FPSTR(HTTP_TEXT_FIELD), "Topic for setting values:", "mts", "16", getMqttSetTopic());

			page->print(FPSTR(HTTP_DIV_END));
			page->printAndReplace(FPSTR(HTTP_TOGGLE_FUNCTION_SCRIPT), "tg()", "sa", "ga", "gb");
			page->print(FPSTR(HTTP_CONFIG_SAVE_BUTTON));
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			delete page;
		}
	}

	void handleHttpSaveConfiguration() {
		if (isWebServerRunning()) {
			settings->saveOnPropertyChanges = false;
			this->idx->setString(webServer->arg("i").c_str());
			this->ssid->setString(webServer->arg("s").c_str());
			settings->setString("password", webServer->arg("p").c_str());
			this->supportingMqtt->setBoolean(webServer->arg("sa") == HTTP_TRUE);
			settings->setString("mqttServer", webServer->arg("ms").c_str());
			String mqtt_port = webServer->arg("mo");
			settings->setString("mqttPort", (mqtt_port != "" ? mqtt_port.c_str() : "1883"));
			settings->setString("mqttUser", webServer->arg("mu").c_str());
			settings->setString("mqttPassword", webServer->arg("mp").c_str());
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
			this->restart("Settings saved. If MQTT activated, subscribe to topic 'devices/#' at your broker.");
		}
	}

	void handleHttpSubmittedCustomPage(WPage *&customPage) {
		if (isWebServerRunning()) {
			wlog->notice(F("Save custom page: %s"), customPage->getId());
			settings->saveOnPropertyChanges = false;
			WStringStream* page = new WStringStream(1024);
			customPage->submittedPage(webServer, page);
			settings->save();
			delay(300);
			this->restart(strlen(page->c_str()) == 0 ? "Settings saved." : page->c_str());
			delete page;
		}
	}

	void handleHttpInfo() {
		if (isWebServerRunning()) {
			WStringStream* page = new WStringStream(SIZE_WEB_PAGE);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Info");
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
			page->print("<tr><th>Running since:</th><td>");
			page->print(((millis() - this->startupTime)/1000/60));
			page->print(" minutes</td></tr>");
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
		page->print("</h2><h3>Revision ");
		page->print(firmwareVersion);
		page->print(debugging ? " (debug)" : "");
		page->print("</h3>");
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
			WStringStream* page = new WStringStream(SIZE_WEB_PAGE);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Firmware update");
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
				uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
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

	void restart(const char* reasonMessage) {
		this->restartFlag = reasonMessage;
		webServer->client().setNoDelay(true);
		WStringStream* page = new WStringStream(SIZE_WEB_PAGE);
		page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), reasonMessage);
		page->print(FPSTR(HTTP_STYLE));
		page->print(FPSTR(HTTP_HEAD_END));
		printHttpCaption(page);
		page->printAndReplace(FPSTR(HTTP_SAVED), reasonMessage);
		page->print(FPSTR(HTTP_BODY_END));
		webServer->send(200, TEXT_HTML, page->c_str());
		delete page;
	}

	bool loadSettings() {
		this->idx = settings->setString("idx", 16, this->getClientName(true).c_str());
		this->ssid = settings->setString("ssid", 32, "");
		settings->setString("password", 32, "");
		this->supportingWebThing = true;
		this->supportingMqtt = settings->setBoolean("supportingMqtt", false);
		settings->setString("mqttServer", 32, "");
		settings->setString("mqttPort", 4, "1883");
		settings->setString("mqttUser", 16, "");
		settings->setString("mqttPassword", 32, "");
		this->mqttBaseTopic = settings->setString("mqttTopic", 32, getIdx());
		this->mqttStateTopic = settings->setString("mqttStateTopic", 16, DEFAULT_TOPIC_STATE);
		this->mqttSetTopic = settings->setString("mqttSetTopic", 16, DEFAULT_TOPIC_SET);

		bool settingsStored = settings->existsNetworkSettings();
		if (settingsStored) {
			if (getMqttBaseTopic() == "") {
				this->mqttBaseTopic->setString(this->getClientName(true).c_str());
			}
			if ((isSupportingMqtt()) && (this->mqttClient != nullptr)) {
				this->disconnectMqtt();
			}
			settingsStored = ((strcmp(getSsid(), "") != 0)
					&& (((isSupportingMqtt()) && (strcmp(getMqttServer(), "") != 0) && (strcmp(getMqttPort(), "") != 0)) || (isSupportingWebThing())));
			if (settingsStored) {
				wlog->notice(F("Network settings loaded successfully."));
			} else {
				wlog->notice(F("Network settings are missing."));
			}
			wlog->notice(F("SSID: '%s'; MQTT enabled: %T; MQTT server: '%s'; MQTT port: %s; WebThings enabled: %T"),
								  getSsid(), isSupportingMqtt(), getMqttServer(), getMqttPort(), isSupportingWebThing());
		}
		EEPROM.end();
		settings->addingNetworkSettings = false;
		return settingsStored;
	}

	void handleUnknown() {
		webServer->send(404);
	}

	void sendDevicesStructure() {
		wlog->notice(F("Send description for all devices... "));
		WStringStream* response = new WStringStream(SIZE_WEB_PAGE);
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
		delete response;
	}

	void sendDeviceStructure(WDevice *&device) {
		wlog->notice(F("Send description for device: %s"), device->getId());
		WStringStream* response = new WStringStream(SIZE_WEB_PAGE);
		WJson json(response);
		device->toJsonStructure(&json, "", WEBTHING);
		webServer->send(200, APPLICATION_JSON, response->c_str());
		delete response;
	}

	void sendDeviceValues(WDevice *&device) {
		wlog->notice(F("Send all properties for device: "), device->getId());
		WStringStream* response = new WStringStream(SIZE_WEB_PAGE);
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
		delete response;
	}

	void getPropertyValue(AsyncWebServerRequest *request, WProperty *property) {
		WStringStream* response = new WStringStream(SIZE_WEB_PAGE);
		WJson json(response);
		json.beginObject();
		property->toJsonValue(&json);
		json.endObject();
		property->setRequested(true);
		wlog->notice(F("getPropertyValue %s"), response->c_str());
		request->send(200, APPLICATION_JSON, response->c_str());
		delete response;

		if (deepSleepSeconds > 0) {
			WDevice *device = firstDevice;
			while ((device != nullptr) && (deepSleepFlag == nullptr)) {
				if ((!this->isSupportingWebThing()) || (device->areAllPropertiesRequested())) {
					deepSleepFlag = device;
				}
				device = device->next;
			}
		}
	}

	void setPropertyValue(AsyncWebServerRequest *request, WDevice *device) {
		if (request->hasArg("plain") == false) {
			request->send(422);
			return;
		}
		WJsonParser parser;
		WProperty* property = parser.parse(request->arg("plain").c_str(), device);
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

	void bindWebServerCalls(WDevice *device) {
		if (this->isWebServerRunning()) {
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
