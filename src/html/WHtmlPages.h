#ifndef WEB_THING_HTML_PAGES_H
#define WEB_THING_HTML_PAGES_H

#include "Arduino.h"

const char* HTTP_SELECTED = "selected";
const char* HTTP_CHECKED = "checked";
const char* HTTP_NONE = "none";
const char* HTTP_BLOCK = "block";
const char* HTTP_TRUE = "true";
const char* HTTP_FALSE = "false";
const char* VALUE_GET = "get";

/*const static __FlashStringHelper* TEXT_HTML = F("text/html");
const static __FlashStringHelper* b_O = F("<");
const static __FlashStringHelper* b_C = F(">");
const static __FlashStringHelper* b_S = F("/");
const static __FlashStringHelper* b_N = F("");
const static __FlashStringHelper* b_B = F("\n");
const static __FlashStringHelper* b_Q = F("'");
const static __FlashStringHelper* b_div = F("div");
const static __FlashStringHelper* b_table = F("table");
const static __FlashStringHelper* b_tr = F("tr");
const static __FlashStringHelper* b_th = F("th");
const static __FlashStringHelper* b_td = F("td");
const static char b_colspan[] PROGMEM = R"=====( colspan='%d')=====";
const static char b_class[] PROGMEM = R"=====( class='%s')=====";
const static char b_style[] PROGMEM = R"=====( style='%s')=====";*/

/*static void HTTP_COMMAND(Print* stream, const __FlashStringHelper* c, bool end, const char* key1 = nullptr, const char* value1 = nullptr, const char* key2 = nullptr, const char* value2 = nullptr) {
  stream->print(b_O);
  stream->print(end ? b_S : b_N);
  stream->print(c);
	if (key1 != nullptr) stream->printf(key1, value1);
	if (key2 != nullptr) stream->printf(key2, value2);
  stream->print(b_C);
}

const static void HTTP_CONFIG_PAGE_BEGIN(Print* stream, const char* pageName) {
  stream->print(b_O);
  stream->printf(PSTR("form method='get' class='mc' action='submit%s'"), pageName);
  stream->print(b_C);
}*/
/*
static void HTTP_DIV_ID(Print* stream, const char* id) { HTTP_COMMAND(stream, b_div, false, b_class, id); }

static void HTTP_DIV(Print* stream, const char* key1 = nullptr, const char* value1 = nullptr, const char* key2 = nullptr, const char* value2 = nullptr) { HTTP_COMMAND(stream, b_div, false, key1, value1, key2, value2); }

static void HTTP_DIV_END(Print* stream) { HTTP_COMMAND(stream, b_div, true); }

static void HTTP_TABLE(Print* stream, const char* tableId) {
  stream->print(b_O);
  stream->print(b_table);
  stream->printf(PSTR(" class='%s'"), tableId);
  stream->print(b_C);
}

static void HTTP_TABLE_END(Print* stream) { HTTP_COMMAND(stream, b_table, true); }

static void HTTP_TR(Print* stream, bool end = false) { HTTP_COMMAND(stream, b_tr, end); }

static void HTTP_TD(Print* stream, byte colspan = 1) { 
  stream->print(b_O);
  stream->print(b_td);
  stream->printf(b_colspan, colspan);
  stream->print(b_C);
}

static void HTTP_TD_END(Print* stream) { HTTP_COMMAND(stream, b_td, true); }

static void HTTP_TH(Print* stream, byte colspan = 1) { 
  stream->print(b_O);
  stream->print(b_th);
  stream->printf(b_colspan, colspan);
  stream->print(b_C);
}

static void HTTP_TH_END(Print* stream) { HTTP_COMMAND(stream, b_th, true); }*/

const static char HTTP_HEAD_BEGIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='en'>
	<head>
		<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'/>
		<title>%s</title>
)=====";

const static char HTTP_STYLE[] PROGMEM = R"=====(
<style>
body{
	text-align: center;
	font-family: arial, sans-serif;
	background-color: #474e5d;
}

div{
  color:black;
	border:1.0rem
  border-radius:0.3rem;
	background-size: 1em;
	padding:5px;
}

.ip {
  width: 100%;
	-webkit-box-sizing: border-box; /* Safari/Chrome, other WebKit */
  -moz-box-sizing: border-box;    /* Firefox, other Gecko */
  box-sizing: border-box;         /* Opera/IE 8+ */
}

button, .cbtn {
	border:0;
	border-radius:0.3rem;
	background-color:#0070AF;
	color:white;
	line-height:2.4rem;
	font-size:1.2rem;
	width:100%;
	opacity: 0.85;
}

button:hover{
	opacity: 1.0;
}

.vd {
	border:0;
	border-radius:1.5rem;
	background-color:#007000;	
	line-height:2.4rem;
	font-size:1.2rem;
	width:100%;
	text-align: center;
}

.mc {
	display:inline-block;
	background-color: #fefefe;
	padding: 10px;
  text-align: center;
	border-radius:0.3rem;
	min-width:300px;
	text-align:left;
}

.cbtn{
	background-color:#FF3030;
}

.st, .tt {
	margin-left: auto;
  margin-right: auto;
}

.st th, .st td {
	border: 1px solid lightgray;
  border-collapse: collapse;
}

.st input[type='text']{
	width: 40px;
}

.switch {
	position: relative; 
	display: inline-block; 
	width: 120px; 
	height: 68px
} 

.switch input {
	display: none
}

.slider {
	position: absolute; 
	top: 0; 
	left: 0; 
	right: 0; 
	bottom: 0; 
	background-color: #ccc; 
	border-radius: 34px
}

