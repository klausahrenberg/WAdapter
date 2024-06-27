#ifndef W_WEB_CONTROLS_H
#define W_WEB_CONTROLS_H

#include "WebResources.h"

class WebControl {
 protected:
  char* _tag;
  char* _content = nullptr;
  bool _closing = true;
  WList<WKeyValue>* _params = nullptr;
  WList<WebControl>* _items = nullptr;

 public:
  WebControl(const char* tag, const char* params, ...) {
    _tag = new char[strlen_P(tag) + 1];
    strcpy_P(_tag, tag);

    va_list arg;
    char* key = nullptr;
    va_start(arg, params);
    while (params) {
      if (key == nullptr) {
        key = new char[strlen_P(params) + 1];
        strcpy_P(key, params);
      } else {
        // WKeyValue* kv = new WKeyValue(key, params);
        addParam(key, params);
        delete key;
        key = nullptr;
      }
      params = va_arg(arg, const char*);
    }
    if (key != nullptr) {
      addParam(key, nullptr);
      delete key;
      key = nullptr;
    }
    va_end(arg);
  }

  ~WebControl() {
    if (_tag) delete _tag;
    if (_content) delete _content;
    if (_params) {
      _params->clear();
      delete _params;
    }
    if (_items) {
      _items->clear();
      delete _items;
    }
  }

  void content(const char* content) {
    delete _content;
    _content = new char[strlen_P(content) + 1];
    strcpy_P(_content, content);
  }

  const char* content() { return _content; }

  void closing(bool closing) { _closing = closing; }

  bool closing() { return _closing; }

  void add(WebControl* kv) {
    if (_items == nullptr) _items = new WList<WebControl>();
    _items->add(kv);
  }

  // void addParam(WKeyValue* kv) {

  void addParam(const char* key, const char* param) {
    addParam(key, param, nullptr);
  }

  void addParam(const char* key, const char* pattern, const char* params, ...) {
    if (_params == nullptr) _params = new WList<WKeyValue>();    
    if ((pattern != nullptr) && (params != nullptr)) {
      va_list args;
      va_start(args, params);
      char buffer[128];
      snprintf(buffer, sizeof(buffer), pattern, params, args);
      va_end(args);
      _params->add(new WKeyValue(key, buffer));
    } else {
      _params->add(new WKeyValue(key, pattern));
    }
  }

  virtual void createStyles(WKeyValues* styles) {
    if (_items) _items->forEach([this, styles](WebControl* wc) { wc->createStyles(styles); });
  }

  virtual void toString(Print* stream) {
    WHtml::command(stream, _tag, true, _params);
    stream->print(_content);
    if (_items) _items->forEach([this, stream](WebControl* wc) { wc->toString(stream); });
    if (_closing) WHtml::command(stream, _tag, false, nullptr);
  }
};

class WebButton : public WebControl {
 public:
  WebButton(const char* id, const char* title) : WebControl(WC_BUTTON, WC_ID, id, nullptr) {
    content(title);
  }
  ~WebButton() {}

  virtual void createStyles(WKeyValues* styles) {
    styles->add(WC_BUTTON, WC_STYLE_BUTTON);
    styles->add(WC_CSS_BUTTON_HOVER, WC_STYLE_BUTTON_HOVER);
    WebControl::createStyles(styles);
  }

  void onClickNavigateBack() { addParam(WC_ON_CLICK, WC_HISTORY_BACK); }

  void onClickNavigateTo(const char* target) { addParam(WC_ON_CLICK, WC_LOCATION_HREF, target, nullptr); }

  virtual void toString(Print* stream) { WebControl::toString(stream); }
};

class WebCheckbox : public WebControl {
 public:
  WebCheckbox(const char* id, const char* title) : WebControl(WC_DIV, WC_CLASS, "cb", nullptr) {
    WebControl* input = new WebControl(WC_INPUT, WC_ID, id, WC_TYPE, "checkbox", nullptr);
    input->closing(false);
    this->add(input);
    WebControl* label = new WebControl(WC_LABEL, WC_FOR, id, nullptr);
    label->content(title);
    this->add(label);
  }

  virtual void createStyles(WKeyValues* styles) {
    styles->add(WC_CSS_CHECK_BOX, WC_STYLE_CHECK_BOX);
    styles->add(WC_CSS_CHECK_BOX_LABEL, WC_STYLE_CHECK_BOX_LABEL);
    styles->add(WC_CSS_CHECK_BOX_LABEL_BEFORE, WC_STYLE_CHECK_BOX_LABEL_BEFORE);
    styles->add(WC_CSS_CHECK_BOX_CHECKED_LABEL_BEFORE, WC_STYLE_CHECK_BOX_CHECKED_LABEL_BEFORE);
    styles->add(WC_CSS_INPUT_CHECKED_SLIDER, WC_STYLE_INPUT_CHECKED_SLIDER);
    styles->add(WC_CSS_INPUT_CHECKED_SLIDER_BEFORE, WC_STYLE_INPUT_CHECKED_SLIDER_BEFORE);
    WebControl::createStyles(styles);
  }
};

class WebSwitch : public WebControl {
 public:
  WebSwitch(const char* id, const char* title) : WebControl(WC_LABEL, WC_CLASS, "switch", nullptr) {
    WebControl* input = new WebControl(WC_INPUT, WC_ID, id, WC_TYPE, "checkbox", WC_ON_CHANGE, "toggleCheckbox(this)", "checked", nullptr);
    input->closing(false);    
    this->add(input);
    /*WebControl* label = new WebControl(WC_LABEL, WC_FOR, id, nullptr);
    label->content(title);
    this->add(label);*/
    
    WebControl* slider = new WebControl(WC_SPAN, WC_CLASS, "slider", nullptr);    
    this->add(slider);
    
  }

  virtual void createStyles(WKeyValues* styles) {
    styles->add(WC_CSS_SWITCH, WC_STYLE_SWITCH);
    styles->add(WC_CSS_SWITCH_INPUT, WC_STYLE_SWITCH_INPUT);
    styles->add(WC_CSS_SLIDER, WC_STYLE_SLIDER);
    styles->add(WC_CSS_SLIDER_BEFORE, WC_STYLE_SLIDER_BEFORE);
    styles->add(WC_CSS_INPUT_CHECKED_SLIDER, WC_STYLE_INPUT_CHECKED_SLIDER);
    styles->add(WC_CSS_INPUT_CHECKED_SLIDER_BEFORE, WC_STYLE_INPUT_CHECKED_SLIDER_BEFORE);
    WebControl::createStyles(styles);
  }
};

#endif