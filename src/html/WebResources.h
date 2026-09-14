#ifndef WEB_RESOURCES_H
#define WEB_RESOURCES_H

#include "WebCSS.h"

const static char WC_ACCEPT[] PROGMEM = "accept";
const static char WC_ACTION[] PROGMEM = "action";
const static char WC_ALL[] PROGMEM = "all";
const static char WC_BACK_TO_MAINMENU[] PROGMEM = "Back to configuration";
const static char WC_BAR[] PROGMEM = "bar";
const static char WC_BODY[] PROGMEM = R"=====(body)=====";
const static char WC_BUTTON[] PROGMEM = R"=====(button)=====";
const static char WC_CLASS[] PROGMEM = R"=====(class)=====";
const static char WC_CONTENT[] PROGMEM = R"=====(content)=====";
const static char WC_CONTENT_EDITABLE[] PROGMEM = "contenteditable"; 
const static char WC_CHARSET[] PROGMEM = "charset";
const static char WC_CHECKBOX[] PROGMEM = "checkbox";
const static char WC_CHECKED[] PROGMEM = "checked";
const static char WC_CSS_CHECK_BOX[] PROGMEM = ".cb input[type='checkbox']";
const static char WC_CSS_CHECK_BOX_LABEL[] PROGMEM = ".cb input[type='checkbox']+label";
const static char WC_CSS_CHECK_BOX_LABEL_BEFORE[] PROGMEM = ".cb input[type='checkbox']+label:before";
const static char WC_CSS_CHECK_BOX_CHECKED_LABEL_BEFORE[] PROGMEM = ".cb input[type='checkbox']:checked+label:before";
const static char WC_COL[] PROGMEM = "col";
const static char WC_COLS[] PROGMEM = "cols";
const static char WC_DIV[] PROGMEM = "div";
const static char WC_DISPLAY_NONE[] PROGMEM = "display:none;";
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
const static char WC_H3[] PROGMEM = "h3";
const static char WC_HEAD[] PROGMEM = "head";
const static char WC_HIDDEN[] PROGMEM = "hidden";
const static char WC_HISTORY_BACK[] PROGMEM = "history.back()";
const static char WC_HTML[] PROGMEM = "html";
const static char WC_HTML_SNIPPET[] PROGMEM = "htmlSnippet";
//no PROGMEM: mDNS reads it byte wise
const static char WC_HTTP[] = "http";
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
const static char WC_MAX[] PROGMEM = "max";
const static char WC_MAXLENGTH[] PROGMEM = "maxlength";
const static char WC_META[] PROGMEM = "meta";
const static char WC_METHOD[] PROGMEM = "method";
const static char WC_MQTT_PASSWORD[] PROGMEM = "mqttpassword";
const static char WC_MQTT_PORT[] PROGMEM = "mqttport";
const static char WC_MQTT_SERVER[] PROGMEM = "mqttserver";
const static char WC_MQTT_USER[] PROGMEM = "mqttuser";
const static char WC_MULTIPART_FORM_DATA[] PROGMEM = "multipart/form-data";
const static char WC_NAME[] PROGMEM = "name";
const static char WC_NAV[] PROGMEM = "nav";
const static char WC_ON_CHANGE[] PROGMEM = "onchange";
const static char WC_ON_CLICK[] PROGMEM = "onclick";
const static char WC_OPTION[] PROGMEM = "option";
const static char WC_PASSWORD[] PROGMEM = "password";
const static char WC_PING[] PROGMEM = "PING";
const static char WC_POST[] PROGMEM = "post";
const static char WC_PROGRESS[] PROGMEM = "progress";
const static char WC_REFUSED[] PROGMEM = "refused";
const static char WC_REL[] PROGMEM = "rel";
const static char WC_RESET[] PROGMEM = "reset";
const static char WC_ROW[] PROGMEM = "row";
const static char WC_ROWS[] PROGMEM = "rows";
const static char WC_SAVE_CONFIGURATION[] PROGMEM = "Save configuration";
const static char WC_SCRIPT[] PROGMEM = "script";
const static char WC_SECTION[] PROGMEM = "section";
const static char WC_SELECT[] PROGMEM = "select";
const static char WC_SELECTED[] PROGMEM = "selected";
const static char WC_SHELL[] PROGMEM = "shell";
const static char WC_SIDE[] PROGMEM = "side";
const static char WC_SPAN[] PROGMEM = "span";
const static char WC_SPELLCHECK[] PROGMEM = "spellcheck";
const static char WC_SSID[] PROGMEM = "ssid";
const static char WC_STATE[] PROGMEM = "state";
const static char WC_STYLE[] PROGMEM = "style";
const static char WC_SUBMIT[] PROGMEM = "submit";

