#pragma once

//https://github.com/iforce2d/inavFollowme/blob/master/FollowMeTag/GPS.h
//https://content.u-blox.com/sites/default/files/MAX-M10S_IntegrationManual_UBX-20053088.pdf  M10 high performance mode

#include "Arduino.h"
#include "sys/time.h"
#include "SD_card.h"
#include <driver/rtc_io.h>
#include <driver/gpio.h>

inline const char* gpsChip(int longname) {
  return longname ? "u-blox M10" : "M10";
}

const char UBLOX_M10_BEIDOU_OFF[] PROGMEM = {
0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x22,0x00,0x31,0x10,0x00,0xFE,0x97
};
const char UBLOX_M10_BEIDOU_B1_OFF[] PROGMEM = {
0XB5,0X62,0X06,0X8A,0X09,0X00,0X01,0X01,0X00,0X00,0X0D,0X00,0X31,0X10,0X00,0XE9,0X2E
};
const char UBLOX_M10_BEIDOU_B1C_ON[] PROGMEM = {
0XB5,0X62,0X06,0X8A,0X09,0X00,0X01,0X01,0X00,0X00,0X0F,0X00,0X31,0X10,0X01,0XEC,0X39
};
const char UBLOX_M10_GAL_ON[] PROGMEM = {
0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x21,0x00,0x31,0x10,0x01,0xFE,0x93
};
const char UBLOX_M10_GAL_OFF[] PROGMEM = {
0XB5,0X62,0X06,0X8A,0X09,0X00,0X01,0X01,0X00,0X00,0X21,0X00,0X31,0X10,0X00,0XFD,0X92
};
const char UBLOX_M10_GLONAS_ON[] PROGMEM = {//GLONAS  enable
0XB5,0X62,0X06,0X8A,0X09,0X00,0X01,0X01,0X00,0X00,0X25,0X00,0X31,0X10,0X01,0X02,0XA7
};
const char UBLOX_M10_GLONAS_OFF[] PROGMEM = {//GLONAS disable
0XB5,0X62,0X06,0X8A,0X09,0X00,0X01,0X01,0X00,0X00,0X25,0X00,0X31,0X10,0X00,0X01,0XA6
};
const char UBLOX_M10_4GNSS[] PROGMEM = {//GPS+GAL+BEIDOU_B1C+GLONAS
0XB5,0X62,0X06,0X8A,0X13,0X00,0X01,0X01,0X00,0X00,0X0D,0X00,0X31,0X10,0X00,0X0F,0X00,0X31,0X10,0X01,0X25,0X00,0X31,0X10,0X01,0XAB,0X1B
};
const char UBLOX_M10_NMEA_OFF[] PROGMEM = {  //NMEA OFF, generated with ucenter2 !!
0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x02,0x00,0x74,0x10,0x00,0x21,0xC0
};
const char UBLOX_M10_UBX[] PROGMEM = {  //CFG-UART1OUTPROT-UBX = 1, generated with ucenter2 !!
  0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x74, 0x10, 0x01, 0x21, 0xBC
};

const char UBLOX_M10_NAV_PVT[] PROGMEM = {  //CFG-MSGOUT-UBX_NAV_PVT_UART1, generated with ucenter2 !!
0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x07,0x00,0x91,0x20,0x01,0x54,0x51
};
const char UBLOX_M10_NAV_DOP[] PROGMEM = {  //CFG-MSGOUT-UBX_NAV_DOP_UART1, generated with ucenter2 !!
0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x39,0x00,0x91,0x20,0x01,0x86,0x4B
};
const char UBLOX_M10_NAV_SAT[] PROGMEM = {  //CFG-MSGOUT-UBX_NAV_SAT_UART1, rate 1/10, generated with ucenter2 !!
0xB5,0x62,0x06,0x8A,0x09,0x00,0x01,0x01,0x00,0x00,0x16,0x00,0x91,0x20,0x0A,0x6C,0xA5//rate 1/10
};

