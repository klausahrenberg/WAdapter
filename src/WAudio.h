#ifndef W_WAUDIO_H
#define W_WAUDIO_H

#include "Audio.h"
#include "WProperty.h"

class WAudio;
WAudio* wAudio = nullptr;
//void audioTask(void* parameter);

struct audioMessage {
  uint8_t cmd;
  const char* txt;
  uint32_t value;
  uint32_t ret;
} audioTxMessage, audioRxMessage;
   
enum : uint8_t { SET_VOLUME, GET_VOLUME, CONNECTTOHOST, CONNECTTOSD };

typedef std::function<void(void)> TOnStateChange;

class WAudio : public Audio, public WGpio {
 public:
  WAudio(byte pinBck, byte pinLrc, byte pinDout, byte pinXsmt = NO_PIN, IWExpander* expander = nullptr) : Audio(I2S_NUM_0), WGpio(GPIO_TYPE_LED, pinXsmt, OUTPUT, expander) {    
    wAudio = this;
    this->setVolume(21); 
    setPinout(pinBck, pinLrc, pinDout);
  }

  ~WAudio() {
    if (xHandle != nullptr) {
      vTaskDelete(xHandle);
      xHandle = nullptr;
    }
    if (audioSetQueue != nullptr) {
      vQueueDelete(audioSetQueue);
      audioSetQueue = nullptr;
    }
    if (audioGetQueue != nullptr) {
      vQueueDelete(audioGetQueue);
      audioGetQueue = nullptr;
    }
  }

  void setOnChange(TOnStateChange onChange) { this->onChange = onChange; }

  bool play(String url) {
    /*if (xHandle == nullptr) {
      audioSetQueue = xQueueCreate(10, sizeof(struct audioMessage));
      audioGetQueue = xQueueCreate(10, sizeof(struct audioMessage));
      xTaskCreatePinnedToCore(
          audioTask,    // Function to implement the task
          "audioplay",  // Name of the task
          4096,         // Stack size in words
          nullptr,      // Task input parameter
          (configMAX_PRIORITIES - 3), // Priority of the task 2 | portPRIVILEGE_BIT
          &xHandle,  // Task handle.
          tskNO_AFFINITY);
    }*/

    return this->connecttohost(url.c_str());
  }

  void updateStreamTitle(const char* attrText) {
    this->streamtitle = String(attrText);
    if (onChange) {
      onChange();
    }
  }

  String getStreamTitle() {
    return this->streamtitle;
  }

  TaskHandle_t xHandle = nullptr;
  QueueHandle_t audioSetQueue = nullptr;
  QueueHandle_t audioGetQueue = nullptr;

  virtual void loop(unsigned long now) {
    WGpio::loop(now);
    if (isOn()) {
      Audio::loop();
      vTaskDelay(1);
    }
  }  

 protected:
  audioMessage transmitReceive(audioMessage msg) {
    xQueueSend(audioSetQueue, &msg, portMAX_DELAY);
    if (xQueueReceive(audioGetQueue, &audioRxMessage, portMAX_DELAY) ==
        pdPASS) {
      if (msg.cmd != audioRxMessage.cmd) {
        log_e("wrong reply from message queue");
      }
    }
    return audioRxMessage;
  }

  virtual void _updateOn() {
    WGpio::_updateOn();
    if (isOn()) {
      if (!_starting) {
        _starting = true;
        LOG->debug("radio on..");
        // play
        if (NETWORK->isWifiConnected()) {
          LOG->debug("radio on...");

          log_w("a) radio gaga");
          //_tuner = new WAudio();

          // this->radio->init(WM8978_I2S_BCK, WM8978_I2S_WS, WM8978_I2S_DOUT, WM8978_I2S_MCLKPIN);
          //_tuner->setPinout(PIN_DAC_BCK, PIN_DAC_LRC, PIN_DAC_DOUT);
          if (pin() != NO_PIN) {
            writeOutput(pin(), HIGH);
          }
          if (!play("https://onefm.ice.infomaniak.ch/onefm-high.mp3")) {
            // if (!_tuner->play("http://stream.104.6rtl.com/rtl-live/mp3-192/play.m3u")) {  // this->getStationUrl(station->enumIndex())->asString())) {
            NETWORK->debug(F("Can't connect to station"));
          }
          setVolume(20);
        }
        _starting = false;
      }
    } else {
      LOG->debug("radio off.");
      //if (_tuner != nullptr) {
        stopSong();
        delay(50);        
      //}
    }
  }

