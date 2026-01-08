#include "gpx.h" // Include the header file
#include <FS.h>   // Include necessary files for FS support
#include "Globals.h"   // Include necessary files for FS support

void log_GPX(int part, File file) {
    char bufferTx[512]; 
    int i, y; 
    int year, month, day, hour, minute, sec, sat;
    
    if (part == GPX_HEADER) { 
        y = 0;
        i = sprintf(bufferTx, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"); y = y + i;
        i = sprintf(&bufferTx[y], "<gpx version=\"1.0\" creator=\"ESP-GPS %s\" ", SW_version); y = y + i;
        i = sprintf(&bufferTx[y], "xmlns=\"http://www.topografix.com/GPX/1/0\" "); y = y + i;
        i = sprintf(&bufferTx[y], "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "); y = y + i;
        i = sprintf(&bufferTx[y], "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 https://www.topografix.com/GPX/1/0/gpx.xsd\">\n"); y = y + i;
        i = sprintf(&bufferTx[y], "  <trk>\n"); y = y + i;
        i = sprintf(&bufferTx[y], "    <trkseg>\n"); y = y + i;
        file.write((const uint8_t *)&bufferTx, y);
    } 
    
    if (part == GPX_FRAME) {  
        if (ubxMessage.navPvt.nano / 1000000 == 0) { // only log every full second
            float lat, lon, hdop, speed, msl, course;
            lat = ubxMessage.navPvt.lat / 10000000.0f;
            lon = ubxMessage.navPvt.lon / 10000000.0f;
            hdop = ubxMessage.navDOP.hDOP / 100.0f; // resolution in ubx nav dop is 0.01
            course = ubxMessage.navPvt.heading / 100000.0f;
            speed = ubxMessage.navPvt.gSpeed / 1000.0f;
            msl = ubxMessage.navPvt.hMSL / 1000.0f;
            sat = ubxMessage.navPvt.numSV;
            year = ubxMessage.navPvt.year;
            month = ubxMessage.navPvt.month;
            day = ubxMessage.navPvt.day;
            hour = ubxMessage.navPvt.hour;
            minute = ubxMessage.navPvt.minute;
            sec = ubxMessage.navPvt.second;

            y = 0;
            i = sprintf(bufferTx, "      <trkpt lat=\"%.7f\" lon=\"%.7f\">\n", lat, lon); y = y + i;
            i = sprintf(&bufferTx[y], "        <ele>%.2f</ele>\n", msl); y = y + i; // was float !!!
            i = sprintf(&bufferTx[y], "        <time>%d-%'02d-%'02dT%'02d:%'02d:%'02dZ</time>\n", year, month, day, hour, minute, sec); y = y + i;
            i = sprintf(&bufferTx[y], "        <course>%.0f</course>\n", course); y = y + i;
            i = sprintf(&bufferTx[y], "        <speed>%.2f</speed>\n", speed); y = y + i;
            i = sprintf(&bufferTx[y], "        <sat>%d</sat>\n", sat); y = y + i;
            i = sprintf(&bufferTx[y], "        <hdop>%.2f</hdop>\n", hdop); y = y + i;
            i = sprintf(&bufferTx[y], "      </trkpt>\n"); y = y + i;
            file.write((const uint8_t *)&bufferTx, y);
        }
    }

    if (part == GPX_END) {
        y = 0;
        i = sprintf(bufferTx, "    </trkseg>\n"); y = y + i;
        i = sprintf(&bufferTx[y], "  </trk>\n"); y = y + i;
        i = sprintf(&bufferTx[y], "</gpx>\n"); y = y + i;
        file.write((const uint8_t *)&bufferTx, y);
    }      
}
