#ifndef WEB_RESOURCES_H
#define WEB_RESOURCES_H

#include "../WList.h"
#include "Arduino.h"

const static char WC_ACCEPT[] PROGMEM = "accept";
const static char WC_ACTION[] PROGMEM = "action";
const static char WC_ALL[] PROGMEM = "all";
const static char WC_BACK_TO_MAINMENU[] PROGMEM = "Back to configuration";
const static char WC_BODY[] PROGMEM = R"=====(body)=====";
const static char WC_BUTTON[] PROGMEM = R"=====(button)=====";
const static char WC_CLASS[] PROGMEM = R"=====(class)=====";
const static char WC_CONTENT[] PROGMEM = R"=====(content)=====";
const static char WC_CONTENT_EDITABLE[] PROGMEM = "contenteditable"; 
const static char WC_CONFIG[] PROGMEM = "config";
const static char WC_CHECKBOX[] PROGMEM = "checkbox";
const static char WC_CHECKED[] PROGMEM = "checked";
const static char WC_CSS_BUTTON_HOVER[] PROGMEM = "button:hover";
const static char WC_CSS_FORM_WHITE_BOX[] PROGMEM = "form, .wb";
const static char WC_CSS_CHECK_BOX[] PROGMEM = ".cb input[type='checkbox']";
const static char WC_CSS_CHECK_BOX_LABEL[] PROGMEM = ".cb input[type='checkbox']+label";
const static char WC_CSS_CHECK_BOX_LABEL_BEFORE[] PROGMEM = ".cb input[type='checkbox']+label:before";
const static char WC_CSS_CHECK_BOX_CHECKED_LABEL_BEFORE[] PROGMEM = ".cb input[type='checkbox']:checked+label:before";
const static char WC_COL[] PROGMEM = "col";
const static char WC_COLS[] PROGMEM = "cols";
const static char WC_DIV[] PROGMEM = "div";
const static char WC_ENCTYPE[] PROGMEM = "enctype";
const static char WC_EVENT[] PROGMEM = "event";
const static char WC_DATA[] PROGMEM = "data";
const static char WC_DOCTYPE_HTML[] PROGMEM = "!DOCTYPE";
const static char WC_FIELDSET[] PROGMEM = "fieldset";
const static char WC_FILE[] PROGMEM = "file";
const static char WC_FIRMWARE[] PROGMEM = "firmware";
const static char WC_FOR[] PROGMEM = "for";
const static char WC_FORM[] PROGMEM = "form";
const static char WC_FUNCTION[] PROGMEM = "function";
const static char WC_GET[] PROGMEM = "get";
const static char WC_H1[] PROGMEM = "h1";
const static char WC_H2[] PROGMEM = "h2";
const static char WC_HEAD[] PROGMEM = "head";
const static char WC_HIDDEN[] PROGMEM = "hidden";
const static char WC_HISTORY_BACK[] PROGMEM = "history.back()";
const static char WC_HTML[] PROGMEM = "html";
const static char WC_HTML_SNIPPET[] PROGMEM = "htmlSnippet";
const static char WC_HTTP[] PROGMEM = "http";
const static char WC_HREF[] PROGMEM = "href";
const static char WC_INDETERMINATE[] PROGMEM = "indeterminate";
const static char WC_INDEX[] PROGMEM = "index";
const static char WC_INFO[] PROGMEM = "info";
const static char WC_INPUT[] PROGMEM = "input";
const static char WC_LABEL[] PROGMEM = "label";
const static char WC_LANG[] PROGMEM = "lang";
const static char WC_LEGEND[] PROGMEM = "legend";
const static char WC_LINK[] PROGMEM = "link";
const static char WC_LOCATION_HREF[] PROGMEM = "document.location='%s'";
const static char WC_MAXLENGTH[] PROGMEM = "maxlength";
const static char WC_META[] PROGMEM = "meta";
const static char WC_METHOD[] PROGMEM = "method";
const static char WC_MQTT_PASSWORD[] PROGMEM = "mqttpassword";
const static char WC_MQTT_PORT[] PROGMEM = "mqttport";
const static char WC_MQTT_SERVER[] PROGMEM = "mqttserver";
const static char WC_MQTT_USER[] PROGMEM = "mqttuser";
const static char WC_MULTIPART_FORM_DATA[] PROGMEM = "multipart/form-data";
const static char WC_NAME[] PROGMEM = "name";
const static char WC_ON_CHANGE[] PROGMEM = "onchange";
const static char WC_ON_CLICK[] PROGMEM = "onclick";
const static char WC_OPTION[] PROGMEM = "option";
const static char WC_PASSWORD[] PROGMEM = "password";
const static char WC_PING[] PROGMEM = "PING";
const static char WC_POST[] PROGMEM = "post";
const static char WC_REFUSED[] PROGMEM = "refused";
const static char WC_REL[] PROGMEM = "rel";
const static char WC_RESET[] PROGMEM = "reset";
const static char WC_ROW[] PROGMEM = "row";
const static char WC_ROWS[] PROGMEM = "rows";
const static char WC_SAVE_CONFIGURATION[] PROGMEM = "Save configuration";
const static char WC_SCRIPT[] PROGMEM = "script";
const static char WC_SELECT[] PROGMEM = "select";
const static char WC_SELECTED[] PROGMEM = "selected";
const static char WC_SPAN[] PROGMEM = "span";
const static char WC_SPELLCHECK[] PROGMEM = "spellcheck";
const static char WC_SSID[] PROGMEM = "ssid";
const static char WC_STATE[] PROGMEM = "state";
const static char WC_STYLE[] PROGMEM = "style";
const static char WC_SUBMIT[] PROGMEM = "submit";
const static char WC_TABLE[] PROGMEM = "table";
const static char WC_TABLE_BODY[] PROGMEM = "tbody";
const static char WC_TABLE_DATA[] PROGMEM = "td";
const static char WC_TABLE_HEAD[] PROGMEM = "thead";
const static char WC_TABLE_HEADER[] PROGMEM = "th";
const static char WC_TABLE_ROW[] PROGMEM = "tr";
const static char WC_TCP[] PROGMEM = "tcp";
const static char WC_TEXT[] PROGMEM = "text";
const static char WC_TEXTAREA[] PROGMEM = "textarea";
const static char WC_TEXT_HTML[] PROGMEM = "text/html";
const static char WC_URL[] PROGMEM = "url";
const static char WC_VALUE[] PROGMEM = "value";
const static char WC_WHITE_BOX[] PROGMEM = "wb";
const static char WC_WIDTH_100PERCENT[] PROGMEM = "width:100%";
const static char WC_WIFI[] PROGMEM = "wifi";
const static char WC_ICON_KAMSA[] PROGMEM = "data:image/svg+xml;base64,PHN2ZyB4bWxucz0naHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmcnIHZpZXdCb3g9JzAgMCAxMDAgMTAwJz48cGF0aCBkPSdNIDUwIDAgQSA1MCA1MCAwIDAgMCAwIDUwIEEgNTAgNTAgMCAwIDAgNTAgMTAwIEEgNTAgNTAgMCAwIDAgMTAwIDUwIEEgNTAgNTAgMCAwIDAgNTAgMCBBIDUwIDUwIDAgMCAwIDUwIDAgeiBNIDUwIDUgQSA0NSA0NSAwIDAgMSA1MCA1IEEgNDUgNDUgMCAwIDEgOTUgNTAgQSA0NSA0NSAwIDAgMSA1MCA5NSBBIDQ1IDQ1IDAgMCAxIDUgNTAgQSA0NSA0NSAwIDAgMSA1MCA1IHogTSA0NSw2IDQzLDM3IDY0LDU4IDQxLjUsNzAuNSB2IDAgbCA0Miw4IC0xOSwtMjAgMCwwIHogTSA0MywzNyAyMCw4MiA0MS4zLDcwLjcgNDMsMzcgWicgZmlsbD0nIzI0QjNBOCcgLz48L3N2Zz4=";
const static char WC_STYLE_BODY[] PROGMEM = "text-align:center; font-family:sans-serif; font-size:1.2rem; background-color: #474e5d; color: white;";    
const static char WC_STYLE_FORM_WHITE_BOX[] PROGMEM = "text-align:left; display: inline-block;	border-radius: 0.3rem; padding: 1rem; background-color: white; color: #404040;";      
const static char WC_STYLE_BUTTON[] PROGMEM = R"=====(
width:100%; 
border:none; 
color:white; 
font-size:1.2rem; 
padding:0.5rem 1.0rem; 
text-align:center; 
text-decoration:none; 
/*display:inline-block; */
margin:.4rem .2rem;
/*cursor:pointer; */
background-color:#04AA6D; 
border-radius:0.5rem;
)=====";      
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

