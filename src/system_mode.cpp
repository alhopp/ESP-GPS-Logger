#include "system_mode.h"
#include "wifi_manager.h"

volatile SystemMode currentMode = MODE_BOOT;

SystemMode getMode() {
  return currentMode;
}

void setMode(SystemMode newMode) {
  if (newMode == currentMode) return;

  // -------- EXIT actions --------
  switch (currentMode) {
    case MODE_FIELD_CONFIG:
    case MODE_HOME:
      wifi_stop();
      break;
    default:
      break;
  }

  // -------- ENTER actions --------
  switch (newMode) {

    case MODE_LOGGING:
      wifi_stop();
      break;

    case MODE_FIELD_CONFIG:
      wifi_start_ap();
      break;

    case MODE_HOME:
      wifi_start_sta();
      break;

    case MODE_SLEEP:
      wifi_stop();
      break;

    default:
      break;
  }

  currentMode = newMode;
}
