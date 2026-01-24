#pragma once

#include <Arduino.h>

// GPS (UART2)
constexpr uint8_t GPS_UART_RX_PIN = 32;   // u-blox TX
constexpr uint8_t GPS_UART_TX_PIN = 33;   // u-blox RX

// u-blox power / control pins (GPIO numbers only — no ESP-IDF types here)
constexpr uint8_t UBLOX_POWER1     = 25;
constexpr uint8_t UBLOX_POWER2     = 26;
constexpr uint8_t UBLOX_POWER3     = 27;

constexpr uint8_t UBLOX_RTC_GPIO1  = 25;
constexpr uint8_t UBLOX_RTC_GPIO2  = 26;
constexpr uint8_t UBLOX_GPIO3      = 27;

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

// Enable a GNSS constellations simultaneously:
//
//   • GPS
//   • GLONASS
//   • GALILEO
//
// Improves fix robustness and satellite availability.
// Used once during Init_ubloxM10().
//
// Enable GNSS constellations simultaneously:
//
//   • GPS
//   • GLONASS
//   • GALILEO
//
// Disable others to preserve tracking channels.
// Safe for M10 at ≤5 Hz.
// Applied once during Init_ubloxM10().
//
constexpr uint8_t m10_3gnss[] PROGMEM = {
  0xB5, 0x62,
  0x06, 0x8A,              // CFG-VALSET
  0x25, 0x00,              // payload length = 37 bytes
  0x01, 0x01,              // version=1, layer=RAM
  0x00, 0x00,              // reserved

  // CFG-GNSS-GPS_ENA      = 1
  0x1F, 0x00, 0x31, 0x10,  0x01,

  // CFG-GNSS-GLO_ENA      = 1
  0x20, 0x00, 0x31, 0x10,  0x01,

  // CFG-GNSS-GAL_ENA      = 1
  0x21, 0x00, 0x31, 0x10,  0x01,

  // CFG-GNSS-BDS_ENA      = 0
  0x22, 0x00, 0x31, 0x10,  0x00,

  // CFG-GNSS-QZSS_ENA    = 0
  0x23, 0x00, 0x31, 0x10,  0x00,

  // CFG-GNSS-SBAS_ENA    = 0
  0x24, 0x00, 0x31, 0x10,  0x00,

  // checksum
  0x7A, 0x12
};

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

