#pragma once

// ============================================================================
// geojson_sbp_reader.h
//
// SBP reader API for GeoJSON export. Owns the SBP frame layout, header offset,
// frame counting, and basic frame-to-display-unit conversions.
// ============================================================================

#include <FS.h>

struct GeoJsonSbpFrame {
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

bool geojson_sbp_open(File& file, const char* sbpPath);
bool geojson_sbp_read_frame(File& file, GeoJsonSbpFrame& frame);
int geojson_sbp_count_frames(const char* sbpPath);

double geojson_sbp_frame_lat(const GeoJsonSbpFrame& frame);
double geojson_sbp_frame_lon(const GeoJsonSbpFrame& frame);
float geojson_sbp_frame_knots(const GeoJsonSbpFrame& frame);
