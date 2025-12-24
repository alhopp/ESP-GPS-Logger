#include <Arduino.h>
#include <SPI.h>
#include <sys/time.h>

#include "boot_manager.h"
#include "ESP_functions.h"
#include "config_manager.h"
#include "E_paper.h"
#include "rtc_state.h"

// Fonts (e-paper friendly)
#include "Fonts.h"

// -----------------------------------------------------------------------------
// EXTERNAL / RTC STATE
// -----------------------------------------------------------------------------

extern bool reset_boot;

// Battery scaling (must match hardware divider)
#ifndef BAT_SCALE
#define BAT_SCALE 1.0f
#endif

// -----------------------------------------------------------------------------
// PUBLIC ENTRY POINT (declare intent first)
// -----------------------------------------------------------------------------
void initBoot();

// -----------------------------------------------------------------------------
// LOCAL HELPERS (file-scope only)
// -----------------------------------------------------------------------------
static void shutdownWithMessage(const char* msg)
{
  RTC_OFF_screen = 1;

  strncpy(RTC_Sleep_txt, msg, sizeof(RTC_Sleep_txt) - 1);
  RTC_Sleep_txt[sizeof(RTC_Sleep_txt) - 1] = '\0';

  Shut_down();   // does not return
}

static void drawBootScreen()
{
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    int16_t  x1, y1;
    uint16_t w, h;

    // ---------------- TOP: ESP GPS ----------------
    display.setFont(Fonts::Big30);
    display.getTextBounds("ESP GPS", 0, 0, &x1, &y1, &w, &h);
    const int topY = 48;
    display.setCursor((display.width() - w) / 2, topY);
    display.print("ESP GPS");

    // ---------------- DIVIDER ----------------
    const int lineY = topY + 15;
    display.drawFastHLine(40, lineY, display.width() - 80, GxEPD_BLACK);

    // ---------------- BOTTOM: HW INFO ----------------
    display.setFont(Fonts::Body12);
    display.getTextBounds("M10 128MB", 0, 0, &x1, &y1, &w, &h);
    const int bottomY = lineY + 28;
    display.setCursor((display.width() - w) / 2, bottomY);
    display.print("M10 128MB");

  } while (display.nextPage());

  Serial.println("[BOOT   ] ESP Booted successfully");
}

// -----------------------------------------------------------------------------
// BOOT ENTRY POINT
// -----------------------------------------------------------------------------
void initBoot()
{
  // ---------------------------------------------------------------------------
  // Serial (ESP32-correct, bounded, deterministic)
  // ---------------------------------------------------------------------------
  Serial.begin(115200);

  // Short, bounded pause to allow USB terminal attach.
  // On ESP32, `while(!Serial)` is NOT a reliable indicator.
  const uint32_t t0 = millis();
  while (millis() - t0 < 400) {
    delay(10);
  }

  Serial.println(F("[BOOT   ] Initializing"));

  // ---------------------------------------------------------------------------
  // Battery ADC (fresh sample – never trust stale RTC data)
  // ---------------------------------------------------------------------------
  analogRead(PIN_BAT);          // discard first read
  delay(5);
  analog_mean = analogRead(PIN_BAT);

  // Convert to real voltage immediately
  RTC_voltage_bat = analog_mean * BAT_SCALE;

  Serial.print("[BOOT   ] Battery voltage = ");
  Serial.println(RTC_voltage_bat, 2);

  // ---------------------------------------------------------------------------
  // SPI + Timebase
  // ---------------------------------------------------------------------------
  Serial.println("[BOOT   ] SPI init");
  SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);

  // Reset system time; GNSS will set it later
  struct timeval tv = {};
  settimeofday(&tv, nullptr);

  // ---------------------------------------------------------------------------
  // Display init (no dependencies on storage or Wi-Fi)
  // ---------------------------------------------------------------------------
  Serial.println("[BOOT   ] Display init");
  display.init(115200, true, 2, false);
  display.setRotation(1);

  // ---------------------------------------------------------------------------
  // Boot UI
  // ---------------------------------------------------------------------------
  drawBootScreen();

  // Hold splash ONLY on true cold boot
  if (!reset_boot) {
    delay(1200);
  }

  // ---------------------------------------------------------------------------
  // Safety exits (hard stops)
  // ---------------------------------------------------------------------------
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    Serial.println("[BOOT   ] Low battery shutdown");
    shutdownWithMessage("Shut down Low Bat!");
    return; // for clarity only
  }

  if (reset_boot) {
    Serial.println("[BOOT   ] Shutdown after reset");
    shutdownWithMessage("Shutdown after reset!");
    return;
  }

  Serial.println("[BOOT   ] Boot checks passed");
}
