#ifndef W_SWITCH_H_
#define W_SWITCH_H_

#include "WInput.h"

#define MODE_BUTTON 0
#define MODE_BUTTON_LONG_PRESS 1
#define MODE_SWITCH 2
#define SWITCH_PRESSED_PEGEL HIGH

const unsigned long SWITCH_SENSITIVENESS = 200;
const unsigned long BUTTON_SENSITIVENESS = 20;
const unsigned long SWICTH_LONG_PRESS_DURATION = 5000;


class WSwitch: public WInput {
public:	
	WSwitch(int switchPin, byte mode, bool inverted = false)
	: WInput(switchPin, (inverted ? INPUT_PULLUP : INPUT)) {		
		_startTime = 0;
		_longPressStartTime = 0;
		_inverted = inverted;
		triggerProperty = nullptr;
		this->mode = mode;
		if (this->isInitialized()) {
			state = digitalRead(this->pin());
			lastState = state;
		}
	}
	
	void loop(unsigned long now) {
		if (this->isInitialized()) {
			//1. Eliminate flickering input
			bool stateChanged = false;
			bool newState = digitalRead(this->pin());					
			bool expectedPegel = (this->mode == MODE_SWITCH ? !state : getOnLevel());
			unsigned long sensitiveness = (this->mode == MODE_SWITCH ? SWITCH_SENSITIVENESS : BUTTON_SENSITIVENESS);

			if ((newState != lastState) && (_startTime == 0)) {
				_startTime = now;	
			} else if ((newState == state) && (_startTime > 0)) {
				_startTime = now;
			} else if ((newState != state) && (now - _startTime >= sensitiveness)) {
				stateChanged = true;	
			}

			lastState = newState;
			//2. If state really changed, now switch logic
			if (stateChanged) {
				state = !state;
				_startTime = 0;
				if (state == expectedPegel) {					
					if (this->mode != MODE_SWITCH) {
						//Button handling
						if (this->mode == MODE_BUTTON) {
							//Button
							handleButtonOrSwitchPressed();
							_longPressStartTime = 0;
						} else {
							_longPressStartTime = now;
						}
					} else {
						//Switch handling
						handleButtonOrSwitchPressed();
						_longPressStartTime = 0;
					}
				} else {					
					if (this->mode == MODE_SWITCH) {
						//Switch handling
						handleButtonOrSwitchPressed();
					}	else if ((this->mode == MODE_BUTTON_LONG_PRESS) && (_longPressStartTime >0) && (now - _longPressStartTime < SWICTH_LONG_PRESS_DURATION)) {
						//Long press button was released before long press time
						handleButtonOrSwitchPressed();						
					}
					_longPressStartTime = 0;
				}
			} else {
				//3. If state not changed, reset trigger and handle long press buttons
				setTriggerValue(false);
				//Long press time is up				
				if ((this->mode == MODE_BUTTON_LONG_PRESS) && (_longPressStartTime > 0) && (now - _longPressStartTime >= SWICTH_LONG_PRESS_DURATION)) {
					handleLongButtonPressed();
					_longPressStartTime = 0;
				}
			}
		}
	}

	bool isLongPressing() {
		return ((this->mode == MODE_BUTTON_LONG_PRESS) && (_longPressStartTime > 0));
	}

	void handleButtonOrSwitchPressed() {

		setTriggerValue(true);
		if (getProperty() != nullptr) {
			getProperty()->setBoolean(!getProperty()->getBoolean());
		}
		if (onPressed) {
			onPressed();
		}
	}

	void handleLongButtonPressed() {
		if (onLongPressed) {
			onLongPressed();
		}
	}

	void setOnPressed(THandlerFunction onPressed) {
		this->onPressed = onPressed;
	}

	void setOnLongPressed(THandlerFunction onLongPressed) {
		this->onLongPressed = onLongPressed;
	}

	void setTriggerProperty(WProperty* triggerProperty) {
		if (this->triggerProperty != triggerProperty) {
			this->triggerProperty = triggerProperty;
		}
	}

	bool hasTriggerProperty() {
		return (this->triggerProperty != nullptr);
	}

protected:

	byte getOnLevel() {
		return (!_inverted ? SWITCH_PRESSED_PEGEL : !SWITCH_PRESSED_PEGEL);
	}

	byte getOffLevel() {
		return !getOnLevel();
	}

private:
	THandlerFunction onPressed;
	THandlerFunction onLongPressed;
	byte mode;
	bool state, lastState, _inverted;
	unsigned long _startTime, _longPressStartTime;
	WProperty* triggerProperty;

	void setTriggerValue(bool triggered) {
		if (triggerProperty != nullptr) {
			triggerProperty->setBoolean(triggered);
		}
	}

};

#endif
