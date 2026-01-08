#include "sbp.h"
#include "Ublox/Ublox.h"

// Define the SBP Header and SBP Frame as global variables
struct SBP_Header sbp_header = {
    30,        // Text_length
    0xa0,      // Id1
    0xa2,      // Id2
    30,        // Again_length
    0xfd,      // Start
    "ESP-GPS,0,unknown,unknown"  // Identity
};

struct SBP_frame sbp_frame;

// Function definition: Logging SBP header
void log_header_SBP(File file) {
    for (int i = 32; i < 64; i++) {
        ((unsigned char*)(&sbp_header))[i] = 0xFF; // Fill unused bytes with 0xFF
    }
    file.write((const uint8_t *)&sbp_header, 64);
}

// Function definition: Logging SBP frame
void log_SBP(File file) {
    uint32_t year = ubxMessage.navPvt.year;
    uint8_t month = ubxMessage.navPvt.month;
    uint8_t day = ubxMessage.navPvt.day;
    uint8_t hour = ubxMessage.navPvt.hour;
    uint16_t min = ubxMessage.navPvt.minute;
    uint16_t sec = ubxMessage.navPvt.second;
    uint32_t numSV = 0xFFFFFFFF;
    uint32_t HDOP = (ubxMessage.navDOP.hDOP + 1) / 20; // From 0.01 resolution to 0.2, reformat pDOP to HDOP 8-bit !!
    if (HDOP > 255) HDOP = 255; // Ensure it fits in 8 bits
    uint32_t sdop = ubxMessage.navPvt.sAcc / 10; // Was sAcc
    if (sdop > 255) sdop = 255;
    uint32_t vsdop = ubxMessage.navPvt.vAcc / 10; // Was headingAcc ???
    if (vsdop > 255) vsdop = 255;

    sbp_frame.UtcSec = ubxMessage.navPvt.second * 1000 + (ubxMessage.navPvt.nano + 500000) / 1000000; // Round off
    sbp_frame.date_time_UTC_packed = (((year - 2000) * 12 + month) << 22) + (day << 17) + (hour << 12) + (min << 6) + sec;
    sbp_frame.Lat = ubxMessage.navPvt.lat;
    sbp_frame.Lon = ubxMessage.navPvt.lon;
    sbp_frame.AltCM = ubxMessage.navPvt.hMSL / 10; // Convert to cm/s
    sbp_frame.Sog = ubxMessage.navPvt.gSpeed / 10; // Convert to cm/s
    sbp_frame.Cog = ubxMessage.navPvt.heading / 1000; // Convert to 0.01 degrees
    sbp_frame.SVIDCnt = ubxMessage.navPvt.numSV;
    sbp_frame.SVIDList = numSV >> (32 - ubxMessage.navPvt.numSV);
    sbp_frame.HDOP = HDOP;
    sbp_frame.ClmbRte = -ubxMessage.navPvt.velD / 10; // Convert to cm/s
    sbp_frame.sdop = sdop;
    sbp_frame.vsdop = vsdop;

    file.write((const uint8_t *)&sbp_frame, 32);
}
