/***************************************************************************
  Copyright (c) 2021-2022 Lars Wessels

  This file a part of the "ESP32-Irrigation-Automation" source code.
  https://github.com/lrswss/esp32-irrigation-automation

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
   
  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

***************************************************************************/

const char HEADER_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<meta name="author" content="(c) 2021-2022 Lars Wessels, Karlsruhe, GERMANY">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Bewässerungsautomat</title>
<style>
body{text-align:center;font-family:verdana;background:white;}
div,fieldset,input,select{padding:5px;font-size:1em;}
h2{margin-top:2px;margin-bottom:8px}
h3{font-size:0.8em;margin-top:-2px;margin-bottom:2px;font-weight:lighter;}
p{margin:0.5em 0;}
a{text-decoration:none;color:inherit;}
input{width:100%;box-sizing:border-box;-webkit-box-sizing:border-box;-moz-box-sizing:border-box;}
input[type=checkbox],input[type=radio]{width:1em;margin-right:6px;vertical-align:-1px;}
p,input[type=text]{font-size:0.96em;}
select{width:100%;}
textarea{resize:none;width:98%;height:318px;padding:5px;overflow:auto;}
button{border:0;border-radius:0.3rem;background:#009374;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;-webkit-transition-duration:0.4s;transition-duration:0.4s;cursor:pointer;}
button:hover{background:#007364;}
button:focus{outline:0;}
td{text-align:right;}
.bred{background:#d43535;}
.bred:hover{background:#931f1f;}
.bgrn{background:#47c266;}
.bgrn:hover{background:#5aaf6f;}
.footer{font-size:0.6em;color:#aaa;}
.switch{position:relative;display:inline-block;width:42px;height:24px;}
.switch input{opacity:0;width:0;height:0;}
.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;
  background-color:#ccc;-webkit-transition:.2s;transition:.2s;border-radius:24px;}
.sliderDisabled{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;
  background-color:#d43535;-webkit-transition:.2s;transition:.2s;border-radius:24px;}
.slider:before,.sliderDisabled:before{position:absolute;content:"";height:18px;width:18px;left:3px;bottom:3px;
  background-color:white;-webkit-transition:.2s;transition:.2s;border-radius:50%; }
input:checked+.slider{background-color:#47c266;}
input:focus+.slider{box-shadow: 0 0 1px #47c266;}
input:checked+.slider:before {-webkit-transform: translateX(18px);
  -ms-transform:translateX(18px);transform:translateX(18px);}
.pin_label{width:77%;}
.pin_value{width:18%;margin-left:9px;}
</style>
)=====";


const char ROOT_html[] PROGMEM = R"=====(
<script>
var suspendReadings = false;

function restartSystem() {
  var xhttp = new XMLHttpRequest();
  suspendReadings = true;
  if (confirm("System wirklich neustarten?")) {
    document.getElementById("wifiOffline").style.display = "none";
    document.getElementById("sysRestart").style.display = "block";
    document.getElementById("message").style.height = "16px";
    document.getElementById("heading").scrollIntoView();
    xhttp.open("GET", "/restart", true);
    xhttp.send();
    setTimeout(function(){location.href='/';}, 15000);
  }
}


function getReadings() {
  var xhttp = new XMLHttpRequest();
  var json;
  var height = 0;
  var err = 0;

  if (suspendReadings)
    return;
 
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      json = JSON.parse(xhttp.responseText);
      if (json.logging == "1")
        document.getElementById("logfileDownload").style.display = "block";
      if (json.wifi == "1") {
        document.getElementById("wifiOffline").style.display = "none";
      } else {
        document.getElementById("wifiOffline").style.display = "block";
        err++;
      }
      if ("date" in json) {
        document.getElementById("Date").innerHTML = json.date;
        document.getElementById("Time").innerHTML = json.time;
        document.getElementById("TZ").innerHTML = json.tz;
      }
      document.getElementById("Runtime").innerHTML = json.runtime;
      if ("temp" in json) {
        document.getElementById("Temp").innerHTML = json.temp;
        document.getElementById("TempInfo").style.display = "table-row";
      }
      if ("hum" in json) {
        document.getElementById("Hum").innerHTML = json.hum;
        document.getElementById("HumInfo").style.display = "table-row";
      }
      if ("heap" in json) {
        document.getElementById("Heap").innerHTML = json.heap;
        document.getElementById("HeapInfo").style.display = "table-row";
      }
      if ("level" in json) {
        if (Number(json.level) > 0) {
          document.getElementById("Level").innerHTML = json.level;
        } else {
          document.getElementById("Level").innerHTML = "--";
        }
        if (Number(json.level) < 0 || Number(json.level) > __WATER_RESERVOIR_HEIGHT__) {
          document.getElementById("waterLevelErr").style.display = "block";
          err++;
        } else {
          document.getElementById("waterLevelErr").style.display = "none";
        }
        document.getElementById("LevelInfo").style.display = "table-row";
      }
      if (err > 0) {
        height = (err * 12) + 4;
        document.getElementById("message").style.height = height + "px";
        document.getElementById("message").style.display = "block";
        document.getElementById("heading").scrollIntoView();
      } else {
        document.getElementById("message").style.display = "none";
      }
    }
  };
  xhttp.open("GET", "/ui", true);
  xhttp.send();
}


function initPage() {
  setTimeout(function() { getReadings(); }, 250);
  setTimeout(function() { setValve(0); }, 500);
  setInterval(function() { getReadings(); setTimeout(function() { setValve(0); }, 500); }, 3000);
}


function setValveSwitch(num, state) {
  if (state == 0 || state == 1) {
    document.getElementById("valve"+num+"_slider").classList.remove("sliderDisabled");
    document.getElementById("valve"+num+"_slider").classList.add("slider");  
    document.getElementById("valve"+num).disabled = false;
    if (state == 0)
      document.getElementById("valve"+num).checked = false;
    else
      document.getElementById("valve"+num).checked = true;
  } else if (state == 2) {
    document.getElementById("valve"+num).checked = false;
    document.getElementById("valve"+num).disabled = true;
    document.getElementById("valve"+num+"_slider").classList.remove("slider");
    document.getElementById("valve"+num+"_slider").classList.add("sliderDisabled");
   }
}


function setValve(pin) {
  var xhttp = new XMLHttpRequest();

  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      json = JSON.parse(xhttp.responseText);

      if (json.valve1 == "0") {
        setValveSwitch(1, 0);
      } else if (json.valve1 == "1") {
        setValveSwitch(1, 1);
      } else if (json.valve1 == "2") {
        setValveSwitch(1, 2);
      }
      if (json.valve2 == "0") {
        setValveSwitch(2, 0);
      } else if (json.valve2 == "1") {
        setValveSwitch(2, 1);
      } else if (json.valve2 == "2") {
        setValveSwitch(2, 2);
      }
      if (json.valve3 == "0") {
        setValveSwitch(3, 0);
      } else if (json.valve3 == "1") {
        setValveSwitch(3, 1);
      } else if (json.valve3 == "2") {
        setValveSwitch(3, 2);
      }      
      if (json.valve4 == "0") {
        setValveSwitch(4, 0);
      } else if (json.valve4 == "1") {
        setValveSwitch(4, 1);
      } else if (json.valve4 == "2") {
        setValveSwitch(4, 2);
      }    
    }
  }

  if (pin > 0) {
    if (document.getElementById("valve" + pin).checked == true) {
      xhttp.open("GET", "/valve?on=" + pin, true);
    } else {
      xhttp.open("GET", "/valve?off=" + pin, true);
    }
  } else {
    xhttp.open("GET", "/valve?status", true);
  }  
  xhttp.send();
}

</script>
</head>
<body onload="initPage();">
<div style="text-align:left;display:inline-block;min-width:340px;">
<div style="text-align:center;">
<h2 id="heading">Bewässerung __SYSTEMID__</h2>
<div id="message" style="display:none;margin-top:10px;margin-bottom:8px;color:red;text-align:center;font-weight:bold;max-width:335px">
<span id="sysRestart" style="display:none">System wird neu gestartet...</span>
<span id="wifiOffline" style="display:none">Kein WiFi uplink!</span>
<span id="waterLevelErr" style="display:none">Wasserstand prüfen!</span>

</div>
<noscript>Bitte JavaScript aktivieren!<br /></noscript>
</div>
<div id="readings" style="margin-top:0px">
<table style="min-width:340px">
  <tr><th>Systemzeit (<span id="TZ">---</span>):</th><td><span id="Date">--.--.--</span> <span id="Time">--:--</span></td></tr>
  <tr><th>Laufzeit (netto):</th><td><span id="Runtime">--d --h --m</span></td></tr>
  <tr id="TempInfo" style="display:none;"><th>Temperatur:</th><td><span id="Temp">--</span> &deg;C</td></tr>
  <tr id="HumInfo" style="display:none;"><th>Luftfeuchtigkeit:</th><td><span id="Hum">--</span> %</td></tr>
  <tr id="LevelInfo" style="display:none;"><th>Wasserstand:</th><td><span id="Level">--</span> cm</td></tr>
  <tr id="HeapInfo" style="display:none;"><th>Free Heap Memory:</th><td><span id="Heap">-------</span> bytes</td></tr>
</table>
</div>
<div id="valves" style="margin-top:10px;">
<fieldset><legend><b>&nbsp;Manuelle Ventilsteuerung&nbsp;</b></legend>
<table style="min-width:325px">
  <tr><th>__RELAIS1_LABEL__</th><td><label class="switch"><input id="valve1" type="checkbox" onclick="setValve(1);"><span id="valve1_slider" class="slider"></span></label></td></tr>
  <tr><th>__RELAIS2_LABEL__</th><td><label class="switch"><input id="valve2" type="checkbox" onclick="setValve(2);"><span id="valve2_slider" class="slider"></span></label></td></tr>
  <tr><th>__RELAIS3_LABEL__</th><td><label class="switch"><input id="valve3" type="checkbox" onclick="setValve(3);"><span id="valve3_slider" class="slider"></span></label></td></tr>
  <tr><th>__RELAIS4_LABEL__</th><td><label class="switch"><input id="valve4" type="checkbox" onclick="setValve(4);"><span id="valve4_slider" class="slider"></span></label></td></tr>
</table>
</fieldset>
</div>
<div id="buttons" style="margin-top:10px">
<p><button onclick="location.href='/config';">Einstellungen</button></p>
<p id="logfileDownload" style="display:none;"><button onclick="location.href='/logs';">Logdateien herunterladen</button></p>
<p><button onclick="location.href='/update';">Firmware aktualisieren</button></p>
<p><button class="button bred" onclick="restartSystem();">System neustarten</button></p>
</div>
)=====";


const char FOOTER_html[] PROGMEM = R"=====(
<div class="footer"><hr/>
<p style="float:left;margin-top:-2px"><a href="https://github.com/lrswss/esp32-irrigation-automation" title="build on __BUILD__">Firmware __FIRMWARE__</a></p>
<p style="float:right;margin-top:-2px"><a href="mailto:software@bytebox.org">&copy; 2021-2022 Lars Wessels</a></p>
<div style="clear:both;"></div>
</div>
</div>
</body>
</html>
)=====";


const char UPDATE_html[] PROGMEM = R"=====(
</head>
<body>
<div style="text-align:left;display:inline-block;min-width:340px;">
<div style="text-align:center;">
<h2>Firmware-Update</h2>
</div>
<div style="margin-left:50px;margin-bottom:10px">
1. Firmware-Datei ausw&auml;hlen<br>
2. Aktualisierung starten<br>
3. Upload dauert ca. 20 Sek.<br>
4. System neu starten
</div>
<div>
<form method="POST" action="/update" enctype="multipart/form-data" id="upload_form">
  <input type="file" name="update">
  <p><button class="button bred" type="submit">Aktualisierung durchf&uuml;hren</button></p>
</form>
<p><button onclick="location.href='/';">Startseite</button></p>
</div>
)=====";


const char UPDATE_ERR_html[] PROGMEM = R"=====(
</head>
<body>
<div style="text-align:left;display:inline-block;min-width:340px;">
<div style="text-align:center;">
<h2>Firmware-Update<br />fehlgeschlagen</h2>
</div>
<div>
<p><button onclick="location.href='/update';">Update wiederholen</button></p>
<p><button onclick="location.href='/';">Startseite</button></p>
</div>
)=====";


const char UPDATE_OK_html[] PROGMEM = R"=====(
<script>

function resetSystem() {
  var xhttp = new XMLHttpRequest();
  if (confirm("System wirklich neu starten?")) {
    document.getElementById("sysReset").style.display = "block";
    document.getElementById("message").style.display = "block";
    xhttp.open("GET", "/reset", true);
    xhttp.send();
    setTimeout(function(){location.href='/';}, 15000);
  }
}

function clearSettings() {
  var xhttp = new XMLHttpRequest();
  if (confirm("Alle Einstellungen zurücksetzen?")) {
    xhttp.open("GET", "/delnvs", true);
    xhttp.send();
  }
}
</script>
</head>
<body>
<div style="text-align:left;display:inline-block;min-width:340px;">
<div style="text-align:center;">
<h2 id="heading">Firmware-Update<br />erfolgreich</h2>
<div id="message" style="display:none;margin-top:8px;color:red;font-weight:bold;height:16px;text-align:center;max-width:335px">
<span id="sysReset" style="display:none">System wird zurückgesetzt...<br></span>
</div>
</div>
<div>
<p><button class="button bred" onclick="clearSettings(); resetSystem();">System zurücksetzen</button></p>
<p><button onclick="location.href='/';">Startseite</button></p>
</div>
)=====";


