// ============================================================================
// GPS_data.cpp
//
// ROLE IN THE SYSTEM
// ------------------
// This file is the *root of the GPS data graph*.
//
// It owns the ONLY global GPS circular buffers and is the single ingestion
// point for NAV-PVT samples. Everything else (speed, alpha, track, time)
// *derives* from the data written here.
//
// Data flow:
//
//   UBX NAV-PVT
//        │
//        ▼
//   GPS_data::push_data()
//        │
//        ├─ _gSpeed[]   → gps_speed / gps_time / gps_alpha
//        ├─ _lat/_long  → gps_alpha / gps_track / geometry
//        ├─ distances   → session / run / alfa accumulation
//        └─ _secSpeed[] → long time-window averages
//
// This file deliberately contains NO analysis logic.
// It only stores, accumulates, and resets shared state.
//
// Other GPS modules include GPS_data.h and access data via `extern`.
// ============================================================================

#include "Ublox/ublox.h"
#include "Core/Definitions.h"
#include "MANAGERS/config_manager.h"
#include "Core/Globals.h"
#include "core/system_info.h"
#include <algorithm>

#include "GPS/GPS_data.h"
#include "GPS/gps_manager.h"
#include "GPS/gps_speed.h"
#include "GPS/gps_alpha.h"
#include "GPS/gps_geometry.h"
#include "GPS/gps_utils.h"

// ============================================================================
// Global GPS buffers (single source of truth)
// ============================================================================

uint16_t _gSpeed[BUFFER_SIZE];      // Doppler speed per sample (mm/s)
uint16_t _secSpeed[BUFFER_SIZE];    // 1-second averaged speed (mm/s)

float    _lat[BUFFER_ALFA];         // Latitude buffer (deg)
float    _long[BUFFER_ALFA];        // Longitude buffer (deg)

int      index_GPS = -1;            // NAV-PVT sample index
int      index_sec = -1;            // 1-second buffer index

int      alfa_counter;              // Jibe counter (shared run/alpha state)
float    total_distance = 0.0f;     // Session distance (mm)

// ============================================================================
// GPS_data
// ============================================================================

GPS_data::GPS_data(){ index_GPS = 0; }

// -----------------------------------------------------------------------------
// push_data
//
// Ingests ONE NAV-PVT sample.
// This is the only place where raw GPS observables enter the system.
// -----------------------------------------------------------------------------
void GPS_data::push_data(float latitude,float longitude,uint32_t gSpeed)
{
    index_GPS++;

    // --- raw circular buffers ---
    _gSpeed[index_GPS % BUFFER_SIZE] = gSpeed;
    _lat   [index_GPS % BUFFER_ALFA] = latitude;
    _long  [index_GPS % BUFFER_ALFA] = longitude;

    // --- distance accumulation (quality-gated) ---
    if(ubxMessage.navPvt.numSV >= FILTER_MIN_SATS &&
       (ubxMessage.navPvt.sAcc * 0.001f) < FILTER_MAX_sACC)
    {
        const uint32_t d = gSpeed / systemInfo.sample_rate; // mm per sample
        total_distance += d;
        run_distance   += d;
        alfa_distance  += d;
    }

    // --- build 1-second averaged speed buffer ---
    static uint32_t acc = 0;
    acc += gSpeed;

    if((index_GPS % systemInfo.sample_rate) == 0){
        index_sec++;
        _secSpeed[index_sec % BUFFER_SIZE] = acc / systemInfo.sample_rate;
        acc = 0;
    }
}

// ============================================================================
// GPS_SAT_info
// ============================================================================
//
// Independent NAV-SAT statistics helper.
// Tracks CNO quality for satellites used in the nav solution.
// ============================================================================

GPS_SAT_info::GPS_SAT_info(){ index_SAT_info = 0; }

void GPS_SAT_info::push_SAT_info(const NAV_SAT_HDR&,
                                 const sVs_NAV_SAT* sats,
                                 uint8_t count)
{
    mean_cno=0; min_cno=0xFF; max_cno=0; nr_sats=0;

    for(uint8_t i=0;i<count;i++){
        if(sats[i].flags & 0x08){ // used in nav solution
            mean_cno += sats[i].cno;
            min_cno   = std::min(min_cno,(uint32_t)sats[i].cno);
            max_cno   = std::max(max_cno,(uint32_t)sats[i].cno);
            nr_sats++;
        }
    }

    if(!nr_sats){ index_SAT_info++; return; }

    mean_cno /= nr_sats;
    int idx = index_SAT_info % NAV_SAT_BUFFER;

    sat_info.Mean_cno[idx] = mean_cno;
    sat_info.Max_cno [idx] = max_cno;
    sat_info.Min_cno [idx] = min_cno;
    sat_info.numSV   [idx] = nr_sats;

    // rolling averages over NAV_SAT_BUFFER
    if(index_SAT_info > NAV_SAT_BUFFER){
        mean_cno=max_cno=min_cno=nr_sats=0;
        for(int i=0;i<NAV_SAT_BUFFER;i++){
            int j=(index_SAT_info-NAV_SAT_BUFFER+i)%NAV_SAT_BUFFER;
            mean_cno += sat_info.Mean_cno[j];
            max_cno  += sat_info.Max_cno [j];
            min_cno  += sat_info.Min_cno [j];
            nr_sats  += sat_info.numSV   [j];
        }
        sat_info.Mean_mean_cno = mean_cno / NAV_SAT_BUFFER;
        sat_info.Mean_max_cno  = max_cno  / NAV_SAT_BUFFER;
        sat_info.Mean_min_cno  = min_cno  / NAV_SAT_BUFFER;
        sat_info.Mean_numSV    = nr_sats  / NAV_SAT_BUFFER;
    }

    index_SAT_info++;
}

// ============================================================================
// Session reset
//
// Central, authoritative reset of ALL GPS-derived state.
// Called on new session / clear log / hard reset.
// ============================================================================
void reset_session_stats()
{
    total_distance=0; Ublox.run_distance=0; Ublox.alfa_distance=0;
    run_count=0; old_run_count=0; alfa_counter=0;

    S2.Reset_stats();  s2.Reset_stats();
    S10.Reset_stats(); s10.Reset_stats();
    S1800.Reset_stats(); S3600.Reset_stats();
    A250.Reset_stats(); A500.Reset_stats(); a500.Reset_stats();

    M100.m_distance=M250.m_distance=M500.m_distance=M1852.m_distance=0;
    M100.m_index=M250.m_index=M500.m_index=M1852.m_index=0;

    nav_pvt_message=0; old_message=0;
}
