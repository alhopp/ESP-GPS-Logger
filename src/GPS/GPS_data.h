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

extern int index_GPS;
extern int index_sec;

extern float alfa_exit;

void sort_display(double a[], int size);

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

extern float total_distance;   // Total distance since power-on (mm)

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

// -----------------------------------------------------------------------------
// Run sorting and detection helpers
//
// Responsibilities:
// - Sort run result arrays by speed while keeping all associated metadata aligned
// - Detect the start of a new run based on heading changes and speed thresholds
//
// Notes:
// - Sorting is performed in-place on fixed-size arrays
// - All parallel arrays (time, distance, CNO, run index, etc.) are kept in sync
// - New_run_detection() implements heuristic-based run detection using:
//     - Heading change
//     - Speed thresholds
//     - Temporal stability
// -----------------------------------------------------------------------------

// Sort run results by speed (descending), keeping timing and satellite data aligned
void sort_run(double a[],
              uint8_t hour[],
              uint8_t minute[],
              uint8_t seconde[],
              uint8_t mean_cno[],
              uint8_t max_cno[],
              uint8_t min_cno[],
              uint8_t nrSats[],
              int runs[],
              int size);

// Sort alfa run results by speed, keeping distance and sample metadata aligned
void sort_run_alfa(double a[],
                   int dis[],
                   int message[],
                   uint8_t hour[],
                   uint8_t minute[],
                   uint8_t seconde[],
                   int runs[],
                   int samples[],
                   int size);

// Detect the start of a new run based on heading evolution and short-term speed
int New_run_detection(float actual_heading,
                      float S2_speed);



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
 * GPS_Track
 *
 * Calculates average speed over a fixed track defined by two virtual lines:
 * a start line and an end line (e.g. a 500 m course).
 *
 * - The track is defined by two pairs of GPS coordinates:
 *     * Line 1: start line (lon1/lat1 → lon2/lat2)
 *     * Line 2: end line   (lon3/lat3 → lon4/lat4)
 * - When the start line is crossed, a run begins.
 * - When the end line is crossed, the run ends and speed is calculated.
 *
 * Speed calculation:
 * - Distance is either the theoretical track distance or the measured GPS
 *   distance between crossing points.
 * - Time is derived from UBX iTOW timestamps.
 *
 * Notes:
 * - Uses global GPS state (NAV-PVT data and circular buffers).
 * - Does NOT manage run detection globally (only start/end line crossings).
 * - Intended for fixed-distance course measurements (e.g. 500 m speed run).
 * --------------------------------------------------------------------------- */
class GPS_Track {
public:
    GPS_Track(void);

    // Define the track geometry using two lines and a theoretical distance (meters)
    void Set_course(double lon_1, double lat_1,
                    double lon_2, double lat_2,
                    double lon_3, double lat_3,
                    double lon_4, double lat_4,
                    int distance);

    // Update track state; returns distance to end line (sign indicates side)
    float Update_Track(void);

    // Track geometry
    double lon1, lat1, lon2, lat2;
    double lon3, lat3, lon4, lat4;

    // Start / end positions
    double Start_lon, Start_lat;
    double End_lon, End_lat;

    float  set_distance;              // Configured track distance (meters)
    int    Start_iTOW_ms;              // UBX time at start line crossing
    int    End_iTOW_ms;                // UBX time at end line crossing
    int    Track_time_ms;              // Duration of the run (ms)
    float  Track_speed;                // Calculated average speed
    float  distance_startline;         // Signed distance to start line
    float  distance_endline;           // Signed distance to end line
    float  track_distance;             // Measured GPS distance
    int    theoretical_track_distance; // Nominal track length
    float  distance_p1p3;
    float  distance_p2p4;

    // Result buffers (top runs)
    double  avg_speed[10];
    double  display_speed[10];
    int     m_Distance[10];
    uint8_t time_hour[10];
    uint8_t time_min[10];
    uint8_t time_sec[10];

    // Padding / legacy placeholders (can be removed later)
    uint8_t dummy[10];
    int     dummy_int[10];

private:
    float Old_distance_start;  // Previous signed distance to start line
    float Old_distance_end;    // Previous signed distance to end line
    bool  Run_started;         // Run-in-progress flag
};

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

// -----------------------------------------------------------------------------
// Geometry helper: signed distance from a point to a line
//
// Calculates the perpendicular distance (in meters) from point P to the line AB.
// The sign of the returned value indicates which side of the line the point lies on.
//
// Parameters:
// - lambda0, phi0 : longitude / latitude of point P
// - lambda1, phi1 : longitude / latitude of line point A
// - lambda2, phi2 : longitude / latitude of line point B
//
// Returns:
// - Signed distance in meters
//   > 0  : point is on one side of the line
//   < 0  : point is on the opposite side
// -----------------------------------------------------------------------------
// float Dis_point_line(float long_act,float lat_act,float long_1,float lat_1,float long_2,float lat_2);
double Dis_point_line(double lambda0, double phi0,
                      double lambda1, double phi1,
                      double lambda2, double phi2);

// -----------------------------------------------------------------------------
// Distance between two GPS points (local planar approximation)
//
// Computes the straight-line distance between two latitude/longitude points
// using a locally flat Earth approximation (sufficient for short distances).
//
// Parameters:
// - lambda1, phi1 : longitude / latitude of point 1
// - lambda2, phi2 : longitude / latitude of point 2
//
// Returns:
// - Distance in meters
// -----------------------------------------------------------------------------
double afstandPunten(double lambda1, double phi1,
                     double lambda2, double phi2);

// -----------------------------------------------------------------------------
// Alpha indicator helper
//
// Computes the perpendicular distance of the current position relative to
// the alpha reference line defined by two GPS_speed windows (typically 250 m
// and 100 m before the jibe).
//
// Used to determine whether the current position still lies within the
// allowed alpha corridor (e.g. ≤ 50 m).
//
// Parameters:
// - M250 : GPS_speed instance for 250 m window
// - M100 : GPS_speed instance for 100 m window
// - actual_heading : current course heading (degrees)
//
// Returns:
// - Signed perpendicular distance in meters
// -----------------------------------------------------------------------------
float Alfa_indicator(GPS_speed M250,
                     GPS_speed M100,
                     float actual_heading);





extern GPS_time S2;
extern GPS_time s2;
extern GPS_time S10;
extern GPS_time s10;
extern GPS_time S1800;
extern GPS_time S3600;
extern GPS_Track M_500;

// --- GPS core buffers ---
extern float    _lat[];
extern float    _long[];
extern int      index_GPS;
extern int      alfa_counter;



