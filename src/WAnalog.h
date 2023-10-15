#ifndef W_ANALOG_INPUT_H
#define W_ANALOG_INPUT_H

#include "WInput.h"

class WAnalog : public WInput {
 public:
  WAnalog(int analogPin, int minimum = 0, int maximum = 100) : WInput(analogPin, INPUT) {	
		_minimum = minimum;
		_maximum = maximum;
		_analogMinimum = 0;
		#ifdef ESP8266
		_analogMaximum = 1023;
		#elif ESP32
		_analogMaximum = 4095;
		#endif
	}

	void loop(unsigned long now) {
		if ((this->isInitialized()) && (hasProperty())) {
			int ain = analogRead(pin());
			ain = constrain(ain, _analogMinimum, _analogMaximum);			
			ain = map(ain, _minimum, _maximum, _analogMinimum, _analogMaximum);
			
			switch (property()->type()) {
				INTEGER: {
					property()->asInt(ain);
					break;
				}
				BYTE: {
					property()->asByte(ain);
					break;
				}
				DOUBLE: {
					property()->asDouble(ain);
					break;
				}
			}
		}
	}	

 private:
	int _analogMinimum;
	int _analogMaximum;
	int _minimum;
	int _maximum;

};

#endif