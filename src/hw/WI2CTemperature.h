#ifndef W_I2C_TEMPERATURE_H
#define W_I2C_TEMPERATURE_H

#include "../WI2C.h"

#define ERROR_NONE 0
#define ERROR_I2C_TIMEOUT 1
#define ERROR_BAD_CRC	2
#define ERROR_I2C_REQUEST 3

class WI2CTemperature: public WI2C {
public:
	WI2CTemperature(byte address, int sda, int scl, TwoWire* i2cPort = &Wire)
			: WI2C(address, sda, scl, NO_PIN, i2cPort) {				
		_humidity = nullptr;		
		_lastError = ERROR_NONE;
	}

	virtual void loop(unsigned long now) {
		WI2C::loop(now);	
	}
	

	WProperty* humidity() { return _humidity; }

  void setHumidity(WProperty* humidity) {
    if (_humidity != humidity) {
      _humidity = humidity;
      this->loop(millis());
    }
  }

  bool hasHumidity() { return (_humidity != nullptr); }

	bool hasError() { return (_lastError != ERROR_NONE); }

	byte lastError() { return _lastError; }	

protected:
	WProperty* _humidity;	
	byte _lastError;

private:
	

};

#endif
