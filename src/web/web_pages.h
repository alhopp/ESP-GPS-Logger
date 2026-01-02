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
nav{display:flex;background:#e9eff7;border-bottom:1px solid var(--border)}
nav button{flex:1;padding:12px;border:0;background:none;color:var(--muted);font-weight:500}
nav button.a{color:var(--accent);border-bottom:3px solid var(--accent);background:#f8fbff}
section{display:none;padding:16px}
section.a{display:block}
.card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:14px;margin-bottom:14px}
label{display:block;margin-top:12px;font-size:13px;color:var(--muted)}
input,select{
width:100%;padding:10px;margin-top:4px;
border:1px solid var(--border);border-radius:6px;
background:#fff;color:var(--text);font-size:16px;caret-color:var(--accent)
}
input:focus,select:focus{outline:none;border-color:var(--accent)}
details{margin-top:12px}
summary{cursor:pointer;font-size:13px;color:var(--accent)}
footer{
position:relative;background:#f0f4fa;padding:12px;
padding-bottom:env(safe-area-inset-bottom);
display:flex;gap:10px;border-top:1px solid var(--border)
}
footer button{flex:1;padding:12px;border-radius:8px;border:0;font-weight:600;font-size:15px}
.save{background:var(--accent);color:#fff}
.reboot{background:var(--danger);color:#fff}
.small{font-size:12px;color:var(--muted);margin-top:6px}
</style>
</head>

<body>
<header>ESP32 GPS Configuration</header>

<nav>
<button class="a" onclick="tab('system',this)">System</button>
<button onclick="tab('gps',this)">GPS</button>
<button onclick="tab('power',this)">Power</button>
<button onclick="tab('wifi',this)">Wi-Fi</button>
</nav>


<section id="wifi">
<div class="card">
<label>Home Wi-Fi Network</label>
<input id="ssid"
       list="ssid_list"
       placeholder="Select or type network name"
       autocomplete="off">

<datalist id="ssid_list"></datalist>

<label>Password</label>
<input id="password"
       type="text"
       placeholder="Wi-Fi password"
       inputmode="text"
       autocomplete="off"
       autocorrect="off"
       autocapitalize="off"
       spellcheck="false"
       onfocus="this.type='password'">

<button id="scanBtn" style="margin-top:12px" onclick="scan()">Scan Networks</button>
<div class="small" id="scanInfo">Scanning may take ~5 seconds</div>
</div>
</section>

<section id="system" class="a">
<div class="card">
<label>CPU Frequency (MHz)</label>
<select id="cpu_freq"><option>80<option>160<option>240</select>
<label>Timezone</label>
<input id="timezone" type="number" step="0.5">
<details>
<summary>Advanced</summary>
<label><input type="checkbox" id="timezone_dst"> Daylight saving</label>
</details>
</div>
</section>

<section id="gps">
<div class="card">
<label>Sample Rate (Hz)</label>
<select id="sample_rate"><option>1<option>5<option>10</select>
<label>GNSS</label>
<select id="gnss">
<option value="1">GPS</option>
<option value="2">GPS + GLONASS</option>
<option value="3">GPS + GALILEO</option>
</select>
<details>
<summary>Advanced</summary>
<label>Dynamic Model</label>
<select id="dynamic_model"><option value="0">Portable<option value="1">Sea<option value="2">Auto</select>
<label>Speed Calibration</label>
<input id="cal_speed" type="number" step="0.01">
</details>
</div>
</section>

<section id="power">
<div class="card">
<label>Shutdown Voltage</label>
<input id="shutdown_voltage" type="number" step="0.1">
<details>
<summary>Advanced</summary>
<label><input type="checkbox" id="bat_choice"> Show battery %</label>
</details>
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

function addSSID(name){
  const list = $("ssid_list");

  if (![...list.options].some(o => o.value === name)) {
    const o = document.createElement("option");
    o.value = name;
    list.appendChild(o);
  }

  $("ssid").value = name;
}


async function load(){
const c=await (await fetch("/api/config")).json();
set($("cpu_freq"),c.system?.cpu_freq);
set($("timezone"),c.system?.timezone);
set($("timezone_dst"),c.system?.timezone_dst);
set($("sample_rate"),c.gps?.sample_rate);
set($("gnss"),c.gps?.gnss);
set($("dynamic_model"),c.gps?.dynamic_model);
set($("cal_speed"),c.gps?.cal_speed);
set($("shutdown_voltage"),c.power?.shutdown_voltage);
set($("bat_choice"),c.power?.bat_choice);

if(c.wifi?.ssid){
addSSID(c.wifi.ssid,true);
$("password").placeholder="Leave blank to keep existing password";
$("scanBtn").style.display="none";
$("scanInfo").textContent="Network already configured";
}
}

async function scan(){
  const list = $("ssid_list");
  list.innerHTML = "";

  const r = await fetch("/api/wifi/scan");
  const a = await r.json();

  a.sort((x,y)=>y.rssi-x.rssi).forEach(n=>{
    const o = document.createElement("option");
    o.value = n.ssid;
    list.appendChild(o);
  });
}


async function save(){
const wifi={ssid:$("ssid").value};
if($("password").value.length) wifi.password=$("password").value;

await fetch("/api/config",{
method:"POST",
headers:{"Content-Type":"application/json"},
body:JSON.stringify({wifi})
});

alert(
"Wi-Fi saved.\n\n" +
"The device is switching to Home Wi-Fi.\n" +
"Reconnect your phone to your normal network.\n\n" +
"Then open:\nhttp://esp32-gps.local"
);
}

function reboot(){fetch("/api/reboot",{method:"POST"});}
load();
</script>
</body>
</html>
)rawliteral";
