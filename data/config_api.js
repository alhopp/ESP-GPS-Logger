// config_api.js
async function loadConfig(els){
  const r=await fetch("/api/config",{cache:"no-store"});
  const c=await r.json();

  setVal(els.Sleep_info,c.ui?.Sleep_info);
  setChk(els.logTXT,c.logging?.logTXT);
  setChk(els.logUBX,c.logging?.logUBX);
  setChk(els.logSBP,c.logging?.logSBP);
  setVal(els.ssid,c.wifi?.ssid);
  setVal(els.password,c.wifi?.password);

  setText(els.sys_gnss_module,c.system?.gnss_module);
  setText(els.sys_gnss,c.system?.gnss_mode);
  setText(els.sys_version,c.system?.software_version);

  if(els.saveBtn){ els.saveBtn.disabled=true; dirty=false; }
}

async function saveConfig(els){
  const p={
    ui:{Sleep_info:els.Sleep_info?.value??""},
    logging:{
      logTXT:!!els.logTXT?.checked,
      logUBX:!!els.logUBX?.checked,
      logSBP:!!els.logSBP?.checked
    },
    wifi:{
      ssid:els.ssid?.value??"",
      ...(els.password?.value?{password:els.password.value}:{})
    }
  };

  const r=await fetch("/api/config",{
    method:"POST",
    headers:{"Content-Type":"application/json"},
    body:JSON.stringify(p)
  });

  if(r.ok){ els.saveBtn.disabled=true; dirty=false; }
}