 private:
  String streamtitle;
  TOnStateChange onChange = nullptr;
  bool _starting = false;
};

void audio_showstreamtitle(const char* info) {
  // Serial.print("info        "); Serial.println(info);
  if (wAudio != nullptr) {
    wAudio->updateStreamTitle(info);
  }
}

// optional
void audio_info(const char *info) {  
  Serial.print("audio info: "); Serial.println(info);
}

void audio_commercial(const char *info) {  //duration in sec
  Serial.print("commercial: "); Serial.println(info);
}

void audio_bitrate(const char *info){
  Serial.print("bitrate: "); Serial.println(info);
}

/*void audio_id3data(const char *info){  //id3 metadata
  network->debug(F("id3data %s"), info);
}
void audio_eof_mp3(const char *info){  //end of file
  network->debug(F("eof_mp3 %s"), info);
}
void audio_showstation(const char *info){
  network->debug(F("station %s"), info);
}
void audio_showstreamtitle(const char *info){
  network->debug(F("streamtitle %s"), info);
  radio->getStreamTitle()->setString(info);
}
void audio_bitrate(const char *info){
  network->debug(F("bitrate %s"), info);
}
void audio_commercial(const char *info){  //duration in sec
  network->debug(F("commercial %s"), info);
}
void audio_icyurl(const char *info){  //homepage
  network->debug(F("icyurl %s"), info);
}
void audio_lasthost(const char *info){  //stream URL played
  network->debug(F("lasthost %s"), info);
}
void audio_eof_speech(const char *info){
  network->debug(F("eof_speech %s"), info);
}*/

// audioTask


/*
void audioTask(void* parameter) {
  if (!wAudio->audioSetQueue || !wAudio->audioGetQueue) {
    log_e("queues are not initialized");
    while (true) {
      ;
    }  // endless loop
  }
  struct audioMessage audioRxTaskMessage;
  struct audioMessage audioTxTaskMessage;

  while (true) {
    if (xQueueReceive(wAudio->audioSetQueue, &audioRxTaskMessage, 1) ==
        pdPASS) {
      if (audioRxTaskMessage.cmd == SET_VOLUME) {
        audioTxTaskMessage.cmd = SET_VOLUME;
        wAudio->setVolume(audioRxTaskMessage.value);
        audioTxTaskMessage.ret = 1;
        xQueueSend(wAudio->audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else if (audioRxTaskMessage.cmd == CONNECTTOHOST) {
        audioTxTaskMessage.cmd = CONNECTTOHOST;
        audioTxTaskMessage.ret = wAudio->connecttohost(audioRxTaskMessage.txt);
        xQueueSend(wAudio->audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else if (audioRxTaskMessage.cmd == CONNECTTOSD) {
        audioTxTaskMessage.cmd = CONNECTTOSD;
        audioTxTaskMessage.ret = wAudio->connecttoSD(audioRxTaskMessage.txt);
        xQueueSend(wAudio->audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else if (audioRxTaskMessage.cmd == GET_VOLUME) {
        audioTxTaskMessage.cmd = GET_VOLUME;
        audioTxTaskMessage.ret = wAudio->getVolume();
        xQueueSend(wAudio->audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else {
        log_i("error");
      }
    }
    wAudio->loop();
  }
}*/

#endif