const char UBLOX_M10_UBX_BD19200[] PROGMEM = {  //Set baudrate to 19200 baud, generated with ucenter2 !!
0xB5,0x62,0x06,0x8A,0x0C,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x52,0x40,0x00,0x4B,0x00,0x00,0x7C,0x4A
};
const char UBLOX_M10_UBX_BD38400[] PROGMEM = {  //Set baudrate to 38400 baud, generated with ucenter2 !!
0xB5,0x62,0x06,0x8A,0x0C,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x52,0x40,0x00,0x96,0x00,0x00,0xC7,0x2B
};

const char UBLOX_M10_RATE[] PROGMEM = {//18 bytes !!
0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0xE8,0x03,0xD9,0xCE,//1 Hz
0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0xF4,0x01,0xE3,0xE4,//2Hz  
0XB5,0X62,0X06,0X8A,0X0A,0X00,0X01,0X01,0X00,0X00,0X01,0X00,0X21,0X30,0XFA,0X00,0XE8,0XEF,//4Hz
0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0xC8,0x00,0xB6,0x8B,//5Hz
0XB5,0X62,0X06,0X8A,0X0A,0X00,0X01,0X01,0X00,0X00,0X01,0X00,0X21,0X30,0X7D,0X00,0X6B,0XF5,//8Hz
0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0x64,0x00,0x52,0xC3,//10Hz
0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0x43,0x00,0x31,0x81,//15Hz 72-89 = 5
0xB5,0x62,0x06,0x8A,0x0A,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x21,0x30,0x32,0x00,0x20,0x5F//20Hz
};


const char UBX_M10_SEA[] PROGMEM = {
  0xB5,0x62,0x06 ,0x8A ,0x09 ,0x00 ,0x01 ,0x01 ,0x00 ,0x00 ,0x21 ,0x00 ,0x11 ,0x20 ,0x05 ,0xF2 ,0x57
};

const char UBX_M10_GET_NAV_RATE[] PROGMEM = {0xB5,0x62,0x06,0x8B,0x14,0x00,0x00,0x04,0x00,0x00,0x01,0x00,0xA4,0x40,0x03,0x00,0xA4,0x40,0x05,0x00,0xA4,0x40,0x0A,0x00,0xA4,0x40,0x4C,0x15
};//answer with NACK if default, or with CFGVALGET 44 bytes + ACK if high nav rate is set. Current consumption + 3-5 mA !!! 
const char UBX_M10_SET_HIGH_NAV_RATE[] PROGMEM = {0xB5,0x62,0x06,0x41,0x10,0x00,0x03,0x00,0x04,0x1F,0x54,0x5E,0x79,0xBF,0x28,0xEF,0x12,0x05,0xFD,0xFF,0xFF,0xFF,0x8F,0x0D,0xB5,0x62,0x06,0x41,0x1C,
0x00,0x04,0x01,0xA4,0x10,0xBD,0x34,0xF9,0x12,0x28,0xEF,0x12,0x05,0x05,0x00,0xA4,0x40,0x00,0xB0,0x71,0x0B,0x0A,0x00,0xA4,0x40,0x00,0xD8,0xB8,
0x05,0xDE,0xAE
};
const char UBX_MON_GNSS[] PROGMEM = {0xB5,0x62,0x0A,0x28,0x00,0x00,0x32,0xA0};//poll GNSS setting
const char UBX_MON_VER[] PROGMEM =  {0xB5,0x62,0x0A,0x04,0x00,0x00,0x0E,0x34};//poll SW version
const char UBX_NAV_SAT[] PROGMEM =  {0xB5,0x62,0x01,0x35,0x00,0x00,0x36,0xA3};//poll NAV_SAT
const char UBX_ID[] PROGMEM = {0xB5, 0x62, 0x27, 0x03, 0x00, 0x00, 0x2A, 0xA5}; //poll unique ID B5 62 27 03 00 00 2A A5 


