#ifndef W_SWITCH_H_
#define W_SWITCH_H_

#include "WPin.h"

#define MODE_BUTTON 0
#define MODE_BUTTON_LONG_PRESS 1
#define MODE_SWITCH 2
#define SWITCH_PRESSED_PEGEL HIGH

const unsigned long SWITCH_SENSITIVENESS = 200;
const unsigned long SWICTH_LONG_PRESS_DURATION = 5000;


class WSwitch: public WPin {
public:
	typedef std::function<void()> THandlerFunction;
	WSwitch(int switchPin, byte mode)
	: WPin(switchPin, INPUT) {
		startTime = 0;
		longPressStartTime =0;
		//_pressed = false;
		//_pressedLong = false;
		triggerProperty = nullptr;
		this->mode = mode;
		//longPressDuration = 5000;
		//switchChangeDuration = 1000;
		if (this->isInitialized()) {
			state = digitalRead(this->getPin());
			lastState = state;
			/*if (state == SWITCH_PRESSED_PEGEL) {
				_pressed = true;
			}*/
		}

	}
	void loop(unsigned long now) {
		if (this->isInitialized()) {
			//1. Eliminate flickering input
			bool stateChanged = false;
			bool newState = digitalRead(this->getPin());
			bool expectedPegel = !state;

			if ((newState != lastState) && (startTime == 0)) {
				startTime = now;
			} else if ((newState == state) && (startTime > 0)) {
				startTime = now;
			} else if ((newState != state) && (now - startTime >= SWITCH_SENSITIVENESS)) {
				stateChanged = true;
			}

			lastState = newState;
			/*if (newState != lastState) {
				if (startTime > 0) {
					if (newState != state) {
						stateChanged = (now - startTime >= SWITCH_SENSITIVENESS);
					} else {
						//flickering signal, reset start time
						startTime = now;
					}
				} else {
					startTime = now;
				}
				lastState = newState;
			} else if (startTime > 0) {
				stateChanged = ((newState != state) && (now - startTime >= SWITCH_SENSITIVENESS));
			}*/
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
					}	else if (this->mode == MODE_BUTTON_LONG_PRESS) {
						if (now - longPressStartTime >= SWICTH_LONG_PRESS_DURATION) {
							handleLongButtonPressed();
						} else {
							handleButtonOrSwitchPressed();
						}
					}
					//Button handling
					longPressStartTime = 0;
				}
			} else {
				setTriggerValue(false);
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
				if (now - startTime >= SWITCH_SENSITIVENESS) {
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

private:
	THandlerFunction onPressed;
	THandlerFunction onLongPressed;
	byte mode;
	//int longPressDuration, switchChangeDuration;
	bool state, lastState;
	unsigned long startTime, longPressStartTime;
	//bool _pressed;
	//bool _pressedLong;
	WProperty* triggerProperty;

	void setTriggerValue(bool triggered) {
		if (triggerProperty != nullptr) {
			triggerProperty->setBoolean(triggered);
		}
	}

};

#endif
