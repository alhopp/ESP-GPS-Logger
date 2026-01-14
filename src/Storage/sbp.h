#pragma once
#include <FS.h>

struct SBP_Header { // 64 bytes
  uint16_t Text_length;
  uint8_t  Id1;
  uint8_t  Id2;
  uint16_t Again_length;
  uint8_t  Start;
  char     Identity[57];
} __attribute__((packed));


struct SBP_frame { // 32 bytes
  uint8_t  HDOP;
  uint8_t  SVIDCnt;
  uint16_t UtcSec;
  uint32_t date_time_UTC_packed;
  uint32_t SVIDList;
  int32_t  Lat;
  int32_t  Lon;
  int32_t  AltCM;
  uint16_t Sog;
  uint16_t Cog;
  int16_t  ClmbRte;
  uint8_t  sdop;
  uint8_t  vsdop;
} __attribute__((packed));

extern SBP_Header sbp_header;
extern SBP_frame  sbp_frame;

void log_header_SBP(File &file);
void log_SBP(File &file);
