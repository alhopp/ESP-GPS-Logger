window.SystemTab = (function(){

  function $(id){ return document.getElementById(id); }
  function setText(el,v){
    if(!el) return;
    el.textContent = (v!==undefined && v!==null && v!=="") ? v : "-";
  }

  function load(system){
    if(!system) return;

    const yesNo = v => v ? "Yes" : "No";
    const storageText = system.storage_detected
      ? `${system.storage_mb} MB`
      : "Not detected";
    const storageUsedText = system.storage_detected
      ? `${system.storage_used_mb} MB used / ${system.storage_free_mb} MB free`
      : null;

    setText($("sys_gnss_module"),   system.gnss_module);
    setText($("sys_gnss"),          system.gnss_mode);
    setText($("sys_sample_rate"),   system.sample_rate ? system.sample_rate+" Hz" : null);
    setText($("sys_dynamic_model"), system.dynamic_model);
    setText($("sys_speed_units"),   system.speed_units);
    setText($("sys_display"),       system.display);
    setText($("sys_storage"),       storageText);
    setText($("sys_storage_used"),  storageUsedText);
    setText($("sys_cpu_freq"),      system.cpu_freq ? system.cpu_freq+" MHz" : null);
    setText($("sys_version"),       system.software_version);
    setText($("sys_simulator"),     yesNo(system.simulator));
    setText($("sys_dev_wifi"),      yesNo(system.dev_wifi));
    setText($("sys_logging"),       yesNo(system.logging_enabled));
  }

  return { load };
})();

