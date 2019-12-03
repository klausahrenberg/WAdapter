#ifndef W_TARGET_TEMPERATURE_PROPERTY_H
#define W_TARGET_TEMPERATURE_PROPERTY_H

#include "WProperty.h"

class WTargetTemperatureProperty: public WProperty {
public:
	WTargetTemperatureProperty(const char* id, const char* title)
	: WProperty(id, title, DOUBLE) {
		this->atType = "TargetTemperatureProperty";
		this->setUnit("celsius");
	}


protected:

private:

};

#endif
