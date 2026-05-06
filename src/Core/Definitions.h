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
// Calibration / unit helpers
// ---------------------------------------------------------------------------
constexpr int   STARTVALUE_HIGHEST_READ = 1800;

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
