#pragma once
// u-blox M10 UBX interface (Level 2 refactor)
// Safe, grouped PROGMEM commands + binary-exact message structs

#include <Arduino.h>
#include <sys/time.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include "SD_card.h"

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
inline const char* gpsChip(int longname) {
  return longname ? "u-blox M10" : "M10";
}

// --------------------------------------------------
// Single GPS UART (UART2)
// --------------------------------------------------
extern HardwareSerial UbloxSerial;

// --------------------------------------------------
// Initialisation helper
// --------------------------------------------------
void ubloxSerialInit(int delay_ms);

// --------------------------------------------------
// UBX sender helper
// --------------------------------------------------
template <size_t N>
inline void sendUbx(const uint8_t (&cmd)[N])
{
  for (size_t i = 0; i < N; i++) {
    UbloxSerial.write(pgm_read_byte(cmd + i));
  }
  UbloxSerial.flush();
}

// -----------------------------------------------------------------------------
// UBX command blobs (grouped by intent)
// -----------------------------------------------------------------------------
namespace ubx {

// ========================== CONFIG ===========================================
namespace cfg {


  // CFG-PRT: UART1 @ 38400, UBX output enabled
  constexpr uint8_t uart1_ubx_out[] PROGMEM = {
    0xB5,0x62,
    0x06,0x00,        // CFG-PRT
    0x14,0x00,        // length
    0x01,             // portID = UART1
    0x00,             // reserved
    0x00,0x00,        // txReady
    0xD0,0x08,0x00,0x00, // mode (8N1)
    0x00,0x96,0x00,0x00, // baudrate = 38400
    0x01,0x00,        // inProtoMask  = UBX
    0x01,0x00,        // outProtoMask = UBX   ⭐ IMPORTANT
    0x00,0x00,        // flags
    0x00,0x00,        // reserved
    0x00,0x00         // reserved
};

// Protocol
constexpr uint8_t nmea_off[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x02,0x00,0x74,0x10,0x00,0x21,0xC0};

constexpr uint8_t ubx_only[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x74,0x10,0x01,0x21,0xBC};

// GNSS constellation
constexpr uint8_t beidou_b1c_on[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x0F,0x00,0x31,0x10,0x01,0xEC,0x39};

constexpr uint8_t gal_on[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x21,0x00,0x31,0x10,0x01,0xFE,0x93};

constexpr uint8_t glonass_on[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x25,0x00,0x31,0x10,0x01,0x02,0xA7};

// GPS + Galileo + BeiDou(B1C) + GLONASS
constexpr uint8_t all_4gnss[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x13,0x00,0x01,0x01,0x00,0x00,0x0D,0x00,0x31,0x10,0x00,0x0F,0x00,0x31,0x10,0x01,0x25,0x00,0x31,0x10,0x01,0xAB,0x1B};

// Dynamic model
constexpr uint8_t sea_model[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x21,0x00,0x11,0x20,0x05,0xF2,0x57};

} // namespace cfg

// ========================== MESSAGE ENABLES ===================================
namespace msg {

constexpr uint8_t nav_pvt[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x07,0x00,0x91,0x20,0x01,0x54,0x51};

constexpr uint8_t nav_dop[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x39,0x00,0x91,0x20,0x01,0x86,0x4B};

constexpr uint8_t nav_sat[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x16,0x00,0x91,0x20,0x0A,0x6C,0xA5};

} // namespace msg

// ========================== RATE / BAUD =======================================
namespace rate {

constexpr uint8_t baud_19200[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x0C,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x52,0x40,0x00,0x4B,0x00,0x00,0x7C,0x4A};

constexpr uint8_t baud_38400[] PROGMEM =
{0xB5,0x62,0x06,0x8A,0x0C,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x52,0x40,0x00,0x96,0x00,0x00,0xC7,0x2B};

// 1,2,4,5,8,10,15,20 Hz (18 bytes each)
constexpr uint8_t table[] PROGMEM = {
  0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0xE8,0x03,0xD9,0xCE,
  0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0xF4,0x01,0xE3,0xE4,
  0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0xFA,0x00,0xE8,0xEF,
  0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0xC8,0x00,0xB6,0x8B,
  0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0x7D,0x00,0x6B,0xF5,
  0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0x64,0x00,0x52,0xC3,
  0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0x43,0x00,0x31,0x81,
  0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0x32,0x00,0x20,0x5F
};

} // namespace rate

// ========================== HIGH NAV RATE =====================================
namespace highnav {

// Poll current "high nav" settings (your original blob)
constexpr uint8_t get_nav_rate[] PROGMEM =
{0xB5,0x62,0x06,0x8B,0x14,0x00,0x00,0x04,0x00,0x00,0x01,0x00,0xA4,0x40,0x03,0x00,0xA4,0x40,
 0x05,0x00,0xA4,0x40,0x0A,0x00,0xA4,0x40,0x4C,0x15};

// Apply high nav rate settings (your original blob)
constexpr uint8_t set_high_nav_rate[] PROGMEM =
{0xB5,0x62,0x06,0x41,0x10,0x00,0x03,0x00,0x04,0x1F,0x54,0x5E,0x79,0xBF,0x28,0xEF,0x12,0x05,
 0xFD,0xFF,0xFF,0xFF,0x8F,0x0D,
 0xB5,0x62,0x06,0x41,0x1C,0x00,0x04,0x01,0xA4,0x10,0xBD,0x34,0xF9,0x12,0x28,0xEF,0x12,
 0x05,0x05,0x00,0xA4,0x40,0x00,0xB0,0x71,0x0B,0x0A,0x00,0xA4,0x40,0x00,0xD8,0xB8,0x05,0xDE,0xAE};

} // namespace highnav

// ========================== POLLS =============================================
namespace poll {

constexpr uint8_t mon_gnss[] PROGMEM = {0xB5,0x62,0x0A,0x28,0x00,0x00,0x32,0xA0};
constexpr uint8_t mon_ver[]  PROGMEM = {0xB5,0x62,0x0A,0x04,0x00,0x00,0x0E,0x34};
constexpr uint8_t nav_sat[]  PROGMEM = {0xB5,0x62,0x01,0x35,0x00,0x00,0x36,0xA3};
constexpr uint8_t uid[]      PROGMEM = {0xB5,0x62,0x27,0x03,0x00,0x00,0x2A,0xA5};

} // namespace poll

} // namespace ubx

