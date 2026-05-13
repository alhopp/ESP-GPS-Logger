window.SystemTab = (function(){

  function load(system){
    if(!system) return;

    const yesNo = v => v ? "Yes" : "No";
    const compactMb = { mbDecimals:0, compact:true };
    const storageText = system.storage_detected
      ? AppUtil.formatStorageSize(system.storage_bytes, system.storage_mb, compactMb)
      : "Not detected";
    const storageUsedText = system.storage_detected
      ? `${AppUtil.formatStorageSize(system.storage_used_bytes, system.storage_used_mb, compactMb)} used / ${AppUtil.formatStorageSize(system.storage_free_bytes, system.storage_free_mb, compactMb)} free`
      : null;

    AppUtil.setText($("sys_gnss_module"),   system.gnss_module);
    AppUtil.setText($("sys_gnss"),          system.gnss_mode);
    AppUtil.setText($("sys_sample_rate"),   system.sample_rate ? system.sample_rate+" Hz" : null);
    AppUtil.setText($("sys_dynamic_model"), system.dynamic_model);
    AppUtil.setText($("sys_speed_units"),   system.speed_units);
    AppUtil.setText($("sys_display"),       system.display);
    AppUtil.setText($("sys_storage"),       storageText);
    AppUtil.setText($("sys_storage_used"),  storageUsedText);
    AppUtil.setText($("sys_cpu_freq"),      system.cpu_freq ? system.cpu_freq+" MHz" : null);
    AppUtil.setText($("sys_version"),       system.software_version);
    AppUtil.setText($("sys_simulator"),     yesNo(system.simulator));
    AppUtil.setText($("sys_dev_wifi"),      yesNo(system.dev_wifi));
    AppUtil.setText($("sys_logging"),       yesNo(system.logging_enabled));
    AppUtil.setText($("sys_wifi_status"),   system.wifi_connected ? "Connected" : "Not connected");
    AppUtil.setText($("sys_wifi_ssid"),     system.wifi_ssid);
    AppUtil.setText($("sys_wifi_ip"),       system.wifi_ip);
    AppUtil.setText($("sys_wifi_phone"),    system.wifi_phone_ssid);
  }

  return { load };
})();

