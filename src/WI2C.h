#ifndef W_I2C_H
#define W_I2C_H

#include <Wire.h>
#include "WGpio.h"

class WI2C: public WGpio {
public:
	WI2C(WGpioType gpioType, byte address, int sda, int scl, int interrupt, TwoWire* i2cPort = &Wire)
			: WGpio(gpioType, interrupt, INPUT_PULLUP) {
		_i2cPort = i2cPort;
		_address = address;
		_sda = sda;
		_scl = scl;
		if (this->isInitialized()) {
			_i2cPort->begin(_sda, _scl);
		}
	}

	byte address() {
		return _address;
	}

	virtual void loop(unsigned long now) {
		WGpio::loop(now);
	}

protected:
	TwoWire* _i2cPort;
	byte _address;
	int _sda;
	int _scl;	

	bool isInitialized() {
		return ((_sda != NO_PIN) && (_scl != NO_PIN));
	}

private:
	
};

#endif
