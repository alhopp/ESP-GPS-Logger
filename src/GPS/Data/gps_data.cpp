// ============================================================================
// gps_data.cpp
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
//        ├─ _gSpeed[]   → gps_distance_speed / gps_time_speed / gps_alpha_speed
//        ├─ _sogCms[]   → SBP-parity per-sample speed (cm/s)
//        ├─ _lat/_long  → gps_alpha_speed / gps_track / geometry
//        ├─ _distCm[]   → cumulative distance snapshot (cm) for exports
//        ├─ distances   → session / run / alfa accumulation
//        └─ _secSpeed[] → 1-second averaged speed (mm/s)
//
// This file deliberately contains NO analysis logic.
// It only stores, accumulates, and resets shared state.
// ============================================================================

#include "GPS/Ublox/ublox_driver.h"
#include "Core/Definitions.h"
#include "Core/Globals.h"
#include "Core/system_info.h"

#include "GPS/Data/gps_data.h"

#include <math.h>

// ============================================================================
// Global GPS buffers (single source of truth)
// ============================================================================

uint16_t _gSpeed [BUFFER_SIZE];   // Doppler speed per sample (mm/s)
uint16_t _sogCms [BUFFER_SIZE];   // SBP-parity speed per sample (cm/s)
uint16_t _secSpeed[BUFFER_SIZE];  // 1-second averaged speed (mm/s)
bool     _sampleGood[BUFFER_SIZE];

uint32_t _distCm [BUFFER_SIZE];   // cumulative sailed distance (cm) per sample

float    _lat[BUFFER_ALFA];       // Latitude buffer (deg)
float    _long[BUFFER_ALFA];      // Longitude buffer (deg)

int      index_GPS = -1;          // NAV-PVT sample index
int      index_sec = -1;          // 1-second buffer index

int      alfa_counter;            // Jibe counter (shared run/alpha state)
float    total_distance = 0.0f;   // Session distance (mm)


volatile int alpha_gybe_index = -1;
volatile int alpha_holdoff_ticks = 0;
volatile int  alpha_gybe_start    = -1;   // inclusive
volatile int  alpha_gybe_end      = -1;   // inclusive
volatile bool alpha_window_valid  = false;

// -----------------------------------------------------------------------------
// Second → GPS index mapping
// -----------------------------------------------------------------------------
int sec_to_gps_index[BUFFER_SIZE] = {0};




// ============================================================================
// GPS_data
// ============================================================================

GPS_data::GPS_data(){ index_GPS = 0; }

namespace {
constexpr float MIN_VALID_COORD = 0.000001f;
constexpr float BAD_JUMP_MIN_M = 50.0f;
constexpr float BAD_JUMP_MARGIN_M = 20.0f;
constexpr float BAD_JUMP_SPEED_MULT = 3.0f;

bool have_last_good_position = false;
float last_good_lat = 0.0f;
float last_good_lon = 0.0f;

float distanceMeters(float lat0, float lon0, float lat1, float lon1)
{
    const float dlat = lat1 - lat0;
    const float dlon = (lon1 - lon0) * cosf((lat0 + lat1) * 0.5f * DEG2RAD);
    return sqrtf(dlat * dlat + dlon * dlon) * 111195.0f;
}

bool sampleQualityOk(float latitude, float longitude, uint32_t gSpeed)
{
    if (ubxMessage.navPvt.fixType < 3) return false;
    if (ubxMessage.navPvt.numSV < FILTER_MIN_SATS) return false;
    if ((ubxMessage.navPvt.sAcc * 0.001f) >= FILTER_MAX_sACC) return false;
    if (gSpeed > (uint32_t)MAX_GPS_SPEED_OK * 1000U) return false;
    if (fabsf(latitude) < MIN_VALID_COORD && fabsf(longitude) < MIN_VALID_COORD) return false;

    if (have_last_good_position) {
        const float sr = systemInfo.sample_rate > 0 ? (float)systemInfo.sample_rate : 5.0f;
        const float expected_m = (float)gSpeed * 0.001f / sr;
        const float max_jump_m = fmaxf(BAD_JUMP_MIN_M, expected_m * BAD_JUMP_SPEED_MULT + BAD_JUMP_MARGIN_M);
        if (distanceMeters(last_good_lat, last_good_lon, latitude, longitude) > max_jump_m) {
            return false;
        }
    }

    return true;
}
}

void gps_data_reset_quality_state()
{
    have_last_good_position = false;
    last_good_lat = 0.0f;
    last_good_lon = 0.0f;

    for (int i = 0; i < BUFFER_SIZE; i++) {
        _sampleGood[i] = false;
    }
}

// -----------------------------------------------------------------------------
// push_data
//
// Ingests ONE NAV-PVT sample.
// This is the only place where raw GPS observables enter the system.
// -----------------------------------------------------------------------------
void GPS_data::push_data(float latitude,float longitude,uint32_t gSpeed)
{
    index_GPS++;

    const int i = index_GPS % BUFFER_SIZE;
    const bool good_sample = sampleQualityOk(latitude, longitude, gSpeed);

    if (!good_sample) {
        gSpeed = 0;
        if (have_last_good_position) {
            latitude = last_good_lat;
            longitude = last_good_lon;
        }
    } else {
        have_last_good_position = true;
        last_good_lat = latitude;
        last_good_lon = longitude;
    }

    // -------------------------------------------------------------------------
    // Raw circular buffers
    // -------------------------------------------------------------------------
    _gSpeed [i] = gSpeed;
    _sogCms [i] = (uint16_t)(gSpeed * 0.1f); // mm/s → cm/s (SBP parity)
    _sampleGood[i] = good_sample;
    _lat    [index_GPS % BUFFER_ALFA] = latitude;
    _long   [index_GPS % BUFFER_ALFA] = longitude;

    // -------------------------------------------------------------------------
    // Distance accumulation (quality-gated, RP6/Speedreader unit model, mm)
    // -------------------------------------------------------------------------
    if(good_sample)
    {
        const float d_mm = (float)gSpeed / systemInfo.sample_rate; // mm per sample

        total_distance += d_mm;   // session
        run_distance   += d_mm;   // run
        alfa_distance  += d_mm;   // alpha
    }

    // -------------------------------------------------------------------------
    // Cumulative distance snapshot (ALWAYS written)
    // Used by Alpha for (exit - entry) distance diffing
    // -------------------------------------------------------------------------
    _distCm[i] = (uint32_t)((total_distance * 0.1f) + 0.5f);

    // -------------------------------------------------------------------------
    // Build 1-second averaged speed buffer
    // -------------------------------------------------------------------------
    static uint32_t acc_mmps = 0;
    acc_mmps += _gSpeed[i];

    if((index_GPS % systemInfo.sample_rate) == 0){
        index_sec++;
        _secSpeed[index_sec % BUFFER_SIZE] = acc_mmps / systemInfo.sample_rate;

        // map this 1Hz bucket → the GPS sample index it corresponds to
        sec_to_gps_index[index_sec % BUFFER_SIZE] = index_GPS;

        acc_mmps = 0;
    }

}

