#ifndef W_WEB_CONTROLS_H
#define W_WEB_CONTROLS_H

#include "WebAppSockets.h"
#include "WebResources.h"

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
        delete[] key;
        key = nullptr;
      }
      params = va_arg(arg, const char*);
    }
    if (key != nullptr) {
      param(key, nullptr);
      delete[] key;
      key = nullptr;
    }
    va_end(arg);
  }

  virtual ~WebControl() {
    if (_tag) delete[] _tag;
    if (_content) delete[] _content;
    if (_params) delete _params;
    if (_items) delete _items;
  }

  typedef std::function<void(Print* stream)> WOnPrint;
  void contentFactory(WOnPrint contentFactory) {
    _contentFactory = contentFactory;
  }

  virtual WebControl* content(const char* content) {
    if (_content) delete[] _content;
    _content = WString::duplicate(content);
    //news for the browser is every content that arrives after the control was
    //printed, everything before that goes out with the page itself
    if (_printed) WebAppSockets::sendMessage(WC_EVENT_TEXTAREA_UPDATE, id(), _content);
    return this;
  }

  virtual const char* content() { return _content; }

  /**
   * Whether the browser shows this control. A page that is already printed is
   * told about it, so a card can come and go while the page stands open.
   */
  virtual WebControl* visible(bool visible) {
    if (_visible != visible) {
      _visible = visible;
      if (_printed) WebAppSockets::sendMessage(WC_EVENT_ELEMENT_SHOW, id(), (visible ? WC_TRUE : WC_FALSE));
    }
    return this;
  }

  virtual bool visible() { return _visible; }

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
      //both the pattern and what is put into it can lie in the flash, so the
      //pattern is read byte safe and the value is copied to the ram before
      char buffer[128];
      String value = String(FPSTR(params));
      snprintf_P(buffer, sizeof(buffer), pattern, value.c_str());
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
    //the attribute the browser hides a control with. The own params are meant
    //here, a wrapper hides itself and not what it wraps
    if (!_visible) WebControl::param(WC_HIDDEN, WC_TRUE);
    WHtml::command(stream, _tag, true, _params);
    if (_contentFactory) {
      _contentFactory(stream);
    } else if (_content) {
      stream->print(_content);
    }
    if (_items) _items->forEach([this, stream](int index, WebControl* wc, const char* id) { wc->toString(stream); });
    if (_closing) WHtml::command(stream, _tag, false, nullptr);
    _printed = true;
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
  //tells whether the browser has seen this control already
  bool _printed = false;
  bool _visible = true;
  WStringList* _params = nullptr;
  WList<WebControl>* _items = nullptr;
};

class WebDiv : public WebControl {
 public:
  WebDiv() : WebControl(WC_DIV, nullptr) {
    
  }

  WebDiv(WebControl* child) : WebControl(WC_DIV, nullptr) {
    this->add(child);
  }
};

class WebSpan : public WebControl {
 public:
  WebSpan(const char* className, WebControl* child) : WebControl(WC_SPAN, nullptr) {
    this->param(WC_CLASS, className);
    this->add(child);
  }  
};

/**
 * A card with a heading and a row per value: the look of the control panel.
 * A row carries the label at the left and the control at the right, a control
 * that is a box of its own (class 'ctl') is taken as that box.
 */
class WebCard : public WebControl {
 public:
  WebCard(const char* title, const char* subTitle = nullptr) : WebControl(WC_SECTION, WC_CLASS, CSS_CARD_CLASS, nullptr) {
    _head = new WebControl(WC_H3, nullptr);
    _head->content(title);
    //subTitle can point to PROGMEM, so don't compare it byte wise
    if ((subTitle != nullptr) && (strlen_P(subTitle) != 0)) {
      _head->add((new WebControl(WC_SPAN, WC_CLASS, WC_CLASS_CARD_TYPE, nullptr))->content(subTitle));
    }
    this->add(_head);
  }

