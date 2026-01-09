// ============================================================================
// gps_simulator.cpp
//
// Real-time GPS simulator for ESP-GPS
//
// Simulates the following lifecycle:
//
//  PHASE 0 : No fix, satellites slowly appear (~15s)
//  PHASE 1 : 3D fix acquisition, accuracy improves (~10s)
//  PHASE 2 : Straight run (~2 km)
//  PHASE 3 : 180° turn and repeat straight run
//
// IMPORTANT DESIGN RULE:
// - Simulator time advances using millis(), NOT loop rate
// - Safe to call from a fast task (e.g. every 5 ms)
// - Mode logic remains untouched
// ============================================================================

#include "gps_simulator.h"
#include "Ublox/Ublox.h"
#include <Arduino.h>
#include <math.h>

// ----------------------------------------------------------------------------
// Simulator state
// ----------------------------------------------------------------------------
static uint32_t sim_ms        = 0;     // simulated GPS time (ms)
static uint32_t last_ms       = 0;     // real time reference
static uint32_t phase_start   = 0;     // start time of current phase
static int      phase         = 0;     // current simulation phase
static int      sat_count     = 0;

// ----------------------------------------------------------------------------
// Simulated position / motion
// ----------------------------------------------------------------------------
static double lat        = -32.014400;
static double lon        = 115.850700;
static double heading    = 0.0;        // degrees
static double speed_mps  = 8.0;        // ~15.5 knots

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
static constexpr double DEG2RAD = 0.0174532925;
static constexpr double METERS_PER_DEG = 111111.0;

// ----------------------------------------------------------------------------
// Reset simulator
// ----------------------------------------------------------------------------
void gps_simulator_init()
{
    sim_ms      = 0;
    last_ms     = 0;
    phase       = 0;
    sat_count   = 0;
    phase_start = 0;

    lat     = -32.014400;
    lon     = 115.850700;
    heading = 0.0;
}

// ----------------------------------------------------------------------------
// Advance simulator by REAL elapsed time
// ----------------------------------------------------------------------------
static void advance_sim_time()
{
    uint32_t now = millis();

    if (last_ms == 0) {
        last_ms = now;
        return;
    }

    uint32_t dt = now - last_ms;
    last_ms = now;

    sim_ms += dt;
}

// ----------------------------------------------------------------------------
// Main simulator step
// ----------------------------------------------------------------------------
int gps_simulator_step()
{
    advance_sim_time();

    // Expose simulated GPS time to UBX
    ubxMessage.navPvt.iTOW = sim_ms;

    // ------------------------------------------------------------
    // PHASE 0 — NO FIX (≈15 seconds)
    // ------------------------------------------------------------
    if (phase == 0)
    {
        if (phase_start == 0) phase_start = sim_ms;

        uint32_t elapsed = sim_ms - phase_start;

        // One satellite every ~3 seconds (max 4)
        sat_count = min(1 + int(elapsed / 3000), 4);

        ubxMessage.navPvt.fixType = 0;   // No fix
        ubxMessage.navPvt.numSV  = sat_count;
        ubxMessage.navPvt.sAcc   = 5000; // very poor accuracy

        if (elapsed >= 15000) {
            phase = 1;
            phase_start = sim_ms;
        }
    }

    // ------------------------------------------------------------
    // PHASE 1 — FIX ACQUISITION (≈10 seconds)
    // ------------------------------------------------------------
    else if (phase == 1)
    {
        uint32_t elapsed = sim_ms - phase_start;

        ubxMessage.navPvt.fixType = 3;   // 3D fix
        ubxMessage.navPvt.numSV  = min(5 + int(elapsed / 2000), 10);

        // Accuracy improves over time (mm/s)
        ubxMessage.navPvt.sAcc = max(300, 2000 - int(elapsed / 5));

        if (elapsed >= 10000) {
            phase = 2;
            phase_start = sim_ms;
        }
    }

    // ------------------------------------------------------------
    // PHASE 2 & 3 — STRAIGHT RUNS (2 km each)
    // ------------------------------------------------------------
    else if (phase == 2 || phase == 3)
    {
        // Distance traveled since last call (meters)
        static uint32_t last_move_ms = sim_ms;
        double dt = (sim_ms - last_move_ms) / 1000.0;
        last_move_ms = sim_ms;

        double d = speed_mps * dt;

        // Move position along heading
        lat += (d / METERS_PER_DEG) * cos(heading * DEG2RAD);
        lon += (d / (METERS_PER_DEG * cos(lat * DEG2RAD))) *
               sin(heading * DEG2RAD);

        ubxMessage.navPvt.fixType = 3;
        ubxMessage.navPvt.numSV  = 10;
        ubxMessage.navPvt.gSpeed = speed_mps * 1000;   // mm/s
        ubxMessage.navPvt.heading = heading * 100000; // degrees * 1e5
        ubxMessage.navPvt.sAcc   = 300;

        // Track distance within this leg
        static double leg_dist = 0;
        leg_dist += d;

        if (leg_dist >= 2000.0) {
            leg_dist = 0;
            phase = (phase == 2) ? 3 : 2;
            heading = fmod(heading + 180.0, 360.0);
        }
    }

    // ------------------------------------------------------------
    // COMMON NAV-PVT FIELDS
    // ------------------------------------------------------------
    ubxMessage.navPvt.lat   = lat * 1e7;
    ubxMessage.navPvt.lon   = lon * 1e7;
    ubxMessage.navPvt.nano  = 0;
    ubxMessage.navPvt.valid = 7;   // fully valid fix

    return MT_NAV_PVT;
}
