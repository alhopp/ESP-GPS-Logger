#include "GPS/Data/gps_satellite_quality.h"

// ============================================================================
// gps_satellite_quality.cpp
//
// Rolling NAV-SAT C/N0 and satellite-count metadata.
// These values annotate stats/diagnostics; NAV-PVT acceptance is handled by
// gps_sample_quality instead.
// ============================================================================

#include <algorithm>

#include "GPS/Ublox/ublox_driver.h"

namespace {
bool satelliteUsedInNavigation(const sVs_NAV_SAT& sat)
{
  return sat.flags & 0x08;
}
} // namespace

GPS_SAT_info::GPS_SAT_info()
{
  index_SAT_info = 0;
}

void GPS_SAT_info::push_SAT_info(const NAV_SAT_HDR&,
                                 const sVs_NAV_SAT* sats,
                                 uint8_t count)
{
  // First summarize this NAV-SAT frame. Only satellites used by the receiver's
  // navigation solution are useful for result-quality metadata.
  mean_cno = 0;
  min_cno = 0xFF;
  max_cno = 0;
  nr_sats = 0;

  for (uint8_t i = 0; i < count; i++) {
    if (satelliteUsedInNavigation(sats[i])) {
      mean_cno += sats[i].cno;
      min_cno = std::min(min_cno, (uint32_t)sats[i].cno);
      max_cno = std::max(max_cno, (uint32_t)sats[i].cno);
      nr_sats++;
    }
  }

  // No used satellites: advance the frame counter but do not overwrite the
  // rolling ring with meaningless C/N0 values.
  if (!nr_sats) {
    index_SAT_info++;
    return;
  }

  mean_cno /= nr_sats;
  const int idx = index_SAT_info % NAV_SAT_BUFFER;

  // Store this frame in the ring.
  sat_info.Mean_cno[idx] = mean_cno;
  sat_info.Max_cno[idx] = max_cno;
  sat_info.Min_cno[idx] = min_cno;
  sat_info.numSV[idx] = nr_sats;

  // Once the ring is warm, publish rolling means for the speed-result metadata.
  if (index_SAT_info > NAV_SAT_BUFFER) {
    mean_cno = 0;
    max_cno = 0;
    min_cno = 0;
    nr_sats = 0;

    for (int i = 0; i < NAV_SAT_BUFFER; i++) {
      int j = (index_SAT_info - NAV_SAT_BUFFER + i) % NAV_SAT_BUFFER;
      mean_cno += sat_info.Mean_cno[j];
      max_cno += sat_info.Max_cno[j];
      min_cno += sat_info.Min_cno[j];
      nr_sats += sat_info.numSV[j];
    }

    sat_info.Mean_mean_cno = mean_cno / NAV_SAT_BUFFER;
    sat_info.Mean_max_cno = max_cno / NAV_SAT_BUFFER;
    sat_info.Mean_min_cno = min_cno / NAV_SAT_BUFFER;
    sat_info.Mean_numSV = nr_sats / NAV_SAT_BUFFER;
  }

  index_SAT_info++;
}