const char LOGS_HEADER_html[] PROGMEM = R"=====(
<script>
function deleteLogs() {
  var xhttp = new XMLHttpRequest();
  if (confirm("Wirklich alle Dateien löschen?")) {
    xhttp.open("GET", "/rmlogs", true);
    xhttp.send();
    document.getElementById("filelist").style.display = "none";
  }
}
</script>
</head>
<body>
<div style="text-align:left;display:inline-block;min-width:340px;">
<div style="text-align:center;">
<h2>Logdateien</h2>
<h3>__BYTES_FREE__ kb frei</h3>
</div>
<div id="filelist" style="margin-left:50px;margin-top:20px;margin-bottom:10px">
)=====";


const char LOGS_FOOTER_html[] PROGMEM = R"=====(
</div>
<div>
<p><button class="button bred" onclick="deleteLogs();">Logdateien l&ouml;schen</button></p>
<p><button onclick="location.href='/sendlogs';">Logdateien herunterladen</button></p>
<p><button onclick="location.href='/';">Startseite</button></p>
</div>
)=====";


const char CONFIG_html[] PROGMEM = R"=====(
<script>
function hideMessages() {
  document.getElementById("configSaved").style.display = "none";
  document.getElementById("appassError").style.display = "none";
  document.getElementById("pinError").style.display = "none";
  document.getElementById("message").style.display = "none";
}

