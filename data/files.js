// files.js
// Outlook-style file list with date grouping + swipe delete.

let swipeBound = false;   // Files-tab local state.
let filesStorageCache = null;

// -----------------------------------------------------------------------------
// Helpers: filename -> date
// -----------------------------------------------------------------------------
function parseDateKey(name){
  // New: YYYY-MM-DD_S01_BBBC2C.ext
  let m = name.match(/^(\d{4})-(\d{2})-(\d{2})_/);
  if(m) return `${m[1]}-${m[2]}-${m[3]}`;

  // Legacy: BBBC2C_YYYYMMDD_HHMMSS.ext
  m = name.match(/_(\d{4})(\d{2})(\d{2})_/);
  if(m) return `${m[1]}-${m[2]}-${m[3]}`;

  return "unknown";
}

function formatDateLabel(key){
  if(key === "unknown") return "Unknown date";
  const d = new Date(key);
  return d.toLocaleDateString(undefined,{
    weekday:"short",
    day:"numeric",
    month:"short",
    year:"numeric"
  });
}

function sessionTitle(name){
  return name.replace(/\.(geojson|sbp|ubx|txt)$/i,"");
}

function fileSizeKb(size){
  const bytes = size || 0;
  if(bytes >= 1024 * 1024){
    return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
  }
  return `${(bytes / 1024).toFixed(1)} kB`;
}

function formatStorageSize(bytes, fallbackMb){
  const value = Number(bytes);
  if(Number.isFinite(value) && value > 0){
    if(value < 1024 * 1024){
      return `${(value / 1024).toFixed(1)} kB`;
    }
    return `${(value / (1024 * 1024)).toFixed(1)} MB`;
  }

  const mb = Number(fallbackMb);
  return Number.isFinite(mb) ? `${mb} MB` : null;
}

function updateLogsTitle(storage){
  const el = document.getElementById("logsTitle");
  if(!el) return;
  if(storage) filesStorageCache = { ...storage };

  const used = formatStorageSize(storage?.storage_used_bytes, storage?.storage_used_mb);
  const free = formatStorageSize(storage?.storage_free_bytes, storage?.storage_free_mb);
  if(used && free){
    el.textContent = `Logs - ${used} used / ${free} free`;
  }else{
    el.textContent = "Logs";
  }
}

function applyDeletedStorageBytes(deletedBytes){
  const bytes = Number(deletedBytes);
  if(!filesStorageCache || !Number.isFinite(bytes) || bytes <= 0) return;

  const used = Number(filesStorageCache.storage_used_bytes);
  const free = Number(filesStorageCache.storage_free_bytes);
  if(Number.isFinite(used)){
    filesStorageCache.storage_used_bytes = Math.max(0, used - bytes);
    filesStorageCache.storage_used_mb = Math.floor(filesStorageCache.storage_used_bytes / (1024 * 1024));
  }
  if(Number.isFinite(free)){
    filesStorageCache.storage_free_bytes = free + bytes;
    filesStorageCache.storage_free_mb = Math.floor(filesStorageCache.storage_free_bytes / (1024 * 1024));
  }

  updateLogsTitle(filesStorageCache);
}

