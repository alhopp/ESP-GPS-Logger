
// files_ui.js
let swipeBound = false;   // Files-tab local state


// -----------------------------------------------------------------------------
// Load file list from device SD
// - Renders file rows
// - Updates SD info text
// -----------------------------------------------------------------------------
async function loadFiles(fileList, sdInfo){
  if(!fileList||!sdInfo) return;

  fileList.innerHTML="";
  sdInfo.textContent="Scanning…";

  try{
    const r = await fetch("/api/files",{cache:"no-store"});
    if(!r.ok) throw new Error("files api missing");

    const j = await r.json();
    if(!j.ok) throw new Error("no sd");

    sdInfo.textContent = `${j.files.length} files`;

    j.files.forEach(f=>fileList.insertAdjacentHTML("beforeend",`
      <div class="file-row file-swipe" data-name="${f.name}">
        <div class="file-delete">🗑</div>
        <div class="file-swipe-inner">
          <div class="file-icon">📄</div>
          <div class="file-text">
            <div class="file-name">${f.name}</div>
            <div class="file-size">${(f.size/1024).toFixed(1)} KB</div>
          </div>
        </div>
      </div>`));

  }catch(e){
    // ✅ LOCAL DEV / OFFLINE SAFE PATH
    sdInfo.textContent="SD not available";
    console.warn("Files API unavailable");
  }
}


// -----------------------------------------------------------------------------
// Enable swipe + click behaviour
// - Swipe left to reveal delete
// - Tap to download
// - Tap delete to remove file
// -----------------------------------------------------------------------------
function enableSwipe(container,fileList,sdInfo){
  if(swipeBound||!container) return;
  swipeBound=true;

  let row,icon,x0,y0,dx=0,sw=false,moved=false;
  let tapRow=null,tapOnDelete=false;

  const download=name=>{
    name&&(location.href=`/api/download?file=${encodeURIComponent(name)}&t=${Date.now()}`);
  };

  // Touch start
  container.addEventListener("touchstart",e=>{
    row=e.target.closest(".file-swipe"); if(!row) return;
    tapRow=row; tapOnDelete=!!e.target.closest(".file-delete");
    icon=row.querySelector(".file-icon");
    x0=e.touches[0].clientX; y0=e.touches[0].clientY;
    dx=0; sw=true; moved=false;
    icon&&(icon.style.transition="none");
  },{passive:true});

  // Touch move (horizontal swipe only)
  container.addEventListener("touchmove",e=>{
    if(!sw) return;
    const x=e.touches[0].clientX,y=e.touches[0].clientY;
    if(Math.abs(x-x0)>Math.abs(y-y0)+8){
      moved=true; e.preventDefault();
      dx=Math.max(-72,Math.min(0,x-x0));
      if(icon){
        icon.style.transform=`translateX(${dx}px)`;
        icon.style.opacity=1+dx/72;
      }
    }
  },{passive:false});

  // Touch end
  container.addEventListener("touchend",()=>{
    if(!sw) return; sw=false;
    icon&&(icon.style.transition="");
    row.classList.toggle("delete",dx<-36);

    if(tapRow&&!moved&&!tapOnDelete&&!row.classList.contains("delete"))
      download(row.dataset.name);

    tapRow=null; moved=false;
  },{passive:true});

  // Click (desktop + fallback)
  container.addEventListener("click",e=>{
    const r=e.target.closest(".file-swipe"); if(!r) return;

    // Delete
    if(e.target.closest(".file-delete")){
      fetch("/api/file",{
        method:"DELETE",
        headers:{"Content-Type":"application/json"},
        body:JSON.stringify({name:r.dataset.name})
      });
      r.remove();
      sdInfo&&(sdInfo.textContent=`${fileList.children.length} files`);
    }
    // Download
    else if(!r.classList.contains("delete"))
      download(r.dataset.name);
  });
}
