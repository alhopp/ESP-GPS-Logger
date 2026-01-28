
#include "Ublox/ublox.h"
#include "Core/Definitions.h"
#include "MANAGERS/config_manager.h"
#include "Core/Globals.h"
#include "core/system_info.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_manager.h"
#include "GPS/gps_speed.h"
#include "GPS/gps_alpha.h"
#include "GPS/gps_geometry.h"

uint16_t _gSpeed[BUFFER_SIZE]; 
float _lat[BUFFER_ALFA]; 
float _long[BUFFER_ALFA];
float alfa_exit;//test voor functie afstand punt tot lijn !!
uint16_t _secSpeed[BUFFER_SIZE];
int index_GPS=-1;//bij eerste doorgang op 0 beginnen !!
int index_sec=-1;//bij eerste doorgang op 0 beginnen !!
int alfa_counter;

float total_distance = 0.0f;
// -----------------------------------------------------------------------------
// GPS_data::push_data
//
// Continuously stores core GPS observables into circular buffers:
//
//   - Doppler ground speed (mm/s)
//   - Latitude (degrees)
//   - Longitude (degrees)
//
// A global buffer is intentionally used so this data is accessible by:
//   - GPS_speed   (distance-based speed windows)
//   - GPS_time    (time-based speed windows)
//
// The most recent buffer index is tracked via the global variable `index_GPS`.
//
// Notes (M10-specific):
// - Motion model is fixed to SEA at startup (no runtime switching)
// - Doppler speed is trusted directly (no smoothing here)
// - All higher-level filtering happens downstream
// -----------------------------------------------------------------------------
void GPS_data::push_data(float latitude,
                         float longitude,
                         uint32_t gSpeed)   // Doppler speed in mm/s
{
    // ------------------------------------------------------------
    // Advance circular buffer index
    // ------------------------------------------------------------
    index_GPS++;

    // ------------------------------------------------------------
    // Store raw GPS observables
    // ------------------------------------------------------------
    _gSpeed[index_GPS % BUFFER_SIZE] = gSpeed;     // mm/s (Doppler)
    _lat   [index_GPS % BUFFER_ALFA] = latitude;   // degrees
    _long  [index_GPS % BUFFER_ALFA] = longitude;  // degrees

    // ------------------------------------------------------------
    // Distance accumulation (only with a valid navigation solution)
    //
    // sAcc is in mm → convert to meters before comparison
    // ------------------------------------------------------------
    if ((ubxMessage.navPvt.numSV >= FILTER_MIN_SATS) &&
        ((ubxMessage.navPvt.sAcc * 0.001f) < FILTER_MAX_sACC))
    {
        const uint32_t delta_dist = gSpeed / systemInfo.sample_rate; // mm per sample

        total_distance += delta_dist;
        run_distance   += delta_dist;
        alfa_distance  += delta_dist;
    }

    // ------------------------------------------------------------
    // Build 1-second averaged speed buffer
    //
    // This is used by long time-window averages (30s / 60s / etc.)
    // to avoid excessive buffer traversal at high sample rates.
    // ------------------------------------------------------------
    static uint32_t avg_gSpeed = 0;  // mm/s accumulator

    avg_gSpeed += gSpeed;

    if ((index_GPS % systemInfo.sample_rate) == 0) {
        index_sec++;
        _secSpeed[index_sec % BUFFER_SIZE] =
            avg_gSpeed / systemInfo.sample_rate;

        avg_gSpeed = 0;
    }
}

//constructor for GPS_data
GPS_data::GPS_data() {
  index_GPS=0; 
}

//constructor for SAT_info
GPS_SAT_info::GPS_SAT_info() {
  index_SAT_info=0; 
}

//function to extract info out of NAV_SAT, and push it to array
//For every NAV_SAT frame, the Mean CNO, the Max cno, the Min cno and the nr of sats in the nav solution are stored
//Then, the means are calculated out of the last NAV_SAT_BUFFER frames (now 16 frames, @5Hz, this 0.5Hz NAV_SAT ca 32 s)
void GPS_SAT_info::push_SAT_info(const NAV_SAT_HDR& hdr,
                                 const sVs_NAV_SAT* sats,
                                 uint8_t count)

