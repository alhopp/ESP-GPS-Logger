const $=i=>document.getElementById(i);
let swipeBound=false;

/* Tabs */
function tab(id,b){
  document.querySelectorAll("nav button,section").forEach(e=>e.classList.remove("a"));
  b.classList.add("a");$(id).classList.add("a");
}

/* Load files */
async function loadFiles(){
  fileList.innerHTML="";
  const r=await fetch("/api/files"),j=await r.json();
  if(!j.ok){sdInfo.textContent="No SD";return}
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

/* Swipe-to-delete */
function enableSwipe(){
  if(swipeBound) return; swipeBound=true;
  let row,inner,x0,y0,dx=0,sw=false;

  fileList.addEventListener("touchstart",e=>{
    row=e.target.closest(".file-swipe"); if(!row) return;
    inner=row.querySelector(".file-swipe-inner");
    x0=e.touches[0].clientX; y0=e.touches[0].clientY;
    dx=0; sw=true; inner.style.transition="none";
  },{passive:true});

  fileList.addEventListener("touchmove",e=>{
    if(!sw) return;
    const x=e.touches[0].clientX,y=e.touches[0].clientY;
    const ddx=x-x0,ddy=y-y0;
    if(Math.abs(ddx)>Math.abs(ddy)+6){
      e.preventDefault();
      dx=Math.max(-72,Math.min(0,ddx));
      inner.style.transform=`translateX(${dx}px)`;
    }
  },{passive:false});

  fileList.addEventListener("touchend",()=>{
    if(!sw) return; sw=false; inner.style.transition="";
    dx<-36?row.classList.add("delete"):row.classList.remove("delete");
    inner.style.transform=row.classList.contains("delete")?"translateX(-72px)":"";
  },{passive:true});

  fileList.addEventListener("click",async e=>{
    const d=e.target.closest(".file-delete"); if(!d) return;
    const r=d.closest(".file-swipe");
    await fetch("/api/file",{
      method:"DELETE",
      headers:{"Content-Type":"application/json"},
      body:JSON.stringify({name:r.dataset.name})
    });
    loadFiles();
  });
}

/* Init */
loadFiles();
enableSwipe();
