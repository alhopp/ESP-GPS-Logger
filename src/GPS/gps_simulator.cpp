// -----------------------------------------------------------------------------
// gps_simulator.cpp
// NAV-PVT simulator (fixed 5 Hz tick, REAL TIME)
//
// Guarantees:
// - Monotonic UTC (no minute/hour rollback)
// - Stable iTOW + nano for SBP
// - No turn-exit chord / jump
//
// Realism:
// - Heading wander (±5°)
// - Lateral slop (±6 m)
// - Speed texture
// - Rare “oops” events
// -----------------------------------------------------------------------------

#include "GPS/gps_simulator.h"
#include "Ublox/Ublox.h"

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Demo tracks
// -----------------------------------------------------------------------------
struct DemoTrack{ const char *name; double a,b,c,d; };

static const DemoTrack DEMO_TRACKS[] = {
  { "Melville",  -32.02359564767922,115.82221434124064, -32.01173340024278,115.80573484987477 },
  { "Albany",    -35.05076619337528,117.86558258089270, -35.03444301790581,117.85398982270590 },
  { "Mandurah",  -32.57086521672320,115.75513888479900, -32.56995256554500,115.72798730137100 }
};
static constexpr int NUM_TRACKS = sizeof(DEMO_TRACKS)/sizeof(DEMO_TRACKS[0]);

static double START_LAT,START_LON,END_LAT,END_LON;

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
static constexpr float    KNOTS_TO_MPS = 0.514444f;
static constexpr uint32_t SIM_RATE_MS  = 200;
static constexpr double   DT           = 0.2;

static constexpr double STRAIGHT_MIN_KTS = 30.0;
static constexpr double STRAIGHT_MAX_KTS = 40.0;
static constexpr double TURN_TARGET_KTS  = 15.0;

static constexpr double SPEED_RAMP_MPS2  = 0.8;
static constexpr double TURN_RADIUS_M    = 25.0;

static constexpr double HEADING_WANDER_MAX = 5.0;
static constexpr double LATERAL_SLOP_MAX   = 6.0;
static constexpr double SPEED_JITTER_MAX   = 0.30;

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------
static uint32_t last_emit_ms=0, sim_ms=0, last_sat_ms=0;
static bool sim_initialised=false;
static int  sat_count=0;

static double lat=0, lon=0;
static double speed_mps=0, target_speed_mps=0, heading_deg=0;

static double track_len_m=0, track_pos_m=0;
static double dir_n=0, dir_e=0, nrm_n=0, nrm_e=0;
static int    track_dir=+1;
static double leg_offset_m=+TURN_RADIUS_M;

enum { MODE_STRAIGHT, MODE_TURN };
static int mode=MODE_STRAIGHT;

static double turn_center_lat=0, turn_center_lon=0;
static double turn_phi=0, turn_fwd_n=0, turn_fwd_e=0, turn_nrm_n=0, turn_nrm_e=0;

// Realism noise
static double heading_bias=0, heading_rate=0;
static double lateral_noise=0, lateral_rate=0;
static int    exit_blend_ticks=0;

// Time
static bool     time_init=false;
static uint32_t last_sec_tick=0;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static inline int rand_int(int a,int b){ return a + rand()%(b-a+1); }
static inline double randf(double a,double b){ return a + (b-a)*(double(rand())/RAND_MAX); }
static inline double clampd(double v,double lo,double hi){ return v<lo?lo:v>hi?hi:v; }

static inline void ramp_speed(){
  const double max_d = SPEED_RAMP_MPS2 * DT;
  const double diff  = target_speed_mps - speed_mps;
  speed_mps += clampd(diff, -max_d, max_d);
}

static inline double heading_from_vec(double n,double e){
  double h = atan2(e,n) * RAD_TO_DEG;
  return h<0?h+360:h;
}

static inline double m_to_deg_lat(double m){ return m/111111.0; }
static inline double m_to_deg_lon(double m,double lat){
  return m/(111111.0*cos(lat*DEG_TO_RAD));
}

// UTC carry (simple calendar)
static inline void utc_add_one_second(){
  auto &p = ubxMessage.navPvt;
  if(++p.sec<60) return; p.sec=0;
  if(++p.min<60) return; p.min=0;
  if(++p.hour<24) return; p.hour=0;
  if(++p.day<=28) return; p.day=1;
  if(++p.month<=12) return; p.month=1;
  p.year++;
}

