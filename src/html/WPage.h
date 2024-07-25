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
  WPageItem(WPageInitializer initializer, const char* title, bool showInMainMenu = true) {
    this->initializer = initializer;
    this->title = title;
    this->showInMainMenu = showInMainMenu;
  }

  virtual ~WPageItem() {
   
  }

  WPageInitializer initializer;
  const char* title; 
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

  virtual WFormResponse* submitForm(WList<WValue>* args) {
    LOG->debug("handle sf in page");
    return new WFormResponse();
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

  virtual void printPage() {
    if (_onPrintPage) _onPrintPage(this);
  }

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

 protected:
  
 private:
  const char* _title;
  Print* _stream;
  WPage* _targetAfterSubmitting;  
  TPrintPage _onPrintPage;
  TSubmitPage _onSubmitPage;
};

#endif
