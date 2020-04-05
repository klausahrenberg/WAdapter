#ifndef W_OPEN_PROPERTY_H
#define W_OPEN_PROPERTY_H

#include "WProperty.h"

class WOpenProperty: public WProperty {
public:
	WOpenProperty(const char* id, const char* title)
	: WProperty(id, title, BOOLEAN) {
		this->atType = "OpenProperty";
	}

protected:

private:

};

#endif
