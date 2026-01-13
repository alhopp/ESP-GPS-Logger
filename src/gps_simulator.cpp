// -----------------------------------------------------------------------------
// gps_simulator.cpp
//
// Real-time u-blox NAV-PVT simulator for ESP-GPS testing.
//
// Simulates:
// - 5 Hz–ish NAV-PVT updates (driven by call frequency)
// - Satellite ramp-up over ~15 seconds (real time)
// - No fix until >= 5 SV, then 3D fix
// - No movement until 3D fix exists
// - Track pattern:
//     * ~2000 m straight @ ~40 kn (with noise)
//     * Smooth 180° turn @ ~20 kn
//     * Turn radius randomly chosen: 25–35 m
//     * Repeats forever
//
// Outputs RAW u-blox style values:
// - lat/lon   : degrees * 1e7
// - gSpeed    : mm/s
// - heading   : degrees * 1e5
// - sAcc      : mm/s
// - iTOW      : ms
// - valid     : 7
// -----------------------------------------------------------------------------

#include "gps_simulator.h"
#include "Ublox/Ublox.h"

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>

// ============================================================================
// Constants
// ============================================================================
static constexpr float STRAIGHT_DIST_M = 200.0f;
static constexpr float KNOTS_TO_MPS    = 0.514444f;

static constexpr float STRAIGHT_KTS    = 40.0f;
static constexpr float TURN_KTS        = 20.0f;



// ============================================================================
// Simulator state
// ============================================================================


static uint32_t last_emit_ms = 0;
static constexpr uint32_t SIM_RATE_MS = 200; // 5 Hz

static bool     sim_initialised = false;

static uint32_t sim_ms          = 0;
static uint32_t last_step_ms   = 0;
static uint32_t last_sat_step  = 0;

// GPS quality
static int sat_count = 0;

// Position
static double lat = -32.014400;
static double lon = 115.850700;

// Motion
static double heading_deg = 0.0;
static double speed_mps   = STRAIGHT_KTS * KNOTS_TO_MPS;
static double leg_distance = 0.0;

// Turn state
static bool   turning = false;
static double turn_radius = 30.0;
static double turn_progress_deg = 0.0;
static double turn_rate_deg_per_sec = 0.0;

// ============================================================================
// Helpers
// ============================================================================
static inline double randf(double minv, double maxv)
{
  return minv + (maxv - minv) * (double(rand()) / RAND_MAX);
}

// ============================================================================
// Init
// ============================================================================
void gps_simulator_init()
{
  sim_initialised = true;

  sim_ms        = 0;
  last_step_ms  = millis();
  last_sat_step = last_step_ms;

  sat_count = 0;

  lat = -32.014400;
  lon = 115.850700;

  heading_deg  = 0.0;
  speed_mps    = STRAIGHT_KTS * KNOTS_TO_MPS;
  leg_distance = 0.0;

  turning = false;
  turn_radius = 30.0;
  turn_progress_deg = 0.0;
  turn_rate_deg_per_sec = 0.0;
}

