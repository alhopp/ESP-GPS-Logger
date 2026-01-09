#include "Storage/gpy.h"
#include "Globals.h"          // Global firmware state (SW_version, mac, timezone, flags)
#include "Ublox/Ublox.h"      // u-blox NAV-PVT / NAV-DOP data (ubxMessage)

// -----------------------------------------------------------------------------
// GPY file state
// -----------------------------------------------------------------------------
// These structures hold the last-written GPY header, full frame, and compressed
// frame. They are reused and updated incrementally for delta encoding.

struct GPY_Header gpy_header = {};
struct GPY_Frame gpy_frame = {};
struct GPY_Frame_compressed gpy_frame_compressed = {};

// -----------------------------------------------------------------------------
// Fletcher16 checksum
//
// Calculates a Fletcher-16 checksum over a binary structure.
// The last two bytes of the buffer are assumed to be the checksum field
// and are filled in by this function.
// -----------------------------------------------------------------------------
uint16_t Fletcher16(uint8_t *data, int count) {
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;

    // Sum all bytes except the final 2 checksum bytes
    for (int i = 0; i < (count - 2); ++i) {
        sum1 = (sum1 + data[i]) & 0xFF;   // modulo 256
        sum2 = (sum2 + sum1) & 0xFF;
    }

    // Store checksum into the last two bytes
    data[count - 2] = sum1;
    data[count - 1] = sum2;

    return (sum2 << 8) | sum1;
}

// -----------------------------------------------------------------------------
// log_GPY_Header
//
// Writes the GPY file header to an already-open file.
// This includes firmware version, device serial number, and checksum.
// -----------------------------------------------------------------------------
void log_GPY_Header(File file) {

    // Copy firmware version string into header (fixed 16 bytes)
    for (int i = 0; i < 16; i++) {
        gpy_header.firmwareVersion[i] = SW_version[i];
    }

    // Encode device serial number from ESP32 MAC address
    sprintf(
        gpy_header.serialNumber,
        "_%2X%2X%2X%2X%2X%2X_",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );

    // Calculate and insert header checksum
    Fletcher16((uint8_t*)&gpy_header, 72);

    // Write header to file
    file.write((const uint8_t *)&gpy_header, 72);
}

// -----------------------------------------------------------------------------
// log_GPY
//
// Writes one GPY data frame to the file.
// Uses delta compression where possible to reduce file size.
// Falls back to a full frame if deltas exceed signed 16-bit limits.
// -----------------------------------------------------------------------------
void log_GPY(File file) {

    // --------------------------------------------------
    // Convert GPS date/time to Unix time (milliseconds)
    // --------------------------------------------------
    time_t utc_Sec;
    struct tm frame_time;

    frame_time.tm_sec  = ubxMessage.navPvt.second;
    frame_time.tm_hour = ubxMessage.navPvt.hour;
    frame_time.tm_min  = ubxMessage.navPvt.minute;
    frame_time.tm_mday = ubxMessage.navPvt.day;
    frame_time.tm_mon  = ubxMessage.navPvt.month - 1;     // tm_mon: 0–11
    frame_time.tm_year = ubxMessage.navPvt.year - 1900;   // tm_year: since 1900
    frame_time.tm_isdst = 0;

    // Convert to UTC seconds, compensating for local timezone offset
    utc_Sec = mktime(&frame_time) - _timezone;

    // Convert to milliseconds and add nanosecond rounding
    int64_t utc_ms =
        utc_Sec * 1000LL +
        (ubxMessage.navPvt.nano + 500000) / 1000000LL;

    // --------------------------------------------------
    // Calculate deltas against previous frame
    // --------------------------------------------------
    int delta_time        = utc_ms - gpy_frame.Unix_time;               // ms
    int delta_Speed       = ubxMessage.navPvt.gSpeed - gpy_frame.Speed; // mm/s
    int delta_Speed_error = ubxMessage.navPvt.sAcc - gpy_frame.Speed_error;
    int delta_Latitude    = ubxMessage.navPvt.lat - gpy_frame.Latitude;
    int delta_Longitude   = ubxMessage.navPvt.lon - gpy_frame.Longitude;
    int delta_COG         =
        ubxMessage.navPvt.heading / 1000 - gpy_frame.COG / 1000;

    // --------------------------------------------------
    // Decide whether a full frame is required
    // --------------------------------------------------
    #define SIGNED_INT 30000   // 16-bit signed safety margin

    int full_frame = 0;
    static int first_frame = 0;

    // If any delta exceeds int16 range → full frame
    if ((delta_time > SIGNED_INT) || (delta_time < -SIGNED_INT)) full_frame = 1;
    if ((delta_Speed > SIGNED_INT) || (delta_Speed < -SIGNED_INT)) full_frame = 1;
    if ((delta_Speed_error > SIGNED_INT) || (delta_Speed_error < -SIGNED_INT)) full_frame = 1;
    if ((delta_Latitude > SIGNED_INT) || (delta_Latitude < -SIGNED_INT)) full_frame = 1;
    if ((delta_Longitude > SIGNED_INT) || (delta_Longitude < -SIGNED_INT)) full_frame = 1;
    if ((delta_COG > SIGNED_INT) || (delta_COG < -SIGNED_INT)) full_frame = 1;

    // First frame must always be a full frame
    if (first_frame == 0) full_frame = 1;

    // Force full frame if previous NAV-PVT frame was missed
    if (next_gpy_full_frame) {
        full_frame = 1;
        next_gpy_full_frame = 0;
    }

    // --------------------------------------------------
    // Write full GPY frame
    // --------------------------------------------------
    if (full_frame == 1) {

        gpy_frame.HDOP        = ubxMessage.navDOP.hDOP;
        gpy_frame.Unix_time  = utc_ms;
        gpy_frame.Speed      = ubxMessage.navPvt.gSpeed;
        gpy_frame.Speed_error= ubxMessage.navPvt.sAcc;
        gpy_frame.Latitude   = ubxMessage.navPvt.lat;
        gpy_frame.Longitude  = ubxMessage.navPvt.lon;
        gpy_frame.COG        = ubxMessage.navPvt.heading;
        gpy_frame.Sat        = ubxMessage.navPvt.numSV;
        gpy_frame.fix        = ubxMessage.navPvt.fixType;

        // Calculate checksum and write full frame
        Fletcher16((uint8_t*)&gpy_frame, 36);
        file.write((const uint8_t*)&gpy_frame, 36);

        first_frame = 1;

    // --------------------------------------------------
    // Write compressed GPY delta frame
    // --------------------------------------------------
    } else {

        gpy_frame_compressed.HDOP               = ubxMessage.navDOP.hDOP;
        gpy_frame_compressed.delta_time         = delta_time;
        gpy_frame_compressed.delta_Speed        = delta_Speed;
        gpy_frame_compressed.delta_Speed_error  = delta_Speed_error;
        gpy_frame_compressed.delta_Latitude     = delta_Latitude;
        gpy_frame_compressed.delta_Longitude    = delta_Longitude;
        gpy_frame_compressed.delta_COG          = delta_COG;
        gpy_frame_compressed.Sat                = ubxMessage.navPvt.numSV;
        gpy_frame_compressed.fix                = ubxMessage.navPvt.fixType;

        // Calculate checksum and write compressed frame
        Fletcher16((uint8_t*)&gpy_frame_compressed, 20);
        file.write((const uint8_t *)&gpy_frame_compressed, 20);
    }
}