const unsigned char UBX_HEADER[] = { 0xB5, 0x62 };
const unsigned char NAV_DUMMY_HEADER[] = { 0x01, 0x00 };
const unsigned char NAV_PVT_HEADER[] = { 0x01, 0x07 };
const unsigned char NAV_ACK_HEADER[] = { 0x05, 0x01 };
const unsigned char NAV_NACK_HEADER[] = { 0x05, 0x00 };
const unsigned char NAV_ID_HEADER[] = { 0x27, 0x03 };
const unsigned char MON_GNSS_HEADER[] = { 0x0A, 0x28 };
const unsigned char NAV_DOP_HEADER[] = { 0x01, 0x04 };
const unsigned char MON_VER_HEADER[] = { 0x0A, 0x04 };
const unsigned char NAV_SAT_HEADER[] = { 0x01, 0x35 };

enum _ubxMsgType {
  MT_NONE,
  MT_NAV_DUMMY,
  MT_NAV_PVT,
  MT_NAV_ACK,
  MT_NAV_NACK,
  MT_NAV_ID,
  MT_MON_GNSS,
  MT_NAV_DOP,
  MT_MON_VER,
  MT_NAV_SAT
};


struct NAV_DUMMY {
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  unsigned char msg_cls;
  unsigned char msg_id;
  unsigned char chkA;
  unsigned char chkB;
}__attribute__((__packed__));


struct NAV_PVT {  // 88 bytes payload, 92 bytes total, with Beitian BN220 100 bytes total ????(0xB5,0x62,....,chkA,chkB
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  unsigned long iTOW;        // GPS time of week of the navigation epoch (ms)
  unsigned short year;       // Year (UTC)
  unsigned char month;       // Month, range 1..12 (UTC)
  unsigned char day;         // Day of month, range 1..31 (UTC)
  unsigned char hour;        // Hour of day, range 0..23 (UTC)
  unsigned char minute;      // Minute of hour, range 0..59 (UTC)
  unsigned char second;      // Seconds of minute, range 0..60 (UTC)
  char valid;                // Validity Flags (see graphic below)
  unsigned long tAcc;        // Time accuracy estimate (UTC) (ns)
  long nano;                 // Fraction of second, range -1e9 .. 1e9 (UTC) (ns)
  unsigned char fixType;     // GNSSfix Type, range 0..5
  char flags;                // Fix Status Flags
  unsigned char reserved1;   // reserved
  unsigned char numSV;       // Number of satellites used in Nav Solution
  long lon;                  // Longitude (deg)
  long lat;                  // Latitude (deg)
  long height;               // Height above Ellipsoid (mm)
  long hMSL;                 // Height above mean sea level (mm)
  unsigned long hAcc;        // Horizontal Accuracy Estimate (mm)
  unsigned long vAcc;        // Vertical Accuracy Estimate (mm)
  long velN;                 // NED north velocity (mm/s)
  long velE;                 // NED east velocity (mm/s)
  long velD;                 // NED down velocity (mm/s)
  long gSpeed;               // Ground Speed (2-D) (mm/s)
  long heading;              // Heading of motion 2-D (deg)
  unsigned long sAcc;        // Speed Accuracy Estimate
  unsigned long headingAcc;  // Heading Accuracy Estimate
  unsigned short pDOP;       // Position dilution of precision
  short reserved2;           // Reserved
  unsigned long reserved3;   // Reserved
  long headVeh;              //only valid for adr4.1, beitian bn220 !
  short magDec;              //only valid for adr4.1,beitian bn220 !
  short magAcc;              //only valid for adr4.1,beitian bn220 !
  unsigned char chkA;
  unsigned char chkB;
}__attribute__((__packed__));


struct NAV_ACK {
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  unsigned char msg_cls;
  unsigned char msg_id;
  unsigned char chkA;
  unsigned char chkB;
}__attribute__((__packed__));


struct NAV_NACK {
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  unsigned char msg_cls;
  unsigned char msg_id;
  unsigned char chkA;
  unsigned char chkB;
}__attribute__((__packed__));