// ============================================================================
// One simulation step
// ============================================================================
int gps_simulator_step()
{
  if (!sim_initialised) {gps_simulator_init();}

  // --------------------------------------------------------------------------
  // Real-time delta
  // --------------------------------------------------------------------------
  uint32_t now = millis();
  float dt = (now - last_step_ms) / 1000.0f;
  last_step_ms = now;

  if (dt < 0.0f) dt = 0.0f;
  if (dt > 0.5f) dt = 0.5f;   // clamp large gaps

  sim_ms += uint32_t(dt * 1000.0f);
  ubxMessage.navPvt.iTOW = sim_ms;

  // --------------------------------------------------------------------------
  // Enforce simulator output rate (5 Hz)
  // --------------------------------------------------------------------------
  // Enforce simulator output rate (exact 5 Hz)
    if (now - last_emit_ms < SIM_RATE_MS) {
      return MT_NONE;
    }
    last_emit_ms += SIM_RATE_MS;

    sim_ms += SIM_RATE_MS;
    ubxMessage.navPvt.iTOW = sim_ms;



  // --------------------------------------------------------------------------
  // Advance simulated UTC seconds
  // --------------------------------------------------------------------------
  static uint32_t last_sec_tick = 0;

  uint32_t sec_tick = sim_ms / 1000;
  if (sec_tick != last_sec_tick) {
    last_sec_tick = sec_tick;
    ubxMessage.navPvt.sec++;
    if (ubxMessage.navPvt.sec >= 60) ubxMessage.navPvt.sec = 0;
  }


  // --------------------------------------------------------------------------
  // Satellite acquisition (≈15 seconds total)
  // --------------------------------------------------------------------------
  if (sat_count < 10 && (now - last_sat_step) >= 1500) {
    sat_count++;
    last_sat_step = now;
  }

  ubxMessage.navPvt.numSV = sat_count;
  ubxMessage.navPvt.fixType = (sat_count >= 5) ? 3 : 0;
  ubxMessage.navPvt.sAcc = 800;    // 0.8 m/s
  ubxMessage.navPvt.valid = 7;

  // --------------------------------------------------------------------------
  // Do not move until 3D fix exists
  // --------------------------------------------------------------------------
  if (ubxMessage.navPvt.fixType < 3) {
    ubxMessage.navPvt.gSpeed  = 0;
    ubxMessage.navPvt.heading = heading_deg * 100000.0;
    ubxMessage.navPvt.nano    = 0;
    return MT_NAV_PVT;
  }

   ubxMessage.navDOP.hDOP = 120;  // 1.20 HDOP (reasonable)
   ubxMessage.navPvt.velD = 0;    // flat motion


  // --------------------------------------------------------------------------
  // Motion model
  // --------------------------------------------------------------------------
  if (!turning)
  {
    // ---------- STRAIGHT ----------
    const double target = STRAIGHT_KTS * KNOTS_TO_MPS;
    speed_mps = target + randf(-1.5, 1.5) * KNOTS_TO_MPS;

    const double d = speed_mps * dt;
    leg_distance += d;

    if (leg_distance >= STRAIGHT_DIST_M)
    {
      turning = true;
      leg_distance = 0.0;

      turn_radius = randf(25.0, 35.0);
      turn_progress_deg = 0.0;

      const double omega = speed_mps / turn_radius; // rad/sec
      turn_rate_deg_per_sec = omega * (180.0 / M_PI);
    }
  }
  else
  {
    // ---------- TURN ----------
    const double target = TURN_KTS * KNOTS_TO_MPS;
    speed_mps = target + randf(-1.0, 1.0) * KNOTS_TO_MPS;

    const double dHead = turn_rate_deg_per_sec * dt;
    heading_deg += dHead;
    turn_progress_deg += fabs(dHead);

    if (turn_progress_deg >= 180.0) {
      heading_deg = fmod(heading_deg, 360.0);
      turning = false;
    }
  }

  // --------------------------------------------------------------------------
  // Position update (local Earth approximation)
  // --------------------------------------------------------------------------
  const double d = speed_mps * dt;

  lat += (d / 111111.0) * cos(heading_deg * DEG_TO_RAD);
  lon += (d / (111111.0 * cos(lat * DEG_TO_RAD))) *
         sin(heading_deg * DEG_TO_RAD);

  // --------------------------------------------------------------------------
  // Populate NAV-PVT (RAW)
  // --------------------------------------------------------------------------
  ubxMessage.navPvt.lat     = lat * 1e7;
  ubxMessage.navPvt.lon     = lon * 1e7;
  ubxMessage.navPvt.gSpeed  = speed_mps * 1000.0;     // mm/s
  ubxMessage.navPvt.heading = heading_deg * 100000.0; // deg * 1e5


  // --------------------------------------------------------------------------
  // Simulated GPS date/time (UTC)
  // --------------------------------------------------------------------------
    // --------------------------------------------------------------------------
    // Simulated GPS date/time (UTC)
    // --------------------------------------------------------------------------
    static bool time_init = false;

    if (!time_init && ubxMessage.navPvt.fixType >= 3)
    {
    ubxMessage.navPvt.year  = 2026;
    ubxMessage.navPvt.month = 1;
    ubxMessage.navPvt.day   = 10;

    ubxMessage.navPvt.hour = 8;
    ubxMessage.navPvt.min  = 30;
    ubxMessage.navPvt.sec  = 0;

    ubxMessage.navPvt.nano = 0;
    ubxMessage.navPvt.tAcc = 50000;   // 50 µs

    // validDate | validTime | fullyResolved
    ubxMessage.navPvt.valid = 0b111;

    time_init = true;
    }



  return MT_NAV_PVT;
}
