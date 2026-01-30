#pragma once
#include <stdint.h>
#include <math.h>

#include "GPS/gps_time.h"
#include "GPS/gps_speed.h"


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
constexpr int    NAV_SAT_BUFFER  = 10;     // Rolling NAV-SAT statistics window

// -----------------------------------------------------------------------------
// Shared GPS raw buffers (owned by GPS_data.cpp)
// -----------------------------------------------------------------------------
extern uint16_t _gSpeed[BUFFER_SIZE];
extern uint16_t _secSpeed[BUFFER_SIZE];
extern uint16_t _sogCms[BUFFER_SIZE];   // SBP-parity SOG per sample (cm/s, integer trunc)


extern int index_GPS;
extern int index_sec;





void reset_session_stats(); 

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

extern float total_distance;   // session disatnce cm

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

// -----------------------------------------------------------------------------
// GPS_SAT_info
//
// Responsibilities:
// - Process u-blox NAV-SAT messages
// - Extract signal quality metrics for satellites used in navigation
// - Maintain rolling statistics over multiple NAV-SAT frames
//
// Stored metrics (navigation satellites only):
// - Mean C/N0
// - Max C/N0
// - Min C/N0
// - Number of satellites used in the solution
//
// Notes:
// - Only satellites flagged as "used in navigation" are considered
// - Rolling averages are computed over NAV_SAT_BUFFER frames
// - Intended for diagnostics, logging, and quality indicators (not filtering)
// -----------------------------------------------------------------------------
// Forward declarations
struct NAV_SAT_HDR;
struct sVs_NAV_SAT;

class GPS_SAT_info {
  public:
    GPS_SAT_info();

    struct SAT_info {
      uint8_t Mean_cno[NAV_SAT_BUFFER];   // Mean C/N0 per NAV-SAT frame
      uint8_t Max_cno[NAV_SAT_BUFFER];    // Max C/N0 per NAV-SAT frame
      uint8_t Min_cno[NAV_SAT_BUFFER];    // Min C/N0 per NAV-SAT frame
      uint8_t numSV[NAV_SAT_BUFFER];      // Satellites used in navigation

      uint8_t Mean_mean_cno;              // Rolling mean of Mean_cno
      uint8_t Mean_max_cno;               // Rolling mean of Max_cno
      uint8_t Mean_min_cno;               // Rolling mean of Min_cno
      uint8_t Mean_numSV;                 // Rolling mean satellite count
    } sat_info;

    int      index_SAT_info;              // NAV-SAT frame counter
    uint32_t mean_cno;
    uint32_t max_cno;
    uint32_t min_cno;
    uint32_t nr_sats;

    // Process one NAV-SAT message and update rolling statistics
    void push_SAT_info(const NAV_SAT_HDR& hdr,
                       const sVs_NAV_SAT* sats,
                       uint8_t count);
};



/* -----------------------------------------------------------------------------
 * GPS_speed
 *
 * Calculates average speed over a fixed distance window.
 *
 * - The distance window is defined at construction time (e.g. 100 m, 250 m, 500 m).
 * - Speed is derived from Doppler ground speed samples stored in the global
 *   GPS circular buffer.
 * - The calculation itself is distance-based; the sample rate only affects
 *   internal resolution, not the physical distance window.
 *
 * The class tracks:
 * - Current average speed over the distance window
 * - Maximum speed achieved in the current run
 * - Per-run statistics (time, distance, samples, message index)
 *
 * This class does NOT:
 * - Own the raw GPS buffers
 * - Perform filtering or smoothing
 * - Control run detection
 * --------------------------------------------------------------------------- */


/* -----------------------------------------------------------------------------
 * GPS_time
 *
 * Calculates average GPS speed over a fixed time window
 * (e.g. 2 s, 10 s, 30 min / 1800 s).
 *
 * Two operating modes are used internally:
 * - High-rate mode:
 *     Uses the raw per-sample Doppler speed buffer when
 *     (time_window * sample_rate) fits inside BUFFER_SIZE.
 *
 * - Long-window mode:
 *     Switches to a 1 Hz averaged speed buffer (_secSpeed[])
 *     for large time windows to avoid excessive buffer traversal.
 *
 * Features:
 * - Tracks maximum speed per run.
 * - Maintains sorted top-10 speeds for display and logging.
 * - Computes rolling averages (e.g. best 5 runs).
 * - Captures timestamp and satellite quality (CNO, SV count)
 *   at the moment a new maximum is detected.
 *
 * Notes:
 * - Uses global GPS circular buffers (_gSpeed, _secSpeed).
 * - Depends on external run detection (actual_run).
 * - Intended for time-based performance metrics, not distance-based ones
 *   (see GPS_speed for distance windows).
 * --------------------------------------------------------------------------- */






extern GPS_time S2;
extern GPS_time s2;
extern GPS_time S10;
extern GPS_time s10;
extern GPS_time S1800;
extern GPS_time S3600;


// --- GPS core buffers ---
extern float    _lat[];
extern float    _long[];
extern int      index_GPS;
extern int      alfa_counter;



