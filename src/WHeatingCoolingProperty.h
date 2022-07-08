#ifndef W_HEATING_COOLING_PROPERTY_H
#define W_HEATING_COOLING_PROPERTY_H

#include "WProperty.h"

const char* VALUE_OFF = "off";
const char* VALUE_HEATING = "heating";
const char* VALUE_COOLING = "cooling";

class WHeatingCoolingProperty: public WProperty {
public:
	WHeatingCoolingProperty(const char* id, const char* title)
	: WProperty(id, title, STRING) {
		this->atType = "HeatingCoolingProperty";
		this->setReadOnly(true);
		this->addEnumString(VALUE_OFF);
		this->addEnumString(VALUE_HEATING);
		this->addEnumString(VALUE_COOLING);
	}


protected:

private:

};

#endif
