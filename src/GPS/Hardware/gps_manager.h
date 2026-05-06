#pragma once

// GPS hardware bring-up orchestration for ESP32 + u-blox.
//
// This module owns startup sequencing only: power the receiver, find a working
// baud rate, apply u-blox configuration, and optionally inject RTC time for a
// warm start. Runtime parsing and statistics live in separate GPS modules.

// Bring up GPS.
//
// Returns:
// - true  = GPS responded and is alive
// - false = no GPS detected
bool initGPS();
