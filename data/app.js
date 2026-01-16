const $ = i => document.getElementById(i);

// ---------------- Dirty tracking (authoritative) ----------------
let dirty = false;
let swipeBound = false;

function markDirty(saveBtn){
  if (dirty) return;
  dirty = true;
  if (saveBtn) saveBtn.disabled = false;
}

addEventListener("load",()=>{
  const saveBtn = $("saveBtn");

  const onDirty = e=>{
    if (!e.target || e.target._loading) return;
    if (!(e.target.matches("input,select,textarea"))) return;
    markDirty(saveBtn);
  };

  // Text inputs, selects, sliders
  document.addEventListener("input", onDirty);

  // Checkboxes + mobile Safari/WebView
  document.addEventListener("change", onDirty);
});


/* ---------------- Tabs ---------------- */
function tab(id, btn){
  document.querySelectorAll("nav button, section")
    .forEach(e => e.classList.remove("a"));

  btn.classList.add("a");
  document.getElementById(id).classList.add("a");

  if (id === "map" && window.MapView) {
    MapView.init();
  }
}




/* ---------------- Helpers ---------------- */
function setVal(el,v){ if (!el) return; el._loading=true; el.value=v??""; el._loading=false; }
function setChk(el,v){ if (!el) return; el._loading=true; el.checked=!!v; el._loading=false; }
function setText(el,v){ if (!el) return; el.textContent=(v!==undefined&&v!==null&&v!=="")?v:"-"; }

