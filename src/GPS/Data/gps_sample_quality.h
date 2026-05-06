#pragma once

// Sample-level quality gate for GPS statistics.
// Bad samples are sanitized before gps_data stores them, preventing spikes from
// inflating 2s/10s/distance/alpha results.

#include <stdint.h>

struct GpsSampleQualityResult {
  bool good;
  float latitude;
  float longitude;
  uint32_t gSpeed;
};

GpsSampleQualityResult gps_sample_quality_filter(float latitude,
                                                 float longitude,
                                                 uint32_t gSpeed);

void gps_sample_quality_reset();
