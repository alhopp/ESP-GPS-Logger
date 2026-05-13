#pragma once

// ============================================================================
// sbp_format.h
//
// Canonical binary layout for project SBP session logs. Writers, readers, and
// diagnostics include this file so the on-disk format has one source of truth.
// ============================================================================

#include <stddef.h>
#include <stdint.h>

#include "GPS/gps_config.h"

constexpr size_t SBP_HEADER_SIZE = 64;
constexpr size_t SBP_FRAME_SIZE = 32;

struct SbpHeader {
  uint16_t Text_length;
  uint8_t  Id1;
  uint8_t  Id2;
  uint16_t Again_length;
  uint8_t  Start;
  char     Identity[57];
} __attribute__((packed));

struct SbpFrame {
  uint8_t  HDOP;
  uint8_t  SVIDCnt;
  uint16_t UtcSec;
  uint32_t date_time_UTC_packed;
  uint32_t SVIDList;
  int32_t  Lat;
  int32_t  Lon;
  int32_t  AltCM;
  uint16_t Sog;
  uint16_t Cog;
  int16_t  ClmbRte;
  uint8_t  sdop;
  uint8_t  vsdop;
} __attribute__((packed));

static_assert(sizeof(SbpHeader) == SBP_HEADER_SIZE, "SBP header layout changed");
static_assert(sizeof(SbpFrame) == SBP_FRAME_SIZE, "SBP frame layout changed");

inline double sbp_frame_lat(const SbpFrame& frame)
{
  return frame.Lat * 0.0000001;
}

inline double sbp_frame_lon(const SbpFrame& frame)
{
  return frame.Lon * 0.0000001;
}

inline float sbp_frame_knots(const SbpFrame& frame)
{
  return static_cast<float>(frame.Sog) * 10.0f * MMPS_TO_KNOTS;
}
