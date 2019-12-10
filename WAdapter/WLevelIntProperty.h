#ifndef W_LEVEL_INT_PROPERTY_H
#define W_LEVEL_INT_PROPERTY_H

#include "WProperty.h"

class WLevelIntProperty: public WProperty {
public:
	WLevelIntProperty(const char* id, const char* title, int minimum, int maximum)
	: WProperty(id, title, INTEGER) {
		this->atType = "LevelProperty";
		this->minimum = minimum;
		this->maximum = maximum;
	}

	int getMinimum() {
		return minimum;
	}

	void setMinimum(int minimum) {
		this->minimum = minimum;
	}

	int getMaximum() {
		return maximum;
	}

	void setMaximum(int maximum) {
		this->maximum = maximum;
	}

	void toJsonStructureAdditionalParameters(WJson* json) {
		json->propertyInteger("minimum", this->getMinimum());
		json->propertyInteger("maximum", this->getMaximum());
	}

protected:

private:
	int minimum, maximum;
};

#endif