/* ---------------- Files ---------------- */
async function loadFiles(fileList, sdInfo){
  if (!fileList || !sdInfo) return;

  fileList.innerHTML="";
  const r=await fetch("/api/files",{cache:"no-store"});
  const j=await r.json();

  if (!j.ok){ sdInfo.textContent="No SD"; return; }

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

/* ---------------- Swipe delete + download ---------------- */
function enableSwipe(container, fileList, sdInfo){
  if (swipeBound || !container || !fileList || !sdInfo) return;
  swipeBound = true;

  let row, icon, x0, y0, dx = 0, sw = false, moved = false;
  let tapCandidate = null, tapStartOnDelete = false;

  function triggerDownload(name){
    if (!name) return;
    window.location.href =
      `/api/download?file=${encodeURIComponent(name)}&t=${Date.now()}`;
  }

  // ---------------- Touch start ----------------
  container.addEventListener("touchstart", e=>{
    row = e.target.closest(".file-swipe");
    if (!row) return;

    tapCandidate = row;
    tapStartOnDelete = !!e.target.closest(".file-delete");

    icon = row.querySelector(".file-icon");
    x0 = e.touches[0].clientX;
    y0 = e.touches[0].clientY;

    dx = 0; sw = true; moved = false;
    if (icon) icon.style.transition = "none";
  }, { passive:true });

  // ---------------- Touch move ----------------
  container.addEventListener("touchmove", e=>{
    if (!sw) return;

    const x = e.touches[0].clientX;
    const y = e.touches[0].clientY;
    const adx = Math.abs(x - x0);
    const ady = Math.abs(y - y0);

    if (adx > ady + 8 && adx > 12) {
      moved = true;
      e.preventDefault();

      dx = Math.max(-72, Math.min(0, x - x0));
      if (icon) {
        icon.style.transform = `translateX(${dx}px)`;
        icon.style.opacity   = 1 + dx / 72;
      }
    }
  }, { passive:false });

  // ---------------- Touch end ----------------
  container.addEventListener("touchend", ()=>{
    if (!sw) return;
    sw = false;

    if (icon) icon.style.transition = "";

    if (dx < -36) {
      row.classList.add("delete");
      if (icon) {
        icon.style.transform = "translateX(-72px)";
        icon.style.opacity = 0;
      }
    } else {
      row.classList.remove("delete");
      if (icon) {
        icon.style.transform = "";
        icon.style.opacity = "";
      }
    }

    if (
      tapCandidate &&
      !moved &&
      !tapStartOnDelete &&
      !tapCandidate.classList.contains("delete")
    ) {
      triggerDownload(tapCandidate.dataset.name);
    }

    tapCandidate = null;
    tapStartOnDelete = false;
    moved = false;
  }, { passive:true });

  // ---------------- Click download ----------------
  container.addEventListener("click", e=>{
    const r = e.target.closest(".file-swipe");
    if (!r || e.target.closest(".file-delete") || r.classList.contains("delete"))
      return;
    triggerDownload(r.dataset.name);
  });

  // ---------------- Click delete ----------------
  container.addEventListener("click", async e=>{
    let d = e.target;
    if (!d.classList || !d.classList.contains("file-delete"))
      d = d.closest && d.closest(".file-delete");
    if (!d) return;

    const r = d.closest(".file-swipe");
    if (!r) return;

    await fetch("/api/file",{
      method:"DELETE",
      headers:{ "Content-Type":"application/json" },
      body:JSON.stringify({ name:r.dataset.name })
    });

    r.style.transition = "height .2s,opacity .2s";
    r.style.opacity = 0;
    r.style.height = 0;
    setTimeout(()=>r.remove(),200);

    sdInfo.textContent = `${fileList.children.length - 1} files`;
  });
}


/* ---------------- Config load ---------------- */
async function loadConfig(els){
  const r=await fetch("/api/config",{cache:"no-store"});
  const c=await r.json();

  setVal(els.Sleep_info,c.ui?.Sleep_info);

  setChk(els.logTXT,c.logging?.logTXT);
  setChk(els.logUBX,c.logging?.logUBX);
  setChk(els.logSBP,c.logging?.logSBP);


  setVal(els.ssid,c.wifi?.ssid);
  setVal(els.password, c.wifi?.password);

  setText(els.sys_gnss_module,   c.system?.gnss_module);
  setText(els.sys_gnss,          c.system?.gnss_mode);
  setText(els.sys_sample_rate,  c.system?.sample_rate ? (c.system.sample_rate+" Hz") : null);
  setText(els.sys_dynamic_model,c.system?.dynamic_model);

  setText(els.sys_speed_units,  c.system?.speed_units);
  setText(els.sys_cal_speed,    c.system?.cal_speed != null ? c.system.cal_speed : null);

  setText(els.sys_display,      c.system?.display);
  setText(els.sys_storage,      c.system?.storage_mb ? (c.system.storage_mb+" MB") : null);
  setText(els.sys_cpu_freq,     c.system?.cpu_freq ? (c.system.cpu_freq+" MHz") : null);

  setText(els.sys_version,      c.system?.software_version);

  // performance stats
  setChk(els.stat_2s,        c.stats?.s2);
  setChk(els.stat_5x10,      c.stats?.s10);
  setChk(els.stat_alpha,     c.stats?.alpha);
  setChk(els.stat_nm,        c.stats?.nm);
  setChk(els.stat_hour,      c.stats?.h1);
  setChk(els.stat_distance,  c.stats?.distance);



  if(els.saveBtn) els.saveBtn.disabled=true; dirty=false;
}

/* ---------------- Save ---------------- */
async function save(els){
  const p={
    ui:{Sleep_info:els.Sleep_info?.value??""},

    logging:{
      logTXT:!!els.logTXT?.checked,
      logUBX:!!els.logUBX?.checked,
      logSBP:!!els.logSBP?.checked
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

  const r=await fetch("/api/config",{
    method:"POST",
    headers:{"Content-Type":"application/json"},
    body:JSON.stringify(p)
  });

  if(r.ok){
    if(els.saveBtn) els.saveBtn.disabled=true;
    dirty=false;
  }
}


/* ---------------- Splash ---------------- */
addEventListener("load",()=>setTimeout(()=>{
  const s=$("splash"); if(!s) return;
  s.classList.add("hide"); setTimeout(()=>s.remove(),500);
},2000));

/* ---------------- Init ---------------- */
addEventListener("load",async()=>{
  const els={
    // common
    saveBtn:$("saveBtn"),

    // files
    fileList:$("fileList"),sdInfo:$("sdInfo"),

    // settings
    Sleep_info:$("Sleep_info"),
    logTXT:$("logTXT"),logUBX:$("logUBX"),logSBP:$("logSBP"),
    ssid:$("ssid"),password:$("password"),

    // performance stats toggles
    stat_2s:       $("stat_2s"),
    stat_5x10:     $("stat_5x10"),
    stat_alpha:    $("stat_alpha"),
    stat_nm:       $("stat_nm"),
    stat_hour:     $("stat_hour"),
    stat_distance: $("stat_distance"),

    
    // system
    sys_gnss_module:$("sys_gnss_module"),
    sys_gnss:$("sys_gnss"),
    sys_sample_rate:$("sys_sample_rate"),
    sys_dynamic_model:$("sys_dynamic_model"),
    sys_speed_units:$("sys_speed_units"),
    sys_cal_speed:$("sys_cal_speed"),
    sys_cpu_freq:$("sys_cpu_freq"),
    sys_display:$("sys_display"),
    sys_storage:$("sys_storage"),
    sys_version:$("sys_version")
  };

  if(els.saveBtn) els.saveBtn.addEventListener("click",()=>save(els));
  await loadFiles(els.fileList,els.sdInfo);
  enableSwipe($("files"), els.fileList, els.sdInfo);
  await loadConfig(els);
});

// Close modal if clicking outside the modal card
addEventListener("load", () => {
  const modal = $("infoModal");

  modal.addEventListener("click", (e) => {
    if (!e.target.closest(".modal-card")) {
      closeInfo();
    }
  });
});

// Show info modal when an info button is clicked
document.addEventListener("click", (e) => {
  const btn = e.target.closest(".info-btn");
  if (!btn) return;

  const key = btn.dataset.info;
  if (key) showInfoModal(key);
});


const infoTexts = {
  performance: "Toggle performance screen types for session analysis. Each option logs additional calculated data.",
  wifi: "Enter the SSID and optional password of the Wi-Fi network you'd like the device to connect to in config mode.",
  logging: "Choose which formats of raw GNSS data to log. UBX and SBP are binary protocols from different chipsets.",
  sleep: "This text will be displayed on the screen when the device is sleeping to help identify it."
};

function showInfoModal(key) {
  const title = key.charAt(0).toUpperCase() + key.slice(1) + " Info";
  const text = infoTexts[key] || "No information available.";

  $("infoTitle").textContent = title;
  $("infoText").textContent = text;
  $("infoModal").classList.add("show");
}

function closeInfo() {
  $("infoModal").classList.remove("show");
}

// Show info modal when ⓘ button clicked
document.addEventListener("click", (e) => {
  const btn = e.target.closest(".info-btn");
  if (btn) {
    const key = btn.dataset.info;
    if (key) showInfoModal(key);
  }
});


