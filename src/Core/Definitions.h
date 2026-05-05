#pragma once
#include <stdint.h>

// ============================================================================
// Core/Definitions.h
//
// Compile-time system configuration only.
// - No runtime state
// - No hardware drivers
// - No protocol-specific wiring
// ============================================================================

// ---------------------------------------------------------------------------
// Timing / run detection
// ---------------------------------------------------------------------------
constexpr int TIME_DELAY_NEW_RUN = 10;

// ---------------------------------------------------------------------------
// SD / storage (board-level, minimal)
// ---------------------------------------------------------------------------
constexpr uint8_t SDMMC_DAT0_PIN = 2;   // MUST be pulled HIGH when no card present

// ---------------------------------------------------------------------------
// Battery / voltage calibration
// ---------------------------------------------------------------------------
constexpr float CALIBRATION_BAT_V      = 1.7f;
constexpr float VOLTAGE_100            = 4.15f;
constexpr float VOLTAGE_0              = 3.4f;
constexpr int   VOLTAGE_LOW            = 25;
constexpr float MINIMUM_VOLTAGE        = 0.0f;
constexpr float MINIMUM_VOLTAGE_CHANGE = 0.1f;
constexpr int   TOLERANCE              = 100;

// ---------------------------------------------------------------------------
// Watchdog / sleep
// ---------------------------------------------------------------------------
constexpr int WDT_TIMEOUT         = 120;   // seconds
constexpr int MAX_COUNT_WDT_TASK0 = 10;

// ---------------------------------------------------------------------------
// GPS quality thresholds (policy, not protocol)
// ---------------------------------------------------------------------------
constexpr int MIN_numSV_FIRST_FIX    = 5;
constexpr int MAX_Sacc_FIRST_FIX     = 2;

constexpr int MIN_numSV_GPS_SPEED_OK = 4;
constexpr int MAX_Sacc_GPS_SPEED_OK  = 1;
constexpr int MAX_GPS_SPEED_OK       = 40;   // m/s

// ---------------------------------------------------------------------------
// Calibration / unit helpers
// ---------------------------------------------------------------------------
constexpr int   STARTVALUE_HIGHEST_READ = 1800;
constexpr int   NO_M10_GPS              = 0;
constexpr double MMPS_TO_KNOTS = 0.0019438444924406;
constexpr double CMPS_TO_KNOTS = 0.019438444924406;

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

// Column widths
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
