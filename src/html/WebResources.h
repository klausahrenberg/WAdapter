#ifndef WEB_RESOURCES_H
#define WEB_RESOURCES_H

#include "../WList.h"
#include "Arduino.h"

const static char WC_ACTION[] PROGMEM = "action";
const static char WC_BODY[] PROGMEM = R"=====(body)=====";
const static char WC_BUTTON[] PROGMEM = R"=====(button)=====";
const static char WC_CLASS[] PROGMEM = R"=====(class)=====";
const static char WC_CONTENT[] PROGMEM = R"=====(content)=====";
const static char WC_CONFIG[] PROGMEM = "config";
const static char WC_CSS_BUTTON_HOVER[] PROGMEM = "button:hover";
const static char WC_CSS_FORM_WHITE_BOX[] PROGMEM = "form, .wb";
const static char WC_CSS_CHECK_BOX[] PROGMEM = ".cb input[type='checkbox']";
const static char WC_CSS_CHECK_BOX_LABEL[] PROGMEM = ".cb input[type='checkbox']+label";
const static char WC_CSS_CHECK_BOX_LABEL_BEFORE[] PROGMEM = ".cb input[type='checkbox']+label:before";
const static char WC_CSS_CHECK_BOX_CHECKED_LABEL_BEFORE[] PROGMEM = ".cb input[type='checkbox']:checked+label:before";
const static char WC_DIV[] PROGMEM = "div";
const static char WC_DOCTYPE_HTML[] PROGMEM = "!DOCTYPE";
const static char WC_FOR[] PROGMEM = "for";
const static char WC_FORM[] PROGMEM = "form";
const static char WC_FUNCTION[] PROGMEM = "function";
const static char WC_GET[] PROGMEM = "get";
const static char WC_HEAD[] PROGMEM = R"=====(head)=====";
const static char WC_HIDDEN[] PROGMEM = "hidden";
const static char WC_HISTORY_BACK[] PROGMEM = "history.back()";
const static char WC_HTML[] PROGMEM = R"=====(html)=====";
const static char WC_HREF[] PROGMEM = R"=====(href)=====";
const static char WC_ID[] PROGMEM = "id";
const static char WC_INFO[] PROGMEM = "info";
const static char WC_INPUT[] PROGMEM = R"=====(input)=====";
const static char WC_LABEL[] PROGMEM = R"=====(label)=====";
const static char WC_LANG[] PROGMEM = R"=====(lang)=====";
const static char WC_LINK[] PROGMEM = R"=====(link)=====";
const static char WC_LOCATION_HREF[] PROGMEM = "document.location='%s'";
const static char WC_MAXLENGTH[] PROGMEM = "maxlength";
const static char WC_META[] PROGMEM = R"=====(meta)=====";
const static char WC_METHOD[] PROGMEM = "method";
const static char WC_MQTT_PASSWORD[] PROGMEM = "mqttpassword";
const static char WC_MQTT_PORT[] PROGMEM = "mqttport";
const static char WC_MQTT_SERVER[] PROGMEM = "mqttserver";
const static char WC_MQTT_USER[] PROGMEM = "mqttuser";
const static char WC_NAME[] PROGMEM = R"=====(name)=====";
const static char WC_ON_CHANGE[] PROGMEM = "onchange";
const static char WC_ON_CLICK[] PROGMEM = "onclick";
const static char WC_PASSWORD[] PROGMEM = "password";
const static char WC_POST[] PROGMEM = "post";
const static char WC_REL[] PROGMEM = "rel";
const static char WC_RESET[] PROGMEM = "reset";
const static char WC_SCRIPT[] PROGMEM = "script";
const static char WC_SPAN[] PROGMEM = "span";
const static char WC_SSID[] PROGMEM = "ssid";
const static char WC_STYLE[] PROGMEM = R"=====(style)=====";
const static char WC_SUBMIT[] PROGMEM = "submit";
const static char WC_TABLE[] PROGMEM = "table";
const static char WC_TABLE_DATA[] PROGMEM = "td";
const static char WC_TABLE_HEADER[] PROGMEM = "th";
const static char WC_TABLE_ROW[] PROGMEM = "tr";
const static char WC_TEXT[] PROGMEM = "text";
const static char WC_TEXT_HTML[] PROGMEM = R"=====(text/html)=====";
const static char WC_TYPE[] PROGMEM = "type";
const static char WC_TITLE[] PROGMEM = "title";
const static char WC_VALUE[] PROGMEM = "value";
const static char WC_WHITE_BOX[] PROGMEM = "wb";
const static char WC_WIFI[] PROGMEM = "wifi";
const static char WC_ICON_KAMSA[] PROGMEM = "data:image/svg+xml;base64,PHN2ZyB4bWxucz0naHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmcnIHZpZXdCb3g9JzAgMCAxMDAgMTAwJz48cGF0aCBkPSdNIDUwIDAgQSA1MCA1MCAwIDAgMCAwIDUwIEEgNTAgNTAgMCAwIDAgNTAgMTAwIEEgNTAgNTAgMCAwIDAgMTAwIDUwIEEgNTAgNTAgMCAwIDAgNTAgMCBBIDUwIDUwIDAgMCAwIDUwIDAgeiBNIDUwIDUgQSA0NSA0NSAwIDAgMSA1MCA1IEEgNDUgNDUgMCAwIDEgOTUgNTAgQSA0NSA0NSAwIDAgMSA1MCA5NSBBIDQ1IDQ1IDAgMCAxIDUgNTAgQSA0NSA0NSAwIDAgMSA1MCA1IHogTSA0NSw2IDQzLDM3IDY0LDU4IDQxLjUsNzAuNSB2IDAgbCA0Miw4IC0xOSwtMjAgMCwwIHogTSA0MywzNyAyMCw4MiA0MS4zLDcwLjcgNDMsMzcgWicgZmlsbD0nIzI0QjNBOCcgLz48L3N2Zz4=";
const static char WC_STYLE_BODY[] PROGMEM = "text-align: center; font-family: sans-serif; font-size: 1.2rem; background-color: #474e5d; color: white;";    
const static char WC_STYLE_FORM_WHITE_BOX[] PROGMEM = "display: inline-block;	border-radius: 0.3rem; padding: 1rem; background-color: white; color: #404040;";      
const static char WC_STYLE_BUTTON[] PROGMEM = "width:100%; border:none; color:white; font-size:1.2rem; padding:0.5rem 1.0rem; text-align:center; text-decoration:none; display:inline-block; margin:4px 2px; cursor:pointer; background-color:#04AA6D; border-radius:0.5rem;";      
const static char WC_STYLE_BUTTON_HOVER[] PROGMEM = "background-color:blueviolet;";
const static char WC_STYLE_CHECK_BOX[] PROGMEM = R"=====(
display:none;
)=====";
const static char WC_STYLE_CHECK_BOX_LABEL[] PROGMEM = R"=====(
display:inline-block;
)=====";
const static char WC_STYLE_CHECK_BOX_LABEL_BEFORE[] PROGMEM = R"=====(
content:'';
display:inline-block;
width:1.6rem;
height:1.6rem;
margin-right:1.6rem;
border-radius:.3rem;
border:.1rem solid currentColor;
transition: all .12s, border-color .1s;
)=====";
const static char WC_STYLE_CHECK_BOX_CHECKED_LABEL_BEFORE[] PROGMEM = R"=====(
width:.8rem;
margin-left:.8rem;
border-radius:0;
border-top-color:transparent;
border-left-color:transparent;
transform: rotate(45deg) translateX(-.4rem);
)=====";

