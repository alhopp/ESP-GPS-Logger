#pragma once

// ============================================================================
// Serial logging helpers
//
// Fixed-width tagged logging for boot/runtime diagnostics. Logging is compiled
// out when LOG_ENABLED is false or stats-only serial output is enabled.
// ============================================================================

#include <Arduino.h>

#include "Core/build_config.h"

constexpr int LOG_TAG_W = 7;
constexpr int LOG_ITEM_W = 12;

#if LOG_ENABLED && !STATS_ONLY_SERIAL
#define LOG_FMT(tag, item, fmt, ...)                                      \
  do {                                                                    \
    Serial.printf("[%-*s] %-*s : " fmt "\n",                              \
                  LOG_TAG_W, tag,                                         \
                  LOG_ITEM_W, item,                                       \
                  ##__VA_ARGS__);                                         \
  } while (0)
#else
#define LOG_FMT(tag, item, fmt, ...) do { } while (0)
#endif

#define LOG_BOOT(item, fmt, ...)     LOG_FMT("BOOT",    item, fmt, ##__VA_ARGS__)
#define LOG_STORAGE(item, fmt, ...)  LOG_FMT("STORAGE", item, fmt, ##__VA_ARGS__)
#define LOG_CONFIG(item, fmt, ...)   LOG_FMT("CONFIG",  item, fmt, ##__VA_ARGS__)
#define LOG_WIFI(item, fmt, ...)     LOG_FMT("WiFi",    item, fmt, ##__VA_ARGS__)
#define LOG_TASK(item, fmt, ...)     LOG_FMT("TASK",    item, fmt, ##__VA_ARGS__)
#define LOG_LOOP(item, fmt, ...)     LOG_FMT("LOOP",    item, fmt, ##__VA_ARGS__)
#define LOG_ERROR(item, fmt, ...)    LOG_FMT("ERROR",   item, fmt, ##__VA_ARGS__)
#define LOG_GPS(item, fmt, ...)      LOG_FMT("GPS",     item, fmt, ##__VA_ARGS__)
#define LOG_SYS(item, fmt, ...)      LOG_FMT("MODE",    item, fmt, ##__VA_ARGS__)
