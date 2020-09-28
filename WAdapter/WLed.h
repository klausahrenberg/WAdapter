#ifndef W_LED_H
#define W_LED_H

#include "WPin.h"

#ifdef ESP8266
const byte LED_ON = LOW;
const byte LED_OFF = HIGH;
#elif ESP32
const byte LED_ON = HIGH;
const byte LED_OFF = LOW;
#endif

class WLed: public WPin {
public:
	WLed(int ledPin)
		: WPin(ledPin, OUTPUT) {
		this->blinkMillis = 0;
		this->ledOn = false;
		if (this->isInitialized()) {
			digitalWrite(this->getPin(), LED_OFF);
		}
	}

	bool isOn() {
		return (this->getProperty() != nullptr ? this->getProperty()->getBoolean() : this->ledOn);
	}

	void setOn(bool ledOn) {
		if (this->getProperty() != nullptr) {
			this->getProperty()->setBoolean(ledOn);
		} else if (ledOn != isOn()) {
			this->ledOn = ledOn;
		}
		blinkOn = false;
		lastBlinkOn = 0;
	}

	void setOn(bool ledOn, int blinkMillis) {
		if ((this->isOn()) && (this->blinkMillis != blinkMillis)) {
			this->setOn(false);
		}
		this->blinkMillis = blinkMillis;
		this->setOn(ledOn);
	}

	void on() {
		setOn(true);
	}

	void off() {
		setOn(false);
	}

	void toggle() {
		setOn(isOn() ? false : true);
	}

	bool isBlinking() {
		return (this->blinkMillis > 0);
	}

	void loop(unsigned long now) {
		if (isOn()) {
			if (isBlinking()) {
				if ((lastBlinkOn == 0) || (now - lastBlinkOn > this->blinkMillis)) {
					blinkOn = !blinkOn;
					lastBlinkOn = now;
					digitalWrite(this->getPin(), blinkOn ? LED_ON : LED_OFF);
				}
			} else {
				digitalWrite(this->getPin(), LED_ON);
			}
		} else {
			//switchoff
			digitalWrite(this->getPin(), LED_OFF);
		}
	}

protected:
private:
	bool ledOn, blinkOn;
	unsigned long blinkMillis, lastBlinkOn;
};

#endif
