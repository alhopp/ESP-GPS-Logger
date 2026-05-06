/* ============================================================================
 * Config (ESP32 / STA-only)
 * - Loads from /api/config
 * - Owns Settings UI + info modal
 * - Delegates System rendering to SystemTab
 * ========================================================================== */

const CONFIG_URL = "/api/config";
const PASSWORD_PLACEHOLDER = "********";

/* ---------------------------------------------------------------------------
 * UI helpers
 * ------------------------------------------------------------------------- */
function setVal(el,v){ if(el){ el._loading=true; el.value=v??""; el._loading=false; } }
function setChk(el,v){ if(el){ el._loading=true; el.checked=!!v; el._loading=false; } }
function setPasswordPlaceholder(el,hasPassword){
  if(!el) return;
  el._loading = true;
  el.value = hasPassword ? PASSWORD_PLACEHOLDER : "";
  el.dataset.passwordTouched = "0";
  el._loading = false;
}

/* ---------------------------------------------------------------------------
 * Load configuration
 * ------------------------------------------------------------------------- */
window.loadConfig = async function loadConfig(els){
  try{
    const r = await fetch(CONFIG_URL,{ cache:"no-store" });
    if(!r.ok) throw new Error("config fetch failed");

    const c = await r.json();

    setVal(els.Sleep_info1, c.ui?.Sleep_info1);
    setVal(els.Sleep_info2, c.ui?.Sleep_info2);


    /* ---------- Logging ---------- */
    setChk(els.logUBX, c.logging?.logUBX);
    setChk(els.logSBP, c.logging?.logSBP);

    /* ---------- Wi-Fi ---------- */
    if (c.wifi) {
      setVal(els.phone_ssid, c.wifi.phone_ssid);
      setPasswordPlaceholder(els.phone_pass, c.wifi.phone_pass_set);
    }


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
  const phonePass = els.phone_pass?.value ?? "";
  const payload={
    ui:{
      Sleep_info1: els.Sleep_info1?.value ?? "",
      Sleep_info2: els.Sleep_info2?.value ?? ""
      },
    logging:{
      logUBX:!!els.logUBX?.checked,
      logSBP:!!els.logSBP?.checked
    },
    wifi:{
      phone_ssid: els.phone_ssid?.value ?? ""
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

  const phonePassTouched = els.phone_pass?.dataset.passwordTouched === "1";
  if(phonePassTouched || (phonePass && phonePass !== PASSWORD_PLACEHOLDER)){
    payload.wifi.phone_pass = phonePass;
  }

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
