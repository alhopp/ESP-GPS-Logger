const $ = i => document.getElementById(i);
let swipeBound = false, dirty = false;

/* ---------------- Tabs ---------------- */
function tab(id, b){
  document.querySelectorAll("nav button,section").forEach(e => e.classList.remove("a"));
  b.classList.add("a");
  const s = $(id);
  if (s) s.classList.add("a");
}

/* ---------------- Dirty tracking ---------------- */
const markDirty = (saveBtn) => {
  if (!dirty){
    dirty = true;
    if (saveBtn) saveBtn.disabled = false;
  }
};

addEventListener("load", () => {
  const saveBtn = $("saveBtn");
  document.addEventListener("input", e => {
    if (e.target && !e.target._loading) markDirty(saveBtn);
  });
});

/* ---------------- Helpers ---------------- */
function setVal(el, v){
  if (!el) return;
  el._loading = true;
  el.value = v ?? "";
  el._loading = false;
}

function setChk(el, v){
  if (!el) return;
  el._loading = true;
  el.checked = !!v;
  el._loading = false;
}

function setText(el, v){
  if (!el) return;
  el.textContent = (v !== undefined && v !== null && v !== "") ? v : "-";
}

/* ---------------- Files ---------------- */
async function loadFiles(fileList, sdInfo){
  if (!fileList || !sdInfo) return;

  fileList.innerHTML = "";
  const r = await fetch("/api/files", { cache: "no-store" });
  const j = await r.json();

  if (!j.ok){
    sdInfo.textContent = "No SD";
    return;
  }

  sdInfo.textContent = `${j.files.length} files`;

  j.files.forEach(f => {
    fileList.innerHTML += `
      <div class="file-row file-swipe" data-name="${f.name}">
        <div class="file-delete">🗑</div>
        <div class="file-swipe-inner">
          <div class="file-icon">📄</div>
          <div class="file-text">
            <div class="file-name">${f.name}</div>
            <div class="file-size">${(f.size / 1024).toFixed(1)} KB</div>
          </div>
        </div>
      </div>`;
  });
}

/* ---------------- Swipe delete ---------------- */
function enableSwipe(fileList, sdInfo){
  if (swipeBound) return;
  if (!fileList || !sdInfo) return;

  swipeBound = true;

  let row, icon, x0, y0, dx = 0, sw = false;

  fileList.addEventListener("touchstart", e => {
    row = e.target.closest(".file-swipe");
    if (!row) return;

    icon = row.querySelector(".file-icon");
    x0 = e.touches[0].clientX;
    y0 = e.touches[0].clientY;
    dx = 0;
    sw = true;

    if (icon) icon.style.transition = "none";
  }, { passive: true });

  fileList.addEventListener("touchmove", e => {
    if (!sw) return;

    const x = e.touches[0].clientX;
    const y = e.touches[0].clientY;

    if (Math.abs(x - x0) > Math.abs(y - y0) + 6){
      e.preventDefault();
      dx = Math.max(-72, Math.min(0, x - x0));

      if (icon){
        icon.style.transform = `translateX(${dx}px)`;
        icon.style.opacity = 1 + dx / 72;
      }
    }
  }, { passive: false });

  fileList.addEventListener("touchend", () => {
    if (!sw) return;
    sw = false;

    if (icon) icon.style.transition = "";

    if (dx < -36){
      if (row) row.classList.add("delete");
      if (icon){
        icon.style.transform = "translateX(-72px)";
        icon.style.opacity = 0;
      }
    } else {
      if (row) row.classList.remove("delete");
      if (icon){
        icon.style.transform = "";
        icon.style.opacity = "";
      }
    }
  }, { passive: true });

fileList.addEventListener("click", async e => {
  let d = e.target;

  // climb up manually if needed
  if (!d.classList || !d.classList.contains("file-delete")) {
    d = d.closest && d.closest(".file-delete");
  }
  if (!d) return;

  const r = d.closest(".file-swipe");
  if (!r) return;

  await fetch("/api/file", {
    method: "DELETE",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ name: r.dataset.name })
  });

  // smooth remove
  r.style.transition = "height .2s,opacity .2s";
  r.style.opacity = 0;
  r.style.height = 0;
  setTimeout(() => r.remove(), 200);

  // update counter only
  const n = fileList.children.length;
  sdInfo.textContent = `${n - 1} files`;
});

}

