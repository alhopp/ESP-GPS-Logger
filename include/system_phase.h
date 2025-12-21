#pragma once
#include <Arduino.h>

enum SystemPhase : uint8_t {
  PH_BOOT_START = 0,
  PH_STORAGE_INIT,
  PH_CONFIG_LOADED,
  PH_WIFI_INIT,
  PH_TASKS_CREATED,
  PH_RUNNING,
  PH_ERROR
};

extern volatile SystemPhase systemPhase;

// Phase logging macro
#define PHASE(phase, label) do { \
  systemPhase = phase; \
  Serial.printf("[PHASE] %-18s @ %lu ms\n", label, millis()); \
} while (0)
