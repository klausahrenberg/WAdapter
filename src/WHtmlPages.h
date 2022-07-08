#ifndef WEB_THING_HTML_PAGES_H
#define WEB_THING_HTML_PAGES_H

#include "Arduino.h"

const char* HTTP_SELECTED = "selected";
const char* HTTP_CHECKED = "checked";
const char* HTTP_NONE = "none";
const char* HTTP_BLOCK = "block";
const char* HTTP_TRUE = "true";
const char* HTTP_FALSE = "false";

const static char HTTP_TR[] PROGMEM = R"=====(<tr>)=====";
void tr(Print* page) { page->print(FPSTR(HTTP_TR)); }
const static char HTTP_TRE[] PROGMEM = R"=====(</tr>)=====";
void trEnd(Print* page) { page->print(FPSTR(HTTP_TRE)); }
const static char HTTP_TH[] PROGMEM = R"=====(<th>)=====";
void th(Print* page) { page->print(FPSTR(HTTP_TH)); }
const static char HTTP_THE[] PROGMEM = R"=====(</th>)=====";
void thEnd(Print* page) { page->print(FPSTR(HTTP_THE)); }
const static char HTTP_TD[] PROGMEM = R"=====(<td>)=====";
void td(Print* page) { page->print(FPSTR(HTTP_TD)); }
const static char HTTP_TDE[] PROGMEM = R"=====(</td>)=====";
void tdEnd(Print* page) { page->print(FPSTR(HTTP_TDE)); }

const static char HTTP_HEAD_BEGIN[]         PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='en'>
	<head>
		<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'/>
		<title>%s</title>
)=====";

const static char HTTP_STYLE[]              PROGMEM = R"=====(
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

</style>
)=====";

const static char HTTP_HEAD_END[]           PROGMEM = R"=====(
	</head>
	<body>
		<div class='mc'>
)=====";

const static char HTTP_BODY_END[]           PROGMEM = R"=====(
		</div>
	</body>
</html>
)=====";

const static char HTTP_BUTTON[]    PROGMEM = R"=====(
	<div>
  	<form action='/%s' method='%s'>
    	<button>%s</button>
    </form>
  </div>
)=====";

const static char HTTP_BUTTON_ALERT[]    PROGMEM = R"=====(
	<div>
  	<form action='/%s' method='%s'>
    	<button class='cbtn'>%s</button>
    </form>
  </div>
)=====";

const static char HTTP_BUTTON_SUBMIT[]         PROGMEM = R"=====(
		<div>
			<button type='submit'>%s</button>
		</div>
</form>
)=====";

const static char HTTP_CONFIG_SAVE_BUTTON[]         PROGMEM = R"=====(
		<div>
			<button type='submit'>Save configuration</button>
		</div>
</form>
)=====";

const static char HTTP_DIV_BEGIN[]    PROGMEM = R"=====(
	<div>
)=====";

const static char HTTP_DIV_ID_BEGIN[]    PROGMEM = R"=====(
	<div id='%s'>
)=====";

const static char HTTP_DIV_END[]    PROGMEM = R"=====(
	</div>
)=====";

const static char HTTP_TOGGLE_FUNCTION_SCRIPT[]    PROGMEM = R"=====(
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

const static char HTTP_TOGGLE_GROUP_STYLE[]    PROGMEM = R"=====(
<style>
#%s {
  display:%s;
}
#%s {
	display:%s;
}
</style>
)=====";

const static char HTTP_SAVED[]              PROGMEM = R"=====(
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

const static char HTTP_CONFIG_PAGE_BEGIN[]         PROGMEM = R"=====(
<form method='get' class='mc' action='submit%s'>
)=====";

const static char HTTP_INPUT_FIELD[]    PROGMEM = R"=====(
	<input type='text' name='%s' maxlength='%s' value='%s'>
)=====";

const static char HTTP_TEXT_FIELD[]    PROGMEM = R"=====(
	<div>
		%s<br>
		<input type='text' class='ip' name='%s' maxlength='%s' value='%s'>
	</div>
)=====";

const static char HTTP_PASSWORD_FIELD[]    PROGMEM = R"=====(
	<div>
		%s<br>
		<input type='password' class='ip' name='%s' length=%s value='%s'>
	</div>
)=====";

const static char HTTP_CHECKBOX[]         PROGMEM = R"=====(
		<div>
			<label>
				<input type='checkbox' name='%s' value='true' %s>%s
			</label>
		</div>
)=====";

const static char HTTP_CHECKBOX_OPTION[]    PROGMEM = R"=====(
	<div>
		<label>
			<input type='checkbox' id='%s' name='%s' value='true' %s onclick='%s'>%s
		</label>
	</div>
)=====";

const static char HTTP_RADIO_OPTION[]    PROGMEM = R"=====(
	<label>
		<input type='radio' id='%s' name='%s' value='%s' %s onclick='%s'>%s
	</label>
)=====";

const static char HTTP_COMBOBOX_BEGIN[]         PROGMEM = R"=====(
        <div>
			%s<br>
        	<select class='ip' name='%s'>
)=====";
const static char HTTP_COMBOBOX_ITEM[]         PROGMEM = R"=====(
				<option class='ip' value='%s' %s>%s</option>
)=====";
const static char HTTP_COMBOBOX_END[]         PROGMEM = R"=====(
			</select>
        </div>
)=====";

#endif
