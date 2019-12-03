#ifndef W_INTEGER_PROPERTY_H
#define W_INTEGER_PROPERTY_H

#include "WProperty.h"

class WIntegerProperty: public WProperty {
public:
	WIntegerProperty(const char* id, const char* title)
	: WProperty(id, title, INTEGER) {
		this->atType = "IntegerProperty";
	}

protected:

private:

};

#endif
