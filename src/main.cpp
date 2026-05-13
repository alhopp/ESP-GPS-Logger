// ============================================================================
// main.cpp
// - System entry point
// - Boot, init subsystems, start RTOS tasks, then enter idle mode
// - loop() acts only as a lightweight supervisor
// ============================================================================

#include <Arduino.h>

#include "System/app_supervisor.h"
#include "System/app_startup.h"

// ============================================================================
// Setup (runs once at boot)
// ============================================================================
void setup() {
  appStartup();
}

// ============================================================================
// Loop (runs continuously)
// ============================================================================
void loop() {
  appSupervisorLoop();
}
