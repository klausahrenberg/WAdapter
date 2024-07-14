#ifndef W_PAGE_H
#define W_PAGE_H

#include "WebControls.h"

class WPage;
typedef std::function<WPage* ()> WPageInitializer;
enum WFormOperation { FO_NONE, FO_RESTART, FO_FORCE_AP, FO_RESET_ALL };
struct WFormResponse { 
  WFormResponse(WFormOperation operation = FO_NONE, const char* message = nullptr) {
    this->operation = operation;
    this->message = message;
  }  
  const char* message;
  WFormOperation operation; 
};

struct WPageItem {
  WPageItem(WPageInitializer initializer, bool showInMainMenu = true) {
    this->initializer = initializer;
    this->showInMainMenu = showInMainMenu;
  }

  virtual ~WPageItem() {
   
  }

  WPageInitializer initializer; 
  bool showInMainMenu;
};

class WPage {
 public:
  WPage() {    
    _stream = nullptr;
    _targetAfterSubmitting = nullptr;
    _onPrintPage = nullptr;
    _onSubmitPage = nullptr;
  }

  virtual ~WPage() {
  }

  typedef std::function<void(WPage*)> TPrintPage;
  typedef std::function<void(AsyncWebServerRequest*)> TSubmitPage;
  void onPrintPage(TPrintPage onPrintPage) { _onPrintPage = onPrintPage; }
  void onSubmitPage(TSubmitPage onSubmitPage) { _onSubmitPage = onSubmitPage; }

  static void bind(AsyncWebServer* webServer, const char* id, WPageItem* pi) {    
    String target = "/" + String(id);    
    webServer->on(target.c_str(), HTTP_GET, std::bind(&WPage::handleGet, std::placeholders::_1, id, pi));    
  }

  static void handleGet(AsyncWebServerRequest* request, const char* id, WPageItem* pi) {
    AsyncResponseStream* stream = request->beginResponseStream(WC_TEXT_HTML);
    WPage* page = pi->initializer();
    page->toString(stream);
    delete page;
    request->send(stream);
  }

  void _handleHttp(AsyncWebServerRequest* request) {
    AsyncResponseStream* stream = request->beginResponseStream(WC_TEXT_HTML, 6100U);    
    this->toString(stream);    
    request->send(stream);    
  }

  virtual void createControls(WebControl* parentNode) {

  }

  virtual WFormResponse submitForm(WStringList* args) {
    return WFormResponse();
  }

  void toString(Print* stream) {
    WebControl* parentNode = new WebControl(WC_DIV, nullptr);
    this->createControls(parentNode);
    WStringList* styles = new WStringList();   
    styles->add(WC_STYLE_BODY, WC_BODY);
    styles->add(WC_STYLE_FORM_WHITE_BOX, WC_CSS_FORM_WHITE_BOX);        
    parentNode->createStyles(styles);
    WStringList* scripts = new WStringList();        
    parentNode->createScripts(scripts);
    //Print
    WHtml::commandParamsAndNullptr(stream, WC_DOCTYPE_HTML, true, WC_HTML, nullptr);
    WHtml::commandParamsAndNullptr(stream, WC_HTML, true, WC_LANG, F("en"), nullptr);
    // Head
    WHtml::command(stream, WC_HEAD, true);
    WHtml::commandParamsAndNullptr(stream, WC_META, true, WC_NAME, F("viewport"), WC_CONTENT, F("width=device-width, initial-scale=1, user-scalable=no"), nullptr);
    WHtml::command(stream, WC_TITLE, true);
    stream->print(APPLICATION);
    WHtml::command(stream, WC_TITLE, false);  // Title end  
    WHtml::commandParamsAndNullptr(stream, WC_LINK, true, WC_REL, F("shortcut icon"), WC_TYPE, F("image/svg"), WC_HREF, WC_ICON_KAMSA, nullptr);        
    // Style
    WHtml::command(stream, WC_STYLE, true);    
    styles->forEach([this, stream] (const char* style, const char* id) { WHtml::styleToString(stream, id, style); });
    WHtml::command(stream, WC_STYLE, false);  // Style end    
    WHtml::command(stream, WC_HEAD, false);   // Head end    
    // Body
    WHtml::command(stream, WC_BODY, true);
    if (APPLICATION) {    
      WHtml::command(stream, WC_H1, true);
      stream->print(APPLICATION);
      WHtml::command(stream, WC_H1, false);
    }
    const char* id = SETTINGS->getString(WC_ID);
    if (id != nullptr) {
      WHtml::command(stream, WC_H2, true);      
      stream->print(F("Id: "));
      stream->print(id);
      WHtml::command(stream, WC_H2, false);
    }
    if (VERSION) {
      WHtml::command(stream, WC_H2, true);
      stream->print(F("Rev: "));
      stream->print(VERSION);
      if (DEBUG) stream->print(F(" (debug)"));
      WHtml::command(stream, WC_H2, false);
    }
    parentNode->toString(stream);
    // Scripts
    if (!scripts->empty()) {
      WHtml::command(stream, WC_SCRIPT, true);  
      scripts->forEach([this, stream] (const char* script, const char* id) { WHtml::scriptToString(stream, id, script); });
      WHtml::command(stream, WC_SCRIPT, false);  
    }
    WHtml::command(stream, WC_BODY, false);  // Body end    
    WHtml::command(stream, WC_HTML, false);  // Page end   
    //Cleanup
    delete styles;
    delete scripts;
    delete parentNode;
  }

