#include <Arduino.h> 
#include "system_mode.h"
#include "wifi_manager.h"
#include "Definitions.h"


volatile SystemMode currentMode = MODE_BOOT;

SystemMode getMode()
{
  return currentMode;
}

void setMode(SystemMode newMode)
{

  if (newMode == currentMode) return;

  // ---------------------------------------------------------------------------
  // EXIT actions (based on OLD mode)
  // ---------------------------------------------------------------------------
  switch (currentMode) {
    case MODE_FIELD_CONFIG:
    case MODE_HOME:
      wifi_stop();
      break;

    default:
      break;
  }

  // ---------------------------------------------------------------------------
  // STATE TRANSITION (THIS MUST HAPPEN BEFORE ENTER ACTIONS)
  // ---------------------------------------------------------------------------


        currentMode = newMode;

  // ---------------------------------------------------------------------------
  // ENTER actions (based on NEW mode)
  // ---------------------------------------------------------------------------
  // ---------------------------------------------------------------------------
// ENTER actions (based on NEW mode)
//
// NOTE:
// - This switch performs side-effects only (no UI).
// - Display task reacts separately based on getMode().
// ---------------------------------------------------------------------------
switch (currentMode) {

  case MODE_LOGGING:
    LOG_SYS("MODE", "ENTER LOGGING → WiFi OFF");
    // Wi-Fi must be OFF during logging
    wifi_stop();
    break;

  case MODE_FIELD_CONFIG:
    LOG_SYS("MODE", "ENTER FIELD_CONFIG → WiFi AP");
    // Start access point + captive portal
    wifi_start_ap();
    break;

  case MODE_HOME:
    LOG_SYS("MODE", "ENTER HOME → WiFi STA");
    // Connect to saved home network
    wifi_start_sta();
    break;

  case MODE_SLEEP:
    LOG_SYS("MODE", "ENTER SLEEP → WiFi OFF");
    wifi_stop();
    break;

  default:
    LOG_SYS("MODE", "ENTER UNKNOWN (%d)", currentMode);
    break;
}

}
