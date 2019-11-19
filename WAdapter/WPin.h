#ifndef W_PIN_H
#define W_PIN_H

#include "WProperty.h"

class WPin {
public:
	WPin* next = nullptr;

	WPin(bool debug, int pin, int mode) {
		this->debug = debug;
		this->pin = pin;
		if (this->isInitialized() && ((mode == INPUT) || (mode == OUTPUT) || (mode == INPUT_PULLUP))) {
			pinMode(this->pin, mode);
		}
	}

	WProperty* getProperty() {
		return property;
	}

	void setProperty(WProperty* property) {
		if (this->property != property) {
			this->property = property;
			this->loop(millis());
		}
	}

	virtual void loop(unsigned long now) {
	}

protected:

	void log(String debugMessage) {
		if (debug) {
			Serial.println(debugMessage);
		}
	}

	virtual bool isInitialized() {
		return (pin > -1);
	}

	int getPin() {
		return pin;
	}
private:
	bool debug;
	int pin;
	WProperty* property;
};

#endif
