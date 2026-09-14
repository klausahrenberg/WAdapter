#ifndef W_WEBAPP_H
#define W_WEBAPP_H

#include "WebAppSockets.h"
#include "WebPage.h"
#include "WebSocketsServer.h"

// 4096 crashes with PSRAM
#define SIZE_RESPONSE_STREAM 2048U

class WebApp {
 public:
  WebApp() {
    //_webSocketsServer = new WebSocketsServer();
    WEB_SOCKETS = new WebSocketsServer(81);
    //"/ws", _webSocketHandler->eventHandler());
    // WEB_SOCKETS->begin();

    WEB_SOCKETS->onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
      switch (type) {
        case WStype_DISCONNECTED: {
          LOG->notice(F("WebSocket [%d] disconnected"), num);
          //_sessions->removeIf([clientId](WssSessionDetail *detail) { return (detail->client->id() == clientId); });
          break;
        }
        case WStype_CONNECTED: {
          IPAddress ip = WEB_SOCKETS->remoteIP(num);
          LOG->notice(F("WebSocket [%d] connected from %d.%d.%d.%d"), num, ip[0], ip[1], ip[2], ip[3]);
          break;
        }
        case WStype_TEXT: {
          LOG->notice(F("[%d] get Text: %s"), num, (const char*)payload);
          WList<WValue>* args = WJsonParser::asMap((const char*)payload);
          //a broken payload gives no list at all
          WValue* event = (args != nullptr ? args->getById(WC_EVENT) : nullptr);
          if (event != nullptr) {
            WValue* form = args->getById(WC_FORM);
            if (form != nullptr) {
              WebPageItem* pi = _webPages->getById(form->asString());
              if (pi != nullptr) {
                if (event->equals(WC_PING)) {
                  if (pi != nullptr) pi->lastAlive = millis();
                } else if (pi->instance != nullptr) {
                  WValue* id = args->getById(WC_ID);
                  WValue* cdata = args->getById(WC_DATA);
                  WList<WValue>* eventData = (((cdata != nullptr) && (cdata->type() == WDataType::LIST)) ? cdata->asList() : nullptr);
                  if (id == nullptr) {
                    // an event that names no control is one of the page itself
                    pi->instance->handleEvent(event, eventData);
                  } else {
                    WebControl* control = pi->instance->getElementById(id->asString());
                    if (control != nullptr) {
                      LOG->debug("control found");
                      control->handleEvent(event, eventData);
                    } else {
                      LOG->debug(F("Control for handling not found %s"), id->asString());
                    }
                  }
                }
              }
            }
          }
          delete args;
        }
      }
    });

    /*_webSocketHandler->onConnect([this](AsyncWebSocket *server, AsyncWebSocketClient *client) {
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
                if (control != nullptr)  {
                  LOG->debug("control found");
                  control->handleEvent(event, ((cdata != nullptr) && (cdata->type() == LIST)) ? cdata->asList() : nullptr);
                } else {
                  LOG->debug(F("Control for handling not found %s"), id->asString());
                }
              }
            }
          }
        }
      }
      delete args;
    });*/
  }

  ~WebApp() {
    WEB_SOCKETS->close();
    delete WEB_SOCKETS;
    WEB_SOCKETS = nullptr;
  }

  // AsyncWebSocket *webSockets() { return WEB_SOCKETS; }

  WList<WebPageItem>* webPages() { return _webPages; }

  void addWebPage(const char* id, WebPageInitializer initializer, const char* title, bool showInMainMenu = true) {
    _webPages->add(new WebPageItem(initializer, title, showInMainMenu), id);
  }

  void webSocketBroadcast(const char* payload) {
    WEB_SOCKETS->broadcastTXT(payload);
  }

  void loop(unsigned long now) {
    WEB_SOCKETS->loop();
    if ((_lastPing == 0) || (now - _lastPing > 10000)) {
      _lastPing = now;
#ifdef ARDUINO_ARCH_ESP8266
      LOG->debug("Ping  %d (Memory Free: %u  Max: %u)", now, ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
#else
      LOG->debug("Ping  %d (Memory Free: %u  Min: %u  Max: %u)", now, ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
#endif
      WebAppSockets::sendMessage(WC_PING, nullptr, nullptr);
      _cleanUpDeadSessions();
    }
  }

  void registerSession(WValue uuid) {
  }

  void bindWebServerCalls(AsyncWebServer* webServer) {
    _webPages->forEach([this, webServer](int index, WebPageItem* pageItem, const char* id) { _bind(webServer, pageItem, id); });
  }

  void bindRootPage(AsyncWebServer* webServer) {
    // without webthings the control panel is the entry as well, it is the page
    // the user is meant to land on
    const char* id = WC_UI;
    WebPageItem* pi = _webPages->getById(id);
    if ((pi == nullptr) && (_webPages->size() > 0)) {
      pi = _webPages->get(0);
      id = _webPages->_getNode(0)->id;
    }
    // the page keeps its own id, so its events find it although it is the root
    if (pi != nullptr) webServer->on("/", HTTP_GET, std::bind(&WebApp::_handleGet, this, std::placeholders::_1, pi, String(id)));
  }

  /**
   * Answers a request for the root with the control panel. Used where the root
   * is the description of the webthings for every other client, so the page is
   * the one of the menu and not a copy that no event would reach.
   */
  bool handleUiPage(AsyncWebServerRequest* request) {
    WebPageItem* pi = _webPages->getById(WC_UI);
    if (pi == nullptr) return false;
    _handleGet(request, pi, WC_UI);
    return true;
  }

  /**
   * Prints a page that no url is bound to - the answer to a submitted form,
   * which is followed by a restart - in the same frame as every other page.
   * Without the frame the browser gets the bare content of the page without a
   * document around it and shows nothing at all.
   */
  void toString(Print* stream, WebPage* page, const char* title) {
    //the item is only the handle WebApp::toString draws a page by, it is not
    //added to the pages the menu and the events are looked up in
    WebPageItem pi(nullptr, title, false);
    pi.instance = page;
    page->add(page->createControls());
    toString(stream, &pi);
  }

  /**
   * Works a submitted form out. Where it finds nothing to hand the form to it
   * answers the request itself - 'answered' says so, so the caller knows the
   * request is settled and does not answer it a second time.
   */
  WFormResponse handleHttpEventArgs(AsyncWebServerRequest* request, WList<WValue>* args, bool* answered = nullptr) {
    //a post body that is no json at all gives no list
    WValue* formName = (args != nullptr ? args->getById(WC_FORM) : nullptr);
    if (formName != nullptr) {
      WebPageItem* pi = _webPages->getById(formName->asString());
      if (pi != nullptr) {
        WebPage* p = pi->initializer();
        return p->submitForm(args);
      } else {
        LOG->debug(F("No page '%s' found."), formName->asString());
        request->send(404);
        if (answered != nullptr) *answered = true;
      }
    } else {
      LOG->debug(F("No form name found."));
      request->send(404);
      if (answered != nullptr) *answered = true;
    }
    return WFormResponse();
  }

  virtual void toString(Print* stream, WebPageItem* pi) {
    WStringList* styles = new WStringList();
    styles->add(CSS_GENERAL_STYLE, CSS_GENERAL_ID);
    styles->add(CSS_BODY_STYLE, CSS_BODY_ID);
    styles->add(CSS_H2_STYLE, WC_H2);
    styles->add(CSS_SHELL_STYLE, CSS_SHELL_ID);
    WStringList* scripts = new WStringList();
    // Bar - Styles and Scripts
    WebControl* bar = _createBar(styles);
    bar->createStyles(styles);
    bar->createScripts(scripts);
    // Side - Styles and Scripts
    WebControl* side = _createSide(styles, pi);
    side->createStyles(styles);
    side->createScripts(scripts);
    // Mobile View
    styles->add(CSS_MEDIA_MOBILE_STYLE, CSS_MEDIA_MOBILE_ID);
    //Light/Dark-Mode
    styles->add(CSS_MEDIA_LIGHT_STYLE, CSS_MEDIA_LIGHT_ID);
    // WebPage - Styles and Scripts
    pi->instance->createStyles(styles);
    pi->instance->createScripts(scripts);
    // Print
    WHtml::commandParamsAndNullptr(stream, WC_DOCTYPE_HTML, true, WC_HTML, nullptr);
    WHtml::commandParamsAndNullptr(stream, WC_HTML, true, WC_LANG, F("en"), nullptr);
    // Head
    WHtml::command(stream, WC_HEAD, true);
    WHtml::commandParamsAndNullptr(stream, WC_META, true, WC_CHARSET, F("utf-8"), nullptr);
    WHtml::commandParamsAndNullptr(stream, WC_META, true, WC_NAME, F("viewport"), WC_CONTENT, F("width=device-width, initial-scale=1, user-scalable=no"), nullptr);
    WHtml::command(stream, WC_TITLE, true);
    if (APPLICATION) stream->print(APPLICATION);
    const char* heading = (pi->title != nullptr ? pi->title : pi->instance->title());
    if (heading != nullptr) {
      if (APPLICATION) stream->print(F(" - "));
      // a heading of a page lies in the flash, so read it byte safe
      stream->print(FPSTR(heading));
    }
    WHtml::command(stream, WC_TITLE, false);  // Title end
    WHtml::commandParamsAndNullptr(stream, WC_LINK, true, WC_REL, F("shortcut icon"), WC_TYPE, F("image/svg"), WC_HREF, WC_ICON_KAMSA, nullptr);
    // Style
    WHtml::command(stream, WC_STYLE, true);
    // stream->print(FPSTR(WC_STYLE_SHELL));
    styles->forEach([this, stream](int index, const char* style, const char* id) { WHtml::styleToString(stream, id, style); });
    WHtml::command(stream, WC_STYLE, false);  // Style end
    WHtml::command(stream, WC_HEAD, false);   // Head end
    // Body
    WHtml::command(stream, WC_BODY, true);
    // Bar with the name of the application and what it is running on

    bar->toString(stream);
    // Navigation and content
    WHtml::commandParamsAndNullptr(stream, WC_DIV, true, WC_CLASS, WC_SHELL, nullptr);

    side->toString(stream);
    pi->instance->toString(stream);
    WHtml::command(stream, WC_DIV, false);  // End of WC_SHELL
    // Scripts
    if (!scripts->empty()) {
      WHtml::command(stream, WC_SCRIPT, true);
      if (pi->instance->statefulWebPage()) {
        // the page tells the socket which page it is, the url it was opened
        // with can be the root one as well
        stream->print(FPSTR(WC_SCRIPT_FORM_ID));
        if (pi->instance->pageId()) stream->print(pi->instance->pageId());
        stream->print(FPSTR(WC_SCRIPT_FORM_ID_END));
        scripts->add(WC_SCRIPT_INITIALIZE_SOCKET);
      }
      scripts->forEach([this, stream](int index, const char* script, const char* id) {
        stream->print(script);
      });
      WHtml::command(stream, WC_SCRIPT, false);
    }
    WHtml::command(stream, WC_BODY, false);  // Body end
    WHtml::command(stream, WC_HTML, false);  // Page end
    delete bar;
    delete side;
    delete styles;
    delete scripts;
  }

 private:
  WStringStream* _stream = nullptr;
  unsigned long _lastPing = 0;
  WList<WebPageItem>* _webPages = new WList<WebPageItem>();

  WebControl* _createBar(WStringList* styles) {
    WebControl* bar = new WebControl(WC_DIV, WC_CLASS, WC_BAR, nullptr);
    // Burger
    bar->add((new WebIconButton("&#9776;"))->param(WC_CLASS, CSS_BURGER_BUTTON_CLASS)->param(WC_ON_CLICK, PSTR("document.body.classList.toggle('nav')")));
    bar->add((new WebControl(WC_H2, nullptr))->content(APPLICATION));
    const char* deviceId = SETTINGS->getString(WC_ID);
    //deviceId can point to PROGMEM, so don't compare it byte wise
    if ((deviceId != nullptr) && (strlen_P(deviceId) != 0)) {
      bar->add(new WebLabel(deviceId));
    }  
    if (VERSION) {
      bar->add(new WebLabel(VERSION));
    }  
    if (DEBUG) {
      bar->add(new WebLabel(PSTR("(debug)")));
    }
    styles->add(CSS_BAR_STYLE, CSS_BAR_ID);
    styles->add(WC_DISPLAY_NONE, CSS_BURGER_ID);
    return bar;

    /*stream->print(FPSTR(WC_HTML_BAR));
    if (APPLICATION) stream->print(APPLICATION);
    stream->print(FPSTR(WC_HTML_BAR_SUB));
    const char* deviceId = SETTINGS->getString(WC_ID);
    //deviceId can point to PROGMEM, so don't compare it byte wise
    if ((deviceId != nullptr) && (strlen_P(deviceId) != 0)) {
      stream->print(deviceId);
      if (VERSION) stream->print(WC_SPACE);
    }
    if (VERSION) {
      stream->print(F("Rev "));
      stream->print(VERSION);
      if (DEBUG) stream->print(F(" (debug)"));
    }*/
  }

  WebControl* _createSide(WStringList* styles, WebPageItem* pi) {
    WebControl* side = new WebControl(WC_NAV, WC_CLASS, WC_SIDE, nullptr);
    _webPages->forEach([this, pi, side](int index, WebPageItem* item, const char* id) {
      if ((item->showInMainMenu) && (item->title != nullptr) && (id != nullptr)) {
        WebIconButton* wib = new WebIconButton(item->title);
        wib->onClickNavigateTo(String(WC_SLASH + String(id)).c_str());
        if (pi == item) {
          wib->param(WC_CLASS, PSTR("icon on"));
        }
        side->add(wib);
      }
    });

    styles->add(CSS_SIDE_STYLE, CSS_SIDE_ID);
    styles->add(CSS_SIDE_BUTTON_STYLE, CSS_SIDE_BUTTON_ID);
    return side;
    /*stream->print(FPSTR(WC_HTML_NAV));
    _printNavigation(stream);
    stream->print(FPSTR(WC_HTML_MAIN));
    if (heading != nullptr) {
      WHtml::command(stream, WC_H2, true);
      stream->print(heading);
      WHtml::command(stream, WC_H2, false);
    }
    _parentNode->toString(stream);
    stream->print(FPSTR(WC_HTML_SHELL_END));

    void _printNavigation(Print* stream) {
    if (WEB_PAGES == nullptr) return;
    WEB_PAGES->forEach([this, stream](int index, WebPageItem* item, const char* id) {
      if ((item->showInMainMenu) && (item->title != nullptr) && (id != nullptr)) {
        //id can point to PROGMEM, _pageId is always in RAM
        bool active = ((_pageId != nullptr) && (strcmp_P(_pageId, id) == 0));
        stream->print(FPSTR(WC_HTML_LINK));
        stream->print(id);
        stream->print(active ? FPSTR(WC_HTML_LINK_ON) : FPSTR(WC_HTML_LINK_OFF));
        //a title of a page lies in the flash, so read it byte safe
        stream->print(FPSTR(item->title));
        stream->print(FPSTR(WC_HTML_LINK_END));
      }
    });
  }

    */
  }

  void _handleGet(AsyncWebServerRequest* request, WebPageItem* pi, String id) {
    LOG->notice(F("Request with id '%s'"), id);
    // a reloaded page leaves its session behind
    if (pi->instance != nullptr) {
      delete pi->instance;
      pi->instance = nullptr;
    }
    WebPage* page = pi->initializer();
    page->add(page->createControls());
    page->pageId(id.c_str());

    AsyncResponseStream* stream = request->beginResponseStream(WC_TEXT_HTML, SIZE_RESPONSE_STREAM);
    pi->instance = page;
    toString(stream, pi);
    if (!page->statefulWebPage()) {
      delete page;
      pi->instance = nullptr;
    } else {
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
    // WEB_SOCKETS->cleanupClients();
  }

  WStringStream* _prepareStream() {
    if (_stream == nullptr) {
      _stream = new WStringStream(SIZE_JSON_PACKET);
    }
    _stream->flush();
    return _stream;
  }
};

#endif