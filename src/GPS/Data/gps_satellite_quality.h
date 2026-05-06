#pragma once

#include <stdint.h>

// ============================================================================
// gps_satellite_quality.h
//
// Rolling NAV-SAT signal quality metadata.
//
// This does not decide whether a GPS point is valid. gps_sample_quality handles
// point acceptance. This class summarizes the last few NAV-SAT frames so top
// speed results can be stored with C/N0 and satellite-count context.
// ============================================================================

constexpr int NAV_SAT_BUFFER = 10;

struct NAV_SAT_HDR;
struct sVs_NAV_SAT;

class GPS_SAT_info {
public:
  GPS_SAT_info();

  struct SAT_info {
    uint8_t Mean_cno[NAV_SAT_BUFFER];   // Mean C/N0 per accepted NAV-SAT frame.
    uint8_t Max_cno[NAV_SAT_BUFFER];    // Max C/N0 per accepted NAV-SAT frame.
    uint8_t Min_cno[NAV_SAT_BUFFER];    // Min C/N0 per accepted NAV-SAT frame.
    uint8_t numSV[NAV_SAT_BUFFER];      // Satellites used in navigation.

    uint8_t Mean_mean_cno;              // Rolling mean of Mean_cno.
    uint8_t Mean_max_cno;               // Rolling mean of Max_cno.
    uint8_t Mean_min_cno;               // Rolling mean of Min_cno.
    uint8_t Mean_numSV;                 // Rolling mean satellite count.
  } sat_info;

  int      index_SAT_info;              // Monotonic NAV-SAT frame counter.
  uint32_t mean_cno;                    // Scratch total while parsing a frame.
  uint32_t max_cno;                     // Scratch max while parsing a frame.
  uint32_t min_cno;                     // Scratch min while parsing a frame.
  uint32_t nr_sats;                     // Scratch accepted satellite count.

  // Ingest one UBX NAV-SAT frame. Only satellites with the "used in nav" flag
  // contribute to the stored quality metadata.
  void push_SAT_info(const NAV_SAT_HDR& hdr,
                     const sVs_NAV_SAT* sats,
                     uint8_t count);
};
