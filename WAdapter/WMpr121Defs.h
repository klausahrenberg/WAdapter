/*******************************************************************************

 Bare Conductive MPR121 library
 ------------------------------

 MPR121_defs.h - MPR121 register definitions

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

#ifndef W_MPR121_DEFS_H
#define W_MPR121_DEFS_H

#define NOT_INITED_BIT 0
#define ADDRESS_UNKNOWN_BIT 1
#define READBACK_FAIL_BIT 2
#define OVERCURRENT_FLAG_BIT 3
#define OUT_OF_RANGE_BIT 4

// idea behind this is to create a settings structure that we can use to store
// all the setup variables for a particular setup - comes pre-instantiated with
// defaults and can be easily tweaked - we pass by reference (as a pointer) to
// save RAM

struct MPR121_settings_t {
    // touch and release thresholds
    unsigned char TTHRESH;
    unsigned char RTHRESH;

    unsigned char INTERRUPT_MPR;

    // general electrode touch sense baseline filters
    // rising filter
    unsigned char MHDR;
    unsigned char NHDR;
    unsigned char NCLR;
    unsigned char FDLR;

    // falling filter
    unsigned char MHDF;
    unsigned char NHDF;
    unsigned char NCLF;
    unsigned char FDLF;

    // touched filter
    unsigned char NHDT;
    unsigned char NCLT;
    unsigned char FDLT;

    // proximity electrode touch sense baseline filters
    // rising filter
    unsigned char MHDPROXR;
    unsigned char NHDPROXR;
    unsigned char NCLPROXR;
    unsigned char FDLPROXR;

    // falling filter
    unsigned char MHDPROXF;
    unsigned char NHDPROXF;
    unsigned char NCLPROXF;
    unsigned char FDLPROXF;

    // touched filter
    unsigned char NHDPROXT;
    unsigned char NCLPROXT;
    unsigned char FDLPROXT;

    // debounce settings
    unsigned char DTR;

    // configuration registers
    unsigned char AFE1;
    unsigned char AFE2;
    unsigned char ECR;

    // auto-configuration registers
    unsigned char ACCR0;
    unsigned char ACCR1;
    unsigned char USL;
    unsigned char LSL;
    unsigned char TL;

    // default values in initialisation list

    MPR121_settings_t() :
    TTHRESH(40),
    RTHRESH(20),
    INTERRUPT_MPR(4), // note that this is not a hardware interrupt, just the digital
    // pin that the MPR121 ~INT pin is connected to
    MHDR(0x01),
    NHDR(0x01),
    NCLR(0x10),
    FDLR(0x20),
    MHDF(0x01),
    NHDF(0x01),
    NCLF(0x10),
    FDLF(0x20),
    NHDT(0x01),
    NCLT(0x10),
    FDLT(0xFF),
    MHDPROXR(0x0F),
    NHDPROXR(0x0F),
    NCLPROXR(0x00),
    FDLPROXR(0x00),
    MHDPROXF(0x01),
    NHDPROXF(0x01),
    NCLPROXF(0xFF),
    FDLPROXF(0xFF),
    NHDPROXT(0x00),
    NCLPROXT(0x00),
    FDLPROXT(0x00),
    DTR(0x11),
    AFE1(0xFF),
    AFE2(0x30),
    ECR(0xCC), // default to fast baseline startup and 12 electrodes enabled, no prox
    ACCR0(0x00),
    ACCR1(0x00),
    USL(0x00),
    LSL(0x00),
    TL(0x00) {
    }

};

// GPIO pin function constants

enum mpr121_pinf_t {
    // INPUT and OUTPUT are already defined by Arduino, use its definitions

    //INPUT,		// digital input
    INPUT_PU, // digital input with pullup
    INPUT_PD, // digital input with pulldown
    //OUTPUT,		// digital output (push-pull)
    OUTPUT_HS, // digital output, open collector (high side)
    OUTPUT_LS // digital output, open collector (low side)
};

// "13th electrode" proximity modes
// N.B. this does not relate to normal proximity detection
// see http://cache.freescale.com/files/sensors/doc/app_note/AN3893.pdf

enum mpr121_proxmode_t {
    DISABLED_MPR, // proximity mode disabled
    PROX0_1, // proximity mode for ELE0..ELE1
    PROX0_3, // proximity mode for ELE0..ELE3
    PROX0_11 // proximity mode for ELE0..ELE11
};

// error codes

enum mpr121_error_t {
    NO_ERROR, // no error
    RETURN_TO_SENDER, // not implemented
    ADDRESS_UNKNOWN, // no MPR121 found at specified I2C address
    READBACK_FAIL, // readback from MPR121 was not as expected
    OVERCURRENT_FLAG, // overcurrent on REXT pin
    OUT_OF_RANGE, // autoconfiguration fail, often a result of shorted pins
    NOT_INITED // device has not been initialised
};

// sample intervals

enum mpr121_sample_interval_t {
    SAMPLE_INTERVAL_1MS = 0x00,
    SAMPLE_INTERVAL_2MS = 0x01,
    SAMPLE_INTERVAL_4MS = 0x02,
    SAMPLE_INTERVAL_8MS = 0x03,
    SAMPLE_INTERVAL_16MS = 0x04,
    SAMPLE_INTERVAL_32MS = 0x05,
    SAMPLE_INTERVAL_64MS = 0x06,
    SAMPLE_INTERVAL_128MS = 0x07
};

// MPR121 Register Defines

// touch and OOR statuses
static unsigned char const MPR121_TS1 = 0x00;
static unsigned char const MPR121_TS2 = 0x01;
static unsigned char const MPR121_OORS1 = 0x02;
static unsigned char const MPR121_OORS2 = 0x03;

// filtered data
static unsigned char const MPR121_E0FDL = 0x04;
static unsigned char const MPR121_E0FDH = 0x05;
static unsigned char const MPR121_E1FDL = 0x06;
static unsigned char const MPR121_E1FDH = 0x07;
static unsigned char const MPR121_E2FDL = 0x08;
static unsigned char const MPR121_E2FDH = 0x09;
static unsigned char const MPR121_E3FDL = 0x0A;
static unsigned char const MPR121_E3FDH = 0x0B;
static unsigned char const MPR121_E4FDL = 0x0C;
static unsigned char const MPR121_E4FDH = 0x0D;
static unsigned char const MPR121_E5FDL = 0x0E;
static unsigned char const MPR121_E5FDH = 0x0F;
static unsigned char const MPR121_E6FDL = 0x10;
static unsigned char const MPR121_E6FDH = 0x11;
static unsigned char const MPR121_E7FDL = 0x12;
static unsigned char const MPR121_E7FDH = 0x13;
static unsigned char const MPR121_E8FDL = 0x14;
static unsigned char const MPR121_E8FDH = 0x15;
static unsigned char const MPR121_E9FDL = 0x16;
static unsigned char const MPR121_E9FDH = 0x17;
static unsigned char const MPR121_E10FDL = 0x18;
static unsigned char const MPR121_E10FDH = 0x19;
static unsigned char const MPR121_E11FDL = 0x1A;
static unsigned char const MPR121_E11FDH = 0x1B;
static unsigned char const MPR121_E12FDL = 0x1C;
static unsigned char const MPR121_E12FDH = 0x1D;

// baseline values
static unsigned char const MPR121_E0BV = 0x1E;
static unsigned char const MPR121_E1BV = 0x1F;
static unsigned char const MPR121_E2BV = 0x20;
static unsigned char const MPR121_E3BV = 0x21;
static unsigned char const MPR121_E4BV = 0x22;
static unsigned char const MPR121_E5BV = 0x23;
static unsigned char const MPR121_E6BV = 0x24;
static unsigned char const MPR121_E7BV = 0x25;
static unsigned char const MPR121_E8BV = 0x26;
static unsigned char const MPR121_E9BV = 0x27;
static unsigned char const MPR121_E10BV = 0x28;
static unsigned char const MPR121_E11BV = 0x29;
static unsigned char const MPR121_E12BV = 0x2A;

// general electrode touch sense baseline filters
// rising filter
static unsigned char const MPR121_MHDR = 0x2B;
static unsigned char const MPR121_NHDR = 0x2C;
static unsigned char const MPR121_NCLR = 0x2D;
static unsigned char const MPR121_FDLR = 0x2E;

// falling filter
static unsigned char const MPR121_MHDF = 0x2F;
static unsigned char const MPR121_NHDF = 0x30;
static unsigned char const MPR121_NCLF = 0x31;
static unsigned char const MPR121_FDLF = 0x32;

// touched filter
static unsigned char const MPR121_NHDT = 0x33;
static unsigned char const MPR121_NCLT = 0x34;
static unsigned char const MPR121_FDLT = 0x35;

// proximity electrode touch sense baseline filters
// rising filter
static unsigned char const MPR121_MHDPROXR = 0x36;
static unsigned char const MPR121_NHDPROXR = 0x37;
static unsigned char const MPR121_NCLPROXR = 0x38;
static unsigned char const MPR121_FDLPROXR = 0x39;

// falling filter
static unsigned char const MPR121_MHDPROXF = 0x3A;
static unsigned char const MPR121_NHDPROXF = 0x3B;
static unsigned char const MPR121_NCLPROXF = 0x3C;
static unsigned char const MPR121_FDLPROXF = 0x3D;

// touched filter
static unsigned char const MPR121_NHDPROXT = 0x3E;
static unsigned char const MPR121_NCLPROXT = 0x3F;
static unsigned char const MPR121_FDLPROXT = 0x40;

// electrode touch and release thresholds
static unsigned char const MPR121_E0TTH = 0x41;
static unsigned char const MPR121_E0RTH = 0x42;
static unsigned char const MPR121_E1TTH = 0x43;
static unsigned char const MPR121_E1RTH = 0x44;
static unsigned char const MPR121_E2TTH = 0x45;
static unsigned char const MPR121_E2RTH = 0x46;
static unsigned char const MPR121_E3TTH = 0x47;
static unsigned char const MPR121_E3RTH = 0x48;
static unsigned char const MPR121_E4TTH = 0x49;
static unsigned char const MPR121_E4RTH = 0x4A;
static unsigned char const MPR121_E5TTH = 0x4B;
static unsigned char const MPR121_E5RTH = 0x4C;
static unsigned char const MPR121_E6TTH = 0x4D;
static unsigned char const MPR121_E6RTH = 0x4E;
static unsigned char const MPR121_E7TTH = 0x4F;
static unsigned char const MPR121_E7RTH = 0x50;
static unsigned char const MPR121_E8TTH = 0x51;
static unsigned char const MPR121_E8RTH = 0x52;
static unsigned char const MPR121_E9TTH = 0x53;
static unsigned char const MPR121_E9RTH = 0x54;
static unsigned char const MPR121_E10TTH = 0x55;
static unsigned char const MPR121_E10RTH = 0x56;
static unsigned char const MPR121_E11TTH = 0x57;
static unsigned char const MPR121_E11RTH = 0x58;
static unsigned char const MPR121_E12TTH = 0x59;
static unsigned char const MPR121_E12RTH = 0x5A;

// debounce settings
static unsigned char const MPR121_DTR = 0x5B;

// configuration registers
static unsigned char const MPR121_AFE1 = 0x5C;
static unsigned char const MPR121_AFE2 = 0x5D;
static unsigned char const MPR121_ECR = 0x5E;

// electrode currents
static unsigned char const MPR121_CDC0 = 0x5F;
static unsigned char const MPR121_CDC1 = 0x60;
static unsigned char const MPR121_CDC2 = 0x61;
static unsigned char const MPR121_CDC3 = 0x62;
static unsigned char const MPR121_CDC4 = 0x63;
static unsigned char const MPR121_CDC5 = 0x64;
static unsigned char const MPR121_CDC6 = 0x65;
static unsigned char const MPR121_CDC7 = 0x66;
static unsigned char const MPR121_CDC8 = 0x67;
static unsigned char const MPR121_CDC9 = 0x68;
static unsigned char const MPR121_CDC10 = 0x69;
static unsigned char const MPR121_CDC11 = 0x6A;
static unsigned char const MPR121_CDCPROX = 0x6B;

// electrode charge times
static unsigned char const MPR121_CDT01 = 0x6C;
static unsigned char const MPR121_CDT23 = 0x6D;
static unsigned char const MPR121_CDT45 = 0x6E;
static unsigned char const MPR121_CDT67 = 0x6F;
static unsigned char const MPR121_CDT89 = 0x70;
static unsigned char const MPR121_CDT1011 = 0x71;
static unsigned char const MPR121_CDTPROX = 0x72;

// GPIO
static unsigned char const MPR121_CTL0 = 0x73;
static unsigned char const MPR121_CTL1 = 0x74;
static unsigned char const MPR121_DAT = 0x75;
static unsigned char const MPR121_DIR = 0x76;
static unsigned char const MPR121_EN = 0x77;
static unsigned char const MPR121_SET = 0x78;
static unsigned char const MPR121_CLR = 0x79;
static unsigned char const MPR121_TOG = 0x7A;

// auto-config
static unsigned char const MPR121_ACCR0 = 0x7B;
static unsigned char const MPR121_ACCR1 = 0x7C;
static unsigned char const MPR121_USL = 0x7D;
static unsigned char const MPR121_LSL = 0x7E;
static unsigned char const MPR121_TL = 0x7F;

// soft reset
static unsigned char const MPR121_SRST = 0x80;

// PWM
static unsigned char const MPR121_PWM0 = 0x81;
static unsigned char const MPR121_PWM1 = 0x82;
static unsigned char const MPR121_PWM2 = 0x83;
static unsigned char const MPR121_PWM3 = 0x84;

#endif
