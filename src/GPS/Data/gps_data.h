#pragma once

#include <stdint.h>

// ============================================================================
// gps_data.h
//
// Public view of the GPS sample buffers owned by gps_data.cpp.
//
// These globals preserve the RP6/Speedreader-style metric model: each metric
// module reads from fixed-size rings rather than receiving copied sample lists.
// Keep ownership centralized in gps_data.cpp; other modules should read these
// buffers but not create competing storage.
// ============================================================================

// ----------------------------------------------------------------------------
// Buffer and filtering constants
// ----------------------------------------------------------------------------

constexpr double DEG2RAD = 0.017453292519943295; // PI / 180.

constexpr int BUFFER_SIZE = 1000;     // Speed/quality ring depth.
constexpr int BUFFER_ALFA = 2000;     // Position ring depth for alpha geometry.
constexpr int FILTER_MIN_SATS = 5;    // Minimum satellites for trusted samples.
constexpr int FILTER_MAX_sACC = 2;    // Maximum speed accuracy, m/s.

// ----------------------------------------------------------------------------
// High-rate GPS sample rings
// ----------------------------------------------------------------------------

extern uint16_t _gSpeed[BUFFER_SIZE];      // SBP-quantized Doppler speed, mm/s.
extern uint16_t _sogCms[BUFFER_SIZE];      // Same speed in cm/s for SBP output.
extern int      _sbpIndex[BUFFER_SIZE];    // 1-based SBP frame index for sample.
extern bool     _sampleGood[BUFFER_SIZE];  // False means sanitized/held sample.

extern float _lat[BUFFER_ALFA];            // Latitude, decimal degrees.
extern float _long[BUFFER_ALFA];           // Longitude, decimal degrees.

// Monotonic GPS sample index. Ring indexes are derived with modulo operations.
extern int index_GPS;

// ----------------------------------------------------------------------------
// 1 Hz speed ring for long time-window stats
// ----------------------------------------------------------------------------

extern uint16_t _secSpeed[BUFFER_SIZE];    // 1-second averaged speed, mm/s.
extern int index_sec;                      // Monotonic 1 Hz bucket index.

// Maps _secSpeed[] buckets back to the GPS sample that closed that second.
// This lets 30-minute/1-hour results be exported with GPS coordinates.
extern int sec_to_gps_index[BUFFER_SIZE];

// ----------------------------------------------------------------------------
// Session distance and alpha/run state
// ----------------------------------------------------------------------------

extern float total_distance;               // Full session distance, mm.
extern int alfa_counter;                   // Incremented by jibe/run detection.

// ----------------------------------------------------------------------------
// GPS_data
// ----------------------------------------------------------------------------

class GPS_data {
public:
  GPS_data();

  float run_distance;   // Distance since current run started, mm.
  float alfa_distance;  // Distance since alpha/jibe reference reset, mm.

  // Ingest one NAV-PVT sample into the shared rings.
  // latitude/longitude are decimal degrees; gSpeed is Doppler speed in mm/s.
  void push_data(float latitude, float longitude, uint32_t gSpeed);
};

// Reset stateful sample-quality filtering and clear the quality ring.
void gps_data_reset_quality_state();
