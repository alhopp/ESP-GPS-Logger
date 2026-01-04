#pragma once
static const char PAGE_CONFIG_APP[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 GPS Config</title>

<style>
:root{
--bg:#f4f7fb;--card:#fff;--header:#0b2a4a;--accent:#1e88e5;
--text:#0f172a;--muted:#5b6b82;--border:#dbe2ee;--danger:#d32f2f
}
*{box-sizing:border-box}
body{margin:0;font:15px system-ui;background:var(--bg);color:var(--text)}
header{padding:14px;text-align:center;font-weight:600;background:var(--header);color:#fff}
nav{display:flex;flex-wrap:wrap;background:#e9eff7;border-bottom:1px solid var(--border)}
nav button{flex:1;padding:12px;border:0;background:none;color:var(--muted);font-weight:500}
nav button.a{color:var(--accent);border-bottom:3px solid var(--accent);background:#f8fbff}
section{display:none;padding:16px}
section.a{display:block}
.card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:14px;margin-bottom:14px}
h3{margin:0 0 6px 0;font-size:14px;color:var(--accent)}
label{display:block;margin-top:10px;font-size:13px;color:var(--muted)}
input,select{
width:100%;padding:10px;margin-top:4px;
border:1px solid var(--border);border-radius:6px;
background:#fff;color:var(--text);font-size:16px
}
footer{
background:#f0f4fa;padding:12px;
display:flex;gap:10px;border-top:1px solid var(--border)
}
footer button{flex:1;padding:12px;border-radius:8px;border:0;font-weight:600}
.save{background:var(--accent);color:#fff}
.reboot{background:var(--danger);color:#fff}
.small{font-size:12px;color:var(--muted);margin-top:6px}
.warn{font-size:12px;color:#b45309;margin-top:6px}
</style>
</head>

<body>
<header>ESP32 GPS Configuration</header>

<nav>
<button class="a" onclick="tab('system',this)">System</button>
<button onclick="tab('gps',this)">GPS</button>
<button onclick="tab('power',this)">Power</button>
<button onclick="tab('logging',this)">Logging</button>
<button onclick="tab('ui',this)">UI</button>
<button onclick="tab('wifi',this)">Wi-Fi</button>
<button onclick="tab('advanced',this)">Advanced</button>
</nav>

<!-- SYSTEM -->
<section id="system" class="a">
<div class="card">
<h3>System</h3>
<label>CPU Frequency (MHz)</label>
<select id="cpu_freq"><option>80<option>160<option>240</select>

<label>Timezone (hours)</label>
<input id="timezone" type="number" step="0.5">

<label><input type="checkbox" id="timezone_dst"> Daylight Saving</label>
</div>
</section>

<!-- GPS -->
<section id="gps">
<div class="card">
<h3>GPS</h3>
<label>Sample Rate (Hz)</label>
<select id="sample_rate"><option>1<option>5<option>10</select>

<label>GNSS</label>
<select id="gnss">
<option value="1">GPS</option>
<option value="2">GPS + GLONASS</option>
<option value="3">GPS + GALILEO</option>
</select>

<label>Speed Calibration</label>
<input id="cal_speed" type="number" step="0.01">
</div>
</section>

<!-- POWER -->
<section id="power">
<div class="card">
<h3>Power</h3>
<label>Shutdown Voltage</label>
<input id="shutdown_voltage" type="number" step="0.1">

<label><input type="checkbox" id="bat_choice"> Show Battery %</label>
</div>
</section>

<!-- LOGGING -->
<section id="logging">
<div class="card">
<h3>Logging</h3>
<label>Track Distance (m)</label>
<input id="track_distance" type="number">

<label><input type="checkbox" id="logTXT"> TXT</label>
<label><input type="checkbox" id="logUBX"> UBX</label>
<label><input type="checkbox" id="logSBP"> SBP</label>
<label><input type="checkbox" id="logGPY"> GPY</label>
<label><input type="checkbox" id="logGPX"> GPX</label>
</div>
</section>

<!-- UI -->
<section id="ui">
<div class="card">
<h3>Display</h3>
<label>Bar Length (m)</label>
<input id="bar_length" type="number">

<label><input type="checkbox" id="speed_large_font"> Large Speed Font</label>

<label>Sleep / Off Screen Code</label>
<input id="sleep_off_screen" type="number">

<label><input type="checkbox" id="Board_Logo"> Board Logo</label>
<label><input type="checkbox" id="Sail_Logo"> Sail Logo</label>
</div>
</section>

<!-- WIFI -->
<section id="wifi">
<div class="card">
<h3>Wi-Fi</h3>
<label>Home Wi-Fi</label>
<input id="ssid" list="ssid_list">
<datalist id="ssid_list"></datalist>

<label>Password</label>
<input id="password" type="password">

<button onclick="scan()">Scan</button>
<div class="small" id="wifiInfo"></div>
</div>
</section>

<!-- ADVANCED -->
<section id="advanced">
<div class="card">
<h3>Advanced – rarely changed</h3>
<div class="warn">Changing these incorrectly may affect logging or UI behaviour.</div>

<label>Speed Screen Sequence</label>
<input id="speed_screen">

<label>Stat Screen Sequence</label>
<input id="stat_screen">

<label>GPIO12 Screen Sequence</label>
<input id="gpio12_screen">

<label>Field</label>
<input id="field" type="number">

<label>Stat Screens</label>
<input id="Stat_screens" type="number">

<label>Stat Screen Time (s)</label>
<input id="Stat_screens_time" type="number">

<label>Stat Speed Threshold</label>
<input id="stat_speed" type="number">

<label>Start Logging Speed</label>
<input id="start_logging_speed" type="number">

<label>Archive Days</label>
<input id="archive_days" type="number">

<label>File Date Mode</label>
<input id="file_date_time" type="number">

<label>Sleep Info Text</label>
<input id="Sleep_info">
</div>
</section>

<footer>
<button class="save" onclick="save()">Save</button>
<button class="reboot" onclick="reboot()">Reboot</button>
</footer>

<script>
const $=id=>document.getElementById(id);
function tab(id,b){
document.querySelectorAll("nav button,section").forEach(e=>e.classList.remove("a"));
b.classList.add("a");$(id).classList.add("a");
}
function set(el,v){if(!el)return;el.type==="checkbox"?el.checked=!!v:el.value=v??"";}

async function load(){
const c=await (await fetch("/api/config")).json();
set($("cpu_freq"),c.system?.cpu_freq);
set($("timezone"),c.system?.timezone);
set($("timezone_dst"),c.system?.timezone_dst);

set($("sample_rate"),c.gps?.sample_rate);
set($("gnss"),c.gps?.gnss);
set($("cal_speed"),c.gps?.cal_speed);

set($("shutdown_voltage"),c.power?.shutdown_voltage);
set($("bat_choice"),c.power?.bat_choice);

set($("track_distance"),c.logging?.track_distance);
set($("logTXT"),c.logging?.logTXT);
set($("logUBX"),c.logging?.logUBX);
set($("logSBP"),c.logging?.logSBP);
set($("logGPY"),c.logging?.logGPY);
set($("logGPX"),c.logging?.logGPX);

set($("bar_length"),c.ui?.bar_length);
set($("speed_large_font"),c.ui?.speed_large_font);
set($("sleep_off_screen"),c.ui?.sleep_off_screen);
set($("Board_Logo"),c.ui?.Board_Logo);
set($("Sail_Logo"),c.ui?.Sail_Logo);

set($("speed_screen"),c.ui?.speed_screen);
set($("stat_screen"),c.ui?.stat_screen);
set($("gpio12_screen"),c.ui?.gpio12_screen);
set($("field"),c.ui?.field);
set($("Stat_screens"),c.ui?.Stat_screens);
set($("Stat_screens_time"),c.ui?.Stat_screens_time);
set($("stat_speed"),c.ui?.stat_speed);
set($("start_logging_speed"),c.ui?.start_logging_speed);
set($("archive_days"),c.ui?.archive_days);
set($("file_date_time"),c.ui?.file_date_time);
set($("Sleep_info"),c.ui?.Sleep_info);

if(c.wifi?.ssid){$("ssid").value=c.wifi.ssid;$("wifiInfo").textContent="Saved network";}
}

async function scan(){
const list=$("ssid_list");list.innerHTML="";
const a=await (await fetch("/api/wifi/scan")).json();
a.sort((x,y)=>y.rssi-x.rssi).forEach(n=>{
const o=document.createElement("option");o.value=n.ssid;list.appendChild(o);
});
}

async function save(){
const payload={
system:{cpu_freq:+$("cpu_freq").value,timezone:+$("timezone").value,timezone_dst:$("timezone_dst").checked},
gps:{sample_rate:+$("sample_rate").value,gnss:+$("gnss").value,cal_speed:+$("cal_speed").value,
stat_speed:+$("stat_speed").value,start_logging_speed:+$("start_logging_speed").value},
power:{shutdown_voltage:+$("shutdown_voltage").value,bat_choice:$("bat_choice").checked},
logging:{track_distance:+$("track_distance").value,archive_days:+$("archive_days").value,
file_date_time:+$("file_date_time").value,
logTXT:$("logTXT").checked,logUBX:$("logUBX").checked,logSBP:$("logSBP").checked,
logGPY:$("logGPY").checked,logGPX:$("logGPX").checked},
ui:{bar_length:+$("bar_length").value,speed_large_font:$("speed_large_font").checked,
sleep_off_screen:+$("sleep_off_screen").value,Board_Logo:$("Board_Logo").checked,
Sail_Logo:$("Sail_Logo").checked,field:+$("field").value,
speed_screen:$("speed_screen").value,stat_screen:$("stat_screen").value,
gpio12_screen:$("gpio12_screen").value,Stat_screens:+$("Stat_screens").value,
Stat_screens_time:+$("Stat_screens_time").value,Sleep_info:$("Sleep_info").value},
wifi:{ssid:$("ssid").value}
};
if($("password").value) payload.wifi.password=$("password").value;
await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(payload)});
alert("Configuration saved");
}
function reboot(){fetch("/api/reboot",{method:"POST"});}
load();
</script>
</body>
</html>
)rawliteral";
