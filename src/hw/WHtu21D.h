#ifndef W_HTU21D_H
#define W_HTU21D_H

#include "../WI2C.h"

#define HTU21D_ADDRESS 0x40

#define ERROR_I2C_TIMEOUT 	998
#define ERROR_BAD_CRC		999
#define TRIGGER_TEMP_MEASURE_HOLD  0xE3
#define TRIGGER_HUMD_MEASURE_HOLD  0xE5
#define TRIGGER_TEMP_MEASURE_NOHOLD  0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD  0xF5
#define MAX_COUNTER (MAX_WAIT/DELAY_INTERVAL)
#define DELAY_INTERVAL 10
#define MAX_WAIT 100
#define SHIFTED_DIVISOR 0x988000
#define AVERAGE_COUNTS 3
#define CORRECTION_TEMPERATURE -2.0
#define CORRECTION_HUMIDITY 0.0

class WHtu21D: public WI2C {
public:
	WHtu21D(int sda, int scl)
			: WI2C(HTU21D_ADDRESS, sda, scl, NO_PIN) {
		_lastMeasure = 0;
		_measureInterval = 10000;
		_measuring = false;
		_measureValue = 0;
		_humidityValue = 0;
		_measureCounts = 0;
		_humidity = nullptr;
	}

	virtual void loop(unsigned long now) {
		WI2C::loop(now);
		if ((_lastMeasure == 0) || (now - _lastMeasure > (unsigned long)(_measureInterval)) || ((_measuring) && (now - _lastMeasure > 2100))) {
    	_lastMeasure = now;           
    	float t = readTemperature();
			float h = readHumidity();			
    	if ((!isnan(t)) && (!isnan(h))) {
      	_measureValue = _measureValue + t;
				_humidityValue = _humidityValue + h;
      	_measureCounts++;
      	_measuring = (_measureCounts < AVERAGE_COUNTS);
      	if (!_measuring) {
					_measureValue = (double) (round(_measureValue * 10.0 / (double) _measureCounts) / 10.0) + CORRECTION_TEMPERATURE;		
					_humidityValue = (double) (round(_humidityValue * 10.0 / (double) _measureCounts) / 10.0) + CORRECTION_HUMIDITY;					
					if (hasProperty()) {
						property()->setDouble(_measureValue);
					}	
					if (hasHumidity()) {
						humidity()->setDouble(_humidityValue);
					}
        	_measureValue = 0;
					_humidityValue = 0;
        	_measureCounts = 0;
      	}
    	}
		}
	}

	float readHumidity() {
  	uint16_t rawHumidity = _readValue(TRIGGER_HUMD_MEASURE_NOHOLD);
  	if(rawHumidity == ERROR_I2C_TIMEOUT || rawHumidity == ERROR_BAD_CRC) return(rawHumidity);
  	//Given the raw humidity data, calculate the actual relative humidity
  	float tempRH = rawHumidity * (125.0 / 65536.0); //2^16 = 65536
  	float rh = tempRH - 6.0; //From page 14
  	return (rh);
	}

	float readTemperature() {
  	uint16_t rawTemperature = _readValue(TRIGGER_TEMP_MEASURE_NOHOLD);		
  	if(rawTemperature == ERROR_I2C_TIMEOUT || rawTemperature == ERROR_BAD_CRC) return(rawTemperature);
  	//Given the raw temperature data, calculate the actual temperature
  	float tempTemperature = rawTemperature * (175.72 / 65536.0); //2^16 = 65536
  	float realTemperature = tempTemperature - 46.85; //From page 14

  	return (realTemperature);
	}

	WProperty* humidity() { return _humidity; }

  void setHumidity(WProperty* humidity) {
    if (_humidity != humidity) {
      _humidity = humidity;
      this->loop(millis());
    }
  }

  bool hasHumidity() { return (_humidity != nullptr); }

protected:

private:
	unsigned long _lastMeasure;
	int _measureInterval, _measureCounts;
	bool _measuring;
	double _measureValue, _humidityValue;
	WProperty* _humidity;

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

	uint16_t _readValue(byte cmd) {
  	//Request a humidity reading
  	_i2cPort->beginTransmission(HTU21D_ADDRESS);
  	_i2cPort->write(cmd); //Measure value (prefer no hold!)
  	_i2cPort->endTransmission();
  
  	//Hang out while measurement is taken. datasheet says 50ms, practice may call for more
  	bool validResult;
  	byte counter;
  	for (counter = 0, validResult = 0 ; counter < MAX_COUNTER && !validResult ; counter++) {
    	delay(DELAY_INTERVAL);

    	//Comes back in three bytes, data(MSB) / data(LSB) / Checksum
    	validResult = (3 == _i2cPort->requestFrom(HTU21D_ADDRESS, 3));
  	}

  	if (!validResult) return (ERROR_I2C_TIMEOUT); //Error out

  	byte msb, lsb, checksum;
  	msb = _i2cPort->read();
  	lsb = _i2cPort->read();
  	checksum = _i2cPort->read();

  	uint16_t rawValue = ((uint16_t) msb << 8) | (uint16_t) lsb;

  	if (_checkCRC(rawValue, checksum) != 0) return (ERROR_BAD_CRC); //Error out

  	return rawValue & 0xFFFC; // Zero out the status bits
	}

};

#endif
