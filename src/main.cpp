#include <Arduino.h>

// Managers
#include "wifi_manager.h"
#include "storage_manager.h"
#include "config_manager.h"
#include "eeprom_manager.h"
#include "watchdog_manager.h"

// Tasks
#include "task_gps.h"
#include "task_display.h"

// System
#include "system_mode.h"

// UI / misc you already rely on
#include "E_paper.h"
#include "screen_system.h"

static void startTasks();

extern RTC_DATA_ATTR int  RTC_Sail_Logo;
extern RTC_DATA_ATTR char RTC_Sleep_txt[32];

bool sleep_mode = false;
extern bool reset_boot;

void bootInit();




void setup() {
  bootInit();     // hardware + boot screen
  initStorage();  // SD + filesystems
  initConfig();   // load config

  wifi_init();    // <-- THIS decides HOME vs FIELD_CONFIG

  startTasks();
}

static void startTasks() {
  // GPS
  xTaskCreatePinnedToCore(taskOne, "TaskGPS", 10000, nullptr, 1, &t1, 1);

  // Display
  xTaskCreatePinnedToCore(taskTwo, "TaskDisplay", 10000, nullptr, 1, &t2, 0);

}

void loop() {
  watchdogLoop();

  SystemMode mode = getMode();

  if (mode == MODE_HOME || mode == MODE_FIELD_CONFIG) {
    wifi_loop();     // <-- web server + DNS
  }

  delay(10);
}


/*
  (your big SYSTEM OVERVIEW comment block can stay here or at the top)
*/


/*
====================================================================
 ESP32 GPS LOGGER — SYSTEM OVERVIEW
====================================================================

This firmware is built as a MODE-DRIVEN, MULTI-TASK system using
FreeRTOS tasks. Each subsystem has a single responsibility and
is enabled or disabled based on the current SystemMode.

--------------------------------------------------------------------
 SYSTEM MODES
--------------------------------------------------------------------

MODE_BOOT
  - Initial power-up state
  - System decides which mode to enter next based on:
      * saved configuration
      * button state
      * wake reason

MODE_LOGGING (Field / Normal Use)
  - Primary GPS logging mode
  - GPS task runs continuously
  - Display task shows live speed / stats
  - Wi-Fi is completely OFF
  - Lowest power usage during active logging

MODE_FIELD_CONFIG (Field Wi-Fi / Phone)
  - ESP32 runs as a Wi-Fi Access Point
  - Phone/tablet connects directly to the ESP32
  - Used to:
      * change settings
      * view stats/maps
      * download data
  - GPS continues running
  - Display remains active
  - No dependency on external internet

MODE_HOME (Home Wi-Fi / Maintenance)
  - ESP32 connects to home Wi-Fi (STA mode)
  - Web interface + OTA updates enabled
  - GPS may be paused or deprioritised
  - Display may show status instead of live data
  - Intended for maintenance, uploads, updates

MODE_SLEEP
  - Low power / idle state
  - GPS stopped
  - Wi-Fi OFF
  - Display OFF or static
  - Used for deep sleep or standby

--------------------------------------------------------------------
 TASK ARCHITECTURE
--------------------------------------------------------------------

task_gps.cpp
  - Owns all GPS, logging, and SD-card operations
  - Runs ONLY in MODE_LOGGING and MODE_FIELD_CONFIG
  - Never contains Wi-Fi or network logic
  - Real-time safe

task_display.cpp
  - Owns all e-paper / UI rendering
  - Updates based on SystemMode
  - Never starts/stops Wi-Fi

wifi_manager.cpp
  - Owns all Wi-Fi, web server, DNS, OTA logic
  - No blocking calls (no while-loops waiting on Wi-Fi)
  - Activated only via system_mode transitions

--------------------------------------------------------------------
 MODE CONTROL
--------------------------------------------------------------------

system_mode.cpp is the single authority that:
  - Changes SystemMode
  - Starts/stops Wi-Fi (AP / STA / OFF)
  - Ensures subsystems do not overlap incorrectly

main.cpp
  - Starts tasks
  - Services watchdog
  - Calls wifi_loop() ONLY when Wi-Fi is active
  - Does NOT directly control Wi-Fi or GPS logic

--------------------------------------------------------------------
 DESIGN PRINCIPLES
--------------------------------------------------------------------

- No blocking calls in tasks
- No network code in real-time GPS paths
- Wi-Fi is OFF by default unless explicitly enabled
- Power saving is achieved by disabling subsystems,
  not by delaying execution
- Each file has ONE clear responsibility

====================================================================
*/

 
