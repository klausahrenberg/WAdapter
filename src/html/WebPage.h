#ifndef W_PAGE_H
#define W_PAGE_H

#include "WebControls.h"

class WebPage;
typedef std::function<WebPage*()> WebPageInitializer;
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

class WebPage {
 public:
  WebPage(const char* title = nullptr, bool statefulWebPage = false)
      : _title(WString::duplicate(title)),
        _statefulWebPage(statefulWebPage) {
    _stream = nullptr;
    _targetAfterSubmitting = nullptr;
    _onPrintPage = nullptr;
    _onSubmitPage = nullptr;
    /*if (title) {
      _title = new char[strlen_P(title) + 1];
      strcpy_P(_title, title);
    }*/
  }

  virtual ~WebPage() {
    if (_parentNode) delete _parentNode;
    if (_title) delete[] _title;
  }

  typedef std::function<void(WebPage*)> TPrintPage;
  typedef std::function<void(AsyncWebServerRequest*)> TSubmitPage;
  void onPrintPage(TPrintPage onPrintPage) { _onPrintPage = onPrintPage; }
  void onSubmitPage(TSubmitPage onSubmitPage) { _onSubmitPage = onSubmitPage; }

  void _handleHttp(AsyncWebServerRequest* request) {
    // WebApp->registerSession(WValue.of("hallo"));
    AsyncResponseStream* stream = request->beginResponseStream(WC_TEXT_HTML, 6100U);
    this->toString(stream);
    request->send(stream);
  }

  virtual void createControls(WebControl* parentNode) {
  }

  virtual WFormResponse submitForm(WList<WValue>* args) {
    return WFormResponse();
  }

  void toString(Print* stream) {
    if (_parentNode == nullptr) {
      _parentNode = new WebControl(WC_DIV, nullptr);
      this->createControls(_parentNode);
    }
    WStringList* styles = new WStringList();
    styles->add(WC_STYLE_BODY, WC_BODY);
    styles->add(WC_STYLE_FORM_WHITE_BOX, WC_CSS_FORM_WHITE_BOX);
    _parentNode->createStyles(styles);
    WStringList* scripts = new WStringList();
    _parentNode->createScripts(scripts);
    // Print
    WHtml::commandParamsAndNullptr(stream, WC_DOCTYPE_HTML, true, WC_HTML, nullptr);
    WHtml::commandParamsAndNullptr(stream, WC_HTML, true, WC_LANG, F("en"), nullptr);
    // Head
    WHtml::command(stream, WC_HEAD, true);
    WHtml::commandParamsAndNullptr(stream, WC_META, true, WC_NAME, F("viewport"), WC_CONTENT, F("width=device-width, initial-scale=1, user-scalable=no"), nullptr);
    WHtml::command(stream, WC_TITLE, true);
    if (_title) stream->print(_title);
    WHtml::command(stream, WC_TITLE, false);  // Title end
    WHtml::commandParamsAndNullptr(stream, WC_LINK, true, WC_REL, F("shortcut icon"), WC_TYPE, F("image/svg"), WC_HREF, WC_ICON_KAMSA, nullptr);
    // Style
    WHtml::command(stream, WC_STYLE, true);
    styles->forEach([this, stream](int index, const char* style, const char* id) { WHtml::styleToString(stream, id, style); });
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
    _parentNode->toString(stream);
    // Scripts
    if (!scripts->empty()) {
      WHtml::command(stream, WC_SCRIPT, true);
      if (statefulWebPage()) {
        scripts->add(WC_SCRIPT_INITIALIZE_SOCKET);
      }
      scripts->forEach([this, stream](int index, const char* script, const char* id) {
        stream->print(script);
        stream->print(WC_SEND);
      });
      WHtml::command(stream, WC_SCRIPT, false);
    }
    WHtml::command(stream, WC_BODY, false);  // Body end
    WHtml::command(stream, WC_HTML, false);  // Page end
    // Cleanup
    delete styles;
    delete scripts;
  }

  virtual void printPage() {
    if (_onPrintPage) _onPrintPage(this);
  }

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
    return (_parentNode != nullptr ? _parentNode->getElementById(id) : nullptr);
  }

 protected:
  bool _statefulWebPage;

 private:
  char* _title;
  Print* _stream;
  WebPage* _targetAfterSubmitting;
  TPrintPage _onPrintPage;
  TSubmitPage _onSubmitPage;
  WebControl* _parentNode = nullptr;
};

#endif
