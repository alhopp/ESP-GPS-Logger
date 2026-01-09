#pragma once

#include <FS.h>

// -----------------------------------------------------------------------------
// SBP (Simple Binary Protocol–style) structures and logging API
//
// This header defines the binary layout used for SBP logging and declares
// the functions used to write SBP data to an already-open file.
//
// Notes:
// - Structures are written directly to disk (binary, packed layout)
// - Data is populated from the global u-blox NAV-PVT / NAV-DOP state
// - File open/close lifecycle is handled elsewhere
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// SBP Header structure (written once per file)
// -----------------------------------------------------------------------------
struct SBP_Header {
    uint8_t  Text_length;      // Length of identity string
    uint8_t  Id1;              // Protocol identifier byte 1 (0xA0)
    uint8_t  Id2;              // Protocol identifier byte 2 (0xA2)
    uint8_t  Again_length;     // Repeated length field
    uint8_t  Start;            // Frame start marker (0xFD)
    char     Identity[57];     // Device identity string
} __attribute__((__packed__));

// -----------------------------------------------------------------------------
// SBP Data Frame structure (written repeatedly)
// -----------------------------------------------------------------------------
struct SBP_frame {
    uint32_t UtcSec;                  // Milliseconds within current second
    uint32_t date_time_UTC_packed;    // Packed date/time representation
    int32_t  Lat;                     // Latitude (u-blox fixed-point)
    int32_t  Lon;                     // Longitude (u-blox fixed-point)
    int32_t  AltCM;                   // Altitude above MSL (cm)
    uint32_t Sog;                     // Speed over ground (cm/s)
    uint32_t Cog;                     // Course over ground (0.01 deg)
    uint8_t  SVIDCnt;                 // Number of satellites
    uint32_t SVIDList;                // Satellite bitmask
    uint8_t  HDOP;                    // Horizontal dilution of precision
    int16_t  ClmbRte;                 // Vertical speed (cm/s)
    uint8_t  sdop;                    // Speed accuracy (scaled)
    uint8_t  vsdop;                   // Vertical speed accuracy (scaled)
} __attribute__((__packed__));

// -----------------------------------------------------------------------------
// Global SBP instances (defined in sbp.cpp)
// -----------------------------------------------------------------------------
extern struct SBP_Header sbp_header;
extern struct SBP_frame  sbp_frame;

// -----------------------------------------------------------------------------
// Public logging API
// -----------------------------------------------------------------------------

// Write SBP header block to file (must be called once per file)
void log_header_SBP(File file);

// Write one SBP data frame to file
void log_SBP(File file);
