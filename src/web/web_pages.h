#pragma once
static const char PAGE_CONFIG_APP[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1"><title>ESP32 GPS</title>
<style>
:root{--a:#1e88e5;--b:#0b2a4a;--c:#f4f7fb;--d:#fff;--e:#dbe2ee;--f:#5b6b82}
*{box-sizing:border-box}body{margin:0;font:15px system-ui;background:var(--c)}
header{padding:14px;text-align:center;font-weight:600;background:var(--b);color:#fff}
nav{display:flex;background:#e9eff7;border-bottom:1px solid var(--e)}
nav button{flex:1;padding:12px;border:0;background:none;color:var(--f)}
nav button.a{color:var(--a);border-bottom:3px solid var(--a);background:#f8fbff}
section{display:none;padding:16px}section.a{display:block}
.card{background:var(--d);border:1px solid var(--e);border-radius:10px;padding:14px;margin-bottom:14px}
h3{margin:0 0 6px;font-size:14px;color:var(--a)}
label{display:block;margin-top:10px;font-size:13px;color:var(--f)}
input,select{width:100%;padding:10px;margin-top:4px;border:1px solid var(--e);border-radius:6px;font-size:16px}
footer{display:flex;gap:10px;padding:12px;border-top:1px solid var(--e);background:#f0f4fa}
footer button{flex:1;padding:12px;border-radius:8px;border:0;font-weight:600;background:var(--a);color:#fff}
.small{font-size:12px;color:var(--f);margin-top:6px}
</style></head><body>

<header>ESP32 GPS</header>

<nav>
<button class=a onclick="t('system',this)">System</button>
<button onclick="t('gps',this)">GPS</button>
<button onclick="t('power',this)">Power</button>
<button onclick="t('logging',this)">Logging</button>
<button onclick="t('ui',this)">UI</button>
<button onclick="t('wifi',this)">Wi-Fi</button>
</nav>

<section id=system class=a><div class=card>
<h3>System</h3>
<label>CPU</label><select id=cpu_freq><option>80<option>160<option>240</select>
<label>Timezone</label><input id=timezone type=number step=.5>
<label><input type=checkbox id=timezone_dst> DST</label>
</div></section>

<section id=gps><div class=card>
<h3>GPS</h3>
<label>Rate</label><select id=sample_rate><option>1<option>5<option>10</select>
<label>GNSS</label><select id=gnss><option value=1>GPS<option value=2>GPS+GLO<option value=3>GPS+GAL</select>
<label>Cal</label><input id=cal_speed type=number step=.01>
</div></section>

<section id=power><div class=card>
<h3>Power</h3>
<label>Shutdown</label><input id=shutdown_voltage type=number step=.1>
<label><input type=checkbox id=bat_choice> Battery %</label>
</div></section>

<section id=logging><div class=card>
<h3>Logging</h3>
<label>Distance</label><input id=track_distance type=number>
<label><input type=checkbox id=logTXT> TXT</label>
<label><input type=checkbox id=logUBX> UBX</label>
<label><input type=checkbox id=logSBP> SBP</label>
<label><input type=checkbox id=logGPY> GPY</label>
<label><input type=checkbox id=logGPX> GPX</label>
</div></section>

<section id=ui><div class=card>
<h3>UI</h3>
<label>Bar</label><input id=bar_length type=number>
<label><input type=checkbox id=speed_large_font> Large Speed</label>
<label>Sleep Code</label><input id=sleep_off_screen type=number>
</div></section>

<section id=wifi><div class=card>
<h3>Wi-Fi</h3>
<label>SSID</label><input id=ssid>
<label>Password</label><input id=password type=password>
<div class=small id=wifiInfo>Checking…</div>
</div></section>

<footer>
<button onclick=save()>Save</button>
<button id=wifiBtn onclick=connectWiFi()>Connect</button>
</footer>

<script>
const $=i=>document.getElementById(i);
function t(id,b){document.querySelectorAll("nav button,section").forEach(e=>e.classList.remove("a"));b.classList.add("a");$(id).classList.add("a")}
function set(e,v){e&&(e.type=="checkbox"?e.checked=!!v:e.value=v??"")}

async function load(){
const c=await(await fetch("/api/config")).json();
set(cpu_freq,c.system?.cpu_freq);set(timezone,c.system?.timezone);set(timezone_dst,c.system?.timezone_DST);
set(sample_rate,c.gps?.sample_rate);set(gnss,c.gps?.gnss);set(cal_speed,c.gps?.cal_speed);
set(shutdown_voltage,c.power?.shutdown_voltage);set(bat_choice,c.power?.bat_choice);
set(track_distance,c.logging?.track_distance);
["logTXT","logUBX","logSBP","logGPY","logGPX"].forEach(k=>set($(k),c.logging?.[k]));
set(bar_length,c.ui?.bar_length);set(speed_large_font,c.ui?.speed_large_font);set(sleep_off_screen,c.ui?.sleep_off_screen);
if(c.wifi?.ssid)ssid.value=c.wifi.ssid;
update();
}

async function save(){
const p={system:{cpu_freq:+cpu_freq.value,timezone:+timezone.value,timezone_DST:timezone_dst.checked},
gps:{sample_rate:+sample_rate.value,gnss:+gnss.value,cal_speed:+cal_speed.value},
power:{shutdown_voltage:+shutdown_voltage.value,bat_choice:bat_choice.checked},
logging:{track_distance:+track_distance.value},
ui:{bar_length:+bar_length.value,speed_large_font:speed_large_font.checked,sleep_off_screen:+sleep_off_screen.value},
wifi:{ssid:ssid.value}};
if(password.value)p.wifi.password=password.value;
await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(p)});
alert("Saved");
}

async function connectWiFi(){
wifiBtn.disabled=true;wifiInfo.textContent="Connecting…";
await fetch("/api/wifi/connect",{method:"POST"});
setTimeout(update,2000);
}

async function update(){
const s=await(await fetch("/api/netstatus")).json();
if(s.sta){
wifiInfo.textContent=`Connected ${s.ssid} ${s.ip}`;
wifiBtn.textContent="Connected";wifiBtn.disabled=true;
}else{
wifiInfo.textContent="Not connected";
wifiBtn.textContent="Connect";wifiBtn.disabled=false;
}
}

load();
</script></body></html>
)rawliteral";
