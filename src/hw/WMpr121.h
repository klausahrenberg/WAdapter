#ifndef W_MPR121_H
#define W_MPR121_H

/*******************************************************************************

 Bare Conductive MPR121 library
 ------------------------------

 MPR121.h - MPR121 class header file

 Based on code by Jim Lindblom and plenty of inspiration from the Freescale
 Semiconductor datasheets and application notes.

 Bare Conductive code written by Stefan Dzisiewski-Smith and Peter Krige.

 This work is licensed under a MIT license https://opensource.org/licenses/MIT

 Copyright (c) 2016, Bare Conductive

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 *******************************************************************************/

#include "Arduino.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <Wire.h>
#include "WMpr121Defs.h"
#include "WI2C.h"

class WMpr121: public WHtu21D {
public:

  typedef std::function<void(byte pin, bool isRising)> THandlerFunction;

  WMpr121(byte address, int sda, int scl, int interrupt)
      : WHtu21D(address, sda, scl, interrupt) {
    ECR_backup = 0x00;
  	running = false;
  	error = 1 << NOT_INITED_BIT; // initially, we're not initialised
  	//touchData = 0;
  	lastTouchData = 0;
  	autoTouchStatusFlag = false;
  }

  void loop(unsigned long now) {
  	unsigned int newState = updateTouchData();
  	updateBaselineData();
  	updateFilteredData();
  	if (newState != lastTouchData) {
  		unsigned int tempState = lastTouchData;
  		lastTouchData = newState;
  		//notify every pin that goes high
  		for (int i = 0; i < 12; i++) {
  			bool newBit = readABit(lastTouchData, i);
  			if (newBit != readABit(tempState, i)) {
  				notify(i, newBit);
  			}
  		}
  	}
  }

  void onNotify(THandlerFunction fn) {
  	_callback = fn;
  }

  bool begin() {
  	error &= ~(1 << NOT_INITED_BIT); // clear NOT_INITED error flag
  	if (reset()) {
  		// default values...
  		applySettings(&defaultSettings);
  		return true;
  	} else {
  		return false;
  	}
  }

  void goSlow(){
  	Wire.setClock(100000L); // set I2C clock to 100kHz
  }

  void goFast() {
  	Wire.setClock(400000L); // set I2C clock to 400kHz
  }

  mpr121_error_t getError() {

  	// important - this resets the IRQ pin - as does any I2C comms

  	getRegister(MPR121_OORS1); // OOR registers - we may not have read them yet,
  	getRegister(MPR121_OORS2); // whereas the other errors should have been caught

  	// order of error precedence is determined in this logic block

  	if (!isInited())
  		return NOT_INITED; // this has its own checker function

  	if ((error & (1 << ADDRESS_UNKNOWN_BIT)) != 0) {
  		return ADDRESS_UNKNOWN;
  	} else if ((error & (1 << READBACK_FAIL_BIT)) != 0) {
  		return READBACK_FAIL;
  	} else if ((error & (1 << OVERCURRENT_FLAG_BIT)) != 0) {
  		return OVERCURRENT_FLAG;
  	} else if ((error & (1 << OUT_OF_RANGE_BIT)) != 0) {
  		return OUT_OF_RANGE;
  	} else
  		return NO_ERROR;
  }

  String getErrorMessage() {
  	switch (getError()) {
  	case NO_ERROR:
  		return "no error";
  	case ADDRESS_UNKNOWN:
  		return "incorrect address";
  	case READBACK_FAIL:
  		return "readback failure";
  	case OVERCURRENT_FLAG:
  		return "overcurrent on REXT pin";
  	case OUT_OF_RANGE:
  		return "electrode out of range";
  	case NOT_INITED:
  		return "not initialised";
  	default:
  		return "unknown error";
  	}
  }

  void clearError() {
  	error = 0;
  }

  bool touchStatusChanged() {
  	// :: here forces the compiler to use Arduino's digitalRead, not MPR121's
  	return (autoTouchStatusFlag || (getPin() == NO_PIN)	|| (!digitalRead(getPin())));
  }

  unsigned int updateTouchData() {
  	if (!isInited())
  		return 0;

  	autoTouchStatusFlag = false;

  	return (unsigned int) getRegister(MPR121_TS1)
  			+ ((unsigned int) getRegister(MPR121_TS2) << 8);
  }

