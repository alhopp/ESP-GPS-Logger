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

// ============================================================================
// Board logo IDs (legacy, DO NOT RENUMBER)
// ============================================================================

enum BoardLogoID : uint8_t {
  BOARD_LOGO_NONE = 0,
  BOARD_STARBOARD = 1,
  BOARD_FANATIC = 2,
  BOARD_JP = 3,
  BOARD_NOVE_NOVE = 4,
  BOARD_MISTRAL = 5,
  BOARD_GOYA = 6,
  BOARD_PATRIK = 7,
  BOARD_SEVERNE = 8,
  BOARD_TABOU = 9,
  BOARD_F2 = 10,
  BOARD_SCHWEIGHOFER = 11,
  BOARD_THOMMEN = 12,
  BOARD_BIC = 13,
  BOARD_CARBON_ART = 14,
  BOARD_FUTURE_FLY = 15,
  BOARD_ONE_HUNDRED = 16,
  BOARD_FMX = 17,
  BOARD_PHANTOM = 18,
  BOARD_F4_FOIL = 19,
  BOARD_LISA = 20
};

// ============================================================================
// Sail logo IDs (legacy, DO NOT RENUMBER)
// ============================================================================

enum SailLogoID : uint8_t {
  SAIL_LOGO_NONE = 0,
  SAIL_GA = 1,
  SAIL_DUOTONE = 2,
  SAIL_NP_NP = 3,
  SAIL_NP_O = 4,
  SAIL_LOFT = 5,
  SAIL_GUN = 6,
  SAIL_POINT7 = 7,
  SAIL_SIMMER = 8,
  SAIL_NAISH = 9,
  SAIL_SEVERNE = 10,
  SAIL_S2_MAUI = 11,
  SAIL_NORTH = 12,
  SAIL_CHALLENGER = 13,
  SAIL_PHANTOM = 14,
  SAIL_PATRIK = 15,
  SAIL_LISA_V = 16
};

// ============================================================================
// Public API
// ============================================================================

const LogoDef* getBoardLogo(uint8_t id);
const LogoDef* getSailLogo(uint8_t id);
