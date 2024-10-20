#ifndef W_PWM_DIMMER_H
#define W_PWM_DIMMER_H

#include "WDimmer.h"

class WPwmDimmer: public WDimmer {
public:
	#ifdef ESP8266
	WPwmDimmer(int pwmPin = NO_PIN, byte pwmChannel = 0)
	: WDimmer(GPIO_TYPE_PWM, pwmPin, OUTPUT) {
		//analogWriteFreq(100);
	#elif ESP32
	WPwmDimmer(int pwmPin = NO_PIN, byte pwmChannel = 0)
	: WDimmer(GPIO_TYPE_PWM, pwmPin, NO_MODE) {
	#endif
		_pwmChannel = pwmChannel;
	}

	/*bool isOn() {
		#ifdef ESP8266
		return (analogRead(this->pin()) > 0);
		#elif ESP32
		return ledcRead(_pwmChannel);
		#endif
	}*/

	void loop(unsigned long now) {
		WDimmer::loop(now);
	}

	
protected:

	virtual void _updateOn() {
    WDimmer::_updateOn();
	}

	virtual void _onChange() {    
    if (isInitialized()) {
      #ifdef ESP8266
			analogWrite(this->pin(), 0);
			#elif ESP32
			ledcSetup(_pwmChannel, 100000, 8);
			ledcAttachPin(pin(), _pwmChannel);
			//ledcWrite(_pwmChannel, 0xFF);
			ledcWrite(_pwmChannel, 0x30);

			/*ledcSetup(_pwmChannel, 5000, 0x400);
			ledcAttachPin(pin(), _pwmChannel);
			ledcWrite(_pwmChannel, 0);*/
			#endif						
    }
  } 

	virtual void _writeLevelCurrent(int levelCurrent) {
		/*WDimmer::_writeLevelCurrent(levelCurrent);
		if (isInitialized()) {		
			levelCurrent = levelCurrent * 0x3FF / 100;
			#ifdef ESP8266
				analogWrite(this->pin(), levelCurrent);
			#elif ESP32
				ledcWrite(_pwmChannel, levelCurrent);
			#endif
		}*/
	}	

private:
	byte _pwmChannel;


};

#endif
