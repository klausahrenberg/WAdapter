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
	typedef std::function<void()> THandlerFunction;
	WSwitch(int switchPin, byte mode)
	: WInput(switchPin, INPUT) {
		startTime = 0;
		longPressStartTime = 0;
		inverted = false;
		triggerProperty = nullptr;
		this->mode = mode;
		if (this->isInitialized()) {
			state = digitalRead(this->getPin());
			lastState = state;
		}
	}
	void loop(unsigned long now) {
		if (this->isInitialized()) {
			//1. Eliminate flickering input
			bool stateChanged = false;
			bool newState = digitalRead(this->getPin());
			bool expectedPegel = (this->mode == MODE_SWITCH ? !state : getOnLevel());
			unsigned long sensitiveness = (this->mode == MODE_SWITCH ? SWITCH_SENSITIVENESS : BUTTON_SENSITIVENESS);

			if ((newState != lastState) && (startTime == 0)) {
				startTime = now;
			} else if ((newState == state) && (startTime > 0)) {
				startTime = now;
			} else if ((newState != state) && (now - startTime >= sensitiveness)) {
				stateChanged = true;
			}

			lastState = newState;
			//2. If state really changed, now switch logic
			if (stateChanged) {
				state = !state;
				startTime = 0;
				if (state == expectedPegel) {
					if (this->mode != MODE_SWITCH) {
						//Button handling
						if (this->mode == MODE_BUTTON) {
							//Button
							handleButtonOrSwitchPressed();
						}
						longPressStartTime = now;
					} else {
						//Switch handling
						handleButtonOrSwitchPressed();
					}
				} else {
					if (this->mode == MODE_SWITCH) {
						//Switch handling
						handleButtonOrSwitchPressed();
					}	else if ((this->mode == MODE_BUTTON_LONG_PRESS) && (longPressStartTime >0) && (now - longPressStartTime < SWICTH_LONG_PRESS_DURATION)) {
						//Long press button was released before long press time
						handleButtonOrSwitchPressed();
						longPressStartTime = 0;
					}
				}
			} else {
				//3. If state not changed, reset trigger and handle long press buttons
				setTriggerValue(false);
				//Long press time is up
				if ((this->mode == MODE_BUTTON_LONG_PRESS) && (longPressStartTime >0) && (now - longPressStartTime >= SWICTH_LONG_PRESS_DURATION)) {
					handleLongButtonPressed();
					longPressStartTime = 0;
				}
			}





			/*bool currentState = digitalRead(this->getPin());
			if (triggerProperty != nullptr) {
				triggerProperty->setBoolean(false);
			}
			if (currentState == SWITCH_PRESSED_PEGEL) { // buttons has been pressed
				// starting timer. used for switch sensitiveness
				if (startTime == 0) {
					startTime = now;
				}
				if (now - startTime >= sensitiveness) {
					// switch pressed, sensitiveness taken into account
					if (!_pressed) {
						// This is set only once when switch is pressed
						state = !state;
						_pressed = true;
						if ((this->mode == MODE_BUTTON) || (this->mode == MODE_SWITCH)) {
							//log("Switch pressed short. pin:" + String(this->getPin()));
							if (triggerProperty != nullptr) {
								triggerProperty->setBoolean(true);
							}
							if (getProperty() != nullptr) {
								getProperty()->setBoolean(!getProperty()->getBoolean());
							}
							notify();
						}
					}
					if (this->mode == MODE_BUTTON_LONG_PRESS) {
						if (now - startTime >= longPressDuration && !_pressedLong) {
							_pressedLong = true;
							//log("Switch pressed long. pin:" + String(this->getPin()));
							notify();
						}
					}
				}
			} else if ((currentState == (!SWITCH_PRESSED_PEGEL)) && (startTime > 0)) {
				if ((_pressed) && (!_pressedLong) &&
					((this->mode == MODE_BUTTON_LONG_PRESS) || ((this->mode == MODE_SWITCH) && (now - startTime >= switchChangeDuration)))) {
					//log("Switch pressed short. pin:" + String(this->getPin()));
					if (triggerProperty != nullptr) {
						triggerProperty->setBoolean(true);
					}
					if (getProperty() != nullptr) {
						getProperty()->setBoolean(!getProperty()->getBoolean());
					}
					//notify(false);
				}
				startTime = 0;
				_pressedLong = false;
				_pressed = false;
			}*/
		}
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

	bool isInverted() {
		return inverted;
	}

	void setInverted(bool inverted) {
		this->inverted = inverted;
	}

protected:

	byte getOnLevel() {
		return (!inverted ? SWITCH_PRESSED_PEGEL : !SWITCH_PRESSED_PEGEL);
	}

	byte getOffLevel() {
		return !getOnLevel();
	}

private:
	THandlerFunction onPressed;
	THandlerFunction onLongPressed;
	byte mode;
	bool state, lastState, inverted;
	unsigned long startTime, longPressStartTime;
	WProperty* triggerProperty;

	void setTriggerValue(bool triggered) {
		if (triggerProperty != nullptr) {
			triggerProperty->setBoolean(triggered);
		}
	}

};

#endif
