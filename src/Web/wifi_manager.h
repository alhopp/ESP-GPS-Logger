#pragma once

// ============================================================================
// wifi_manager.h
//
// Wi-Fi lifecycle and provisioning state.
//
// CONFIG mode calls wifi_init()/wifi_loop()/wifi_stop(). The manager first tries
// STA mode using saved phone hotspot credentials, then falls back to AP setup.
// ============================================================================

enum WifiUiState {
  WIFI_UI_TRYING,
  WIFI_UI_FAILED,
  WIFI_UI_AP,
  WIFI_UI_CONNECTED,  
  WIFI_UI_OFF
};

void wifi_init();
void wifi_stop();
void wifi_loop();

// Display-facing state for the config/status screen.
WifiUiState wifi_get_ui_state();

// SSID/password status that the firmware will actually use for STA mode.
// In dev builds this reflects the forced dev hotspot, not the saved config.
const char* wifi_effective_phone_ssid();
bool wifi_effective_phone_password_set();

// True when either STA or AP networking is active.
bool wifi_net_active();

// True when the root page should serve AP provisioning UI.
bool wifi_show_ap_page();
