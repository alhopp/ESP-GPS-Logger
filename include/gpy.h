#pragma once

#include <FS.h>
#include "Globals.h"

// Declare the structure of GPY headers and frames
struct GPY_Header {
    uint8_t Type_identifier;  // Frame identifier, header = 0xF0
    uint8_t Flags;
    uint16_t Length;  // length = 6 + 4 * STRING_IO_LENGTH + 2 = 72
    uint16_t DeviceType;  // ublox = 2
    char deviceDescription[16];
    char deviceName[16];
    char serialNumber[16];
    char firmwareVersion[16];
    uint16_t Checksum;
} __attribute__((__packed__));

struct GPY_Frame {
    uint8_t Type_identifier;  // Frame identifier for full frame = 0xE0
    uint8_t Flags;
    uint16_t HDOP;  // HDOP
    int64_t Unix_time; // ms
    uint32_t Speed; // mm/s
    uint32_t Speed_error; // sAccCourse_Over_Ground;
    int32_t Latitude;
    int32_t Longitude;
    int32_t COG; // Course over ground
    uint8_t Sat; // number of sats
    uint8_t fix;
    uint16_t Checksum;
} __attribute__((__packed__)); // total = 36 bytes

struct GPY_Frame_compressed {
    uint8_t Type_identifier;  // Frame identifier for compressed frame = 0xD0
    uint8_t Flags;
    uint16_t HDOP;  // HDOP
    int16_t delta_time; // ms
    int16_t delta_Speed; // mm/
    int16_t delta_Speed_error; // sAccCourse_Over_Ground;
    int16_t delta_Latitude;
    int16_t delta_Longitude;
    int16_t delta_COG; // delta (course / 1000)!
    uint8_t Sat; // number of sats
    uint8_t fix;
    uint16_t Checksum;
}; // total = 20 bytes

extern struct GPY_Header gpy_header;
extern struct GPY_Frame gpy_frame;
extern struct GPY_Frame_compressed gpy_frame_compressed;

// Function declarations
void log_GPY_Header(File file);
void log_GPY(File file);
uint16_t Fletcher16(uint8_t *data, int count);


