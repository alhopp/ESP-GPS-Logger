#pragma once
static const char PAGE_CONFIG_APP[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1"><title>ESP32 GPS</title>

<!-- ======================= STYLES ======================= -->
<style>
:root{--a:#1e88e5;--b:#0b2a4a;--c:#f4f7fb;--d:#fff;--e:#dbe2ee;--f:#5b6b82}
*{box-sizing:border-box}
body{margin:0;font:18px system-ui;background:var(--c);padding-bottom:96px}

/* Header + Nav */
header{padding:18px;font-size:22px;font-weight:700;text-align:center;background:var(--b);color:#fff}
nav{display:flex;background:#e9eff7;border-bottom:1px solid var(--e)}
nav button{flex:1;padding:18px;border:0;background:none;color:var(--f);font-size:19px;font-weight:600}
nav button.a{color:var(--a);border-bottom:3px solid var(--a);background:#f8fbff}

/* Sections */
section{display:none;padding:16px}
section.a{display:flex;flex-direction:column;height:calc(100vh - 112px);overflow:hidden}

/* Cards */
.card{background:var(--d);border:1px solid var(--e);border-radius:14px;padding:16px;margin-bottom:16px}
h3{margin:0 0 10px;font-size:18px;color:var(--a)}

/* Toggles + Inputs */
label{display:block;margin-top:10px;font-size:14px;color:var(--f)}
input,select{width:100%;padding:12px;margin-top:4px;border:1px solid var(--e);border-radius:10px;font-size:17px}
.toggle{display:flex;justify-content:space-between;align-items:center;margin-top:14px}
.toggle input{display:none}
.slider{width:52px;height:30px;background:#cfd6e4;border-radius:30px;position:relative}
.slider:before{content:"";position:absolute;width:26px;height:26px;left:2px;top:2px;background:#fff;border-radius:50%;transition:.2s}
.toggle input:checked+.slider{background:var(--a)}
.toggle input:checked+.slider:before{transform:translateX(22px)}

/* File list */
.file-list{flex:1;overflow-y:auto;-webkit-overflow-scrolling:touch;min-height:0}
.file-row{border-bottom:1px solid var(--e)}
.file-swipe{position:relative;overflow:hidden}

/* Delete layer */
.file-delete{
  position:absolute;right:0;top:0;bottom:0;width:72px;
  background:#fdecea;color:#c62828;
  display:flex;align-items:center;justify-content:center;
  font-size:22px;user-select:none
}

/* Swipe layer (covers full width!) */
.file-swipe-inner{
  width:100%;display:flex;align-items:center;
  background:var(--d);padding:12px 0;
  transition:transform .2s ease;
  transform:translateX(0);will-change:transform
}
.file-swipe.delete .file-swipe-inner{transform:translateX(-72px)}
.file-name{font-size:15px}
.file-size{font-size:13px;color:var(--f)}

/* System rows */
.sys-row{display:flex;justify-content:space-between;padding:12px 0;border-bottom:1px solid var(--e);font-size:16px}
.sys-row span:first-child{color:var(--f)}
.sys-row span:last-child{font-weight:600}

/* Footer */
footer{position:sticky;bottom:0;padding:16px;border-top:1px solid var(--e);background:#f0f4fa}
footer button{width:100%;padding:16px;border-radius:12px;border:0;font-size:18px;font-weight:600;background:var(--a);color:#fff}
footer button:disabled{opacity:.45}

/* Modal */
.modal{position:fixed;inset:0;background:rgba(0,0,0,.4);display:none;align-items:center;justify-content:center}
.modal-card{background:#fff;border-radius:14px;padding:20px;width:90%;max-width:420px}
</style>
</head><body>

<!-- ======================= UI ======================= -->
<header>ESP32 GPS</header>
<nav>
  <button class=a onclick="tab('files',this)">Files</button>
  <button onclick="tab('settings',this)">Settings</button>
  <button onclick="tab('system',this)">System</button>
</nav>

<section id=files class=a>
  <div class=card>
    <h3>Logs</h3>
    <div id=sdInfo>Scanning…</div>
    <div class=file-list id=fileList></div>
  </div>
</section>

<section id=settings>…</section>
<section id=system>…</section>

<footer><button id=saveBtn onclick=save() disabled>Save</button></footer>

<!-- ======================= SCRIPT ======================= -->
<script>
const $=i=>document.getElementById(i);
let dirty=false,swipeBound=false;

/* Tabs */
function tab(id,b){
  document.querySelectorAll("nav button,section").forEach(e=>e.classList.remove("a"));
  b.classList.add("a");$(id).classList.add("a");
}

/* Load files */
async function loadFiles(){
  fileList.innerHTML="";
  const r=await fetch("/api/files"),j=await r.json();
  if(!j.ok){sdInfo.textContent="No SD";return}
  sdInfo.textContent=`${j.files.length} files`;
  j.files.forEach(f=>{
    fileList.innerHTML+=`
    <div class="file-row file-swipe" data-name="${f.name}">
      <div class="file-delete">🗑</div>
      <div class="file-swipe-inner">
        <div>
          <div class="file-name">${f.name}</div>
          <div class="file-size">${(f.size/1024).toFixed(1)} KB</div>
        </div>
      </div>
    </div>`;
  });
}

/* Swipe-to-delete (delegated, iOS-safe) */
function enableSwipe(){
  if(swipeBound) return; swipeBound=true;
  let row,inner,x0,y0,dx=0,sw=false;

  fileList.addEventListener("touchstart",e=>{
    row=e.target.closest(".file-swipe"); if(!row) return;
    inner=row.querySelector(".file-swipe-inner");
    x0=e.touches[0].clientX; y0=e.touches[0].clientY;
    dx=0; sw=true; inner.style.transition="none";
  },{passive:true});

  fileList.addEventListener("touchmove",e=>{
    if(!sw) return;
    const x=e.touches[0].clientX,y=e.touches[0].clientY;
    const ddx=x-x0,ddy=y-y0;
    if(Math.abs(ddx)>Math.abs(ddy)+6){
      e.preventDefault();
      dx=Math.max(-72,Math.min(0,ddx));
      inner.style.transform=`translateX(${dx}px)`;
    }
  },{passive:false});

  fileList.addEventListener("touchend",()=>{
    if(!sw) return; sw=false; inner.style.transition="";
    dx<-36?row.classList.add("delete"):row.classList.remove("delete");
    inner.style.transform=row.classList.contains("delete")?"translateX(-72px)":"";
  },{passive:true});

  fileList.addEventListener("click",async e=>{
    const d=e.target.closest(".file-delete"); if(!d) return;
    const r=d.closest(".file-swipe");
    await fetch("/api/file",{method:"DELETE",headers:{"Content-Type":"application/json"},body:JSON.stringify({name:r.dataset.name})});
    loadFiles();
  });
}

/* Init */
loadFiles(); enableSwipe();
</script>
</body></html>
)rawliteral";
