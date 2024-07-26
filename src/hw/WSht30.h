
#ifndef W_SHT30_H
#define W_SHT30_H

#include "hw/WI2CTemperature.h"
#include "Wire.h"
#include "WProperty.h"

#define SHT30_ADDRESS 0x44
#define I2C_REQUEST_TO_RESULT 100
#define I2C_BETWEEN 800
#define I2C_TIMEOUT 500
#define SHT30_AVERAGE_COUNTS 3

#define ERROR_NONE 0
#define ERROR_I2C_TIMEOUT 1
#define ERROR_BAD_CRC	2
#define ERROR_I2C_REQUEST 3


enum WSht30ReadState {
  STH30_STATE_IDLE,
  STH30_STATE_READ,
  STH30_STATE_READ_PAUSED
};

class WSht30 : public WI2CTemperature {
 public:
  WSht30(int sda, int scl, TwoWire* i2cPort = &Wire)
      : WI2CTemperature(SHT30_ADDRESS, sda, scl, i2cPort) {
    _lastMeasure = 0;
    _measureInterval = 10000;
		_correctionTemperature = -0.5;
		_correctionHumidity = 0.0;		
    _reset();
  }

  virtual void loop(unsigned long now) {
		WI2CTemperature::loop(now);		
    if (_state == STH30_STATE_IDLE) {
      if ((_lastMeasure == 0) || (now - _lastMeasure > (unsigned long)(_measureInterval))) {
				// start measurement
      	_lastMeasure = now;
      	_reset();
      	if (_request(now)) {
					_state = STH30_STATE_READ;
				} else {
					_lastError = ERROR_I2C_REQUEST;
				}
			}
    } else if (_state == STH30_STATE_READ) {
      if ((now - _lastRequest > I2C_REQUEST_TO_RESULT) && (_i2cPort->requestFrom(address(), 6) == 6)) {				
				unsigned int data[6];
        for (int i = 0; i < 6; i++) {
          data[i] = _i2cPort->read();
        };
				_temperatureValue = _temperatureValue + (((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45);        
        _humidityValue = _humidityValue + ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);        
				_counter++;		
				if (_counter < SHT30_AVERAGE_COUNTS) {														
					_state = STH30_STATE_READ_PAUSED;
				} else {
					_temperatureValue = _temperatureValue / (double) SHT30_AVERAGE_COUNTS;							
					if (hasProperty()) {
						property()->value()->asDouble(_temperatureValue + _correctionTemperature);				
					}				
					_humidityValue = _humidityValue / (double) SHT30_AVERAGE_COUNTS;
					if (hasHumidity()) {
						humidity()->value()->asDouble(_humidityValue + _correctionHumidity);
					}
					_lastMeasure = now;
        	_reset(); //IDLE		
					_lastError = ERROR_NONE;			
				}	
      } else if (now - _lastRequest > I2C_TIMEOUT) {
        // Timeout
        _reset();
        _lastError = ERROR_I2C_TIMEOUT;
      }
    } else {
			//STH30_STATE_READ_PAUSED
			if (now - _lastRequest > I2C_BETWEEN) {
				if (_request(now)) {
					_state = STH30_STATE_READ;
				} else {
					_reset();
					_lastError = ERROR_I2C_REQUEST;
				}
			}	
		}
  }

	WProperty* humidity() { return _humidity; }

  void setHumidity(WProperty* humidity) {
    if (_humidity != humidity) {
      _humidity = humidity;
      this->loop(millis());
    }
  }

  bool hasHumidity() { return (_humidity != nullptr); }

 private:
  WSht30ReadState _state;
  unsigned long _lastMeasure, _lastRequest;
  int _measureInterval;
  double _temperatureValue, _humidityValue;
	double _correctionTemperature, _correctionHumidity;
  byte _counter;

  void _reset() {
    _state = STH30_STATE_IDLE;
    _lastRequest = 0;
    _temperatureValue = 0.0;
    _humidityValue = 0.0;
    _counter = 0;
  }

	byte _request(unsigned long now) {				
		_i2cPort->beginTransmission(_address);      
    _i2cPort->write(0x2C);
    _i2cPort->write(0x06);      
    if (_i2cPort->endTransmission() == 0) {
			_lastRequest = now;
			return true;
		} else {
			return false;
		}
	}	
};

#endif
