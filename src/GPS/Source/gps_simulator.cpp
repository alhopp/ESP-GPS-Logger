// -----------------------------------------------------------------------------
// gps_simulator.cpp
// STRAIGHT + CONSTANT RADIUS TURN SIMULATOR (5 Hz, REAL TIME)
//
// Behaviour:
// - Straight: 500 m @ 30–40 kn with gentle speed variance
// - Turn    : 24 m radius, 180° arc @ 15–20 kn
// - Speed ramps smoothly like a windsurfer
//
// Guarantees:
// - Wait-for-sats before motion
// - Proper UTC init + carry (no rollback)
// - Clean geometry (no drift, no spirals)
// -----------------------------------------------------------------------------

#include "GPS/Source/gps_simulator.h"
#include "GPS/Ublox/ublox_driver.h"

#include <Arduino.h>
#include <math.h>

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
static constexpr float    SIM_DT          = 0.2f;     // 5 Hz
static constexpr uint32_t SIM_RATE_MS     = 200;
static constexpr float    KNOTS_TO_MPS    = 0.514444f;

static constexpr float STRAIGHT_MIN_KTS   = 30.0f;
static constexpr float STRAIGHT_MAX_KTS   = 40.0f;
static constexpr float TURN_MIN_KTS       = 15.0f;
static constexpr float TURN_MAX_KTS       = 20.0f;

static constexpr float SPEED_RAMP_MPS2    = 1.2f;     // accel/decel
static constexpr float STRAIGHT_LEN_M     = 500.0f;
static constexpr float TURN_RADIUS_M      = 24.0f;
static constexpr float TURN_ANGLE_RAD     = 180.0f * DEG_TO_RAD;

// Speed texture
static constexpr float STRAIGHT_WIND_AMPL_KTS = 2.5f;   // ± knots
static constexpr float WIND_OSC_PERIOD_S     = 10.0f;  // seconds

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------
enum Mode { STRAIGHT, TURN };
static Mode mode = STRAIGHT;

static float lat = -32.0236f;
static float lon = 115.8222f;

static float heading_deg = 45.0f;
static float speed_mps   = 0.0f;
static float target_mps  = 0.0f;

// Straight
static float straight_dist     = 0.0f;
static float straight_base_mps = 0.0f;
static float wind_phase        = 0.0f;

// Turn geometry
static float turn_phi        = 0.0f;
static float turn_center_lat = 0.0f;
static float turn_center_lon = 0.0f;
static float turn_dir_n      = 0.0f;
static float turn_dir_e      = 0.0f;
static float turn_nrm_n      = 0.0f;
static float turn_nrm_e      = 0.0f;

// Timing
static uint32_t sim_ms = 0;

// Satellites
static int      sat_count   = 0;
static uint32_t last_sat_ms = 0;

// Time
static bool     time_init     = false;
static uint32_t last_sec_tick = 0;

// Init guard
static bool sim_initialised = false;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static float randf(float a, float b)
{
  return a + (b - a) * (float(rand()) / RAND_MAX);
}

static float clampf(float v, float lo, float hi)
{
  return v < lo ? lo : (v > hi ? hi : v);
}

static void ramp_speed()
{
  const float max_d = SPEED_RAMP_MPS2 * SIM_DT;
  const float diff  = target_mps - speed_mps;
  speed_mps += clampf(diff, -max_d, max_d);
}

static float m_to_deg_lat(float m)
{
  return m / 111111.0f;
}

static float m_to_deg_lon(float m, float lat)
{
  return m / (111111.0f * cos(lat * DEG_TO_RAD));
}

// UTC carry
static inline void utc_add_one_second()
{
  auto &p = ubxMessage.navPvt;
  if (++p.sec < 60) return;
  p.sec = 0;
  if (++p.min < 60) return;
  p.min = 0;
  if (++p.hour < 24) return;
  p.hour = 0;
  if (++p.day <= 28) return;
  p.day = 1;
  if (++p.month <= 12) return;
  p.month = 1;
  p.year++;
}

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------
void gps_simulator_init()
{
  sim_initialised = true;
  srand(esp_random());

  mode            = STRAIGHT;
  straight_dist   = 0.0f;
  wind_phase      = randf(0, TWO_PI);

  heading_deg     = 45.0f;
  speed_mps       = 0.0f;

  straight_base_mps =
    randf(STRAIGHT_MIN_KTS, STRAIGHT_MAX_KTS) * KNOTS_TO_MPS;
  target_mps = straight_base_mps;

  sim_ms        = 0;
  sat_count     = 0;
  last_sat_ms   = millis();

  time_init     = false;
  last_sec_tick = 0;
}

