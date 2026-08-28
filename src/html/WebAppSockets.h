#ifndef W_WEBAPPSOCKETS_H
#define W_WEBAPPSOCKETS_H

#include "WebSocketsServer.h"
#include "WebResources.h"

WebSocketsServer* WEB_SOCKETS = nullptr;

//Writes a json object from a lambda, so a message needs no struct of its own
class WJsonWriter : public IWJsonable {
 public:
  typedef std::function<void(WJson*)> WOnWriteJson;
  WJsonWriter(WOnWriteJson onWriteJson) { _onWriteJson = onWriteJson; }

  virtual void toJson(WJson* json) {
    if (_onWriteJson) _onWriteJson(json);
  }

  virtual void fromJson(WList<WValue>* list) {}

  virtual void registerSettings() {}

 private:
  WOnWriteJson _onWriteJson;
};

class WebAppSockets {
 public:
  static bool sendMessage(const char* event, const char* id, const char* data) {
    if (WEB_SOCKETS != nullptr) { //} && (WEB_SOCKETS->availableForWriteAll())) {
      WStringStream* response = createResponseStream();
      WJson* json = new WJson(response);
      json->beginObject();
      json->propertyString(WC_EVENT, event, nullptr);
      if (id != nullptr) json->propertyString(WC_ID, id, nullptr);
      if (data != nullptr) {
        json->propertyString(WC_DATA, data, nullptr);
      }
      json->endObject();
      delete json;
      LOG->debug("Send> %s", response->c_str());
      bool result = WEB_SOCKETS->broadcastTXT(response->c_str());
      delete response;
      return result;
    }
    return false;
  }

  static bool sendJson(const char* event, const char* id, WJsonWriter::WOnWriteJson data) {
    WJsonWriter writer(data);
    return sendJsonable(event, id, &writer);
  }

  static bool sendJsonable(const char* event, const char* id, IWJsonable* data) {
    if (WEB_SOCKETS != nullptr) { //} && (WEB_SOCKETS->availableForWriteAll())) {
      WStringStream* response = createResponseStream();
      WJson* json = new WJson(response);
      json->beginObject();
      json->propertyString(WC_EVENT, event, nullptr);
      if (id != nullptr) json->propertyString(WC_ID, id, nullptr);
      if (data != nullptr) {
        json->propertyObject(WC_DATA, data);
      }
      json->endObject();
      delete json;
      LOG->debug("Send> %s", response->c_str());
      bool result = WEB_SOCKETS->broadcastTXT(response->c_str());
      delete response;
      return result;
    }
    return false;
  }
};

#endif
