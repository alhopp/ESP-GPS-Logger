#pragma once

// ============================================================================
// geojson_json_writer.h
//
// Tiny formatting helpers for the streaming GeoJSON writers. These intentionally
// stay small: the export still writes directly to File without a JSON framework.
// ============================================================================

#include <FS.h>

inline void geojson_write_comma_line(File& file)
{
  file.println(",");
}

inline void geojson_write_property_name(File& file, const char* name)
{
  file.print("\"");
  file.print(name);
  file.print("\":");
}
