#ifndef W_LONG_PROPERTY_H
#define W_LONG_PROPERTY_H

#include "WProperty.h"

class WLongProperty: public WProperty {
public:
	WLongProperty(String id)
	: WProperty(id, id, "", LONG) {
		this->atType = "LongProperty";
	}

protected:

private:

};

#endif
