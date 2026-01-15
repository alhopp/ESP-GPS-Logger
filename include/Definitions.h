#pragma once
#include <stdint.h>

// ============================================================================
// Definitions.h
//
// Compile-time configuration, hardware wiring, thresholds, and logging macros.
// No runtime state or logic should live here.
// ============================================================================


// ============================================================================
// Build / feature flags
// ============================================================================

//#define STATIC_DEBUG            // GPS test without speed, WiFi active
//#define DLS                     // Force daylight saving test
#define T5_E_PAPER               // Compile without display functions
#define GPIO12_ACTIF             // GPIO12 used as wake-up pin (normal GPIO disabled)

#define GPS_SIMULATOR


// ============================================================================
// Firmware / timing
// ============================================================================

constexpr int VERSION                = 0;
constexpr int TIME_DELAY_FIRST_FIX   = 10;     // navPVT messages before logging
constexpr int TIME_DELAY_NEW_RUN     = 10;

constexpr uint32_t EPOCH_2022        = 1640995200UL;   // 2022-01-01


// ============================================================================
// u-blox configuration
// ============================================================================

constexpr uint8_t UBLOX_TYPE_UNKNOWN = 0;

constexpr uint8_t M8_9600BD          = 1;
constexpr uint8_t M10_9600BD         = 2;
constexpr uint8_t M8_38400BD         = 3;
constexpr uint8_t M10_38400BD        = 4;
constexpr uint8_t M9_9600BD          = 5;
constexpr uint8_t M9_38400BD         = 6;
constexpr uint8_t M8_115200BD        = 7;
constexpr uint8_t M9_115200BD        = 8;
constexpr uint8_t M10_115200BD       = 9;

constexpr uint8_t M10_DEFAULT_NAV    = 1;
constexpr uint8_t SET_M10_HIGH_NAV   = 2;
constexpr uint8_t M10_HIGH_NAV_RATE  = 3;
constexpr uint8_t AUTO_DETECT        = 0xFF;

inline bool isM10() { return true; }  // for now: hard lock


// ============================================================================
// Reference points (GPS test / calibration)
// ============================================================================

constexpr double Punt1_lat = 51.341970;
constexpr double Punt1_lon =  3.244888;

constexpr double Punt2_lat = 51.342876;
constexpr double Punt2_lon =  3.245013;

constexpr double Punt3_lat = 51.341907;
constexpr double Punt3_lon =  3.245205;

constexpr double Punt4_lat = 51.342886;
constexpr double Punt4_lon =  3.245366;


// ============================================================================
// Storage configuration (policy / behaviour)
// ============================================================================

// SPI SD tuning
constexpr uint32_t SD_SPI_FREQ_HZ = 25000000UL;   // drop to 10MHz if cards are flaky


// ============================================================================
// Board wiring
// ============================================================================

// SPI SD
constexpr uint8_t SD_SPI_CS   = 13;
constexpr uint8_t SD_SPI_SCK  = 14;
constexpr uint8_t SD_SPI_MOSI = 15;
constexpr uint8_t SD_SPI_MISO =  2;

// SD_MMC
// NOTE: GPIO2 is shared between SPI MISO and SD_MMC DAT0.
// Only one SD interface may be active at a time.
constexpr uint8_t SDMMC_DAT0_PIN = 2;   // MUST be pulled HIGH when no card present

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
// Battery & voltage calibration
// ============================================================================

constexpr float CALIBRATION_BAT_V      = 1.7f;

constexpr float VOLTAGE_100            = 4.15f;
constexpr float VOLTAGE_0              = 3.4f;

constexpr int   VOLTAGE_LOW            = 25;

constexpr float MINIMUM_VOLTAGE        = 0.0f;
constexpr float MINIMUM_VOLTAGE_CHANGE = 0.1f;

constexpr int   TOLERANCE              = 100;


// ============================================================================
// Sleep / watchdog / timing
// ============================================================================
constexpr int WDT_TIMEOUT         = 120;
constexpr int MAX_COUNT_WDT_TASK0 = 10;


// ============================================================================
// GPS quality thresholds
// ============================================================================

constexpr int MIN_numSV_FIRST_FIX    = 5;
constexpr int MAX_Sacc_FIRST_FIX     = 2;

constexpr int MIN_numSV_GPS_SPEED_OK = 4;
constexpr int MAX_Sacc_GPS_SPEED_OK  = 1;
constexpr int MAX_GPS_SPEED_OK       = 40;    // m/s


// ============================================================================
// EEPROM / calibration constants
// ============================================================================

constexpr int   STARTVALUE_HIGHEST_READ    = 1800;
constexpr int   NO_M10_GPS                 = 0;
constexpr float MMPS_TO_KNOTS              = 0.00194384f;

// ============================================================================
// Hardware pins (misc)
// ============================================================================

constexpr uint8_t MAGNET_PIN = 39;



// ============================================================================
// Logging
// ============================================================================

// Widths (tune once, applies everywhere)
constexpr int LOG_TAG_W  = 7;
constexpr int LOG_ITEM_W = 12;

// Core formatter
#define LOG_FMT(tag, item, fmt, ...)                                      \
  do {                                                                    \
    Serial.printf("[%-*s] %-*s : " fmt "\n",                              \
                  LOG_TAG_W, tag,                                         \
                  LOG_ITEM_W, item,                                       \
                  ##__VA_ARGS__);                                         \
  } while (0)

// Convenience wrappers
#define LOG_BOOT(item, fmt, ...)     LOG_FMT("BOOT",    item, fmt, ##__VA_ARGS__)
#define LOG_STORAGE(item, fmt, ...)  LOG_FMT("STORAGE", item, fmt, ##__VA_ARGS__)
#define LOG_CONFIG(item, fmt, ...)   LOG_FMT("CONFIG",  item, fmt, ##__VA_ARGS__)
#define LOG_WIFI(item, fmt, ...)     LOG_FMT("WiFi",    item, fmt, ##__VA_ARGS__)
#define LOG_TASK(item, fmt, ...)     LOG_FMT("TASK",    item, fmt, ##__VA_ARGS__)
#define LOG_LOOP(item, fmt, ...)     LOG_FMT("LOOP",    item, fmt, ##__VA_ARGS__)
#define LOG_ERROR(item, fmt, ...)    LOG_FMT("ERROR",   item, fmt, ##__VA_ARGS__)
#define LOG_GPS(item, fmt, ...)      LOG_FMT("GPS",     item, fmt, ##__VA_ARGS__)
#define LOG_SYS(item, fmt, ...)      LOG_FMT("MODE",    item, fmt, ##__VA_ARGS__)
