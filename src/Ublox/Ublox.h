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
#include "Core/board_pins.h"
#include "Ublox/Ublox_ubx.h"

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
  uint8_t month,day,hour,min,sec,valid;
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
// Read by UI, SD logging, stats, SBP writers.
// ============================================================================
struct UBXMessage {
  NAV_PVT     navPvt;
  NAV_DOP     navDOP;
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