const static char WC_SWITCH_ID[] = ".sw";
const static char WC_SWITCH_STYLE[] PROGMEM = R"=====(
  position: relative;
  border: 0;
  flex: 0 0 auto;
  width: 2.9rem;
  height: 1.6rem;
  margin: 0;
  padding: 0;
  border-radius: 1rem;
  background: #5a6472;
  transition: background .18s;
)=====";
const static char WC_SWITCH_ID_CHECKED[] = ".sw[aria-checked=true]";
const static char WC_SWITCH_STYLE_CHECKED[] PROGMEM = R"=====(
  background: #24b3a8;
)=====";
const static char WC_SWITCH_ID_I[] = ".sw i";
const static char WC_SWITCH_STYLE_I[] PROGMEM = R"=====(
  position: absolute;
  top: .2rem;
  left: .2rem;
  width: 1.2rem;
  height: 1.2rem;
  border-radius: 50%;
  background: #fff;
  transition: transform .18s;
)=====";
const static char WC_SWITCH_ID_CHECKED_I[] = ".sw[aria-checked=true] i";
const static char WC_SWITCH_STYLE_CHECKED_I[] PROGMEM = R"=====(
  transform: translateX(1.3rem);
)=====";

const static char WC_TABLE[] PROGMEM = "table";
const static char WC_TABLE_BODY[] PROGMEM = "tbody";
const static char WC_TABLE_DATA[] PROGMEM = "td";
const static char WC_TABLE_HEAD[] PROGMEM = "thead";
const static char WC_TABLE_HEADER[] PROGMEM = "th";
const static char WC_TABLE_ROW[] PROGMEM = "tr";
//no PROGMEM: mDNS reads it byte wise
const static char WC_TCP[] = "tcp";
const static char WC_TEXT[] PROGMEM = "text";
const static char WC_TEXTAREA[] PROGMEM = "textarea";
//no PROGMEM: the web server reads the content type byte wise
const static char WC_TEXT_HTML[] = "text/html";
const static char WC_UI[] PROGMEM = "ui";
//no PROGMEM: mDNS reads it byte wise
const static char WC_URL[] = "url";
const static char WC_VALUE[] PROGMEM = "value";
const static char WC_WHITE_BOX[] PROGMEM = "wb";
const static char WC_WIDTH_100PERCENT[] PROGMEM = "width:100%";
const static char WC_WIFI[] PROGMEM = "wifi";
const static char WC_ICON_KAMSA[] PROGMEM = "data:image/svg+xml;base64,PHN2ZyB4bWxucz0naHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmcnIHZpZXdCb3g9JzAgMCAxMDAgMTAwJz48cGF0aCBkPSdNIDUwIDAgQSA1MCA1MCAwIDAgMCAwIDUwIEEgNTAgNTAgMCAwIDAgNTAgMTAwIEEgNTAgNTAgMCAwIDAgMTAwIDUwIEEgNTAgNTAgMCAwIDAgNTAgMCBBIDUwIDUwIDAgMCAwIDUwIDAgeiBNIDUwIDUgQSA0NSA0NSAwIDAgMSA1MCA1IEEgNDUgNDUgMCAwIDEgOTUgNTAgQSA0NSA0NSAwIDAgMSA1MCA5NSBBIDQ1IDQ1IDAgMCAxIDUgNTAgQSA0NSA0NSAwIDAgMSA1MCA1IHogTSA0NSw2IDQzLDM3IDY0LDU4IDQxLjUsNzAuNSB2IDAgbCA0Miw4IC0xOSwtMjAgMCwwIHogTSA0MywzNyAyMCw4MiA0MS4zLDcwLjcgNDMsMzcgWicgZmlsbD0nIzI0QjNBOCcgLz48L3N2Zz4=";
//The card of the control panel: a box with a heading and a row per value
const static char WC_CLASS_CARD_TYPE[] PROGMEM = "type";

