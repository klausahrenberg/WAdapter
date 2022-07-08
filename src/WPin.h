#ifndef W_PIN_H
#define W_PIN_H

#include "WProperty.h"

const int NO_PIN = -1;
const int NO_MODE = -1;

class WPin {
public:
	WPin(int pin, int mode) {
		this->pin = pin;
		this->property = nullptr;
		if ((this->pin != NO_PIN) && ((mode == INPUT) || (mode == OUTPUT) || (mode == INPUT_PULLUP))) {
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

	bool hasProperty() {
		return (this->property != nullptr);
	}

	virtual void loop(unsigned long now) {
	}

	WPin* next = nullptr;

protected:

	virtual bool isInitialized() {
		return (pin != NO_PIN);
	}

	int getPin() {
		return pin;
	}

private:
	int pin;
	WProperty* property;
};

#endif
