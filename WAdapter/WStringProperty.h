#ifndef W_STRING_PROPERTY_H
#define W_STRING_PROPERTY_H

#include "WProperty.h"

class WStringProperty: public WProperty {
public:
	WStringProperty(String id, String title, String description, byte length)
	: WProperty(id, title, description, STRING, length) {
		this->atType = "StringProperty";
	}


protected:

private:

};

#endif
