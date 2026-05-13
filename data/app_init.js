// ============================================================================
// App shell and startup
// ============================================================================

let dirty = false;

function splashReady(){
  requestAnimationFrame(() => {
    $("splash")?.classList.add("ready");
  });
}

function tab(id, btn){
  document.querySelectorAll("nav button,section")
    .forEach(e => e.classList.remove("a"));

  btn.classList.add("a");
  const section = $(id);
  section.classList.add("a");

  if(id === "map" && window.MapView){
    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        MapView.init();
        setTimeout(() => {
          MapView.map && MapView.map.invalidateSize(true);
        }, 100);
      });
    });
  }
}

window.tab = tab;

function markDirty(saveBtn){
  if(dirty) return;
  dirty = true;
  if(saveBtn) saveBtn.disabled = false;
}

addEventListener("load", () => {
  const saveBtn = $("saveBtn");
  const onDirty = e => {
    if(!e.target || e.target._loading) return;
    if(!e.target.matches("input,select,textarea")) return;
    markDirty(saveBtn);
  };

  document.addEventListener("input", onDirty);
  document.addEventListener("change", onDirty);

  document.querySelectorAll("nav button[data-tab]").forEach(btn => {
    btn.addEventListener("click", () => tab(btn.dataset.tab, btn));
  });

  $("infoCloseBtn")?.addEventListener("click", () => window.closeInfo?.());
});

addEventListener("load", async () => {
  const els = {
    saveBtn:$("saveBtn"),
    fileList:$("fileList"),
    sdInfo:$("sdInfo"),

    Sleep_info1:$("Sleep_info1"),
    Sleep_info2:$("Sleep_info2"),

    logUBX:$("logUBX"),
    logSBP:$("logSBP"),

    phone_ssid:$("phone_ssid"),
    phone_pass:$("phone_pass"),

    stat_2s:$("stat_2s"),
    stat_5x10:$("stat_5x10"),
    stat_alpha:$("stat_alpha"),
    stat_nm:$("stat_nm"),
    stat_hour:$("stat_hour"),
    stat_distance:$("stat_distance")
  };

  els.saveBtn?.addEventListener("click", () => saveConfig(els));
  els.phone_pass?.addEventListener("focus", () => {
    if(els.phone_pass.value === PASSWORD_PLACEHOLDER){
      els.phone_pass.value = "";
      els.phone_pass.dataset.editingPlaceholder = "1";
    }
  });
  els.phone_pass?.addEventListener("input", () => {
    els.phone_pass.dataset.passwordTouched = "1";
  });
  els.phone_pass?.addEventListener("blur", () => {
    if(els.phone_pass.dataset.editingPlaceholder === "1" &&
       els.phone_pass.dataset.passwordTouched !== "1" &&
       !els.phone_pass.value){
      els.phone_pass.value = PASSWORD_PLACEHOLDER;
    }
    els.phone_pass.dataset.editingPlaceholder = "0";
  });

  const splash = $("splash");
  const img = splash?.querySelector("img");
  if(img?.decode){
    img.decode().then(splashReady).catch(splashReady);
  }else{
    splashReady();
  }

  await loadFiles(els.fileList, els.sdInfo);
  enableSwipe($("files"), els.fileList, els.sdInfo);

  await loadConfig(els);

  const mapBtn = document.querySelector("nav button[data-tab='map']");
  if(mapBtn) tab("map", mapBtn);

  setTimeout(() => {
    if(!splash) return;
    splash.classList.add("hide");
    setTimeout(() => splash.remove(), 220);
  }, 650);
});
