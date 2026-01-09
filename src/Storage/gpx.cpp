#include "Storage/gpx.h"            // GPX format definitions (GPX_HEADER / GPX_FRAME / GPX_END)
#include <FS.h>             // Arduino filesystem abstraction (File)
#include "Globals.h"        // Global GPS state (ubxMessage, SW_version, etc.)
#include "Ublox/Ublox.h"

// -----------------------------------------------------------------------------
// log_GPX
//
// Writes GPX data to an already-open file.
// The function is called multiple times with different "part" values:
//
//   GPX_HEADER → write XML + GPX header
//   GPX_FRAME  → append one track point (once per second)
//   GPX_END    → close GPX structure
//
// Notes:
// - File must already be opened for writing
// - Uses u-blox NAV-PVT + NAV-DOP data (global ubxMessage)
// - Track points are written only on full-second boundaries
// -----------------------------------------------------------------------------
void log_GPX(int part, File file)
{
    char bufferTx[512];   // Temporary transmit buffer for one GPX block
    int i, y;              // i = sprintf return length, y = running buffer index

    // Time and satellite fields
    int year, month, day, hour, minute, sec, sat;

    // -------------------------------------------------------------------------
    // GPX HEADER
    // -------------------------------------------------------------------------
    if (part == GPX_HEADER)
    {
        y = 0;

        // XML declaration
        i = sprintf(bufferTx,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        y += i;

        // GPX root element with creator name
        i = sprintf(&bufferTx[y],
                    "<gpx version=\"1.0\" creator=\"ESP-GPS %s\" ",
                    SW_version);
        y += i;

        // GPX namespace
        i = sprintf(&bufferTx[y],
                    "xmlns=\"http://www.topografix.com/GPX/1/0\" ");
        y += i;

        // XML schema namespace
        i = sprintf(&bufferTx[y],
                    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
        y += i;

        // Schema location
        i = sprintf(&bufferTx[y],
                    "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 "
                    "https://www.topografix.com/GPX/1/0/gpx.xsd\">\n");
        y += i;

        // Track + track segment start
        i = sprintf(&bufferTx[y], "  <trk>\n");     y += i;
        i = sprintf(&bufferTx[y], "    <trkseg>\n"); y += i;

        // Write header to file
        file.write((const uint8_t *)bufferTx, y);
    }

    // -------------------------------------------------------------------------
    // GPX TRACK POINT (one per second)
    // -------------------------------------------------------------------------
    if (part == GPX_FRAME)
    {
        // Only log on full second boundaries
        // ubxMessage.navPvt.nano is nanoseconds within the current second
        if (ubxMessage.navPvt.nano / 1000000 == 0)
        {
            float lat, lon, hdop, speed, msl, course;

            // Convert u-blox fixed-point values to human units
            lat    = ubxMessage.navPvt.lat     / 10000000.0f;
            lon    = ubxMessage.navPvt.lon     / 10000000.0f;
            hdop   = ubxMessage.navDOP.hDOP    / 100.0f;      // 0.01 resolution
            course = ubxMessage.navPvt.heading / 100000.0f;  // degrees
            speed  = ubxMessage.navPvt.gSpeed  / 1000.0f;    // m/s
            msl    = ubxMessage.navPvt.hMSL    / 1000.0f;    // meters
            sat    = ubxMessage.navPvt.numSV;

            // Timestamp components
            year   = ubxMessage.navPvt.year;
            month  = ubxMessage.navPvt.month;
            day    = ubxMessage.navPvt.day;
            hour   = ubxMessage.navPvt.hour;
            minute = ubxMessage.navPvt.minute;
            sec    = ubxMessage.navPvt.second;

            y = 0;

            // Track point start with latitude / longitude
            i = sprintf(bufferTx,
                        "      <trkpt lat=\"%.7f\" lon=\"%.7f\">\n",
                        lat, lon);
            y += i;

            // Elevation (meters above sea level)
            i = sprintf(&bufferTx[y],
                        "        <ele>%.2f</ele>\n",
                        msl);
            y += i;

            // UTC timestamp (ISO-8601)
            i = sprintf(&bufferTx[y],
                        "        <time>%d-%02d-%02dT%02d:%02d:%02dZ</time>\n",
                        year, month, day, hour, minute, sec);
            y += i;

            // Course over ground (degrees)
            i = sprintf(&bufferTx[y],
                        "        <course>%.0f</course>\n",
                        course);
            y += i;

            // Ground speed (m/s)
            i = sprintf(&bufferTx[y],
                        "        <speed>%.2f</speed>\n",
                        speed);
            y += i;

            // Number of satellites used
            i = sprintf(&bufferTx[y],
                        "        <sat>%d</sat>\n",
                        sat);
            y += i;

            // Horizontal dilution of precision
            i = sprintf(&bufferTx[y],
                        "        <hdop>%.2f</hdop>\n",
                        hdop);
            y += i;

            // Track point end
            i = sprintf(&bufferTx[y],
                        "      </trkpt>\n");
            y += i;

            // Write frame to file
            file.write((const uint8_t *)bufferTx, y);
        }
    }

    // -------------------------------------------------------------------------
    // GPX FOOTER
    // -------------------------------------------------------------------------
    if (part == GPX_END)
    {
        y = 0;

        // Close track segment, track, and GPX document
        i = sprintf(bufferTx, "    </trkseg>\n"); y += i;
        i = sprintf(&bufferTx[y], "  </trk>\n");  y += i;
        i = sprintf(&bufferTx[y], "</gpx>\n");    y += i;

        file.write((const uint8_t *)bufferTx, y);
    }
}
