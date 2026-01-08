#pragma once
// ============================================================================
// Ublox.h — Legacy-compatible u-blox M10 interface
//
// This header defines:
//   • Raw UBX configuration blobs (CFG-VALSET, CFG-RATE, etc.)
//   • Binary-exact UBX message structs (NAV-PVT, NAV-DOP, MON-VER, …)
//   • A single shared UBXMessage container used by Ublox.cpp + loggers
//
// DESIGN GOALS
//   ✔ Zero STL usage inside namespace ubx (prevents std corruption)
//   ✔ Binary-exact structs for memcpy-style parsing
//
// IMPORTANT
//   • This file intentionally mixes *modern M10 CFG-VALSET* blobs with
//     *legacy CFG-RATE* messages because Ublox.cpp expects both.
//   • Do NOT “simplify” unless Ublox.cpp is refactored at the same time.
// ============================================================================

#include <Arduino.h>
#include <sys/time.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include "SD_card.h"

// ============================================================================
// HELPERS
// ============================================================================

// Human-readable chip name for UI / logs
inline const char* gpsChip(int longname) {
  return longname ? "u-blox M10" : "M10";
}

// ============================================================================
// UART
// ============================================================================
//
// UbloxSerial is the *only* GPS transport used in the system.
// It is initialised externally (typically UART2 on ESP32).
//
extern HardwareSerial UbloxSerial;

// Bring up GPS serial with a short delay to allow module power-up
void ubloxSerialInit(int delay_ms);

// ============================================================================
// UBX SEND HELPER
// ============================================================================
//
// Sends a PROGMEM-stored UBX message byte-by-byte.
// Used for *all* config, poll, and rate commands.
//
template <size_t N>
inline void sendUbx(const uint8_t (&cmd)[N]) {
  for (size_t i = 0; i < N; i++) {
    UbloxSerial.write(pgm_read_byte(cmd + i));
  }
  UbloxSerial.flush(); // ensure command fully transmitted
}

// ============================================================================
// RAW UBX BLOBS
//
// These are **pre-computed binary UBX packets** with correct checksums.
// They are grouped by intent and sent verbatim to the GPS.
//
// NOTE: These are *not* parsed at runtime — they are write-only.
// ============================================================================
namespace ubx {

// ============================================================================
// CONFIG (CFG-VALSET)
// ============================================================================
namespace cfg {

// Enable all major GNSS constellations simultaneously:
//
//   • GPS
//   • GLONASS
//   • GALILEO
//   • BEIDOU
//
// Improves fix robustness and satellite availability.
// Used once during Init_ubloxM10().
//
constexpr uint8_t all_4gnss[] PROGMEM = {0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x3F,0x00,0x31,0x10,0x01,0x3F,0xFF};

// Disable all NMEA output
// (prevents ASCII noise on the serial line)
constexpr uint8_t nmea_off[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x02,0x00,0x74,0x10,0x00,0x21,0xC0};

// Enable UBX protocol only (binary messages)
// Required for high-rate logging and deterministic parsing
constexpr uint8_t ubx_only[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x74,0x10,0x01,0x21,0xBC};

// Set dynamic platform model = SEA
// Optimises navigation filter for marine / windsurfing use
constexpr uint8_t sea_model[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x21,0x00,0x11,0x20,0x05,0xF2,0x57};

}

// ============================================================================
// MESSAGE ENABLE (CFG-VALSET)
// ============================================================================
namespace msg {

// Enable NAV-PVT
//   • Position, velocity, time
//   • Primary navigation message used everywhere
//   • Includes Doppler-derived velocity:
//      gSpeed (ground speed, cm/s)
//      heading (course over ground)
constexpr uint8_t nav_pvt[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x07,0x00,0x91,0x20,0x01,0x54,0x51};

// Enable NAV-DOP
//   • Dilution of precision (HDOP, PDOP, etc.)
//   • Used for quality metrics and logging
constexpr uint8_t nav_dop[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x39,0x00,0x91,0x20,0x01,0x86,0x4B};

// Enable NAV-SAT
//   • Per-satellite SNR, elevation, azimuth
//   • Used for satellite bars / diagnostics
constexpr uint8_t nav_sat[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x16,0x00,0x91,0x20,0x0A,0x6C,0xA5};

}

// ============================================================================
// RATE / BAUD (mixed legacy + modern)
// ============================================================================
namespace rate {

// Change UART1 baud rate to 38400
//
// This is sent early during init, followed by a serial re-sync.
// Uses CFG-VALSET (M10-style).
//
constexpr uint8_t baud_38400[] PROGMEM = {
  0xB5,0x62,0x06,0x8A,0x0C,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x52,0x40,0x00,0x96,0x00,0x00,0xC7,0x2B};

// -----------------------------------------------------------------------------
// Fixed navigation rate: 5 Hz
//
// -----------------------------------------------------------------------------
constexpr uint8_t rate_5hz[] PROGMEM = {
  0xB5,0x62,0x06,0x08,0x06,0x00,0xC8,0x00,0x01,0x00,0x01,0x00,0xDE,0x6A};
}

// ============================================================================
// POLLS (request-response messages)
// ============================================================================
namespace poll {

// Query GNSS configuration
constexpr uint8_t mon_gnss[] PROGMEM = {0xB5,0x62,0x0A,0x28,0x00,0x00,0x32,0xA0};

// Query firmware / hardware version
constexpr uint8_t mon_ver[]  PROGMEM = {0xB5,0x62,0x0A,0x04,0x00,0x00,0x0E,0x34};

// Query unique chip ID
constexpr uint8_t uid[]      PROGMEM = {0xB5,0x62,0x27,0x03,0x00,0x00,0x2A,0xA5};

}

} // namespace ubx

