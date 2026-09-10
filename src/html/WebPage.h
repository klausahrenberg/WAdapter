#ifndef W_PAGE_H
#define W_PAGE_H

#include "WebControls.h"

class WebPage;
typedef std::function<WebPage*()> WebPageInitializer;

struct WebPageItem {
  WebPageItem(WebPageInitializer initializer, const char* title, bool showInMainMenu = true) {
    this->initializer = initializer;
    this->title = title;
    this->showInMainMenu = showInMainMenu;
  }

  WebPageInitializer initializer;
  const char* title;
  bool showInMainMenu;
  WebPage* instance = nullptr;
  unsigned long lastAlive = 0;
};

enum WFormOperation { FO_NONE,
                      FO_RESTART,
                      FO_FORCE_AP,
                      FO_RESET_ALL };
struct WFormResponse {
  WFormResponse(WFormOperation operation = FO_NONE, const char* message = nullptr) {
    this->operation = operation;
    this->message = message;
  }
  const char* message;
  WFormOperation operation;
};

class WebPage : public WebControl {
 public:
  WebPage(const char* title = nullptr, bool statefulWebPage = false) 
      : WebControl(CSS_MAIN_CLASS, nullptr),
        _title(WString::duplicate(title)),
        _statefulWebPage(statefulWebPage) {
    _stream = nullptr;
    _targetAfterSubmitting = nullptr;
    _onPrintPage = nullptr;
    _onSubmitPage = nullptr;
  }

  virtual ~WebPage() {
    if (_title) delete[] _title;
    if (_pageId) delete[] _pageId;
  }

  const char* pageId() { return _pageId; }

  /** The id the page is bound to, told by the WebApp before it is printed. */
  void pageId(const char* pageId) {
    if (_pageId) delete[] _pageId;
    _pageId = WString::duplicate(pageId);
  }

  typedef std::function<void(WebPage*)> TPrintPage;
  typedef std::function<void(AsyncWebServerRequest*)> TSubmitPage;
  void onPrintPage(TPrintPage onPrintPage) { _onPrintPage = onPrintPage; }
  void onSubmitPage(TSubmitPage onSubmitPage) { _onSubmitPage = onSubmitPage; }

  virtual WebControl* createControls() {
    return nullptr;
  }; 

  virtual void createStyles(WStringList* styles) {
    WebControl::createStyles(styles);
    styles->add(CSS_MAIN_STYLE, CSS_MAIN_ID);
  }

  virtual WFormResponse submitForm(WList<WValue>* args) {
    return WFormResponse();
  }

  virtual void printPage() {
    if (_onPrintPage) _onPrintPage(this);
  }

  /** The address the device is reachable at, in the access point as well. */
  static String deviceIp() {
    return ((WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : WiFi.softAPIP()).toString();
  }

  /** An event of the socket that names no control is meant for the page. */
  virtual void handleEvent(WValue* event, WList<WValue>* data) {
  }

  const char* title() { return _title; }

  WebPage* targetAfterSubmitting() { return _targetAfterSubmitting; }

  void targetAfterSubmitting(WebPage* targetAfterSubmitting) { _targetAfterSubmitting = targetAfterSubmitting; }

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

  bool statefulWebPage() { return _statefulWebPage; }

  WebPage* statefulWebPage(bool statefulWebPage) {
    _statefulWebPage = statefulWebPage;
    return this;
  }

  WebControl* getElementById(const char* id) {
    return ((_items != nullptr) && (_items->size() > 0) ? _items->get(0)->getElementById(id) : nullptr);
  }

 protected:
  bool _statefulWebPage;

 private:
  char* _title;
  char* _pageId = nullptr;
  Print* _stream;
  WebPage* _targetAfterSubmitting;
  TPrintPage _onPrintPage;
  TSubmitPage _onSubmitPage;
};

#endif
