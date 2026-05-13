// ============================================================================
// geojson_stats_writer.cpp
//
// Writes the stats property object attached to the GeoJSON base track feature.
// ============================================================================

#include "Logging/GeoJSON/geojson_stats_writer.h"

#include "Logging/GeoJSON/geojson_json_writer.h"

void geojson_write_stats(File& file, const GeoJSONStats& stats)
{
  geojson_write_comma_line(file);
  file.println("\"stats\":{");
  geojson_write_property_name(file, "nm");
  file.print(stats.nm, 3);
  geojson_write_comma_line(file);
  geojson_write_property_name(file, "alpha");
  file.print(stats.alpha, 3);
  geojson_write_comma_line(file);
  geojson_write_property_name(file, "alphaDistance");
  file.print(stats.alphaDistance, 1);
  geojson_write_comma_line(file);
  geojson_write_property_name(file, "alphaClosure");
  file.print(stats.alphaClosure, 1);
  geojson_write_comma_line(file);
  geojson_write_property_name(file, "h1");
  file.print(stats.h1, 3);
  geojson_write_comma_line(file);
  geojson_write_property_name(file, "max");
  file.print(stats.max, 3);
  geojson_write_comma_line(file);
  geojson_write_property_name(file, "avg10");
  file.print(stats.avg10, 3);
  geojson_write_comma_line(file);
  geojson_write_property_name(file, "r10");
  file.print("[");
  for (int i = 0; i < 5; i++) {
    if (i) file.print(",");
    file.print(stats.r10[i], 3);
  }
  file.println("],");
  geojson_write_property_name(file, "distance");
  file.print(stats.distance, 3);
  file.println("}");
}
