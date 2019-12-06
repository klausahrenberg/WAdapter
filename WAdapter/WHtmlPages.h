#ifndef WEB_THING_HTML_PAGES_H
#define WEB_THING_HTML_PAGES_H

#include "Arduino.h"

const static char HTTP_HEAD_BEGIN[]         PROGMEM = R"=====(
<!DOCTYPE html>
<html lang=\"en\">
	<head>
		<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>
		<title>%s</title>
)=====";

const static char HTTP_STYLE[]              PROGMEM = R"=====(
<style>
body{
	text-align: center;
	font-family: arial, sans-serif;
} 

#bodyDiv{  
  display:inline-block;
  min-width:300px;
  text-align:left;
}

div{
	background-color:white;
  color:black;
	border:1.0rem
	border-color:black;
  border-radius:0.3rem;
	background-size: 1em;
	padding:5px;
	text-align:left;
} 

input[type="text"] {
  width: 100%;
}

input[type="password"] {
  width: 100%;
}

select{
	width:100%;
}

button{
	border:0;
	border-radius:0.3rem;
	background-color:#1fa3ec;
	color:#fff;
	line-height:2.4rem;
	font-size:1.2rem;
	width:100%;
}
</style>
)=====";

const static char HTTP_SCRIPT[]             PROGMEM = R"=====(
<script>
	function c(l){
		document.getElementById('s').value=l.innerText||l.textContent;
		document.getElementById('p').focus();
	}
</script>
)=====";

const static char HTTP_HEAD_END[]           PROGMEM = R"=====(
	</head>
	<body>
		<div id="bodyDiv">
)=====";

const static char HTTP_BODY_END[]           PROGMEM = R"=====(
		</div>
	</body>
</html>
)=====";

const static char HTTP_BUTTON_DEVICE[]         PROGMEM = R"=====(
        	<div>
        		<form action="/device_%s" method="get">
        			<button>Configure %s</button>
        		</form>
        	</div>
)=====";

const static char HTTP_PAGE_ROOT[]         PROGMEM = R"=====(
        	<div>
        		<form action="/wifi" method="get">
        			<button>Configure network</button>
        		</form>
        	</div>
        	<div>
        		<form action="/firmware" method="get">
        			<button>Update firmware</button>
        		</form>
        	</div>
        	<div>
        		<form action="/info" method="get">
        			<button>Info</button>
        		</form>
        	</div>
        	<div>
        		<form action="/reset" method="post">
        			<button>Reset</button>
        		</form>
        	</div>
)=====";

const static char HTTP_PAGE_CONFIGURATION_STYLE[]    PROGMEM = R"=====(
<style>
#mqttEnabled,#webthingEnabled{
	width:15%;
}
#mqttGroup {
  display:%s;
}
</style>
)=====";
const static char HTTP_PAGE_CONFIGURATION_GENERAL[]    PROGMEM = R"=====(
<form method='get' action='saveConfiguration'>
	<div>
		Thing IDX:<br>
		<input type="text" id='i' name='i' length=32 placeholder='idx or location of thing' value='%s'>
	</div>
	<div>
		SSID:<br>
		<input type="text" id='s' name='s' length=32 placeholder='WLAN name (only 2G)' value='%s'>
	</div>
	<div>
		Wifi Password:<br>
		<input type="password" id='p' name='p' length=64 placeholder='' value='%s'>
	</div>
)=====";
const static char HTTP_PAGE_CONFIGURATION_SERVICE[]    PROGMEM = R"=====(
	<div>
		<label>
			<input type="checkbox" id="webthingEnabled" name="wt" value="true" %s>
			Support Mozilla WebThings
		</label>
	</div>
	<div>
		<label>
			<input type="checkbox" id="mqttEnabled" name="mq" value="true" %s onclick="hideMqttGroup()">
			Support MQTT
		</label>
	</div>
)=====";
const static char HTTP_PAGE_CONFIGURATION_MQTT[]    PROGMEM = R"=====(
	<div id="mqttGroup">
		<div>
			MQTT Server:<br>
			<input type="text" id='ms' name='ms' length=32 placeholder='node' value='%s'>
		</div>
		<div>
			MQTT user:<br>
			<input type="text" id='mu' name='mu' length=32 placeholder='' value='%s'>
		</div>
		<div>
			MQTT password:<br>
			<input type="password" id='mp' name='mp' length=64 placeholder=''  value='%s'>
		</div>
		<div>
			Topic:<br>
			<input type="text" id='mt' name='mt' length=64 placeholder='home/room/thing' value='%s'>
		</div>
	</div>
	<script>
		function hideMqttGroup() {
			var cb = document.getElementById("mqttEnabled"); 
  			var x = document.getElementById("mqttGroup");
  			if (cb.checked) {
	    		x.style.display = "block";
  			} else {
    			x.style.display = "none";
  			}
		}
	</script>
	<div>
		<button type='submit'>Save configuration</button>
	</div>
</form>
)=====";

const static char HTTP_SAVED[]              PROGMEM = R"=====(
<div>
	{v}<br>
	Try reboot ESP.<br>
	If connect fails, start configuration again.
</div>
<div>
	<form action=\"/\" method=\"get\">
		<button>Back to configuration</button>
	</form>
</div>
)=====";

const static char HTTP_FORM_FIRMWARE[] PROGMEM = R"=====(
<form method='POST' action='/firmware' enctype='multipart/form-data'> 
	<div>
    	<input type='file' accept='.bin' name='update'>
    </div>
    <div>
		<button type='submit'>Update firmware</button> 
    </div>
</form>
)=====";

const static char HTTP_CONFIG_PAGE_BEGIN[]         PROGMEM = R"=====(
<form method='get' action='saveDeviceConfiguration_%s'>
)=====";

const static char HTTP_TEXT_FIELD[]    PROGMEM = R"=====(
	<div>
		%s<br>
		<input type="text" name="%s" length=%s value='%s'>
	</div>
)=====";

const static char HTTP_CONFIG_SAVE_BUTTON[]         PROGMEM = R"=====(	
		<div>
			<button type='submit'>Save configuration</button>
		</div>
</form>
)=====";

#endif
