#include "Storage/sbp.h"
#include "Ublox/Ublox.h"

// -----------------------------------------------------------------------------
// SBP logging implementation
//
// This file implements logging of SBP (Simple Binary Protocol–style) data
// derived from u-blox NAV-PVT and NAV-DOP messages.
//
// It writes:
//  - A fixed SBP header once per file
//  - Repeated SBP frames containing time, position, velocity, and quality data
//
// Notes:
// - All data is sourced from the global `ubxMessage` structure
// - Files must already be opened by the caller
// - No file ownership or lifecycle management occurs here
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Global SBP structures
// -----------------------------------------------------------------------------
//
// These are written directly to disk as packed binary structures.
// They are declared globally so their previous values can be reused
// between frames if needed.

// SBP header (written once at the start of a file)
struct SBP_Header sbp_header = {
    30,        // Text_length: length of the identity string
    0xa0,      // Id1: protocol identifier byte 1
    0xa2,      // Id2: protocol identifier byte 2
    30,        // Again_length: repeated length field
    0xfd,      // Start: frame start marker
    "ESP-GPS,0,unknown,unknown"  // Identity string
};

// SBP frame (written repeatedly)
struct SBP_frame sbp_frame;

// -----------------------------------------------------------------------------
// log_header_SBP
//
// Writes the SBP header block to the file.
//
// Behaviour:
// - Pads unused bytes in the header with 0xFF
// - Writes a fixed 64-byte header block
//
// Parameters:
// - file : already-open File object for writing
// -----------------------------------------------------------------------------
void log_header_SBP(File file) {
    // Fill unused bytes in the header with 0xFF
    for (int i = 32; i < 64; i++) {
        ((unsigned char*)(&sbp_header))[i] = 0xFF;
    }

    // Write the complete header to the file
    file.write((const uint8_t *)&sbp_header, 64);
}

// -----------------------------------------------------------------------------
// log_SBP
//
// Writes a single SBP data frame to the file.
//
// Data sources:
// - u-blox NAV-PVT (position, speed, heading, accuracy)
// - u-blox NAV-DOP (HDOP)
//
// Unit conversions:
// - Speeds converted to cm/s
// - Heading converted to 0.01 degrees
// - Accuracies clamped to 8-bit ranges where required
//
// Parameters:
// - file : already-open File object for writing
// -----------------------------------------------------------------------------
void log_SBP(File file) {

    // -------------------------------------------------------------------------
    // Extract date and time fields from NAV-PVT
    // -------------------------------------------------------------------------
    uint32_t year  = ubxMessage.navPvt.year;
    uint8_t  month = ubxMessage.navPvt.month;
    uint8_t  day   = ubxMessage.navPvt.day;
    uint8_t  hour  = ubxMessage.navPvt.hour;
    uint16_t min   = ubxMessage.navPvt.min;
    uint16_t sec   = ubxMessage.navPvt.sec;

    // Satellite mask placeholder (filled later)
    uint32_t numSV = 0xFFFFFFFF;

    // -------------------------------------------------------------------------
    // Dilution of precision and accuracy handling
    // -------------------------------------------------------------------------

    // Convert HDOP from 0.01 resolution to ~0.2 resolution (8-bit)
    uint32_t HDOP = (ubxMessage.navDOP.hDOP + 1) / 20;
    if (HDOP > 255) HDOP = 255;

    // Speed accuracy (scaled and clamped)
    uint32_t sdop = ubxMessage.navPvt.sAcc / 10;
    if (sdop > 255) sdop = 255;

    // Vertical speed accuracy (scaled and clamped)
    uint32_t vsdop = ubxMessage.navPvt.vAcc / 10;
    if (vsdop > 255) vsdop = 255;

    // -------------------------------------------------------------------------
    // Populate SBP frame fields
    // -------------------------------------------------------------------------

    // UTC milliseconds within the current second (rounded)
    sbp_frame.UtcSec =
        ubxMessage.navPvt.sec * 1000 +
        (ubxMessage.navPvt.nano + 500000) / 1000000;

    // Pack date/time into a compact integer format
    sbp_frame.date_time_UTC_packed =
        (((year - 2000) * 12 + month) << 22) +
        (day  << 17) +
        (hour << 12) +
        (min  << 6) +
        sec;

    // Position (fixed-point, directly from u-blox)
    sbp_frame.Lat = ubxMessage.navPvt.lat;
    sbp_frame.Lon = ubxMessage.navPvt.lon;

    // Altitude above mean sea level (cm)
    sbp_frame.AltCM = ubxMessage.navPvt.hMSL / 10;

    // Speed over ground (cm/s)
    sbp_frame.Sog = ubxMessage.navPvt.gSpeed / 10;

    // Course over ground (0.01 degrees)
    sbp_frame.Cog = ubxMessage.navPvt.heading / 1000;

    // Satellite count and bitmask
    sbp_frame.SVIDCnt  = ubxMessage.navPvt.numSV;
    sbp_frame.SVIDList = numSV >> (32 - ubxMessage.navPvt.numSV);

    // Quality metrics
    sbp_frame.HDOP  = HDOP;
    sbp_frame.ClmbRte = -ubxMessage.navPvt.velD / 10; // Vertical speed (cm/s)
    sbp_frame.sdop  = sdop;
    sbp_frame.vsdop = vsdop;

    // -------------------------------------------------------------------------
    // Write frame to file
    // -------------------------------------------------------------------------
    file.write((const uint8_t *)&sbp_frame, 32);
}
