#ifndef W_TARGET_TEMPERATURE_PROPERTY_H
#define W_TARGET_TEMPERATURE_PROPERTY_H

#include "WProperty.h"

class WTargetTemperatureProperty: public WProperty {
public:
	WTargetTemperatureProperty(String id, String title, String description)
	: WProperty(id, title, description, DOUBLE) {
		this->atType = "TargetTemperatureProperty";
		this->setUnit("celsius");
	}


protected:

private:

};

#endif