  bool updateBaselineData() {
  	if (!isInited())
  		return (false);

  	Wire.beginTransmission(this->getAddress());
  	Wire.write(MPR121_E0BV); // set address register to read from the start of the
  	// baseline data
  	Wire.endTransmission(false); // repeated start

  	if (touchStatusChanged()) {
  		autoTouchStatusFlag = true;
  	}

  	if (Wire.requestFrom(this->getAddress(), (unsigned char) 13) == 13) {
  		for (int i = 0; i < 13; i++) { // 13 filtered values
  			if (touchStatusChanged()) {
  				autoTouchStatusFlag = true;
  			}
  			baselineData[i] = Wire.read() << 2;
  		}
  		return (true);
  	} else {
  		// if we don't get back all 26 values we requested, don't update the BVAL values
  		// and return false
  		return (false);
  	}
  }

  bool updateFilteredData() {
  	if (!isInited())
  		return (false);

  	unsigned char LSB, MSB;

  	Wire.beginTransmission(this->getAddress());
  	Wire.write(MPR121_E0FDL); // set address register to read from the start of the
  	//filtered data
  	Wire.endTransmission(false); // repeated start

  	if (touchStatusChanged()) {
  		autoTouchStatusFlag = true;
  	}

  	if (Wire.requestFrom(this->getAddress(), (unsigned char) 26) == 26) {
  		for (int i = 0; i < 13; i++) { // 13 filtered values
  			if (touchStatusChanged()) {
  				autoTouchStatusFlag = true;
  			}
  			LSB = Wire.read();
  			if (touchStatusChanged()) {
  				autoTouchStatusFlag = true;
  			}
  			MSB = Wire.read();
  			filteredData[i] = ((MSB << 8) | LSB);
  		}
  		return (true);
  	} else {
  		// if we don't get back all 26 values we requested, don't update the FDAT values
  		// and return false
  		return (false);
  	}
  }

  void updateAll() {
  	updateTouchData();
  	updateBaselineData();
  	updateFilteredData();
  }

  int getFilteredData(unsigned char electrode) {
  	if (electrode > 12 || !isInited())
  		return (0xFFFF); // avoid out of bounds behaviour
  	return (filteredData[electrode]);
  }

  int getBaselineData(unsigned char electrode) {
  	if (electrode > 12 || !isInited())
  		return (0xFFFF); // avoid out of bounds behaviour

  	return (baselineData[electrode]);
  }

  void setTouchThreshold(unsigned char val) {
  	if (!isInited())
  		return;
  	bool wasRunning = running;

  	if (wasRunning)
  		stop(); // can only change thresholds when not running
  	// checking here avoids multiple stop() / run()
  	// calls

  	for (unsigned char i = 0; i < 13; i++) {
  		setTouchThreshold(i, val);
  	}

  	if (wasRunning)
  		run();
  }

  void setTouchThreshold(unsigned char electrode, unsigned char val) {
  	if (electrode > 12 || !isInited())
  		return; // avoid out of bounds behaviour

  	// this relies on the internal register map of the MPR121
  	setRegister(MPR121_E0TTH + (electrode << 1), val);
  }

  void setReleaseThreshold(unsigned char val) {
  	if (!isInited())
  		return;
  	bool wasRunning = running;

  	if (wasRunning)
  		stop(); // can only change thresholds when not running
  	// checking here avoids multiple stop / starts

  	for (unsigned char i = 0; i < 13; i++) {
  		setReleaseThreshold(i, val);
  	}

  	if (wasRunning)
  		run();
  }

  void setReleaseThreshold(unsigned char electrode, unsigned char val) {
  	if (electrode > 12 || !isInited())
  		return; // avoid out of bounds behaviour
  	// this relies on the internal register map of the MPR121
  	setRegister(MPR121_E0RTH + (electrode << 1), val);
  }

  unsigned char getTouchThreshold(unsigned char electrode) {
  	if (electrode > 12 || !isInited())
  		return (0xFF); // avoid out of bounds behaviour
  	return (getRegister(MPR121_E0TTH + (electrode << 1))); // "255" issue is in here somewhere
  	//return(101);
  }

