#ifndef W_UNSIGNED_LONG_PROPERTY_H
#define W_UNSIGNED_LONG_PROPERTY_H

#include "WProperty.h"

class WUnsignedLongProperty: public WProperty {
public:
	WUnsignedLongProperty(const char* id)
	: WProperty(id, id, UNSIGNED_LONG) {
		this->atType = "LongProperty";
	}

protected:

private:

};

#endif
