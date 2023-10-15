#ifndef W_PT2314_h
#define W_PT2314_h

/* Arduino PT2314 Library
 * Copyright (C) 2013 by Andy Karpov <andy.karpov@gmail.com>
 *
 * This file is part of the Arduino PT2314 Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * <http://www.gnu.org/licenses/>.
 */

#include "Wire.h"
#include "../WProperty.h"
#include "../WI2C.h"

#define PT2314_I2C_ADDRESS 0x44

#define PT2314_BASS_TONE_CONTROL 0x60    // 0b01100000
#define PT2314_TREBLE_TONE_CONTROL 0x70  // 0b01110000

#define PT2314_AUDIO_BYTE 0x58
#define PT2314_AUDIO_BYTE_LOUD_ON 0x00
#define PT2314_AUDIO_BYTE_LOAD_OFF 0x04

#define PT2314_SPEAKER_ATTN_L 0xc0  // 0b11000000
#define PT2314_SPEAKER_ATTN_R 0xe0  // 0b11100000
#define PT2314_SPEAKER_ATTN_L_MUTE 0xdf
#define PT2314_SPEAKER_ATTN_R_MUTE 0xff

unsigned char eq_map[] = {
  0b0000, // -7
  0b0001, // -6
  0b0010, // -5
  0b0011, // -4
  0b0100, // -3
  0b0101, // -2
  0b0110, // -1
  0b0111, //  0
  0b1110, // +1
  0b1101, // +2
  0b1100, // +3
  0b1011, // +4
  0b1010, // +5
  0b1001, // +6
  0b1000, // +7
};

class PT2314 : public WI2C{
 public:
 PT2314(int sda = 21, int scl = 22, TwoWire* i2cPort = &Wire)
			: WI2C(PT2314_I2C_ADDRESS, sda, scl, NO_PIN, i2cPort) {  
    _bass = WProps::createLevelIntProperty("bass", "Bass", -7, 7);
    _bass->setInteger(0);
    _bass->addListener([this] (WProperty* p) { _updateBass(); }); 
    _treble = WProps::createLevelIntProperty("treble", "Treble", -7, 7);
    _treble->setInteger(0);
    _treble->addListener([this] (WProperty* p) { _updateTreble(); }); 

    _loudness = WProps::createBooleanProperty("loudness", "Loudness");
    _loudness->setBoolean(false);
    _loudness->addListener([this] (WProperty* p) { _updateAudioSwitch(); });
    _gain = WProps::createByteProperty("gain", "Gain");
    _gain->setByte(0);
    _gain->addListener([this] (WProperty* p) { _updateAudioSwitch(); });

    _channel = WProps::createByteProperty("channel", "Channel");
    _channel->setByte(0);
    _channel->addListener([this] (WProperty* p) { _updateAudioSwitch(); });

    _volume = WProps::createLevelIntProperty("volume", "Volume", 0, 100);
    _volume->setInteger(100);
    _volume->addListener([this] (WProperty* p) { _updateVolume(); }); 
    _mute = WProps::createBooleanProperty("mute", "Mute");
    _mute->setBoolean(false);
    _mute->addListener([this] (WProperty* p) { _updateAttenuation(); _updateVolume(); });

    _attenuationL = 100;
    _attenuationR = 100;      
    if (isInitialized()) {
      _updateAll();
    }
  }

  WProperty* mute() { return _mute; }

  WProperty* volume() { return _volume; }

  WProperty* channel() { return _channel; }

  WProperty* loudness() { return _loudness; }

  WProperty* gain() { return _gain; }

  void attenuation(int l, int r) {
    _attenuationL = constrain(l, 0, 100);
    _attenuationR = constrain(r, 0, 100);
    _updateAttenuation();
  }

  WProperty* bass() { return _bass; }

  WProperty* treble() { return _treble; }

 private:
  WProperty* _volume;
  int _attenuationL;
  int _attenuationR;
  WProperty* _mute;
  WProperty* _loudness;
  WProperty* _gain;
  WProperty* _channel;
  WProperty* _bass;
  WProperty* _treble;

  int volume_to_pt2314(int vol) { return 63 - ((vol * 63) / 100); }

  int writeI2CChar(unsigned char c) {
    Wire.beginTransmission(PT2314_I2C_ADDRESS);
    Wire.write(c);
    return Wire.endTransmission();
  }

  bool _updateVolume() {
    unsigned int val = volume_to_pt2314(_volume->getInteger());
    return (writeI2CChar(val) == 0) ? true : false;
  }

  bool _updateAttenuation() {
    unsigned int aL = map(_attenuationL, 0, 100, 0b00011111, 0b00000000);
    unsigned int aR = map(_attenuationR, 0, 100, 0b00011111, 0b00000000);
    if (_mute->getBoolean()) {
      if (writeI2CChar(0b11011111) != 0) {
        return false;
      }
      if (writeI2CChar(0b11111111) != 0) {
        return false;
      }
    } else {
      if (writeI2CChar(0b11000000 | aL) != 0) {
        return false;
      }
      if (writeI2CChar(0b11100000 | aR) != 0) {
        return false;
      }
    }

    return true;
  }

  bool _updateAudioSwitch() {
  /*
    AUDIO SWITCH DATA BYTE
    MSB LSB Function
     0  1  0 G1 G0 S2 S1 S0 Audio Switch
                       0  0 Stereo 1
                       0  1 Stereo 2
                       1  0 Stereo 3
                       1  1 Stereo 4
                    0       Loudness ON
                    1       Loudness OFF
              0  0          +11.25dB
              0  1          +7.5dB
              1  0          +3.75dB
              1  1          0dB
    */

    int audioByte = 0b01000000;  // audio switch + gain +11.25dB.
    // gain byte, 0b00011000 = no gain, 0b00010000 = +3.75dB, 0b00001000 =
    // +7.5dB, 0b00000000 = +11.25dB
    byte g = ((3 - _gain->getByte()) << 3);
    audioByte |= g;
    if (_loudness->getBoolean()) {
      audioByte |= 0b00000000;
    } else {
      audioByte |= 0b00000100;
    }
    audioByte |= _channel->getByte();
    return (writeI2CChar(audioByte) == 0) ? true : false;
  }

  bool _updateBass() {
    unsigned int val = eq_map[_bass->getInteger()];// eq_to_pt2314(map(_bass->getInteger(), -10, 10, 0, 28));
    return (writeI2CChar(0x60 | val) == 0) ? true : false;
  }

  bool _updateTreble() {
    unsigned int val = eq_map[_treble->getInteger()];// eq_to_pt2314(map(_treble->getInteger(), -10, 10, 0, 28));
    return (writeI2CChar(0x70 | val) == 0) ? true : false;
  }

  bool _updateAll() {
    if (_updateVolume() == false) {
      return false;
    }

    if (_updateAttenuation() == false) {
      return false;
    }

    if (_updateAudioSwitch() == false) {
      return false;
    }

    if (_updateBass() == false) {
      return false;
    }

    return _updateTreble();
  }
};

#endif