  unsigned char getReleaseThreshold(unsigned char electrode) {
  	if (electrode > 12 || !isInited())
  		return (0xFF); // avoid out of bounds behaviour
  	return (getRegister(MPR121_E0RTH + (electrode << 1))); // "255" issue is in here somewhere
  	//return(51);
  }

  void applySettings(MPR121_settings_t *settings) {
  	bool wasRunning = running;
  	if (wasRunning)
  		stop(); // can't change most regs when running - checking
  	// here avoids multiple stop() / run() calls

  	setRegister(MPR121_MHDR, settings->MHDR);
  	setRegister(MPR121_NHDR, settings->NHDR);
  	setRegister(MPR121_NCLR, settings->NCLR);
  	setRegister(MPR121_FDLR, settings->FDLR);
  	setRegister(MPR121_MHDF, settings->MHDF);
  	setRegister(MPR121_NHDF, settings->NHDF);
  	setRegister(MPR121_NCLF, settings->NCLF);
  	setRegister(MPR121_FDLF, settings->FDLF);
  	setRegister(MPR121_NHDT, settings->NHDT);
  	setRegister(MPR121_NCLT, settings->NCLT);
  	setRegister(MPR121_FDLT, settings->FDLT);
  	setRegister(MPR121_MHDPROXR, settings->MHDPROXR);
  	setRegister(MPR121_NHDPROXR, settings->NHDPROXR);
  	setRegister(MPR121_NCLPROXR, settings->NCLPROXR);
  	setRegister(MPR121_FDLPROXR, settings->FDLPROXR);
  	setRegister(MPR121_MHDPROXF, settings->MHDPROXF);
  	setRegister(MPR121_NHDPROXF, settings->NHDPROXF);
  	setRegister(MPR121_NCLPROXF, settings->NCLPROXF);
  	setRegister(MPR121_FDLPROXF, settings->FDLPROXF);
  	setRegister(MPR121_NHDPROXT, settings->NHDPROXT);
  	setRegister(MPR121_NCLPROXT, settings->NCLPROXT);
  	setRegister(MPR121_FDLPROXT, settings->FDLPROXT);
  	setRegister(MPR121_DTR, settings->DTR);
  	setRegister(MPR121_AFE1, settings->AFE1);
  	setRegister(MPR121_AFE2, settings->AFE2);
  	setRegister(MPR121_ACCR0, settings->ACCR0);
  	setRegister(MPR121_ACCR1, settings->ACCR1);
  	setRegister(MPR121_USL, settings->USL);
  	setRegister(MPR121_LSL, settings->LSL);
  	setRegister(MPR121_TL, settings->TL);

  	setRegister(MPR121_ECR, settings->ECR);

  	error &= ~(1 << NOT_INITED_BIT); // clear not inited error as we have just inited!
  	setTouchThreshold(settings->TTHRESH);
  	setReleaseThreshold(settings->RTHRESH);

  	if (wasRunning)
  		run();
  }

  void setRegister(unsigned char reg, unsigned char value) {

  	bool wasRunning = false;
  	;

  	if (reg == MPR121_ECR) { // if we are modding MPR121_ECR, update our internal running status
  		if (value & 0x3F) {
  			running = true;
  		} else {
  			running = false;
  		}
  	} else if (reg < MPR121_CTL0) {
  		wasRunning = running;
  		if (wasRunning)
  			stop(); // we should ALWAYS be in stop mode for this
  		// unless modding MPR121_ECR or GPIO / LED register
  	}

  	Wire.beginTransmission(this->getAddress());
  	Wire.write(reg);
  	Wire.write(value);
  	if (Wire.endTransmission() != 0) {
  		error |= 1 << ADDRESS_UNKNOWN_BIT; // set address unknown bit
  	} else {
  		error &= ~(1 << ADDRESS_UNKNOWN_BIT);
  	}

  	if (wasRunning)
  		run(); // restore run mode if necessary
  }

