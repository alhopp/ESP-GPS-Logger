#pragma once
#include <stdint.h> 

// ============================================================================
// Definitions.h
//
// Compile-time configuration, hardware wiring, thresholds, and logging macros.
// No runtime state or logic should live here.
// ============================================================================


//#define STATIC_DEBUG            // GPS test without speed, WiFi active
//#define DLS                     // Force daylight saving test
#define T5_E_PAPER               // Compile without display functions
#define GPIO12_ACTIF             // GPIO12 used as wake-up pin (normal GPIO disabled)


// ============================================================================
// Boot mode & buttons
// ============================================================================

enum BootMode {
  MODE_CONFIG,
  MODE_RUN
};

extern BootMode bootMode;

// ============================================================================
// Firmware / timing
// ============================================================================

#define VERSION                     0
#define TIME_DELAY_FIRST_FIX        10      // navPVT messages before logging
#define TIME_DELAY_NEW_RUN          10
#define EPOCH_2022                  1640995200UL   // 2022-01-01


// ============================================================================
// u-blox configuration
// ============================================================================

#define UBLOX_TYPE_UNKNOWN          0

#define M8_9600BD                   1
#define M10_9600BD                  2
#define M8_38400BD                  3
#define M10_38400BD                 4
#define M9_9600BD                   5
#define M9_38400BD                  6
#define M8_115200BD                 7
#define M9_115200BD                 8
#define M10_115200BD                9

#define NO_M10_GPS                  0
#define M10_DEFAULT_NAV             1
#define SET_M10_HIGH_NAV            2
#define M10_HIGH_NAV_RATE           3
#define AUTO_DETECT                 0xFF


// ============================================================================
// Reference points (GPS test / calibration)
// ============================================================================

#define Punt1_lat                   51.341970
#define Punt1_lon                    3.244888
#define Punt2_lat                   51.342876
#define Punt2_lon                    3.245013
#define Punt3_lat                   51.341907
#define Punt3_lon                    3.245205
#define Punt4_lat                   51.342886
#define Punt4_lon                    3.245366


// ============================================================================
// STORAGE CONFIG (policy / behaviour)
// ============================================================================

// SPI SD tuning
#define SD_SPI_FREQ_HZ              25000000UL   // drop to 10MHz if cards are flaky

// SD_MMC behaviour
#define SD_MMC_MOUNTPOINT           "/sdcard"
#define SD_MMC_1BIT_MODE            true         // safest default across SD / SDNAND


// ============================================================================
// BOARD WIRING
// ============================================================================

// SPI SD
#define SD_SPI_CS                   13
#define SD_SPI_SCK                  14
#define SD_SPI_MOSI                 15
#define SD_SPI_MISO                  2

// SD_MMC
#define SDMMC_DAT0_PIN               2    // MUST be pulled HIGH when no card present

// GPS (UART2)
#define RXD2                        32    // u-blox TX
#define TXD2                        33    // u-blox RX

// Battery / power
#define PIN_BAT                     35

#define UBLOX_POWER1                25
#define UBLOX_RTC_GPIO1             GPIO_NUM_25
#define UBLOX_POWER2                26
#define UBLOX_RTC_GPIO2             GPIO_NUM_26
#define UBLOX_POWER3                27
#define UBLOX_GPIO3                 GPIO_NUM_27

// Sleep / wake / control
#define GO_TO_SLEEP_GPIO            39
#define WAKE_UP_GPIO_NUM            GPIO_NUM_39
#define GO_TO_SLEEP_PULLDOWN        19
#define HOLD_PIN                    21


// ============================================================================
// Battery & voltage calibration
// ============================================================================

#define CALIBRATION_BAT_V            1.7
#define VOLTAGE_100                 4.15
#define VOLTAGE_0                   3.4
#define VOLTAGE_LOW                 25

#define MINIMUM_VOLTAGE             0
#define MINIMUM_VOLTAGE_CHANGE      0.1

#define STARTVALUE_HIGHEST_READ     2300
#define MAXVALUE_HIGHEST_READ       2700
#define TOLERANCE                   100

#define FULLY_CHARGED_LIPO_VOLTAGE  4200.0


// ============================================================================
// Sleep / watchdog / timing
// ============================================================================

#define uS_TO_S_FACTOR              1000000UL
#define TIME_TO_SLEEP               3600UL
#define WDT_TIMEOUT                 120
#define MAX_COUNT_WDT_TASK0         10


// ============================================================================
// GPS quality thresholds
// ============================================================================

#define MIN_numSV_FIRST_FIX          5
#define MAX_Sacc_FIRST_FIX           2

#define MIN_numSV_GPS_SPEED_OK       4
#define MAX_Sacc_GPS_SPEED_OK        1
#define MAX_GPS_SPEED_OK             40    // m/s


// ============================================================================
// Storage / filesystem
// ============================================================================

#define EEPROM_SIZE                 32
#define TIME_OUT_NAV_PVT            10000
#define FORMAT_LITTLEFS_IF_FAILED   true


/// ----------------------------------------------------
// Hardware pins
// ----------------------------------------------------
constexpr uint8_t MAGNET_PIN = 39;


// ============================================================================
// Logging
// ============================================================================

// Widths (tune once, applies everywhere)
#define LOG_TAG_W                   7
#define LOG_ITEM_W                  12

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