/* ---------------- Config load ---------------- */
async function loadConfig(els){
  const r = await fetch("/api/config", { cache: "no-store" });
  const c = await r.json();

  // UI
  setVal(els.Sleep_info, c.ui?.Sleep_info);
  setVal(els.Board_Logo, c.ui?.Board_Logo);
  setVal(els.Sail_Logo,  c.ui?.Sail_Logo);

  // Logging
  setChk(els.logTXT, c.logging?.logTXT);
  setChk(els.logUBX, c.logging?.logUBX);
  setChk(els.logSBP, c.logging?.logSBP);
  setChk(els.logGPY, c.logging?.logGPY);
  setChk(els.logGPX, c.logging?.logGPX);

  // Wi-Fi
  setVal(els.ssid, c.wifi?.ssid);

  // System (read-only)
  setText(els.sys_gnss_module, c.system?.gnss_module);
  setText(els.sys_gnss,         c.gps?.gnss);
  setText(els.sys_sample_rate,  c.gps?.sample_rate ? (c.gps.sample_rate + " Hz") : null);
  setText(els.sys_dynamic_model,c.gps?.dynamic_model);
  setText(els.sys_display,      c.system?.display);
  setText(els.sys_storage,      c.system?.storage_mb ? (c.system.storage_mb + " MB") : null);
  setText(els.sys_version,      c.system?.software_version);

  if (els.saveBtn) els.saveBtn.disabled = true;
  dirty = false;
}

/* ---------------- Save ---------------- */
async function save(els){
  const p = {
    ui:{
      Sleep_info: els.Sleep_info?.value ?? "",
      Board_Logo: +(els.Board_Logo?.value ?? 0),
      Sail_Logo:  +(els.Sail_Logo?.value ?? 0)
    },
    logging:{
      logTXT: !!els.logTXT?.checked,
      logUBX: !!els.logUBX?.checked,
      logSBP: !!els.logSBP?.checked,
      logGPY: !!els.logGPY?.checked,
      logGPX: !!els.logGPX?.checked
    },
    wifi:{
      ssid: els.ssid?.value ?? "",
      password: els.password?.value ?? ""
    }
  };

  const r = await fetch("/api/config", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(p)
  });

  if (r.ok){
    if (els.saveBtn) els.saveBtn.disabled = true;
    dirty = false;
  }
}

/* ---------------- Splash ---------------- */
addEventListener("load", () => setTimeout(() => {
  const s = $("splash");
  if (!s) return;
  s.classList.add("hide");
  setTimeout(() => s.remove(), 500);
}, 2000));

/* ---------------- Init ---------------- */
addEventListener("load", async () => {
  const els = {
    // common
    saveBtn: $("saveBtn"),

    // Files
    fileList: $("fileList"),
    sdInfo: $("sdInfo"),

    // Settings
    Sleep_info: $("Sleep_info"),
    Board_Logo: $("Board_Logo"),
    Sail_Logo: $("Sail_Logo"),
    logTXT: $("logTXT"),
    logUBX: $("logUBX"),
    logSBP: $("logSBP"),
    logGPY: $("logGPY"),
    logGPX: $("logGPX"),
    ssid: $("ssid"),
    password: $("password"),

    // System
    sys_gnss_module: $("sys_gnss_module"),
    sys_gnss: $("sys_gnss"),
    sys_sample_rate: $("sys_sample_rate"),
    sys_dynamic_model: $("sys_dynamic_model"),
    sys_display: $("sys_display"),
    sys_storage: $("sys_storage"),
    sys_version: $("sys_version")
  };

  // bind save button
  if (els.saveBtn) els.saveBtn.addEventListener("click", () => save(els));

  await loadFiles(els.fileList, els.sdInfo);
  enableSwipe(els.fileList, els.sdInfo);
  await loadConfig(els);
});