  unsigned char getRegister(unsigned char reg) {
  	unsigned char scratch;

  	Wire.beginTransmission(this->getAddress());
  	Wire.write(reg); // set address to read from our requested register
  	Wire.endTransmission(false); // repeated start
  	Wire.requestFrom(this->getAddress(), (unsigned char) 1); // just a single byte
  	if (Wire.endTransmission() != 0) {
  		error |= 1 << ADDRESS_UNKNOWN_BIT;
  	} else {
  		error &= ~(1 << ADDRESS_UNKNOWN_BIT);
  	}
  	scratch = Wire.read();
  	// auto update errors for registers with error data
  	if (reg == MPR121_TS2 && ((scratch & 0x80) != 0)) {
  		error |= 1 << OVERCURRENT_FLAG_BIT;
  	} else {
  		error &= ~(1 << OVERCURRENT_FLAG_BIT);
  	}
  	if ((reg == MPR121_OORS1 || reg == MPR121_OORS2) && (scratch != 0)) {
  		error |= 1 << OUT_OF_RANGE_BIT;
  	} else {
  		error &= ~(1 << OUT_OF_RANGE_BIT);
  	}
  	return scratch;
  }

  void run() {
  	if (!isInited())
  		return;
  	setRegister(MPR121_ECR, ECR_backup); // restore backup to return to run mode
  }

  void stop() {
  	if (!isInited())
  		return;
  	ECR_backup = getRegister(MPR121_ECR); // backup MPR121_ECR to restore when we enter run
  	setRegister(MPR121_ECR, ECR_backup & 0xC0); // turn off all electrodes to stop
  }

  bool reset() {
  	// return true if we successfully reset a device at the
  	// address we are expecting

  	// MPR121_AFE2 is one of the few registers that defaults to a non-zero value -
  	// checking it is sensible as reading back an incorrect value implies
  	// something went wrong - we also check MPR121_TS2 bit 7 to see if we have an
  	// overcurrent flag set

  	setRegister(MPR121_SRST, 0x63); // soft reset

  	if (getRegister(MPR121_AFE2) != 0x24) {
  		error |= 1 << READBACK_FAIL_BIT;
  	} else {
  		error &= ~(1 << READBACK_FAIL_BIT);
  	}

  	if ((getRegister(MPR121_TS2) & 0x80) != 0) {
  		error |= 1 << OVERCURRENT_FLAG_BIT;
  	} else {
  		error &= ~(1 << OVERCURRENT_FLAG_BIT);
  	}

  	if (getError() == NOT_INITED || getError() == NO_ERROR) { // if our only error is that we are not inited...
  		return true;
  	} else {
  		return false;
  	}
  }

  bool isRunning() {
  	return running;
  }

  bool isInited() {
  	return (error & (1 << NOT_INITED_BIT)) == 0;
  }

  void setProxMode(mpr121_proxmode_t mode) {

  	if (!isInited())
  		return;

  	bool wasRunning = running;

  	if (wasRunning)
  		stop();

  	switch (mode) {
  	case DISABLED_MPR:
  		ECR_backup &= ~(3 << 4); // ELEPROX[0:1] = 00
  		break;
  	case PROX0_1:
  		ECR_backup |= (1 << 4); // ELEPROX[0:1] = 01
  		ECR_backup &= ~(1 << 5);
  		break;
  	case PROX0_3:
  		ECR_backup &= ~(1 << 4); // ELEPROX[0:1] = 10
  		ECR_backup |= (1 << 5);
  		break;
  	case PROX0_11:
  		ECR_backup |= (3 << 4); // ELEPROX[0:1] = 11
  		break;
  	}

  	if (wasRunning)
  		run();
  }

  void setNumDigPins(unsigned char numPins) {
  	if (!isInited())
  		return;
  	bool wasRunning = running;

  	if (numPins > 8)
  		numPins = 8; // maximum number of GPIO pins is 8 out of 12

  	if (wasRunning) {
  		stop(); // have to stop to change MPR121_ECR
  	}
  	ECR_backup = (0x0F & (12 - numPins)) | (ECR_backup & 0xF0);
  	if (wasRunning) {
  		run();
  	}

  }

