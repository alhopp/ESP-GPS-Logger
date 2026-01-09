// -----------------------------------------------------------------------------
// gps_simulator.cpp
//
// Simple u-blox NAV-PVT simulator for ESP-GPS testing.
//
// What it simulates:
// - 5 Hz NAV-PVT messages
// - Satellite ramp-up: 0 → 10 SV, then stable
// - Fix: no-fix until SV >= 5, then 3D fix
// - Track pattern:
//     * Straight leg ~2000 m @ ~40 kn (with small noise)
//     * Smooth 180° turn @ ~20 kn (with small noise)
//     * Turn radius randomly picked each turn: 25–35 m
//     * Repeats forever
//
// Raw u-blox style fields:
// - lat/lon: degrees * 1e7
// - gSpeed : mm/s
// - heading: degrees * 1e5
// - sAcc   : mm/s
// - iTOW   : ms
// - valid  : set to 7 (date/time validity flags used in your code)
//
// NOTE:
// - This produces “raw GPS-like” values. Your firmware’s calibration_speed
//   determines how that displays (knots/kph/etc).
// -----------------------------------------------------------------------------

#include "gps_simulator.h"
#include "Ublox/Ublox.h"
#include <math.h>
#include <stdlib.h>

// ============================================================================
// Simulator constants
// ============================================================================
static constexpr float UPDATE_DT        = 0.2f;       // 5 Hz (0.2s per tick)
static constexpr float STRAIGHT_DIST_M  = 2000.0f;    // Straight leg length
static constexpr float KNOTS_TO_MPS     = 0.514444f;  // knots → m/s

// Speeds
static constexpr float STRAIGHT_KTS     = 40.0f;      // Target on straight
static constexpr float TURN_KTS         = 20.0f;      // Target in turn

// ============================================================================
// Simulator state
// ============================================================================
static uint32_t sim_ms = 0;
static uint32_t sat_timer_ms = 0;

static uint32_t sim_start_ms = 0;
static uint32_t last_sat_step_ms = 0;


// Start location (anywhere you like)
static double lat = -32.014400;
static double lon = 115.850700;

// Motion
static double heading_deg = 0.0;
static double speed_mps   = STRAIGHT_KTS * KNOTS_TO_MPS;

static double leg_distance = 0.0;

// Turn state
static bool   turning          = false;
static double turn_radius      = 30.0;   // meters (randomised per turn)
static double turn_progress_deg = 0.0;   // accumulates until 180°
static double turn_rate_deg     = 0.0;   // heading delta per tick (deg/tick)

// GPS quality
static int sat_count = 0;

// ============================================================================
// Helpers
// ============================================================================
static inline double randf(double minv, double maxv)
{
  // Uniform float random in [minv, maxv]
  return minv + (maxv - minv) * (double(rand()) / RAND_MAX);
}

// ============================================================================
// Init
// ============================================================================
void gps_simulator_init()
{
  sim_start_ms     = millis();
  last_sat_step_ms = sim_start_ms;
  sat_count        = 0;

  sim_ms = 0;

  lat = -32.014400;
  lon = 115.850700;

  heading_deg = 0.0;
  speed_mps   = STRAIGHT_KTS * KNOTS_TO_MPS;

  leg_distance = 0.0;
  turning = false;
}


