#ifndef W_DIMMER_H
#define W_DIMMER_H

#define LEVEL_STEPS 5

#include "WOutput.h"

class WDimmer: public WOutput {
public:
	WDimmer(byte pin, uint8_t mode = OUTPUT)
			: WOutput(pin, mode) {				
		_level = nullptr;
		_levelCurrent = 0;
		_stopLevelAdjusting();		
	}

	virtual void loop(unsigned long now) {
		WOutput::loop(now);	
		if ((this->isInitialized()) && (_levelAdjusting) && ((_levelLastUpdate == 0) || (_levelLastUpdate + 200 < now))) {
			int targetLevel = (isOn() ? (_level != nullptr ? _level->value().asInt() : 100) : 0);
			if (targetLevel != _levelCurrent) {
				if (_levelStep == 0) {
					_levelStep = (targetLevel - _levelCurrent) / LEVEL_STEPS;
					Serial.print("_levelStep is ");
					Serial.println(_levelStep);
					
				}
				_levelCurrent += _levelStep;
				Serial.print("_levelCurrent is ");
				Serial.println(_levelCurrent);
				_writeLevelCurrent(_levelCurrent);
				if (((_levelStep >= 0) && (_levelCurrent >= targetLevel)) || ((_levelStep < 0) && (_levelCurrent <= targetLevel))) {
					_stopLevelAdjusting();	
					Serial.println("_finito ");
				}
			} else {
				_stopLevelAdjusting();
			}
			_levelLastUpdate = now;
		}
	}

	WProperty* level() { return _level; }

	void level(WProperty* level) { 
		_level = level; 
		_level->addListener([this]() {_updateLevel();});
	}  

protected:
	WProperty* _level = nullptr;

	bool _levelAdjusting;
	int _levelStep;
	int _levelCurrent;
	unsigned long _levelLastUpdate;
	
	virtual void _writeLevelCurrent(int levelCurrent) {
		_levelCurrent = levelCurrent;
	}

	virtual void _updateOn() {
		WOutput::_updateOn();
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
