#ifndef W_WEB_CONTROLS_H
#define W_WEB_CONTROLS_H

#include "WebResources.h"

typedef std::function<void()> OnWebControlChange;

class WebControl {
 protected:
  char* _tag = nullptr;
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

  virtual ~WebControl() {
    if (_tag) delete _tag;
    if (_content) delete _content;
    if (_params) delete _params;
    if (_items) delete _items; 
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
    if (kv != nullptr) {
      if (_items == nullptr) _items = new WList<WebControl>();
      _items->add(kv);
    }
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
    if (_items) _items->forEach([this, styles](WebControl* wc, const char* id) { wc->createStyles(styles); });
  }

  virtual void createScripts(WKeyValues* scripts) {
    if (_items) _items->forEach([this, scripts](WebControl* wc, const char* id) { wc->createScripts(scripts); });
  }

  virtual void toString(Print* stream) {
    WHtml::command(stream, _tag, true, _params);
    stream->print(_content);
    if (_items) _items->forEach([this, stream](WebControl* wc, const char* id) { wc->toString(stream); });
    if (_closing) WHtml::command(stream, _tag, false, nullptr);
  }
};

class WebDiv : public WebControl {
 public:
  WebDiv(WebControl* child) : WebControl(WC_DIV, nullptr) {    
    this->add(child);
  }
};

class WebForm : public WebControl {
 public:
  WebForm(const char* id, WebControl* child = nullptr) : WebControl(WC_FORM, WC_METHOD, WC_POST, WC_ACTION, "events", nullptr) {   
    this->add((new WebControl(WC_INPUT, WC_TYPE, WC_HIDDEN, WC_NAME, WC_FORM, WC_VALUE, id, nullptr)));
    this->add(child);
  } 
};  

const static char WC_SCRIPT_TEST[] PROGMEM = R"=====(
const xhr = new XMLHttpRequest();
xhr.open("POST", "/events");
xhr.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
const body = JSON.stringify({
  userId: 1,
  title: "Fix my bugs",
  completed: false
});
xhr.onload = () => {
  if (xhr.readyState == 4 && xhr.status == 201) {
    console.log(JSON.parse(xhr.responseText));
  } else {
    console.log(`Error: ${xhr.status}`);
  }
  document.location='config';
};
xhr.send(body);
)=====";

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

  virtual void createScripts(WKeyValues* scripts) {
    if (_onClick) {
      scripts->add("onButtonClick", WC_SCRIPT_TEST);
    }
    WebControl::createScripts(scripts);
  }


  void onClickNavigateBack() { addParam(WC_ON_CLICK, WC_HISTORY_BACK); }

  WebButton* onClickNavigateTo(const char* target) { addParam(WC_ON_CLICK, WC_LOCATION_HREF, target, nullptr); return this; }

  WebButton* onClick(OnWebControlChange onClick) {
    _onClick = onClick;
    this->addParam(WC_ON_CLICK, PSTR("onButtonClick(this)"));
    return this;
  }

  virtual void toString(Print* stream) { 
    WebControl::toString(stream); 
  }

 protected:
  OnWebControlChange _onClick = nullptr; 
};

class WebSubmitButton : public WebControl {
 public:
  WebSubmitButton(const char* title) : WebControl(WC_BUTTON, WC_TYPE, WC_SUBMIT, nullptr) {
    content(title);
  }
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

class WebTextField : public WebControl {
 public:
  WebTextField(const char* id, const char* title, const char* text, byte maxLength = 32, bool passwordField = false) : WebControl(WC_DIV, nullptr) {
    WebControl* label = new WebControl(WC_LABEL, WC_FOR, id, nullptr);
    label->content(title);
    this->add(label);
    WebControl* input = new WebControl(WC_INPUT, WC_ID, id, WC_NAME, id, WC_MAXLENGTH, String(maxLength).c_str(), WC_TYPE, (passwordField ? WC_PASSWORD : WC_TEXT), nullptr);
    if (text != nullptr) {
      input->addParam(WC_VALUE, text);
    }
    input->closing(false);
    this->add(input);
  }

};

class WebTable : public WebControl {
 public:
  WebTable(WList<WProperty>* datas) : WebControl(WC_TABLE, nullptr) {
    _datas = datas;
  }  

  virtual ~WebTable() {
    if (_datas) {
      delete _datas;
    }
  }

  virtual void toString(Print* stream) {             
    WHtml::command(stream, _tag, true, _params);    
    _datas->forEach([this, stream](WProperty* property, const char* id) { 
      WHtml::command(stream, WC_TABLE_ROW, true, nullptr);    
      WHtml::command(stream, WC_TABLE_HEADER, true, nullptr);          
      if (id) stream->print(id); 
      WHtml::command(stream, WC_TABLE_HEADER, false, nullptr);   
      WHtml::command(stream, WC_TABLE_DATA, true, nullptr);    
      property->toString(stream);
      //if (item) stream->print();      
      WHtml::command(stream, WC_TABLE_DATA, false, nullptr);   
      WHtml::command(stream, WC_TABLE_ROW, false, nullptr);    
    });      
    if (_closing) WHtml::command(stream, _tag, false, nullptr);    
  }

 private:
  WList<WProperty>* _datas; 
};

#endif