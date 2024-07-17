#ifndef W_WEB_CONTROLS_H
#define W_WEB_CONTROLS_H

#include "WebResources.h"

class WebControl {
 protected:
  char* _tag = nullptr;
  char* _content = nullptr;
  bool _closing = true;
  WStringList* _params = nullptr;
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

  WebControl* closing(bool closing) { _closing = closing; return this; }

  bool closing() { return _closing; }

  void add(WebControl* kv) {
    if (kv != nullptr) {
      if (_items == nullptr) _items = new WList<WebControl>();
      _items->add(kv);
    }
  }

  // void addParam(WKeyValue* kv) {

  WebControl* addParam(const char* key, const char* param) {
    return addParam(key, param, nullptr);
  }

  WebControl* addParam(const char* key, const char* pattern, const char* params, ...) {    
    if (_params == nullptr) _params = new WStringList();    
    if ((pattern != nullptr) && (params != nullptr)) {
      va_list args;
      va_start(args, params);
      char buffer[128];
      snprintf(buffer, sizeof(buffer), pattern, params, args);
      va_end(args);
      _params->add(buffer, key);
    } else {
      _params->add(pattern, key);
    }
    return this;
  }

  bool hasParam(const char* key) {
    return ((_params != nullptr) && (_params->getById(key) != nullptr));
  }

  virtual void createStyles(WStringList* styles) {
    if (_items) _items->forEach([this, styles](WebControl* wc, const char* id) { wc->createStyles(styles); });
  }

  virtual void createScripts(WStringList* scripts) {
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
    this->add((new WebControl(WC_INPUT, WC_TYPE, WC_HIDDEN, WC_NAME, WC_FORM, WC_VALUE, id, nullptr))->closing(false));
    this->add(child);
  } 
};  

const static char WC_SCRIPT_TEST[] PROGMEM = R"=====(
const xhr = new XMLHttpRequest();
xhr.open("POST", "/events");
xhr.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
const body = JSON.stringify({
  form: window.location.href.substring(window.location.href.lastIndexOf('/') + 1),
  id: elem.id,
  value: elem.value
});
xhr.onload = () => {
  if (xhr.readyState == 4 && xhr.status == 200) {    
    document.write(xhr.responseText);
  } else {
    console.log(`Error: ${xhr.status}`);
  }
};
xhr.send(body);
)=====";

class WebButton : public WebControl {
 public:
  WebButton(const char* title, const char* id = nullptr) : WebControl(WC_BUTTON, nullptr) {
    if (id) addParam(WC_ID, id);
    content(title);
  }
  ~WebButton() {}

  virtual void createStyles(WStringList* styles) {
    styles->add(WC_STYLE_BUTTON, WC_BUTTON);
    styles->add(WC_STYLE_BUTTON_HOVER, WC_CSS_BUTTON_HOVER);
    WebControl::createStyles(styles);
  }

  virtual void createScripts(WStringList* scripts) {     
    if (hasParam(WC_ON_CLICK)) scripts->add(WC_SCRIPT_TEST, "onButtonClick");  
    WebControl::createScripts(scripts);
  }


  void onClickNavigateBack() { addParam(WC_ON_CLICK, WC_HISTORY_BACK); }

  WebButton* onClickNavigateTo(const char* target) { addParam(WC_ON_CLICK, WC_LOCATION_HREF, target, nullptr); return this; }

  WebButton* onClickSendValue(const char* value) {
    this->addParam(WC_VALUE, value);
    this->addParam(WC_ON_CLICK, PSTR("onButtonClick(this)"));
    return this;
  }

  virtual void toString(Print* stream) { 
    WebControl::toString(stream); 
  }

 protected:

};

class WebSubmitButton : public WebControl {
 public:
  WebSubmitButton(const char* title) : WebControl(WC_BUTTON, WC_TYPE, WC_SUBMIT, nullptr) {
    content(title);
  }

  virtual void createStyles(WStringList* styles) {
    styles->add(WC_STYLE_BUTTON, WC_BUTTON);
    styles->add(WC_STYLE_BUTTON_HOVER, WC_CSS_BUTTON_HOVER);
    WebControl::createStyles(styles);
  }
};

class WebLabel : public WebControl {
 public: 
  WebLabel(const char* title, const char* forId = nullptr) : WebControl(WC_LABEL, nullptr) {  
    if (forId) addParam(WC_FOR, forId);
    content(title);
  }    

