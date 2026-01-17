// -----------------------------------------------------------------------------
// App shell
// -----------------------------------------------------------------------------
const $=i=>document.getElementById(i);
let dirty=false;   // REQUIRED

window.IS_LOCAL =
  location.hostname === "localhost" ||
  location.hostname === "127.0.0.1";
  
// -----------------------------------------------------------------------------
// Tab switching
// -----------------------------------------------------------------------------
window.tab=function(id,btn){
  document.querySelectorAll("nav button,section").forEach(e=>e.classList.remove("a"));
  btn.classList.add("a"); document.getElementById(id).classList.add("a");

if(id==="map" && window.MapView){
  requestAnimationFrame(()=>{
    requestAnimationFrame(()=>{
      MapView.init();
      MapView.map && MapView.map.invalidateSize();
    });
  });
}


};

// -----------------------------------------------------------------------------
// Dirty tracking
// -----------------------------------------------------------------------------
function markDirty(saveBtn){
  if(dirty) return;
  dirty=true; saveBtn&&(saveBtn.disabled=false);
}

addEventListener("load",()=>{
  const saveBtn=$("saveBtn");
  const onDirty=e=>{
    if(!e.target||e.target._loading) return;
    if(!e.target.matches("input,select,textarea")) return;
    markDirty(saveBtn);
  };
  document.addEventListener("input",onDirty);
  document.addEventListener("change",onDirty);
});

// -----------------------------------------------------------------------------
// App init
// -----------------------------------------------------------------------------
addEventListener("load",async()=>{
  const els={
    saveBtn:$("saveBtn"),
    fileList:$("fileList"),
    sdInfo:$("sdInfo"),
    Sleep_info:$("Sleep_info"),
    logTXT:$("logTXT"),
    logUBX:$("logUBX"),
    logSBP:$("logSBP"),
    ssid:$("ssid"),
    password:$("password")
  };

  els.saveBtn?.addEventListener("click",()=>saveConfig(els));

  await loadFiles(els.fileList,els.sdInfo);
  enableSwipe($("files"),els.fileList,els.sdInfo);

  await loadConfig(els);   // must delegate to SystemTab internally

  setTimeout(()=>{
    const s=$("splash"); if(!s) return;
    s.classList.add("hide");
    setTimeout(()=>s.remove(),500);
  },2000);
});
