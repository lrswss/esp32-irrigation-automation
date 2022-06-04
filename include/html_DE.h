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
        if (Number(json.level) <= __MIN_WATER_LEVEL__ || Number(json.level) > __WATER_RESERVOIR_HEIGHT__) {
          if (Number(json.level) == -2) {
            document.getElementById("waterLevelIgnored").style.display = "block";
            document.getElementById("waterLevelErr").style.display = "none";
          } else {
            document.getElementById("waterLevelIgnored").style.display = "none";
            document.getElementById("waterLevelErr").style.display = "block";
          }
          err++;
        } else {
            document.getElementById("waterLevelIgnored").style.display = "none";
            document.getElementById("waterLevelErr").style.display = "none";
        }
        document.getElementById("LevelInfo").style.display = "table-row";
      }
      if ("moist1" in json) {
        document.getElementById("moisture").style.display = "block";
        document.getElementById("sensor_moist1").style.display = "table-row";
        if (Number(json.moist1) > 100) {
          document.getElementById("Moist1").innerHTML = json.moist1;
        } else if (Number(json.moist1) >= 0) {
          document.getElementById("Moist1").innerHTML = json.moist1 + " %"
        } else {
          document.getElementById("Moist1").innerHTML = "--";
        }
      }
      if ("moist2" in json) {
        document.getElementById("moisture").style.display = "block";
        document.getElementById("sensor_moist2").style.display = "table-row";
        document.getElementById("Moist2").innerHTML = json.moist2;
        if (Number(json.moist2) > 100) {
          document.getElementById("Moist2").innerHTML = json.moist2;
        } else if (Number(json.moist2) >= 0) {
          document.getElementById("Moist2").innerHTML = json.moist2 + " %";
        } else {
          document.getElementById("Moist2").innerHTML = "--";
        }
      }
      if ("moist3" in json) {
        document.getElementById("moisture").style.display = "block";
        document.getElementById("sensor_moist3").style.display = "table-row";
        document.getElementById("Moist3").innerHTML = json.moist3;
        if (Number(json.moist3) > 100) {
          document.getElementById("Moist3").innerHTML = json.moist3;
        } else if (Number(json.moist3) >= 0) {
          document.getElementById("Moist3").innerHTML = json.moist3 + " %";
        } else {
          document.getElementById("Moist3").innerHTML = "--";
        }
      }
      if ("moist4" in json) {
        document.getElementById("moisture").style.display = "block";
        document.getElementById("sensor_moist4").style.display = "table-row";
        document.getElementById("Moist4").innerHTML = json.moist4;
        if (Number(json.moist4) > 100) {
          document.getElementById("Moist4").innerHTML = json.moist4;
        } else if (Number(json.moist4) >= 0) {
          document.getElementById("Moist4").innerHTML = json.moist4 + " %";
        } else {
          document.getElementById("Moist4").innerHTML = "--";
        }
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

      if (Number(json.valve1) >= 0) {
        setValveSwitch(1, Number(json.valve1));
        document.getElementById("switch_valve1").style.display = "table-row";
      }
      if (Number(json.valve2) >= 0) {
        setValveSwitch(2, Number(json.valve2));
        document.getElementById("switch_valve2").style.display = "table-row";
      }
      if (Number(json.valve3) >= 0) {
        setValveSwitch(3, Number(json.valve3));
        document.getElementById("switch_valve3").style.display = "table-row";
      }
      if (Number(json.valve4) >= 0) {
        setValveSwitch(4, Number(json.valve4));
        document.getElementById("switch_valve4").style.display = "table-row";
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
<span id="waterLevelIgnored" style="display:none">Wasserstand wird ignoriert!</span>
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
  <tr id="switch_valve1" style="display:none"><th>__RELAY1_LABEL__</th><td><label class="switch"><input id="valve1" type="checkbox" onclick="setValve(1);"><span id="valve1_slider" class="slider"></span></label></td></tr>
  <tr id="switch_valve2" style="display:none"><th>__RELAY2_LABEL__</th><td><label class="switch"><input id="valve2" type="checkbox" onclick="setValve(2);"><span id="valve2_slider" class="slider"></span></label></td></tr>
  <tr id="switch_valve3" style="display:none"><th>__RELAY3_LABEL__</th><td><label class="switch"><input id="valve3" type="checkbox" onclick="setValve(3);"><span id="valve3_slider" class="slider"></span></label></td></tr>
  <tr id="switch_valve4" style="display:none"><th>__RELAY4_LABEL__</th><td><label class="switch"><input id="valve4" type="checkbox" onclick="setValve(4);"><span id="valve4_slider" class="slider"></span></label></td></tr>
</table>
</fieldset>
</div>
<div id="moisture" style="margin-top:10px;display:none">
<fieldset><legend><b>&nbsp;Sensoren Bodenfeuchte&nbsp;</b></legend>
<table style="min-width:325px">
  <tr id="sensor_moist1" style="display:none"><th>__MOIST1_LABEL__</th><td><span id="Moist1">--</span></td></tr>
  <tr id="sensor_moist2" style="display:none"><th>__MOIST2_LABEL__</th><td><span id="Moist2">--</span></td></tr>
  <tr id="sensor_moist3" style="display:none"><th>__MOIST3_LABEL__</th><td><span id="Moist3">--</span></td></tr>
  <tr id="sensor_moist4" style="display:none"><th>__MOIST4_LABEL__</th><td><span id="Moist4">--</span></td></tr>
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
  document.getElementById("message").style.display = "none";
  document.getElementById("configSaved").style.display = "none";
  document.getElementById("timeError").style.display = "none";
  document.getElementById("irrTimeError").style.display = "none";
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

  if (document.getElementById("checkbox_auto_irrigation").checked == true) {
    if (!document.getElementById("input_irrigation_time").value.match(/^\d{2}:\d{2}$/)) {
      document.getElementById("timeError").style.display = "block";
      err++;
    }
    for (var i = 1; i <= 4; i++) {
      if (document.getElementById("input_irrigation_relay"+i).value > 
            document.getElementById("input_pump_autostop").value) {
        document.getElementById("irrTimeError").style.display = "block";
        err++;
        break;
      }
    }

    if (document.getElementById("checkbox_ignore_water_level").checked == true) {
      alert("Wenn der Wasserstand nicht berücksichtigt wird, kann die Wasserpumpe trockenlaufen und beschädigt werden!")
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

function timeOnly(input) {
  var regex = /[^0-9:]/g;
  input.value = input.value.replace(regex, "");
}

function toggleAutoIrrigation() {
  if (document.getElementById("checkbox_auto_irrigation").checked == true) {
    document.getElementById("auto_irrigation").style.display = "block";
  } else {
    document.getElementById("auto_irrigation").style.display = "none";
  }
}

function toggleWaterLevel() {
  if (document.getElementById("checkbox_ignore_water_level").checked == true) {
    document.getElementById("input_min_water_level").style.display = "none";
  } else {
    document.getElementById("input_min_water_level").style.display = "block";
  }
}

</script>
</head>
<body onload="configSaved(); toggleAutoIrrigation(); toggleWaterLevel();">
<div style="text-align:left;display:inline-block;min-width:340px;">
<div style="text-align:center;">
<h2 id="heading">Einstellungen</h2>
<div id="message" style="display:none;margin-top:10px;color:red;font-weight:bold;text-align:center;max-width:335px">
<span id="configSaved" style="display:none;color:green">Einstellungen gespeichert</span>
<span id="timeError" style="display:none">Uhrzeitformat prüfen (HH:MM)</span>
<span id="irrTimeError" style="display:none">Dauer Bewässerungszeiten prüfen!</span>
</div>
</div>

<div style="max-width:335px;margin-top:10px;">
<form method="POST" action="/config" onsubmit="return checkInput();">
  <fieldset><legend><b>&nbsp;Automatische Bewässerung&nbsp;</b></legend>
  <p><input id="checkbox_auto_irrigation" name="auto_irrigation" type="checkbox" __AUTO_IRRIGATION__ onclick="toggleAutoIrrigation();"><b>Aktivieren</b></p>
  <span style="display:none" id="auto_irrigation">
  <p><b>Uhrzeit (HH:MM)</b><br />
  <input id="input_irrigation_time" name="irrigation_time" type="text" value="__IRRIGATION_TIME__" maxlength="5" onkeyup="timeOnly(this);"></p>
  <p><b>Min. Bewässerungspause (Std.)</b><br />
  <input id="input_irrigation_pause" name="irrigation_pause" type="text" value="__IRRIGATION_PAUSE__" maxlength="2" onkeyup="digitsOnly(this);"></p>
  <p><b>__RELAY1_LABEL__ (Sek.)</b><br />
  <input id="input_irrigation_relay1" name="irrigation_relay1_secs" type="text" value="__IRRIGATION_RELAY1_SECS__" maxlength="3" onkeyup="digitsOnly(this);"></p>
  <p><b>__RELAY2_LABEL__ (Sek.)</b><br />
  <input id="input_irrigation_relay2" name="irrigation_relay2_secs" type="text" value="__IRRIGATION_RELAY2_SECS__" maxlength="3" onkeyup="digitsOnly(this);"></p>
  <p><b>__RELAY3_LABEL__ (Sek.)</b><br />
  <input id="input_irrigation_relay3" name="irrigation_relay3_secs" type="text" value="__IRRIGATION_RELAY3_SECS__" maxlength="3" onkeyup="digitsOnly(this);"></p>
  <p><b>__RELAY4_LABEL__ (Sek.)</b><br />
  <input id="input_irrigation_relay4" name="irrigation_relay4_secs" type="text" value="__IRRIGATION_RELAY4_SECS__" maxlength="3" onkeyup="digitsOnly(this);"></p>
  </span>
  </fieldset>
  <br />
  
  <fieldset><legend><b>&nbsp;Wasserpumpe&nbsp;</b></legend>
  <p><b>Autoabschaltung (Sek.)</b><br />
  <input id="input_pump_autostop" name="pump_autostop" type="text" value="__PUMP_AUTOSTOP__" maxlength="3" onkeyup="digitsOnly(this);"></p>
  <p><b>Bewässerungssperre (Min.)</b><br />
  <input id="input_pump_blocktime" name="pump_blocktime" type="text" value="__PUMP_BLOCKTIME__" maxlength="4" onkeyup="digitsOnly(this);"></p>
  <p><b>Höhe des Wasserbehälters (cm)</b><br />
  <input id="input_reservoir_height" name="reservoir_height" type="text" value="__RESERVOIR_HEIGHT__" maxlength="3" onkeyup="digitsOnly(this);"></p>
  <span id="input_min_water_level">
  <p><b>Minimaler Wasserstand (cm)</b><br />
  <input name="min_water_level" type="text" value="__MIN_WATER_LEVEL__" maxlength="3" onkeyup="digitsOnly(this);"></p></span>
  <p><input id="checkbox_ignore_water_level" name="ignore_water_level" type="checkbox" __IGNORE_WATER_LEVEL__ onclick="toggleWaterLevel();"><b>Wasserstand ignorieren</b></p>
  </fieldset>
  <br />

  <fieldset><legend><b>&nbsp;Sonstiges&nbsp;</b></legend>
  <p><input id="checkbox_logging" name="logging" type="checkbox" __LOGGING__ onclick="toggleLogging();"><b>Protokollierung aktivieren</b></p>
  </fieldset>
  <br />

  <p><button class="button bred" type="submit">Einstellungen speichern</button></p>
</form>
<!-- <p><button class="button bred" onclick="clearSettings(); return false;">Einstellungen zur&uuml;cksetzen</button></p> -->
<p><button onclick="location.href='/pins';">Anschlussbelegung</button></p>
<p><button onclick="location.href='/network';">Netzwerkeinstellungen</button></p>
<p><button onclick="location.href='/';">Startseite</button></p>
</div>
)=====";


const char NETWORK_html[] PROGMEM = R"=====(
<script>
function hideMessages() {
  document.getElementById("message").style.display = "none";
  document.getElementById("configSaved").style.display = "none";
  document.getElementById("appassError").style.display = "none";
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

function toggleMQTT() {
  if (document.getElementById("checkbox_mqtt").checked == true) {
    document.getElementById("mqtt").style.display = "block";
  } else {
    document.getElementById("mqtt").style.display = "none";
  }
}

function toggleMQTTAuth() {
  if (document.getElementById("checkbox_mqttauth").checked == true) {
    document.getElementById("mqttauth").style.display = "block";
  } else {
    document.getElementById("mqttauth").style.display = "none";
  }
}

</script>
</head>
<body onload="configSaved(); toggleMQTT(); toggleMQTTAuth();">
<div style="text-align:left;display:inline-block;min-width:340px;">
<div style="text-align:center;">
<h2 id="heading">Netzwerkeinstellungen</h2>
<div id="message" style="display:none;margin-top:10px;color:red;font-weight:bold;text-align:center;max-width:335px">
<span id="configSaved" style="display:none;color:green">Einstellungen gespeichert</span>
<span id="appassError" style="display:none">Passwort f&uuml;r Access-Point zu kurz!</span>
</div>
</div>

<div style="max-width:335px;margin-top:10px;">
<form method="POST" action="/network" onsubmit="return checkInput();">
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
  <p><input id="checkbox_mqtt" name="mqtt" onclick="toggleMQTT();" type="checkbox" __MQTT__><b>Aktivieren</b></p>
  <span id="mqtt" style="display:none">
  <p><b>Adresse des Brokers</b><br />
  <input name="mqttbroker" size="16" maxlength="64" value="__MQTT_BROKER__"></p>
  <p><b>Topic f&uuml;r Steuerung</b><br />
  <input name="mqtttopiccmd" size="16" maxlength="64" value="__MQTT_TOPIC_CMD__"></p>
  <p><b>Topic f&uuml;r Nachrichten</b><br />
  <input name="mqtttopicstate" size="16" maxlength="64" value="__MQTT_TOPIC_STATE__"></p>
  <p><b>Nachrichteninterval (Sek.)</b><br />
  <input name="mqttinterval" value="__MQTT_INTERVAL__" maxlength="4 onkeyup="digitsOnly(this)"></p>
  <p><input id="checkbox_mqttauth" name="mqttauth" onclick="toggleMQTTAuth();" type="checkbox" __MQTT_AUTH__><b>Authentifizierung aktivieren</b></p>
     <span style="display:none" id="mqttauth">
     <p><b>Benutzername</b><br />
     <input id="input_mqttuser" name="mqttuser" size="16" maxlength="32" value="__MQTT_USERNAME__"></p>
     <p><b>Passwort</b><br />
     <input id="input_mqttpassword" type="password" name="mqttpassword" size="16" maxlength="32" value="__MQTT_PASSWORD__"></p>
     </span>
  </span>
  </fieldset>
  <br />

  <p><button class="button bred" type="submit">Einstellungen speichern</button></p>
</form>
<p><button onclick="location.href='/config';">Einstellungen</button></p>
<p><button onclick="location.href='/';">Startseite</button></p>
</div>
)=====";


const char PINS_html[] PROGMEM = R"=====(
<script>
function hideMessages() {
  document.getElementById("message").style.display = "none";
  document.etElementById("configSaved").style.display = "none";
  document.getElementById("pinError").style.display = "none";
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

  for (var i = 1; i <= 3 && err == 0; i++) {
    for (var j = i+1; j <= 4 && err == 0; j++) {
      select1 = document.getElementById("relay"+i+"_pin_selector");
      select2 = document.getElementById("relay"+j+"_pin_selector");
      if (select1.options[select1.selectedIndex].value != -1 && 
        select1.options[select1.selectedIndex].value == select2.options[select2.selectedIndex].value) {  
        document.getElementById("pinError").style.display = "block";
        err++;
      }
    }
  }
  for (var i = 1; i <= 3 && err == 0; i++) {
    for (var j = i+1; j <= 4 && err == 0; j++) {
      select1 = document.getElementById("moist"+i+"_pin_selector");
      select2 = document.getElementById("moist"+j+"_pin_selector");
      if (select1.options[select1.selectedIndex].value != -1 && 
        select1.options[select1.selectedIndex].value == select2.options[select2.selectedIndex].value) {  
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

function pinSelector(pins, id, parent, value) {
    var selectList;
    var option;

    selectList = document.createElement("select");
    selectList.id = id + "_selector";
    selectList.name = id;
    selectList.classList.add("pin_value");
    option = document.createElement("option");
    option.value = "-1";
    option.text = "-";
    selectList.appendChild(option);
    parent.appendChild(selectList);
    for (var i = 0; i < pins.length; i++) {
        var option = document.createElement("option");
        option.value = pins[i];
        option.text = pins[i];
        selectList.appendChild(option);
    }
    selectList.value = value;
}

function initPage() {
    pinSelector([ __RELAY_PINS__ ], "relay1_pin", document.getElementById("relay1"), __RELAY1_PIN__);
    pinSelector([ __RELAY_PINS__ ], "relay2_pin", document.getElementById("relay2"), __RELAY2_PIN__);
    pinSelector([ __RELAY_PINS__ ], "relay3_pin", document.getElementById("relay3"), __RELAY3_PIN__);
    pinSelector([ __RELAY_PINS__ ], "relay4_pin", document.getElementById("relay4"), __RELAY4_PIN__);
    pinSelector([ __MOISTURE_PINS__ ], "moist1_pin", document.getElementById("moist1"), __MOIST1_PIN__);
    pinSelector([ __MOISTURE_PINS__ ], "moist2_pin", document.getElementById("moist2"), __MOIST2_PIN__);
    pinSelector([ __MOISTURE_PINS__ ], "moist3_pin", document.getElementById("moist3"), __MOIST3_PIN__);
    pinSelector([ __MOISTURE_PINS__ ], "moist4_pin", document.getElementById("moist4"), __MOIST4_PIN__);
}

</script>
</head>
<body onload="initPage(); configSaved(); toggleAutoIrrigation();">
<div style="text-align:left;display:inline-block;min-width:340px;">
<div style="text-align:center;">
<h2 id="heading">Anschlussbelegung</h2>
<div id="message" style="display:none;margin-top:10px;color:red;font-weight:bold;text-align:center;max-width:335px">
<span id="configSaved" style="display:none;color:green">Einstellungen gespeichert</span>
<span id="pinError" style="display:none">Pin-Zuweisungen überprüfen!</span>
</div>
</div>

<div style="max-width:335px;margin-top:10px;">
<form method="POST" action="/pins" onsubmit="return checkInput();">
  <fieldset><legend><b>&nbsp;Ventilnamen und Pins&nbsp;</b></legend>
  <p id="relay1"><input id="input_relay1" class="pin_label" type="text" name="relay1_name" size="16" maxlength="24" value="__RELAY1_LABEL__"></p>
  <p id="relay2"><input id="input_relay2" class="pin_label" type="text" name="relay2_name" size="16" maxlength="24" value="__RELAY2_LABEL__"></p>
  <p id="relay3"><input id="input_relay3" class="pin_label" type="text" name="relay3_name" size="16" maxlength="24" value="__RELAY3_LABEL__"></p>
  <p id="relay4"><input id="input_relay4" class="pin_label" type="text" name="relay4_name" size="16" maxlength="24" value="__RELAY4_LABEL__"></p>
  </fieldset>
  <br />

  <fieldset><legend><b>&nbsp;Feuchtesensoren und Pins&nbsp;</b></legend>
  <p id="moist1"><input id="input_moist1" class="pin_label" type="text" name="moist1_name" size="16" maxlength="24" value="__MOIST1_LABEL__"></p>
  <p id="moist2"><input id="input_moist2" class="pin_label" type="text" name="moist2_name" size="16" maxlength="24" value="__MOIST2_LABEL__"></p>
  <p id="moist3"><input id="input_moist3" class="pin_label" type="text" name="moist3_name" size="16" maxlength="24" value="__MOIST3_LABEL__"></p>
  <p id="moist4"><input id="input_moist4" class="pin_label" type="text" name="moist4_name" size="16" maxlength="24" value="__MOIST4_LABEL__"></p>
  <p><b>Sensormesswert für 0%</b><br />
  <input id="input_moist_min" type="text" name="moisture_min" maxlength="3" value="__MOISTURE_MIN__"></p>
  <p><b>Sensormesswert für 100%</b><br />
  <p><input id="input_moist_max" type="text" name="moisture_max" maxlength="3" value="__MOISTURE_MAX__"></p>
  <p><input id="checkbox_moisture_raw" name="moisture_raw" type="checkbox" __MOISTURE_RAW__ "><b>Sensormesswert anzeigen</b></p>
  <p><input id="checkbox_moisture_avg" name="moisture_avg" type="checkbox" __MOISTURE_AVG__ "><b>Gleitender Durchschnitt</b></p>
  </fieldset>
  <br />
  <p><button class="button bred" type="submit">Einstellungen speichern</button></p>
</form>
<p><button onclick="location.href='/config';">Einstellungen</button></p>
<p><button onclick="location.href='/';">Startseite</button></p>
</div>
)=====";
