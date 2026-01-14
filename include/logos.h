#pragma once
#include <stdint.h>

#if defined(ESP8266) || defined(ESP32)
  #include <pgmspace.h>
#else
  #include <avr/pgmspace.h>
#endif


// ============================================================================
// Logo descriptor
// ============================================================================

struct LogoDef {
  uint8_t id;              // legacy config ID (DO NOT CHANGE)
  const char* name;        // UI / debug only
  const uint8_t* bitmap;   // PROGMEM bitmap
  uint8_t width;
  uint8_t height;
};

