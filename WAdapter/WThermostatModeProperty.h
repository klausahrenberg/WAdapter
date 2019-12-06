#ifndef W_THERMOSTAT_MODE_PROPERTY_H
#define W_THERMOSTAT_MODE_PROPERTY_H

#include "WProperty.h"

const char* THERMOSTAT_MODE_OFF = "off";
const char* THERMOSTAT_MODE_HEAT = "heat";
const char* THERMOSTAT_MODE_COOL = "cool";
const char* THERMOSTAT_MODE_AUTO = "auto";

class WThermostatModeProperty: public WProperty {
public:
	WThermostatModeProperty(const char* id, const char* title)
	: WProperty(id, title, STRING) {
		this->atType = "ThermostatModeProperty";
		this->setReadOnly(true);
		this->addEnumString(THERMOSTAT_MODE_OFF);
		this->addEnumString(THERMOSTAT_MODE_AUTO);
		//this->addEnumString(THERMOSTAT_MODE_HEAT);
		//this->addEnumString(THERMOSTAT_MODE_COOL);
	}


protected:

private:

};

#endif
