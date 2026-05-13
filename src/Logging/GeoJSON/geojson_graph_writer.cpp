// ============================================================================
// geojson_graph_writer.cpp
//
// Writes the graph series and graph metadata objects attached to the GeoJSON
// base track feature.
// ============================================================================

#include "Logging/GeoJSON/geojson_graph_writer.h"

namespace {
void writeGraphSeries(File& file, const char* name, const GeoJSONGraphSeries& series)
{
  file.print("\"");
  file.print(name);
  file.print("\":[");

  for (int i = 0; i < series.count; i++) {
    if (i) file.print(",");
    file.print(series.values[i], 2);
  }

  file.print("]");
}

void writeGraphSeriesList(File& file, const char* name, const GeoJSONGraphSeries* series, int count)
{
  file.print("\"");
  file.print(name);
  file.print("\":[");

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
  file.print("\"");
  file.print(name);
  file.print("\":{\"xMax\":");
  file.print(series.xMax, 3);
  file.print(",\"xUnit\":\"");
  file.print(series.xUnit ? series.xUnit : "");
  file.print("\"}");
}
}

void geojson_write_graphs(File& file, const GeoJSONGraphs& graphs)
{
  file.println(",");
  file.println("\"graphs\":{");
  writeGraphSeries(file, "2s", graphs.s2);
  file.println(",");
  writeGraphSeriesList(file, "10s", graphs.s10, graphs.s10Count);
  file.println(",");
  writeGraphSeries(file, "alpha", graphs.alpha);
  file.println(",");
  writeGraphSeries(file, "nm", graphs.nm);
  file.println(",");
  writeGraphSeries(file, "1h", graphs.h1);
  file.println(",");
  writeGraphSeries(file, "distance", graphs.distance);
  file.println();
  file.print("},");

  file.println();
  file.println("\"graph_meta\":{");
  writeGraphMetaSeries(file, "2s", graphs.s2);
  file.println(",");
  writeGraphMetaSeries(file, "10s", graphs.s10Count > 0 ? graphs.s10[0] : graphs.s2);
  file.println(",");
  writeGraphMetaSeries(file, "alpha", graphs.alpha);
  file.println(",");
  writeGraphMetaSeries(file, "nm", graphs.nm);
  file.println(",");
  writeGraphMetaSeries(file, "1h", graphs.h1);
  file.println(",");
  writeGraphMetaSeries(file, "distance", graphs.distance);
  file.println();
  file.print("}");
}