{
  //#define NAV_SAT_BUFFER 10
  mean_cno = 0;
  min_cno  = 0xFF;
  max_cno  = 0;
  nr_sats  = 0;

  // only evaluate number of Sats in NAV_SAT
  for (uint8_t i = 0; i < count; i++) {
    // only evaluate sats used in nav solution (bit3)
    if (sats[i].flags & 0x08) {
      mean_cno += sats[i].cno;
      if (sats[i].cno < min_cno) min_cno = sats[i].cno;
      if (sats[i].cno > max_cno) max_cno = sats[i].cno;
      nr_sats++;
    }
  }

  if (nr_sats) { // protect divide by zero
    mean_cno = mean_cno / nr_sats;

    sat_info.Mean_cno[index_SAT_info % NAV_SAT_BUFFER] = mean_cno;
    sat_info.Max_cno[index_SAT_info % NAV_SAT_BUFFER]  = max_cno;
    sat_info.Min_cno[index_SAT_info % NAV_SAT_BUFFER]  = min_cno;
    sat_info.numSV[index_SAT_info % NAV_SAT_BUFFER]    = nr_sats;

    mean_cno = 0;
    min_cno  = 0;
    max_cno  = 0;
    nr_sats  = 0;

    if (index_SAT_info > NAV_SAT_BUFFER) {
      for (int i = 0; i < NAV_SAT_BUFFER; i++) {
        mean_cno += sat_info.Mean_cno[(index_SAT_info - NAV_SAT_BUFFER + i) % NAV_SAT_BUFFER];
        max_cno  += sat_info.Max_cno[(index_SAT_info - NAV_SAT_BUFFER + i) % NAV_SAT_BUFFER];
        min_cno  += sat_info.Min_cno[(index_SAT_info - NAV_SAT_BUFFER + i) % NAV_SAT_BUFFER];
        nr_sats  += sat_info.numSV[(index_SAT_info - NAV_SAT_BUFFER + i) % NAV_SAT_BUFFER];
      }

      mean_cno /= NAV_SAT_BUFFER;
      max_cno  /= NAV_SAT_BUFFER;
      min_cno  /= NAV_SAT_BUFFER;
      nr_sats  /= NAV_SAT_BUFFER;

      sat_info.Mean_mean_cno = mean_cno;
      sat_info.Mean_max_cno  = max_cno;
      sat_info.Mean_min_cno  = min_cno;
      sat_info.Mean_numSV    = nr_sats;
    }
  }

  index_SAT_info++;
}

void sort_display(double a[],int size){
  for(int i=0; i<(size-1); i++) {
        for(int o=0; o<(size-(i+1)); o++) {
                if(a[o] > a[o+1]) {
                    double t = a[o];
                    a[o] = a[o+1];
                    a[o+1] = t;
                    }
        }
  }     
}


void sort_run(double a[], uint8_t hour[], uint8_t minute[],uint8_t seconde[],uint8_t mean_cno[],uint8_t max_cno[],uint8_t min_cno[],uint8_t nrSats[],int runs[], int size) {
    for(int i=0; i<(size-1); i++) {
        for(int o=0; o<(size-(i+1)); o++) {
                if(a[o] > a[o+1]) {
                    double t = a[o];int b=hour[o];int c=minute[o];int d=seconde[o];int e=runs[o];int f=mean_cno[o];int g=max_cno[o];int h=min_cno[o];int j=nrSats[o];
                    a[o] = a[o+1];hour[o] = hour[o+1];minute[o] = minute[o+1];seconde[o]=seconde[o+1];runs[o]=runs[o+1];mean_cno[o]=mean_cno[o+1];max_cno[o]=max_cno[o+1];min_cno[o]=min_cno[o+1];nrSats[o]=nrSats[o+1];
                    a[o+1] = t; hour[o+1] = b; minute[o+1] = c;seconde[o+1]=d;runs[o+1]=e;mean_cno[o+1]=f;max_cno[o+1]=g;min_cno[o+1]=h;nrSats[o+1]=j;
                }
        }
    }
}


void sort_run_results(double a[], int dis[],int message[],uint8_t hour[], uint8_t minute[],uint8_t seconde[],int runs[], int samples[],int size) {
    for(int i=0; i<(size-1); i++) {
        for(int o=0; o<(size-(i+1)); o++) {
                if(a[o] > a[o+1]) {
                    double t = a[o];int v=dis[o];int x=message[o];int b=hour[o];int c=minute[o];int d=seconde[o];int e=runs[o];int f=samples[o];
                    a[o] = a[o+1];dis[o] = dis[o+1];message[o]=message[o+1];hour[o] = hour[o+1];minute[o] = minute[o+1];seconde[o]=seconde[o+1];runs[o]=runs[o+1];samples[o]=samples[o+1];
                    a[o+1] = t; dis[o+1] = v;message[o+1]=x;hour[o+1] = b; minute[o+1] = c;seconde[o+1]=d;runs[o+1]=e;samples[o+1]=f;
                }
        }
    }
}
GPS_Track:: GPS_Track(void){
  
}


void GPS_Track::Set_course(double lon_1,double lat_1,double lon_2,double lat_2,double lon_3,double lat_3,double lon_4,double lat_4,int distance){
  double midpoint_lat=(lat_1+lat_3)/2;
  double midpoint_lon=(lon_1+lon_3)/2;
  float distance_midpoint=Dis_point_line(midpoint_lon,midpoint_lat,lon_1,lat_1,lon_2,lat_2);
  if(distance_midpoint>0){
    lon1=lon_1;
    lat1=lat_1;
    lon2=lon_2;
    lat2=lat_2;
    }
  else{
    lon2=lon_1;
    lat2=lat_1;
    lon1=lon_2;
    lat1=lat_2; 
    }
  distance_midpoint=Dis_point_line(midpoint_lon,midpoint_lat,lon_3,lat_3,lon_4,lat_4);  
  if(distance_midpoint<0){
    lon3=lon_3;
    lat3=lat_3;
    lon4=lon_4;
    lat4=lat_4;
    }
  else{
    lon4=lon_3;
    lat4=lat_3;
    lon3=lon_4;
    lat3=lat_4; 
    }
  theoretical_track_distance=distance;
  distance_p1p3=afstandPunten(lon1,lat1,lon3,lat3);
  distance_p2p4=afstandPunten(lon2,lat2,lon4,lat4);
}


