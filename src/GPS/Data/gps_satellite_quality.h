#pragma once

#include <stdint.h>

// Rolling NAV-SAT signal quality statistics.

constexpr int NAV_SAT_BUFFER = 10;

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

  void push_SAT_info(const NAV_SAT_HDR& hdr,
                     const sVs_NAV_SAT* sats,
                     uint8_t count);
};
