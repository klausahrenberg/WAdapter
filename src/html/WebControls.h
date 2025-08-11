#ifndef W_WEB_CONTROLS_H
#define W_WEB_CONTROLS_H

#include "WebResources.h"
#include "WebSockets.h"

class WebControl {
 public:
  typedef std::function<void(const char*)> WebControlHandler;
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
        param(key, params);
        delete key;
        key = nullptr;
      }
      params = va_arg(arg, const char*);
    }
    if (key != nullptr) {
      param(key, nullptr);
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

  typedef std::function<void(Print* stream)> WOnPrint;
  void contentFactory(WOnPrint contentFactory) {
    _contentFactory = contentFactory;
  }

  WebControl* content(const char* content) {
    if (_content) delete _content;
    _content = new char[strlen_P(content) + 1];
    strcpy_P(_content, content);
    WebSockets::sendMessage("textAreaUpdate", id(), _content);
    return this;
  }

  const char* content() { return _content; }

  WebControl* closing(bool closing) {
    _closing = closing;
    return this;
  }

  bool closing() { return _closing; }

  void add(WebControl* kv) {
    if (kv != nullptr) {
      if (_items == nullptr) _items = new WList<WebControl>();
      _items->add(kv, kv->param(WC_ID));
    }
  }

  virtual WebControl* param(const char* key, const char* value) {
    return param(key, value, nullptr);
  }

  virtual WebControl* param(const char* key, const char* pattern, const char* params, ...) {
    if (_params == nullptr) _params = new WStringList();
    if ((pattern != nullptr) && (params != nullptr)) {
      va_list args2;
      va_start(args2, params);
      char buffer[128];
      snprintf(buffer, sizeof(buffer), pattern, params, args2);
      va_end(args2);
      _params->add(buffer, key);
    } else {
      _params->add(pattern, key);
    }
    return this;
  }

  virtual bool hasParam(const char* key) {
    return (param(key) != nullptr);
  }

  virtual const char* param(const char* key) {
    return (_params != nullptr ? _params->getById(key) : nullptr);
  }

  virtual const char* id() {
    return param(WC_ID);
  }

  virtual WebControl* id(const char* id) {
    param(WC_ID, id);
    return this;
  }

  virtual const char* value() {
    return param(WC_VALUE);
  }

  virtual WebControl* value(const char* value) {
    param(WC_VALUE, value);
    return this;
  }

  virtual void createStyles(WStringList* styles) {
    if (_items) _items->forEach([this, styles](int index, WebControl* wc, const char* id) { wc->createStyles(styles); });
  }

  virtual void createScripts(WStringList* scripts) {
    if (_items) _items->forEach([this, scripts](int index, WebControl* wc, const char* id) { wc->createScripts(scripts); });
  }

  virtual void toString(Print* stream) {
    WHtml::command(stream, _tag, true, _params);
    if (_contentFactory) {
      _contentFactory(stream);
    } else if (_content) {
      stream->print(_content);
    }
    if (_items) _items->forEach([this, stream](int index, WebControl* wc, const char* id) { wc->toString(stream); });
    if (_closing) WHtml::command(stream, _tag, false, nullptr);
  }

  WList<WebControl>* items() { return _items; }

  WebControl* getElementById(const char* id) {
    if (_items != nullptr) {
      WebControl* result = _items->getById(id);
      for (int i = 0; ((result == nullptr) && (i < _items->size())); i++) {
        result = _items->get(i)->getElementById(id);
      }
      return result;
    } else {
      return nullptr;
    }
  }

  virtual void handleEvent(WValue* event, WList<WValue>* data) {
  }

 protected:
  char* _tag = nullptr;
  char* _content = nullptr;
  WOnPrint _contentFactory = nullptr;
  bool _closing = true;
  WStringList* _params = nullptr;
  WList<WebControl>* _items = nullptr;
};

class WebDiv : public WebControl {
 public:
  WebDiv(WebControl* child) : WebControl(WC_DIV, nullptr) {
    this->add(child);
  }