//Switch
const static char WC_CSS_SWITCH[] PROGMEM = ".switch";
const static char WC_CSS_SWITCH_INPUT[] PROGMEM = ".switch input";
const static char WC_CSS_SLIDER[] PROGMEM = ".slider";
const static char WC_CSS_SLIDER_BEFORE[] PROGMEM = ".slider:before";
const static char WC_CSS_INPUT_CHECKED_SLIDER[] PROGMEM = "input:checked+.slider";
const static char WC_CSS_INPUT_CHECKED_SLIDER_BEFORE[] PROGMEM = "input:checked+.slider:before";
const static char WC_STYLE_SWITCH[] PROGMEM = R"=====(
  position:relative; 
	display:inline-block; 
	width:3.7rem; 
	height:2rem;
)=====";

const static char WC_STYLE_SWITCH_INPUT[] PROGMEM = R"=====(
  display:none;
)=====";

const static char WC_STYLE_SLIDER[] PROGMEM = R"=====(
  position:absolute; 
	top:0; 
	left:0; 
	right:0; 
	bottom:0; 
	background-color:#ccc; 
	border-radius:1rem;
)=====";

const static char WC_STYLE_SLIDER_BEFORE[] PROGMEM = R"=====(
  position:absolute; 
	content:''; 
	height:1.6rem; 
	width:1.6rem; 
	left:.2rem; 
	bottom:.2rem; 
	background-color:#fff; 
	transition:.4s; 
	border-radius:1.2rem;
)=====";

