// files_ui.js
async function loadFiles(fileList, sdInfo){
  if(!fileList || !sdInfo) return;

  fileList.innerHTML="";
  const r=await fetch("/api/files",{cache:"no-store"});
  const j=await r.json();

  if(!j.ok){ sdInfo.textContent="No SD"; return; }

  sdInfo.textContent=`${j.files.length} files`;

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
      </div>`);
  });
}

function enableSwipe(container, fileList, sdInfo){
  if(swipeBound || !container) return;
  swipeBound=true;

  let row, icon, x0, y0, dx=0, sw=false, moved=false;
  let tapCandidate=null, tapStartOnDelete=false;

  const triggerDownload = name=>{
    if(name)
      location.href=`/api/download?file=${encodeURIComponent(name)}&t=${Date.now()}`;
  };

  container.addEventListener("touchstart",e=>{
    row=e.target.closest(".file-swipe");
    if(!row) return;

    tapCandidate=row;
    tapStartOnDelete=!!e.target.closest(".file-delete");
    icon=row.querySelector(".file-icon");
    x0=e.touches[0].clientX;
    y0=e.touches[0].clientY;
    dx=0; sw=true; moved=false;
    if(icon) icon.style.transition="none";
  },{passive:true});

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

  container.addEventListener("touchend",()=>{
    if(!sw) return; sw=false;
    if(icon) icon.style.transition="";
    row.classList.toggle("delete",dx<-36);

    if(tapCandidate && !moved && !tapStartOnDelete && !row.classList.contains("delete"))
      triggerDownload(row.dataset.name);

    tapCandidate=null; moved=false;
  },{passive:true});

  container.addEventListener("click",e=>{
    const r=e.target.closest(".file-swipe");
    if(!r) return;

    if(e.target.closest(".file-delete")){
      fetch("/api/file",{
        method:"DELETE",
        headers:{"Content-Type":"application/json"},
        body:JSON.stringify({name:r.dataset.name})
      });
      r.remove();
      sdInfo.textContent=`${fileList.children.length} files`;
    }else if(!r.classList.contains("delete")){
      triggerDownload(r.dataset.name);
    }
  });
}
