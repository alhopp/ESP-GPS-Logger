#pragma once

enum SystemMode {
  MODE_BOOT,          // power-up
  MODE_LOGGING,       // GPS primary, WiFi off
  MODE_FIELD_CONFIG,  // AP + captive portal
  MODE_HOME,          // STA on home WiFi
  MODE_SLEEP          // low power
};

extern volatile SystemMode currentMode;

// Mode control API
void setMode(SystemMode newMode);
SystemMode getMode();