  virtual void createStyles(WStringList* styles) {
    WebControl::createStyles(styles);
    styles->add(WC_STYLE_FORM_PLAIN, WC_FORM);
    styles->add(CSS_CARD_STYLE, CSS_CARD_ID);
    styles->add(CSS_CARD_H3_STYLE, CSS_CARD_H3_ID);
    styles->add(CSS_CARD_ROW_STYLE, CSS_CARD_ROW_ID);
    styles->add(CSS_CARD_ROW_READ_ONLY_STYLE, CSS_CARD_ROW_READ_ONLY_ID);
    styles->add(CSS_CARD_LABEL_STYLE, CSS_CARD_LABEL_ID);
    styles->add(CSS_CARD_CTL_STYLE, CSS_CARD_CTL_ID);
    //the rules come after the ones of the buttons themselves, so a tool keeps
    //the look of its kind and is only made small and square here
    if (_tools != nullptr) {
      styles->add(CSS_CARD_TOOLS_STYLE, CSS_CARD_TOOLS_ID);
      styles->add(CSS_CARD_TOOLS_BUTTON_STYLE, CSS_CARD_TOOLS_BUTTON_ID);
    }
  }
  /*
  virtual void createStyles(WStringList* styles) {
    //the rule comes after the one of the page, so a form around the cards
    //loses the box it would draw behind them
    styles->add(WC_STYLE_FORM_PLAIN, WC_FORM);
    WebControl::createStyles(styles);
  }*/

  WebCard* addRow(const char* label, WebControl* control, bool readOnly = false) {
    WebControl* row = new WebControl(WC_DIV, WC_CLASS, (readOnly ? CSS_CARD_ROW_READ_ONLY_CLASS : CSS_CARD_ROW_CLASS), nullptr);
    row->add((new WebControl(WC_DIV, WC_CLASS, CSS_CARD_LABEL_CLASS, nullptr))->content(label));
    const char* cls = control->param(WC_CLASS);
    if ((cls != nullptr) && (strcmp_P(cls, CSS_CARD_CTL_CLASS) == 0)) {
      row->add(control);
    } else {
      WebControl* box = new WebControl(WC_DIV, WC_CLASS, CSS_CARD_CTL_CLASS, nullptr);
      box->add(control);
      row->add(box);
    }
    this->add(row);
    return this;
  }

  /** A row that shows a text only, at the right where every value stands. */
  WebCard* addRow(const char* label, const char* value) {
    return addRow(label, (new WebControl(WC_SPAN, WC_CLASS, WC_CLASS_VALUE, nullptr))->content(value), true);
  }

  /** Something that is no row of its own: a table, a text area, a file. */
  WebCard* addContent(WebControl* content) {
    WebControl* box = new WebControl(WC_DIV, WC_CLASS, CSS_CARD_ROW_CLASS, nullptr);
    box->add(content);
    this->add(box);
    return this;
  }

  /** A line instead of rows, where a card has nothing to show. */
  WebCard* addMessage(const char* message) {
    this->add((new WebControl(WC_DIV, WC_CLASS, WC_CLASS_MESSAGE, nullptr))->content(message));
    return this;
  }

  /** A line of text under the rows, the address of the device for example. */
  WebCard* addNote(const char* note) {
    this->add((new WebControl(WC_DIV, WC_CLASS, CSS_CARD_ROW_READ_ONLY_CLASS, nullptr))->content(note));
    return this;
  }

  /**
   * A small button at the right end of the title, for what acts on the card as
   * a whole - adding a row, removing what is selected. Tools stand in the head
   * and keep their place there, whatever the card shows below and however long
   * that gets. They are added in the order they are meant to be read in.
   */
  WebCard* addTool(WebControl* tool) {
    if (tool != nullptr) {
      if (_tools == nullptr) {
        _tools = new WebControl(WC_DIV, WC_CLASS, CSS_CARD_TOOLS_CLASS, nullptr);
        _head->add(_tools);
      }
      _tools->add(tool);
    }
    return this;
  }

 private:
  WebControl* _head;
  //the box the tools stand in, made at the first one so a card without them
  //prints no empty box and asks for no rule of its own
  WebControl* _tools = nullptr;
};

