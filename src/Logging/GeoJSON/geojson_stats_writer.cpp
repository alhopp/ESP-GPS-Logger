// ============================================================================
// geojson_stats_writer.cpp
//
// Writes the stats property object attached to the GeoJSON base track feature.
// ============================================================================

#include "Logging/GeoJSON/geojson_stats_writer.h"

namespace {
void writeCommaLine(File& file)
{
  file.println(",");
}

void writePropertyName(File& file, const char* name)
{
  file.print("\"");
  file.print(name);
  file.print("\":");
}
}

void geojson_write_stats(File& file, const GeoJSONStats& stats)
{
  writeCommaLine(file);
  file.println("\"stats\":{");
  writePropertyName(file, "nm");
  file.print(stats.nm, 3);
  writeCommaLine(file);
  writePropertyName(file, "alpha");
  file.print(stats.alpha, 3);
  writeCommaLine(file);
  writePropertyName(file, "alphaDistance");
  file.print(stats.alphaDistance, 1);
  writeCommaLine(file);
  writePropertyName(file, "alphaClosure");
  file.print(stats.alphaClosure, 1);
  writeCommaLine(file);
  writePropertyName(file, "h1");
  file.print(stats.h1, 3);
  writeCommaLine(file);
  writePropertyName(file, "max");
  file.print(stats.max, 3);
  writeCommaLine(file);
  writePropertyName(file, "avg10");
  file.print(stats.avg10, 3);
  writeCommaLine(file);
  writePropertyName(file, "r10");
  file.print("[");
  for (int i = 0; i < 5; i++) {
    if (i) file.print(",");
    file.print(stats.r10[i], 3);
  }
  file.println("],");
  writePropertyName(file, "distance");
  file.print(stats.distance, 3);
  file.println("}");
}