// -----------------------------------------------------------------------------
// Load file list from device SD (grouped by date like Outlook)
// -----------------------------------------------------------------------------
async function loadFiles(fileList, sdInfo){
  if(!fileList || !sdInfo) return;

  fileList.innerHTML = "";
  sdInfo.textContent = "Scanning...";

  try{
    const r = await fetch("/api/files",{ cache:"no-store" });
    if(!r.ok) throw new Error("files api missing");

    const j = await r.json();
    if(!j.ok) throw new Error("no sd");
    updateLogsTitle(j);

    const groups = {};
    j.files.forEach(f=>{
      const dateKey = parseDateKey(f.name);
      (groups[dateKey] ||= []).push(f);
    });

    Object.keys(groups)
      .sort((a,b)=>b.localeCompare(a))
      .forEach(date=>{
        fileList.insertAdjacentHTML("beforeend",`
          <div class="file-date" data-date="${date}">
            ${formatDateLabel(date)}
          </div>
        `);

        groups[date]
          .sort((a,b)=>b.name.localeCompare(a.name))
          .forEach(f=>{
          const downloadName = f.sbp_name || f.name;
          const displayName = f.sbp_name || f.name;
          const details = f.sbp_name
            ? `Download SBP ${fileSizeKb(f.sbp_size)} | Map preview ${fileSizeKb(f.size)}`
            : `Map preview only ${fileSizeKb(f.size)}`;

          fileList.insertAdjacentHTML("beforeend",`
            <div class="file-row file-swipe"
                 data-name="${f.name}"
                 data-download="${downloadName}"
                 data-date="${date}">
              <div class="file-delete">&#128465;</div>
              <div class="file-swipe-inner">
                <div class="file-icon">&#128196;</div>
                <div class="file-text">
                  <div class="file-name">${sessionTitle(displayName)}</div>
                  <div class="file-size">${details}</div>
                </div>
              </div>
            </div>
          `);
        });
      });

    sdInfo.textContent = "";

  }catch(e){
    updateLogsTitle(null);
    sdInfo.textContent = "SD not available";
    console.warn("Files API unavailable", e);
  }
}

// -----------------------------------------------------------------------------
// Enable swipe + click behaviour
// -----------------------------------------------------------------------------
function enableSwipe(container, fileList, sdInfo){
  if(swipeBound || !container) return;
  swipeBound = true;

  let row = null, x0 = 0, y0 = 0, dx = 0, sw = false;

  const closeAll = () =>
    container.querySelectorAll(".file-swipe.show-delete")
      .forEach(r => r.classList.remove("show-delete"));

  const download = name => {
    name && (location.href =
      `/api/download?file=${encodeURIComponent(name)}&t=${Date.now()}`);
  };

  container.addEventListener("touchstart", e=>{
    const del = e.target.closest(".file-delete");
    if(del){
      e.preventDefault();
      e.stopImmediatePropagation();

      const r = del.closest(".file-swipe");
      if(!r) return;

      const date = r.dataset.date;

      fetch("/api/file",{
        method:"DELETE",
        headers:{ "Content-Type":"application/json" },
        body:JSON.stringify({ name:r.dataset.name })
      })
        .then(res => res.ok ? res.json() : { ok:false })
        .then(j => {
          if(!j.ok) throw new Error("delete failed");
          r.remove();
          applyDeletedStorageBytes(j.deleted_bytes);

          // Remove date header if no files remain for that date.
          if(!fileList.querySelector(`.file-row[data-date="${date}"]`)){
            const h = fileList.querySelector(`.file-date[data-date="${date}"]`);
            h && h.remove();
          }

          sdInfo.textContent = "";

          // Refresh map sessions if map is already running.
          if(window.MapSessions?._started){
            MapSessions.removeDeleted(r.dataset.name);
          }
        })
        .catch(err => {
          sdInfo && (sdInfo.textContent = "Delete failed");
          console.warn("Delete failed", err);
        });

      return;
    }

    row = e.target.closest(".file-swipe");
    if(!row) return;

    closeAll();
    x0 = e.touches[0].clientX;
    y0 = e.touches[0].clientY;
    dx = 0;
    sw = true;
  }, { passive:false });

  container.addEventListener("touchmove", e=>{
    if(!sw) return;

    const x = e.touches[0].clientX;
    const y = e.touches[0].clientY;

    if(Math.abs(x - x0) > Math.abs(y - y0) + 8){
      e.preventDefault();
      dx = x - x0;
    }
  }, { passive:false });

  container.addEventListener("touchend", ()=>{
    if(!sw) return;
    sw = false;

    row.classList.toggle("show-delete", dx < -36);
  }, { passive:true });

  container.addEventListener("click", e=>{
    const r = e.target.closest(".file-swipe");
    if(!r) return;

    if(!r.classList.contains("show-delete")){
      download(r.dataset.download || r.dataset.name);
    }
  });
}