float GPS_Track::Update_Track(void){
  distance_startline= Dis_point_line(ubxMessage.navPvt.lon/10000000.0f,ubxMessage.navPvt.lat/10000000.0f,lon1,lat1,lon2,lat2);
  if((distance_startline>0)&&(Old_distance_start<0)){//lijn gepasseerd in van + naar -
    getLocalTime(&tmstruct, 0);
    Start_lon=ubxMessage.navPvt.lon/10000000.0f;
    Start_lat=ubxMessage.navPvt.lat/10000000.0f;
    Start_iTOW_ms= ubxMessage.navPvt.iTOW;
    Run_started=true;
    }
    Old_distance_start=distance_startline;
  distance_endline= Dis_point_line(ubxMessage.navPvt.lon/10000000.0f,ubxMessage.navPvt.lat/10000000.0f,lon3,lat3,lon4,lat4);
  if((distance_endline>0)&&(Old_distance_end<0)&Run_started){//lijn gepasseerd in van + naar -
    getLocalTime(&tmstruct, 0);
    End_lon=ubxMessage.navPvt.lon/10000000.0f;// _lon[(index_GPS-1)%BUFFER_ALFA] = vorige positie
    End_lat=ubxMessage.navPvt.lat/10000000.0f; //_lat[(index_GPS-1)%BUFFER_ALFA] = vorige positie
    track_distance=afstandPunten(Start_lon,Start_lat,End_lon,End_lat);
    End_iTOW_ms= ubxMessage.navPvt.iTOW;
    Track_time_ms=End_iTOW_ms-Start_iTOW_ms;
    float track_dis=(float)theoretical_track_distance;
    Track_speed=track_dis*1000/Track_time_ms*systemInfo.cal_speed;
    time_hour[0]=tmstruct.tm_hour;
    time_min[0]=tmstruct.tm_min;
    time_sec[0]=tmstruct.tm_sec;
    avg_speed[0]=track_distance*1000/Track_time_ms;
    Run_started=false;
    sort_run(avg_speed,time_hour,time_min,time_sec,dummy,dummy,dummy,dummy,dummy_int,10);
        }
    Old_distance_end=distance_endline;  
    return distance_endline;
}



/**
 * Bereken de afstand in meters en bepaal de zijde van punt P ten opzichte van lijn AB.
 * Dit is de versie van chatgpt, hier komt ook een teken uit aan welke zijde het punt zich bevindt !
 * @param lambda1 lengtegraad van punt A (longitude)
 * @param phi1 breedtegraad van punt A  (latitude)
 * @param lambda2 lengtegraad van punt B
 * @param phi2 breedtegraad van punt B
 * @param lambda0 lengtegraad van punt P
 * @param phi0 breedtegraad van punt P
 * @param zijde pointer naar string pointer om de zijde te retourneren ("links", "rechts" of "op de lijn")
 * @return afstand in meters
 */

 

// GPS_data.cpp
void reset_session_stats()
{
  // -------------------------------------------------------------------------
  // Core distances
  // -------------------------------------------------------------------------
  total_distance        = 0.0f;
  Ublox.run_distance    = 0.0f;
  Ublox.alfa_distance   = 0.0f;

  // -------------------------------------------------------------------------
  // Reset run / alfa counters
  // -------------------------------------------------------------------------
  run_count       = 0;
  old_run_count   = 0;
  alfa_counter    = 0;

  // -------------------------------------------------------------------------
  // Reset speed calculators
  // -------------------------------------------------------------------------
  S2.Reset_stats();
  s2.Reset_stats();
  S10.Reset_stats();
  s10.Reset_stats();
  S1800.Reset_stats();
  S3600.Reset_stats();

  A250.Reset_stats();
  A500.Reset_stats();
  a500.Reset_stats();

  // -------------------------------------------------------------------------
  // Reset distance-based windows
  // -------------------------------------------------------------------------
  M100.m_distance  = 0;
  M250.m_distance  = 0;
  M500.m_distance  = 0;
  M1852.m_distance = 0;

  M100.m_index  = 0;
  M250.m_index  = 0;
  M500.m_index  = 0;
  M1852.m_index = 0;

  // -------------------------------------------------------------------------
  // Reset NAV-PVT sequencing
  // -------------------------------------------------------------------------
  nav_pvt_message = 0;
  old_message     = 0;
}


