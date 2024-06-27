#ifndef W_PWM_DIMMER_H
#define W_PWM_DIMMER_H

#include "WDimmer.h"

class WPwmDimmer: public WDimmer {
public:
	#ifdef ESP8266
	WPwmDimmer(int pwmPin, byte pwmChannel = 0)
	: WDimmer(pwmPin, OUTPUT) {
		//analogWriteFreq(100);
	#elif ESP32
	WPwmDimmer(int pwmPin, byte pwmChannel = 0)
	: WDimmer(pwmPin, NO_MODE) {
	#endif
		_pwmChannel = pwmChannel;
		if (this->isInitialized()) {
			#ifdef ESP8266
			analogWrite(this->pin(), 0);
			#elif ESP32
			ledcSetup(_pwmChannel, 9000, 10);
			ledcAttachPin(pin(), _pwmChannel);
			ledcWrite(_pwmChannel, 0);
			#endif
		}
	}

	bool isOn() {
		return (analogRead(this->pin()) > 0);
	}

	void loop(unsigned long now) {
		WDimmer::loop(now);
	}

	
protected:

	virtual void _writeLevelCurrent(int levelCurrent) {
		WDimmer::_writeLevelCurrent(levelCurrent);
		levelCurrent = levelCurrent * 0x3FF / 100;
		Serial.println(levelCurrent);
		#ifdef ESP8266
			analogWrite(this->pin(), levelCurrent);
		#elif ESP32
			ledcWrite(_pwmChannel, levelCurrent);
		#endif
	}	

private:
	byte _pwmChannel;

};

#endif
