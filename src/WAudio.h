#ifndef W_WAUDIO_H
#define W_WAUDIO_H

#include "Audio.h"
#include "WProperty.h"

// Forward declarations
class WAudio;
WAudio* wAudio = nullptr;
void audioTaskEvent(Audio::msg_t m);

class WAudio : public Audio, public WGpio {
 public:
  WAudio(byte pinBck, byte pinLrc, byte pinDout) : Audio(I2S_NUM_0), WGpio(GPIO_TYPE_LED, NO_PIN, OUTPUT, nullptr) {
    wAudio = this;
    Audio::audio_info_callback = audioTaskEvent;
    Audio::setAudioTaskCore(0);
    Audio::setVolume(21);
    _initialized = Audio::setPinout(pinBck, pinLrc, pinDout);
  }

  virtual void loop(unsigned long now) {
    WGpio::loop(now);
    if (isOn()) {
      Audio::loop();
      vTaskDelay(1);
    }
  }

  virtual bool isPending() {
    return _pending;
  }

  void audioEvent(Audio::msg_t m) {
    // Zeigt die rohe Nachricht im Seriellen Monitor an (ähnlich wie das alte audio_info)
    Serial.printf("Event: %s -> %s (ID: %d)\n", m.s, m.msg, m.e);

    // Filter nach den spezifischen Event-IDs (m.e)
    switch (m.e) {
      case Audio::evt_name:  // Entspricht dem alten audio_showstation
        Serial.printf(">>> Radiosender Name: %s <<<\n", m.msg);
        _pending = false;
        break;

      case Audio::evt_streamtitle:  // Entspricht dem alten audio_showstreamtitle
        Serial.printf(">>> Aktueller Song: %s <<<\n", m.msg);
        break;

      // 3. Bitrate abfangen (Optional)
      case Audio::evt_bitrate:
        Serial.printf("Stream-Qualitaet: %s\n", m.msg);
        break;

      // 4. Stream-URL des Senders abfangen (Optional)
      case Audio::evt_icyurl:
        Serial.printf("Sender-Homepage: %s\n", m.msg);
        break;
    }
  }

 protected:
  virtual bool _isInitialized() { return _initialized; }

  virtual void _updateOn() {
    WGpio::_updateOn();
    if (isOn()) {
      //if (!_starting) {
      //  _starting = true;
        LOG->debug("radio on..");
        // play
        if (NETWORK->isWifiConnected()) {
          LOG->debug("radio on...");

          log_w("a) radio gaga");
          //_tuner = new WAudio();

          // this->radio->init(WM8978_I2S_BCK, WM8978_I2S_WS, WM8978_I2S_DOUT, WM8978_I2S_MCLKPIN);
          //_tuner->setPinout(PIN_DAC_BCK, PIN_DAC_LRC, PIN_DAC_DOUT);
          if (pin() != NO_PIN) {
            LOG->debug("b) XSMT on");
            WGpio::writeOutput(pin(), HIGH);
          }
          delay(100);
          if (!Audio::connecttohost("http://onefm.ice.infomaniak.ch/onefm-high.mp3")) {
            // if (!play("http://onefm.ice.infomaniak.ch/onefm-high.mp3")) {
            // if (!play("http://stream.104.6rtl.com/rtl-live/mp3-192/play.m3u")) {  // this->getStationUrl(station->enumIndex())->asString())) {
            NETWORK->debug(F("Can't connect to station"));
          } else {
            log_w("b) should play");
            _pending = true;
          }
          log_w("c) radio gaga");
          setVolume(20);
        }
      //  _starting = false;
      //}
    } else {
      LOG->debug("radio off.");
      // if (_tuner != nullptr) {
      Audio::stopSong();
      //_starting = false;
      if (pin() != NO_PIN) {
        WGpio::writeOutput(pin(), LOW);
      }
      delay(50);
      //}
    }
  }

 private:
  //bool _starting = false;
  bool _initialized = false;
  bool _pending = false;
};

void audioTaskEvent(Audio::msg_t m) {
  wAudio->audioEvent(m);
}

#endif
