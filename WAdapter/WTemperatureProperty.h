#ifndef W_TEMPERATURE_PROPERTY_H
#define W_TEMPERATURE_PROPERTY_H

#include "WProperty.h"

class WTemperatureProperty: public WProperty {
public:
	WTemperatureProperty(String id, String title, String description)
	: WProperty(id, title, description, DOUBLE) {
		this->atType = "TemperatureProperty";
		this->setUnit("celsius");
	}


protected:

private:

};

#endif
