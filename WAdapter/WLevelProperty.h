#ifndef W_LEVEL_PROPERTY_H
#define W_LEVEL_PROPERTY_H

#include "WProperty.h"

class WLevelProperty: public WProperty {
public:
	WLevelProperty(const char* id, const char* title, double minimum, double maximum)
	: WProperty(id, title, DOUBLE) {
		this->atType = "LevelProperty";
		this->minimum = minimum;
		this->maximum = maximum;
	}

	double getMinimum() {
		return minimum;
	}

	void setMinimum(double minimum) {
		this->minimum = minimum;
	}

	double getMaximum() {
		return maximum;
	}

	void setMaximum(double maximum) {
		this->maximum = maximum;
	}

	void toJsonStructureAdditionalParameters(WJson* json) {
		json->propertyDouble("minimum", this->getMinimum());
		json->propertyDouble("maximum", this->getMaximum());
	}

protected:

private:
	double minimum, maximum;
};

#endif
