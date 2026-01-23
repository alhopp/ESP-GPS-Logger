window.SystemTab = (function(){

  function $(id){ return document.getElementById(id); }
  function setText(el,v){
    if(!el) return;
    el.textContent = (v!==undefined && v!==null && v!=="") ? v : "-";
  }

  function load(system){
    if(!system) return;

    setText($("sys_gnss_module"),   system.gnss_module);
    setText($("sys_gnss"),          system.gnss_mode);
    setText($("sys_sample_rate"),   system.sample_rate ? system.sample_rate+" Hz" : null);
    setText($("sys_dynamic_model"), system.dynamic_model);
    setText($("sys_speed_units"),   system.speed_units);
    setText($("sys_cal_speed"),     system.cal_speed);
    setText($("sys_display"),       system.display);
    setText($("sys_storage"),       system.storage_mb ? system.storage_mb+" MB" : null);
    setText($("sys_cpu_freq"),      system.cpu_freq ? system.cpu_freq+" MHz" : null);
    setText($("sys_version"),       system.software_version);
  }

  return { load };
})();