class WebForm : public WebControl {
 public:
  //the target is absolute: a page is reachable both as '/wifi' and, after the
  //captive portal redirect, as '/wifi/' - a relative target would turn into
  //'/wifi/events' there, which no handler answers, so the post would end up in
  //the redirect of the unknown-url handler and silently lose the form
  WebForm(const char* id, WebControl* child = nullptr) : WebControl(WC_FORM, WC_METHOD, WC_POST, WC_ACTION, "/events", nullptr) {
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
    //html reads a button without a type as a submit one, so a button in a form
    //would send it off besides doing what it was made for
    param(WC_TYPE, WC_BUTTON);
    if (id) param(WC_ID, id);
    content(title);
  }

  virtual ~WebButton() {
  }

  virtual void createStyles(WStringList* styles) {
    WebControl::createStyles(styles);
    styles->add(CSS_BUTTON_STYLE, WC_BUTTON);
    styles->add(CSS_BUTTON_HOVER_STYLE, CSS_BUTTON_HOVER_ID);
    styles->add(CSS_BUTTON_ON_STYLE, CSS_BUTTON_ON_ID);
    if (_danger) {
      styles->add(WC_STYLE_BUTTON_DANGER, WC_CSS_BUTTON_DANGER);
      styles->add(WC_STYLE_BUTTON_DANGER_HOVER, WC_CSS_BUTTON_DANGER_HOVER);
    }
  }

  virtual void createScripts(WStringList* scripts) {
    WebControl::createScripts(scripts);
    if (hasParam(WC_ON_CLICK)) scripts->add(WC_SCRIPT_CONTROL_EVENT, WC_SCRIPT_NAME_CONTROL_EVENT);
  }

  /** Marks an action that cannot be taken back. */
  WebButton* danger() {
    param(WC_CLASS, WC_DANGER);
    _danger = true;
    return this;
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
    if ((event->equals(WC_ON_CLICK)) && (_onClick)) _onClick(nullptr);
  }

 private:
  WebControlHandler _onClick = nullptr;
  bool _danger = false;
};

class WebIconButton : public WebButton {
 public:
  WebIconButton(const char* title, const char* id = nullptr) : WebButton(title, id) {
    param(WC_CLASS, CSS_BUTTON_ICON_CLASS);
  }

  virtual void createStyles(WStringList* styles) {
    WebButton::createStyles(styles);
    styles->add(CSS_BUTTON_ICON_STYLE, CSS_BUTTON_ICON_ID);
    styles->add(CSS_BUTTON_ICON_HOVER_STYLE, CSS_BUTTON_ICON_HOVER_ID);
  }
};    

class WebSubmitButton : public WebControl {
 public:
  WebSubmitButton(const char* title) : WebControl(WC_BUTTON, WC_TYPE, WC_SUBMIT, nullptr) {
    content(title);
  }

  virtual void createStyles(WStringList* styles) {
    styles->add(CSS_BUTTON_STYLE, WC_BUTTON);
    styles->add(CSS_BUTTON_HOVER_STYLE, CSS_BUTTON_HOVER_ID);
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
    this->add((new WebControl(WC_INPUT, WC_ID, id, WC_TYPE, WC_CHECKBOX, nullptr))->closing(false));
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
    WebControl* input = new WebControl(WC_INPUT, WC_ID, id, WC_TYPE, WC_CHECKBOX, WC_ON_CHANGE, "toggleCheckbox(this)", WC_CHECKED, nullptr);
    input->closing(false);
    this->add(input);
    /*WebControl* label = new WebControl(WC_LABEL, WC_FOR, id, nullptr);
    label->content(title);
    this->add(label);*/

    WebControl* slider = new WebControl(WC_SPAN, WC_CLASS, "slider", nullptr);
    this->add(slider);
  }

  virtual void createStyles(WStringList* styles) {
    WebControl::createStyles(styles);
    styles->add(WC_STYLE_SWITCH, WC_CSS_SWITCH);
    styles->add(WC_STYLE_SWITCH_INPUT, WC_CSS_SWITCH_INPUT);
    styles->add(WC_STYLE_SLIDER, WC_CSS_SLIDER);
    styles->add(WC_STYLE_SLIDER_BEFORE, WC_CSS_SLIDER_BEFORE);
    styles->add(WC_STYLE_INPUT_CHECKED_SLIDER, WC_CSS_INPUT_CHECKED_SLIDER);
    styles->add(WC_STYLE_INPUT_CHECKED_SLIDER_BEFORE, WC_CSS_INPUT_CHECKED_SLIDER_BEFORE);
  }
};

