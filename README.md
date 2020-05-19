# WAdapter

Network management tools for ESP8266 programming
* Supports devices and properties
* Devices can be configured via Web-Interface
* Network connection is managed by this library
* Support for Mozilla Webthing and MQTT can be choosen
* Changes on properties trigger Webthing and/or MQTT update messages automaticly

## Version 1.10
* `WDevice.h`: Configuration pages removed. Handled over pages in WNetwork.h, devices should add configuration pages over `network->addCustomPage(WPage *customPage)`
* `WNetwork.h`: Integration of custom pages
* `WPage.h`: added (thanks @fashberg)

