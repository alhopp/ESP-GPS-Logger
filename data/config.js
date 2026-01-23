/* ============================================================================
 * Config (ESP32 / STA-only)
 * - Loads from /api/config
 * - Owns Settings UI + info modal
 * - Delegates System rendering to SystemTab
 * ========================================================================== */

const CONFIG_URL = "/api/config";

/* ---------------------------------------------------------------------------
 * UI helpers
 * ------------------------------------------------------------------------- */
function setVal(el,v){ if(el){ el._loading=true; el.value=v??""; el._loading=false; } }
function setChk(el,v){ if(el){ el._loading=true; el.checked=!!v; el._loading=false; } }

/* ---------------------------------------------------------------------------
 * Load configuration
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

    /* ---------- Performance / Stats ---------- */
    setChk(els.stat_2s,       c.stats?.s2);
    setChk(els.stat_5x10,     c.stats?.s10);
    setChk(els.stat_alpha,    c.stats?.alpha);
    setChk(els.stat_nm,       c.stats?.nm);
    setChk(els.stat_hour,     c.stats?.h1);
    setChk(els.stat_distance, c.stats?.distance);

    /* ---------- System ---------- */
    if(c.system) window.SystemTab?.load(c.system);
    else console.warn("No system object in /api/config");

  }catch(err){
    console.error("Config load failed", err);
  }

  els.saveBtn && (els.saveBtn.disabled=true, dirty=false);
};

/* ---------------------------------------------------------------------------
 * Save configuration
 * ------------------------------------------------------------------------- */
window.saveConfig = async function saveConfig(els){
  const payload={
    ui:{ Sleep_info: els.Sleep_info?.value ?? "" },
    logging:{
      logTXT:!!els.logTXT?.checked,
      logUBX:!!els.logUBX?.checked,
      logSBP:!!els.logSBP?.checked
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

  const r = await fetch(CONFIG_URL,{
    method:"POST",
    headers:{ "Content-Type":"application/json" },
    body:JSON.stringify(payload)
  });

  r.ok
    ? (els.saveBtn.disabled=true, dirty=false)
    : console.warn("Config save failed");
};

/* ---------------------------------------------------------------------------
 * Settings info modal
 * ------------------------------------------------------------------------- */
const infoTexts={
  performance:"Toggle performance screen types for session analysis.",
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
  const m=document.getElementById("infoModal");
  m&&m.addEventListener("click",e=>{
    !e.target.closest(".modal-card")&&window.closeInfo();
  });
});
