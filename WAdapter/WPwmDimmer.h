#ifndef W_PWM_DIMMER_H
#define W_PWM_DIMMER_H

#include "WPin.h"

class WPwmDimmer: public WPin {
public:
	#ifdef ESP8266
	WPwmDimmer(int pwmPin, byte pwmChannel = 0)
	: WPin(pwmPin, OUTPUT) {
	#elif ESP32
	WPwmDimmer(int pwmPin, byte pwmChannel = 0)
	: WPin(pwmPin, NO_MODE) {
	#endif
		this->pwmChannel = pwmChannel;
		if (this->isInitialized()) {
			#ifdef ESP8266
			analogWrite(this->getPin(), 0);
			#elif ESP32
			ledcSetup(pwmChannel, 9000, 10);
			ledcAttachPin(getPin(), this->pwmChannel);
			ledcWrite(this->pwmChannel, 0);
			#endif
		}
		brightness = 0;
		changeBrightness = false;
		changeProperty = false;
	}

	bool isOn() {
		return (analogRead(this->getPin()) > 0);
	}

	void loop(unsigned long now) {
		if ((this->isInitialized()) && (getProperty() != nullptr) && ((getProperty()->getBoolean() != changeProperty) || (changeBrightness == true))) {
			#ifdef ESP8266
			analogWrite(this->getPin(), getProperty()->getBoolean() ? getBrigthness() : 0);
			#elif ESP32
			ledcWrite(this->pwmChannel, getProperty()->getBoolean() ? getBrigthness() : 0);
			#endif
			changeBrightness = false;
			changeProperty = getProperty()->getBoolean();
		}
	}

	int getBrigthness() {
		return this->brightness;
	}

	void setBrightness(int brightness) {
		if (this->brightness != brightness) {
			changeBrightness = true;
			this->brightness = brightness;
		}
	}
protected:

private:
	byte pwmChannel;
	int brightness;
	bool changeBrightness;
	bool changeProperty;

};

#endif
