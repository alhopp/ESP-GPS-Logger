


// -----------------------------------------------------------------------------
// UI helpers (safe setters)
// -----------------------------------------------------------------------------
function setVal(el,v){ if(el){ el._loading=true; el.value=v??""; el._loading=false; } }
function setChk(el,v){ if(el){ el._loading=true; el.checked=!!v; el._loading=false; } }

// -----------------------------------------------------------------------------
// Config API – load
// - Fetches full config blob from device
// - Populates editable Settings UI only
// - System rendering delegated elsewhere
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Load full device configuration
// - Populates all UI controls
// - Populates read-only System fields
// -----------------------------------------------------------------------------
async function loadConfig(els){
  try{
    const r = await fetch("/api/config",{cache:"no-store"});
    if(!r.ok) throw new Error("config api missing");

    const c = await r.json();

    // ---------------- UI ----------------
    setVal(els.Sleep_info, c.ui?.Sleep_info);

    // ---------------- Logging ----------------
    setChk(els.logTXT, c.logging?.logTXT);
    setChk(els.logUBX, c.logging?.logUBX);
    setChk(els.logSBP, c.logging?.logSBP);

    // ---------------- Wi-Fi ----------------
    setVal(els.ssid,     c.wifi?.ssid);
    setVal(els.password, c.wifi?.password);

    // ---------------- Performance / Stats ----------------
    setChk(els.stat_2s,       c.stats?.s2);
    setChk(els.stat_5x10,     c.stats?.s10);
    setChk(els.stat_alpha,    c.stats?.alpha);
    setChk(els.stat_nm,       c.stats?.nm);
    setChk(els.stat_hour,     c.stats?.h1);
    setChk(els.stat_distance,c.stats?.distance);

    // ---------------- System (handoff) ----------------
    window.SystemTab?.load(c.system);

  }catch(err){
    // ✅ LOCAL / OFFLINE SAFE PATH
    console.warn("Config API unavailable – using defaults");

    setVal(els.Sleep_info,"");
    setChk(els.logTXT,false);
    setChk(els.logUBX,false);
    setChk(els.logSBP,false);
  }

  // ---------------- State ----------------
  els.saveBtn && (els.saveBtn.disabled=true, dirty=false);
}



// -----------------------------------------------------------------------------
// Save configuration
// - Sends ONLY mutable fields
// - System values are owned by firmware
// -----------------------------------------------------------------------------
async function saveConfig(els){
  const p = {
    ui:{
      Sleep_info: els.Sleep_info?.value ?? ""
    },

    logging:{
      logTXT: !!els.logTXT?.checked,
      logUBX: !!els.logUBX?.checked,
      logSBP: !!els.logSBP?.checked
    },

    wifi:{
      ssid: els.ssid?.value ?? "",
      ...(els.password?.value ? { password: els.password.value } : {})
    },

    stats:{
      s2:       !!els.stat_2s?.checked,
      s10:      !!els.stat_5x10?.checked,
      alpha:    !!els.stat_alpha?.checked,
      nm:       !!els.stat_nm?.checked,
      h1:       !!els.stat_hour?.checked,
      distance: !!els.stat_distance?.checked
    }
  };

  const r = await fetch("/api/config",{
    method:"POST",
    headers:{ "Content-Type":"application/json" },
    body:JSON.stringify(p)
  });

  r.ok && (els.saveBtn.disabled=true, dirty=false);
}


// -----------------------------------------------------------------------------
// Settings info modal (ⓘ buttons)
// - Centralised help text for config UI
// - Event delegation (no per-button listeners)
// - Globals required for inline HTML hooks
// -----------------------------------------------------------------------------
const infoTexts={
  performance:"Toggle performance screen types for session analysis. Each option logs additional calculated data.",
  wifi:"Enter the SSID and optional password of the Wi-Fi network you'd like the device to connect to in config mode.",
  logging:"Choose which formats of raw GNSS data to log. UBX and SBP are binary protocols from different chipsets.",
  sleep:"This text is shown on the device screen while sleeping to help identify it."
};

// Show info modal for a given key
window.showInfoModal=function(key){
  const m=document.getElementById("infoModal"); if(!m) return;
  document.getElementById("infoTitle").textContent=
    key.charAt(0).toUpperCase()+key.slice(1)+" Info";
  document.getElementById("infoText").textContent=
    infoTexts[key]||"No information available.";
  m.classList.add("show");
};

// Close info modal
window.closeInfo=function(){
  document.getElementById("infoModal")?.classList.remove("show");
};

// ⓘ click handler (scoped to Settings tab)
document.addEventListener("click",e=>{
  const s=document.getElementById("settings");
  if(!s||!s.contains(e.target)) return;
  const b=e.target.closest(".info-btn");
  b&&b.dataset.info&&window.showInfoModal(b.dataset.info);
});

// Click outside modal card closes it
addEventListener("load",()=>{
  const m=document.getElementById("infoModal"); if(!m) return;
  m.addEventListener("click",e=>{
    !e.target.closest(".modal-card")&&window.closeInfo();
  });
});


