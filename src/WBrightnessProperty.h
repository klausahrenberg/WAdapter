#ifndef W_BRIGHTNESS_H
#define W_BRIGHTNESS_H

#include "WProperty.h"

class WBrightnessProperty: public WProperty {
public:
	WBrightnessProperty(const char* id, const char* title, byte minimum = 0, byte maximum = 100)
	: WProperty(id, title, BYTE, TYPE_BRIGHTNESS_PROPERTY) {
		_minimum = minimum;
		_maximum = maximum;
	}

	byte minimum() {
		return _minimum;
	}

	byte maximum() {
		return _maximum;
	}

	byte getScaledToMax0xFF() {
		int v = getByte() * 0xFF / maximum();
		return (byte) v;
	}

	void toJsonStructureAdditionalParameters(WJson* json) {
		json->propertyInteger("minimum", minimum());
		json->propertyInteger("maximum", maximum());
	}

protected:

private:
	byte _minimum, _maximum;
};

#endif
