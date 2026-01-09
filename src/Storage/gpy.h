#pragma once

#include <FS.h>

// -----------------------------------------------------------------------------
// GPY binary log format
//
// This header defines the on-disk binary layout for GPY files and the public
// logging API implemented in gpy.cpp.
//
// Design notes:
// - Structures are packed and written directly to disk
// - State is held in global frame instances (delta encoding depends on history)
// - Callers only provide the File handle; all GPS data comes from ubxMessage
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// GPY file header (written once at file creation)
// -----------------------------------------------------------------------------
struct GPY_Header {
    uint8_t  Type_identifier;   // 0xF0 = GPY header
    uint8_t  Flags;
    uint16_t Length;            // Fixed length (72 bytes)
    uint16_t DeviceType;        // e.g. u-blox = 2

    char deviceDescription[16];
    char deviceName[16];
    char serialNumber[16];
    char firmwareVersion[16];

    uint16_t Checksum;          // Fletcher16
} __attribute__((__packed__));

// -----------------------------------------------------------------------------
// GPY full frame (absolute values)
// Written when deltas exceed int16 range or on first frame
// -----------------------------------------------------------------------------
struct GPY_Frame {
    uint8_t  Type_identifier;   // 0xE0 = full frame
    uint8_t  Flags;
    uint16_t HDOP;

    int64_t  Unix_time;         // milliseconds UTC
    uint32_t Speed;             // mm/s
    uint32_t Speed_error;       // sAcc
    int32_t  Latitude;          // 1e-7 degrees
    int32_t  Longitude;         // 1e-7 degrees
    int32_t  COG;               // heading (1e-5 degrees)

    uint8_t  Sat;               // satellites used
    uint8_t  fix;               // fix type

    uint16_t Checksum;          // Fletcher16
} __attribute__((__packed__));  // 36 bytes

// -----------------------------------------------------------------------------
// GPY compressed frame (delta encoded)
// Written when all deltas fit into int16
// -----------------------------------------------------------------------------
struct GPY_Frame_compressed {
    uint8_t  Type_identifier;   // 0xD0 = compressed frame
    uint8_t  Flags;
    uint16_t HDOP;

    int16_t  delta_time;        // ms
    int16_t  delta_Speed;       // mm/s
    int16_t  delta_Speed_error;
    int16_t  delta_Latitude;
    int16_t  delta_Longitude;
    int16_t  delta_COG;

    uint8_t  Sat;
    uint8_t  fix;

    uint16_t Checksum;          // Fletcher16
} __attribute__((__packed__));  // 20 bytes

// -----------------------------------------------------------------------------
// Global GPY frame state
// (defined in gpy.cpp; required for delta encoding)
// -----------------------------------------------------------------------------
extern GPY_Header            gpy_header;
extern GPY_Frame             gpy_frame;
extern GPY_Frame_compressed  gpy_frame_compressed;

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

// Calculate Fletcher-16 checksum and write it into the last 2 bytes
uint16_t Fletcher16(uint8_t *data, int count);

// Write GPY file header (call once after opening file)
void log_GPY_Header(File file);

// Write one GPY data frame (full or compressed, chosen automatically)
void log_GPY(File file);
