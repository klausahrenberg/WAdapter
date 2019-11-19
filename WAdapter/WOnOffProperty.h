#ifndef W_ON_OFF_PROPERTY_H
#define W_ON_OFF_PROPERTY_H

#include "WProperty.h"

class WOnOffProperty: public WProperty {
public:
	WOnOffProperty(String id, String title, String description)
	: WProperty(id, title, description, BOOLEAN) {
		this->atType = "OnOffProperty";
	}


protected:

private:

};

#endif
