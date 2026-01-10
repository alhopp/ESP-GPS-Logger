#pragma once
static const char PAGE_CONFIG_APP[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>ESP32 GPS</title>

<style>
:root{
  --a:#1e88e5;--b:#0b2a4a;--c:#f4f7fb;
  --d:#fff;--e:#dbe2ee;--f:#5b6b82
}
*{box-sizing:border-box}
body{
  margin:0;
  font:17px system-ui;
  background:var(--c)
}
header{
  padding:16px;
  font-size:22px;
  font-weight:700;
  text-align:center;
  background:var(--b);
  color:#fff
}

nav{
  display:flex;
  background:#e9eff7;
  border-bottom:1px solid var(--e)
}
nav button{
  flex:1;
  padding:16px;
  border:0;
  background:none;
  color:var(--f);
  font-size:18px;
  font-weight:600
}
nav button.a{
  color:var(--a);
  border-bottom:3px solid var(--a);
  background:#f8fbff
}

section{display:none;padding:16px}
section.a{display:block}

.card{
  background:var(--d);
  border:1px solid var(--e);
  border-radius:12px;
  padding:16px;
  margin-bottom:16px
}

h3{
  margin:0 0 10px;
  font-size:18px;
  color:var(--a)
}

label{
  display:block;
  margin-top:12px;
  font-size:15px;
  color:var(--f)
}

input,select{
  width:100%;
  padding:12px;
  margin-top:6px;
  border:1px solid var(--e);
  border-radius:8px;
  font-size:17px
}

.small{
  font-size:15px;
  color:var(--f);
  margin-top:6px
}

/* Toggle switches */
.toggle{
  display:flex;
  justify-content:space-between;
  align-items:center;
  margin-top:14px;
  font-size:16px
}
.toggle input{display:none}
.slider{
  width:52px;height:30px;
  background:#cfd6e4;
  border-radius:30px;
  position:relative
}
.slider:before{
  content:"";
  position:absolute;
  width:26px;height:26px;
  left:2px;top:2px;
  background:#fff;
  border-radius:50%;
  transition:.2s
}
.toggle input:checked + .slider{
  background:var(--a)
}
.toggle input:checked + .slider:before{
  transform:translateX(22px)
}

/* File list */
.file-list{
  max-height:60vh;
  overflow-y:auto;
  -webkit-overflow-scrolling:touch
}
.file-row{
  display:flex;
  justify-content:space-between;
  padding:14px 4px;
  border-bottom:1px solid var(--e)
}
.file-name{font-size:15px;word-break:break-all}
.file-size{font-size:14px;color:var(--f)}
.file-actions button{
  border:none;
  background:none;
  font-size:24px;
  padding:6px;
  color:var(--a)
}

/* System read-only list */
.sys-row{
  display:flex;
  justify-content:space-between;
  padding:14px 0;
  border-bottom:1px solid var(--e);
  font-size:16px
}
.sys-row span:first-child{color:var(--f)}
.sys-row span:last-child{
  font-weight:600;
  color:#000
}

footer{
  padding:16px;
  border-top:1px solid var(--e);
  background:#f0f4fa
}
footer button{
  width:100%;
  padding:16px;
  border-radius:12px;
  border:0;
  font-weight:700;
  font-size:18px;
  background:var(--a);
  color:#fff
}
</style>
</head>

<body>

<header>ESP32 GPS</header>

<nav>
<button class=a onclick="tab('files',this)">Files</button>
<button onclick="tab('settings',this)">Settings</button>
<button onclick="tab('system',this)">System</button>
</nav>

<!-- FILES -->
<section id=files class=a>
<div class=card>
<h3>SD Card Files</h3>
<div class=small id=sdInfo>Scanning SD card…</div>
<div class=file-list id=fileList></div>
</div>
</section>

<!-- SETTINGS -->
<section id=settings>

<div class=card>
<h3>User</h3>
<label>Name</label>
<input id=Sleep_info>
</div>

<div class=card>
<h3>Statistics</h3>
<label>Stat Screens</label>
<input id=Stat_screens type=number>
<label>Stat Screen Sequence</label>
<input id=stat_screen>
</div>

<div class=card>
<h3>Display</h3>
<label>Board Logo</label>
<select id=Board_Logo>
<option value=0>Off</option>
<option value=1>Generic</option>
<option value=2>Slalom</option>
<option value=3>Foil</option>
<option value=4>Wave</option>
<option value=5>Speed</option>
</select>

<label>Sail Logo</label>
<select id=Sail_Logo>
<option value=0>Off</option>
<option value=1>Neutral</option>
<option value=2>Race</option>
<option value=3>Wave</option>
<option value=4>Foil</option>
</select>
</div>

<div class=card>
<h3>Logging</h3>
<div class=toggle><span>TXT</span><label><input type=checkbox id=logTXT><div class=slider></div></label></div>
<div class=toggle><span>UBX</span><label><input type=checkbox id=logUBX><div class=slider></div></label></div>
<div class=toggle><span>SBP</span><label><input type=checkbox id=logSBP><div class=slider></div></label></div>
<div class=toggle><span>GPY</span><label><input type=checkbox id=logGPY><div class=slider></div></label></div>
<div class=toggle><span>GPX</span><label><input type=checkbox id=logGPX><div class=slider></div></label></div>
</div>

<div class=card>
<h3>Power</h3>
<label>Battery Calibration</label>
<input id=cal_bat type=number step=0.01>
</div>

<div class=card>
<h3>Wi-Fi</h3>
<label>SSID</label>
<input id=ssid>
<label>Password</label>
<input id=password type=password>
</div>

</section>

<!-- SYSTEM -->
<section id=system>
<div class=card>
<h3>System</h3>

<div class=sys-row><span>GNSS Module</span><span>u-blox NEO-M10</span></div>
<div class=sys-row><span>Storage</span><span>128 MB</span></div>
<div class=sys-row><span>Software</span><span>Version 1</span></div>

<hr style="border:none;border-top:1px solid var(--e);margin:10px 0">

<div class=sys-row><span>Speed Units</span><span>Knots</span></div>
<div class=sys-row><span>Sample Rate</span><span id=sys_sample_rate></span></div>
<div class=sys-row><span>GNSS</span><span id=sys_gnss></span></div>
<div class=sys-row><span>Dynamic Model</span><span id=sys_dynamic_model></span></div>

</div>
</section>


<footer id=footer>
<button onclick=save()>Save</button>
</footer>

<script>
const $=i=>document.getElementById(i);

function tab(id,b){
  document.querySelectorAll("nav button,section").forEach(e=>e.classList.remove("a"));
  b.classList.add("a");
  $(id).classList.add("a");
  footer.style.display = (id==="system") ? "none" : "block";
}

function set(e,v){
  if(!e)return;
  e.type==="checkbox" ? e.checked=!!v : e.value=v??"";
}

async function load(){
  const c=await(await fetch("/api/config")).json();

  set(Sleep_info,c.ui?.Sleep_info);
  set(Stat_screens,c.ui?.Stat_screens);
  set(stat_screen,c.ui?.stat_screen);
  set(Board_Logo,c.ui?.Board_Logo);
  set(Sail_Logo,c.ui?.Sail_Logo);
  set(cal_bat,c.power?.cal_bat);

  set(logTXT,c.logging?.logTXT);
  set(logUBX,c.logging?.logUBX);
  set(logSBP,c.logging?.logSBP);
  set(logGPY,c.logging?.logGPY);
  set(logGPX,c.logging?.logGPX);

  // System (firmware-defined)
  $("sys_sample_rate").textContent   = c.gps?.sample_rate || "";
  $("sys_gnss").textContent          = c.gps?.gnss || "";
  $("sys_dynamic_model").textContent = c.gps?.dynamic_model || "";

  ssid.value=c.wifi?.ssid||"";

  loadFiles();
}

async function save(){
  const p={
    ui:{
      Sleep_info:Sleep_info.value,
      Stat_screens:+Stat_screens.value,
      stat_screen:stat_screen.value,
      Board_Logo:+Board_Logo.value,
      Sail_Logo:+Sail_Logo.value
    },
    power:{ cal_bat:+cal_bat.value },
    logging:{
      logTXT:logTXT.checked,
      logUBX:logUBX.checked,
      logSBP:logSBP.checked,
      logGPY:logGPY.checked,
      logGPX:logGPX.checked
    },
    wifi:{
      ssid:ssid.value,
      password:password.value
    }
  };

  await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(p)});
  alert("Saved");
}

async function loadFiles(){
  fileList.innerHTML="";
  sdInfo.textContent="Scanning SD card…";
  const r=await fetch("/api/files");
  const j=await r.json();
  if(!j.ok){sdInfo.textContent="SD card not available";return;}
  sdInfo.textContent=`Free ${Math.floor(j.free_kb/1024)} MB`;

  j.files.forEach(f=>{
    const row=document.createElement("div");
    row.className="file-row";
    row.innerHTML=`
      <div>
        <div class=file-name>${f.name}</div>
        <div class=file-size>${(f.size/1024).toFixed(1)} KB</div>
      </div>
      <div class=file-actions>
        <button onclick="location='/api/file?name=${encodeURIComponent(f.name)}'">⬇</button>
        <button onclick="delFile('${f.name}')">🗑</button>
      </div>`;
    fileList.appendChild(row);
  });
}

async function delFile(name){
  if(!confirm("Delete "+name+"?"))return;
  await fetch("/api/file",{method:"DELETE",headers:{"Content-Type":"application/json"},body:JSON.stringify({name})});
  loadFiles();
}

load();
</script>

</body>
</html>
)rawliteral";