// ============================================================================
// MESSAGE TYPE ENUM
// ============================================================================
enum _ubxMsgType {
  MT_NONE,
  MT_NAV_PVT,
  MT_NAV_DOP,
  MT_NAV_SAT,
  MT_NAV_ACK,
  MT_NAV_NACK,
  MT_MON_GNSS,
  MT_MON_VER,
  MT_NAV_ID
};

// ============================================================================
// BINARY MESSAGE STRUCTS
//
// These structs mirror the *on-wire UBX payload layout exactly*.
// They are written into via byte offsets in processGPS().
// ============================================================================
struct NAV_ACK  { uint8_t cls,id; uint16_t len; uint8_t msg_cls,msg_id,chkA,chkB; } __attribute__((packed));
struct NAV_NACK { uint8_t cls,id; uint16_t len; uint8_t msg_cls,msg_id,chkA,chkB; } __attribute__((packed));

struct NAV_ID {
  uint8_t cls,id; uint16_t len;
  uint8_t Version,r1,r2,r3;
  uint8_t ubx_id_1,ubx_id_2,ubx_id_3,ubx_id_4,ubx_id_5,ubx_id_6;
} __attribute__((packed));

struct MON_GNSS {
  uint8_t cls,id; uint16_t len;
  uint8_t Version,supported_Gnss,default_Gnss,enabled_Gnss,simultaneous,r1,r2,r3;
} __attribute__((packed));

struct VER_EXT { char extension[30]; } __attribute__((packed));

struct MON_VER {
  uint8_t cls,id; uint16_t len;
  char swVersion[30];
  char hwVersion[10];
  VER_EXT ext[6];
} __attribute__((packed));

struct NAV_DOP {
  uint8_t cls,id; uint16_t len;
  uint32_t iTOW;
  uint16_t gDOP,pDOP,tDOP,vDOP,hDOP,nDOP,eDOP;
} __attribute__((packed));

struct NAV_PVT {
  uint32_t iTOW;
  uint16_t year;
  uint8_t month,day,hour,minute,second,valid;
  uint32_t tAcc;
  int32_t nano;
  uint8_t fixType,flags,flags2,numSV;
  int32_t lon,lat,height,hMSL;
  uint32_t hAcc,vAcc;
  int32_t velN,velE,velD,gSpeed,heading;
  uint32_t sAcc,headAcc;
  uint16_t pDOP;
  uint8_t reserved1[6];
  int32_t headVeh;
  int16_t magDec;
  uint16_t magAcc;
} __attribute__((packed));

// ============================================================================
// NAV-SAT (per-satellite info)
// ============================================================================
constexpr uint8_t UBX_MAX_SVS = 64;

struct sVs_NAV_SAT {
  uint8_t gnssId,svId,cno;
  int8_t elev;
  int16_t azim,prRes;
  uint32_t flags;
} __attribute__((packed));

struct NAV_SAT_HDR {
  uint8_t cls,id; uint16_t len;
  uint32_t iTOW;
  uint8_t version,numSvs,r1,r2;
} __attribute__((packed));

// ============================================================================
// CENTRAL MESSAGE CONTAINER
//
// All parsed UBX messages land here.
// Read by UI, SD logging, stats, GPX/GPY/SBP writers.
// ============================================================================
struct UBXMessage {
  NAV_PVT     navPvt;
  NAV_DOP     navDOP;
  NAV_ACK     navAck;
  NAV_NACK    navNack;
  NAV_ID      ubxId;
  MON_GNSS    monGNSS;
  MON_VER     monVER;
  NAV_SAT_HDR navSatHdr;
  sVs_NAV_SAT navSat[UBX_MAX_SVS];
  uint8_t     navSatCount;
  _ubxMsgType lastMsgType;
};

extern UBXMessage ubxMessage;

// ============================================================================
// PUBLIC API
// ============================================================================
void Init_ubloxM10(void);               // Full GPS initialisation sequence
bool Set_GPS_Time(float time_offset);
int  processGPS(void);                  // UBX byte-stream parser


