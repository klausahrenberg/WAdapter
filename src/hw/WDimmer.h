#ifndef W_DIMMER_H
#define W_DIMMER_H

#define LEVEL_STEPS 5

#include "WGpio.h"

class WDimmer: public WGpio {
public:
	WDimmer(WGpioType gpioType, byte pin, uint8_t mode = OUTPUT)
			: WGpio(gpioType, pin, mode) {						
		_levelCurrent = 0;
		_stopLevelAdjusting();		
	}

	virtual ~WDimmer() {
		delete _level;
	}

	virtual void loop(unsigned long now) {
		WGpio::loop(now);	
		Serial.print(isInitialized()); Serial.print("-"); Serial.print(_levelAdjusting); Serial.print("-"); Serial.print(_levelLastUpdate); Serial.print("-"); Serial.println((_levelLastUpdate + 200 < now));
		if ((this->isInitialized()) && (_levelAdjusting) && ((_levelLastUpdate == 0) || (_levelLastUpdate + 200 < now))) {
			Serial.println(isOn());
			int targetLevel = (isOn() ? (_level != nullptr ? _level->asInt() : 100) : 0);
			if (targetLevel != _levelCurrent) {
				if (_levelStep == 0) {
					_levelStep = (targetLevel - _levelCurrent) / LEVEL_STEPS;					
				}
				_levelCurrent += _levelStep;
				_writeLevelCurrent(_levelCurrent);
				if (((_levelStep >= 0) && (_levelCurrent >= targetLevel)) || ((_levelStep < 0) && (_levelCurrent <= targetLevel))) {
					_stopLevelAdjusting();	
				}
			} else {
				_stopLevelAdjusting();
			}
			_levelLastUpdate = now;
		}
	}

	byte level() { return _level->asByte(); }

	WDimmer* level(byte level) { 
		if (_level->asByte() != level) {
			_level->asByte(level); 
			_updateLevel();
		}
		return this;
	}  

protected:
	WValue* _level = new WValue((byte) 100);

	bool _levelAdjusting;
	int _levelStep;
	int _levelCurrent;
	unsigned long _levelLastUpdate;
	
	virtual void _writeLevelCurrent(int levelCurrent) {
		_levelCurrent = levelCurrent;
	}

	virtual void _updateOn() {
		WGpio::_updateOn();
		_updateLevel();
  };

	void _updateLevel() {
		_levelAdjusting = true;
		_levelStep = 0;
		_levelLastUpdate = 0;

	}

	void _stopLevelAdjusting() {
		_updateLevel();
		_levelAdjusting = false;		
	}

private:
	

};

#endif