  virtual void createStyles(WStringList* styles) {
    styles->add(PSTR("display:block;"), WC_LABEL);
    WebControl::createStyles(styles);
  }
};

class WebCheckbox : public WebControl {
 public:
  WebCheckbox(const char* id, const char* title) : WebControl(WC_DIV, WC_CLASS, "cb", nullptr) {    
    this->add((new WebControl(WC_INPUT, WC_ID, id, WC_TYPE, "checkbox", nullptr))->closing(false));    
    this->add(new WebLabel(title, id));
  }

  virtual void createStyles(WStringList* styles) {
    styles->add(WC_STYLE_CHECK_BOX, WC_CSS_CHECK_BOX);
    styles->add(WC_STYLE_CHECK_BOX_LABEL, WC_CSS_CHECK_BOX_LABEL);
    styles->add(WC_STYLE_CHECK_BOX_LABEL_BEFORE, WC_CSS_CHECK_BOX_LABEL_BEFORE);
    styles->add(WC_STYLE_CHECK_BOX_CHECKED_LABEL_BEFORE, WC_CSS_CHECK_BOX_CHECKED_LABEL_BEFORE);
    styles->add(WC_STYLE_INPUT_CHECKED_SLIDER, WC_CSS_INPUT_CHECKED_SLIDER);
    styles->add(WC_STYLE_INPUT_CHECKED_SLIDER_BEFORE, WC_CSS_INPUT_CHECKED_SLIDER_BEFORE);
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

  virtual void createStyles(WStringList* styles) {
    styles->add(WC_STYLE_SWITCH, WC_CSS_SWITCH);
    styles->add(WC_STYLE_SWITCH_INPUT, WC_CSS_SWITCH_INPUT);
    styles->add(WC_STYLE_SLIDER, WC_CSS_SLIDER);
    styles->add(WC_STYLE_SLIDER_BEFORE, WC_CSS_SLIDER_BEFORE);
    styles->add(WC_STYLE_INPUT_CHECKED_SLIDER, WC_CSS_INPUT_CHECKED_SLIDER);
    styles->add(WC_STYLE_INPUT_CHECKED_SLIDER_BEFORE, WC_CSS_INPUT_CHECKED_SLIDER_BEFORE);
    WebControl::createStyles(styles);
  }
};

class WebTextField : public WebControl {
 public:
  WebTextField(const char* id, const char* title, const char* text, byte maxLength = 32, bool passwordField = false) : WebControl(WC_DIV, nullptr) {    
    this->add(new WebLabel(title, id));
    WebControl* input = new WebControl(WC_INPUT, WC_ID, id, WC_NAME, id, WC_MAXLENGTH, String(maxLength).c_str(), WC_TYPE, (passwordField ? WC_PASSWORD : WC_TEXT), nullptr);
    if (text != nullptr) {
      input->addParam(WC_VALUE, text);
    }
    input->closing(false);
    this->add(input);
  }

};

class WebTextArea : public WebControl {
 public:
  WebTextArea(const char* id, const char* title, const char* text, byte rows = 20) : WebControl(WC_DIV, nullptr) {    
    this->add(new WebLabel(title, id));
    WebControl* input = new WebControl(WC_TEXTAREA, WC_ID, id, WC_NAME, id, WC_ROWS, String(rows).c_str(), nullptr);
    if (text != nullptr) {
      input->content(text);
    }
    this->add(input);
    
  }

};

class WebInputFile : public WebControl {
 public: 
  WebInputFile(const char* id) : WebControl(WC_DIV, nullptr) {    
    this->add(new WebLabel(PSTR("Add file"), id));
    WebControl* input = new WebControl(WC_INPUT, WC_ID, id, WC_NAME, id, WC_TYPE, WC_FILE, WC_ACCEPT, PSTR(".bin"), nullptr);
    input->closing(false);
    this->add(input);
    this->addParam(WC_CLASS, WC_BUTTON);
  }

  virtual void createStyles(WStringList* styles) {
    //styles->add(WC_STYLE_BUTTON, PSTR("input::file-selector-button"));    
    //styles->add("width:fit-content", PSTR("input::file-selector-button"));    
    WebControl::createStyles(styles);
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