// -----------------------------------------------------------------------------
// UBX headers (CLASS, ID)
// -----------------------------------------------------------------------------
constexpr uint8_t UBX_HEADER[]      = {0xB5,0x62};
constexpr uint8_t NAV_PVT_HEADER[]  = {0x01,0x07};
constexpr uint8_t NAV_DOP_HEADER[]  = {0x01,0x04};
constexpr uint8_t NAV_SAT_HEADER[]  = {0x01,0x35};
constexpr uint8_t NAV_ACK_HEADER[]  = {0x05,0x01};
constexpr uint8_t NAV_NACK_HEADER[] = {0x05,0x00};
constexpr uint8_t NAV_ID_HEADER[]   = {0x27,0x03};
constexpr uint8_t MON_GNSS_HEADER[] = {0x0A,0x28};
constexpr uint8_t MON_VER_HEADER[]  = {0x0A,0x04};

// -----------------------------------------------------------------------------
// Message types
// -----------------------------------------------------------------------------
enum _ubxMsgType { MT_NONE, MT_NAV_DUMMY, MT_NAV_PVT, MT_NAV_ACK, MT_NAV_NACK, MT_NAV_ID, MT_MON_GNSS, MT_NAV_DOP, MT_MON_VER, MT_NAV_SAT };

// -----------------------------------------------------------------------------
// UBX message structs (binary-exact)
// -----------------------------------------------------------------------------
struct NAV_DUMMY { uint8_t cls,id; uint16_t len; uint8_t msg_cls,msg_id,chkA,chkB; } __attribute__((packed));
struct NAV_ACK   { uint8_t cls,id; uint16_t len; uint8_t msg_cls,msg_id,chkA,chkB; } __attribute__((packed));
struct NAV_NACK  { uint8_t cls,id; uint16_t len; uint8_t msg_cls,msg_id,chkA,chkB; } __attribute__((packed));

struct NAV_ID {
  uint8_t cls,id; uint16_t len;
  uint8_t Version,r1,r2,r3,ubx_id_1,ubx_id_2,ubx_id_3,ubx_id_4,ubx_id_5,ubx_id_6;
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
  uint8_t cls,id; uint16_t len; uint32_t iTOW;
  uint16_t gDOP,pDOP,tDOP,vDOP,hDOP,nDOP,eDOP;
} __attribute__((packed));

struct NAV_PVT {
  uint32_t iTOW;        // 0
  uint16_t year;        // 4
  uint8_t  month;       // 6
  uint8_t  day;         // 7
  uint8_t  hour;        // 8
  uint8_t  minute;         // 9
  uint8_t  second;         // 10
  uint8_t  valid;       // 11

  uint32_t tAcc;        // 12
  int32_t  nano;        // 16

  uint8_t  fixType;     // 20
  uint8_t  flags;       // 21
  uint8_t  flags2;      // 22
  uint8_t  numSV;       // 23

  int32_t  lon;         // 24
  int32_t  lat;         // 28
  int32_t  height;      // 32
  int32_t  hMSL;        // 36

  uint32_t hAcc;        // 40
  uint32_t vAcc;        // 44

  int32_t  velN;        // 48
  int32_t  velE;        // 52
  int32_t  velD;        // 56
  int32_t  gSpeed;      // 60
  int32_t  heading;     // 64

  uint32_t sAcc;        // 68
  uint32_t headAcc;    // 72
  uint16_t pDOP;       // 76

  uint8_t  reserved1[6]; // 78–83

  int32_t  headVeh;    // 84
  int16_t  magDec;     // 88
  uint16_t magAcc;    // 90
} __attribute__((packed));


// NAV-SAT
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

// -----------------------------------------------------------------------------
// Container
// -----------------------------------------------------------------------------
struct UBXMessage {
  NAV_DUMMY navDummy;
  NAV_PVT   navPvt;
  NAV_DOP   navDOP;
  NAV_ACK   navAck;
  NAV_NACK  navNack;
  NAV_ID    ubxId;
  MON_GNSS  monGNSS;
  MON_VER   monVER;
  NAV_SAT_HDR navSatHdr;
  sVs_NAV_SAT navSat[UBX_MAX_SVS];
  uint8_t navSatCount;
  _ubxMsgType lastMsgType;
};

extern UBXMessage ubxMessage;
extern bool sdOK;
extern char dataStr[255];
extern char Buffer[50];

static_assert(sizeof(NAV_PVT) == 92, "NAV_PVT size mismatch");

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------
void calcChecksum(uint8_t* CK,int msgType,int msgSize);
bool compareMsgHeader(const uint8_t* msgHeader);

void Init_ubloxM10(void);
void Set_rate_ubloxM10(int rate);
bool Set_GPS_Time(float time_offset);
int  processGPS(void);
int  Check_M10_nav_rate(void);
int  Set_M10_high_nav_rate(void);

