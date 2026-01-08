#include "gpy.h"
#include <FS.h>
#include "Globals.h"  // Ensure to include any necessary global variables
#include "Ublox/Ublox.h"  // Ensure to include any necessary global variables

// Initialization of GPY structures
struct GPY_Header gpy_header = {};
struct GPY_Frame gpy_frame = {};
struct GPY_Frame_compressed gpy_frame_compressed = {};

// Function to calculate Fletcher16 checksum
uint16_t Fletcher16(uint8_t *data, int count) {
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    // Sum all the bytes, but not the last 2
    for (int i = 0; i < (count - 2); ++i) {
        sum1 = (sum1 + data[i]) & 0xFF; // divide by 256 instead of 255
        sum2 = (sum2 + sum1) & 0xFF;
    }
    data[count - 2] = sum1;
    data[count - 1] = sum2;
    return (sum2 << 8) | sum1;
}

// Function to log the GPY header to the file
void log_GPY_Header(File file) {
    // Copy firmware version into the header
    for (int i = 0; i < 16; i++) {
        gpy_header.firmwareVersion[i] = SW_version[i];
    }
    // Copy the serial number (mac address)
    sprintf(gpy_header.serialNumber, "_%2X%2X%2X%2X%2X%2X_", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // Calculate checksum
    Fletcher16((uint8_t*)&gpy_header, 72);
    // Write the header to the file
    file.write((const uint8_t *)&gpy_header, 72);
}

// Function to log GPY data to the file
void log_GPY(File file) {
    // Convert a date and time into Unix time, offset 1970, Arduino 8bytes = LL
    time_t utc_Sec;
    struct tm frame_time; // time elements structure
    frame_time.tm_sec = ubxMessage.navPvt.second;
    frame_time.tm_hour = ubxMessage.navPvt.hour;
    frame_time.tm_min = ubxMessage.navPvt.minute;
    frame_time.tm_mday = ubxMessage.navPvt.day;
    frame_time.tm_mon = ubxMessage.navPvt.month - 1; // month 0 - 11 with mktime
    frame_time.tm_year = ubxMessage.navPvt.year - 1900; // years since 1900, so deduct 1900
    frame_time.tm_isdst = 0; // No daylight saving
    utc_Sec = mktime(&frame_time) - _timezone; // mktime returns local time, but global long _timezone has the difference in sec between UTC - Local time
    int64_t utc_ms = utc_Sec * 1000LL + (ubxMessage.navPvt.nano + 500000) / 1000000LL;

    // Calculation of delta values
    int delta_time = utc_ms - gpy_frame.Unix_time; // ms
    int delta_Speed = ubxMessage.navPvt.gSpeed - gpy_frame.Speed; // mm/
    int delta_Speed_error = ubxMessage.navPvt.sAcc - gpy_frame.Speed_error; // sAccCourse_Over_Ground;
    int delta_Latitude = ubxMessage.navPvt.lat - gpy_frame.Latitude;
    int delta_Longitude = ubxMessage.navPvt.lon - gpy_frame.Longitude;
    int delta_COG = ubxMessage.navPvt.heading / 1000 - gpy_frame.COG / 1000; // delta (course / 1000)!
    
    #define SIGNED_INT 30000 // if delta is more, a full frame is written
    int full_frame = 0;
    static int first_frame = 0;
    if ((delta_time > SIGNED_INT) || (delta_time < -SIGNED_INT)) full_frame = 1;
    if ((delta_Speed > SIGNED_INT) || (delta_Speed < -SIGNED_INT)) full_frame = 1;
    if ((delta_Speed_error > SIGNED_INT) || (delta_Speed_error < -SIGNED_INT)) full_frame = 1;
    if ((delta_Latitude > SIGNED_INT) || (delta_Latitude < -SIGNED_INT)) full_frame = 1;
    if ((delta_Longitude > SIGNED_INT) || (delta_Longitude < -SIGNED_INT)) full_frame = 1;
    if ((delta_COG > SIGNED_INT) || (delta_COG < -SIGNED_INT)) full_frame = 1;
    if (first_frame == 0) full_frame = 1; // first frame is always a full frame
    if (next_gpy_full_frame) { full_frame = 1; next_gpy_full_frame = 0; } // if a navPvt frame is lost, next frame = full frame

    if (full_frame == 1) {
        gpy_frame.HDOP = ubxMessage.navDOP.hDOP; // ubxMessage.navPvt.pDOP;
        gpy_frame.Unix_time = utc_ms;
        gpy_frame.Speed = ubxMessage.navPvt.gSpeed;
        gpy_frame.Speed_error = ubxMessage.navPvt.sAcc;
        gpy_frame.Latitude = ubxMessage.navPvt.lat;
        gpy_frame.Longitude = ubxMessage.navPvt.lon;
        gpy_frame.COG = ubxMessage.navPvt.heading;
        gpy_frame.Sat = ubxMessage.navPvt.numSV;
        gpy_frame.fix = ubxMessage.navPvt.fixType;
        Fletcher16((uint8_t*)&gpy_frame, 36);
        file.write((const uint8_t*)&gpy_frame, 36);
        first_frame = 1;
    } else {
        gpy_frame_compressed.HDOP = ubxMessage.navDOP.hDOP; // ubxMessage.navPvt.pDOP
        gpy_frame_compressed.delta_time = delta_time; // ms
        gpy_frame_compressed.delta_Speed = delta_Speed; // mm/
        gpy_frame_compressed.delta_Speed_error = delta_Speed_error; // sAccCourse_Over_Ground;
        gpy_frame_compressed.delta_Latitude = delta_Latitude;
        gpy_frame_compressed.delta_Longitude = delta_Longitude;
        gpy_frame_compressed.delta_COG = delta_COG; // delta (course / 1000)!
        gpy_frame_compressed.Sat = ubxMessage.navPvt.numSV; // number of sats
        gpy_frame_compressed.fix = ubxMessage.navPvt.fixType;
        Fletcher16((uint8_t*)&gpy_frame_compressed, 20);
        file.write((const uint8_t *)&gpy_frame_compressed, 20);
    }   
}
