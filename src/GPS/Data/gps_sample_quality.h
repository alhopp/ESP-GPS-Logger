#pragma once

// ============================================================================
// gps_sample_quality.h
//
// Sample-level quality gate for GPS statistics.
//
// gps_data.cpp calls this before writing a NAV-PVT sample into the shared rings.
// Bad samples are not allowed to contribute speed or distance; their position is
// held at the last trusted coordinate so alpha/distance geometry does not jump.
// ============================================================================

#include <stdint.h>

struct GpsSampleQualityResult {
  bool good;          // True when the raw sample passed every quality check.
  float latitude;     // Raw latitude when good, otherwise last trusted latitude.
  float longitude;    // Raw longitude when good, otherwise last trusted longitude.
  uint32_t gSpeed;    // Raw Doppler speed when good, otherwise 0 mm/s.
};

// Filter one raw GPS sample.
// latitude/longitude are decimal degrees; gSpeed is UBX NAV-PVT gSpeed in mm/s.
GpsSampleQualityResult gps_sample_quality_filter(float latitude,
                                                 float longitude,
                                                 uint32_t gSpeed);

// Clear the "last trusted position" memory at the start of a new session.
void gps_sample_quality_reset();