const static char WC_CLASS_VALUE[] PROGMEM = "val";
const static char WC_CLASS_MESSAGE[] PROGMEM = "msg";
const static char WC_CLASS_BOX[] PROGMEM = "box";
//An action that cannot be taken back does not look like the others
const static char WC_DANGER[] PROGMEM = "danger";
const static char WC_CSS_BUTTON_DANGER[] PROGMEM = "button.danger";
const static char WC_CSS_BUTTON_DANGER_HOVER[] PROGMEM = "button.danger:hover";
const static char WC_STYLE_BUTTON_DANGER[] PROGMEM = "background:#e2564a;color:#fff";
const static char WC_STYLE_BUTTON_DANGER_HOVER[] PROGMEM = "background:#ec6a5f";
//a form that holds cards is the frame for the submit only, the boxes are theirs
const static char WC_STYLE_FORM_PLAIN[] PROGMEM = "padding:0;background:0 0";




//the browser draws the button of a file input on its own, so it needs the look
//of the other buttons by a rule of its own
const static char WC_CSS_FILE[] PROGMEM = "input[type=file]";
const static char WC_CSS_FILE_BUTTON[] PROGMEM = "input[type=file]::file-selector-button";
const static char WC_CSS_FILE_BUTTON_HOVER[] PROGMEM = "input[type=file]::file-selector-button:hover";
const static char WC_STYLE_FILE[] PROGMEM = "color:val(--df);";
const static char WC_STYLE_FILE_BUTTON[] PROGMEM = "margin-right:.7rem;padding:.45rem 1rem;border:0;border-radius:.45rem;background:#24b3a8;color:#08211f;font:inherit;font-size:.9rem;font-weight:600;cursor:pointer";
const static char WC_STYLE_FILE_BUTTON_HOVER[] PROGMEM = "background:#2ecbbe";
//an unstyled progress bar renders with the browser's own white track, so it
//needs the same accent color and dark track as the rest of the page
const static char WC_CSS_PROGRESS[] PROGMEM = "progress";
const static char WC_CSS_PROGRESS_BAR[] PROGMEM = "progress::-webkit-progress-bar";
const static char WC_CSS_PROGRESS_VALUE[] PROGMEM = "progress::-webkit-progress-value";
const static char WC_STYLE_PROGRESS[] PROGMEM = "display:block;width:100%;height:.4rem;margin-top:.6rem;border:0;border-radius:.3rem;background:#2f3540;accent-color:#24b3a8";
const static char WC_STYLE_PROGRESS_BAR[] PROGMEM = "background:#2f3540;border-radius:.3rem";
const static char WC_STYLE_PROGRESS_VALUE[] PROGMEM = "background:#24b3a8;border-radius:.3rem";
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
	background-color:#5a6472; 
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
	background-color:#24b3a8
)=====";

const static char WC_STYLE_INPUT_CHECKED_SLIDER_BEFORE[] PROGMEM = R"=====(	
	transform: translateX(1.7rem)
)=====";

//The frame every page is drawn into: a bar on top, the main menu at the side
//and the controls of the page in the middle. Everything a single page cannot
//bring along is in here, the rules of the single controls are added by the
//controls themselves.
const static char WC_STYLE_SHELL[] PROGMEM = R"=====(
*

.bar 

.brand {
  font-weight: 600;
  letter-spacing: .02em;
}

.sub {
  color: #8f99a7;
  font-size: .75rem;
}

