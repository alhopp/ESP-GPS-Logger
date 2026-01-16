// app_init.js
addEventListener("load",async()=>{
  const els={
    saveBtn:$("saveBtn"),
    fileList:$("fileList"), sdInfo:$("sdInfo"),
    Sleep_info:$("Sleep_info"),
    logTXT:$("logTXT"), logUBX:$("logUBX"), logSBP:$("logSBP"),
    ssid:$("ssid"), password:$("password"),
    sys_gnss_module:$("sys_gnss_module"),
    sys_gnss:$("sys_gnss"),
    sys_version:$("sys_version")
  };

  els.saveBtn?.addEventListener("click",()=>saveConfig(els));
  await loadFiles(els.fileList,els.sdInfo);
  enableSwipe($("files"),els.fileList,els.sdInfo);
  await loadConfig(els);

  // splash
  setTimeout(()=>{
    $("splash")?.classList.add("hide");
    setTimeout(()=>$("splash")?.remove(),500);
  },2000);
});