function clearSettings() {
  var xhttp = new XMLHttpRequest();
  suspendReadings = true;
  if (confirm("Alle Einstellungen zurücksetzen?")) {
    xhttp.open("GET", "/delnvs", true);
    xhttp.send();
    setTimeout(function(){location.href='/config?reset';}, 500);
  }
}

function configSaved() {
    const url = new URLSearchParams(window.location.search);
    if (url.has("saved")) {
        document.getElementById("configSaved").style.display = "block";
        document.getElementById("message").style.display = "block";
        document.getElementById("heading").scrollIntoView();
        setTimeout(hideMessages, 4000);
    }
}

function checkInput() {
  var err = 0;
  var height = 0;
  var xhttp = new XMLHttpRequest();

  if (document.getElementById("input_appassword").value.length > 0 && 
      document.getElementById("input_appassword").value.length < 8) {
    document.getElementById("appassError").style.display = "block";
    err++;
  }
  for (var i = 1; i <= 3 && err == 0; i++) {
    for (var j = i+1; j <= 4 && err == 0; j++) {
      select1 = document.getElementById("relais"+i+"_pin_selector");
      select2 = document.getElementById("relais"+j+"_pin_selector");
      if (select1.options[select1.selectedIndex].value == select2.options[select2.selectedIndex].value) {  
        document.getElementById("pinError").style.display = "block";
        err++;
      }
    }
  }
  if (err > 0) {
    height = (err * 12) + 4;
    document.getElementById("message").style.height = height + "px";
    document.getElementById("message").style.display = "block";
    document.getElementById("heading").scrollIntoView();
    setTimeout(hideMessages, 4000);
    xhttp.open("GET", "tickle", true); // reset webserver timeout
    xhttp.send();
    return false;
  } else {
    document.getElementById("message").style.display = "none";
    return true;
  }
}

