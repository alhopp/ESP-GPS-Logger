// ============================================================================
// geojson_stats_writer.cpp
//
// Writes the stats property object attached to the GeoJSON base track feature.
// ============================================================================

#include "Logging/GeoJSON/geojson_stats_writer.h"

void geojson_write_stats(File& file, const GeoJSONStats& stats)
{
  file.println(",");
  file.println("\"stats\":{");
  file.print("\"nm\":");
  file.print(stats.nm, 3);
  file.println(",");
  file.print("\"alpha\":");
  file.print(stats.alpha, 3);
  file.println(",");
  file.print("\"alphaDistance\":");
  file.print(stats.alphaDistance, 1);
  file.println(",");
  file.print("\"alphaClosure\":");
  file.print(stats.alphaClosure, 1);
  file.println(",");
  file.print("\"h1\":");
  file.print(stats.h1, 3);
  file.println(",");
  file.print("\"max\":");
  file.print(stats.max, 3);
  file.println(",");
  file.print("\"avg10\":");
  file.print(stats.avg10, 3);
  file.println(",");
  file.print("\"r10\":[");
  for (int i = 0; i < 5; i++) {
    if (i) file.print(",");
    file.print(stats.r10[i], 3);
  }
  file.println("],");
  file.print("\"distance\":");
  file.print(stats.distance, 3);
  file.println("}");
}
