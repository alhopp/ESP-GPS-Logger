#pragma once

// ============================================================================
// watchdog_manager.h
//
// Lightweight wrapper around the ESP task watchdog.
//
// watchdogLoop() feeds the watchdog from the Arduino loop supervisor. The
// runtime tasks may also call esp_task_wdt_reset() directly where they have
// long display/storage operations.
// ============================================================================

void watchdogInit();
void watchdogLoop();