const static char WC_STYLE_INPUT_CHECKED_SLIDER[] PROGMEM = R"=====(
	background-color: #2196F3
)=====";

const static char WC_STYLE_INPUT_CHECKED_SLIDER_BEFORE[] PROGMEM = R"=====(	
	transform: translateX(1.7rem)
)=====";

/*
https://stackoverflow.com/questions/4388102/can-you-style-an-active-form-inputs-label-with-just-css

.form-field {
  display: grid;
  gap: 4px;
}

.form-field label {
  grid-row: 1;
  font-size: 12px;
  color: #737373;
}

.form-field input {
  outline: unset;
  border-radius: 6px;
  padding: 6px 10px;
  font-size: 14px;
  border: 1px solid #737373;
}

.form-field input:focus {
  border-color: #328dd2;
}

.form-field input:focus + label {
  color: #328dd2;
}

<div class="form-field">
  <input id="myinput" />
  <label for="myinput">
    My Input
  </label>
</div>
*/

class WKeyValue {
 public:
  WKeyValue(const char* key, const char* value) {
    _key = new char[strlen_P(key) + 1];
    strcpy_P(_key, key);
    if (value != nullptr) {
      _value = new char[strlen_P(value) + 1];
      strcpy_P(_value, value);
    }   
  }

  virtual ~WKeyValue() {
    if (_key) delete _key;
    if (_value) delete _value;
  }
  const char* key() { return _key; }
  const char* value() { return _value; }

 protected:
  char* _key;
  char* _value = nullptr;
};

class WKeyValues {
 public:
  WKeyValues() {
    _values = new WList<WKeyValue>();
  }
  virtual ~WKeyValues() {
    _values->clear();
    delete _values;
  }

  void add(const char* key, const char* value) {
    WKeyValue* en = _values->getIf([key](WKeyValue* en) {
      return (strcmp_P(en->key(), key) == 0);
    });
    if (en == nullptr) {
      _values->add(new WKeyValue(key, value));      
    }
  }

  int size() { return _values->size(); }

  bool empty() { return _values->empty(); }

  typedef std::function<void(WKeyValue* value, const char* id)> TOnValue;
  void forEach(TOnValue consumer) {
    _values->forEach(consumer);
  }  

 protected:
  WList<WKeyValue>* _values;
};

class WHtml {
 public:
  static void commandParamsAndNullptr(Print* stream, const char* tag, bool start, const char* params, ...) {    
    stream->print(WC_BASE[2]);
    if (!start) stream->print(WC_BASE[4]);
    stream->print(FPSTR(tag));
    va_list arg;
    bool b = false;
    va_start(arg, params);
    while (params) {
      if (b) {
        stream->print(WC_BASE[1]);
        stream->print(WC_BASE[5]);
      } else {
        stream->print(WC_BASE[0]);
      }
      stream->print(FPSTR(params));
      if (b) {
        stream->print(WC_BASE[5]);
      }
      b = !b;
      params = va_arg(arg, const char*);
    }
    va_end(arg);
    stream->print(WC_BASE[3]);
  }

  static void command(Print* stream, const char* tag, bool start) {
    command(stream, tag, start, nullptr);
  }

  static void command(Print* stream, const char* tag, bool start, WList<WKeyValue>* keyValues) {
    stream->print(WC_BASE[2]);
    if (!start) stream->print(WC_BASE[4]);
    stream->print(FPSTR(tag));
    if (keyValues != nullptr) {
      keyValues->forEach([stream](WKeyValue* kv, const char* id) {
        stream->print(WC_BASE[0]);
        stream->print(kv->key());
        if (kv->value()) {          
          stream->print(WC_BASE[1]);
          stream->print(WC_BASE[5]);
          stream->print(kv->value());
          stream->print(WC_BASE[5]);
        }
      });
    }
    stream->print(WC_BASE[3]);
  }

  static void styleToString(Print* stream, const char* key, const char* value) {
    stream->print(WC_BASE[0]);
    stream->print(key);
    stream->print(WC_BASE[6]);
    stream->print(value);
    stream->print(WC_BASE[7]);
  }  

  static void scriptToString(Print* stream, const char* key, const char* value) {
    stream->print(WC_BASE[0]);
    stream->print(FPSTR(WC_FUNCTION));
    stream->print(WC_BASE[0]);
    stream->print(key);
    stream->print(WC_BASE[8]);
    stream->print("elem");
    stream->print(WC_BASE[9]);
    stream->print(WC_BASE[6]);
    stream->print(value);
    stream->print(WC_BASE[7]);
  }
};

#endif