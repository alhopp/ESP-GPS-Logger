// files.js
let swipeBound = false;   // Files-tab local state

// -----------------------------------------------------------------------------
// Load file list from device SD
// -----------------------------------------------------------------------------
async function loadFiles(fileList, sdInfo){
  if(!fileList || !sdInfo) return;

  fileList.innerHTML = "";
  sdInfo.textContent = "Scanning…";

  try{
    const r = await fetch("/api/files",{ cache:"no-store" });
    if(!r.ok) throw new Error("files api missing");

    const j = await r.json();
    if(!j.ok) throw new Error("no sd");

    sdInfo.textContent = `${j.files.length} files`;

    j.files.forEach(f=>{
      fileList.insertAdjacentHTML("beforeend",`
        <div class="file-row file-swipe" data-name="${f.name}">
          <div class="file-delete">🗑</div>
          <div class="file-swipe-inner">
            <div class="file-icon">📄</div>
            <div class="file-text">
              <div class="file-name">${f.name}</div>
              <div class="file-size">${(f.size/1024).toFixed(1)} KB</div>
            </div>
          </div>
        </div>
      `);
    });

  }catch(e){
    sdInfo.textContent = "SD not available";
    console.warn("Files API unavailable");
  }
}

// -----------------------------------------------------------------------------
// Enable swipe + click behaviour
// -----------------------------------------------------------------------------
function enableSwipe(container, fileList, sdInfo){
  if(swipeBound || !container) return;
  swipeBound = true;

  let row = null, x0 = 0, y0 = 0, dx = 0, sw = false, moved = false;

  const closeAll = () =>
    container.querySelectorAll(".file-swipe.show-delete")
      .forEach(r => r.classList.remove("show-delete"));

  const download = name => {
    name && (location.href =
      `/api/download?file=${encodeURIComponent(name)}&t=${Date.now()}`);
  };

  // ---------------- Touch start ----------------
  container.addEventListener("touchstart", e=>{
    const del = e.target.closest(".file-delete");
    if(del){
      e.preventDefault();
      e.stopImmediatePropagation();

      const r = del.closest(".file-swipe");
      if(!r) return;

      fetch("/api/file",{
        method:"DELETE",
        headers:{ "Content-Type":"application/json" },
        body:JSON.stringify({ name:r.dataset.name })
      });

      r.remove();
      sdInfo && (sdInfo.textContent = `${fileList.children.length} files`);

      // 🔁 refresh map sessions if map already running
      if(window.MapSessions?._started){
        MapSessions.reload();
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
    moved = false;
  }, { passive:false });

  // ---------------- Touch move ----------------
  container.addEventListener("touchmove", e=>{
    if(!sw) return;

    const x = e.touches[0].clientX;
    const y = e.touches[0].clientY;

    if(Math.abs(x - x0) > Math.abs(y - y0) + 8){
      moved = true;
      e.preventDefault();
      dx = x - x0;
    }
  }, { passive:false });

  // ---------------- Touch end ----------------
  container.addEventListener("touchend", ()=>{
    if(!sw) return;
    sw = false;

    row.classList.toggle("show-delete", dx < -36);
    moved = false;
  }, { passive:true });

  // ---------------- Click (download only) ----------------
  container.addEventListener("click", e=>{
    const r = e.target.closest(".file-swipe");
    if(!r) return;

    if(!r.classList.contains("show-delete")){
      download(r.dataset.name);
    }
  });
}

