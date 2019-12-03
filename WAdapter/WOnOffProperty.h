#ifndef W_ON_OFF_PROPERTY_H
#define W_ON_OFF_PROPERTY_H

#include "WProperty.h"

class WOnOffProperty: public WProperty {
public:
	WOnOffProperty(const char* id, const char* title)
	: WProperty(id, title, BOOLEAN) {
		this->atType = "OnOffProperty";
	}


protected:

private:

};

#endif