class WebLink : public WebControl {
 public:
  WebLink(const char* target, const char* title = nullptr) : WebControl("a", WC_HREF, target, nullptr) {
    content(title != nullptr ? title : target);
  }
};

class WebInput : public WebControl {
 public:
  WebInput(const char* id, const char* value = nullptr, byte maxLength = 32, bool passwordField = false) : WebControl(WC_INPUT, WC_ID, id, WC_NAME, id, WC_MAXLENGTH, String(maxLength).c_str(), WC_TYPE, (passwordField ? WC_PASSWORD : WC_TEXT), nullptr) {
    if (value != nullptr) {
      param(WC_VALUE, value);
    }
    param(WC_ON_CHANGE, WC_SCRIPT_NAME_CONTROL_EVENT, WC_ON_CHANGE, nullptr);
    closing(false);
  }

  virtual void createStyles(WStringList* styles) {
    WebControl::createStyles(styles);
    styles->add(CSS_INPUT_STYLE, CSS_INPUT_ID);
  }  

  virtual void createScripts(WStringList* scripts) {
    WebControl::createScripts(scripts);
    scripts->add(WC_SCRIPT_CONTROL_EVENT, WC_SCRIPT_NAME_CONTROL_EVENT);
    if (_onEnter) scripts->add(WC_SCRIPT_CONTROL_ENTER, WC_SCRIPT_NAME_CONTROL_ENTER);
  }

  //the getter of the base stays reachable next to the setter below
  using WebControl::value;

  /**
   * A value the device learns while the page is open - a name a module
   * answers with, say - is written into the field of the browser as well.
   * A field that is being typed in right now keeps what stands in it, that is
   * settled in the browser where it is known who has the cursor.
   */
  virtual WebControl* value(const char* value) {
    const char* current = this->value();
    bool changed = ((current == nullptr) || (value == nullptr) || (strcmp_P(current, value) != 0));
    WebControl::value(value);
    if ((_printed) && (changed)) WebAppSockets::sendMessage(WC_EVENT_VALUE_UPDATE, id(), this->value());
    return this;
  }

  /**
   * Enter in the field, for what the field is typed in for: sending it off.
   * The handler is given the value that was typed, it is stored before.
   */
  WebInput* onEnter(WebControlHandler onEnter) {
    param(WC_ON_KEYDOWN, WC_SCRIPT_NAME_CONTROL_ENTER);
    _onEnter = onEnter;
    return this;
  }

  virtual void handleEvent(WValue* event, WList<WValue>* data) {
    WebControl::handleEvent(event, data);
    bool enter = event->equals(WC_ON_ENTER);
    if ((event->equals(WC_ON_CHANGE)) || (enter)) {
      //an event that carries no value at all is no reason to go down
      WValue* v = (data != nullptr ? data->getById(WC_VALUE) : nullptr);
      //the value comes from the browser here, it does not have to be sent back
      if (v != nullptr) param(WC_VALUE, v->asString());
    }
    if ((enter) && (_onEnter)) _onEnter(value());
  }

 private:
  WebControlHandler _onEnter = nullptr;
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

  //the label and the control live in this div, a content belongs to the
  //control inside - a text area writes its text there, not into the wrapper
  virtual WebControl* content(const char* content) {
    _control->content(content);
    return this;
  }

  virtual const char* content() { return _control->content(); }

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
    if (event->equals(WC_ON_CHANGE)) {
      //an event that carries no value at all is no reason to go down
      WValue* v = (data != nullptr ? data->getById(WC_VALUE) : nullptr);
      if (v != nullptr) value(v->asString());
    }
  }
};

class WebTextArea : public WebLabeledControl {
 public:
  /** json: the content is laid out, checked and refused when it is broken. */
  WebTextArea(const char* id, const char* title, WOnPrint textFactory, byte rows = 20, byte cols = 80, bool json = false)
      : WebLabeledControl(title, new WebControl(WC_TEXTAREA, WC_ID, id, WC_NAME, id, WC_ROWS, String(rows).c_str(), WC_COLS, String(cols).c_str(),
                                                WC_SPELLCHECK, WC_FALSE, PSTR("autocapitalize"), PSTR("off"), PSTR("wrap"), PSTR("off"), nullptr)) {
    if (textFactory != nullptr) _control->contentFactory(textFactory);
    if (json) _control->param(PSTR("data-json"), WC_TRUE);
  }

