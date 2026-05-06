#pragma once
#include <stdint.h>

// Shared GPS buffers and counters.
// gps_data.cpp owns the storage; metric modules read these legacy-compatible
// arrays to compute distance, time-window, run, and alpha results.

// Define constants
// -----------------------------------------------------------------------------
// GPS constants
// -----------------------------------------------------------------------------
constexpr double DEG2RAD         = 0.017453292519943295; // PI / 180

constexpr int    BUFFER_SIZE     = 1000;   // GPS ground-speed circular buffer
constexpr int    BUFFER_ALFA     = 2000;   // Alfa-speed position buffer
constexpr int    FILTER_MIN_SATS = 5;      // Minimum satellites for valid distance accumulation
constexpr int    FILTER_MAX_sACC = 2;      // Maximum speed accuracy (m/s)
constexpr int    NR_OF_BAR       = 42;     // Bar-graph resolution

// -----------------------------------------------------------------------------
// Shared GPS raw buffers (owned by gps_data.cpp)
// -----------------------------------------------------------------------------
extern uint16_t _gSpeed[BUFFER_SIZE];
extern uint16_t _secSpeed[BUFFER_SIZE];  // 1-second averaged speed (mm/s)
extern uint16_t _sogCms[BUFFER_SIZE];   // SBP-parity SOG per sample (cm/s, integer trunc)
extern bool     _sampleGood[BUFFER_SIZE]; // true when the ingested sample passed quality filters


extern int index_GPS;
extern int index_sec;




// -----------------------------------------------------------------------------
// Second → GPS index mapping (for 1h / decimated geometry)
// -----------------------------------------------------------------------------
// sec index ∈ [0..BUFFER_SIZE)
// value = corresponding index_GPS at that second
extern int sec_to_gps_index[BUFFER_SIZE];


// -----------------------------------------------------------------------------
// GPS_data
//
// Responsibilities:
// - Collect raw GPS observables (latitude, longitude, ground speed)
// - Maintain circular buffers for downstream processing
// - Accumulate total, run, and alfa distances
//
// Notes:
// - Ground speed is expected in mm/s (Doppler-based)
// - Buffer indexing and sample timing are handled internally
// - Higher-level calculations are performed by GPS_speed, GPS_time, etc.
// -----------------------------------------------------------------------------

extern float total_distance;   // session distance (mm)

class GPS_data {
  public:
    GPS_data();  // Constructor

    float run_distance;     // Distance within the current run (mm)
    float alfa_distance;    // Distance used for alfa-speed calculations (mm)
    float delta_dist;       // Last incremental distance update (mm)

    // Push one GPS sample into the internal buffers
    void push_data(float latitude,
                   float longitude,
                   uint32_t gSpeed);  // Ground speed in mm/s

  private:
};

// --- GPS core buffers ---
extern float    _lat[];
extern float    _long[];
extern int      index_GPS;
extern int      alfa_counter;

void gps_data_reset_quality_state();



