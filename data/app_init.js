// -----------------------------------------------------------------------------
// App shell
// -----------------------------------------------------------------------------
const $ = i => document.getElementById(i);
let dirty = false;   // REQUIRED

// -----------------------------------------------------------------------------
// Environment detection (MUST come first)
// -----------------------------------------------------------------------------
window.IS_LOCAL =
  location.hostname === "localhost" ||
  location.hostname === "127.0.0.1";
// -----------------------------------------------------------------------------
// Local API shim (PC dev only)
// -----------------------------------------------------------------------------
if (window.IS_LOCAL) {
  const _fetch = window.fetch;

  window.fetch = async (url, opts) => {

    // ---------------------------------------------------------
    // Fake /api/files  → mirrors ESP response
    // ---------------------------------------------------------
    if (url === "/api/files") {
      return new Response(JSON.stringify({
        ok: true,
        files: [
          { name: "test_track1.geojson",  size: 1234 },
          { name: "test_track2.geojson",  size: 1234 },
          { name: "test_track3.geojson",  size: 1234 }
          //{ name: "test_track4.geojson",  size: 1234 },
         // { name: "test_track5.geojson",  size: 1234 },
         // { name: "test_track6.geojson",  size: 1234 },
         // { name: "test_track7.geojson",  size: 1234 },
         // { name: "test_track8.geojson",  size: 1234 },
         // { name: "test_track9.geojson",  size: 1234 },
         // { name: "test_track10.geojson", size: 1234 }
        ]
      }), {
        headers: { "Content-Type": "application/json" }
      });
    }

    // ---------------------------------------------------------
    // Fake /api/download → serve GeoJSON from /logs/
    // ---------------------------------------------------------
    if (url.startsWith("/api/download")) {
      const p = new URLSearchParams(url.split("?")[1]);
      const file = p.get("file");
      return _fetch(`/logs/${file}`, opts);
    }

    return _fetch(url, opts);
  };
}




  function splashReady(){
  requestAnimationFrame(()=>{
    document.getElementById("splash")?.classList.add("ready");
  });
}

/// -----------------------------------------------------------------------------
// Tab switching
// -----------------------------------------------------------------------------
window.tab = function(id, btn){
  document.querySelectorAll("nav button,section")
    .forEach(e => e.classList.remove("a"));

  btn.classList.add("a");
  const section = document.getElementById(id);
  section.classList.add("a");

  if(id === "map" && window.MapView){
    // 🔑 Wait for iOS layout + paint
    requestAnimationFrame(()=>{
      requestAnimationFrame(()=>{
        MapView.init();

        // 🔑 FORCE iOS to acknowledge size
        setTimeout(()=>{
          MapView.map && MapView.map.invalidateSize(true);
        }, 100);
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
addEventListener("load", async ()=>{

  const els={
    saveBtn:$("saveBtn"),
    fileList:$("fileList"),
    sdInfo:$("sdInfo"),

    // UI
    Sleep_info1:$("Sleep_info1"),
    Sleep_info2:$("Sleep_info2"),
   
    // Logging
    logUBX:$("logUBX"),
    logSBP:$("logSBP"),

    // Wi-Fi
    phone_ssid: $("phone_ssid"),
    phone_pass: $("phone_pass"),

    // Performance / Stats
    stat_2s:$("stat_2s"),
    stat_5x10:$("stat_5x10"),
    stat_alpha:$("stat_alpha"),
    stat_nm:$("stat_nm"),
    stat_hour:$("stat_hour"),
    stat_distance:$("stat_distance")
  };

  els.saveBtn?.addEventListener("click",()=>saveConfig(els));
  els.phone_pass?.addEventListener("focus",()=>{
    if(els.phone_pass.value === "********"){
      els.phone_pass.value = "";
      els.phone_pass.dataset.editingPlaceholder = "1";
    }
  });
  els.phone_pass?.addEventListener("input",()=>{
    els.phone_pass.dataset.passwordTouched = "1";
  });
  els.phone_pass?.addEventListener("blur",()=>{
    if(els.phone_pass.dataset.editingPlaceholder === "1" &&
       els.phone_pass.dataset.passwordTouched !== "1" &&
       !els.phone_pass.value){
      els.phone_pass.value = "********";
    }
    els.phone_pass.dataset.editingPlaceholder = "0";
  });

  // ---------------------------------------------------------------------------
  // Splash: fade logo in once decoded
  // ---------------------------------------------------------------------------
  const splash=$("splash");
  const img=splash?.querySelector("img");

  if(img?.decode){
    img.decode().then(splashReady).catch(splashReady);
  }else{
    splashReady();
  }

  // ---------------------------------------------------------------------------
  // App startup
  // ---------------------------------------------------------------------------
  await loadFiles(els.fileList,els.sdInfo);
  enableSwipe($("files"),els.fileList,els.sdInfo);

  await loadConfig(els);

  // Activate default tab (Map) on first load
  const mapBtn = document.querySelector("nav button");
  if(mapBtn) tab("map", mapBtn);



  // ---------------------------------------------------------------------------
  // Splash: fade out + remove
  // ---------------------------------------------------------------------------
  setTimeout(()=>{
    if(!splash) return;
    splash.classList.add("hide");
    setTimeout(()=>splash.remove(),220);
  },650);
});