  /**
   * Adds text at the end of what the browser shows - for a monitor that only
   * grows. Only the new text goes over the socket, the log itself stays in the
   * browser: a text that is sent as a whole every time runs into the size of a
   * packet after a while, and a message that is cut off there is no json
   * anymore.
   */
  WebTextArea* appendContent(const char* text) {
    if ((text != nullptr) && (_control->id() != nullptr)) {
      WebAppSockets::sendMessage(WC_EVENT_TEXTAREA_APPEND, _control->id(), text);
    }
    return this;
  }

  virtual void createStyles(WStringList* styles) {
    WebControl::createStyles(styles);
    styles->add(WC_STYLE_TEXTAREA, WC_TEXTAREA);
  }

  virtual void createScripts(WStringList* scripts) {
    WebControl::createScripts(scripts);
    //the id keeps the script at one copy, however many textareas a page has
    scripts->add(WC_SCRIPT_TEXTAREA, WC_TEXTAREA);
  }
};

class WebInputFile : public WebControl {
 public:
  WebInputFile(const char* id, const char* label = nullptr) : WebControl(WC_DIV, nullptr) {
    this->add(new WebLabel((label != nullptr ? label : PSTR("Add file")), id));
    WebControl* input = new WebControl(WC_INPUT, WC_ID, id, WC_NAME, id, WC_TYPE, WC_FILE, WC_ACCEPT, PSTR(".bin"), WC_STYLE, WC_DISPLAY_NONE, nullptr);
    input->closing(false);
    this->add(input);
    this->add((new WebButton(PSTR("Select file")))->param(WC_ON_CLICK, PSTR("document.getElementById('update').click()")));
    //the upload script finds this by tag and fills it in while it runs - no
    //id needed, and nothing shows here as long as no upload is in progress
    this->add(new WebControl(WC_PROGRESS, WC_VALUE, "0", WC_MAX, "100", nullptr));
    this->param(WC_CLASS, WC_BUTTON);
    this->param(WC_STYLE, WC_WIDTH_100PERCENT);
  }

  virtual void createStyles(WStringList* styles) {
    styles->add(WC_STYLE_FILE, WC_CSS_FILE);
    styles->add(WC_STYLE_FILE_BUTTON, WC_CSS_FILE_BUTTON);
    styles->add(WC_STYLE_FILE_BUTTON_HOVER, WC_CSS_FILE_BUTTON_HOVER);
    styles->add(WC_STYLE_PROGRESS, WC_CSS_PROGRESS);
    styles->add(WC_STYLE_PROGRESS_BAR, WC_CSS_PROGRESS_BAR);
    styles->add(WC_STYLE_PROGRESS_VALUE, WC_CSS_PROGRESS_VALUE);
    WebControl::createStyles(styles);
  }

  virtual void createScripts(WStringList* scripts) {
    WebControl::createScripts(scripts);
    scripts->add(WC_SCRIPT_FILE_UPLOAD, WC_SCRIPT_NAME_FILE_UPLOAD);
  }
};

#define SIZE_ROW_SNIPPET 512U

/**
 * A table of the items of a list. Rows are not kept as controls, they are
 * printed on demand, so a table costs no memory per cell. A row is addressed
 * by its index in the body and a cell by its index in the row, so no cell
 * needs an id and every event of the table reaches this control.
 */
template <typename T>
class WebTable : public WebControl {
 public:
  static void headerCell(Print* stream, const char* header) {
    WHtml::command(stream, WC_TABLE_HEADER, true, nullptr);
    //a header of a table can lie in the flash, so read it byte safe
    if (header) stream->print(FPSTR(header));
    WHtml::command(stream, WC_TABLE_HEADER, false, nullptr);
  }

  static void dataCell(Print* stream, const char* data, bool editable = false) {
    //empty value: html5 reads the bare attribute as contenteditable="true"
    WHtml::commandParamsAndNullptr(stream, WC_TABLE_DATA, true, (editable ? WC_CONTENT_EDITABLE : nullptr), nullptr);
    //a cell can lie in the flash, so read it byte safe
    if (data) stream->print(FPSTR(data));
    WHtml::command(stream, WC_TABLE_DATA, false, nullptr);
  }

