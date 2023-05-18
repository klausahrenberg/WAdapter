#ifndef W_HTU21D_H
#define W_HTU21D_H

#include "../WI2C.h"

#define HTU21D_ADDRESS 0x40
#define I2C_TIMEOUT 500
#define I2C_BETWEEN 500
#define I2C_REQUEST_TO_RESULT 50
#define ERROR_NONE 0
#define ERROR_I2C_TIMEOUT 1
#define ERROR_BAD_CRC	2
#define TRIGGER_TEMP_MEASURE_NOHOLD 0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD 0xF5
#define MAX_WAIT 100
#define SHIFTED_DIVISOR 0x988000
#define HTU21D_AVERAGE_COUNTS 5

enum WHtu21DState {
  STATE_IDLE,
  STATE_READ_TEMPERATURE,
	STATE_READ_TEMPERATURE_PAUSED,
  STATE_READ_HUMIDITY,
	STATE_READ_HUMIDITY_PAUSED
};

class WHtu21D: public WI2C {
public:
	WHtu21D(int sda, int scl, int interrupt = NO_PIN, TwoWire* i2cPort = &Wire)
			: WI2C(HTU21D_ADDRESS, sda, scl, interrupt, i2cPort) {
		_lastMeasure = 0;
		_measureInterval = 10000;
		_humidity = nullptr;
		_correctionTemperature = -0.5;
		_correctionHumidity = 0.0;
		_reset();
		_lastError = ERROR_NONE;
	}

	virtual void loop(unsigned long now) {
		WI2C::loop(now);		
		if (_state == STATE_IDLE) {			
			if ((_lastMeasure == 0) || (now - _lastMeasure > (unsigned long)(_measureInterval))) {
				//start measurement
				_lastMeasure = now;
				_reset();    
				_request(now, TRIGGER_TEMP_MEASURE_NOHOLD, STATE_READ_TEMPERATURE);				
			}
		} else if ((_state == STATE_READ_TEMPERATURE_PAUSED) || (_state == STATE_READ_HUMIDITY_PAUSED)) {
			if (now - _lastRequest > I2C_BETWEEN) {
				//Restart measurement
				if (_state == STATE_READ_TEMPERATURE_PAUSED) {			
					_request(now, TRIGGER_TEMP_MEASURE_NOHOLD, STATE_READ_TEMPERATURE);
				} else if (_state == STATE_READ_HUMIDITY_PAUSED) {
					_request(now, TRIGGER_HUMD_MEASURE_NOHOLD, STATE_READ_HUMIDITY);						
				}	
			}	
		} else {								
  		if ((now - _lastRequest > I2C_REQUEST_TO_RESULT) && (_i2cPort->requestFrom(HTU21D_ADDRESS, 3) == 3)) {
				//There is a result
				byte msb, lsb, checksum;
  			msb = _i2cPort->read();
  			lsb = _i2cPort->read();
  			checksum = _i2cPort->read();
  			uint16_t rawValue = ((uint16_t) msb << 8) | (uint16_t) lsb;
  			if (_checkCRC(rawValue, checksum) == 0) {
					//Everything fine
					rawValue = rawValue & 0xFFFC; // Zero out the status bits
					if (_state == STATE_READ_TEMPERATURE) {
						double t = rawValue * (175.72 / 65536.0) - 46.85;
						_temperatureValue = _temperatureValue + t;												
						_counter++;
						_lastMeasure = now;
						if (_counter < HTU21D_AVERAGE_COUNTS) {														
							_state = STATE_READ_TEMPERATURE_PAUSED;
						} else {
							_temperatureValue = _temperatureValue / (double) HTU21D_AVERAGE_COUNTS;							
							if (hasProperty()) {
								property()->setDouble(_temperatureValue + _correctionTemperature);
							}
							_counter = 0;
							_request(now, TRIGGER_HUMD_MEASURE_NOHOLD, STATE_READ_HUMIDITY);
						}		
					} else if (_state == STATE_READ_HUMIDITY) {
						double h = rawValue * (125.0 / 65536.0) - 6.0;
						_humidityValue = _humidityValue + h;												
						_counter++;
						_lastMeasure = now;
						if (_counter < HTU21D_AVERAGE_COUNTS) {														
							_state = STATE_READ_HUMIDITY_PAUSED;
						} else {
							_humidityValue = _humidityValue / (double) HTU21D_AVERAGE_COUNTS;
							if (hasHumidity()) {
								humidity()->setDouble(_humidityValue + _correctionHumidity);
							}
							_reset();
							_lastError = ERROR_NONE;
						}		
					}	
				} else {
					//Checksum error
					_reset();
					_lastError = ERROR_BAD_CRC;
				}
			} else if (now - _lastRequest > I2C_TIMEOUT) {
				//Timeout
				_reset();
				_lastError = ERROR_I2C_TIMEOUT;
			}
		}		
	}

	void _reset() {
		_counter = 0;
		_lastRequest = 0;
		_state = STATE_IDLE;
		_temperatureValue = 0.0;
		_humidityValue = 0.0;
	}

	void _request(unsigned long now, byte cmd, WHtu21DState followUpState) {
		_i2cPort->beginTransmission(HTU21D_ADDRESS);		
		_i2cPort->write(cmd);
		_i2cPort->endTransmission();
		_state = followUpState;
		_lastRequest = now;
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

	double correctionTemperature() { return _correctionTemperature; }

	void correctionTemperature(double correctionTemperature) {
		_correctionTemperature = correctionTemperature;
	}

	double correctionHumidity() { return _correctionHumidity; }

	void correctionHumidity(double correctionHumidity) {
		_correctionHumidity = correctionHumidity;
	}

protected:

private:
	unsigned long _lastMeasure, _lastRequest;
	int _measureInterval;
	double _temperatureValue, _humidityValue;
	WProperty* _humidity;
	WHtu21DState _state;
	byte _counter;
	byte _lastError;
	double _correctionTemperature, _correctionHumidity;

	byte _checkCRC(uint16_t message_from_sensor, uint8_t check_value_from_sensor) {  	
  	uint32_t remainder = (uint32_t)message_from_sensor << 8; 
  	remainder |= check_value_from_sensor; 
  	uint32_t divsor = (uint32_t)SHIFTED_DIVISOR;
  	for (int i = 0 ; i < 16 ; i++) {    
    	if ( remainder & (uint32_t)1 << (23 - i) )
      	remainder ^= divsor;
    	divsor >>= 1;
  	}
  	return (byte)remainder;
	}

};

#endif