// -----------------------------------------------------------------------------
// Step
// -----------------------------------------------------------------------------
int gps_simulator_step()
{
  if (!sim_initialised) gps_simulator_init();

  static uint32_t last_emit_ms = 0;
  const uint32_t now = millis();
  if (now - last_emit_ms < SIM_RATE_MS) return MT_NONE;
  last_emit_ms += SIM_RATE_MS;
  sim_ms       += SIM_RATE_MS;

  // ---------------------------------------------------------------------------
  // UBX timing
  // ---------------------------------------------------------------------------
  ubxMessage.navPvt.iTOW = sim_ms;
  ubxMessage.navPvt.nano = (sim_ms % 1000) * 1000000UL;

  // ---------------------------------------------------------------------------
  // Satellite acquisition
  // ---------------------------------------------------------------------------
  if (sat_count < 10 && now - last_sat_ms >= 1500) {
    sat_count++;
    last_sat_ms = now;
  }

  ubxMessage.navPvt.numSV   = sat_count;
  ubxMessage.navPvt.fixType = (sat_count >= 5) ? 3 : 0;

  // ---------------------------------------------------------------------------
  // Time init + carry
  // ---------------------------------------------------------------------------
  if (ubxMessage.navPvt.fixType >= 3) {
    if (!time_init) {
      ubxMessage.navPvt.year  = 2026;
      ubxMessage.navPvt.month = rand()%12 + 1;
      ubxMessage.navPvt.day   = rand()%28 + 1;
      ubxMessage.navPvt.hour  = rand()%12 + 6;
      ubxMessage.navPvt.min   = rand()%60;
      ubxMessage.navPvt.sec   = rand()%60;
      ubxMessage.navPvt.valid = 0b111;
      last_sec_tick = sim_ms / 1000;
      time_init = true;
    }
    else if (sim_ms / 1000 != last_sec_tick) {
      last_sec_tick = sim_ms / 1000;
      utc_add_one_second();
    }
  }

  // ---------------------------------------------------------------------------
  // No fix → static
  // ---------------------------------------------------------------------------
  if (ubxMessage.navPvt.fixType < 3) {
    ubxMessage.navPvt.lat     = lat * 1e7;
    ubxMessage.navPvt.lon     = lon * 1e7;
    ubxMessage.navPvt.gSpeed  = 0;
    ubxMessage.navPvt.heading = heading_deg * 100000.0f;
    return MT_NAV_PVT;
  }

  // ---------------------------------------------------------------------------
  // Motion
  // ---------------------------------------------------------------------------
  if (mode == STRAIGHT) {
    // Wind-like speed variation
    wind_phase += TWO_PI * SIM_DT / WIND_OSC_PERIOD_S;
    if (wind_phase > TWO_PI) wind_phase -= TWO_PI;

    const float wind_mps =
      sinf(wind_phase) * STRAIGHT_WIND_AMPL_KTS * KNOTS_TO_MPS;

    target_mps = clampf(
      straight_base_mps + wind_mps,
      STRAIGHT_MIN_KTS * KNOTS_TO_MPS,
      STRAIGHT_MAX_KTS * KNOTS_TO_MPS
    );
  }

  ramp_speed();

  if (mode == STRAIGHT) {
    straight_dist += speed_mps * SIM_DT;

    lat += m_to_deg_lat(speed_mps * SIM_DT * cos(heading_deg * DEG_TO_RAD));
    lon += m_to_deg_lon(speed_mps * SIM_DT * sin(heading_deg * DEG_TO_RAD), lat);

    if (straight_dist >= STRAIGHT_LEN_M) {
      straight_dist = 0.0f;
      mode = TURN;

      target_mps = randf(TURN_MIN_KTS, TURN_MAX_KTS) * KNOTS_TO_MPS;

      const float h = heading_deg * DEG_TO_RAD;
      turn_dir_n = cos(h);
      turn_dir_e = sin(h);
      turn_nrm_n = -turn_dir_e;
      turn_nrm_e =  turn_dir_n;

      turn_center_lat = lat - m_to_deg_lat(turn_nrm_n * TURN_RADIUS_M);
      turn_center_lon = lon - m_to_deg_lon(turn_nrm_e * TURN_RADIUS_M, lat);

      turn_phi = 0.0f;
    }
  }
  else { // TURN
    turn_phi += speed_mps / TURN_RADIUS_M * SIM_DT;

    if (turn_phi >= TURN_ANGLE_RAD) {
      turn_phi = TURN_ANGLE_RAD;
      mode = STRAIGHT;

      straight_base_mps =
        randf(STRAIGHT_MIN_KTS, STRAIGHT_MAX_KTS) * KNOTS_TO_MPS;

      target_mps = straight_base_mps;
      heading_deg = fmodf(heading_deg + TURN_ANGLE_RAD * RAD_TO_DEG, 360.0f);
    }

    const float x = TURN_RADIUS_M * cos(turn_phi);
    const float y = TURN_RADIUS_M * sin(turn_phi);

    lat = turn_center_lat
        + m_to_deg_lat(turn_nrm_n * x + turn_dir_n * y);

    lon = turn_center_lon
        + m_to_deg_lon(turn_nrm_e * x + turn_dir_e * y, lat);

    heading_deg = atan2(
      turn_dir_e * cos(turn_phi) - turn_nrm_e * sin(turn_phi),
      turn_dir_n * cos(turn_phi) - turn_nrm_n * sin(turn_phi)
    ) * RAD_TO_DEG;

    if (heading_deg < 0) heading_deg += 360.0f;
  }

  // ---------------------------------------------------------------------------
  // Emit NAV-PVT
  // ---------------------------------------------------------------------------
  ubxMessage.navPvt.lat     = lat * 1e7;
  ubxMessage.navPvt.lon     = lon * 1e7;
  ubxMessage.navPvt.gSpeed  = speed_mps * 1000.0f;
  ubxMessage.navPvt.heading = heading_deg * 100000.0f;

  return MT_NAV_PVT;
}