  //spellcheck is inherited by the editable cells, which would be marked up otherwise
  WebTable(const char* id, WList<T>* datas) : WebControl(WC_TABLE, WC_ID, id, WC_SPELLCHECK, WC_FALSE, nullptr) {
    _datas = datas;
    if (_datas != nullptr) {
      _datas->addListener([this](WListChange<T> change) {
        switch (change.type) {
          case WListChangeType::ADDED: {
            WStringStream snippet(SIZE_ROW_SNIPPET);
            WListNode<T>* node = _datas->_getNode(change.index);
            printRow(&snippet, change.index, change.item, (node != nullptr ? node->id : nullptr));
            _notifyClient(WC_ADDED, change.index, snippet.c_str());
            _updateSelectAllBox();
            break;
          }
          case WListChangeType::REMOVED: {
            //the item is gone, so the selection must not point to it any more
            if (_selected != nullptr) {
              int i = _selected->indexOf(change.oldItem);
              if (i > -1) _selected->remove(i, false);
            }
            _notifyClient(WC_REMOVED, change.index, nullptr);
            _updateSelectAllBox();
            break;
          }
          case WListChangeType::CHANGED: {
            break;
          }
        }
      });
    }
  }

  virtual ~WebTable() {
    if (_datas != nullptr) _datas->removeListener();
    if (_selected != nullptr) {
      //the items belong to the list of the table, not to the selection
      _clearSelection();
      delete _selected;
    }
  }

  virtual void createStyles(WStringList* styles) {
    styles->add(WC_STYLE_ROOT, WC_CSS_ROOT);
    styles->add(WC_STYLE_TABLE, WC_TABLE);
    styles->add(WC_STYLE_TABLE_DATA, WC_TABLE_DATA);
    styles->add(WC_STYLE_TABLE_HEADER, WC_TABLE_HEADER);
    styles->add(WC_STYLE_TABLE_ZEBRA, WC_CSS_TABLE_ZEBRA);
    styles->add(WC_STYLE_TABLE_HOVER, WC_CSS_TABLE_HOVER);
    styles->add(WC_STYLE_TABLE_LAST_ROW, WC_CSS_TABLE_LAST_ROW);
    styles->add(WC_STYLE_TABLE_EDIT, WC_CSS_TABLE_EDIT);
    styles->add(WC_STYLE_TABLE_REFUSED, WC_CSS_TABLE_REFUSED);
    if (_selectable) styles->add(WC_STYLE_TABLE_CHECK_BOX, WC_CSS_TABLE_CHECK_BOX);
    WebControl::createStyles(styles);
  }

  virtual void createScripts(WStringList* scripts) {
    WebControl::createScripts(scripts);
    scripts->add(WC_SCRIPT_WEB_TABLE, WC_SCRIPT_WEB_TABLE_NAME);
  }

  typedef std::function<void(Print*, int, T*, const char*)> TOnPrintRow;
  /** Answers whether the edited text was taken over. */
  typedef std::function<bool(int, int, T*, const char*)> TOnCellChange;

  virtual void printRow(Print* stream, int index, T* item, const char* id) {
    WHtml::command(stream, WC_TABLE_ROW, true, nullptr);
    if (_selectable) _printSelectCell(stream, WC_TABLE_DATA, _isSelected(item));
    if (_onPrintRow) _onPrintRow(stream, index, item, id);
    WHtml::command(stream, WC_TABLE_ROW, false, nullptr);
  }

  WebTable* onPrintRow(TOnPrintRow onPrintRow) {
    _onPrintRow = onPrintRow;
    return this;
  }

  WebTable* onPrintHeaderRow(TOnPrintRow onPrintHeaderRow) {
    _onPrintHeaderRow = onPrintHeaderRow;
    return this;
  }

  /**
   * Called with the row, the column and the text a cell was left with. The
   * columns are counted as printed by onPrintRow, the box column of a
   * selectable table is not one of them. Returning false refuses the edit: the
   * cell falls back to the text it had and is marked for a moment.
   */
  WebTable* onCellChange(TOnCellChange onCellChange) {
    _onCellChange = onCellChange;
    return this;
  }

