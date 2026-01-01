#pragma once
static const char PAGE_CONFIG_APP[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 GPS Control</title>

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
  background:#fff;color:var(--text);font-size:16px
}
input:focus,select:focus{outline:none;border-color:var(--accent)}
details{margin-top:12px}
summary{cursor:pointer;font-size:13px;color:var(--accent)}
footer{
  background:#f0f4fa;padding:12px;
  padding-bottom:env(safe-area-inset-bottom);
  display:flex;gap:10px;border-top:1px solid var(--border)
}
footer button{flex:1;padding:12px;border-radius:8px;border:0;font-weight:600;font-size:15px}
.primary{background:var(--accent);color:#fff}
.danger{background:var(--danger);color:#fff}
.small{font-size:12px;color:var(--muted);margin-top:6px}
a.file{display:block;margin:6px 0;color:var(--accent);text-decoration:none}
a.file:hover{text-decoration:underline}
hr{border:none;border-top:1px solid var(--border);margin:12px 0}
</style>
</head>

<body>
<header>ESP32 GPS Control</header>

<nav>
  <button class="a" onclick="tab('wifi',this)">Wi-Fi</button>
  <button onclick="tab('system',this)">System</button>
  <button onclick="tab('gps',this)">GPS</button>
  <button onclick="tab('power',this)">Power</button>
  <button onclick="tab('files',this)">Files</button>
</nav>

<!-- ===================== Wi-Fi ===================== -->
<section id="wifi" class="a">
<div class="card">

<label>Device Connection</label>
<div class="small">
You are connected directly to the device.<br>
This connection always remains active.
</div>

<hr>

<label>Join Home Wi-Fi (optional)</label>
<input id="ssid" list="ssid_list"
       placeholder="Select or type network name"
       autocomplete="off">
<datalist id="ssid_list"></datalist>

<label>Password</label>
<input id="password" type="password"
       placeholder="Wi-Fi password"
       autocomplete="off" autocorrect="off"
       autocapitalize="off" spellcheck="false">

<button style="margin-top:12px" onclick="scan()">Scan Networks</button>
<button class="primary" style="margin-top:8px" onclick="joinHome()">Join Home Wi-Fi</button>
<button style="margin-top:8px" onclick="leaveHome()">Disconnect Home Wi-Fi</button>

<div class="small" id="wifiStatus">
Home Wi-Fi is currently disconnected.
</div>

</div>
</section>

<!-- ===================== System ===================== -->
<section id="system">
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

<!-- ===================== GPS ===================== -->
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
<select id="dynamic_model">
  <option value="0">Portable</option>
  <option value="1">Sea</option>
  <option value="2">Auto</option>
</select>

<label>Speed Calibration</label>
<input id="cal_speed" type="number" step="0.01">
</details>
</div>
</section>

<!-- ===================== Power ===================== -->
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

<!-- ===================== Files ===================== -->
<section id="files">
<div class="card">
<label>SD Card Files</label>
<div id="fileList" class="small">Open this tab to load files…</div>
</div>
</section>

<footer>
<button class="primary" onclick="saveConfig()">Save Settings</button>
<button class="danger" onclick="reboot()">Reboot</button>
</footer>

<script>
const $=id=>document.getElementById(id);

function tab(id,b){
  document.querySelectorAll("nav button,section")
    .forEach(e=>e.classList.remove("a"));
  b.classList.add("a");
  $(id).classList.add("a");
  if(id==="files") loadFiles();
}

function set(el,v){
  if(!el) return;
  el.type==="checkbox" ? el.checked=!!v : el.value=v??"";
}

// ---------- Load config ----------
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
}

// ---------- Wi-Fi scan ----------
async function scan(){
  $("ssid_list").innerHTML="";
  const a=await (await fetch("/api/wifi/scan")).json();
  a.sort((x,y)=>y.rssi-x.rssi)
   .forEach(n=>$("ssid_list")
   .appendChild(new Option(n.ssid,n.ssid)));
}

// ---------- STA control ----------
async function joinHome(){
  const ssid=$("ssid").value.trim();
  if(!ssid){ alert("Please enter an SSID"); return; }
  const wifi={ssid};
  if($("password").value.length) wifi.password=$("password").value;
  await fetch("/api/wifi/join",{
    method:"POST",
    headers:{"Content-Type":"application/json"},
    body:JSON.stringify(wifi)
  });
  $("wifiStatus").textContent=
    "Joining home Wi-Fi… device remains reachable here.";
}

async function leaveHome(){
  await fetch("/api/wifi/leave",{method:"POST"});
  $("wifiStatus").textContent=
    "Home Wi-Fi disconnected. Device Wi-Fi only.";
}

// ---------- Save settings ----------
async function saveConfig(){
  await fetch("/api/config",{method:"POST"});
  alert("Settings saved.");
}

// ---------- Files ----------
async function loadFiles(){
  const box=$("fileList");
  box.textContent="Loading…";
  try{
    const files=await (await fetch("/api/files")).json();
    if(!files.length){ box.textContent="No files found."; return; }
    box.innerHTML="";
    files.forEach(f=>{
      const a=document.createElement("a");
      a.className="file";
      a.href="/api/file?name="+encodeURIComponent(f.name);
      a.textContent=`${f.name} (${(f.size/1024).toFixed(1)} KB)`;
      box.appendChild(a);
    });
  }catch{
    box.textContent="Failed to load files.";
  }
}

function reboot(){ fetch("/api/reboot",{method:"POST"}); }
load();
</script>
</body>
</html>
)rawliteral";