const static char WC_SCRIPT_INITIALIZE_SOCKET[] PROGMEM = R"=====(
let form = window.location.href.substring(window.location.href.lastIndexOf('/') + 1);
var webSocket = new WebSocket("ws://" + location.hostname + ":81/");

webSocket.onopen = function() {
  console.log("WebSocket connected: " + form);
};

webSocket.onmessage = function(event) {
  var payload = event.data;
  console.log(payload);
  var json = JSON.parse(event.data);
  switch (json.event) {
      case "PING":
          sendWebSocketMessage("PING", null, null);
          break;
      default:
          executeFunctionByName(json.event, window, json);
  } 
}

function sendWebSocketMessage(event, id, data) {
    if (webSocket !== null) {
        var payload = {};
        payload["event"] = event;
        payload["form"] = form;
        if (id != null) payload["id"] = id;        
        if (data !== null) payload["data"] = data;
        webSocket.send(JSON.stringify(payload));
    }
}

function executeFunctionByName(functionName, context /*, args */) {
    var args = Array.prototype.slice.call(arguments, 2);
    var namespaces = functionName.split(".");
    var func = namespaces.pop();
    for (var i = 0; i < namespaces.length; i++) {
        context = context[namespaces[i]];
    }
    if (func in context) {
        return context[func].apply(context, args);
    } else {
        console.log("Unknown function: " + func)
    }
}
)=====";

