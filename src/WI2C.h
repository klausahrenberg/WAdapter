#ifndef W_I2C_H
#define W_I2C_H

#include "WInput.h"

class WI2C: public WInput {
public:
	WI2C(byte address, int sda, int scl, int interrupt)
			: WInput(interrupt, INPUT_PULLUP) {
		_address = address;
		_sda = sda;
		_scl = scl;
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
	byte _address;
	int _sda;
	int _scl;
};

#endif
