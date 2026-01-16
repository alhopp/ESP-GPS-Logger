// app_core.js
const $ = i => document.getElementById(i);

// ---------------- Dirty tracking ----------------
let dirty = false;
let swipeBound = false;

function markDirty(saveBtn){
  if (dirty) return;
  dirty = true;
  if (saveBtn) saveBtn.disabled = false;
}

// ---------------- Helpers ----------------
function setVal(el,v){ if(!el) return; el._loading=true; el.value=v??""; el._loading=false; }
function setChk(el,v){ if(!el) return; el._loading=true; el.checked=!!v; el._loading=false; }
function setText(el,v){ if(!el) return; el.textContent=(v!==undefined&&v!==null&&v!=="")?v:"-"; }

// ---------------- Global dirty listeners ----------------
addEventListener("load",()=>{
  const saveBtn = $("saveBtn");

  const onDirty = e=>{
    if(!e.target || e.target._loading) return;
    if(!e.target.matches("input,select,textarea")) return;
    markDirty(saveBtn);
  };

  document.addEventListener("input", onDirty);
  document.addEventListener("change", onDirty);
});
