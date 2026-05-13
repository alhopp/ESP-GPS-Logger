#pragma once

// ============================================================================
// geojson_export_limits.h
//
// Shared GeoJSON export sizing limits. Keeps graph and feature downsampling
// aligned without spreading magic constants through export code.
// ============================================================================

constexpr int GEOJSON_MAX_SERIES_POINTS = 240;
