#pragma once

// ============================================================================
// gps_power.h
//
// Low-level u-blox power pin control.
//
// Keep this module hardware-only. It should not configure UBX messages, parse
// GPS traffic, start logging, or make UI decisions.
// ============================================================================

void gps_power_on();
void gps_power_off();
