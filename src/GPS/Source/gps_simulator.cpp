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
#ifndef GPS_SIM_SPEEDUP
#define GPS_SIM_SPEEDUP 1
#endif

static constexpr float    SIM_DT          = 0.2f;     // 5 Hz
static constexpr uint32_t SIM_RATE_MS     = 200;
static constexpr float    KNOTS_TO_MPS    = 0.514444f;

static constexpr float STRAIGHT_MIN_KTS   = 28.0f;
static constexpr float STRAIGHT_MAX_KTS   = 42.0f;
static constexpr float TURN_MIN_KTS       = 5.0f;
static constexpr float TURN_EXIT_MIN_KTS  = 12.0f;
static constexpr float TURN_EXIT_MAX_KTS  = 24.0f;
static constexpr float TURN_DROP_MIN_KTS  = 10.0f;
static constexpr float TURN_DROP_MAX_KTS  = 16.0f;
static constexpr float TURN_RECOVERY_START_FRACTION = 0.58f;

static constexpr float SPEED_RAMP_MPS2    = 1.2f;     // accel/decel
static constexpr float TURN_SPEED_RAMP_MPS2 = 3.4f;   // stronger speed change through turns
static constexpr float STRAIGHT_LEN_MIN_M = 420.0f;
static constexpr float STRAIGHT_LEN_MAX_M = 780.0f;
static constexpr float TURN_RADIUS_MIN_M  = 18.0f;
static constexpr float TURN_RADIUS_MAX_M  = 44.0f;
static constexpr float TURN_ANGLE_MIN_DEG = 150.0f;
static constexpr float TURN_ANGLE_MAX_DEG = 208.0f;
static constexpr float POST_TURN_MIN_S    = 3.0f;
static constexpr float POST_TURN_MAX_S    = 12.0f;
static constexpr float POST_TURN_MIN_KTS  = 4.0f;
static constexpr float POST_TURN_MAX_KTS  = 12.0f;
static constexpr float COURSE_WANDER_DEG  = 2.4f;
static constexpr float COURSE_JITTER_DEG  = 0.55f;

// Speed texture
static constexpr float STRAIGHT_WIND_AMPL_KTS = 2.5f;   // ± knots
static constexpr float WIND_OSC_PERIOD_S     = 13.0f;  // seconds
static constexpr float GUST_AMPL_KTS         = 0.9f;   // knots
static constexpr float GUST_OSC_PERIOD_S     = 4.6f;   // seconds
static constexpr float RIPPLE_AMPL_KTS       = 0.22f;  // knots
static constexpr float RIPPLE_OSC_PERIOD_S   = 1.7f;   // seconds
static constexpr float COURSE_WANDER_PERIOD_S = 24.0f;
static constexpr float COURSE_JITTER_PERIOD_S = 3.7f;

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
static float straight_len_m    = 0.0f;
static float straight_base_mps = 0.0f;
static float leg_heading_deg   = 45.0f;
static float recovery_time_s   = 0.0f;
static float recovery_mps      = 0.0f;
static float wind_phase        = 0.0f;
static float gust_phase        = 0.0f;
static float ripple_phase      = 0.0f;
static float course_phase      = 0.0f;
static float jitter_phase      = 0.0f;

// Turn geometry
static float turn_phi        = 0.0f;
static float turn_radius_m   = 24.0f;
static float turn_angle_rad  = 180.0f * DEG_TO_RAD;
static float turn_center_lat = 0.0f;
static float turn_center_lon = 0.0f;
static float turn_dir_n      = 0.0f;
static float turn_dir_e      = 0.0f;
static float turn_nrm_n      = 0.0f;
static float turn_nrm_e      = 0.0f;
static float turn_entry_mps  = 0.0f;
static float turn_min_mps    = 0.0f;
static float turn_exit_mps   = 0.0f;

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
static uint32_t last_wall_ms = 0;
static float emit_credit = 0.0f;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static float randf(float a, float b)
{
  return a + (b - a) * (float(rand()) / RAND_MAX);
}

static float normalize_deg(float deg)
{
  while (deg >= 360.0f) deg -= 360.0f;
  while (deg < 0.0f) deg += 360.0f;
  return deg;
}

static float clampf(float v, float lo, float hi)
{
  return v < lo ? lo : (v > hi ? hi : v);
}