const static char WC_SCRIPT_NAME_CONTROL_EVENT[] PROGMEM = "controlEvent(this, '%s')";

const static char WC_SCRIPT_CONTROL_EVENT[] PROGMEM = R"=====(
function controlEvent(elem, event) {
  sendWebSocketMessage(event, elem.id, {"value":elem.value});
}
)=====";

const static char WC_SCRIPT_NAME_TEXTAREA[] PROGMEM = "textAreaUpdate(json)";

const static char WC_SCRIPT_TEXTAREA[] PROGMEM = R"=====(
function textAreaUpdate(json) {
  var textArea = document.getElementById(json.id);
  if (textArea !== null) {
    textArea.innerHTML = json.data;
  }
}    
)=====";

//Table: the seven theme tokens (border, foreground, muted, zebra, highlight,
//accent, radius) are put on :root, so every rule below is a short reference
const static char WC_CSS_ROOT[] PROGMEM = ":root";
const static char WC_CSS_TABLE_ZEBRA[] PROGMEM = "tbody tr:nth-child(2n)";
const static char WC_CSS_TABLE_HOVER[] PROGMEM = "tbody tr:hover";
const static char WC_CSS_TABLE_LAST_ROW[] PROGMEM = "tbody tr:last-child td";
const static char WC_CSS_TABLE_CHECK_BOX[] PROGMEM = "table input[type=checkbox]";
const static char WC_CSS_TABLE_EDIT[] PROGMEM = "td[contenteditable]:focus";
const static char WC_CSS_TABLE_REFUSED[] PROGMEM = "td.refused";
const static char WC_STYLE_ROOT[] PROGMEM = "--tb:#e5e7eb;--tf:#111827;--tm:#6b7280;--tz:#f8f9fb;--th:#eef6f2;--ta:#04AA6D;--tr:8px";
const static char WC_STYLE_TABLE[] PROGMEM = "width:100%;border-collapse:separate;border-spacing:0;border:1px solid var(--tb);border-radius:var(--tr);overflow:hidden;font-size:.9rem;color:var(--tf);text-align:left";
const static char WC_STYLE_TABLE_DATA[] PROGMEM = "padding:.5rem .7rem;border-bottom:1px solid var(--tb)";
const static char WC_STYLE_TABLE_HEADER[] PROGMEM = "padding:.5rem .7rem;position:sticky;top:0;background:#fff;font-size:.72rem;font-weight:600;letter-spacing:.06em;text-transform:uppercase;color:var(--tm);border-bottom:2px solid var(--tb);white-space:nowrap";
const static char WC_STYLE_TABLE_ZEBRA[] PROGMEM = "background:var(--tz)";
const static char WC_STYLE_TABLE_HOVER[] PROGMEM = "background:var(--th)";
const static char WC_STYLE_TABLE_LAST_ROW[] PROGMEM = "border-bottom:0";
const static char WC_STYLE_TABLE_CHECK_BOX[] PROGMEM = "accent-color:var(--ta);width:1rem;height:1rem;vertical-align:middle;margin:0";
const static char WC_STYLE_TABLE_EDIT[] PROGMEM = "outline:2px solid var(--ta);outline-offset:-2px;background:#fff";
const static char WC_STYLE_TABLE_REFUSED[] PROGMEM = "outline:2px solid #dc2626;outline-offset:-2px";

const static char WC_SCRIPT_WEB_TABLE_NAME[] PROGMEM = "WebTable";
const static char WC_TABLE_UPDATE[] PROGMEM = "tableUpdate";
const static char WC_CELL_UPDATE[] PROGMEM = "cellUpdate";
const static char WC_SELECT_ROWS[] PROGMEM = "selectRows";
const static char WC_ADDED[] PROGMEM = "ADDED";
const static char WC_REMOVED[] PROGMEM = "REMOVED";