  bool selectable() { return _selectable; }

  /** Puts a box in front of every row and one in the header for all of them. */
  WebTable* selectable(bool selectable) {
    _selectable = selectable;
    if ((_selectable) && (_selected == nullptr)) _selected = new WList<T>();
    return this;
  }

  WList<T>* selectedRows() { return _selected; }

  bool freeRemovedItems() { return _freeRemovedItems; }

  /** Whether a row taken out by removeSelectedRows() is freed as well. */
  WebTable* freeRemovedItems(bool freeRemovedItems) {
    _freeRemovedItems = freeRemovedItems;
    return this;
  }

  /**
   * Selects or deselects every row. The browser is told once for the whole
   * table rather than once per row.
   */
  WebTable* selectAll(bool selected) {
    if (_selected != nullptr) {
      _clearSelection();
      if ((selected) && (_datas != nullptr)) {
        _datas->forEach([this](int index, T* item, const char* id) { _selected->add(item); });
      }
      _sendSelection(true, selected);
    }
    return this;
  }

  /**
   * Removes every selected row from the list of the table. The change listener
   * tells the client, just as adding a row does, and takes the row out of the
   * selection, so the selection is empty when this returns.
   */
  WebTable* removeSelectedRows() {
    if ((_datas != nullptr) && (_selected != nullptr)) {
      while (!_selected->empty()) {
        int index = _datas->indexOf(_selected->get(0));
        if (index > -1) {
          _datas->remove(index, _freeRemovedItems);
        } else {
          _selected->remove(0, false);
        }
      }
    }
    return this;
  }

  /** Puts a value into a cell of the browser, the table itself is untouched. */
  WebTable* updateCell(int index, int column, const char* value) {
    _sendCell(index, column, value, false);
    return this;
  }

  virtual void toString(Print* stream) {
    WHtml::command(stream, _tag, true, _params);
    if ((_onPrintHeaderRow) || (_selectable)) {
      WHtml::command(stream, WC_TABLE_HEAD, true, nullptr);
      WHtml::command(stream, WC_TABLE_ROW, true, nullptr);
      if (_selectable) _printSelectCell(stream, WC_TABLE_HEADER, false);
      if (_onPrintHeaderRow) _onPrintHeaderRow(stream, -1, nullptr, nullptr);
      WHtml::command(stream, WC_TABLE_ROW, false, nullptr);
      WHtml::command(stream, WC_TABLE_HEAD, false, nullptr);
    }
    WHtml::command(stream, WC_TABLE_BODY, true, nullptr);
    if (_datas != nullptr) {
      _datas->forEach([this, stream](int index, T* item, const char* id) {
        this->printRow(stream, index, item, id);
      });
    }
    WHtml::command(stream, WC_TABLE_BODY, false, nullptr);
    if (_closing) WHtml::command(stream, _tag, false, nullptr);
  }

  virtual void handleEvent(WValue* event, WList<WValue>* data) {
    if (data == nullptr) return;
    WValue* row = data->getById(WC_ROW);
    if (row == nullptr) return;
    WValue* value = data->getById(WC_VALUE);
    if (event->equals(WC_ON_CLICK)) {
      //the client reports the state the box ended up in, a box set from here
      //would answer the next click with the wrong state otherwise
      bool selected = ((value != nullptr) && (value->asBool()));
      if (row->asInt() < 0) {
        selectAll(selected);
      } else {
        _select(row->asInt(), selected);
      }
    } else if (event->equals(WC_ON_CHANGE)) {
      WValue* col = data->getById(WC_COL);
      int column = (col != nullptr ? col->asInt() : 0) - (_selectable ? 1 : 0);
      T* item = (_datas != nullptr ? _datas->get(row->asInt()) : nullptr);
      if ((item != nullptr) && (column > -1)) {
        if ((!_onCellChange) || (!_onCellChange(row->asInt(), column, item, (value != nullptr ? value->asString() : nullptr)))) {
          //not taken over: the browser puts the text back it kept at focus
          _sendCell(row->asInt(), column, nullptr, true);
        }
      }
    }
  }