  void pinMode(unsigned char electrode, mpr121_pinf_t mode) {

  	// only valid for ELE4..ELE11
  	if (electrode < 4 || electrode > 11 || !isInited())
  		return;

  	// LED0..LED7
  	unsigned char bitmask = 1 << (electrode - 4);

  	switch (mode) {
  	case INPUT_PU:
  		// MPR121_EN = 1
  		// MPR121_DIR = 0
  		// MPR121_CTL0 = 1
  		// MPR121_CTL1 = 1
  		setRegister(MPR121_EN, getRegister(MPR121_EN) | bitmask);
  		setRegister(MPR121_DIR, getRegister(MPR121_DIR) & ~bitmask);
  		setRegister(MPR121_CTL0, getRegister(MPR121_CTL0) | bitmask);
  		setRegister(MPR121_CTL1, getRegister(MPR121_CTL1) | bitmask);
  		break;
  	case INPUT_PD:
  		// MPR121_EN = 1
  		// MPR121_DIR = 0
  		// MPR121_CTL0 = 1
  		// MPR121_CTL1 = 0
  		setRegister(MPR121_EN, getRegister(MPR121_EN) | bitmask);
  		setRegister(MPR121_DIR, getRegister(MPR121_DIR) & ~bitmask);
  		setRegister(MPR121_CTL0, getRegister(MPR121_CTL0) | bitmask);
  		setRegister(MPR121_CTL1, getRegister(MPR121_CTL1) & ~bitmask);
  		break;
  	case OUTPUT_HS:
  		// MPR121_EN = 1
  		// MPR121_DIR = 1
  		// MPR121_CTL0 = 1
  		// MPR121_CTL1 = 1
  		setRegister(MPR121_EN, getRegister(MPR121_EN) | bitmask);
  		setRegister(MPR121_DIR, getRegister(MPR121_DIR) | bitmask);
  		setRegister(MPR121_CTL0, getRegister(MPR121_CTL0) | bitmask);
  		setRegister(MPR121_CTL1, getRegister(MPR121_CTL1) | bitmask);
  		break;
  	case OUTPUT_LS:
  		// MPR121_EN = 1
  		// MPR121_DIR = 1
  		// MPR121_CTL0 = 1
  		// MPR121_CTL1 = 0
  		setRegister(MPR121_EN, getRegister(MPR121_EN) | bitmask);
  		setRegister(MPR121_DIR, getRegister(MPR121_DIR) | bitmask);
  		setRegister(MPR121_CTL0, getRegister(MPR121_CTL0) | bitmask);
  		setRegister(MPR121_CTL1, getRegister(MPR121_CTL1) & ~bitmask);
  		break;
  	}
  }

  void pinMode(unsigned char electrode, int mode) {
  	if (!isInited())
  		return;

  	// this is to catch the fact that Arduino prefers its definition of INPUT
  	// and OUTPUT to ours...

  	unsigned char bitmask = 1 << (electrode - 4);

  	if (mode == OUTPUT) {
  		// MPR121_EN = 1
  		// MPR121_DIR = 1
  		// MPR121_CTL0 = 0
  		// MPR121_CTL1 = 0
  		setRegister(MPR121_EN, getRegister(MPR121_EN) | bitmask);
  		setRegister(MPR121_DIR, getRegister(MPR121_DIR) | bitmask);
  		setRegister(MPR121_CTL0, getRegister(MPR121_CTL0) & ~bitmask);
  		setRegister(MPR121_CTL1, getRegister(MPR121_CTL1) & ~bitmask);

  	} else if (mode == INPUT) {
  		// MPR121_EN = 1
  		// MPR121_DIR = 0
  		// MPR121_CTL0 = 0
  		// MPR121_CTL1 = 0
  		setRegister(MPR121_EN, getRegister(MPR121_EN) | bitmask);
  		setRegister(MPR121_DIR, getRegister(MPR121_DIR) & ~bitmask);
  		setRegister(MPR121_CTL0, getRegister(MPR121_CTL0) & ~bitmask);
  		setRegister(MPR121_CTL1, getRegister(MPR121_CTL1) & ~bitmask);
  	} else {
  		return; // anything that isn't a 1 or 0 is invalid
  	}
  }

  void digitalWrite(unsigned char electrode, unsigned char val) {
  	// avoid out of bounds behaviour
  	if (electrode < 4 || electrode > 11 || !isInited())
  		return;
  	if (val) {
  		setRegister(MPR121_SET, 1 << (electrode - 4));
  	} else {
  		setRegister(MPR121_CLR, 1 << (electrode - 4));
  	}
  }

  // Toggles electrode set as GPIO output
  void digitalToggle(unsigned char electrode) {
  	// avoid out of bounds behaviour
  	if (electrode < 4 || electrode > 11 || !isInited())
  		return;
  	setRegister(MPR121_TOG, 1 << (electrode - 4));
  }

