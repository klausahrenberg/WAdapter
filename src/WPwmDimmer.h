#ifndef W_PWM_DIMMER_H
#define W_PWM_DIMMER_H

#include "WOutput.h"

class WPwmDimmer: public WOutput {
public:
	#ifdef ESP8266
	WPwmDimmer(int pwmPin, byte pwmChannel = 0)
	: WOutput(pwmPin, OUTPUT) {
	#elif ESP32
	WPwmDimmer(int pwmPin, byte pwmChannel = 0)
	: WOutput(pwmPin, NO_MODE) {
	#endif
		_pwmChannel = pwmChannel;
		if (this->isInitialized()) {
			#ifdef ESP8266
			analogWrite(this->getPin(), 0);
			#elif ESP32
			ledcSetup(pwmChannel, 9000, 10);
			ledcAttachPin(getPin(), this->pwmChannel);
			ledcWrite(this->pwmChannel, 0);
			#endif
		}
		_brightness = 0;
		_changeBrightness = false;
		_changeProperty = false;
	}

	bool isOn() {
		return (analogRead(this->getPin()) > 0);
	}

	void loop(unsigned long now) {
		if ((this->isInitialized()) && (getProperty() != nullptr) && ((getProperty()->getBoolean() != _changeProperty) || (_changeBrightness == true))) {
			#ifdef ESP8266
			analogWrite(this->getPin(), getProperty()->getBoolean() ? brigthness() : 0);
			#elif ESP32
			ledcWrite(_pwmChannel, getProperty()->getBoolean() ? brigthness() : 0);
			#endif
			_changeBrightness = false;
			_changeProperty = getProperty()->getBoolean();
		}
	}

	int brigthness() {
		return _brightness;
	}

	void setBrightness(int brightness) {
		if (_brightness != brightness) {
			_changeBrightness = true;
			_brightness = brightness;
		}
	}
protected:

private:
	byte _pwmChannel;
	int _brightness;
	bool _changeBrightness;
	bool _changeProperty;

};

#endif