function digitsOnly(input) {
  var regex = /[^0-9]/g;
  input.value = input.value.replace(regex, "");
}

function toggleMQTTAuth() {
  if (document.getElementById("checkbox_mqttauth").checked == true) {
    document.getElementById("mqttauth").style.display = "block";
  } else {
    document.getElementById("mqttauth").style.display = "none";
  }
}

function pinSelector(id, parent, value) {
    var pins = [ __RELAIS_PINS__ ];
    var selectList;

    selectList = document.createElement("select");
    selectList.id = id + "_selector";
    selectList.name = id;
    selectList.classList.add("pin_value");
    parent.appendChild(selectList);
    for (var i = 0; i < pins.length; i++) {
        var option = document.createElement("option");
        option.value = pins[i];
        if (option.value == 0)
            option.text = "-";
        else
            option.text = pins[i];
        selectList.appendChild(option);
    }
    selectList.value = value;
}

function initPage() {
    pinSelector("relais1_pin", document.getElementById("relais1"), __RELAIS1_PIN__);
    pinSelector("relais2_pin", document.getElementById("relais2"), __RELAIS2_PIN__);
    pinSelector("relais3_pin", document.getElementById("relais3"), __RELAIS3_PIN__);
    pinSelector("relais4_pin", document.getElementById("relais4"), __RELAIS4_PIN__);
}

