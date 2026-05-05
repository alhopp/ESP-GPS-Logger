#include "GPS/Ublox/ublox_driver.h"
#include "Core/Definitions.h"


// ============================================================================
// ublox_driver.cpp
//
// High-level u-blox driver entry point.
//
// This file intentionally contains ONLY:
//   • Global driver objects (UART + UBX message storage)
//   • Minimal public-facing initialisation glue
//
// All heavy logic is split out for clarity:
//
//   - ublox_init.cpp    → GPS configuration, UBX setup, time sync
//   - ublox_parser.cpp  → Streaming UBX frame parser (state machine)
//
// Rationale:
//   • Keeps this file small and readable
//   • Makes ownership boundaries obvious
//   • Prevents “god file” creep as features are added
//
// If this file ever grows beyond ~50 lines, something is in the wrong place.
// ============================================================================


// -----------------------------------------------------------------------------
// Global UBX message storage
//
// Parsed UBX messages are written here by the streaming parser.
// The active member depends on the last message type returned by processGPS().
// -----------------------------------------------------------------------------
UBXMessage ubxMessage = {};


// -----------------------------------------------------------------------------
// u-blox UART interface
//
// Owns the dedicated ESP32 UART used to communicate with the u-blox M10.
//
// Design notes:
//   • Uses HardwareSerial port 2 (UART2)
//   • Baud rate is fixed to 38400 after GPS init
//   • UBX-only protocol (NMEA disabled during Init_ubloxM10())
//   • RX/TX pins are defined centrally in Core/Definitions.h
//
// IMPORTANT:
//   • "UART1" in u-blox documentation refers to the GPS module’s UART,
//     NOT the ESP32 UART numbering.
// -----------------------------------------------------------------------------
HardwareSerial UbloxSerial(2);


// -----------------------------------------------------------------------------
// ubloxSerialInit
//
// Initialise the ESP32 UART connected to the u-blox module.
//
// This does NOT configure the GPS itself — it only brings up the ESP32 side.
// Full GPS configuration is handled later in Init_ubloxM10().
//
// Parameters:
//   delay_ms : Optional settle delay after UART start (module dependent)
// -----------------------------------------------------------------------------
void ubloxSerialInit(int delay_ms)
{
    UbloxSerial.begin(
        38400,                // Baud rate (matches final GPS config)
        SERIAL_8N1,            // UART framing
        GPS_UART_RX_PIN,       // ESP32 RX  ← GPS TX
        GPS_UART_TX_PIN        // ESP32 TX  → GPS RX
    );

    delay(delay_ms);           // Allow hardware + GPS UART to stabilise
}