  // Reads electrode set as GPIO input
  bool digitalRead(unsigned char electrode) {
  	// avoid out of bounds behaviour
  	if (electrode < 4 || electrode > 11 || !isInited())
  		return false;
  	return (((getRegister(MPR121_DAT) >> (electrode - 4)) & 1) == 1);
  }

  // Writes PWM value to electrode set as GPIO output - very limited
  // (4 bit, although input to function is 0..255 to match Arduino,
  // internally reduced to 4 bit) and broken on ELE9 and ELE10
  // see https://community.freescale.com/thread/305474
  void analogWrite(unsigned char electrode, unsigned char value) {
  	// LED output 5 (ELE9) and output 6 (ELE10) have a PWM bug
  	// https://community.nxp.com/thread/305474

  	// avoid out of bounds behaviour

  	if (electrode < 4 || electrode > 11 || !isInited())
  		return;

  	unsigned char shiftedVal = value >> 4;

  	if (shiftedVal > 0) {
  		setRegister(MPR121_SET, 1 << (electrode - 4)); // normal PWM operation
  	} else {
  		// this make a 0 PWM setting turn off the output
  		setRegister(MPR121_CLR, 1 << (electrode - 4));
  	}

  	unsigned char scratch;

  	switch (electrode - 4) {

  	case 0:
  		scratch = getRegister(MPR121_PWM0);
  		setRegister(MPR121_PWM0, (shiftedVal & 0x0F) | (scratch & 0xF0));
  		break;
  	case 1:
  		scratch = getRegister(MPR121_PWM0);
  		setRegister(MPR121_PWM0, ((shiftedVal & 0x0F) << 4) | (scratch & 0x0F));
  		break;
  	case 2:
  		scratch = getRegister(MPR121_PWM1);
  		setRegister(MPR121_PWM1, (shiftedVal & 0x0F) | (scratch & 0xF0));
  		break;
  	case 3:
  		scratch = getRegister(MPR121_PWM1);
  		setRegister(MPR121_PWM1, ((shiftedVal & 0x0F) << 4) | (scratch & 0x0F));
  		break;
  	case 4:
  		scratch = getRegister(MPR121_PWM2);
  		setRegister(MPR121_PWM2, (shiftedVal & 0x0F) | (scratch & 0xF0));
  		break;
  	case 5:
  		scratch = getRegister(MPR121_PWM2);
  		setRegister(MPR121_PWM2, ((shiftedVal & 0x0F) << 4) | (scratch & 0x0F));
  		break;
  	case 6:
  		scratch = getRegister(MPR121_PWM3);
  		setRegister(MPR121_PWM3, (shiftedVal & 0x0F) | (scratch & 0xF0));
  		break;
  	case 7:
  		scratch = getRegister(MPR121_PWM3);
  		setRegister(MPR121_PWM3, ((shiftedVal & 0x0F) << 4) | (scratch & 0x0F));
  		break;
  	}
  }

  // Sets the sample period of the MPR121 - the time between capacitive
  // readings. Higher values consume less power, but are less responsive.
  void setSamplePeriod(mpr121_sample_interval_t period) {

  	unsigned char scratch;

  	scratch = getRegister(MPR121_AFE2);
  	setRegister(MPR121_AFE2, (scratch & 0xF8) | (period & 0x07));
  }


private:
  MPR121_settings_t defaultSettings;
  THandlerFunction _callback;
  unsigned char ECR_backup; // so we can re-enable the correct number of electrodes
  // when recovering from stop mode
  unsigned char error;
  bool running;

  int filteredData[13];
  int baselineData[13];
  //unsigned int touchData;
  unsigned int lastTouchData;
  //bool getLastTouchData(unsigned char electrode);
  bool autoTouchStatusFlag; // we use this to catch touch / release events that happen
  // during other update calls

  void notify(byte pin, bool isRising) {
  	if (_callback) {
  		_callback(pin, isRising);
  	}
  }

  bool readABit(unsigned int data, byte bit) {
  	unsigned int mask = 0b0000000000000001;
  	mask <<= bit;
  	return ((data & mask) != 0x00);
  }

};

#endif
