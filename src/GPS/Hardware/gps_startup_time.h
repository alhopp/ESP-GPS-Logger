#pragma once

// ============================================================================
// gps_startup_time.h
//
// Optional GPS warm-start time injection.
//
// Called during GPS bring-up after the receiver is powered and configured.
// This only gives the receiver a hint; authoritative system time is still set
// from valid NAV-PVT GPS time later.
// ============================================================================

void gps_send_time_from_rtc();