  virtual ~WebDiv() {
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
function onButtonClick(elem) {
  const xhr = new XMLHttpRequest();
  xhr.open("POST", "/events");
  xhr.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
  var inputs = document.querySelectorAll('input');    
  var payload = {};
  payload["form"] = window.location.href.substring(window.location.href.lastIndexOf('/') + 1);
  payload["id"] = elem.id;
  payload["value"] = elem.value;
  for (var i = 0; i < inputs.length; i++) {
    payload[inputs[i].id] = inputs[i].value;
  }
  const body = JSON.stringify(payload);
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
    if (id) param(WC_ID, id);
    content(title);
  }

  virtual ~WebButton() {
  }

  virtual void createStyles(WStringList* styles) {
    styles->add(WC_STYLE_BUTTON, WC_BUTTON);
    styles->add(WC_STYLE_BUTTON_HOVER, WC_CSS_BUTTON_HOVER);
    WebControl::createStyles(styles);
  }

  virtual void createScripts(WStringList* scripts) {
    WebControl::createScripts(scripts);
    if (hasParam(WC_ON_CLICK)) scripts->add(WC_SCRIPT_CONTROL_EVENT, WC_SCRIPT_NAME_CONTROL_EVENT);
  }

  void onClickNavigateBack() { param(WC_ON_CLICK, WC_HISTORY_BACK); }

  WebButton* onClickNavigateTo(const char* target) {
    param(WC_ON_CLICK, WC_LOCATION_HREF, target, nullptr);
    return this;
  }

  WebButton* onClickSubmit(const char* value) {
    param(WC_VALUE, value);
    param(WC_NAME, WC_VALUE);
    param(WC_TYPE, WC_SUBMIT);
    return this;
  }

  virtual void toString(Print* stream) {
    WebControl::toString(stream);
  }

  WebButton* onClick(WebControlHandler onClick) {
    param(WC_ON_CLICK, WC_SCRIPT_NAME_CONTROL_EVENT, WC_ON_CLICK, nullptr);    
    _onClick = onClick;
    return this;
  }

  virtual void handleEvent(WValue* event, WList<WValue>* data) {
    WebControl::handleEvent(event, data);
    LOG->debug("handle click ");
    if ((event->equalsString(WC_ON_CLICK)) && (_onClick)) _onClick(nullptr);
  }

 private:
  WebControlHandler _onClick = nullptr;
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
    if (forId) param(WC_FOR, forId);
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

class WebInput : public WebControl {
 public:
  WebInput(const char* id, const char* value = nullptr, byte maxLength = 32, bool passwordField = false) : 
    WebControl(WC_INPUT, WC_ID, id, WC_NAME, id, WC_MAXLENGTH, String(maxLength).c_str(), WC_TYPE, (passwordField ? WC_PASSWORD : WC_TEXT), nullptr) {
    if (value != nullptr) {
      param(WC_VALUE, value);
    }
    param(WC_ON_CHANGE, WC_SCRIPT_NAME_CONTROL_EVENT, WC_ON_CHANGE, nullptr);
    closing(false);
  } 

  virtual void createScripts(WStringList* scripts) {
    WebControl::createScripts(scripts);
    scripts->add(WC_SCRIPT_CONTROL_EVENT, WC_SCRIPT_NAME_CONTROL_EVENT);
  }  

};

class WebLabeledControl : public WebControl {
 public:
  WebLabeledControl(const char* title, WebControl* control) : WebControl(WC_DIV, nullptr) {
    _control = control;
    this->add(new WebLabel(title, control->id()));
    this->add(_control);
  }

  virtual WebControl* param(const char* key, const char* value) {
    _control->param(key, value);
    return this;
  }

  virtual WebControl* param(const char* key, const char* pattern, const char* params, ...) {
    _control->param(key, pattern, params);
    return this;
  }

  virtual bool hasParam(const char* key) {
    return _control->hasParam(key);
  }
  
  virtual const char* param(const char* key) {
    return _control->param(key);
  }

  virtual void handleEvent(WValue* event, WList<WValue>* data) {        
    _control->handleEvent(event, data);
  }

  protected:
   WebControl* _control;
};

class WebTextField : public WebLabeledControl {
 public:
  WebTextField(const char* id, const char* title, const char* text = nullptr, byte maxLength = 32, bool passwordField = false) 
   : WebLabeledControl(title, new WebInput(id, text, maxLength, passwordField)) {    
  }

  virtual void handleEvent(WValue* event, WList<WValue>* data) {    
    WebControl::handleEvent(event, data);
    if (event->equalsString(WC_ON_CHANGE)) {
      value(data->getById(WC_VALUE)->asString());
    }
  }

};

class WebTextArea : public WebLabeledControl {
 public:
  WebTextArea(const char* id, const char* title, WOnPrint textFactory, byte rows = 20, byte cols = 80) 
   : WebLabeledControl(title, new WebControl(WC_TEXTAREA, WC_ID, id, WC_NAME, id, WC_ROWS, String(rows).c_str(), WC_COLS, String(cols).c_str(), nullptr)) {    
    if (textFactory != nullptr) _control->contentFactory(textFactory);    
  }

  virtual void createScripts(WStringList* scripts) {
    WebControl::createScripts(scripts);
    scripts->add(WC_SCRIPT_TEXTAREA);
  }

};

class WebInputFile : public WebControl {
 public:
  WebInputFile(const char* id) : WebControl(WC_DIV, nullptr) {
    this->add(new WebLabel(PSTR("Add file"), id));
    WebControl* input = new WebControl(WC_INPUT, WC_ID, id, WC_NAME, id, WC_TYPE, WC_FILE, WC_ACCEPT, PSTR(".bin"), nullptr);
    input->closing(false);
    this->add(input);
    this->param(WC_CLASS, WC_BUTTON);
  }

  virtual void createStyles(WStringList* styles) {
    // styles->add(WC_STYLE_BUTTON, PSTR("input::file-selector-button"));
    // styles->add("width:fit-content", PSTR("input::file-selector-button"));
    WebControl::createStyles(styles);
  }
};

template <typename T>
class WebTable : public WebControl {
 public:
  static bool test() {
    return true;
  }

  static void headerCell(Print* stream, const char* header) {
    WHtml::command(stream, WC_TABLE_HEADER, true, nullptr);
    if (header) stream->print(header);
    WHtml::command(stream, WC_TABLE_HEADER, false, nullptr);
  }

  static void dataCell(Print* stream, const char* data) {
    WHtml::command(stream, WC_TABLE_DATA, true, nullptr);
    if (data) stream->print(data);
    WHtml::command(stream, WC_TABLE_DATA, false, nullptr);
  }

  WebTable(IWIterable<T>* datas) : WebControl(WC_TABLE, nullptr) {
    _datas = datas;
  }

  virtual ~WebTable() {
  }

  typedef std::function<void(Print*, int, T*, const char*)> TOnPrintRow;
  virtual void printRow(Print* stream, int index, T* item, const char* id) {
    if (_onPrintRow) _onPrintRow(stream, index, item, id);
  }

  WebTable* onPrintRow(TOnPrintRow onPrintRow) {
    _onPrintRow = onPrintRow;
    return this;
  }

  virtual void toString(Print* stream) {
    WHtml::command(stream, _tag, true, _params);
    _datas->forEach([this, stream](int index, T* item, const char* id) {
      WHtml::command(stream, WC_TABLE_ROW, true, nullptr);
      this->printRow(stream, index, item, id);
      WHtml::command(stream, WC_TABLE_ROW, false, nullptr);
    });
    if (_closing) WHtml::command(stream, _tag, false, nullptr);
  }

 private:
  IWIterable<T>* _datas;
  TOnPrintRow _onPrintRow;
};

class WebFieldset : public WebControl {
 public:
  WebFieldset(const char* legend) : WebControl(WC_FIELDSET, nullptr) {
    this->add((new WebControl(WC_LEGEND, nullptr))->content(legend));
  }

  virtual ~WebFieldset() {
  }
};

class WebCombobox : public WebControl {
 public:
  WebCombobox(const char* id, const char* title) : WebControl(WC_DIV, nullptr) {
    this->add(new WebLabel(title, id));
    _select = new WebControl(WC_SELECT, WC_ID, id, WC_NAME, id, nullptr);
    this->add(_select);
  }

  WebCombobox* option(const char* option, bool selected) {
    _select->add((new WebControl(WC_OPTION,
                                 WC_VALUE, option,
                                 (selected ? WC_SELECTED : ""), (selected ? "" : nullptr), nullptr))
                     ->content(option));
    return this;
  }

 private:
  WebControl* _select;
};

#endif