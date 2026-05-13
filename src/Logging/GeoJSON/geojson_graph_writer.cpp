// ============================================================================
// geojson_graph_writer.cpp
//
// Writes the graph series and graph metadata objects attached to the GeoJSON
// base track feature.
// ============================================================================

#include "Logging/GeoJSON/geojson_graph_writer.h"

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

void writeGraphSeries(File& file, const char* name, const GeoJSONGraphSeries& series)
{
  writePropertyName(file, name);
  file.print("[");

  for (int i = 0; i < series.count; i++) {
    if (i) file.print(",");
    file.print(series.values[i], 2);
  }

  file.print("]");
}

void writeGraphSeriesList(File& file, const char* name, const GeoJSONGraphSeries* series, int count)
{
  writePropertyName(file, name);
  file.print("[");

  for (int i = 0; i < count; i++) {
    if (i) file.print(",");
    file.print("[");
    for (int j = 0; j < series[i].count; j++) {
      if (j) file.print(",");
      file.print(series[i].values[j], 2);
    }
    file.print("]");
  }

  file.print("]");
}

void writeGraphMetaSeries(File& file, const char* name, const GeoJSONGraphSeries& series)
{
  writePropertyName(file, name);
  file.print("{\"xMax\":");
  file.print(series.xMax, 3);
  file.print(",\"xUnit\":\"");
  file.print(series.xUnit ? series.xUnit : "");
  file.print("\"}");
}
}

void geojson_write_graphs(File& file, const GeoJSONGraphs& graphs)
{
  writeCommaLine(file);
  file.println("\"graphs\":{");
  writeGraphSeries(file, "2s", graphs.s2);
  writeCommaLine(file);
  writeGraphSeriesList(file, "10s", graphs.s10, graphs.s10Count);
  writeCommaLine(file);
  writeGraphSeries(file, "alpha", graphs.alpha);
  writeCommaLine(file);
  writeGraphSeries(file, "nm", graphs.nm);
  writeCommaLine(file);
  writeGraphSeries(file, "1h", graphs.h1);
  writeCommaLine(file);
  writeGraphSeries(file, "distance", graphs.distance);
  file.println();
  file.print("},");

  file.println();
  file.println("\"graph_meta\":{");
  writeGraphMetaSeries(file, "2s", graphs.s2);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "10s", graphs.s10Count > 0 ? graphs.s10[0] : graphs.s2);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "alpha", graphs.alpha);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "nm", graphs.nm);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "1h", graphs.h1);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "distance", graphs.distance);
  file.println();
  file.print("}");
}
