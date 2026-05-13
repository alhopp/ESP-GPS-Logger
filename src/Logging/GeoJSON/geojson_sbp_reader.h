#pragma once

// ============================================================================
// geojson_sbp_reader.h
//
// SBP reader API for GeoJSON export. Owns the SBP frame layout, header offset,
// frame counting, and basic frame-to-display-unit conversions.
// ============================================================================

#include <FS.h>

#include "Logging/SBP/sbp_format.h"

using GeoJsonSbpFrame = SbpFrame;

bool geojson_sbp_open(File& file, const char* sbpPath);
bool geojson_sbp_read_frame(File& file, GeoJsonSbpFrame& frame);
int geojson_sbp_count_frames(const char* sbpPath);

double geojson_sbp_frame_lat(const GeoJsonSbpFrame& frame);
double geojson_sbp_frame_lon(const GeoJsonSbpFrame& frame);
float geojson_sbp_frame_knots(const GeoJsonSbpFrame& frame);
