#include "Logging/geojson.h"
#include <string.h>

#include "Storage/storage_manager.h"

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
namespace {
File geoFile;

bool firstFeature = true;
bool firstPoint = true;

bool hasStats = false;
GeoJSONStats stats;

const char* currentMode = nullptr;
}

// -----------------------------------------------------------------------------
// Stats setter (called once per session)
// -----------------------------------------------------------------------------
void geojson_set_stats(const GeoJSONStats& s)
{
  stats = s;
  hasStats = true;
}

// -----------------------------------------------------------------------------
// Begin GeoJSON file
// -----------------------------------------------------------------------------
bool geojson_begin(const char* filename)
{
  if (geoFile) {
    geoFile.close();
  }

  fs::FS& storage = storage_sd_fs();
  if (storage.exists(filename)) {
    storage.remove(filename);
  }

  geoFile = storage.open(filename, FILE_WRITE);
  if (!geoFile) return false;

  firstFeature = true;
  currentMode = nullptr;

  geoFile.println("{");
  geoFile.println("\"type\":\"FeatureCollection\",");
  geoFile.println("\"features\":[");
  return true;
}

// -----------------------------------------------------------------------------
// Begin a new feature (track / 2s / 10s / alpha / nm / 1h)
// -----------------------------------------------------------------------------
void geojson_begin_feature(const char* mode)
{
  if (!geoFile) return;

  if (!firstFeature) {
    geoFile.println(",");
  }
  firstFeature = false;

  firstPoint = true;
  currentMode = mode;

  geoFile.println("{");
  geoFile.println("\"type\":\"Feature\",");
  geoFile.println("\"geometry\":{");
  geoFile.println("\"type\":\"LineString\",");
  geoFile.println("\"coordinates\":[");
}

// -----------------------------------------------------------------------------
// Append coordinate to current feature
// -----------------------------------------------------------------------------
void geojson_add_point(double lat, double lon)
{
  if (!geoFile || !currentMode) return;

  if (!firstPoint) {
    geoFile.println(",");
  }
  firstPoint = false;

  char buf[64];
  snprintf(buf, sizeof(buf),
           "[%.6f,%.6f]",
           lon, lat);

  geoFile.print(buf);
}


// -----------------------------------------------------------------------------
// End current feature
// -----------------------------------------------------------------------------
void geojson_end_feature()
{
  if (!geoFile || !currentMode) return;

  geoFile.println();
  geoFile.println("]");     // end coordinates array
  geoFile.println("},");    // close geometry object

  geoFile.println("\"properties\":{");

  geoFile.print("\"mode\":\"");
  geoFile.print(currentMode);
  geoFile.print("\"");

  // Attach session stats ONLY to base track
  if (hasStats && strcmp(currentMode, "track") == 0) {
    geoFile.println(",");
    geoFile.println("\"stats\":{");
    geoFile.print("\"nm\":");
    geoFile.print(stats.nm, 3);
    geoFile.println(",");
    geoFile.print("\"alpha\":");
    geoFile.print(stats.alpha, 3);
    geoFile.println(",");
    geoFile.print("\"h1\":");
    geoFile.print(stats.h1, 3);
    geoFile.println(",");
    geoFile.print("\"max\":");
    geoFile.print(stats.max, 3);
    geoFile.println(",");
    geoFile.print("\"avg10\":");
    geoFile.print(stats.avg10, 3);
    geoFile.println(",");
    geoFile.print("\"distance\":");
    geoFile.print(stats.distance, 3);
    geoFile.println("}");
  }

  geoFile.println("}");   // end properties
  geoFile.println("}");   // end feature

  currentMode = nullptr;
}

// -----------------------------------------------------------------------------
// Finalise GeoJSON file
// -----------------------------------------------------------------------------
void geojson_end()
{
  if (!geoFile) return;

  geoFile.println();
  geoFile.println("]");
  geoFile.println("}");

  geoFile.flush();
  geoFile.close();
}