.dot {
  align-self: center;
  width: .55rem;
  height: .55rem;
  margin-left: auto;
  border-radius: 50%;
  background: #24b3a8;
  transition: background .2s;
}

.dot.off {
  background: #e2564a;
}

.burger

.shell

.side {
  
}

.side a {
  display: block;
  padding: .5rem .8rem;
  border-radius: .45rem;
  color: #c8cfd8;
  font-size: .88rem;
  text-decoration: none;
}

.side a:hover {
  background: #3a414d;
  color: #fff;
}

.side a.on {
  background: #24b3a8;
  color: #08211f;
  font-weight: 600;
}

main {
  flex: 1;
  min-width: 0;
  padding: 1.1rem 1.1rem 2.5rem 0;
}

h2 {
  margin: 0 0 .8rem;
  font-size: 1.05rem;
  font-weight: 600;
}

button {
  border: 0;
  font: inherit;
  cursor: pointer;
}

legend {
  padding: 0;
  color: #98a2b0;
  font-size: .75rem;
  letter-spacing: .06em;
  text-transform: uppercase;
}

fieldset {
  margin: 0 0 1rem;
  padding: 0 0 .3rem;
  border: 0;
  border-bottom: 1px solid #4b5361;
}

label {
  margin: .9rem 0 .3rem;
  color: #c8cfd8;
  font-size: .82rem;
}

input[type=text],
input[type=password],
input[type=number],
select,
textarea {
  width: 100%;
  max-width: 24rem;
  padding: .45rem .6rem;
  border: 1px solid #545c69;
  border-radius: .4rem;
  background: #2f3540;
  color: #eef1f5;
  font: inherit;
  font-size: .9rem;
}

input:focus,
select:focus,
textarea:focus {
  border-color: #24b3a8;
  outline: 0;
}

textarea {
  max-width: 100%;
  font-family: ui-monospace, Menlo, Consolas, monospace;
  font-size: .8rem;
}

.msg {
  padding: 2rem 1rem;
  color: #98a2b0;
  font-size: .9rem;
  text-align: center;
}

.thing {
  margin-bottom: 1.1rem;
  background: #3a414d;
  border-radius: .7rem;
  overflow: hidden;
}

.thing > h3 {
  display: flex;
  align-items: baseline;
  gap: .6rem;
  margin: 0;
  padding: .85rem 1.1rem;
  border-bottom: 1px solid #464e5b;
  font-size: 1rem;
  font-weight: 600;
}

.type {
  font-size: .7rem;
  font-weight: 400;
  letter-spacing: .07em;
  text-transform: uppercase;
  color: #8f99a7;
}

.prop {
  display: flex;
  align-items: center;
  gap: 1rem;
  padding: .65rem 1.1rem;
  border-bottom: 1px solid #464e5b;
}

.prop:last-child {
  border-bottom: 0;
}

.prop.ro .lbl {
  color: #98a2b0;
}

.lbl {
  flex: 0 0 10rem;
  margin: 0;
  font-size: .9rem;
  color: #c8cfd8;
}

.ctl {
  display: flex;
  flex: 1;
  min-width: 0;
  align-items: center;
  justify-content: flex-end;
  gap: .7rem;
}

.val {
  min-width: 2.6rem;
  font-size: .9rem;
  font-variant-numeric: tabular-nums;
  text-align: right;
}

.pill {
  padding: .18rem .6rem;
  border-radius: 1rem;
  background: #2f3540;
  color: #98a2b0;
  font-size: .78rem;
}

.pill.on {
  background: #24b3a8;
  color: #08211f;
}

.ctl input[type=range] {
  flex: 1;
  max-width: 15rem;
  height: .3rem;
  padding: 0;
  border: 0;
  border-radius: .3rem;
  background: #5a6472;
  outline: 0;
  -webkit-appearance: none;
  appearance: none;
}

.ctl input[type=range]::-webkit-slider-thumb {
  width: 1.05rem;
  height: 1.05rem;
  border-radius: 50%;
  background: #24b3a8;
  cursor: pointer;
  -webkit-appearance: none;
}

