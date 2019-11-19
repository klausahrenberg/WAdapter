#ifndef W_NETWORK_H
#define W_NETWORK_H

//#if defined(ESP32) || defined(ESP8266)

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
#include <ArduinoJson.h>
#include "WHtmlPages.h"
#include "WAdapterMqtt.h"
#include "ESPAsyncWebServer.h"
#include "WDevice.h"
#include "WLed.h"
#include "WSettings.h"

#define ESP_MAX_PUT_BODY_SIZE 512
#define SIZE_MQTT_PACKET 512
#define NO_LED -1
const String CONFIG_PASSWORD = "12345678";

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
WiFiClient wifiClient;
WAdapterMqtt *mqttClient;

class WNetwork {
public:
	typedef std::function<void(void)> THandlerFunction;
	WNetwork(bool debug, String applicationName, String firmwareVersion, bool startWebServerAutomaticly, int statusLedPin, int jsonBufferSize) {
		WiFi.mode(WIFI_STA);
		this->applicationName = applicationName;
		this->firmwareVersion = firmwareVersion;
		this->startWebServerAutomaticly = startWebServerAutomaticly;
		this->jsonBufferSize = jsonBufferSize;
		this->webServer = nullptr;
		this->dnsApServer = nullptr;
		this->debug = debug;
		this->updateRunning = false;
		this->restartFlag = "";
		this->deepSleepFlag = nullptr;
		this->deepSleepSeconds = 0;
		settings = new WSettings(debug);
		settingsFound = loadSettings();
		this->mqttClient = nullptr;
		lastMqttConnect = lastWifiConnect = 0;
		gotIpEventHandler =
				WiFi.onStationModeGotIP(
						[this](const WiFiEventStationModeGotIP& event) {
							log("Station connected, IP: " + this->getDeviceIpAsString());
							//Connect, if webThing supported and Wifi is connected as client
							if ((this->isSupportingWebThing()) && (isWifiConnected())) {
								this->startWebServer();
							}
							this->notify(false);
						});
		disconnectedEventHandler = WiFi.onStationModeDisconnected(
				[this](const WiFiEventStationModeDisconnected& event) {
					log("Station disconnected");
					this->disconnectMqtt();
					this->lastMqttConnect = 0;
					this->notify(false);
				});
		if (this->isSupportingMqtt()) {
			this->mqttClient = new WAdapterMqtt(debug, wifiClient, SIZE_MQTT_PACKET);
			mqttClient->setCallback(
				[this] (char* topic, byte* payload, unsigned int length) {
					this->mqttCallback(topic, payload, length);
				});
		}
		if (statusLedPin != NO_LED) {
			statusLed = new WLed(debug, statusLedPin);
			statusLed->setOn(true, 500);
		} else {
			statusLed = nullptr;
		}

	}

