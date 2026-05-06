#pragma once

// Low-level u-blox power pin control.
// These functions are idempotent and are valid from boot/system-mode code.

void gps_power_on();
void gps_power_off();
