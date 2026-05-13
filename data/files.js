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

function formatSpeed(value){
  const n = Number(value);
  return Number.isFinite(n) && n > 0 ? `${n.toFixed(2)} kt` : "-";
}

function updateLogsTitle(storage){
  if(storage) filesStorageCache = { ...storage };
}

function updateSessionSummary(name, stats){
  const row = Array.from(document.querySelectorAll(".file-row[data-name]"))
    .find(el => el.dataset.name === name);
  if(!row) return;

  AppUtil.setText(row.querySelector("[data-summary='2s']"), formatSpeed(stats?.max));
  AppUtil.setText(row.querySelector("[data-summary='10s']"), formatSpeed(stats?.avg10));
  AppUtil.setText(row.querySelector("[data-summary='alpha']"), formatSpeed(stats?.alpha));
}

async function loadSessionSummary(file){
  try{
    const gj = await Api.json(`/api/download?file=${encodeURIComponent(file.name)}&t=${Date.now()}`);
    const base = gj.features?.find(f => f.properties?.mode === "track");
    updateSessionSummary(file.name, base?.properties?.stats);
  }catch(err){
    console.warn("Session summary unavailable", file.name, err);
  }
}

function hydrateSessionSummaries(files){
  files
    .filter(f => f.name?.endsWith(".geojson"))
    .forEach((file, index) => {
      setTimeout(() => loadSessionSummary(file), index * 120);
    });
}

function selectFileRow(selectedRow){
  document.querySelectorAll(".file-row[data-name]")
    .forEach(row => row.classList.toggle("selected", row === selectedRow));
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
    const j = await Api.json("/api/files");
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
          <div class="file-date" data-date="${AppUtil.escapeHtml(date)}">
            ${AppUtil.escapeHtml(formatDateLabel(date))}
          </div>
        `);

        groups[date]
          .sort((a,b)=>b.name.localeCompare(a.name))
          .forEach(f=>{
          const downloadName = f.sbp_name || f.name;
          const displayName = f.sbp_name || f.name;
          const details = f.sbp_name
            ? `SBP ${fileSizeKb(f.sbp_size)} | Map ${fileSizeKb(f.size)}`
            : `Map preview only ${fileSizeKb(f.size)}`;

          fileList.insertAdjacentHTML("beforeend",`
            <div class="file-row file-swipe"
                 data-name="${AppUtil.escapeHtml(f.name)}"
                 data-download="${AppUtil.escapeHtml(downloadName)}"
                 data-date="${AppUtil.escapeHtml(date)}">
              <div class="file-actions">
                <button class="file-action file-action-delete" type="button" aria-label="Delete session">
                  <span aria-hidden="true">&#128465;</span><small>Delete</small>
                </button>
                <button class="file-action file-action-download" type="button" aria-label="Download SBP">
                  <span aria-hidden="true">&#8681;</span><small>SBP</small>
                </button>
                <button class="file-action file-action-map" type="button" aria-label="Open on map">
                  <span aria-hidden="true">&#10148;</span><small>Map</small>
                </button>
              </div>
              <div class="file-swipe-inner">
                <div class="file-text">
                  <div class="file-stats">
                    <span><b data-summary="2s">-</b><small>2sec</small></span>
                    <span><b data-summary="10s">-</b><small>10sec</small></span>
                    <span><b data-summary="alpha">-</b><small>Alpha</small></span>
                  </div>
                  <div class="file-size">${AppUtil.escapeHtml(details)}</div>
                  <div class="file-name">${AppUtil.escapeHtml(sessionTitle(displayName))}</div>
                </div>
              </div>
            </div>
          `);
        });
      });

    sdInfo.textContent = "";
    hydrateSessionSummaries(j.files);

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
    container.querySelectorAll(".file-swipe.show-actions")
      .forEach(r => r.classList.remove("show-actions"));

  const download = name => {
    name && (location.href =
      `/api/download?file=${encodeURIComponent(name)}&t=${Date.now()}`);
  };

  const openMap = r => {
    if(!r) return;
    MapSessions.openFile(r.dataset.name)
      .finally(() => AppShell.openTab("map"));
  };

  const deleteRow = r => {
    if(!r) return;
    if(!confirm("Delete this session? This cannot be undone.")) return;

    const date = r.dataset.date;

    Api.deleteJson("/api/file", { name:r.dataset.name })
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
  };

  container.addEventListener("touchstart", e=>{
    const action = e.target.closest(".file-action");
    if(action){
      e.preventDefault();
      e.stopImmediatePropagation();

      const r = action.closest(".file-swipe");
      if(!r) return;

      selectFileRow(r);
      if(action.classList.contains("file-action-download")){
        download(r.dataset.download || r.dataset.name);
      }else if(action.classList.contains("file-action-delete")){
        deleteRow(r);
      }else if(action.classList.contains("file-action-map")){
        openMap(r);
      }

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

    row.classList.toggle("show-actions", dx < -36);
  }, { passive:true });

  container.addEventListener("click", e=>{
    const action = e.target.closest(".file-action");
    if(action){
      e.preventDefault();
      e.stopPropagation();
      const r = action.closest(".file-swipe");
      if(!r) return;

      selectFileRow(r);
      if(action.classList.contains("file-action-download")){
        download(r.dataset.download || r.dataset.name);
      }else if(action.classList.contains("file-action-delete")){
        deleteRow(r);
      }else if(action.classList.contains("file-action-map")){
        openMap(r);
      }
      return;
    }

    const r = e.target.closest(".file-swipe");
    if(!r) return;

    selectFileRow(r);
    r.classList.remove("show-actions");
  });
}
