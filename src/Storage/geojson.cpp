#include "geojson.h"
#include <SD_MMC.h>

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
static File geoFile;
static bool firstPoint = true;

// -----------------------------------------------------------------------------
// Start new GeoJSON session
// -----------------------------------------------------------------------------
void geojson_begin(const char* filename)
{
  // Guard against double-open
  if(geoFile) geoFile.close();

  // FILE_WRITE truncates if file does not exist,
  // but appends if it does — so remove first to be safe
  if(SD_MMC.exists(filename)) SD_MMC.remove(filename);

  geoFile = SD_MMC.open(filename, FILE_WRITE);
  if(!geoFile) return;

  firstPoint = true;

  // GeoJSON header
  geoFile.println("{");
  geoFile.println("\"type\":\"FeatureCollection\",");
  geoFile.println("\"features\":[{");
  geoFile.println("\"type\":\"Feature\",");
  geoFile.println("\"geometry\":{");
  geoFile.println("\"type\":\"LineString\",");
  geoFile.println("\"coordinates\":[");
}

// -----------------------------------------------------------------------------
// Append a GPS coordinate
// -----------------------------------------------------------------------------
// NOTE:
// - Coordinates must be [lon, lat] per GeoJSON spec
// - Decimation / validation happens outside this module
// -----------------------------------------------------------------------------
void geojson_add_point(double lat, double lon)
{
  if(!geoFile) return;

  // Comma between coordinates (not before first)
  if(!firstPoint) geoFile.println(",");
  firstPoint = false;

  geoFile.print("[");
  geoFile.print(lon, 6);
  geoFile.print(",");
  geoFile.print(lat, 6);
  geoFile.print("]");
}

// -----------------------------------------------------------------------------
// Finalise and close GeoJSON file
// -----------------------------------------------------------------------------
void geojson_end()
{
  if(!geoFile) return;

  // Close JSON structure
  geoFile.println();
  geoFile.println("]");            // coordinates
  geoFile.println("},");           // geometry
  geoFile.println("\"properties\":{}");
  geoFile.println("}]");           // features
  geoFile.println("}");            // root

  geoFile.flush();
  geoFile.close();
}