//A row is addressed by its index in the body and a cell by its index in the
//row, so no cell of the table needs an id of its own. Editing is done by the
//browser with contenteditable. Everything is listened to by delegation on the
//document, so a row arriving later over the socket needs no wiring and carries
//no event attribute.
const static char WC_SCRIPT_WEB_TABLE[] PROGMEM = R"=====(
function tableUpdate(json) {
  var d = json.data, b = document.getElementById(json.id).tBodies[0];
  if (d.type == "ADDED") {
    var t = document.createElement("template");
    t.innerHTML = d.htmlSnippet;
    if (d.index < b.rows.length) b.insertBefore(t.content.firstChild, b.rows[d.index]); else b.appendChild(t.content.firstChild);
  } else if (d.type == "REMOVED") {
    if (d.index < b.rows.length) b.deleteRow(d.index);
  }
}

function tableEvent(cell, event, value) {
  var row = cell.parentNode;
  sendWebSocketMessage(event, row.closest("table").id, {"row": (row.parentNode.tagName == "THEAD" ? -1 : row.sectionRowIndex), "col": cell.cellIndex, "value": value});
}

function cellUpdate(json) {
  var d = json.data, t = document.getElementById(json.id);
  if (t === null) return;
  var c = t.tBodies[0].rows[d.row].cells[d.col];
  c.textContent = (d.value != null ? d.value : (c.dataset.old != null ? c.dataset.old : ""));
  if (d.refused) {
    c.classList.add("refused");
    setTimeout(function() { c.classList.remove("refused"); }, 1500);
  }
}

function selectRows(json) {
  var d = json.data, t = document.getElementById(json.id);
  if (t === null) return;
  if (d.all != null) {
    var b = t.tBodies[0].querySelectorAll("input[type=checkbox]");
    for (var i = 0; i < b.length; i++) b[i].checked = d.all;
  }
  var h = (t.tHead != null ? t.tHead.querySelector("input[type=checkbox]") : null);
  if (h != null) {
    h.checked = d.checked;
    h.indeterminate = d.indeterminate;
  }
}

//The box of the first cell selects its row, a box printed by the page itself
//is left alone. Taken by change, so the keyboard toggles a row as well.
document.addEventListener("change", function(e) {
  var b = e.target, c = b.parentNode;
  if ((b.type == "checkbox") && (c.cellIndex === 0)) tableEvent(c, "onclick", b.checked);
});

document.addEventListener("focusin", function(e) {
  if (e.target.isContentEditable) e.target.dataset.old = e.target.textContent;
});

document.addEventListener("focusout", function(e) {
  var c = e.target;
  if ((c.isContentEditable) && (c.textContent != c.dataset.old)) tableEvent(c, "onchange", c.textContent);
});

document.addEventListener("keydown", function(e) {
  var c = e.target;
  if (c.isContentEditable) {
    if (e.key == "Enter") {
      e.preventDefault();
      c.blur();
    } else if (e.key == "Escape") {
      e.preventDefault();
      c.textContent = c.dataset.old;
      c.blur();
    }
  }
});
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

class WHtml {
 public:
  static void commandParamsAndNullptr(Print* stream, const char* tag, bool start, const char* params, ...) {    
    stream->print(WC_SMALLER);
    if (!start) stream->print(WC_SLASH);
    stream->print(FPSTR(tag));
    va_list arg;
    bool b = false;
    va_start(arg, params);
    while (params) {
      if (b) {
        stream->print(WC_EQUAL);
        stream->print(WC_QUOTE);
      } else {
        stream->print(WC_SPACE);
      }
      stream->print(FPSTR(params));
      if (b) {
        stream->print(WC_QUOTE);
      }
      b = !b;
      params = va_arg(arg, const char*);
    }
    va_end(arg);
    stream->print(WC_GREATER);
  }

  static void command(Print* stream, const char* tag, bool start) {
    command(stream, tag, start, nullptr);
  }

  static void command(Print* stream, const char* tag, bool start, WStringList* keyValues) {
    stream->print(WC_SMALLER);
    if (!start) stream->print(WC_SLASH);
    stream->print(FPSTR(tag));
    if (keyValues != nullptr) {
      keyValues->forEach([stream](int index, const char* value, const char* id) {

        stream->print(WC_SPACE);
        stream->print(id);               
        if (value) {
          stream->print(WC_EQUAL);
          stream->print(WC_QUOTE);
          stream->print(value);
          stream->print(WC_QUOTE);        
        }
      });
    }
    stream->print(WC_GREATER);
  }

  static void breakLine(Print* stream) {
    command(stream, PSTR("br"), true);
  }

  static void styleToString(Print* stream, const char* key, const char* value) {    
    stream->print(WC_SPACE);
    stream->print(key);
    stream->print(WC_SBEGIN);
    if (value) stream->print(value);
    stream->print(WC_SEND);
  }  

};

#endif