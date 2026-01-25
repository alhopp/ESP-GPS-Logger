#include "geojson.h"
#include <SD_MMC.h>

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
static File geoFile;
static bool firstPoint = true;

static bool hasStats = false;
static GeoJSONStats stats;

// -----------------------------------------------------------------------------
// Stats setter (called by session controller)
// -----------------------------------------------------------------------------
void geojson_set_stats(const GeoJSONStats& s)
{
  stats = s;
  hasStats = true;
}

// -----------------------------------------------------------------------------
// Start new GeoJSON session
// -----------------------------------------------------------------------------
void geojson_begin(const char* filename)
{
  if(geoFile) geoFile.close();
  if(SD_MMC.exists(filename)) SD_MMC.remove(filename);

  geoFile = SD_MMC.open(filename, FILE_WRITE);
  if(!geoFile) return;

  firstPoint = true;
  hasStats   = false;

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
void geojson_add_point(double lat, double lon)
{
  if(!geoFile) return;

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

  geoFile.println();
  geoFile.println("]");
  geoFile.println("},");
  geoFile.println("\"properties\":{");

  if(hasStats){
    geoFile.println("\"stats\":{");
    geoFile.print("\"nm\":");       geoFile.print(stats.nm,2);       geoFile.println(",");
    geoFile.print("\"alpha\":");    geoFile.print(stats.alpha,2);    geoFile.println(",");
    geoFile.print("\"h1\":");       geoFile.print(stats.h1,2);       geoFile.println(",");
    geoFile.print("\"max\":");      geoFile.print(stats.max,2);      geoFile.println(",");
    geoFile.print("\"avg10\":");    geoFile.print(stats.avg10,2);    geoFile.println(",");
    geoFile.print("\"distance\":"); geoFile.print(stats.distance,2);
    geoFile.println("}");
  }

  geoFile.println("}");
  geoFile.println("}]");
  geoFile.println("}");

  geoFile.flush();
  geoFile.close();
}
