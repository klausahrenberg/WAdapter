#ifndef W_STRING_PROPERTY_H
#define W_STRING_PROPERTY_H

#include "WProperty.h"

class WStringProperty: public WProperty {
public:
	WStringProperty(const char* id, const char* title, byte length)
	: WProperty(id, title, STRING, length) {
		this->atType = "StringProperty";
	}


protected:

private:

};

#endif
