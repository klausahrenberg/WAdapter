#ifndef W_WAUDIO_H
#define W_WAUDIO_H

#include "Audio.h"
#include "WProperty.h"

#define AUDIO_RETRY_DELAY 5000
#define AUDIO_CONNECT_TIMEOUT 20000

// Forward declarations
class WAudio;
WAudio* wAudio = nullptr;
void audioTaskEvent(Audio::msg_t m);

class WAudio : public Audio, public WGpio {
 public:
  //names the url to play. asked every time the audio starts a station, so the
  //audio itself needs to know nothing about where the stations are kept
  typedef std::function<const char*()> TOnStationUrl;

  WAudio(WNetwork* network, byte pinBck, byte pinLrc, byte pinDout) : Audio(I2S_NUM_0), WGpio(GPIO_TYPE_LED, NO_PIN, OUTPUT, nullptr) {
    wAudio = this;
    _network = network;
    Audio::audio_info_callback = audioTaskEvent;
    Audio::setAudioTaskCore(0);
    Audio::setVolume(21);
    _initialized = Audio::setPinout(pinBck, pinLrc, pinDout);
  }

  virtual void loop(unsigned long now) {
    WGpio::loop(now);
    //a station asked for elsewhere is started from here: the ask can come out
    //of the web server task, and connecting is nothing to do in one of those
    if (_playRequested) {
      _playRequested = false;
      _connectToStation();
    } else if ((isOn()) && (_pending) && (millis() - _lastConnect >= (Audio::isRunning() ? AUDIO_CONNECT_TIMEOUT : AUDIO_RETRY_DELAY))) {
      //the station did not come up or the stream broke off: without trying
      //again here only switching the input to and fro would restart it
      LOG->debug(F("Station not playing, try again"));
      _connectToStation();
    }
    if (isOn()) {
      Audio::loop();
      vTaskDelay(1);
    }
  }

  virtual bool isPending() {
    return _pending;
  }

  /** Tells the audio where to ask for the url of the station to play. */
  WAudio* onStationUrl(TOnStationUrl onStationUrl) {
    _onStationUrl = onStationUrl;
    return this;
  }

  /**
   * Asks for the station the supplier names to be started. Called when the
   * audio is switched on and again whenever another station is chosen, so one
   * picked while the radio plays takes over. The connecting itself is left to
   * the next loop(), and it does nothing while the audio is off - being
   * switched on starts the chosen station by itself.
   */
  void playStation() {
    _playRequested = true;
    if (isOn()) _pending = true;
  }

  void audioEvent(Audio::msg_t m) {
    // Zeigt die rohe Nachricht im Seriellen Monitor an (ähnlich wie das alte audio_info)
    Serial.printf("Event: %s -> %s (ID: %d)\n", m.s, m.msg, m.e);

    // Filter nach den spezifischen Event-IDs (m.e)
    switch (m.e) {
      case Audio::evt_info:
        //only now data arrives. evt_name comes too early for this: a playlist
        //(.m3u, .pls) sends it already, before the stream itself is connected
        if ((m.msg != nullptr) && (strcmp(m.msg, "stream ready") == 0)) _pending = false;
        break;

      case Audio::evt_eof:
        //the stream ended or broke off, loop() connects again
        if (isOn()) _pending = true;
        break;

      case Audio::evt_name:  // Entspricht dem alten audio_showstation
        Serial.printf(">>> Radiosender Name: %s <<<\n", m.msg);
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

  /** Ends what plays and goes to the station that is chosen now. */
  void _connectToStation() {
    if (!isOn()) return;
    const char* url = (_onStationUrl ? _onStationUrl() : nullptr);
    //what plays ends in any case: with no station to go to the audio stays on
    //but silent, rather than playing on what was chosen before
    Audio::stopSong();
    if ((url == nullptr) || (strlen(url) == 0)) {
      LOG->debug(F("No station to play"));
      _pending = false;
      return;
    }
    //stays pending until the stream is ready, a failed attempt is repeated
    //from loop() after AUDIO_RETRY_DELAY
    _pending = true;
    _lastConnect = millis();
    if (!_network->isWifiConnected()) {
      LOG->debug(F("No wifi, station '%s' waits"), url);
      return;
    }
    LOG->debug(F("Connect to station '%s'"), url);
    if (!Audio::connecttohost(url)) {
      LOG->debug(F("Can't connect to station '%s'"), url);
    }
  }

  virtual void _updateOn() {
    WGpio::_updateOn();
    if (isOn()) {
      //if (!_starting) {
      //  _starting = true;
        LOG->debug("radio on..");
        // play
        //also without wifi yet: the station then waits in loop() until
        //the wifi is up, before it was never started in that case
        if (pin() != NO_PIN) {
          LOG->debug("b) XSMT on");
          WGpio::writeOutput(pin(), HIGH);
        }
        delay(100);
        playStation();
        setVolume(20);
      //  _starting = false;
      //}
    } else {
      LOG->debug("radio off.");
      // if (_tuner != nullptr) {
      Audio::stopSong();
      _pending = false;
      //_starting = false;
      if (pin() != NO_PIN) {
        WGpio::writeOutput(pin(), LOW);
      }
      delay(50);
      //}
    }
  }

 private:
  WNetwork* _network;
  TOnStationUrl _onStationUrl = nullptr;
  bool _playRequested = false;
  bool _initialized = false;
  bool _pending = false;
  unsigned long _lastConnect = 0;
};

void audioTaskEvent(Audio::msg_t m) {
  wAudio->audioEvent(m);
}

#endif
