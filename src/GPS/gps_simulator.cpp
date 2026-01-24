// -----------------------------------------------------------------------------
// gps_simulator.cpp
// NAV-PVT simulator (fixed 5 Hz tick, REAL TIME)
//
// - Random demo track per run (Melville / Albany / Mandurah)
// - Parallel straight legs with fixed separation
// - TRUE semicircle U-turns (radius = 25 m)
// - Speed realism:
//     * Straights: smoothly wander between 30–40 kn
//     * Turns: ramp down to ~15 kn, ramp back up on exit
// - Exactly 5 Hz logging (200 ms), normal timestamps
// -----------------------------------------------------------------------------

#include "GPS/gps_simulator.h"
#include "Ublox/Ublox.h"

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Demo tracks
// -----------------------------------------------------------------------------
struct DemoTrack { const char *name; double start_lat,start_lon,end_lat,end_lon; };

static const DemoTrack DEMO_TRACKS[] = {
  { "Melville",  -32.02359564767922,115.82221434124064, -32.01173340024278,115.80573484987477 },
  { "Albany",    -35.05076619337528,117.86558258089270, -35.03444301790581,117.85398982270590 },
  { "Mandurah",  -32.57086521672320,115.75513888479900, -32.56995256554500,115.72798730137100 }
};
static constexpr int NUM_TRACKS = sizeof(DEMO_TRACKS)/sizeof(DEMO_TRACKS[0]);

static double START_LAT, START_LON, END_LAT, END_LON;

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
static constexpr float KNOTS_TO_MPS = 0.514444f;

static constexpr uint32_t SIM_RATE_MS = 200;     // 5 Hz
static constexpr double  DT = 0.2;               // seconds

// Speed targets
static constexpr double STRAIGHT_MIN_KTS = 30.0;
static constexpr double STRAIGHT_MAX_KTS = 40.0;
static constexpr double TURN_TARGET_KTS  = 15.0;

// Speed ramping (m/s²)
static constexpr double SPEED_RAMP_MPS2 = 0.8;

// Turn geometry
static constexpr double TURN_RADIUS_M = 25.0;
static constexpr double LEG_SEP_M     = 2.0 * TURN_RADIUS_M;

// -----------------------------------------------------------------------------
// Simulator state
// -----------------------------------------------------------------------------
static uint32_t last_emit_ms=0, sim_ms=0, last_sat_ms=0;
static bool sim_initialised=false;
static int  sat_count=0;

static double lat=0.0, lon=0.0;
static double speed_mps=0.0;
static double target_speed_mps=0.0;
static double heading_deg=0.0;

// Track geometry
static double track_len_m=0.0;
static double track_pos_m=0.0;

static double dir_n=0.0, dir_e=0.0;
static double nrm_n=0.0, nrm_e=0.0;

static int track_dir=+1;
static double leg_offset_m=+TURN_RADIUS_M;

// Turn state
enum { MODE_STRAIGHT, MODE_TURN };
static int mode = MODE_STRAIGHT;

static double turn_center_lat=0.0, turn_center_lon=0.0;
static double turn_phi=0.0;
static double turn_fwd_n=0.0, turn_fwd_e=0.0;
static double turn_nrm_n=0.0, turn_nrm_e=0.0;

// Time
static bool time_init=false;
static uint32_t last_sec_tick=0;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static inline int rand_int(int a,int b){ return a + (rand() % (b-a+1)); }
static inline double randf(double a,double b){
  return a + (b-a) * (double(rand()) / RAND_MAX);
}

static inline void ramp_speed(double dt){
  const double max_delta = SPEED_RAMP_MPS2 * dt;
  const double diff = target_speed_mps - speed_mps;
  if(fabs(diff) <= max_delta) speed_mps = target_speed_mps;
  else speed_mps += (diff > 0 ? max_delta : -max_delta);
}

static inline double heading_from_vec(double n,double e){
  double h = atan2(e,n) * RAD_TO_DEG;
  if(h < 0) h += 360.0;
  return h;
}

static inline double m_to_deg_lat(double m){ return m / 111111.0; }
static inline double m_to_deg_lon(double m,double at_lat){
  return m / (111111.0 * cos(at_lat * DEG_TO_RAD));
}

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------
void gps_simulator_init()
{
  sim_initialised=true;
  last_emit_ms=millis();
  sim_ms=0;
  sat_count=0;
  last_sat_ms=last_emit_ms;
  time_init=false;

  static bool seeded=false;
  if(!seeded){ srand((unsigned)esp_random()); seeded=true; }

  const int idx = rand_int(0, NUM_TRACKS-1);
  START_LAT = DEMO_TRACKS[idx].start_lat;
  START_LON = DEMO_TRACKS[idx].start_lon;
  END_LAT   = DEMO_TRACKS[idx].end_lat;
  END_LON   = DEMO_TRACKS[idx].end_lon;

  Serial.printf("[SIM ] Track=%s  R=%.0fm\n", DEMO_TRACKS[idx].name, TURN_RADIUS_M);

  const double dlat_m = (END_LAT-START_LAT) * 111111.0;
  const double dlon_m = (END_LON-START_LON) * 111111.0 * cos(START_LAT * DEG_TO_RAD);

  track_len_m = sqrt(dlat_m*dlat_m + dlon_m*dlon_m);
  dir_n = dlat_m / track_len_m;
  dir_e = dlon_m / track_len_m;
  nrm_n = -dir_e;
  nrm_e =  dir_n;

  track_pos_m = 0.0;
  track_dir   = +1;
  leg_offset_m = +TURN_RADIUS_M;
  mode = MODE_STRAIGHT;

  target_speed_mps = randf(STRAIGHT_MIN_KTS, STRAIGHT_MAX_KTS) * KNOTS_TO_MPS;
  speed_mps = target_speed_mps;

  heading_deg = heading_from_vec(dir_n, dir_e);
}

