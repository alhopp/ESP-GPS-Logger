#pragma once

#include <stdint.h>

// ============================================================================
// Board pin map
//
// Central source of truth for LilyGO T5 ESP32 GPS logger wiring.
// Keep these values hardware-only: no runtime state and no driver logic.
// ============================================================================

// E-paper display
constexpr uint8_t ELINK_SS = 5;
constexpr uint8_t ELINK_DC = 17;
constexpr uint8_t ELINK_RESET = 16;
constexpr uint8_t ELINK_BUSY = 4;

// Inputs
constexpr uint8_t BATTERY_ADC_PIN = 35;  // ADC input only
constexpr uint8_t MAGNET_PIN = 39;       // Input only, RTC wake capable

// SD_MMC / eMMC
constexpr uint8_t SDMMC_DAT0_PIN = 2;    // Must be pulled high when no card present

// u-blox UART
constexpr uint8_t GPS_UART_RX_PIN = 32;  // ESP32 RX <- u-blox TX
constexpr uint8_t GPS_UART_TX_PIN = 33;  // ESP32 TX -> u-blox RX

// u-blox power / control pins
constexpr uint8_t UBLOX_POWER1 = 25;
constexpr uint8_t UBLOX_POWER2 = 26;
constexpr uint8_t UBLOX_POWER3 = 27;

constexpr uint8_t UBLOX_RTC_GPIO1 = 25;
constexpr uint8_t UBLOX_RTC_GPIO2 = 26;
constexpr uint8_t UBLOX_GPIO3 = 27;
