// -----------------------------------------------------------------------------
// gps_simulator.cpp (NAV-PVT simulator, fixed 5 Hz tick)
// -----------------------------------------------------------------------------

#include "gps_simulator.h"
#include "Ublox/Ublox.h"

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
static constexpr float STRAIGHT_DIST_M=200.0f, KNOTS_TO_MPS=0.514444f, STRAIGHT_KTS=40.0f, TURN_KTS=20.0f;
static constexpr uint32_t SIM_RATE_MS=200; // 5 Hz
static constexpr double SPEED_RAMP_MPS2 = 1.8; // ~0.35 kn/s accel/decel

// -----------------------------------------------------------------------------
// Simulator state
// -----------------------------------------------------------------------------
static uint32_t last_emit_ms=0, sim_ms=0, last_step_ms=0, last_sat_step=0;
static bool sim_initialised=false;
static int sat_count=0;

static double lat=-32.014400, lon=115.850700;
static double heading_deg=0.0, speed_mps=STRAIGHT_KTS*KNOTS_TO_MPS, leg_distance=0.0;

static bool turning=false;
static double turn_radius=30.0, turn_progress_deg=0.0, turn_rate_deg_per_sec=0.0;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static inline double randf(double minv,double maxv){ return minv+(maxv-minv)*(double(rand())/RAND_MAX); }

static inline void ramp_speed(double target, double dt)
{
  const double max_delta = SPEED_RAMP_MPS2 * dt;
  const double diff = target - speed_mps;

  if (fabs(diff) <= max_delta) speed_mps = target;
  else speed_mps += (diff > 0 ? max_delta : -max_delta);
}


// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------
void gps_simulator_init()
{
  sim_initialised=true;
  sim_ms=0; last_step_ms=millis(); last_sat_step=last_step_ms; last_emit_ms=last_step_ms;
  sat_count=0;
  lat=-32.014400; lon=115.850700;
  heading_deg=0.0; speed_mps=STRAIGHT_KTS*KNOTS_TO_MPS; leg_distance=0.0;
  turning=false; turn_radius=30.0; turn_progress_deg=0.0; turn_rate_deg_per_sec=0.0;
}

// -----------------------------------------------------------------------------
// One simulation step
// -----------------------------------------------------------------------------
int gps_simulator_step()
{
  if(!sim_initialised) gps_simulator_init();

  const uint32_t now=millis();
  if(now-last_emit_ms<SIM_RATE_MS) return MT_NONE;
  last_emit_ms+=SIM_RATE_MS;

  constexpr float dt=SIM_RATE_MS/1000.0f; // 0.2 s
  sim_ms+=SIM_RATE_MS; ubxMessage.navPvt.iTOW=sim_ms;

  // advance simulated UTC seconds
  static uint32_t last_sec_tick=0;
  const uint32_t sec_tick=sim_ms/1000;
  if(sec_tick!=last_sec_tick){ last_sec_tick=sec_tick; ubxMessage.navPvt.sec++; if(ubxMessage.navPvt.sec>=60) ubxMessage.navPvt.sec=0; }

  // satellite acquisition (~15s)
  if(sat_count<10 && (now-last_sat_step)>=1500){ sat_count++; last_sat_step=now; }

  ubxMessage.navPvt.numSV=sat_count;
  ubxMessage.navPvt.fixType=(sat_count>=5)?3:0;
  ubxMessage.navPvt.sAcc=800;
  ubxMessage.navPvt.valid=7;

  // no motion until 3D fix
  if(ubxMessage.navPvt.fixType<3){
    ubxMessage.navPvt.gSpeed=0;
    ubxMessage.navPvt.heading=heading_deg*100000.0;
    ubxMessage.navPvt.nano=0;
    return MT_NAV_PVT;
  }

  ubxMessage.navDOP.hDOP=120;
  ubxMessage.navPvt.velD=0;

  // motion model
  if(!turning){
  const double target = STRAIGHT_KTS * KNOTS_TO_MPS;
  ramp_speed(target, dt);
  speed_mps += randf(-0.15, 0.15) * KNOTS_TO_MPS;

  const double d=speed_mps*dt;
  leg_distance+=d;

  if(leg_distance>=STRAIGHT_DIST_M){
    turning=true; leg_distance=0.0;
    turn_radius=randf(25.0,35.0);
    turn_progress_deg=0.0;
    const double omega=speed_mps/turn_radius;
    turn_rate_deg_per_sec=omega*(180.0/M_PI);
    }
  }else{
    const double target = TURN_KTS * KNOTS_TO_MPS;
    ramp_speed(target, dt);
    speed_mps += randf(-0.10, 0.10) * KNOTS_TO_MPS;

    const double dHead=turn_rate_deg_per_sec*dt;
    heading_deg+=dHead; turn_progress_deg+=fabs(dHead);

    if(turn_progress_deg>=180.0){ heading_deg=fmod(heading_deg,360.0); turning=false; }
  }


  // position update (local earth approx)
  const double d=speed_mps*dt;
  lat+=(d/111111.0)*cos(heading_deg*DEG_TO_RAD);
  lon+=(d/(111111.0*cos(lat*DEG_TO_RAD)))*sin(heading_deg*DEG_TO_RAD);

  // populate NAV-PVT (raw)
  ubxMessage.navPvt.lat=lat*1e7;
  ubxMessage.navPvt.lon=lon*1e7;
  ubxMessage.navPvt.gSpeed=speed_mps*1000.0;
  ubxMessage.navPvt.heading=heading_deg*100000.0;

  // simulated UTC (init once after fix)
  static bool time_init=false;
  if(!time_init && ubxMessage.navPvt.fixType>=3){
    ubxMessage.navPvt.year=2026; ubxMessage.navPvt.month=1; ubxMessage.navPvt.day=10;
    ubxMessage.navPvt.hour=8; ubxMessage.navPvt.min=30; ubxMessage.navPvt.sec=0;
    ubxMessage.navPvt.nano=0; ubxMessage.navPvt.tAcc=50000;
    ubxMessage.navPvt.valid=0b111;
    time_init=true;
  }

  return MT_NAV_PVT;
}