struct NAV_ID {
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  byte Version;
  byte reserved1;
  byte reserved2;
  byte reserved3;
  byte ubx_id_1;
  byte ubx_id_2;
  byte ubx_id_3;
  byte ubx_id_4;
  byte ubx_id_5;//M8 has only 5 byte ID !
  byte ubx_id_6;//M10 appeared to have 6 byte ID !!!
  unsigned char chkA;
  unsigned char chkB;
}__attribute__((__packed__));

struct MON_GNSS {
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  byte Version;
  byte supported_Gnss;
  byte default_Gnss;
  byte enabled_Gnss;
  byte simultaneous;
  byte reserved1;
  byte reserved2;
  byte reserved3;
  unsigned char chkA;
  unsigned char chkB;
}__attribute__((__packed__));


struct NAV_DOP {  //payload 18 bytes, total 22 bytes, without (__packed__) 24 bytes !!!
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  unsigned long iTOW;//4
  unsigned short gDOP;//6
  unsigned short pDOP;//8
  unsigned short tDOP;//10
  unsigned short vDOP;//12
  unsigned short hDOP;//14
  unsigned short nDOP;//16
  unsigned short eDOP;//18
  unsigned char chkA;
  unsigned char chkB;
}__attribute__((__packed__));
struct VER_EXT{
  char extension[30];
}__attribute__((__packed__));

struct MON_VER {
        unsigned char cls;
        unsigned char id;
        unsigned short len;
        char swVersion[30];
        char hwVersion[10];
        VER_EXT ext[6];         
        unsigned char chkA;
        unsigned char chkB;
    }__attribute__((__packed__));

    
constexpr uint8_t UBX_MAX_SVS = 64;


struct sVs_NAV_SAT{
        byte gnssId;
        byte svId;
        byte cno;
        int8_t elev;
        short azim;
        short prRes;
        unsigned long flags;  //bit3 = 1  : sV is used in navigation solution
}__attribute__((__packed__));   

struct NAV_SAT_HDR {
  uint8_t cls;
  uint8_t id;
  uint16_t len;
  uint32_t iTOW;
  uint8_t version;
  uint8_t numSvs;
  uint8_t reserved1;
  uint8_t reserved2;
} __attribute__((packed));



enum class UbxMsgKind : uint8_t {
  NONE,
  NAV_DUMMY,
  NAV_PVT,
  NAV_DOP,
  NAV_ACK,
  NAV_NACK,
  NAV_ID,
  MON_GNSS,
  MON_VER,
  NAV_SAT
};

struct UBXMessage {
  // ─────────────────────────────────────────────
  // Legacy flat members (KEEP THESE NAMES!)
  // ─────────────────────────────────────────────
  NAV_DUMMY navDummy;
  NAV_PVT   navPvt;
  NAV_DOP   navDOP;
  NAV_ACK   navAck;
  NAV_NACK  navNack;
  NAV_ID    ubxId;
  MON_GNSS  monGNSS;
  MON_VER   monVER;

  // ─────────────────────────────────────────────
  // NAV-SAT handled safely (NO variable array)
  // ─────────────────────────────────────────────
  NAV_SAT_HDR navSatHdr;
  sVs_NAV_SAT navSat[64];   // fixed max
  uint8_t     navSatCount;

  // ─────────────────────────────────────────────
  // Metadata (optional but useful)
  // ─────────────────────────────────────────────
  _ubxMsgType lastMsgType;
};



extern UBXMessage ubxMessage;  //declaration here, definition in Ublox.cpp
extern bool sdOK;
extern char dataStr[255];  //string for logging NMEA in txt, test for write 2000 chars !!
extern char Buffer[50];

void calcChecksum(unsigned char* CK,int msgType,int msgSize);
boolean compareMsgHeader(const unsigned char* msgHeader);
void Ublox_serial2(int delay_ms);
void Init_ubloxM10(void);
void Set_rate_ubloxM10(int rate);
bool Set_GPS_Time(float time_offset);
int processGPS();

int Check_M10_nav_rate(void);
int Set_M10_high_nav_rate(void);
