#ifndef W_INTEGER_PROPERTY_H
#define W_INTEGER_PROPERTY_H

#include "WProperty.h"

class WIntegerProperty: public WProperty {
public:
	WIntegerProperty(String id, String title, String description)
	: WProperty(id, title, description, INTEGER) {
		this->atType = "IntegerProperty";
	}

protected:

private:

};

#endif