.slider:before {
	position: absolute; 
	content: ""; 
	height: 52px; 
	width: 52px; 
	left: 8px; 
	bottom: 8px; 
	background-color: #fff; 
	-webkit-transition: .4s; 
	transition: .4s; 
	border-radius: 68px
}

input:checked+.slider {
	background-color: #2196F3
}

input:checked+.slider:before {
	-webkit-transform: 
	translateX(52px); 
	-ms-transform: translateX(52px); 
	transform: translateX(52px)
}

</style>
)=====";

const static char HTTP_HEAD_END[] PROGMEM = R"=====(
	</head>
	<body>
		<div class='mc'>
)=====";

const static char HTTP_BODY_END[] PROGMEM = R"=====(
		</div>
	</body>
</html>
)=====";

const static char HTTP_BUTTON[] PROGMEM = R"=====(
	<div>
  	<form action='/%s' method='%s'>
    	<button>%s</button>
    </form>
  </div>
)=====";

const static char HTTP_BUTTON_ALERT[] PROGMEM = R"=====(
	<div>
  	<form action='/%s' method='%s'>
    	<button class='cbtn'>%s</button>
    </form>
  </div>
)=====";

const static char HTTP_BUTTON_SUBMIT[] PROGMEM = R"=====(
		<div>
			<button type='submit'>%s</button>
		</div>
</form>
)=====";

const static char HTTP_CONFIG_SAVE_BUTTON[] PROGMEM = R"=====(
		<div>
			<button type='submit'>Save configuration</button>
		</div>
</form>
)=====";

const static char HTTP_TOGGLE_FUNCTION_SCRIPT[] PROGMEM = R"=====(
	<script>
		function %s {
			var sa = document.getElementById('%s');
			var ga = document.getElementById('%s');
			ga.style.display = (sa.checked ? 'block' : 'none');
			var gb = document.getElementById('%s');
			if (gb != null && gb !== undefined) {
				gb.style.display = (sa.checked ? 'none' : 'block');
			}
		}
	</script>
)=====";

const static char HTTP_TOGGLE_GROUP_STYLE[] PROGMEM = R"=====(
<style>
#%s {
  display:%s;
}
#%s {
	display:%s;
}
</style>
)=====";

const static char HTTP_SAVED[] PROGMEM = R"=====(
<div>
	%s
</div>
<div>
	ESP reboots now...
</div>
<div>
	<form action='/config' method='get'>
		<button>Back to configuration</button>
	</form>
</div>
)=====";

const static char HTTP_FORM_FIRMWARE[] PROGMEM = R"=====(
<form method='POST' action='' enctype='multipart/form-data'>
	<div>
  	<input type='file' accept='.bin' name='update'>
  </div>
  <div>
		<button type='submit' class='cbtn'>Update firmware</button>
  </div>
</form>
)=====";

/*const static char HTTP_CONFIG_PAGE_BEGIN[] PROGMEM = R"=====(
<form method='get' class='mc' action='submit%s'>
)=====";*/

const static char HTTP_INPUT_FIELD[] PROGMEM = R"=====(
	<input type='text' name='%s' maxlength='%s' value='%s'>
)=====";

const static char HTTP_TEXT_FIELD[] PROGMEM = R"=====(
	<div>
		%s<br>
		<input type='text' class='ip' name='%s' maxlength='%s' value='%s'>
	</div>
)=====";

const static char HTTP_PASSWORD_FIELD[] PROGMEM = R"=====(
	<div>
		%s<br>
		<input type='password' class='ip' name='%s' length=%s value='%s'>
	</div>
)=====";

const static char HTTP_CHECKBOX[] PROGMEM = R"=====(
		<label>
			<input type='checkbox' name='%s' value='true' %s>%s
		</label>		
)=====";

const static char HTTP_CHECKBOX_OPTION[] PROGMEM = R"=====(
	<label>
		<input type='checkbox' id='%s' name='%s' value='true' %s onclick='%s'>%s
	</label>
)=====";

const static char HTTP_RADIO_OPTION[] PROGMEM = R"=====(
	<label>
		<input type='radio' id='%s' name='%s' value='%s' %s onclick='%s'>%s
	</label>
)=====";

const static char HTTP_COMBOBOX_BEGIN[] PROGMEM = R"=====(
        <div>
			%s<br>
        	<select class='ip' name='%s'>
)=====";

const static char HTTP_COMBOBOX_ITEM[] PROGMEM = R"=====(
				<option class='ip' value='%s' %s>%s</option>
)=====";
const static char HTTP_COMBOBOX_END[] PROGMEM = R"=====(
			</select>
        </div>
)=====";

class WHtml_old {
 public:
  static void checkBox(Print* page, const char* id, const char* title, bool checked) {
    page->printf(HTTP_CHECKBOX_OPTION, id, id, (checked ? HTTP_CHECKED : ""), "", title);
  }

  static void comboBoxBegin(Print* page, const char* id, const char* title) {
    page->printf(HTTP_COMBOBOX_BEGIN, title, id);
  }

  static void comboBoxEnd(Print* page) {
    page->print(FPSTR(HTTP_COMBOBOX_END));
  }

  static void comboBoxItem(Print* page, const char* title, const char* value, bool selected) {
    page->printf(HTTP_COMBOBOX_ITEM, value, (selected ? HTTP_SELECTED : ""), title);
  }

  static void textField(Print* page, const char* fieldName, const char* title, byte maxLength, const char* value) {
    page->printf(HTTP_TEXT_FIELD, title, fieldName, String(maxLength).c_str(), value);
  }

};

#endif
