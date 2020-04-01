#ifndef W_LONG_PROPERTY_H
#define W_LONG_PROPERTY_H

#include "WProperty.h"

class WLongProperty: public WProperty {
public:
	WLongProperty(const char* id, const char* title)
	: WProperty(id, title, LONG) {
		this->atType = "LongProperty";
	}

protected:

private:

};

#endif
