// ============================================================================
// gps_data.cpp
//
// Root of the GPS statistics data graph.
//
// This file owns the shared circular buffers that all metric calculators read.
// Every accepted NAV-PVT sample enters the stats system through
// GPS_data::push_data(); distance, time, alpha, GeoJSON, and display code all
// derive their values from the buffers maintained here.
//
// Keep this module deliberately simple:
// - store the latest filtered GPS sample
// - maintain quality flags
// - accumulate session/run/alpha distance
// - build the 1 Hz speed buffer used by long time-window stats
//
// It should not decide screen modes, logging policy, or final rankings.
// ============================================================================

#include "GPS/Data/gps_data.h"
#include "Core/system_info.h"
#include "GPS/Data/gps_sample_quality.h"
#include "GPS/gps_runtime_state.h"

// ============================================================================
// Shared GPS buffers
// ============================================================================

uint16_t _gSpeed[BUFFER_SIZE];    // Doppler speed per GPS sample, mm/s.
uint16_t _sogCms[BUFFER_SIZE];    // Same speed in cm/s for SBP-style output.
uint16_t _secSpeed[BUFFER_SIZE];  // 1-second averaged speed, mm/s.
int      _sbpIndex[BUFFER_SIZE];  // 1-based SBP frame index for this sample.
bool     _sampleGood[BUFFER_SIZE];// False means speed was zeroed and position may be held.

float _lat[BUFFER_ALFA];          // Latitude ring buffer, decimal degrees.
float _long[BUFFER_ALFA];         // Longitude ring buffer, decimal degrees.

int index_GPS = -1;               // Monotonic NAV-PVT sample index.
int index_sec = -1;               // Monotonic 1 Hz speed-bucket index.

int   alfa_counter;               // Incremented by run/jibe detection.
float total_distance = 0.0f;      // Session distance, mm.

// Maps each 1 Hz bucket back to the GPS sample index that closed it. This lets
// export code turn long-window results, such as 1 hour, back into coordinates.
int sec_to_gps_index[BUFFER_SIZE] = {0};

namespace {
// Accumulates the current 1-second bucket. Long-window calculators
// (30-minute / 1-hour) use _secSpeed[] instead of the high-rate GPS buffer.
uint32_t second_speed_sum_mmps = 0;

// Ring index for speed and quality buffers. These buffers only keep the most
// recent BUFFER_SIZE samples; index_GPS remains monotonic for window math.
int gpsRingIndex()
{
  return index_GPS % BUFFER_SIZE;
}

// Separate ring index for alpha geometry. Alpha works from positions, not just
// speed, so it has its own buffer depth.
int alphaRingIndex()
{
  return index_GPS % BUFFER_ALFA;
}

// Protects this central ingestion path from divide-by-zero if static system
// info is ever missing or malformed during startup.
int safeSampleRate()
{
  return systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
}

uint32_t sbpQuantizedSpeedMmps(uint32_t speedMmps)
{
  return (speedMmps / 10U) * 10U;
}

// Store the filtered/sanitized sample into the buffers consumed by all metric
// modules. gps_sample_quality_filter() may have zeroed speed and reused the
// previous good position before we get here. Speeds are quantized to the SBP
// cm/s value so logged stats match Speedreader's SBP input.
void storeSample(int gpsIdx,
                 int alphaIdx,
                 const GpsSampleQualityResult& sample)
{
  const uint32_t speedMmps = sbpQuantizedSpeedMmps(sample.gSpeed);

  _gSpeed[gpsIdx] = speedMmps;
  _sogCms[gpsIdx] = static_cast<uint16_t>(speedMmps / 10U);
  _sbpIndex[gpsIdx] = 0;
  _sampleGood[gpsIdx] = sample.good;

  _lat[alphaIdx] = sample.latitude;
  _long[alphaIdx] = sample.longitude;
}

// Distance totals intentionally use only trusted samples. This prevents bad
// points from increasing session distance, run distance, or alpha path distance.
void accumulateDistanceIfGood(const GpsSampleQualityResult& sample,
                              float& runDistance,
                              float& alphaDistance)
{
  if (!sample.good) return;

  const float distanceMm = static_cast<float>(sbpQuantizedSpeedMmps(sample.gSpeed)) / safeSampleRate();

  total_distance += distanceMm;
  runDistance += distanceMm;
  alphaDistance += distanceMm;
}

// Build one averaged speed sample per second. The high-rate _gSpeed[] ring is
// too short for 30-minute/1-hour windows, so those stats run on this 1 Hz ring.
void updateOneSecondSpeed(int gpsIdx)
{
  second_speed_sum_mmps += _gSpeed[gpsIdx];

  if ((index_GPS % safeSampleRate()) != 0) return;

  index_sec++;

  const int secIdx = index_sec % BUFFER_SIZE;
  _secSpeed[secIdx] = second_speed_sum_mmps / safeSampleRate();
  sec_to_gps_index[secIdx] = index_GPS;

  second_speed_sum_mmps = 0;
}
} // namespace

// ============================================================================
// GPS_data
// ============================================================================

GPS_data::GPS_data()
{
  // Preserves the legacy RP6 startup model. index_GPS is also reset by globals,
  // but the singleton GPS_data instance owns the runtime starting point.
  index_GPS = 0;
}

void gps_data_reset_quality_state()
{
  // Reset both the stateful quality filter and the per-sample flags stored in
  // the ring. Call this at session start so old labels cannot leak forward.
  gps_sample_quality_reset();

  for (int i = 0; i < BUFFER_SIZE; i++) {
    _gSpeed[i] = 0;
    _sogCms[i] = 0;
    _secSpeed[i] = 0;
    _sbpIndex[i] = 0;
    _sampleGood[i] = false;
    sec_to_gps_index[i] = 0;
  }

  index_GPS = 0;
  index_sec = -1;
  second_speed_sum_mmps = 0;
}

void GPS_data::push_data(float latitude, float longitude, uint32_t gSpeed)
{
  // The monotonic index is the timeline for all downstream metric windows.
  index_GPS++;

  const int gpsIdx = gpsRingIndex();
  const int alphaIdx = alphaRingIndex();

  const GpsSampleQualityResult sample =
      gps_sample_quality_filter(latitude, longitude, gSpeed);

  // Step 1: write speed/position/quality into the shared rings.
  storeSample(gpsIdx, alphaIdx, sample);

  // Step 2: update distance totals only if this sample was trusted.
  accumulateDistanceIfGood(sample, run_distance, alfa_distance);

  // Step 3: maintain the slower 1 Hz speed ring for long time-window stats.
  updateOneSecondSpeed(gpsIdx);
}