	//returns true, if no configuration mode and no own ap is opened
	bool loop(unsigned long now) {
		bool result = true;
		bool waitForWifiConnection = (deepSleepSeconds > 0);
		if ((!settingsFound) && (startWebServerAutomaticly)) {
			this->startWebServer();
		}
		if (!isWebServerRunning()) {
			if (!getSsid().equals("")) {
				//WiFi connection
				if ((WiFi.status() != WL_CONNECTED)
						&& ((lastWifiConnect == 0)
								|| (now - lastWifiConnect > 300000))) {
					log("Connecting to '" + getSsid() + "'");
					//Workaround: if disconnect is not called, WIFI connection fails after first startup
					WiFi.disconnect();
					String hostName = getIdx();
					hostName.replace(".", "-");
					hostName.replace(" ", "-");
					if (hostName.equals("")) {
						hostName = getClientName(false);
					}
					WiFi.hostname(hostName);
					WiFi.begin(getSsid().c_str(), getPassword().c_str());
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
			//webServer->handleClient();
			result = ((!isSoftAP()) && (!isUpdateRunning()));
		}
		//MQTT connection
		if ((isWifiConnected()) && (isSupportingMqtt())
				&& (!mqttClient->connected())
				&& ((lastMqttConnect == 0) || (now - lastMqttConnect > 300000)) && (!getMqttServer().equals(""))) {
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
		WDevice* device = firstDevice;
		while (device != nullptr) {
			device->loop(now);
			if ((this->isMqttConnected()) && (this->isSupportingMqtt())
					&& ((device->lastStateNotify == 0) || ((device->stateNotifyInterval > 0) && (now - device->lastStateNotify > device->stateNotifyInterval)))
					&& (device->isDeviceStateComplete())) {
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
			this->updateRunning = false;
			stopWebServer();
			delay(1000);
			ESP.restart();
			delay(2000);
		} else if (deepSleepFlag != nullptr) {
			if (deepSleepFlag->off()) {
				//Deep Sleep
				log("Go to deep sleep. Bye...");
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

	WSettings* getSettings() {
		return this->settings;
	}

	void setOnNotify(THandlerFunction onNotify) {
		this->onNotify = onNotify;
	}

	void setOnConfigurationFinished(THandlerFunction onConfigurationFinished) {
		this->onConfigurationFinished = onConfigurationFinished;
	}

	bool publishMqtt(String topic, JsonObject& json) {
		return publishMqttImpl(getMqttTopic() + (topic != "" ? "/" + topic : ""), json);
	}

	bool publishMqtt(String topic, String payload) {
		return publishMqttImpl(getMqttTopic() + (topic != "" ? "/" + topic : ""), payload);
	}

	// Creates a web server. If Wifi is not connected, then an own AP will be created
	void startWebServer() {
		if (!isWebServerRunning()) {
			String apSsid = getClientName(false);
			webServer = new AsyncWebServer(80);
			if (WiFi.status() != WL_CONNECTED) {
				//Create own AP
				log("Start AccessPoint for configuration. SSID '" + apSsid + "'; password '" + CONFIG_PASSWORD + "'");
				dnsApServer = new DNSServer();
				WiFi.softAP(apSsid.c_str(), CONFIG_PASSWORD.c_str());
				dnsApServer->setErrorReplyCode(DNSReplyCode::NoError);
				dnsApServer->start(53, "*", WiFi.softAPIP());
			} else {
				log(
						"Start web server for configuration. IP "
								+ this->getDeviceIpAsString());
			}
			webServer->onNotFound(std::bind(&WNetwork::handleUnknown, this, std::placeholders::_1));
			if ((WiFi.status() != WL_CONNECTED)	|| (!this->isSupportingWebThing())) {
				webServer->on("/", HTTP_GET,
						std::bind(&WNetwork::handleHttpRootRequest, this,
								std::placeholders::_1));
			}
			webServer->on("/config", HTTP_GET,
					std::bind(&WNetwork::handleHttpRootRequest, this,
							std::placeholders::_1));
			WDevice* device = this->firstDevice;
			while (device != nullptr) {
				if (device->isProvidingConfigPage()) {
					String deviceBase = "/device_" + device->getId();
					log("on " + deviceBase);
					webServer->on(deviceBase.c_str(), HTTP_GET, std::bind(&WNetwork::handleHttpDeviceConfiguration, this, std::placeholders::_1, device));
					webServer->on(String("/saveDeviceConfiguration_" + device->getId()).c_str(), HTTP_GET, std::bind(&WNetwork::handleHttpSaveDeviceConfiguration, this, std::placeholders::_1, device));
				}
				device = device->next;
			}
			webServer->on("/wifi", HTTP_GET,
					std::bind(&WNetwork::handleHttpNetworkConfiguration, this,
							std::placeholders::_1));
			webServer->on("/saveConfiguration", HTTP_GET,
					std::bind(&WNetwork::handleHttpSaveConfiguration, this,
							std::placeholders::_1));
			webServer->on("/info", HTTP_GET,
					std::bind(&WNetwork::handleHttpInfo, this,
							std::placeholders::_1));
			webServer->on("/reset", HTTP_ANY,
					std::bind(&WNetwork::handleHttpReset, this,
							std::placeholders::_1));

			//firmware update
			webServer->on("/firmware", HTTP_GET,
					std::bind(&WNetwork::handleHttpFirmwareUpdate, this,
							std::placeholders::_1));
			webServer->on("/firmware", HTTP_POST,
					std::bind(&WNetwork::handleHttpFirmwareUpdateFinished, this,
							std::placeholders::_1),
					std::bind(&WNetwork::handleHttpFirmwareUpdateProgress, this,
							std::placeholders::_1, std::placeholders::_2,
							std::placeholders::_3, std::placeholders::_4,
							std::placeholders::_5, std::placeholders::_6));

			//WebThings
			if ((this->isSupportingWebThing()) && (this->isWifiConnected())) {
				//Make the thing discoverable
				if (MDNS.begin(this->getDeviceIpAsString())) {
					//MDNS.addService("webthing", "tcp", 80);
					//MDNS.addServiceTxt("webthing", "tcp", "path", "/");
					MDNS.addService("http", "tcp", 80);
					MDNS.addServiceTxt("http", "tcp", "url", "http://" + this->getDeviceIpAsString()+ "/");
					MDNS.addServiceTxt("http", "tcp", "webthing", "true");
					log("MDNS responder started at " + this->getDeviceIpAsString());
				}
				DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin",	"*");
				DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "PUT, GET, OPTIONS");
				webServer->on("/", HTTP_GET, std::bind(&WNetwork::sendDescriptionOfDevices, this, std::placeholders::_1));
				WDevice* device = this->firstDevice;
				log("devices...");
				while (device != nullptr) {
					bindWebServerCalls(device);
					device = device->next;
				}
				log("devices finished.");
			}
			//Start http server
			webServer->begin();
			log("webServer started.");
			this->notify(true);
		}
	}

	void stopWebServer() {
		if ((isWebServerRunning()) && (!this->isSupportingWebThing()) && (!this->updateRunning)) {
			log("Close web configuration.");
			delay(100);
			//apServer->client().stop();
			//webServer->stop();
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
		return ((this->isSupportingMqtt()) && (this->mqttClient != nullptr) && (this->mqttClient->connected()));
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

	String getIdx() {
		return this->idx->getString();
	}

	String getSsid() {
		return this->ssid->getString();
	}

	String getPassword() {
		return settings->getString("password");
	}

	String getMqttServer() {
		return settings->getString("mqttServer");
	}

	String getMqttTopic() {
		return this->mqttTopic->getString();
	}

	String getMqttUser() {
		return settings->getString("mqttUser");
	}

	String getMqttPassword() {
		return settings->getString("mqttPassword");
	}

	void addDevice(WDevice* device) {
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
		AsyncWebSocket* webSocket = new AsyncWebSocket("/things/" + device->getId());
		device->setWebSocket(webSocket);
		bindWebServerCalls(device);
	}

	void setDeepSleepSeconds(int dsp) {
		this->deepSleepSeconds = dsp;
	}

	DynamicJsonDocument* getJsonDocument() {
		if (jsonDocument == nullptr) {
			jsonDocument = new DynamicJsonDocument(jsonBufferSize);
		}
		jsonDocument->clear();
		return jsonDocument;
	}

private:
	WDevice* firstDevice = nullptr;
	WDevice* lastDevice = nullptr;
	THandlerFunction onNotify;
	THandlerFunction onConfigurationFinished;
	bool debug, updateRunning, startWebServerAutomaticly;
	int jsonBufferSize;
	String restartFlag;
	DNSServer *dnsApServer;
	AsyncWebServer *webServer;
	int networkState;
	String applicationName, firmwareVersion;
	String firmwareUpdateError;
	WProperty* supportingWebThing;
	WProperty* supportingMqtt;
	WProperty* ssid;
	WProperty* idx;
	WProperty* mqttTopic;
	WAdapterMqtt *mqttClient;
	long lastMqttConnect, lastWifiConnect;
	char body_data[ESP_MAX_PUT_BODY_SIZE];bool b_has_body_data = false;
	DynamicJsonDocument* jsonDocument = nullptr;
	WLed* statusLed;
	WSettings* settings;
	bool settingsFound;
	WDevice* deepSleepFlag;
	int deepSleepSeconds;

	void log(String debugMessage) {
		if (debug) {
			Serial.println(debugMessage);
		}
	}

	void handleDeviceStateChange(WDevice* device) {
		String topic = getMqttTopic() + "/things/" + device->getId() + "/properties";
		mqttSendDeviceState(topic, device);
	}

	void mqttSendDeviceState(String topic, WDevice* device) {
		if ((this->isMqttConnected()) && (isSupportingMqtt()) && (device->isDeviceStateComplete())) {
			log("Send actual device state via MQTT");
			DynamicJsonDocument* jsonDocument = getJsonDocument();
			JsonObject json = jsonDocument->to<JsonObject>();
			json["idx"] = this->getIdx();
			json["ip"] = this->getDeviceIpAsString();
			json["firmware"] = this->firmwareVersion;
			json["webServerRunning"] = this->isWebServerRunning();
			JsonObject& refJson = json;
			device->toJson(refJson);
			device->lastStateNotify = millis();
			this->publishMqttImpl(topic, refJson);

			if ((deepSleepSeconds > 0)
					&& ((!this->isSupportingWebThing()) || (device->areAllPropertiesRequested()))) {
				deepSleepFlag = device;
			}
		}
	}

	void mqttCallback(char* ptopic, byte* payload, unsigned int length) {
		//create character buffer with ending null terminator (string)
		char message_buff[this->mqttClient->getMaxPacketSize()];
		for (unsigned int i = 0; i < length; i++) {
			message_buff[i] = payload[i];
		}
		message_buff[length] = '\0';
		//forward to serial port
		log("Received MQTT callback: " + String(ptopic) + "/{" + String(message_buff) + "}");
		String topic = String(ptopic).substring(getMqttTopic().length() + 1);
		log("Topic short '" + topic + "'");
		if (topic.startsWith("things/")) {
			topic = topic.substring(String("things/").length());
			int i = topic.indexOf("/");
			if (i > -1) {
				String deviceId = topic.substring(0, i);
				log("look for device id '" + deviceId +"'");
				WDevice* device = this->getDeviceById(deviceId);
				if (device != nullptr) {
					topic = topic.substring(i + 1);
					if (topic.startsWith("properties")) {
						topic = topic.substring(String("properties").length() + 1);
						if (topic.equals("")) {
							if (length > 0) {
								DynamicJsonDocument* jsonDocument = getJsonDocument();
								auto error = deserializeJson(*jsonDocument, payload);
								if (error) {
									//If payload is not parseable, send device state
									mqttSendDeviceState(String(ptopic), device);
								} else {
									JsonObject json = jsonDocument->as<JsonObject>();
									for (JsonPair item : json) {
										WProperty* property = device->getPropertyById(String(item.key().c_str()));
										if ((property != nullptr) && (property->isSupportingMqtt())) {
											log("set " + property->getId() + " to " + item.value().as<String>());
											property->setFromJson(item.value());
										}
										//setProperty(device->getPropertyById(String(item.key)), &item.value);
									}
								}
							} else {
								//If payload is empty, send device state
								mqttSendDeviceState(String(ptopic), device);
							}
						} else {
							WProperty* property = device->getPropertyById(topic);
							if ((property != nullptr) && (property->isSupportingMqtt())) {
								DynamicJsonDocument doc(32);
								JsonVariant value = doc.to<JsonVariant>();
								value.set(message_buff);
								//JsonVariant value = message_buff;
								property->setFromJson(value);
							}
							//JsonVariant value = message_buff;
							//setProperty(device->getPropertyById(topic), &value);
						}
					}
				}
			}
		} else if (topic.equals("webServer")) {
			enableWebServer(String(message_buff).equals("true"));
		}
	}

	bool publishMqttImpl(String absolutePath, JsonObject& json) {
		if (this->isSupportingMqtt()) {
			if (isMqttConnected()) {
				//char payloadBuffer[this->mqttClient->getMaxPacketSize()];
				char payloadBuffer[SIZE_MQTT_PACKET];
				serializeJson(json, payloadBuffer);
				//json.printTo(payloadBuffer, sizeof(payloadBuffer));
				if (mqttClient->publish(absolutePath.c_str(), payloadBuffer)) {
					return true;
				} else {
					log("Sending MQTT message failed, rc=" + String(mqttClient->state()));
					this->disconnectMqtt();
					return false;
				}
			} else {
				if (!getMqttServer().equals("")) {
					log("Can't send MQTT. Not connected to server: " + getMqttServer());
				}
				return false;
			}
		}
	}

	bool publishMqttImpl(String absolutePath, String payload) {
		if (this->isSupportingMqtt()) {
			if (isMqttConnected()) {
				char payloadBuffer[this->mqttClient->getMaxPacketSize()];
				payload.toCharArray(payloadBuffer, sizeof(payloadBuffer));
				if (mqttClient->publish(absolutePath.c_str(), payloadBuffer)) {
					return true;
				} else {
					log("Sending MQTT message failed, rc=" + String(mqttClient->state()));
					this->disconnectMqtt();
					return false;
				}
			} else {
				if (!getMqttServer().equals("")) {
					log("Can't send MQTT. Not connected to server: " + getMqttServer());
				}
				return false;
			}
		}
	}

	bool mqttReconnect() {
		if (this->isSupportingMqtt()) {
			log("Connect to MQTT server: " + getMqttServer() + "; user: '" + getMqttUser() + "'; password: '" + getMqttPassword() + "'; clientName: '" + getClientName(true) + "'");
			// Attempt to connect
			this->mqttClient->setServer(getMqttServer(), 1883);
			if (mqttClient->connect(getClientName(true).c_str(),
					getMqttUser().c_str(), //(mqttUser != "" ? mqttUser.c_str() : NULL),
					getMqttPassword().c_str())) { //(mqttPassword != "" ? mqttPassword.c_str() : NULL))) {
				log("Connected to MQTT server.");
				if (this->deepSleepSeconds == 0) {
					//Send device structure and status
					mqttClient->subscribe("devices/#");
					WDevice* device = this->firstDevice;
					while (device != nullptr) {
						//Send device structure
						//To minimize message size, every property as single message
						String deviceHRef = getMqttTopic() + "/things/" + device->getId();
						WProperty* property = device->firstProperty;
						while (property != nullptr) {
							if (property->isSupportingMqtt()) {
								DynamicJsonDocument* jsonDocument = getJsonDocument();
								JsonObject prop = jsonDocument->to<JsonObject>();
								JsonObject& refProp = prop;
								//JsonObject& prop = this->getJsonBuffer()->createObject();
					    		String topic = property->structToJson(refProp, deviceHRef, "topic");
					    		publishMqttImpl("devices/" + topic , refProp);
							}
					    	property = property->next;
						}
						device = device->next;
					}
					mqttClient->unsubscribe("devices/#");
				}
				//Subscribe to device specific topic
				mqttClient->subscribe(String(getMqttTopic() + "/#").c_str());
				notify(false);
				return true;
			} else {
				log("Connection to MQTT server failed, rc=" + String(mqttClient->state()));
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
			WDevice* device = this->firstDevice;
			while (device != nullptr) {
				handleDeviceStateChange(device);
				device = device->next;
			}
		}
		if (onNotify) {
			onNotify();
		}
	}

	void handleHttpRootRequest(AsyncWebServerRequest *request) {
		if (isWebServerRunning()) {
			if (restartFlag.equals("")) {
				String page = FPSTR(HTTP_HEAD_BEGIN);
				page.replace("{v}", applicationName);
				page += FPSTR(HTTP_SCRIPT);
				page += FPSTR(HTTP_STYLE);
				//page += _customHeadElement;
				page += FPSTR(HTTP_HEAD_END);
				page += getHttpCaption();
				WDevice* device = firstDevice;
				while (device != nullptr) {
					if (device->isProvidingConfigPage()) {
						page += FPSTR(HTTP_BUTTON_DEVICE);
						page.replace("{di}", device->getId());
						page.replace("{dn}", device->getName());
					}
					device = device->next;
				}
				page += FPSTR(HTTP_PAGE_ROOT);
				page += FPSTR(HTTP_BODY_END);
				request->send(200, "text/html", page);
			} else {
				String page = FPSTR(HTTP_HEAD_BEGIN);
				page.replace("{v}", F("Info"));
				page += FPSTR(HTTP_SCRIPT);
				page += FPSTR(HTTP_STYLE);
				page += F("<meta http-equiv=\"refresh\" content=\"10\">");
				page += FPSTR(HTTP_HEAD_END);
				page += restartFlag;
				page += F("<br><br>");
				page += F("Module will reset in a few seconds...");
				page += FPSTR(HTTP_BODY_END);
				request->send(200, "text/html", page);
			}
		}
	}

	void handleHttpDeviceConfiguration(AsyncWebServerRequest *request, WDevice*& device) {
		if (isWebServerRunning()) {
			log("Device config page");
			String page = FPSTR(HTTP_HEAD_BEGIN);
			page.replace("{v}", "Device Configuration");
			page += FPSTR(HTTP_SCRIPT);
			page += FPSTR(HTTP_STYLE);
			page += FPSTR(HTTP_HEAD_END);
			page += getHttpCaption();
			page += device->getConfigPage();
			page += FPSTR(HTTP_BODY_END);
			request->send(200, "text/html", page);
		}

	}

	void handleHttpNetworkConfiguration(AsyncWebServerRequest *request) {
		if (isWebServerRunning()) {
			log("Network config page");
			String page = FPSTR(HTTP_HEAD_BEGIN);
			page.replace("{v}", "Network Configuration");
			page += FPSTR(HTTP_SCRIPT);
			page += FPSTR(HTTP_STYLE);
			page += FPSTR(HTTP_HEAD_END);
			page += getHttpCaption();
			page += FPSTR(HTTP_PAGE_CONFIGURATION);
			page += FPSTR(HTTP_BODY_END);

			page.replace("{i}", getIdx());
			page.replace("{s}", getSsid());
			page.replace("{p}", getPassword());
			page.replace("{wt}", (this->isSupportingWebThing() ? "checked" : ""));
			page.replace("{mq}", (this->isSupportingMqtt() ? "checked" : ""));
			page.replace("{mqg}", (this->isSupportingMqtt() ? "block" : "none"));
			page.replace("{ms}", getMqttServer());
			page.replace("{mu}", getMqttUser());
			page.replace("{mp}", getMqttPassword());
			page.replace("{mt}", getMqttTopic());
			request->send(200, "text/html", page);
		}
	}

	void handleHttpSaveConfiguration(AsyncWebServerRequest *request) {
		if (isWebServerRunning()) {
			this->idx->setString(request->arg("i"));
			this->ssid->setString(request->arg("s"));
			settings->setString("password", request->arg("p"));
			this->supportingWebThing->setBoolean(request->arg("wt") == "true");
			this->supportingMqtt->setBoolean(request->arg("mq") == "true");
			settings->setString("mqttServer", request->arg("ms"));
			settings->setString("mqttUser", request->arg("mu"));
			settings->setString("mqttPassword", request->arg("mp"));
			this->mqttTopic->setString(request->arg("mt"));
			if ((startWebServerAutomaticly) && (!isSupportingWebThing()) &&
			    ((!isSupportingMqtt()) || (getMqttServer().equals("")) || (getMqttTopic().equals("")))) {
				//if mqqt is completely unspecified, activate webthings
				this->supportingWebThing->setBoolean(true);
			}
			this->saveSettings();
			this->restart(request, F("Settings saved."));
		}
	}

	void handleHttpSaveDeviceConfiguration(AsyncWebServerRequest *request, WDevice*& device) {
		if (isWebServerRunning()) {
			log("handleHttpSaveDeviceConfiguration " + device->getId());
			device->saveConfigPage(request);
			this->saveSettings();
			this->restart(request, F("Device settings saved."));
		}
	}

	void handleHttpInfo(AsyncWebServerRequest *request) {
		if (isWebServerRunning()) {
			String page = FPSTR(HTTP_HEAD_BEGIN);
			page.replace("{v}", "Info");
			page += FPSTR(HTTP_SCRIPT);
			page += FPSTR(HTTP_STYLE);
			page += FPSTR(HTTP_HEAD_END);
			page += getHttpCaption();
			page += "<table>";
			page += "<tr><th>Chip ID:</th><td>";
			page += ESP.getChipId();
			page += "</td></tr>";
			page += "<tr><th>Flash Chip ID:</th><td>";
			page += ESP.getFlashChipId();
			page += "</td></tr>";
			page += "<tr><th>IDE Flash Size:</th><td>";
			page += ESP.getFlashChipSize();
			page += "</td></tr>";
			page += "<tr><th>Real Flash Size:</th><td>";
			page += ESP.getFlashChipRealSize();
			page += "</td></tr>";
			page += "<tr><th>IP address:</th><td>";
			page += this->getDeviceIpAsString();
			page += "</td></tr>";
			page += "<tr><th>MAC address:</th><td>";
			page += WiFi.macAddress();
			page += "</td></tr>";
			page += "<tr><th>Current sketch size:</th><td>";
			page += ESP.getSketchSize();
			page += "</td></tr>";
			page += "<tr><th>Available sketch size:</th><td>";
			page += ESP.getFreeSketchSpace();
			page += "</td></tr>";
			page += "<tr><th>EEPROM size:</th><td>";
			page += EEPROM.length();
			page += "</td></tr>";
			page += "</table>";
			page += FPSTR(HTTP_BODY_END);
			request->send(200, "text/html", page);
		}
	}

	/** Handle the reset page */
	void handleHttpReset(AsyncWebServerRequest *request) {
		if (isWebServerRunning()) {
			this->restart(request,
					F("Resetting was caused manually by web interface. "));
		}
	}

	String getHttpCaption() {
		return "<h2>" + applicationName + "</h2><h3>Revision " + firmwareVersion
				+ (debug ? " (debug)" : "") + "</h3>";
	}

	String getClientName(bool lowerCase) {
		String result = (applicationName.equals("") ? "ESP" : applicationName);
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

	void handleHttpFirmwareUpdate(AsyncWebServerRequest *request) {
		if (isWebServerRunning()) {
			String page = FPSTR(HTTP_HEAD_BEGIN);
			page.replace("{v}", "Firmware update");
			page += FPSTR(HTTP_SCRIPT);
			page += FPSTR(HTTP_STYLE);
			page += FPSTR(HTTP_HEAD_END);
			page += getHttpCaption();
			page += FPSTR(HTTP_FORM_FIRMWARE);
			page += FPSTR(HTTP_BODY_END);
			request->send(200, "text/html", page);
		}
	}

	void handleHttpFirmwareUpdateFinished(AsyncWebServerRequest *request) {
		if (isWebServerRunning()) {
			if (Update.hasError()) {
				this->restart(request,
						String(F("Update error: ")) + firmwareUpdateError);
			} else {
				this->restart(request, F("Update successful."));
			}
		}
	}

	void handleHttpFirmwareUpdateProgress(AsyncWebServerRequest *request,
			String filename, size_t index, uint8_t *data, size_t len,
			bool final) {
		if (isWebServerRunning()) {
			//Start firmwareUpdate
			this->updateRunning = true;
			//Close existing MQTT connections
			this->disconnectMqtt();
			//(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
			uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000)
					& 0xFFFFF000;
			if (!index) {
				firmwareUpdateError = "";
				log("Update starting");
				Update.runAsync(true);
				if (!Update.begin(free_space)) {
					setFirmwareUpdateError();
					//Update.printError(Serial);
				}
			}

			if (Update.write(data, len) != len) {
				setFirmwareUpdateError();
				//Update.printError(Serial);
			}

			if (final) {
				if (!Update.end(true)) {
					setFirmwareUpdateError();
				} else {
					log("Update complete.");
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

	void setFirmwareUpdateError() {
		firmwareUpdateError = getFirmwareUpdateErrorMessage();
		log(firmwareUpdateError);
	}

	void restart(AsyncWebServerRequest *request, String reasonMessage) {
		this->restartFlag = reasonMessage;
		request->redirect("/config");
	}

	bool loadSettings() {
		this->idx = settings->registerString("idx", 32, this->getClientName(true));
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
			if (getMqttTopic().equals("")) {
				this->mqttTopic->setString(this->getClientName(true));
			}
			if ((isSupportingMqtt()) && (this->mqttClient != nullptr)) {
				this->disconnectMqtt();
			}
			settingsStored = ((!getSsid().equals("")) && (((isSupportingMqtt()) && (!getMqttServer().equals(""))) || (isSupportingWebThing())));
			if (settingsStored) {
				log("Settings loaded successfully:");
			} else {
				log("Settings are not complete:");
			}
			log("SSID '" + getSsid() +
					"'; MQTT is " + (isSupportingMqtt() ? " enabled; MQTT server '" + getMqttServer() + "'" : "disabled") +
					"; Mozilla WebThings is " + (isSupportingWebThing() ? "enabled" : "disabled"));
		}
		EEPROM.end();
		return settingsStored;
	}

	void saveSettings() {
		settings->save();
	}

	void handleUnknown(AsyncWebServerRequest *request) {
		request->send(404);
	}

	void sendDescriptionOfDevices(AsyncWebServerRequest *request) {
		log("Send description for all devices... ");
		AsyncResponseStream* response = request->beginResponseStream("application/json");
		DynamicJsonDocument* json = getJsonDocument();
		//JsonArray things = json.createNestedArray();
		//JsonArray things = jsonBuffer.as<JsonArray>();
		//JsonArray& things = this->getJsonBuffer()->createArray();
		WDevice* device = this->firstDevice;
		while (device != nullptr) {
			JsonObject jsonDevice = json->createNestedObject();
			JsonObject& refDevice = jsonDevice;
			device->structToJson(refDevice, "");
			device = device->next;
		}
		serializeJson(*json, *response);
		//things.printTo(*response);
		request->send(response);
		log("Send description for all devices finished. ");
	}

	void sendDescriptionOfDevice(AsyncWebServerRequest *request, WDevice*& device) {
		log("Send description for device: " + device->getId());
		AsyncResponseStream *response = request->beginResponseStream("application/json");
		DynamicJsonDocument* json = getJsonDocument();
		JsonObject jsonDevice = json->to<JsonObject>();
		JsonObject& refDevice = jsonDevice;
		device->structToJson(refDevice, "");
		serializeJson(*json, *response);
		//descr.printTo(*response);
		request->send(response);
	}

	void getPropertyValue(AsyncWebServerRequest *request,	WProperty* property) {
		log("Send value of property: " + property->getId());
		AsyncResponseStream* response = request->beginResponseStream("application/json");
		DynamicJsonDocument* jsonDocument = getJsonDocument();
		JsonObject json = jsonDocument->to<JsonObject>();
		JsonObject& refJson = json;
		property->toJson(refJson);
		property->setRequested(true);
		serializeJson(*jsonDocument, *response);
		//json.printTo(*response);
		request->send(response);

		if (deepSleepSeconds > 0) {
			WDevice* device = firstDevice;
			while ((device != nullptr) && (deepSleepFlag == nullptr)) {
				if ((!this->isSupportingWebThing()) || (device->areAllPropertiesRequested())) {
					deepSleepFlag = device;
				}
				device = device->next;
			}
		}

	}

	void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
		if (total >= ESP_MAX_PUT_BODY_SIZE	|| index + len >= ESP_MAX_PUT_BODY_SIZE) {
			return; // cannot store this size..
		}
		// copy to internal buffer
		memcpy(&body_data[index], data, len);
		b_has_body_data = true;
	}

	void setPropertyValue(AsyncWebServerRequest *request, WProperty* property) {
		if (!b_has_body_data) {
			request->send(422); // unprocessable entity (b/c no body)
			return;
		}
		DynamicJsonDocument* jsonDocument = getJsonDocument();
		auto error = deserializeJson(*jsonDocument, body_data);
		if (error) {
			// unable to parse json
			request->send(500);
		} else {
			JsonObject newProp = jsonDocument->as<JsonObject>();
			JsonVariant newValue = newProp[property->getId()];
			property->setFromJson(newValue);
			AsyncResponseStream *response = request->beginResponseStream("application/json");
			serializeJson(*jsonDocument, *response);
			//newProp.printTo(*response);
			request->send(response);
			//request->send(200);
		}
		b_has_body_data = false;
		memset(body_data, 0, sizeof(body_data));
	}

	void sendErrorMsg(DynamicJsonDocument prop, AsyncWebSocketClient& client, int status, const char* msg) {
	    //JsonObject& prop = buffer.createObject();
	    prop["error"] = msg;
	    prop["status"] = status;
	    String jsonStr;
	    serializeJson(prop, jsonStr);
	    //prop.printTo(jsonStr);
	    client.text(jsonStr.c_str(), jsonStr.length());
	}

	void handleThingWebSocket(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *rawData, size_t len, WDevice* device) {
	    // Ignore all except data packets
	    if(type != WS_EVT_DATA) return;
	    // Only consider non fragmented data
	    AwsFrameInfo * info = (AwsFrameInfo*)arg;
	    if(!info->final || info->index != 0 || info->len != len) return;
	    // Web Thing only specifies text, not binary websocket transfers
	    if(info->opcode != WS_TEXT) return;
	    // In theory we could just have one websocket for all Things and react on the server->url() to route data.
	    // Controllers will however establish a separate websocket connection for each Thing anyway as of in the
	    // spec. For now each Thing stores its own Websocket connection object therefore.
	    // Parse request
	    DynamicJsonDocument* jsonDocument = getJsonDocument();
	    DeserializationError error = deserializeJson(*jsonDocument, rawData);
	    if (error) {
	    	sendErrorMsg(*jsonDocument, *client, 400, "Invalid json");
	    	return;
	    }
	    JsonObject newProp = jsonDocument->as<JsonObject>();
	    String messageType = newProp["messageType"].as<String>();
	    const JsonVariant& dataVariant = newProp["data"];
	    if (!dataVariant.is<JsonObject>()) {
	    	sendErrorMsg(*jsonDocument, *client, 400, "data must be an object");
	    	return;
	    }
	    const JsonObject data = dataVariant.as<JsonObject>();
	    if (messageType == "setProperty") {
	    	for (auto kv : data) {
	    		WProperty* property = device->getPropertyById(kv.key().c_str());
	    		if ((property != nullptr) && (property->isSupportingWebthing())) {
	    			JsonVariant newValue = newProp[property->getId()];
	    			property->setFromJson(newValue);
	    			//setProperty(property, data[property->getId().c_str()]);
	    		}
	    	}
	    	// Send confirmation by sending back the received property object
	    	String jsonStr;
	    	serializeJson(data, jsonStr);
	    	//data.printTo(jsonStr);
	    	client->text(jsonStr.c_str(), jsonStr.length());
	    } else if (messageType == "requestAction") {
	    	sendErrorMsg(*jsonDocument, *client, 400, "Not supported yet");
	    } else if (messageType == "addEventSubscription") {
	    	// We report back all property state changes. We'd require a map
	    	// of subscribed properties per websocket connection otherwise
	    }
	}

	void bindWebServerCalls(WDevice* device) {
		if (this->isWebServerRunning()) {
			String deviceBase = "/things/" + device->getId();
			WProperty* property = device->firstProperty;
			while (property != nullptr) {
				if (property->isSupportingWebthing()) {
					String propertyBase = deviceBase + "/properties/" + property->getId();
					webServer->on(propertyBase.c_str(), HTTP_GET, std::bind(&WNetwork::getPropertyValue, this, std::placeholders::_1, property));
					webServer->on(propertyBase.c_str(), HTTP_PUT, std::bind(&WNetwork::setPropertyValue, this, std::placeholders::_1, property),
								NULL,
								std::bind(&WNetwork::handleBody, this,
										std::placeholders::_1,
										std::placeholders::_2,
										std::placeholders::_3,
										std::placeholders::_4,
										std::placeholders::_5));
				}
				property = property->next;
			}
			webServer->on(deviceBase.c_str(), HTTP_GET, std::bind(&WNetwork::sendDescriptionOfDevice, this, std::placeholders::_1, device));

			device->getWebSocket()->onEvent(std::bind(&WNetwork::handleThingWebSocket, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, device));
			webServer->addHandler(device->getWebSocket());
		}
	}

	WDevice* getDeviceById(String deviceId) {
		WDevice* device = this->firstDevice;
		while (device != nullptr) {
			if (device->getId().equals(deviceId)) {
				return device;
			}
		}
		return nullptr;
	}

};

//#endif    // ESP

#endif