.ctl input[type=range]::-moz-range-thumb {
  width: 1.05rem;
  height: 1.05rem;
  border: 0;
  border-radius: 50%;
  background: #24b3a8;
  cursor: pointer;
}

.ctl input[type=text],
.ctl input[type=password],
.ctl input[type=number] {
  max-width: none;
}

.ctl select {
  width: auto;
  max-width: 14rem;
}

.ctl button {
  margin: 0;
}

.seg {
  display: flex;
  gap: .25rem;
  padding: .2rem;
  border-radius: .45rem;
  background: #2f3540;
}

.seg button {
  margin: 0;
  padding: .3rem .7rem;
  border-radius: .35rem;
  background: 0 0;
  color: #c8cfd8;
  font-weight: 400;
  font-size: .85rem;
}

.seg button[aria-pressed=true] {
  background: #24b3a8;
  color: #08211f;
}

.box {
  padding: .85rem 1.1rem;
}

.meta {
  padding: .55rem 1.1rem;
  background: #343b46;
  color: #8a94a2;
  font-size: .75rem;
}

@media (max-width: 44rem) {
 
}
)=====";

/*const static char WC_HTML_BAR[] PROGMEM = R"=====(
<div class="bar">
  <button class="burger" onclick="document.body.classList.toggle('nav')">&#9776;</button>
  <span class="brand">
)=====";
const static char WC_HTML_BAR_SUB[] PROGMEM = R"=====(
</span>
<span class="sub">
)=====";
const static char WC_HTML_NAV[] PROGMEM = R"=====(</span><span class="dot" id="dot"></span></div><div class="shell"><nav class="side">)=====";
const static char WC_HTML_MAIN[] PROGMEM = R"=====(</nav><main>)=====";
const static char WC_HTML_SHELL_END[] PROGMEM = R"=====(</main></div>)=====";
const static char WC_HTML_LINK[] PROGMEM = R"=====(<a href="/)=====";
const static char WC_HTML_LINK_ON[] PROGMEM = R"=====(" class="on">)=====";
const static char WC_HTML_LINK_OFF[] PROGMEM = R"=====(">)=====";
const static char WC_HTML_LINK_END[] PROGMEM = R"=====(</a>)=====";*/

//The id of the page is printed in front of this script, so the events find
//their way back to the page they were fired on, whatever url it was opened
//with. The dot in the bar tells whether the socket is still there.
const static char WC_SCRIPT_FORM_ID[] PROGMEM = R"=====(let form = ")=====";
const static char WC_SCRIPT_FORM_ID_END[] PROGMEM = R"=====(";
)=====";

const static char WC_SCRIPT_INITIALIZE_SOCKET[] PROGMEM = R"=====(
var webSocket = new WebSocket("ws://" + location.hostname + ":81/");

function online(connected) {
  var dot = document.getElementById("dot");
  if (dot) dot.className = (connected ? "dot" : "dot off");
}

webSocket.onopen = function() {
  console.log("WebSocket connected: " + form);
  online(true);
  //a page that has something to catch up on says so here
  if (typeof onSocketOpen === "function") onSocketOpen();
};

webSocket.onclose = function() { online(false); };

webSocket.onerror = function() { online(false); };

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
  if (typeof sendWebSocketMessage === "function") sendWebSocketMessage(event, elem.id, {"value":elem.value});
}
)=====";

//dedup id only, never printed - the script itself finds its inputs by tag
const static char WC_SCRIPT_NAME_FILE_UPLOAD[] PROGMEM = "fileUpload";

//uploads a form with a file input via xhr instead of a plain submit, so the
//upload gets a progress bar - the server side is untouched by this, it still
//sees the very same multipart post either way
const static char WC_SCRIPT_FILE_UPLOAD[] PROGMEM = R"=====(
function fileUpload(e) {
  e.preventDefault();
  var form = e.target, bar = form.querySelector('progress');
  var xhr = new XMLHttpRequest();
  xhr.upload.onprogress = function(ev) {
    if (ev.lengthComputable) bar.value = ev.loaded / ev.total * 100;
  };
  xhr.upload.onload = function() { bar.removeAttribute('value'); };
  xhr.onload = function() { document.write(xhr.responseText); };
  xhr.open('POST', '/events');
  xhr.send(new FormData(form));
}
document.querySelectorAll('input[type=file]').forEach(function(input) {
  input.form.addEventListener('submit', fileUpload);
});
)=====";