</script>
</head>
<body onload="initPage(); configSaved(); toggleMQTTAuth();">
<div style="text-align:left;display:inline-block;min-width:340px;">
<div style="text-align:center;">
<h2 id="heading">Einstellungen</h2>
<div id="message" style="display:none;margin-top:10px;color:red;height:16px;font-weight:bold;text-align:center;max-width:335px">
<span id="configSaved" style="display:none;color:green">Einstellungen gespeichert!</span>
<span id="appassError" style="display:none">Passwort f&uuml;r Access-Point zu kurz!</span>
<span id="pinError" style="display:none">Pin für Relais doppelt vergeben!</span>
</div>
</div>
<div style="max-width:335px;margin-top:10px;">
<form method="POST" action="/config" onsubmit="return checkInput();">
  <fieldset><legend><b>&nbsp;WLAN&nbsp;</b></legend>
  <p><b>Netzwerkkennung (SSID)</b><br />
  <input id="input_stassid" name="stassid" size="16" maxlength="32" value="__STA_SSID__"></p>
  <p><b>Passwort (optional)</b><br />
  <input id="input_stapassword" type="password" name="stapassword" size="16" maxlength="32" value="__STA_PASSWORD__"></p>
  <p><b>Passwort f&uuml;r lokalen Access-Point</b><br />
  <input id="input_appassword" type="password" name="appassword" size="16" maxlength="32" value="__AP_PASSWORD__"></p>
  </fieldset>
  <br />
  
  <fieldset><legend><b>&nbsp;MQTT&nbsp;</b></legend>
  <p><b>Adresse des Brokers</b><br />
  <input name="mqttbroker" size="16" maxlength="64" value="__MQTT_BROKER__"></p>
  <p><b>Topic f&uuml;r Steuerung</b><br />
  <input name="mqtttopiccmd" size="16" maxlength="64" value="__MQTT_TOPIC_CMD__"></p>
  <p><b>Topic f&uuml;r Nachrichten</b><br />
  <input name="mqtttopicstate" size="16" maxlength="64" value="__MQTT_TOPIC_STATE__"></p>
  <p><b>Nachrichteninterval (Sek.)</b><br />
  <input name="mqttinterval" value="__MQTT_INTERVAL__" maxlength="4 "onkeyup="digitsOnly(this)"></p>
  <p><input id="checkbox_mqttauth" name="mqttauth" onclick="toggleMQTTAuth();" type="checkbox" __MQTT_AUTH__><b>Authentifizierung aktivieren</b></p>
     <span style="display:none" id="mqttauth">
     <p><b>Benutzername</b><br />
     <input id="input_mqttuser" name="mqttuser" size="16" maxlength="32" value="__MQTT_USERNAME__"></p>
     <p><b>Passwort</b><br />
     <input id="input_mqttpassword" type="password" name="mqttpassword" size="16" maxlength="32" value="__MQTT_PASSWORD__"></p>
     </span>
  </fieldset>
  <br />

  <fieldset><legend><b>&nbsp;Wasserpumpe&nbsp;</b></legend>
  <p><b>Autoabschaltung (Sek.)</b><br />
  <input id="input_pump_autostop" name="pump_autostop" type="text" value="__PUMP_AUTOSTOP__" maxlength="3" onclick="digitsOnly();"></p>
  <p><b>Bewässerungssperre (Sek.)</b><br />
  <input id="input_pump_blocktime" name="pump_blocktime" type="text" value="__PUMP_BLOCKTIME__" maxlength="3" onclick="digitsOnly();"></p>
  <p><b>Höhe des Wasserbehälters (cm)</b><br />
  <input id="input_reservoir_height" name="reservoir_height" type="text" value="__RESERVOIR_HEIGHT__" maxlength="3" onclick="digitsOnly();"></p>
  <p><b>Minimaler Wasserstand (cm)</b><br />
  <input id="input_pump_waterlevel" name="min_water_level" type="text" value="__MIN_WATER_LEVEL__" maxlength="3" onclick="digitsOnly();"></p>
  </fieldset>
  <br />

  <fieldset><legend><b>&nbsp;Ventilnamen und Anschlusspins&nbsp;</b></legend>
  <p id="relais1"><input id="input_relais1" class="pin_label" type="text" name="relais1_name" size="16" maxlength="24" value="__RELAIS1_LABEL__"></p>
  <p id="relais2"><input id="input_relais2" class="pin_label" type="text" name="relais2_name" size="16" maxlength="24" value="__RELAIS2_LABEL__"></p>
  <p id="relais3"><input id="input_relais3" class="pin_label" type="text" name="relais3_name" size="16" maxlength="24" value="__RELAIS3_LABEL__"></p>
  <p id="relais4"><input id="input_relais4" class="pin_label" type="text" name="relais4_name" size="16" maxlength="24" value="__RELAIS4_LABEL__"></p>
  </fieldset>
  <br />

  <!-- <fieldset><legend><b>&nbsp;Feuchtesensoren und Anschlusspins&nbsp;</b></legend>
  <p><input id="input_moist1" class="pin_label" type="text" name="relais1_name" size="16" maxlength="16" value="__MOIST1__">
  <select id="select_moist1" class="pin_value" name="moist1_pin"><option value="0">-</option><option value="2">3</option></select></p>
  <p><input id="input_moist2" class="pin_label" type="text" name="relais2_name" size="16" maxlength="16" value="__MOIST2__">
  <select id="select_moist2" class="pin_value" name="moist2_pin"><option value="0">-</option><option value="2">3</option></select></p>
  <p><input id="input_moist3" class="pin_label" type="text" name="relais3_name" size="16" maxlength="16" value="__MOIST3__">
  <select id="select_moist3" class="pin_value" name="moist3_pin"><option value="0">-</option><option value="2">3</option></select></p>
  <p><input id="input_moist4" class="pin_label" type="text" name="relais2_name" size="16" maxlength="16" value="__MOIST4__">
  <select id="select_moist4" class="pin_value" name="moist4_pin"><option value="0">-</option><option value="2">3</option></select></p>
  </fieldset>
  <br /> -->
  
  <fieldset><legend><b>&nbsp;Sonstiges&nbsp;</b></legend>
  <p><input id="checkbox_logging" name="logging" type="checkbox" __LOGGING__ onclick="toggleLogging();"><b>Protokollierung aktivieren</b></p>
  </fieldset>
  <br />

  <p style="margin-top:25px"><button class="button bred" onclick="clearSettings(); return false;">Einstellungen zur&uuml;cksetzen</button></p>
  <p><button type="submit">Einstellungen speichern</button></p>
</form>
<p><button onclick="location.href='/';">Startseite</button></p>
</div>
)=====";