  /*void _printHttpCaption(Print *page) {
    page->print(F("<h2>"));
    //page->print(_applicationName);
    page->print(F("</h2><h3>Idx: "));
    page->print(getIdx());
    page->print(F("</h3><h3>Rev: "));
    page->print(VERSION);
    page->print(DEBUG ? " (debug)" : "");
    page->print(F("</h3>"));
  }*/

  /*void _handleHttpSubmit(AsyncWebServerRequest* request) {
    // if (customPage->hasSubmittedPage()) {
    //_wlog->notice(F("Save custom page: %s"), customPage->id());
    WStringStream* page = new WStringStream(1024);
    submitPage(request);
    if (_targetAfterSubmitting != nullptr) {
      _targetAfterSubmitting->_handleHttp(request);
    } else {
      //_network->settings()->save();
      //_network->_restart(request, (strlen(page->c_str()) == 0 ? "Settings saved." : page->c_str()));
    }
    delete page;
    //}
  }*/

  virtual void printPage() {
    if (_onPrintPage) _onPrintPage(this);
  }

  /*virtual void submitPage(AsyncWebServerRequest* request) {
    if (_onSubmitPage) _onSubmitPage(request);
  }*/

  //const char* id() { return _id; }

  //const char* getTitle() { return _title; }

  WPage* targetAfterSubmitting() { return _targetAfterSubmitting; }

  void targetAfterSubmitting(WPage* targetAfterSubmitting) { _targetAfterSubmitting = targetAfterSubmitting; }

  Print* stream() { return _stream; }

  void stream(Print* stream) { _stream = stream; }

  void lineBreak() { _stream->print("\n"); };

  void print(const __FlashStringHelper* ifsh) {
    _stream->print(ifsh);
    lineBreak();
  }

  void print(const char* cv) {
    _stream->print(cv);
  }

  void println(const char* cv) {
    _stream->println(cv);
  }

  /*void divId(const char* id = "") { HTTP_DIV_ID(_stream, id); }
  void div(const char* key1 = nullptr, const char* value1 = nullptr, const char* key2 = nullptr, const char* value2 = nullptr) { HTTP_DIV(_stream, key1, value1, key2, value2); }
  void divEnd() { HTTP_DIV_END(_stream); }

  void configPageBegin(const char* pageName) { HTTP_CONFIG_PAGE_BEGIN(_stream, pageName); }

  void table(const char* tableId) { HTTP_TABLE(_stream, tableId); }
  void tableEnd() { HTTP_TABLE_END(_stream); }
  void tr() { HTTP_TR(_stream, false); }
  void trEnd() { HTTP_TR(_stream, true); }
  void td(byte colspan = 1) { HTTP_TD(_stream, colspan); }
  void tdEnd() { HTTP_TD_END(_stream); }
  void th(byte colspan = 1) { HTTP_TH(_stream, colspan); }
  void thEnd() { HTTP_TH_END(_stream); }*/

 protected:
  /*void _printHttpCaption() {
    _stream->print(F("<h2>"));
    _stream->print(_title);
    _stream->print(F("</h2>"));
  }*/

  /*void _restart(AsyncWebServerRequest* request, const char* reasonMessage) {
    //_network->restart(reasonMessage);
    if (request != nullptr) {
      request->client()->setNoDelay(true);
      AsyncResponseStream* page = request->beginResponseStream(TEXT_HTML);
      page->printf(HTTP_HEAD_BEGIN, reasonMessage);
      page->print(FPSTR(HTTP_STYLE));
      page->print(FPSTR(HTTP_HEAD_END));
      _printHttpCaption();
      page->printf(HTTP_SAVED, reasonMessage);
      page->print(FPSTR(HTTP_BODY_END));
      request->send(page);
    }
  }*/

  //WNetwork* _network;
  
 private:
  
  //char* _id;
  //char* _title;
  Print* _stream;
  WPage* _targetAfterSubmitting;  
  TPrintPage _onPrintPage;
  TSubmitPage _onSubmitPage;
};

#endif