 private:
  WList<T>* _datas;
  WList<T>* _selected = nullptr;
  bool _selectable = false;
  bool _freeRemovedItems = true;
  TOnPrintRow _onPrintHeaderRow = nullptr;
  TOnPrintRow _onPrintRow = nullptr;
  TOnCellChange _onCellChange = nullptr;

  bool _isSelected(T* item) {
    return ((_selected != nullptr) && (_selected->indexOf(item) > -1));
  }

  void _clearSelection() {
    while (!_selected->empty()) _selected->remove(0, false);
  }

  void _printSelectCell(Print* stream, const char* tag, bool checked) {
    WHtml::command(stream, tag, true, nullptr);
    WHtml::commandParamsAndNullptr(stream, WC_INPUT, true, WC_TYPE, WC_CHECKBOX, (checked ? WC_CHECKED : nullptr), nullptr);
    WHtml::command(stream, tag, false, nullptr);
  }

  void _select(int index, bool selected) {
    T* item = (_datas != nullptr ? _datas->get(index) : nullptr);
    if ((item == nullptr) || (_selected == nullptr)) return;
    int i = _selected->indexOf(item);
    if ((selected) && (i < 0)) {
      _selected->add(item);
    } else if ((!selected) && (i > -1)) {
      _selected->remove(i, false);
    }
    _updateSelectAllBox();
  }

  /**
   * Draws the box in the header as ticked when every row is selected, as the
   * third html5 state when only some are, and empty when none are.
   */
  void _updateSelectAllBox() {
    if (_selectable) _sendSelection(false, false);
  }

  void _sendSelection(bool allRows, bool selected) {
    int rows = (_datas != nullptr ? _datas->size() : 0);
    int sel = (_selected != nullptr ? _selected->size() : 0);
    WebAppSockets::sendJson(WC_SELECT_ROWS, id(), [allRows, selected, rows, sel](WJson* json) {
      if (allRows) json->propertyBoolean(WC_ALL, selected);
      json->propertyBoolean(WC_CHECKED, ((rows > 0) && (sel == rows)));
      json->propertyBoolean(WC_INDETERMINATE, ((sel > 0) && (sel < rows)));
    });
  }

  void _notifyClient(const char* type, int index, const char* htmlSnippet) {
    WValue vIndex(index);
    WebAppSockets::sendJson(WC_TABLE_UPDATE, id(), [type, &vIndex, htmlSnippet](WJson* json) {
      json->propertyString(WC_TYPE, type, nullptr);
      json->propertyValue(WC_INDEX, &vIndex);
      if (htmlSnippet != nullptr) json->propertyString(WC_HTML_SNIPPET, htmlSnippet, nullptr);
    });
  }

  void _sendCell(int index, int column, const char* value, bool refused) {
    WValue vRow(index);
    WValue vCol(column + (_selectable ? 1 : 0));
    WebAppSockets::sendJson(WC_CELL_UPDATE, id(), [&vRow, &vCol, value, refused](WJson* json) {
      json->propertyValue(WC_ROW, &vRow);
      json->propertyValue(WC_COL, &vCol);
      if (value != nullptr) {
        json->propertyString(WC_VALUE, value, nullptr);
      } else {
        json->propertyNull(WC_VALUE);
      }
      json->propertyBoolean(WC_REFUSED, refused);
    });
  }
};

class WebFieldset : public WebControl {
 public:
  WebFieldset(const char* legend) : WebControl(WC_FIELDSET, nullptr) {
    this->add((new WebControl(WC_LEGEND, nullptr))->content(legend));
  }

  virtual ~WebFieldset() {
  }
};

/** A list to choose one entry from, the label is the one of its row. */
class WebSelect : public WebControl {
 public:
  WebSelect(const char* id) : WebControl(WC_SELECT, WC_ID, id, WC_NAME, id, nullptr) {
  }

  WebSelect* option(const char* option, bool selected, const char* optionTitle = nullptr) {
    this->add((new WebControl(WC_OPTION,
                              WC_VALUE, option,
                              (selected ? WC_SELECTED : ""), (selected ? "" : nullptr), nullptr))
                  ->content(optionTitle != nullptr ? optionTitle : option));
    return this;
  }
};

#endif