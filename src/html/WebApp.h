#ifndef W_WEBAPP_H
#define W_WEBAPP_H

#include "WebPage.h"

struct WebPageItem {
  WebPageItem(WPageInitializer initializer, const char* title, bool showInMainMenu = true) {
    this->initializer = initializer;
    this->title = title;
    this->showInMainMenu = showInMainMenu;
  }

  WPageInitializer initializer;
  const char* title; 
  bool showInMainMenu;
  WPage* instance = nullptr;
  unsigned long lastAlive = 0;
};

class WebApp {
 public:
  WebApp() {
    _webSocketHandler = new AsyncWebSocketMessageHandler();    
    WEB_SOCKETS = new AsyncWebSocket("/ws", _webSocketHandler->eventHandler());    

    _webSocketHandler->onConnect([this](AsyncWebSocket *server, AsyncWebSocketClient *client) {
      IPAddress ip = client->remoteIP();
      LOG->notice(F("WebSocket [%d] connected from %d.%d.%d.%d"), client->id(), ip[0], ip[1], ip[2], ip[3]);      
    });
    _webSocketHandler->onDisconnect([this](AsyncWebSocket *server, uint32_t clientId) {
      LOG->notice(F("WebSocket [%d] disconnected"), clientId);
      //_sessions->removeIf([clientId](WssSessionDetail *detail) { return (detail->client->id() == clientId); });
    });
    _webSocketHandler->onError([](AsyncWebSocket *server, AsyncWebSocketClient *client, uint16_t errorCode, const char *reason, size_t len) {
      LOG->notice(F("WebSocket client %d error: %d: %s"), client->id(), errorCode, reason);
    });
    _webSocketHandler->onMessage([this](AsyncWebSocket *server, AsyncWebSocketClient *client, const uint8_t *data, size_t len) {
      LOG->notice(F("[%d] get Text: %s"), client->id(), (const char *)data);
      WList<WValue> *args = WJsonParser::asMap((const char *) data);
      WValue* event = args->getById(WC_EVENT);
      if ((args != nullptr) && (event != nullptr)) {
        WValue* form = args->getById(WC_FORM);
        if (form != nullptr) {
          WebPageItem* pi = _webPages->getById(form->asString());
          if (pi != nullptr) {            
            if (event->equalsString(WC_PING)) {      
              if (pi != nullptr) pi->lastAlive = millis();
            } else if (pi->instance != nullptr) {
              WValue* id = args->getById(WC_ID);              
              WValue* cdata = args->getById(WC_DATA);
              if (id != nullptr) {
                WebControl* control = pi->instance->getElementById(id->asString());
                if (control != nullptr) {                  
                  control->handleEvent(event, (WList<WValue>*) cdata);
                }  
              }
            }
          }
        }
      }
      delete args;
    });
  }

  ~WebApp() {
    WEB_SOCKETS->closeAll();
    delete WEB_SOCKETS;
    WEB_SOCKETS = nullptr;
    delete _webSocketHandler;
    _webSocketHandler = nullptr;
  }

  //AsyncWebSocket *webSockets() { return WEB_SOCKETS; }

  WList<WebPageItem> *webPages() { return _webPages; }

  void addWebPage(const char *id, WPageInitializer initializer, const char *title, bool showInMainMenu = true) {    
    _webPages->add(new WebPageItem(initializer, title, showInMainMenu), id);
  }  

  void webSocketBroadcast(const char *payload) {
    WEB_SOCKETS->textAll(payload);
  }

  void loop(unsigned long now) {  
    if ((_lastPing == 0) || (now - _lastPing > 10000)) {
      _lastPing = now;
      LOG->debug("ping  %d ... ", now);
      WebSockets::sendMessage(WC_PING, nullptr, nullptr);
      _cleanUpDeadSessions();
    }    
  }

  void registerSession(WValue uuid) {

  }

  void bindWebServerCalls(AsyncWebServer* webServer) {
    _webPages->forEach([this, webServer](int index, WebPageItem *pageItem, const char *id) { _bind(webServer, pageItem, id); });
  }

  void bindRootPage(AsyncWebServer* webServer) {
    if (_webPages->size() > 0) _bind(webServer, _webPages->get(0), "");
  }  

  WFormResponse handleHttpEventArgs(AsyncWebServerRequest *request, WList<WValue> *args) {    
    WValue *formName = args->getById(WC_FORM);
    if (formName != nullptr) {
      WebPageItem *pi = _webPages->getById(formName->asString());
      if (pi != nullptr) {
        WPage *p = pi->initializer();
        return p->submitForm(args);
      } else {
        LOG->debug(F("No page '%s' found."), formName);
        request->send(404);
      }
    } else {
      LOG->debug(F("No form name found."));
      request->send(404);
    }
    return WFormResponse();
  }

 private:
  AsyncWebSocketMessageHandler *_webSocketHandler;    
  WStringStream *_stream = nullptr;
  unsigned long _lastPing = 0;
  WList<WebPageItem> *_webPages  = new WList<WebPageItem>();

  

  void _handleGet(AsyncWebServerRequest* request, WebPageItem* pi, String id) {
    LOG->notice(F("Request with id '%s'"), id);
    WPage* page = pi->initializer();

    AsyncResponseStream* stream = request->beginResponseStream(WC_TEXT_HTML, 4096U);
    page->toString(stream);
    if (!page->statefulWebPage()) {            
      delete page;
    } else {
      pi->instance = page;
      pi->lastAlive = millis();
    }  
    request->send(stream);    
  }

  void _bind(AsyncWebServer* webServer, WebPageItem* pi, const char* id) {    
    String target = "/" + String(id);    
    webServer->on(target.c_str(), HTTP_GET, std::bind(&WebApp::_handleGet, this, std::placeholders::_1, pi, id));    
  }

  void _cleanUpDeadSessions() {
    _webPages->forEach([](int index, WebPageItem* pi, const char* id) {
      if ((pi->instance != nullptr) && (millis() - pi->lastAlive > 30000)) {
        delete pi->instance;
        pi->instance = nullptr;
        pi->lastAlive = 0;
        LOG->debug("Removed dead session '%s'", id);
      }
    });
    WEB_SOCKETS->cleanupClients();
  }

  WStringStream *_prepareStream() {
    if (_stream == nullptr) {
      _stream = new WStringStream(SIZE_JSON_PACKET);
    }
    _stream->flush();
    return _stream;
  }
};

#endif