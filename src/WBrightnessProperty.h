#ifndef W_BRIGHTNESS_H
#define W_BRIGHTNESS_H

#include "WProperty.h"

class WBrightnessProperty: public WProperty {
public:
	WBrightnessProperty(const char* id, const char* title)
	: WProperty(id, title, BYTE, TYPE_BRIGHTNESS_PROPERTY) {
		this->minimum = 0;
		this->maximum = 100;
	}

	byte getMinimum() {
		return minimum;
	}

	byte getMaximum() {
		return maximum;
	}

	byte getScaledToMax0xFF() {
		int v = getByte() * 0xFF / 100;
		return (byte) v;
	}

	void toJsonStructureAdditionalParameters(WJson* json) {
		json->propertyInteger("minimum", this->getMinimum());
		json->propertyInteger("maximum", this->getMaximum());
	}

protected:

private:
	byte minimum, maximum;
};
#endif
