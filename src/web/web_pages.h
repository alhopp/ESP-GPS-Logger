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
  --bg:#f4f7fb;
  --card:#ffffff;
  --header:#0b2a4a;
  --accent:#1e88e5;
  --text:#0f172a;
  --muted:#5b6b82;
  --border:#dbe2ee;
  --danger:#d32f2f;
}

*{box-sizing:border-box}

body{
  margin:0;
  font:15px system-ui;
  background:var(--bg);
  color:var(--text);
}

/* Header */
header{
  padding:14px;
  text-align:center;
  font-weight:600;
  background:var(--header);
  color:white;
}

/* Tabs */
nav{
  display:flex;
  background:#e9eff7;
  border-bottom:1px solid var(--border);
}

nav button{
  flex:1;
  padding:12px;
  border:0;
  background:none;
  color:var(--muted);
  font-weight:500;
}

nav button.a{
  color:var(--accent);
  border-bottom:3px solid var(--accent);
  background:#f8fbff;
}

/* Sections */
section{
  display:none;
  padding:16px;
}

section.a{display:block}

/* Cards */
.card{
  background:var(--card);
  border-radius:10px;
  padding:14px;
  margin-bottom:14px;
  border:1px solid var(--border);
}

/* Inputs */
label{
  display:block;
  margin-top:12px;
  font-size:13px;
  color:var(--muted);
}

input,select{
  width:100%;
  padding:10px;
  margin-top:4px;
  border-radius:6px;
  border:1px solid var(--border);
  background:#f9fbfe;
  color:var(--text);
  font-size:15px;
}

input:focus,select:focus{
  outline:none;
  border-color:var(--accent);
  background:white;
}

/* Advanced */
details{margin-top:12px}
summary{
  cursor:pointer;
  font-size:13px;
  color:var(--accent);
}

/* Footer */
footer{
  position:sticky;
  bottom:0;
  background:#f0f4fa;
  padding:12px;
  display:flex;
  gap:10px;
  border-top:1px solid var(--border);
}

footer button{
  flex:1;
  padding:12px;
  border-radius:8px;
  border:0;
  font-weight:600;
  font-size:15px;
}

.save{
  background:var(--accent);
  color:white;
}

.reboot{
  background:var(--danger);
  color:white;
}
</style>
</head>

<body>

<header>ESP32 GPS Configuration</header>

<nav>
  <button class="a" onclick="tab('wifi',this)">Wi-Fi</button>
  <button onclick="tab('system',this)">System</button>
  <button onclick="tab('gps',this)">GPS</button>
  <button onclick="tab('power',this)">Power</button>
</nav>

<section id="wifi" class="a">
<div class="card">
<label>SSID</label>
<input id="ssid">
</div>
</section>

<section id="system">
<div class="card">
<label>CPU Frequency (MHz)</label>
<select id="cpu_freq">
  <option value="80">80</option>
  <option value="160">160</option>
  <option value="240">240</option>
</select>

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
<select id="sample_rate">
  <option value="1">1</option>
  <option value="5">5</option>
  <option value="10">10</option>
</select>

<label>GNSS</label>
<select id="gnss">
  <option value="1">GPS</option>
  <option value="2">GPS + GLONASS</option>
  <option value="3">GPS + GALILEO</option>
</select>

<details>
<summary>Advanced</summary>
<label>Dynamic Model</label>
<select id="dynamic_model">
  <option value="0">Portable</option>
  <option value="1">Sea</option>
  <option value="2">Automotive</option>
</select>

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
const $ = id => document.getElementById(id);

function tab(id,btn){
  document.querySelectorAll("nav button").forEach(b=>b.classList.remove("a"));
  document.querySelectorAll("section").forEach(s=>s.classList.remove("a"));
  btn.classList.add("a");
  $(id).classList.add("a");
}

function setVal(el,val){
  if(!el) return;
  if(el.type==="checkbox") el.checked=!!val;
  else el.value=val ?? "";
}

async function load(){
  const c=await (await fetch("/api/config")).json();

  setVal($("ssid"),c.wifi?.ssid);
  setVal($("cpu_freq"),c.system?.cpu_freq);
  setVal($("timezone"),c.system?.timezone);
  setVal($("timezone_dst"),c.system?.timezone_dst);

  setVal($("sample_rate"),c.gps?.sample_rate);
  setVal($("gnss"),c.gps?.gnss);
  setVal($("dynamic_model"),c.gps?.dynamic_model);
  setVal($("cal_speed"),c.gps?.cal_speed);

  setVal($("shutdown_voltage"),c.power?.shutdown_voltage);
  setVal($("bat_choice"),c.power?.bat_choice);
}

async function save(){
  const payload={
    wifi:{ssid:$("ssid").value},
    system:{
      cpu_freq:+$("cpu_freq").value,
      timezone:+$("timezone").value,
      timezone_dst:$("timezone_dst").checked
    },
    gps:{
      sample_rate:+$("sample_rate").value,
      gnss:+$("gnss").value,
      dynamic_model:+$("dynamic_model").value,
      cal_speed:+$("cal_speed").value
    },
    power:{
      shutdown_voltage:+$("shutdown_voltage").value,
      bat_choice:$("bat_choice").checked
    }
  };
  await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(payload)});
  alert("Saved");
}

function reboot(){
  fetch("/api/reboot",{method:"POST"});
}

load();
</script>
</body>
</html>
)rawliteral";