// -----------------------------------------------------------------------------
// Step (5 Hz)
// -----------------------------------------------------------------------------
int gps_simulator_step()
{
  if(!sim_initialised) gps_simulator_init();

  const uint32_t now=millis();
  if(now-last_emit_ms < SIM_RATE_MS) return MT_NONE;
  last_emit_ms += SIM_RATE_MS;

  sim_ms += SIM_RATE_MS;
  ubxMessage.navPvt.iTOW = sim_ms;

  // Satellites
  if(sat_count < 10 && (now-last_sat_ms) >= 1500){ sat_count++; last_sat_ms=now; }
  ubxMessage.navPvt.numSV   = sat_count;
  ubxMessage.navPvt.fixType = (sat_count >= 5) ? 3 : 0;

  if(ubxMessage.navPvt.fixType < 3){
    ubxMessage.navPvt.gSpeed = 0;
    return MT_NAV_PVT;
  }

  // Time (always 2026)
  if(!time_init){
    ubxMessage.navPvt.year=2026;
    ubxMessage.navPvt.month=rand_int(1,12);
    ubxMessage.navPvt.day=rand_int(1,28);
    ubxMessage.navPvt.hour=rand_int(6,18);
    ubxMessage.navPvt.min=rand_int(0,59);
    ubxMessage.navPvt.sec=rand_int(0,59);
    ubxMessage.navPvt.valid=0b111;
    last_sec_tick=sim_ms/1000;
    time_init=true;
  } else {
    const uint32_t sec=sim_ms/1000;
    if(sec!=last_sec_tick){ last_sec_tick=sec; ubxMessage.navPvt.sec=(ubxMessage.navPvt.sec+1)%60; }
  }

  // ---------------- Motion ----------------
  if(mode==MODE_STRAIGHT){
    if(rand()%20==0)  // gentle wander
      target_speed_mps = randf(STRAIGHT_MIN_KTS,STRAIGHT_MAX_KTS)*KNOTS_TO_MPS;

    ramp_speed(DT);
    track_pos_m += speed_mps * DT * track_dir;

    bool hit_end=false;
    if(track_pos_m>=track_len_m){ track_pos_m=track_len_m; hit_end=true; }
    if(track_pos_m<=0){ track_pos_m=0; hit_end=true; }

    const double mn = dir_n * track_pos_m;
    const double me = dir_e * track_pos_m;

    lat = START_LAT + m_to_deg_lat(mn + nrm_n*leg_offset_m);
    lon = START_LON + m_to_deg_lon(me + nrm_e*leg_offset_m, lat);
    heading_deg = heading_from_vec(dir_n*track_dir, dir_e*track_dir);

    if(hit_end){
      target_speed_mps = TURN_TARGET_KTS * KNOTS_TO_MPS;
      turn_center_lat = lat - m_to_deg_lat(nrm_n*leg_offset_m);
      turn_center_lon = lon - m_to_deg_lon(nrm_e*leg_offset_m, lat);
      turn_fwd_n = dir_n * track_dir;
      turn_fwd_e = dir_e * track_dir;
      turn_nrm_n = -turn_fwd_e;
      turn_nrm_e =  turn_fwd_n;
      turn_phi=0;
      mode=MODE_TURN;
    }
  }

  else {
    ramp_speed(DT);
    const double omega = speed_mps / TURN_RADIUS_M;
    turn_phi += omega * DT;

    if(turn_phi>=M_PI){
      turn_phi=M_PI;
      track_dir*=-1;
      leg_offset_m=-leg_offset_m;
      target_speed_mps = randf(STRAIGHT_MIN_KTS,STRAIGHT_MAX_KTS)*KNOTS_TO_MPS;
      mode=MODE_STRAIGHT;
    }

    const double x = TURN_RADIUS_M * cos(turn_phi);
    const double y = TURN_RADIUS_M * sin(turn_phi);
    const double xs = (leg_offset_m>0)?x:-x;

    lat = turn_center_lat + m_to_deg_lat(turn_nrm_n*xs + turn_fwd_n*y);
    lon = turn_center_lon + m_to_deg_lon(turn_nrm_e*xs + turn_fwd_e*y, turn_center_lat);

    const double tn = -turn_nrm_n*sin(turn_phi) + turn_fwd_n*cos(turn_phi);
    const double te = -turn_nrm_e*sin(turn_phi) + turn_fwd_e*cos(turn_phi);
    heading_deg = heading_from_vec(tn,te);
  }

  ubxMessage.navPvt.lat = lat*1e7;
  ubxMessage.navPvt.lon = lon*1e7;
  ubxMessage.navPvt.gSpeed = speed_mps*1000.0;
  ubxMessage.navPvt.heading = heading_deg*100000.0;

  return MT_NAV_PVT;
}