const static char WC_SCRIPT_NAME_TEXTAREA[] PROGMEM = "textAreaUpdate(json)";

//A textarea is the code editor of this app: monospace, real tab stops and no
//line wrapping, so a structure stays readable where it is
const static char WC_STYLE_TEXTAREA[] PROGMEM = "width:100%;box-sizing:border-box;font-family:ui-monospace,Menlo,Consolas,monospace;font-size:.82rem;line-height:1.35;tab-size:2;white-space:pre;overflow:auto;resize:vertical";

const static char WC_SCRIPT_TEXTAREA[] PROGMEM = R"=====(
function textAreaUpdate(json) {
  var textArea = document.getElementById(json.id);
  if (textArea !== null) {
    textArea.innerHTML = json.data;
  }
}
document.querySelectorAll('textarea').forEach(function(t) {
  t.addEventListener('keydown', function(e) {
    //tab indents, it does not jump out of the field
    if (e.key != 'Tab') return;
    e.preventDefault();
    var s = t.selectionStart, v = t.value;
    t.value = v.slice(0, s) + '  ' + v.slice(t.selectionEnd);
    t.selectionStart = t.selectionEnd = s + 2;
  });
  if (t.dataset.json == undefined) return;
  //json arrives in one line, the browser is the one that lays it out and checks it
  var check = function() {
    var m = '';
    if (t.value.trim() != '') {
      try { JSON.parse(t.value); } catch (x) { m = x.message; }
    }
    t.setCustomValidity(m);
    t.style.outline = (m == '' ? '' : '2px solid #e05252');
  };
  try { t.value = JSON.stringify(JSON.parse(t.value), null, 1); } catch (x) {}
  //what goes back to the device is packed together again, the upload stays as small as it was
  if (t.form) t.form.addEventListener('submit', function() {
    try { t.value = JSON.stringify(JSON.parse(t.value)); } catch (x) {}
  });
  t.addEventListener('input', check);
  check();
});
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
const static char WC_STYLE_ROOT[] PROGMEM = "--tb:#4b5361;--tf:#eef1f5;--tm:#98a2b0;--tz:#39404b;--th:#434b58;--ta:#24b3a8;--tr:.5rem";
const static char WC_STYLE_TABLE[] PROGMEM = "width:100%;border-collapse:separate;border-spacing:0;border:1px solid var(--tb);border-radius:var(--tr);overflow:hidden;font-size:.9rem;color:var(--tf);text-align:left";
const static char WC_STYLE_TABLE_DATA[] PROGMEM = "padding:.5rem .7rem;border-bottom:1px solid var(--tb);overflow-wrap:anywhere";
const static char WC_STYLE_TABLE_HEADER[] PROGMEM = "padding:.5rem .7rem;position:sticky;top:0;background:#3a414d;font-size:.72rem;font-weight:600;letter-spacing:.06em;text-transform:uppercase;color:var(--tm);border-bottom:2px solid var(--tb);white-space:nowrap";
const static char WC_STYLE_TABLE_ZEBRA[] PROGMEM = "background:var(--tz)";
const static char WC_STYLE_TABLE_HOVER[] PROGMEM = "background:var(--th)";
const static char WC_STYLE_TABLE_LAST_ROW[] PROGMEM = "border-bottom:0";
const static char WC_STYLE_TABLE_CHECK_BOX[] PROGMEM = "accent-color:var(--ta);width:1rem;height:1rem;vertical-align:middle;margin:0";
const static char WC_STYLE_TABLE_EDIT[] PROGMEM = "outline:2px solid var(--ta);outline-offset:-2px;background:#2f3540";
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