// ============================================================================
// One simulation step (called instead of processGPS())
// Returns MT_NAV_PVT so your pipeline behaves like real GPS input.
// ============================================================================
int gps_simulator_step()
{
  static bool sim_initialised = false;

  if (!sim_initialised) {
    gps_simulator_init();
    sim_initialised = true;
  }

  // Advance simulated time
  sim_ms += uint32_t(UPDATE_DT * 1000);
  ubxMessage.navPvt.iTOW = sim_ms;

  // --------------------------------------------------------------------------
  // Satellite acquisition (15 second ramp-up)
  // --------------------------------------------------------------------------
  uint32_t now = millis();

  // Increase satellite count every 1500 ms (real time)
  if (sat_count < 10 && (now - last_sat_step_ms) >= 1500) {
  sat_count++;
  last_sat_step_ms = now;
  }

  if (sat_count < 5) {
   ubxMessage.navPvt.fixType = 0;   // no fix
  } else {
   ubxMessage.navPvt.fixType = 3;   // 3D fix
  }

  ubxMessage.navPvt.numSV  = sat_count;
  ubxMessage.navPvt.sAcc   = 800;   // 0.8 m/s
  ubxMessage.navPvt.valid = 7;

  // Do NOT move until 3D fix exists
  if (ubxMessage.navPvt.fixType < 3) {
   ubxMessage.navPvt.gSpeed  = 0;
   ubxMessage.navPvt.heading = heading_deg * 100000.0;
   ubxMessage.navPvt.nano    = 0;
   return MT_NAV_PVT;
  }
  
  // Accuracy values that satisfy your thresholds
  ubxMessage.navPvt.sAcc   = 800;   // 0.8 m/s
  ubxMessage.navPvt.valid = 7;

  // --------------------------------------------------------------------------
  // Motion model
  // --------------------------------------------------------------------------
  if (!turning)
  {
    // ---------- STRAIGHT ----------
    const double target_speed = STRAIGHT_KTS * KNOTS_TO_MPS;

    // Add small speed noise (±1.5 kn)
    speed_mps = target_speed + randf(-1.5, 1.5) * KNOTS_TO_MPS;

    // Distance travelled this tick
    const double d = speed_mps * UPDATE_DT;
    leg_distance += d;

    // If we completed the straight, start a 180° turn
    if (leg_distance >= STRAIGHT_DIST_M)
    {
      turning = true;
      leg_distance = 0.0;

      // Choose a random turn radius for this jibe
      turn_radius = randf(25.0, 35.0);
      turn_progress_deg = 0.0;

      // Angular speed: omega = v / r  (rad/s)
      // Convert to heading delta per tick in degrees:
      const double omega = speed_mps / turn_radius; // rad/s
      turn_rate_deg = omega * (180.0 / M_PI) * UPDATE_DT; // deg/tick
    }
  }
  else
  {
    // ---------- TURN ----------
    const double target_speed = TURN_KTS * KNOTS_TO_MPS;

    // Add speed noise (±1 kn)
    speed_mps = target_speed + randf(-1.0, 1.0) * KNOTS_TO_MPS;

    // Smoothly rotate heading
    heading_deg += turn_rate_deg;
    turn_progress_deg += fabs(turn_rate_deg);

    // End turn once we’ve accumulated ~180°
    if (turn_progress_deg >= 180.0)
    {
      heading_deg = fmod(heading_deg, 360.0);
      turning = false;
    }
  }

  // --------------------------------------------------------------------------
  // Position update (local Earth approximation)
  // --------------------------------------------------------------------------
  const double d = speed_mps * UPDATE_DT;

  // Very simple lat/lon update:
  // - 1 deg lat ≈ 111111 m
  // - lon scale factor depends on latitude
  lat += (d / 111111.0) * cos(heading_deg * DEG_TO_RAD);
  lon += (d / (111111.0 * cos(lat * DEG_TO_RAD))) *
         sin(heading_deg * DEG_TO_RAD);

  // --------------------------------------------------------------------------
  // Fill UBX NAV-PVT fields (RAW style)
  // --------------------------------------------------------------------------
  ubxMessage.navPvt.lat      = lat * 1e7;                // degrees * 1e7
  ubxMessage.navPvt.lon      = lon * 1e7;                // degrees * 1e7
  ubxMessage.navPvt.gSpeed   = speed_mps * 1000.0;       // mm/s
  ubxMessage.navPvt.heading  = heading_deg * 100000.0;   // deg * 1e5
  ubxMessage.navPvt.nano     = 0;                        // full-second boundary

  return MT_NAV_PVT;
}
