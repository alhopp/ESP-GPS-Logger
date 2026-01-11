
#pragma once
static const char PAGE_CONFIG_APP[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1"><title>ESP32 GPS</title>
<style>
:root{--a:#1e88e5;--b:#0b2a4a;--c:#f4f7fb;--d:#fff;--e:#dbe2ee;--f:#5b6b82}
*{box-sizing:border-box}body{margin:0;font:18px system-ui;background:var(--c)}
header{padding:18px;font-size:22px;font-weight:700;text-align:center;background:var(--b);color:#fff}
nav{display:flex;background:#e9eff7;border-bottom:1px solid var(--e)}
nav button{flex:1;padding:18px;border:0;background:none;color:var(--f);font-size:19px;font-weight:600}
nav button.a{color:var(--a);border-bottom:3px solid var(--a);background:#f8fbff}
section{display:none;padding:16px}section.a{display:block}
.card{background:var(--d);border:1px solid var(--e);border-radius:14px;padding:16px;margin-bottom:16px}
h3{margin:0 0 10px;font-size:18px;color:var(--a)}
label{display:block;margin-top:10px;font-size:14px;color:var(--f)}
input,select{width:100%;padding:12px;margin-top:4px;border:1px solid var(--e);border-radius:10px;font-size:17px}
.toggle{display:flex;justify-content:space-between;align-items:center;margin-top:14px}
.toggle input{display:none}
.slider{width:52px;height:30px;background:#cfd6e4;border-radius:30px;position:relative}
.slider:before{content:"";position:absolute;width:26px;height:26px;left:2px;top:2px;background:#fff;border-radius:50%;transition:.2s}
.toggle input:checked+.slider{background:var(--a)}
.toggle input:checked+.slider:before{transform:translateX(22px)}
.file-list{max-height:60vh;overflow-y:auto}
.file-row{display:flex;justify-content:space-between;padding:12px 0;border-bottom:1px solid var(--e)}
.file-name{font-size:15px}.file-size{font-size:13px;color:var(--f)}
.sys-row{display:flex;justify-content:space-between;padding:12px 0;border-bottom:1px solid var(--e);font-size:16px}
.sys-row span:first-child{color:var(--f)}.sys-row span:last-child{font-weight:600}
footer{padding:16px;border-top:1px solid var(--e);background:#f0f4fa}
footer button{width:100%;padding:16px;border-radius:12px;border:0;font-size:18px;font-weight:600;background:var(--a);color:#fff}
.info{margin-left:10px;font-size:22px;line-height:1;padding:6px;cursor:pointer;color:var(--f);user-select:none}
.modal{position:fixed;inset:0;background:rgba(0,0,0,.4);display:none;align-items:center;justify-content:center;z-index:1000}
.modal-card{background:#fff;border-radius:14px;padding:20px;max-width:420px;width:90%;font-size:15px}
.modal-card ul{padding-left:18px}.modal-card button{margin-top:14px;width:100%;padding:12px;border:0;border-radius:10px;background:var(--a);color:#fff;font-size:16px}
.file-actions{display:flex;gap:12px}
.file-btn{width:44px;height:44px;display:flex;align-items:center;justify-content:center;border-radius:10px;background:var(--e);font-size:22px;cursor:pointer;user-select:none}
.file-btn.download{background:#e3f2fd;color:#1e88e5}
.file-btn.delete{background:#fdecea;color:#c62828}
</style></head><body>

<header>ESP32 GPS</header>
<nav><button class=a onclick="tab('files',this)">Files</button><button onclick="tab('settings',this)">Settings</button><button onclick="tab('system',this)">System</button></nav>

<section id=files class=a><div class=card><h3>Logs</h3><div id=sdInfo>Scanning…</div><div class=file-list id=fileList></div></div></section>

<section id=settings>
<div class=card><h3>User</h3><label>Name</label><input id=Sleep_info></div>
<div class=card><h3>Screens<span class=info onclick="openInfo()">ⓘ</span></h3>
<div class=toggle><span>10 Sec</span><label><input type=checkbox id=stat_10s><div class=slider></div></label></div>
<div class=toggle><span>Alpha</span><label><input type=checkbox id=stat_alpha><div class=slider></div></label></div>
<div class=toggle><span>Nautical Mile</span><label><input type=checkbox id=stat_nm><div class=slider></div></label></div>
<div class=toggle><span>One Hour</span><label><input type=checkbox id=stat_hour><div class=slider></div></label></div>
<div class=toggle><span>Distance</span><label><input type=checkbox id=stat_distance><div class=slider></div></label></div>
</div>

<div class=card><h3>Logos</h3>
<label>Board Logo</label>
<select id=Board_Logo><option value=0>Off</option><option value=1>Generic</option><option value=2>Slalom</option><option value=3>Foil</option><option value=4>Wave</option><option value=5>Speed</option></select>
<label>Sail Logo</label>
<select id=Sail_Logo><option value=0>Off</option><option value=1>Neutral</option><option value=2>Race</option><option value=3>Wave</option><option value=4>Foil</option></select>
</div>

<div class=card><h3>Logging</h3>
<div class=toggle><span>TXT</span><label><input type=checkbox id=logTXT><div class=slider></div></label></div>
<div class=toggle><span>UBX</span><label><input type=checkbox id=logUBX><div class=slider></div></label></div>
<div class=toggle><span>SBP</span><label><input type=checkbox id=logSBP><div class=slider></div></label></div>
<div class=toggle><span>GPY</span><label><input type=checkbox id=logGPY><div class=slider></div></label></div>
<div class=toggle><span>GPX</span><label><input type=checkbox id=logGPX><div class=slider></div></label></div>
</div>

<div class=card><h3>Wi-Fi</h3><label>SSID</label><input id=ssid><label>Password</label><input id=password type=password></div>
</section>

<section id=system>
<div class=card><h3>GNSS</h3>
<div class=sys-row><span>Module</span><span id=sys_gnss_module></span></div>
<div class=sys-row><span>Constellation</span><span id=sys_gnss></span></div>
<div class=sys-row><span>Sample Rate</span><span id=sys_sample_rate></span></div>
<div class=sys-row><span>Dynamic Model</span><span id=sys_dynamic_model></span></div>
</div>
<div class=card><h3>Hardware</h3>
<div class=sys-row><span>Display</span><span id=sys_display></span></div>
<div class=sys-row><span>Storage</span><span id=sys_storage></span></div>
</div>
<div class=card><h3>Software</h3><div class=sys-row><span>Version</span><span id=sys_version></span></div></div>
</section>

<footer id=footer><button onclick=save()>Save</button></footer>

<div id=infoModal class=modal><div class=modal-card>
<h3>Screens explained</h3>
<p>These screens control which performance data will be displayed on the device.</p>
<ul>
<li><b>10 Sec</b> - Best 10-second average speed</li>
<li><b>Alpha</b> - Alpha racing result (gybe performance)</li>
<li><b>Nautical Mile</b> - Best nautical mile run</li>
<li><b>One Hour</b> - Best 1-hour average speed</li>
<li><b>Distance</b> - Total distance sailed</li>
</ul>
<button onclick="closeInfo()">Close</button>
</div></div>

<script>
const $=i=>document.getElementById(i);
function tab(id,b){document.querySelectorAll("nav button,section").forEach(e=>e.classList.remove("a"));b.classList.add("a");$(id).classList.add("a");footer.style.display=id==="system"?"none":"block"}
function set(e,v){if(!e)return;e.type==="checkbox"?e.checked=!!v:e.value=v??""}
function openInfo(){$("infoModal").style.display="flex"}
function closeInfo(){$("infoModal").style.display="none"}
function downloadFile(n){window.location="/api/file?name="+encodeURIComponent(n)}
async function deleteFile(n){if(!confirm("Delete "+n+"?"))return;await fetch("/api/file",{method:"DELETE",headers:{"Content-Type":"application/json"},body:JSON.stringify({name:n})});loadFiles()}
async function load(){
const c=await(await fetch("/api/config")).json();
set(Sleep_info,c.ui?.Sleep_info);set(Board_Logo,c.ui?.Board_Logo);set(Sail_Logo,c.ui?.Sail_Logo);
set(logTXT,c.logging?.logTXT);set(logUBX,c.logging?.logUBX);set(logSBP,c.logging?.logSBP);set(logGPY,c.logging?.logGPY);set(logGPX,c.logging?.logGPX);
$("sys_gnss_module").textContent=c.system?.gnss_module||"-";
$("sys_gnss").textContent=c.gps?.gnss||"-";
$("sys_sample_rate").textContent=c.gps?.sample_rate||"-";
$("sys_dynamic_model").textContent=c.gps?.dynamic_model||"-";
$("sys_display").textContent=c.system?.display||"-";
$("sys_storage").textContent=c.system?.storage_mb?c.system.storage_mb+" MB":"-";
$("sys_version").textContent=c.system?.software_version||"-";
ssid.value=c.wifi?.ssid||"";loadFiles()}
async function save(){
const p={
  ui:{Sleep_info:Sleep_info.value,Board_Logo:+Board_Logo.value,Sail_Logo:+Sail_Logo.value},
  logging:{logTXT:logTXT.checked,logUBX:logUBX.checked,logSBP:logSBP.checked,logGPY:logGPY.checked,logGPX:logGPX.checked},
  wifi:{ssid:ssid.value,password:password.value}
};
const r=await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(p)});
alert(r.ok?"Saved":"Save failed")}
async function loadFiles(){
fileList.innerHTML="";
const r=await fetch("/api/files"),j=await r.json();
if(!j.ok){sdInfo.textContent="No SD";return}
sdInfo.textContent=`Free ${Math.floor(j.free_kb/1024)} MB`;
j.files.forEach(f=>fileList.innerHTML+=`<div class=file-row><div><div class=file-name>${f.name}</div><div class=file-size>${(f.size/1024).toFixed(1)} KB</div></div><div class=file-actions><div class="file-btn download" onclick="downloadFile('${f.name}')">⬇️</div><div class="file-btn delete" onclick="deleteFile('${f.name}')">🗑</div></div></div>`)}
load();
</script>
</body></html>
)rawliteral";
