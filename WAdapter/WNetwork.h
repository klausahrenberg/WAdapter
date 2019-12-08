#ifndef W_NETWORK_H
#define W_NETWORK_H

//#if defined(ESP32) || defined(ESP8266)

#include <Arduino.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#ifdef ESP8266
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif
//#include <WebSocketsClient.h>
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
#define SIZE_JSON_PACKET 1280
#define NO_LED -1
const char* CONFIG_PASSWORD = "12345678";
const char* APPLICATION_JSON = "application/json";
const char* TEXT_HTML = "text/html";
const char* TEXT_PLAIN = "text/plain";

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
WiFiClient wifiClient;
WAdapterMqtt *mqttClient;

class WNetwork {
public:
	typedef std::function<void(void)> THandlerFunction;
	WNetwork(bool debug, String applicationName, String firmwareVersion,
			bool startWebServerAutomaticly, int statusLedPin) {
		WiFi.mode(WIFI_STA);
		this->applicationName = applicationName;
		this->firmwareVersion = firmwareVersion;
		this->startWebServerAutomaticly = startWebServerAutomaticly;
		this->webServer = nullptr;
		this->dnsApServer = nullptr;
		this->debug = debug;
		wlog = new WLog((this->debug ? LOG_LEVEL_VERBOSE : LOG_LEVEL_SILENT), &Serial);
		this->updateRunning = false;
		this->restartFlag = "";
		this->deepSleepFlag = nullptr;
		this->deepSleepSeconds = 0;
		//this->webSocket = nullptr;
		settings = new WSettings(wlog);
		settingsFound = loadSettings();
		this->mqttClient = nullptr;
		lastMqttConnect = lastWifiConnect = 0;
		gotIpEventHandler = WiFi.onStationModeGotIP(
				[this](const WiFiEventStationModeGotIP &event) {
					wlog->notice(F("Station connected, IP: %s"), this->getDeviceIpAsString().c_str());
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
			this->mqttClient = new WAdapterMqtt(debug, wifiClient, SIZE_JSON_PACKET);
			mqttClient->setCallback(std::bind(&WNetwork::mqttCallback, this,
										std::placeholders::_1, std::placeholders::_2,
										std::placeholders::_3));
			/*mqttClient->setCallback(
					[this](char *topic, char *payload, unsigned int length) {
						this->mqttCallback(topic, payload, length);
					});*/
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
						delay(500);
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
			webServer->begin();
			webServer->handleClient();
			/*if (webSocket != nullptr) {
				webSocket->loop();
			}*/
			result = ((!isSoftAP()) && (!isUpdateRunning()));
		}
		//MQTT connection
		if ((isWifiConnected()) && (isSupportingMqtt())
				&& (!mqttClient->connected())
				&& ((lastMqttConnect == 0) || (now - lastMqttConnect > 300000))
				&& (strcmp(getMqttServer(), "") != 0)) {
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
		//Loop Devices
		WDevice *device = firstDevice;
		while (device != nullptr) {
			device->loop(now);
			if ((this->isMqttConnected()) && (this->isSupportingMqtt())
					&& ((device->lastStateNotify == 0)
							|| ((device->stateNotifyInterval > 0)
									&& (now - device->lastStateNotify
											> device->stateNotifyInterval)))
					&& (device->isDeviceStateComplete())) {
				handleDeviceStateChange(device);
			}
			device = device->next;
		}
		//WebThingAdapter
		if ((!isUpdateRunning()) && (this->isSupportingWebThing())
				&& (isWifiConnected())) {
			MDNS.update();
		}
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

	void setOnNotify(THandlerFunction onNotify) {
		this->onNotify = onNotify;
	}

	void setOnConfigurationFinished(THandlerFunction onConfigurationFinished) {
		this->onConfigurationFinished = onConfigurationFinished;
	}

	bool publishMqtt(const char* topic, const char* key, const char* value) {
		if (this->isSupportingMqtt()) {
			if (isMqttConnected()) {
				WStringStream* response = getResponseStream();
				WJson json(response);
				json.beginObject();
				json.propertyString(key, value);
				json.endObject();
				if (mqttClient->publish(topic, response->c_str())) {
					return true;
				} else {
					wlog->notice(F("Sending MQTT message failed, rc=%d"), mqttClient->state());
					this->disconnectMqtt();
					return false;
				}
			} else {
				if (strcmp(getMqttServer(), "") != 0) {
					wlog->notice("Can't send MQTT. Not connected to server: %s", getMqttServer());
				}
				return false;
			}
		}
	}

	// Creates a web server. If Wifi is not connected, then an own AP will be created
	void startWebServer() {
		if (!isWebServerRunning()) {
			String apSsid = getClientName(false);
			webServer = new ESP8266WebServer(80);
			if (WiFi.status() != WL_CONNECTED) {
				//Create own AP
				wlog->notice(F("Start AccessPoint for configuration. SSID '%s'; password '%s'"), apSsid.c_str(), CONFIG_PASSWORD);
				dnsApServer = new DNSServer();
				WiFi.softAP(apSsid.c_str(), CONFIG_PASSWORD);
				dnsApServer->setErrorReplyCode(DNSReplyCode::NoError);
				dnsApServer->start(53, "*", WiFi.softAPIP());
			} else {
				wlog->notice(F("Start web server for configuration. IP %s"), this->getDeviceIpAsString().c_str());
			}
			webServer->onNotFound(std::bind(&WNetwork::handleUnknown, this));
			if ((WiFi.status() != WL_CONNECTED)
					|| (!this->isSupportingWebThing())) {
				webServer->on("/", HTTP_GET, std::bind(&WNetwork::handleHttpRootRequest, this));
			}
			webServer->on("/config", HTTP_GET, std::bind(&WNetwork::handleHttpRootRequest, this));
			WDevice *device = this->firstDevice;
			while (device != nullptr) {
				if (device->isProvidingConfigPage()) {
					String did("/");
					did.concat(device->getId());
					webServer->on(did, HTTP_GET, std::bind(&WNetwork::handleHttpDeviceConfiguration, this, device));
					String deviceConfiguration("/saveDeviceConfiguration_");
					deviceConfiguration.concat(device->getId());
					webServer->on(deviceConfiguration.c_str(), HTTP_GET, std::bind(&WNetwork::handleHttpSaveDeviceConfiguration, this, device));
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
				String mdnsName = this->getDeviceIpAsString();
				if (MDNS.begin(mdnsName)) {
					MDNS.addService("http", "tcp", 80);
					MDNS.addServiceTxt("http", "tcp", "url", "http://" + mdnsName + "/");
					MDNS.addServiceTxt("http", "tcp", "webthing", "true");
					wlog->notice(F("MDNS responder started at %s"), mdnsName.c_str());
				}
				webServer->on("/", HTTP_GET, std::bind(&WNetwork::sendDevicesStructure, this));
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

	IPAddress getDeviceIp() {
		return (isSoftAP() ? WiFi.softAPIP() : WiFi.localIP());
	}

	String getDeviceIpAsString() {
		return getDeviceIp().toString();
	}

	bool isSupportingWebThing() {
		return this->supportingWebThing->getBoolean();
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
		bindWebServerCalls(device);
	}

	void setDeepSleepSeconds(int dsp) {
		this->deepSleepSeconds = dsp;
	}

	WLog* log() {
		return wlog;
	}

	/*void dbg(String debugMessage) {
		if (debug) {
			Serial.println(debugMessage);
		}
	}

	void dbg(String dm1, String dm2) {
		if (debug) {
			Serial.print(dm1);
			Serial.println(dm2);
		}
	}

	void dbg(String* dm1, String dm2, String dm3) {
		if (debug) {
			Serial.print(dm1);
			Serial.print(dm2);
			Serial.println(dm3);
		}
	}*/

private:
	WLog* wlog;
	WDevice *firstDevice = nullptr;
	WDevice *lastDevice = nullptr;
	THandlerFunction onNotify;
	THandlerFunction onConfigurationFinished;
	bool debug, updateRunning, startWebServerAutomaticly;
	//int jsonBufferSize;
	String restartFlag;
	DNSServer *dnsApServer;
	ESP8266WebServer *webServer;
	int networkState;
	String applicationName;
	String firmwareVersion;
	String firmwareUpdateError;
	WProperty *supportingWebThing;
	WProperty *supportingMqtt;
	WProperty *ssid;
	WProperty *idx;
	WProperty *mqttTopic;
	WAdapterMqtt *mqttClient;
	long lastMqttConnect, lastWifiConnect;
	WStringStream* responseStream = nullptr;
	WLed *statusLed;
	WSettings *settings;
	bool settingsFound;
	WDevice *deepSleepFlag;
	int deepSleepSeconds;
	//WebSocketsClient* webSocket;

	WStringStream* getResponseStream() {
		if (responseStream == nullptr) {
			responseStream = new WStringStream(SIZE_JSON_PACKET);
		}
		responseStream->flush();
		return responseStream;
	}

	void handleDeviceStateChange(WDevice *device) {
		String topic = String(getMqttTopic()) + "/things/" + String(device->getId()) + "/properties";
		mqttSendDeviceState(topic, device);
	}

	void mqttSendDeviceState(String topic, WDevice *device) {
		if ((this->isMqttConnected()) && (isSupportingMqtt()) && (device->isDeviceStateComplete())) {
			wlog->notice(F("Send actual device state via MQTT"));

			WStringStream* response = getResponseStream();
			WJson json(response);
			device->toJsonValues(&json, MQTT);
			if (mqttClient->publish(topic.c_str(), response->c_str())) {
				device->lastStateWaitForResponse = true;
			}
			device->lastStateNotify = millis();
			if ((deepSleepSeconds > 0)	&& ((!this->isSupportingWebThing())	|| (device->areAllPropertiesRequested()))) {
				deepSleepFlag = device;
			}
		}
	}

	void mqttCallback(char *ptopic, char *payload, unsigned int length) {
		//create character buffer with ending null terminator (string)
		/*char message_buff[this->mqttClient->getMaxPacketSize()];
		for (unsigned int i = 0; i < length; i++) {
			message_buff[i] = payload[i];
		}
		message_buff[length] = '\0';*/
		//forward to serial port
		wlog->notice(F("Received MQTT callback: %s/{%s}"), ptopic, payload);
		String topic = String(ptopic).substring(strlen(getMqttTopic()) + 1);
		wlog->notice(F("Topic short '%s'"), topic.c_str());
		if (topic.startsWith("things/")) {
			topic = topic.substring(String("things/").length());
			int i = topic.indexOf("/");
			if (i > -1) {
				String deviceId = topic.substring(0, i);
				wlog->notice(F("look for device id '%s'"), deviceId.c_str());
				WDevice *device = this->getDeviceById(deviceId.c_str());
				if (device != nullptr) {
					topic = topic.substring(i + 1);
					if (topic.startsWith("properties")) {
						topic = topic.substring(String("properties").length() + 1);
						if (topic.equals("")) {
							if (length > 0) {
								//Check, if it's only response to a state before
								if (!device->lastStateWaitForResponse) {
									wlog->notice(F("Try to set several properties for device %s"), device->getId());
									WJsonParser* parser = new WJsonParser();
									if (parser->parse(device, (char *) payload) == nullptr) {
										wlog->notice(F("No properties updated for device %s"), device->getId());
									} else {
										wlog->notice(F("One or more properties updated for device %s"), device->getId());
									}
								}
								device->lastStateWaitForResponse = false;
							} else {
								//Empty payload for topic 'properties' -> send device state
								mqttSendDeviceState(String(ptopic), device);
							}
						} else {
							//There are still some more topics after properties
							//Try to find property with that topic and set single value
							WProperty* property = device->getPropertyById(topic.c_str());
							if ((property != nullptr) && (property->isVisible(MQTT))) {
								//Set Property
								wlog->notice(F("Try to set property %s for device %s"), property->getId(), device->getId());
								if (!property->parse((char *) payload)) {
									wlog->notice(F("Property not updated."));
								} else {
									wlog->notice(F("Property updated."));
								}
								//DynamicJsonDocument* getJson(32);
								//JsonVariant value = doc.to<JsonVariant>();
								//value.set(message_buff);
								//JsonVariant value = message_buff;
								//property->setFromJson(value);
							}
						}
					}
				}
			}
		} else if (topic.equals("webServer")) {
			enableWebServer(String((char *) payload).equals("true"));
		}
	}

	bool mqttReconnect() {
		if (this->isSupportingMqtt()) {
			wlog->notice(F("Connect to MQTT server: %s; user: '%s'; password: '%s'; clientName: '%s'"),
					   getMqttServer(), getMqttUser(), getMqttPassword(), getClientName(true).c_str());
			// Attempt to connect
			this->mqttClient->setServer(getMqttServer(), 1883);
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
						json.propertyString("url", "http://", getDeviceIpAsString().c_str(), "/things/", device->getId());
						json.propertyString("ip", getDeviceIpAsString().c_str());
						json.propertyString("topic", getMqttTopic(), "/things/", device->getId());
						json.endObject();
						mqttClient->publish(topic.c_str(), response->c_str());
						device = device->next;
					}

					/*WDevice *device = this->firstDevice;
					while (device != nullptr) {
						//Send device structure
						//To minimize message size, every property as single message
						String deviceHRef = getMqttTopic() + "/things/"
								+ device->getId();
						WProperty *property = device->firstProperty;
						while (property != nullptr) {
							if (property->isVisible(MQTT)) {
								String topic = deviceHRef + "/properties/" + property->getId();
								WStringStream* response = getResponseStream();
								WJson* json = new WJson(response);
								property->toJsonStructure(json, "", deviceHRef);
								mqttClient->publish(topic.c_str(), response->c_str());
							}
							property = property->next;
						}
						device = device->next;
					}*/
					mqttClient->unsubscribe("devices/#");
				}
				//Subscribe to device specific topic
				mqttClient->subscribe(String(String(getMqttTopic()) + "/#").c_str());
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
				handleDeviceStateChange(device);
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
					device = device->next;
				}
				page->printAndReplace(FPSTR(HTTP_BUTTON), "firmware", "get", "Update firmware");
				page->printAndReplace(FPSTR(HTTP_BUTTON), "info", "get", "Info");
				page->printAndReplace(FPSTR(HTTP_BUTTON), "reset", "post", "Reset");
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
			WStringStream* page = new WStringStream(3072);
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

	void handleHttpNetworkConfiguration() {
		if (isWebServerRunning()) {
			wlog->notice(F("Network config page"));
			WStringStream* page = new WStringStream(3072);
			page->printAndReplace(FPSTR(HTTP_HEAD_BEGIN), "Network Configuration");
			page->print(FPSTR(HTTP_SCRIPT));
			page->print(FPSTR(HTTP_STYLE));
			page->print(FPSTR(HTTP_HEAD_END));
			printHttpCaption(page);
			page->printAndReplace(FPSTR(HTTP_PAGE_CONFIGURATION_STYLE), (this->isSupportingMqtt() ? "block" : "none"));
			page->printAndReplace(FPSTR(HTTP_PAGE_CONFIGURATION_GENERAL), getIdx(), getSsid(), getPassword());
			page->printAndReplace(FPSTR(HTTP_PAGE_CONFIGURATION_SERVICE), (this->isSupportingWebThing() ? "checked" : ""), (this->isSupportingMqtt() ? "checked" : ""));
			page->printAndReplace(FPSTR(HTTP_PAGE_CONFIGURATION_MQTT), getMqttServer(), getMqttUser(), getMqttPassword(), getMqttTopic());
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			delete page;
		}
	}

	void handleHttpSaveConfiguration() {
		if (isWebServerRunning()) {
			this->idx->setString(webServer->arg("i").c_str());
			this->ssid->setString(webServer->arg("s").c_str());
			settings->setString("password", webServer->arg("p").c_str());
			this->supportingWebThing->setBoolean(webServer->arg("wt") == "true");
			this->supportingMqtt->setBoolean(webServer->arg("mq") == "true");
			settings->setString("mqttServer", webServer->arg("ms").c_str());
			settings->setString("mqttUser", webServer->arg("mu").c_str());
			settings->setString("mqttPassword", webServer->arg("mp").c_str());
			this->mqttTopic->setString(webServer->arg("mt").c_str());
			if ((startWebServerAutomaticly) && (!isSupportingWebThing())
					&& ((!isSupportingMqtt()) || (strcmp(getMqttServer(), "") == 0)
							|| (strcmp(getMqttTopic(), "") == 0))) {
				//if mqqt is completely unspecified, activate webthings
				this->supportingWebThing->setBoolean(true);
			}
			settings->save();
			this->restart(F("Settings saved."));
		}
	}

	void handleHttpSaveDeviceConfiguration(WDevice *&device) {
		if (isWebServerRunning()) {
			wlog->notice(F("handleHttpSaveDeviceConfiguration "), device->getId());
			device->saveConfigPage(webServer);
			settings->save();
			delay(300);
			this->restart(F("Device settings saved."));
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
			page->print(this->getDeviceIpAsString());
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
			page->print("</table>");
			page->print(FPSTR(HTTP_BODY_END));
			webServer->send(200, TEXT_HTML, page->c_str());
			delete page;
		}
	}

	/** Handle the reset page */
	void handleHttpReset() {
		if (isWebServerRunning()) {
			this->restart(F("Resetting was caused manually by web interface. "));
		}
	}

	void printHttpCaption(WStringStream* page) {
		page->print("<h2>");
		page->print(applicationName);
		page->print("</h2><h3>Revision ");
		page->print(firmwareVersion);
		page->print(debug ? " (debug)" : "");
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
				this->restart(String(F("Update error: ")) + firmwareUpdateError);
			} else {
				this->restart(F("Update successful."));
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

	String getFirmwareUpdateErrorMessage() {
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

	void restart(String reasonMessage) {
		//webServer->send(302, TEXT_HTML, reasonMessage);
		this->restartFlag = reasonMessage;
		//Redirect
		webServer->sendHeader("Location", "/config", true);
		webServer->send(302, TEXT_PLAIN, "");
		//webServer->redirect("/config");
	}

	bool loadSettings() {
		this->idx = settings->registerString("idx", 32,	this->getClientName(true).c_str());
		this->ssid = settings->registerString("ssid", 32, "");
		settings->registerString("password", 64, "");
		this->supportingWebThing = settings->registerBoolean("supportingWebThing", true);
		this->supportingMqtt = settings->registerBoolean("supportingMqtt", false);
		settings->registerString("mqttServer", 32, "");
		settings->registerString("mqttUser", 32, "");
		settings->registerString("mqttPassword", 64, "");
		this->mqttTopic = settings->registerString("mqttTopic", 64, getIdx());
		bool settingsStored = settings->existsSettings();
		if (settingsStored) {
			if (getMqttTopic() == "") {
				this->mqttTopic->setString(this->getClientName(true).c_str());
			}
			if ((isSupportingMqtt()) && (this->mqttClient != nullptr)) {
				this->disconnectMqtt();
			}
			settingsStored = ((strcmp(getSsid(), "") != 0)
					&& (((isSupportingMqtt()) && (strcmp(getMqttServer(), "") != 0))
							|| (isSupportingWebThing())));
			if (settingsStored) {
				wlog->notice(F("Settings loaded successfully:"));
			} else {
				wlog->notice(F("Settings are not complete:"));
			}
			wlog->notice(F("SSID '%s'; MQTT enabled: %T; MQTT server '%s'; WebThings enabled: %T"),
								  getSsid(), isSupportingMqtt(), getMqttServer(), isSupportingWebThing());
		}
		EEPROM.end();
		return settingsStored;
	}

	void handleUnknown() {
		webServer->send(404);
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

		/*log("x >>>>");
		log("client: " + webServer->client().remoteIP().toString());
		log("x <<<<");*/
		/*if (webSocket = nullptr) {
			webSocket = new WebSocketsClient();
			webSocket->onEvent(std::bind(&WNetwork::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			String url = "ws://" + webServer->client().remoteIP().toString() + "/things/" + device->getId();
			webSocket->begin(url, 80, "/things/thermostat");
		}*/
	}



	/*void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

		switch(type) {
			case WStype_DISCONNECTED:
				log("[WSc] Disconnected!\n");
				break;
			case WStype_CONNECTED: {
				log("[WSc] Connected to url: " + String((char*) payload));

				// send message to server when Connected
				webSocket->sendTXT("Connected");
			}
				break;
			case WStype_TEXT:
				log("[WSc] get text: " + String((char*) payload));

				// send message to server
				// webSocket.sendTXT("message here");
				break;
			case WStype_BIN:
				log("[WSc] get binary length: " + String(length));
				hexdump(payload, length);

				// send data to server
				// webSocket.sendBIN(payload, length);
				break;
	        case WStype_PING:
	            // pong will be send automatically
	            log("[WSc] get ping\n");
	            break;
	        case WStype_PONG:
	            // answer to a ping we send
	            log("[WSc] get pong\n");
	            break;
	    }

	}*/

	void sendDeviceStructure(WDevice *&device) {
		wlog->notice(F("Send description for device: %s"), device->getId());
		WStringStream* response = getResponseStream();
		WJson json(response);
		device->toJsonStructure(&json, "", WEBTHING);
		webServer->send(200, APPLICATION_JSON, response->c_str());

		/*log(">>>>");
		log("client " + webServer->client().remoteIP().toString());
		log("<<<<");*/
		//WebSocketsClient* webSocket = new WebSocketsClient();

	}

	void sendDeviceValues(WDevice *&device) {
		wlog->notice(F("Send all properties for device: "), device->getId());
		WStringStream* response = getResponseStream();//request->beginResponseStream("application/json");
		WJson json(response);
		device->toJsonValues(&json, WEBTHING);
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

	void setPropertyValue(WDevice *device) {
		if (webServer->hasArg("plain") == false) {
			webServer->send(422);
			return;
		}
		WJsonParser parser;
		WProperty* property = parser.parse(device, webServer->arg("plain").c_str());
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

	/*void handleThingWebSocket(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght, WDevice* device) {
		log(">      +++++       WebSocket message is received");
		switch (type) {
	    case WStype_DISCONNECTED:             // if the websocket is disconnected
	    	Serial.printf("[%u] Disconnected!\n", num);
	    	break;
	    case WStype_CONNECTED: {              // if a new websocket connection is established
	        IPAddress ip = device->webSocket->remoteIP(num);
	        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
	        //rainbow = false;                  // Turn rainbow off when a new connection is established
	    }
	        break;
	    case WStype_TEXT:                     // if new text data is received
	    	Serial.printf("[%u] get Text: %s\n", num, payload);
	    	if (payload[0] == '#') {            // we get RGB data
	    		uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode rgb data
	    		int r = ((rgb >> 20) & 0x3FF);                     // 10 bits per color, so R: bits 20-29
	    		int g = ((rgb >> 10) & 0x3FF);                     // G: bits 10-19
	    		int b =          rgb & 0x3FF;                      // B: bits  0-9

	    		//analogWrite(LED_RED,   r);                         // write it to the LED output pins
	    		//analogWrite(LED_GREEN, g);
	    		//analogWrite(LED_BLUE,  b);
	    	} else if (payload[0] == 'R') {                      // the browser sends an R when the rainbow effect is enabled
	    		//rainbow = true;
	    	} else if (payload[0] == 'N') {                      // the browser sends an N when the rainbow effect is disabled
	    		//rainbow = false;
	    	}
	    	break;
		}
	}*/

	//ToDo
	/*
	void handleThingWebSocket(AwsEventType type, void *arg, uint8_t *rawData, size_t len, WDevice *device) {
		// Ignore all except data packets
		if (type != WS_EVT_DATA)
			return;
		// Only consider non fragmented data
		AwsFrameInfo *info = (AwsFrameInfo*) arg;
		if (!info->final || info->index != 0 || info->len != len)
			return;
		// Web Thing only specifies text, not binary websocket transfers
		if (info->opcode != WS_TEXT)
			return;
		// In theory we could just have one websocket for all Things and react on the server->url() to route data.
		// Controllers will however establish a separate websocket connection for each Thing anyway as of in the
		// spec. For now each Thing stores its own Websocket connection object therefore.
		// Parse request
		StaticJsonDocument<SIZE_MQTT_PACKET>* jsonDoc = getDynamicJsonDocument((char *) rawData);
		if (jsonDoc == nullptr) {
			sendErrorMsg(*jsonDoc, *client, 400, "Invalid json");
			return;
		}
		JsonObject json = jsonDoc->as<JsonObject>();
		String messageType = json["messageType"].as<String>();
		const JsonVariant &dataVariant = json["data"];
		if (!dataVariant.is<JsonObject>()) {
			sendErrorMsg(*jsonDoc, *client, 400, "data must be an object");
			return;
		}
		const JsonObject data = dataVariant.as<JsonObject>();
		if (messageType == "setProperty") {
			for (auto kv : data) {
				WProperty *property = device->getPropertyById(kv.key().c_str());
				if ((property != nullptr) && (property->isVisible(WEBTHING))) {
					JsonVariant newValue = json[property->getId()];
					property->setFromJson(newValue);
				}
			}
			jsonDoc->clear();
			//ToDo
			// Send confirmation by sending back the received property object
			String jsonStr;
			serializeJson(data, jsonStr);
			//data.printTo(jsonStr);
			client->text(jsonStr.c_str(), jsonStr.length());
		} else if (messageType == "requestAction") {
			jsonDoc->clear();
			sendErrorMsg(*jsonDoc, *client, 400, "Not supported yet");
		} else if (messageType == "addEventSubscription") {
			// We report back all property state changes. We'd require a map
			// of subscribed properties per websocket connection otherwise
			jsonDoc->clear();
		}
	}
	*/

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


			//Websocket
			/*String url = "wss://" + getHostName() + ".local/things/" + device->getId();
			device->webSocket = new WebSocketsServer(81, url, "webthing");
			device->webSocket->onEvent(std::bind(&WNetwork::handleThingWebSocket, this,
										std::placeholders::_1, std::placeholders::_2,
										std::placeholders::_3, std::placeholders::_4,
										device));

			device->webSocket->begin();*/
			//ToDo
			/*
			device->getWebSocket()->onEvent(
					std::bind(&WNetwork::handleThingWebSocket, this,
							std::placeholders::_1, std::placeholders::_2,
							std::placeholders::_3, std::placeholders::_4,
							std::placeholders::_5, std::placeholders::_6,
							device));
			webServer->addHandler(device->getWebSocket());
			*/
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

//#endif    // ESP

#endif