static void ramp_speed()
{
  const float ramp = mode == TURN ? TURN_SPEED_RAMP_MPS2 : SPEED_RAMP_MPS2;
  const float max_d = ramp * SIM_DT;
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

static void choose_next_straight()
{
  straight_len_m = randf(STRAIGHT_LEN_MIN_M, STRAIGHT_LEN_MAX_M);
  straight_base_mps = randf(31.0f, 39.0f) * KNOTS_TO_MPS;
  recovery_time_s = randf(POST_TURN_MIN_S, POST_TURN_MAX_S);
  recovery_mps = randf(POST_TURN_MIN_KTS, POST_TURN_MAX_KTS) * KNOTS_TO_MPS;
  wind_phase = randf(0.0f, TWO_PI);
  gust_phase = randf(0.0f, TWO_PI);
  ripple_phase = randf(0.0f, TWO_PI);
  course_phase = randf(0.0f, TWO_PI);
  jitter_phase = randf(0.0f, TWO_PI);
}

static void choose_next_turn()
{
  turn_radius_m = randf(TURN_RADIUS_MIN_M, TURN_RADIUS_MAX_M);
  turn_angle_rad = randf(TURN_ANGLE_MIN_DEG, TURN_ANGLE_MAX_DEG) * DEG_TO_RAD;
  turn_entry_mps = speed_mps;
  turn_min_mps = turn_entry_mps - randf(TURN_DROP_MIN_KTS, TURN_DROP_MAX_KTS) * KNOTS_TO_MPS;
  if (turn_min_mps < TURN_MIN_KTS * KNOTS_TO_MPS) {
    turn_min_mps = TURN_MIN_KTS * KNOTS_TO_MPS;
  }

  turn_exit_mps = randf(TURN_EXIT_MIN_KTS, TURN_EXIT_MAX_KTS) * KNOTS_TO_MPS;
  if (turn_exit_mps < turn_min_mps + 4.0f * KNOTS_TO_MPS) {
    turn_exit_mps = turn_min_mps + 4.0f * KNOTS_TO_MPS;
  }

  target_mps = turn_min_mps;
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

  leg_heading_deg = randf(35.0f, 55.0f);
  heading_deg     = leg_heading_deg;
  speed_mps       = 0.0f;

  choose_next_straight();
  recovery_time_s = 0.0f;
  target_mps = straight_base_mps;

  sim_ms        = 0;
  sat_count     = 10;
  last_sat_ms   = millis();

  time_init     = false;
  last_sec_tick = 0;
  last_wall_ms  = millis();
  emit_credit   = 0.0f;
}

// -----------------------------------------------------------------------------
// Step
// -----------------------------------------------------------------------------
int gps_simulator_step()
{
  if (!sim_initialised) gps_simulator_init();

  const uint32_t now = millis();
  const uint32_t elapsed_ms = now - last_wall_ms;
  last_wall_ms = now;

  emit_credit += (elapsed_ms * static_cast<float>(GPS_SIM_SPEEDUP)) / SIM_RATE_MS;
  if (emit_credit < 1.0f) return MT_NONE;
  emit_credit -= 1.0f;

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
    // Wind-like speed variation. Multi-frequency texture avoids long clipped
    // plateaus, which makes Speedreader window comparison more useful.
    wind_phase += TWO_PI * SIM_DT / WIND_OSC_PERIOD_S;
    if (wind_phase > TWO_PI) wind_phase -= TWO_PI;
    gust_phase += TWO_PI * SIM_DT / GUST_OSC_PERIOD_S;
    if (gust_phase > TWO_PI) gust_phase -= TWO_PI;
    ripple_phase += TWO_PI * SIM_DT / RIPPLE_OSC_PERIOD_S;
    if (ripple_phase > TWO_PI) ripple_phase -= TWO_PI;
    course_phase += TWO_PI * SIM_DT / COURSE_WANDER_PERIOD_S;
    if (course_phase > TWO_PI) course_phase -= TWO_PI;
    jitter_phase += TWO_PI * SIM_DT / COURSE_JITTER_PERIOD_S;
    if (jitter_phase > TWO_PI) jitter_phase -= TWO_PI;

    const float wind_mps =
      sinf(wind_phase) * STRAIGHT_WIND_AMPL_KTS * KNOTS_TO_MPS;
    const float gust_mps =
      sinf(gust_phase + 0.7f * sinf(wind_phase)) * GUST_AMPL_KTS * KNOTS_TO_MPS;
    const float ripple_mps =
      sinf(ripple_phase) * RIPPLE_AMPL_KTS * KNOTS_TO_MPS;

    if (recovery_time_s > 0.0f) {
      recovery_time_s -= SIM_DT;
      target_mps = recovery_mps + 0.45f * gust_mps + ripple_mps;
    }
    else {
      target_mps = clampf(
        straight_base_mps + wind_mps + gust_mps + ripple_mps,
        STRAIGHT_MIN_KTS * KNOTS_TO_MPS,
        STRAIGHT_MAX_KTS * KNOTS_TO_MPS
      );
    }

    const float course_wander =
      sinf(course_phase) * COURSE_WANDER_DEG +
      sinf(jitter_phase + 0.4f * sinf(gust_phase)) * COURSE_JITTER_DEG;
    heading_deg = normalize_deg(leg_heading_deg + course_wander);
  }
  else {
    const float progress = turn_angle_rad > 0.0f
        ? clampf(turn_phi / turn_angle_rad, 0.0f, 1.0f)
        : 1.0f;

    if (progress < TURN_RECOVERY_START_FRACTION) {
      target_mps = turn_min_mps;
    }
    else {
      const float t = (progress - TURN_RECOVERY_START_FRACTION) /
                      (1.0f - TURN_RECOVERY_START_FRACTION);
      const float smooth = t * t * (3.0f - 2.0f * t);
      target_mps = turn_min_mps + (turn_exit_mps - turn_min_mps) * smooth;
    }
  }

  ramp_speed();

  if (mode == STRAIGHT) {
    straight_dist += speed_mps * SIM_DT;

    lat += m_to_deg_lat(speed_mps * SIM_DT * cos(heading_deg * DEG_TO_RAD));
    lon += m_to_deg_lon(speed_mps * SIM_DT * sin(heading_deg * DEG_TO_RAD), lat);

    if (straight_dist >= straight_len_m) {
      straight_dist = 0.0f;
      mode = TURN;

      choose_next_turn();

      const float h = heading_deg * DEG_TO_RAD;
      turn_dir_n = cos(h);
      turn_dir_e = sin(h);
      turn_nrm_n = -turn_dir_e;
      turn_nrm_e =  turn_dir_n;

      turn_center_lat = lat - m_to_deg_lat(turn_nrm_n * turn_radius_m);
      turn_center_lon = lon - m_to_deg_lon(turn_nrm_e * turn_radius_m, lat);

      turn_phi = 0.0f;
    }
  }
  else { // TURN
    turn_phi += speed_mps / turn_radius_m * SIM_DT;
    bool turn_finished = false;

    if (turn_phi >= turn_angle_rad) {
      turn_phi = turn_angle_rad;
      turn_finished = true;
    }

    const float x = turn_radius_m * cos(turn_phi);
    const float y = turn_radius_m * sin(turn_phi);

    lat = turn_center_lat
        + m_to_deg_lat(turn_nrm_n * x + turn_dir_n * y);

    lon = turn_center_lon
        + m_to_deg_lon(turn_nrm_e * x + turn_dir_e * y, lat);

    heading_deg = atan2(
      turn_dir_e * cos(turn_phi) - turn_nrm_e * sin(turn_phi),
      turn_dir_n * cos(turn_phi) - turn_nrm_n * sin(turn_phi)
    ) * RAD_TO_DEG;

    if (heading_deg < 0) heading_deg += 360.0f;

    if (turn_finished) {
      mode = STRAIGHT;
      straight_dist = 0.0f;
      leg_heading_deg = normalize_deg(heading_deg + randf(-7.0f, 7.0f));
      choose_next_straight();
      target_mps = recovery_mps;
    }
  }

  // ---------------------------------------------------------------------------
  // Emit NAV-PVT
  // ---------------------------------------------------------------------------
  ubxMessage.navPvt.lat     = lat * 1e7;
  ubxMessage.navPvt.lon     = lon * 1e7;
  ubxMessage.navPvt.gSpeed  = speed_mps * 1000.0f;
  ubxMessage.navPvt.heading = heading_deg * 100000.0f;
  ubxMessage.navPvt.velN    = speed_mps * 1000.0f * cos(heading_deg * DEG_TO_RAD);
  ubxMessage.navPvt.velE    = speed_mps * 1000.0f * sin(heading_deg * DEG_TO_RAD);
  ubxMessage.navPvt.velD    = 0;
  ubxMessage.navPvt.sAcc    = static_cast<uint32_t>(randf(150.0f, 360.0f));
  ubxMessage.navPvt.hAcc    = static_cast<uint32_t>(randf(400.0f, 1100.0f));
  ubxMessage.navPvt.vAcc    = static_cast<uint32_t>(randf(700.0f, 1600.0f));
  ubxMessage.navPvt.headAcc = static_cast<uint32_t>(randf(30000.0f, 90000.0f));
  ubxMessage.navPvt.pDOP    = 120;
  ubxMessage.navDOP.hDOP    = 120;

  return MT_NAV_PVT;
}
