// ============================================================================
// geojson_sbp_reader.cpp
//
// Reads the project SBP session format for GeoJSON export. Keeps binary layout
// details out of the higher-level GeoJSON export orchestration.
// ============================================================================

#include "Logging/geojson_sbp_reader.h"

#include "GPS/gps_config.h"
#include "Storage/storage_manager.h"

namespace {
constexpr int SBP_HEADER_SIZE = 64;
}

bool geojson_sbp_open(File& file, const char* sbpPath)
{
  fs::FS& storage = storage_sd_fs();
  file = storage.open(sbpPath, FILE_READ);
  if (!file) return false;
  if (file.size() <= SBP_HEADER_SIZE) {
    file.close();
    return false;
  }
  file.seek(SBP_HEADER_SIZE);
  return true;
}

bool geojson_sbp_read_frame(File& file, GeoJsonSbpFrame& frame)
{
  return file.read(reinterpret_cast<uint8_t*>(&frame), sizeof(frame)) == sizeof(frame);
}

int geojson_sbp_count_frames(const char* sbpPath)
{
  fs::FS& storage = storage_sd_fs();
  File file = storage.open(sbpPath, FILE_READ);
  if (!file) return 0;

  const size_t size = file.size();
  file.close();

  if (size <= SBP_HEADER_SIZE) return 0;
  return (size - SBP_HEADER_SIZE) / sizeof(GeoJsonSbpFrame);
}

double geojson_sbp_frame_lat(const GeoJsonSbpFrame& frame)
{
  return frame.Lat * 0.0000001;
}

double geojson_sbp_frame_lon(const GeoJsonSbpFrame& frame)
{
  return frame.Lon * 0.0000001;
}

float geojson_sbp_frame_knots(const GeoJsonSbpFrame& frame)
{
  return static_cast<float>(frame.Sog) * 10.0f * MMPS_TO_KNOTS;
}
