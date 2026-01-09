#include "gps_simulator.h"
#include "Ublox/Ublox.h"

#include <math.h>

// ============================================================================
// GPS Simulator
//
// Purpose:
// - Generate deterministic, realistic UBX NAV-PVT data
// - Exercise UI, logging, run detection, and speed logic without hardware
//
// Behaviour:
//   Phase 0 : No fix, satellites appear slowly (~15s)
//   Phase 1 : 3D fix acquired, accuracy improves
//   Phase 2 : Straight line motion (~2 km)
//   Phase 3 : 180° turn, repeat straight line
//
// Update rate: 5 Hz (200 ms per step)
// ============================================================================

// ---------------------------------------------------------------------------
// Simulator state
// ---------------------------------------------------------------------------
static uint32_t sim_ms = 0;           // Simulated GPS time (ms)
static uint32_t phase_start_ms = 0;   // Timestamp when current phase began

static int phase = 0;                 // Current simulation phase
static int sat_count = 0;             // Visible satellite count

// Position and motion
static double lat = -32.014400;
static double lon = 115.850700;
static double heading = 0.0;          // Degrees
static double speed_mps = 8.0;         // ~15.5 knots

// Distance tracking for straight runs
static double run_distance_m = 0.0;

// ---------------------------------------------------------------------------
// Initialise simulator
// ---------------------------------------------------------------------------
void gps_simulator_init()
{
    sim_ms = 0;
    phase = 0;
    sat_count = 0;
    phase_start_ms = 0;
    run_distance_m = 0.0;

    lat = -32.014400;
    lon = 115.850700;
    heading = 0.0;
}

// ---------------------------------------------------------------------------
// Step simulator (called once per loop)
// Returns a synthetic UBX message type
// ---------------------------------------------------------------------------
int gps_simulator_step()
{
    // ------------------------------------------------------------
    // Advance simulated time (5 Hz)
    // ------------------------------------------------------------
    sim_ms += 200;
    ubxMessage.navPvt.iTOW = sim_ms;

    // ------------------------------------------------------------
    // PHASE 0: No fix, satellites slowly appear (~15 seconds)
    // ------------------------------------------------------------
    if (phase == 0)
    {
        if (phase_start_ms == 0)
            phase_start_ms = sim_ms;

        ubxMessage.navPvt.fixType = 0;   // No fix
        ubxMessage.navPvt.sAcc    = 9999; // Very poor accuracy

        // Add one satellite every ~3 seconds (up to 4)
        if ((sim_ms - phase_start_ms) % 3000 < 200)
        {
            if (sat_count < 4)
                sat_count++;
        }

        ubxMessage.navPvt.numSV = sat_count;

        // After ~15 seconds, move to fix acquisition
        if (sim_ms - phase_start_ms >= 15000)
        {
            phase = 1;
            phase_start_ms = sim_ms;
        }
    }

    // ------------------------------------------------------------
    // PHASE 1: 3D fix acquired, accuracy improves
    // ------------------------------------------------------------
    else if (phase == 1)
    {
        ubxMessage.navPvt.fixType = 3;   // 3D fix
        ubxMessage.navPvt.numSV  = sat_count < 10 ? sat_count++ : 10;
        ubxMessage.navPvt.sAcc   = 1200; // ~1.2 m/s

        if (ubxMessage.navPvt.numSV >= 10)
        {
            phase = 2;
            phase_start_ms = sim_ms;
            run_distance_m = 0.0;
        }
    }

    // ------------------------------------------------------------
    // PHASE 2 & 3: Straight-line motion (back and forth)
    // ------------------------------------------------------------
    else if (phase == 2 || phase == 3)
    {
        const double dt = 0.2;                  // seconds
        const double d  = speed_mps * dt;       // meters per tick

        // Move position (simple local Earth approximation)
        lat += (d / 111111.0) * cos(heading * DEG_TO_RAD);
        lon += (d / (111111.0 * cos(lat * DEG_TO_RAD))) *
               sin(heading * DEG_TO_RAD);

        run_distance_m += d;

        // Populate NAV-PVT fields
        ubxMessage.navPvt.fixType = 3;
        ubxMessage.navPvt.numSV  = 10;
        ubxMessage.navPvt.gSpeed = speed_mps * 1000;     // mm/s
        ubxMessage.navPvt.heading = heading * 100000;    // 1e-5 deg
        ubxMessage.navPvt.sAcc   = 300;                  // good accuracy

        // After ~2 km, reverse direction
        if (run_distance_m >= 2000.0)
        {
            run_distance_m = 0.0;
            heading = fmod(heading + 180.0, 360.0);
            phase = (phase == 2) ? 3 : 2;
        }
    }

    // ------------------------------------------------------------
    // Common NAV-PVT fields (always valid once simulator runs)
    // ------------------------------------------------------------
    ubxMessage.navPvt.lat   = lat * 1e7;
    ubxMessage.navPvt.lon   = lon * 1e7;
    ubxMessage.navPvt.valid = 7;     // date + time + fully resolved
    ubxMessage.navPvt.nano  = 0;     // full-second boundary

    return MT_NAV_PVT;
}
