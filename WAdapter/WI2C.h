#ifndef W_I2C_H
#define W_I2C_H

#include "WPin.h"

class WI2C: public WPin {
public:
	WI2C(byte address, int sda, int scl, int interrupt)
			: WPin(interrupt, INPUT_PULLUP) {
		this->address = address;
		this->sda = sda;
		this->scl = scl;
		if (this->isInitialized()) {
			Wire.begin(sda, scl);
		}

	}

	byte getAddress() {
		return this->address;
	}

	virtual void loop(unsigned long now) {
	}

protected:
	bool isInitialized() {
		return ((sda != NO_PIN) && (scl != NO_PIN));
	}

private:
	byte address;
	int sda;
	int scl;
};

#endif
