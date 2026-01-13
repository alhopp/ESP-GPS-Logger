const $=i=>document.getElementById(i);
let swipeBound=false,dirty=false;

/* ---------------- Tabs ---------------- */
function tab(id,b){
  document.querySelectorAll("nav button,section").forEach(e=>e.classList.remove("a"));
  b.classList.add("a");$(id).classList.add("a");
}

/* ---------------- Dirty tracking ---------------- */
const markDirty=()=>{if(!dirty){dirty=true;saveBtn.disabled=false}};
document.addEventListener("input",e=>{
  if(e.target&&!e.target._loading)markDirty()
});

/* ---------------- Helpers ---------------- */
function setVal(el,v){
  if(!el)return;
  el._loading=true;
  el.value=v??"";
  el._loading=false;
}
function setChk(el,v){
  if(!el)return;
  el._loading=true;
  el.checked=!!v;
  el._loading=false;
}

/* ---------------- Files ---------------- */
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
        <div class="file-icon">📄</div>
        <div class="file-text">
          <div class="file-name">${f.name}</div>
          <div class="file-size">${(f.size/1024).toFixed(1)} KB</div>
        </div>
      </div>
    </div>`;
  });
}

/* ---------------- Swipe delete ---------------- */
function enableSwipe(){
  if(swipeBound)return; swipeBound=true;
  let row,inner,x0,y0,dx=0,sw=false;

  fileList.addEventListener("touchstart",e=>{
    row=e.target.closest(".file-swipe"); if(!row)return;
    inner=row.querySelector(".file-swipe-inner");
    x0=e.touches[0].clientX; y0=e.touches[0].clientY;
    dx=0; sw=true; inner.style.transition="none";
  },{passive:true});

  fileList.addEventListener("touchmove",e=>{
    if(!sw)return;
    const x=e.touches[0].clientX,y=e.touches[0].clientY;
    if(Math.abs(x-x0)>Math.abs(y-y0)+6){
      e.preventDefault();
      dx=Math.max(-72,Math.min(0,x-x0));
      inner.style.transform=`translateX(${dx}px)`;
    }
  },{passive:false});

  fileList.addEventListener("touchend",()=>{
    if(!sw)return; sw=false; inner.style.transition="";
    dx<-36?row.classList.add("delete"):row.classList.remove("delete");
    inner.style.transform=row.classList.contains("delete")?"translateX(-72px)":"";
  },{passive:true});

  fileList.addEventListener("click",async e=>{
    const d=e.target.closest(".file-delete"); if(!d)return;
    const r=d.closest(".file-swipe");
   await fetch("/api/file",{
      method:"DELETE",
      headers:{"Content-Type":"application/json"},
      body:JSON.stringify({name:r.dataset.name})
    });

    // smooth remove
    r.style.transition="height .2s,opacity .2s";
    r.style.opacity=0;
    r.style.height=0;
    setTimeout(()=>r.remove(),200);

    // update counter only
    const n=fileList.children.length;
    sdInfo.textContent=`${n-1} files`;

      });
    }

/* ---------------- Config load ---------------- */
async function loadConfig(){
  const c=await (await fetch("/api/config")).json();

  /* Settings */
  setVal(Sleep_info,c.ui?.Sleep_info);
  setVal(Board_Logo,c.ui?.Board_Logo);
  setVal(Sail_Logo,c.ui?.Sail_Logo);

  setChk(logTXT,c.logging?.logTXT);
  setChk(logUBX,c.logging?.logUBX);
  setChk(logSBP,c.logging?.logSBP);
  setChk(logGPY,c.logging?.logGPY);
  setChk(logGPX,c.logging?.logGPX);

  setVal(ssid,c.wifi?.ssid);

  /* System (read-only) */
  sys_gnss_module.textContent=c.system?.gnss_module||"-";
  sys_gnss.textContent=c.gps?.gnss||"-";
  sys_sample_rate.textContent=c.gps?.sample_rate||"-";
  sys_dynamic_model.textContent=c.gps?.dynamic_model||"-";
  sys_display.textContent=c.system?.display||"-";
  sys_storage.textContent=c.system?.storage_mb?c.system.storage_mb+" MB":"-";
  sys_version.textContent=c.system?.software_version||"-";

  saveBtn.disabled=true;
  dirty=false;
}

/* ---------------- Save ---------------- */
async function save(){
  const p={
    ui:{
      Sleep_info:Sleep_info.value,
      Board_Logo:+Board_Logo.value,
      Sail_Logo:+Sail_Logo.value
    },
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

  const r=await fetch("/api/config",{
    method:"POST",
    headers:{"Content-Type":"application/json"},
    body:JSON.stringify(p)
  });

  if(r.ok){
    saveBtn.disabled=true;
    dirty=false;
  }
}

addEventListener("load",()=>setTimeout(()=>{let s=document.getElementById("splash");if(!s)return;s.classList.add("hide");setTimeout(()=>s.remove(),500)},2000));


/* ---------------- Init ---------------- */
loadFiles();
enableSwipe();
loadConfig();