// Noise update
static inline void update_noise(){
  heading_rate += randf(-0.2,0.2);
  heading_rate *= 0.95;
  heading_bias += heading_rate * DT;
  heading_bias  = clampd(heading_bias,-HEADING_WANDER_MAX,HEADING_WANDER_MAX);

  lateral_rate += randf(-0.05,0.05);
  lateral_rate *= 0.98;
  lateral_noise += lateral_rate * DT;
  lateral_noise  = clampd(lateral_noise,-LATERAL_SLOP_MAX,LATERAL_SLOP_MAX);

  if(exit_blend_ticks>0){
    const double w = double(exit_blend_ticks)/5.0;
    heading_bias *= w;
    lateral_noise*= w;
    exit_blend_ticks--;
  }

  if((rand()%800)==0 && mode==MODE_STRAIGHT){
    target_speed_mps *= randf(0.7,0.9);
    heading_rate += randf(-2.0,2.0);
  }
}

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------
void gps_simulator_init(){
  sim_initialised=true;
  last_emit_ms=millis();
  sim_ms=0; sat_count=0; last_sat_ms=last_emit_ms;
  heading_bias=heading_rate=0;
  lateral_noise=lateral_rate=0;
  exit_blend_ticks=0;
  time_init=false;

  static bool seeded=false;
  if(!seeded){ srand(esp_random()); seeded=true; }

  const int i=rand_int(0,NUM_TRACKS-1);
  START_LAT=DEMO_TRACKS[i].a; START_LON=DEMO_TRACKS[i].b;
  END_LAT  =DEMO_TRACKS[i].c; END_LON  =DEMO_TRACKS[i].d;

  const double dn=(END_LAT-START_LAT)*111111.0;
  const double de=(END_LON-START_LON)*111111.0*cos(START_LAT*DEG_TO_RAD);

  track_len_m=sqrt(dn*dn+de*de);
  dir_n=dn/track_len_m; dir_e=de/track_len_m;
  nrm_n=-dir_e; nrm_e=dir_n;

  track_pos_m=0; track_dir=+1; leg_offset_m=+TURN_RADIUS_M;
  target_speed_mps=randf(STRAIGHT_MIN_KTS,STRAIGHT_MAX_KTS)*KNOTS_TO_MPS;
  speed_mps=target_speed_mps;
  heading_deg=heading_from_vec(dir_n,dir_e);

  Serial.printf("[SIM ] Track=%s\n",DEMO_TRACKS[i].name);
}

// -----------------------------------------------------------------------------
// Step
// -----------------------------------------------------------------------------
int gps_simulator_step(){
  if(!sim_initialised) gps_simulator_init();

  const uint32_t now=millis();
  if(now-last_emit_ms<SIM_RATE_MS) return MT_NONE;
  last_emit_ms+=SIM_RATE_MS;
  sim_ms+=SIM_RATE_MS;

  ubxMessage.navPvt.iTOW = sim_ms;
  ubxMessage.navPvt.nano = (sim_ms%1000)*1000000UL;

  if(sat_count<10 && now-last_sat_ms>=1500){ sat_count++; last_sat_ms=now; }
  ubxMessage.navPvt.numSV=sat_count;
  ubxMessage.navPvt.fixType=(sat_count>=5)?3:0;
  if(ubxMessage.navPvt.fixType<3) return MT_NAV_PVT;

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
  } else if(sim_ms/1000!=last_sec_tick){
    last_sec_tick=sim_ms/1000;
    utc_add_one_second();
  }

  update_noise();
  ramp_speed();

  // -------------------------------------------------------------------------
  // Motion
  // -------------------------------------------------------------------------
  if(mode==MODE_STRAIGHT){
    track_pos_m+=speed_mps*DT*track_dir;
    bool hit=(track_pos_m>=track_len_m)||(track_pos_m<=0);
    track_pos_m=clampd(track_pos_m,0,track_len_m);

    const double mn=dir_n*track_pos_m;
    const double me=dir_e*track_pos_m;
    const double off=leg_offset_m+lateral_noise;

    lat=START_LAT+m_to_deg_lat(mn+nrm_n*off);
    lon=START_LON+m_to_deg_lon(me+nrm_e*off,lat);
    heading_deg=heading_from_vec(dir_n*track_dir,dir_e*track_dir)+heading_bias;

    if(hit){
      target_speed_mps=TURN_TARGET_KTS*KNOTS_TO_MPS;
      turn_center_lat=lat-m_to_deg_lat(nrm_n*leg_offset_m);
      turn_center_lon=lon-m_to_deg_lon(nrm_e*leg_offset_m,lat);
      turn_fwd_n=dir_n*track_dir; turn_fwd_e=dir_e*track_dir;
      turn_nrm_n=-turn_fwd_e; turn_nrm_e=turn_fwd_n;
      turn_phi=0; mode=MODE_TURN;
    }
  } else {
    turn_phi+=speed_mps/ TURN_RADIUS_M * DT;

    if(turn_phi>=M_PI){
      turn_phi=M_PI;
      track_dir*=-1;
      leg_offset_m=-leg_offset_m;
      exit_blend_ticks=5;
      mode=MODE_STRAIGHT;
      return MT_NAV_PVT;   // <- KEY FIX: no straight recompute this tick
    }

    const double x=TURN_RADIUS_M*cos(turn_phi);
    const double y=TURN_RADIUS_M*sin(turn_phi);
    const double xs=(leg_offset_m>0)?x:-x;
    const double sl=lateral_noise*0.3;

    lat=turn_center_lat+m_to_deg_lat(turn_nrm_n*(xs+sl)+turn_fwd_n*y);
    lon=turn_center_lon+m_to_deg_lon(turn_nrm_e*(xs+sl)+turn_fwd_e*y,turn_center_lat);

    heading_deg=heading_from_vec(
      -turn_nrm_n*sin(turn_phi)+turn_fwd_n*cos(turn_phi),
      -turn_nrm_e*sin(turn_phi)+turn_fwd_e*cos(turn_phi)
    )+heading_bias;
  }

  const double spd_j=randf(-SPEED_JITTER_MAX,SPEED_JITTER_MAX);

  ubxMessage.navPvt.lat     = lat*1e7;
  ubxMessage.navPvt.lon     = lon*1e7;
  ubxMessage.navPvt.gSpeed  = (speed_mps+spd_j)*1000.0;
  ubxMessage.navPvt.heading = heading_deg*100000.0;

  return MT_NAV_PVT;
}
