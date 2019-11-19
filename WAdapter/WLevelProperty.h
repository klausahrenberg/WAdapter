#ifndef W_LEVEL_PROPERTY_H
#define W_LEVEL_PROPERTY_H

#include "WProperty.h"

class WLevelProperty: public WProperty {
public:
	WLevelProperty(String id, String title, String description, double minimum, double maximum)
	: WProperty(id, title, description, DOUBLE) {
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

	String structToJson(JsonObject& json, String deviceHRef, String nameHRef) {
		String result = this->WProperty::structToJson(json, deviceHRef, nameHRef);
		json["minimum"] = this->getMinimum();
		json["maximum"] = this->getMaximum();
		return result;
	}

protected:

private:
	double minimum, maximum;
};

#endif
