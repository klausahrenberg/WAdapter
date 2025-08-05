#ifndef W_WEBSOCKETS_H
#define W_WEBSOCKETS_H

#include <ESPAsyncWebServer.h>

AsyncWebSocket* WEB_SOCKETS = nullptr;

class WebSockets {
 public:
  static bool sendMessage(const char* event, const char* id, const char* data) {
    if ((WEB_SOCKETS != nullptr) && (WEB_SOCKETS->availableForWriteAll())) {
      WStringStream* response = getResponseStream();
      WJson json(response);
      json.beginObject();
      json.propertyString(WC_EVENT, event, nullptr);
      if (id != nullptr) json.propertyString(WC_ID, id, nullptr);
      if (data != nullptr) {
        json.propertyString(WC_DATA, data, nullptr);
      }
      json.endObject();
      LOG->debug("Send> %s", response->c_str());
      return WEB_SOCKETS->textAll(response->c_str());
    }
    return false;
  }
};

#endif