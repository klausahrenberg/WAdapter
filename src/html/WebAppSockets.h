#ifndef W_WEBAPPSOCKETS_H
#define W_WEBAPPSOCKETS_H

#include "WebSocketsServer.h"
#include "WebResources.h"

WebSocketsServer* WEB_SOCKETS = nullptr;

class WebAppSockets {
 public:
  static bool sendMessage(const char* event, const char* id, const char* data) {
    if (WEB_SOCKETS != nullptr) { //} && (WEB_SOCKETS->availableForWriteAll())) {
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
      return WEB_SOCKETS->broadcastTXT(response->c_str());
    }
    return false;
  }
};

#endif
