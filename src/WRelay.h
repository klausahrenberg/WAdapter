#ifndef W_RELAY_H
#define W_RELAY_H

#include "WPin.h"

class WRelay: public WPin {
public:
	WRelay(int relayPin)
	: WPin(relayPin, OUTPUT) {
		this->inverted = false;
		if (this->isInitialized()) {
			digitalWrite(this->getPin(), getOffLevel());
		}
	}

	bool isOn() {
		return (digitalRead(this->getPin()) == getOnLevel());
	}

	void loop(unsigned long now) {
		if ((this->isInitialized()) && (getProperty() != nullptr)) {
			digitalWrite(this->getPin(), getProperty()->getBoolean() ? getOnLevel() : getOffLevel());
		}
	}

	bool isInverted() {
		return inverted;
	}

	void setInverted(bool inverted) {
		this->inverted = inverted;
	}

protected:

	byte getOnLevel() {
		return (!inverted ? HIGH : LOW);
	}

	byte getOffLevel() {
		return !getOnLevel();
	}

private:
	bool inverted;
};

#endif
