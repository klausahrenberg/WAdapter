#ifndef W_PWM_DIMMER_H
#define W_PWM_DIMMER_H

#include "WPin.h"

class WPwmDimmer: public WPin {
public:
	WPwmDimmer(int relayPin)
	: WPin(relayPin, OUTPUT) {
		if (this->isInitialized()) {
			#ifdef ESP8266
			analogWrite(this->getPin(), 0);
			#endif
		}
		brightness = 0;
	}

	bool isOn() {
		return (analogRead(this->getPin()) > 0);
	}

	void loop(unsigned long now) {
		if ((this->isInitialized()) && (getProperty() != nullptr)) {
			#ifdef ESP8266
			analogWrite(this->getPin(), getProperty()->getBoolean() ? getBrigthness() : 0);
			#endif
		}
	}

	int getBrigthness() {
		return this->brightness;
	}

	void setBrightness(int brightness) {
		this->brightness = brightness;
	}
protected:

private:
	int brightness;

};

#endif
