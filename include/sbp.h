#pragma once

#include <FS.h>  // Include necessary libraries for file handling

// Locosys SBP structures
struct SBP_Header {
    uint16_t Text_length;   // byte 0-1 : number of meaningful bytes in header (MID_FILE_ID)
    uint8_t Id1;            // seems to be necessary so that GP3S accept .sbp
    uint8_t Id2;            // seems to be necessary so that GP3S accept .sbp
    uint16_t Again_length;
    uint8_t Start;
    char Identity[57];      // byte 7~63: MID_FILE_ID(0xfd) will stuff 0xff for remaining bytes
} __attribute__((__packed__));

struct SBP_frame {
    uint8_t HDOP;               /* HDOP [0..51] with resolution 0.2 */
    uint8_t SVIDCnt;            /* Number of SVs in solution [0 to 12] */
    uint16_t UtcSec;            /* UTC Second [0 to 59] in seconds with resolution 0.001 */
    uint32_t date_time_UTC_packed; /* refer to protocol doc */
    uint32_t SVIDList;          /* SVs in solution: Bit 0=1: SV1, Bit 1=1: SV2, ... , Bit 31=1: SV32 */
    int32_t Lat;                /* Latitude [-90 to 90] in degrees with resolution 0.0000001 */
    int32_t Lon;                /* Longitude [-180 to 180] in degrees with resolution 0.0000001 */
    int32_t AltCM;              /* Altitude from Mean Sea Level in centimeters */
    uint16_t Sog;               /* Speed Over Ground in m/sec with resolution 0.01 */
    uint16_t Cog;               /* Course Over Ground [0 to 360] in degrees with resolution 0.01 */
    int16_t ClmbRte;            /* Climb rate in m/sec with resolution 0.01 */
    uint8_t sdop;               /* GT31 */
    uint8_t vsdop;
} __attribute__((__packed__));

// Extern declarations for SBP structures
extern struct SBP_Header sbp_header;
extern struct SBP_frame sbp_frame;

// Function declarations for logging SBP data
void log_header_SBP(File file);
void log_SBP(File file);
