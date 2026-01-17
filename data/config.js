/* ============================================================================
 * Config (PC + ESP compatible)
 * - Loads from config.json when local
 * - Loads from /api/config on device
 * - Owns Settings UI + info modal
 * - Delegates System rendering to SystemTab
 * ========================================================================== */



const IS_LOCAL =
  location.hostname === "localhost" ||
  location.hostname === "127.0.0.1";

const CONFIG_URL = IS_LOCAL ? "/config.json" : "/api/config";


/* ---------------------------------------------------------------------------
 * UI helpers (safe setters)
 * ------------------------------------------------------------------------- */
function setVal(el,v){
  if(!el) return;
  el._loading=true; el.value=v??""; el._loading=false;
}
function setChk(el,v){
  if(!el) return;
  el._loading=true; el.checked=!!v; el._loading=false;
}

/* ---------------------------------------------------------------------------
 * Load configuration
 * - Never throws
 * - Never blocks splash
 * ------------------------------------------------------------------------- */
window.loadConfig = async function loadConfig(els){
  try{
    const r = await fetch(CONFIG_URL,{ cache:"no-store" });
    if(!r.ok) throw new Error("config fetch failed");

    const c = await r.json();

    /* ---------- UI ---------- */
    setVal(els.Sleep_info, c.ui?.Sleep_info);

    /* ---------- Logging ---------- */
    setChk(els.logTXT, c.logging?.logTXT);
    setChk(els.logUBX, c.logging?.logUBX);
    setChk(els.logSBP, c.logging?.logSBP);

    /* ---------- Wi-Fi ---------- */
    setVal(els.ssid,     c.wifi?.ssid);
    setVal(els.password, c.wifi?.password);

    /* ---------- Performance / Stats ---------- */
    setChk(els.stat_2s,       c.stats?.s2);
    setChk(els.stat_5x10,     c.stats?.s10);
    setChk(els.stat_alpha,    c.stats?.alpha);
    setChk(els.stat_nm,       c.stats?.nm);
    setChk(els.stat_hour,     c.stats?.h1);
    setChk(els.stat_distance, c.stats?.distance);

    /* ---------- System (handoff) ---------- */
    window.SystemTab?.load(c.system);

  }catch(err){
    console.warn("Config unavailable – using defaults", err);

    /* Minimal safe defaults */
    setVal(els.Sleep_info,"");
    setChk(els.logTXT,false);
    setChk(els.logUBX,false);
    setChk(els.logSBP,false);
  }

  els.saveBtn && (els.saveBtn.disabled=true, dirty=false);
};

/* ---------------------------------------------------------------------------
 * Save configuration (device only)
 * ------------------------------------------------------------------------- */
window.saveConfig = async function saveConfig(els){
  if(IS_LOCAL){
    console.warn("saveConfig skipped (local mode)");
    els.saveBtn && (els.saveBtn.disabled=true, dirty=false);
    return;
  }

  const payload={
    ui:{ Sleep_info: els.Sleep_info?.value ?? "" },

    logging:{
      logTXT:!!els.logTXT?.checked,
      logUBX:!!els.logUBX?.checked,
      logSBP:!!els.logSBP?.checked
    },

    wifi:{
      ssid:els.ssid?.value ?? "",
      ...(els.password?.value ? { password:els.password.value } : {})
    },

    stats:{
      s2:!!els.stat_2s?.checked,
      s10:!!els.stat_5x10?.checked,
      alpha:!!els.stat_alpha?.checked,
      nm:!!els.stat_nm?.checked,
      h1:!!els.stat_hour?.checked,
      distance:!!els.stat_distance?.checked
    }
  };

  const r = await fetch("/api/config",{
    method:"POST",
    headers:{ "Content-Type":"application/json" },
    body:JSON.stringify(payload)
  });

  r.ok && (els.saveBtn.disabled=true, dirty=false);
};

/* ---------------------------------------------------------------------------
 * Settings info modal (ⓘ buttons)
 * ------------------------------------------------------------------------- */
const infoTexts={
  performance:"Toggle performance screen types for session analysis.",
  wifi:"Configure Wi-Fi credentials used in config mode.",
  logging:"Select raw GNSS formats to log (UBX / SBP).",
  sleep:"Text shown on device screen while sleeping."
};

window.showInfoModal=function(key){
  const m=document.getElementById("infoModal"); if(!m) return;
  document.getElementById("infoTitle").textContent=
    key.charAt(0).toUpperCase()+key.slice(1)+" Info";
  document.getElementById("infoText").textContent=
    infoTexts[key]||"No information available.";
  m.classList.add("show");
};

window.closeInfo=function(){
  document.getElementById("infoModal")?.classList.remove("show");
};

document.addEventListener("click",e=>{
  const s=document.getElementById("settings");
  if(!s||!s.contains(e.target)) return;
  const b=e.target.closest(".info-btn");
  b&&b.dataset.info&&window.showInfoModal(b.dataset.info);
});

addEventListener("load",()=>{
  const m=document.getElementById("infoModal"); if(!m) return;
  m.addEventListener("click",e=>{
    !e.target.closest(".modal-card")&&window.closeInfo();
  });
});
