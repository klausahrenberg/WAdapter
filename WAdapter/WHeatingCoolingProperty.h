#ifndef W_HEATING_COOLING_PROPERTY_H
#define W_HEATING_COOLING_PROPERTY_H

#include "WProperty.h"

const String VALUE_OFF = "off";
const String VALUE_HEATING = "heating";
const String VALUE_COOLING = "cooling";

class WHeatingCoolingProperty: public WProperty {
public:
	WHeatingCoolingProperty(String id, String title, String description)
	: WProperty(id, title, description, STRING) {
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
