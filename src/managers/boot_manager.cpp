#include <Arduino.h>
#include <SPI.h>
#include <sys/time.h>

#include "ESP_functions.h"
#include "config_manager.h"
#include "E_paper.h"

// Fonts (e-paper friendly)
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

// ----------------------------------------------------
// RTC / boot state (extern)
// ----------------------------------------------------
extern float RTC_voltage_bat;
extern float RTC_minimum_voltage_bat;
extern int   RTC_OFF_screen;
extern RTC_DATA_ATTR char RTC_Sleep_txt[32];
extern bool reset_boot;

// ----------------------------------------------------
// Local helpers
// ----------------------------------------------------
static void shutdownWithMessage(const char* msg)
{
  RTC_OFF_screen = 1;
  strncpy(RTC_Sleep_txt, msg, sizeof(RTC_Sleep_txt) - 1);
  RTC_Sleep_txt[sizeof(RTC_Sleep_txt) - 1] = '\0';
  Shut_down();
}

static void drawBootScreen()
{
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    int16_t  x1, y1;
    uint16_t w, h;

    // -------- TOP: ESP GPS --------
    display.setFont(&FreeSansBold24pt7b);
    display.getTextBounds("ESP GPS", 0, 0, &x1, &y1, &w, &h);
    int topY = 48;
    display.setCursor((display.width() - w) / 2, topY);
    display.print("ESP GPS");

    // -------- DIVIDER LINE --------
    int lineY = topY + 15;
    display.drawFastHLine(40, lineY, display.width() - 80, GxEPD_BLACK);

    // -------- BOTTOM: M10 128MB --------
    display.setFont(&FreeSansBold12pt7b);
    display.getTextBounds("M10 128MB", 0, 0, &x1, &y1, &w, &h);
    int bottomY = lineY + 28;
    display.setCursor((display.width() - w) / 2, bottomY);
    display.print("M10 128MB");

  } while (display.nextPage());
  Serial.println("[BOOT   ] ESP Booted successfully");
}


// ----------------------------------------------------
// Boot entry point
// ----------------------------------------------------
void bootInit()
{
  // ---------------- Serial ----------------
  Serial.begin(115200);
   uint32_t t0 = millis();
   while (!Serial && millis() - t0 < 3000) delay(200);
  Serial.println(F("[BOOT   ] Initializing"));

  // ---------------- Power / ADC ----------------
  analogRead(PIN_BAT);          // discard first read
  delay(5);
  analog_mean = analogRead(PIN_BAT);

  // ---------------- SPI / Time ----------------
  SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);

  struct timeval tv = {};
  settimeofday(&tv, nullptr);

  // ---------------- Display ----------------
  display.init(115200, true, 2, false);
  display.setRotation(1);

  // ---------------- Boot UI ----------------
  drawBootScreen();
  delay(1200);   // splash hold

  // ---------------- Safety checks ----------------
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    shutdownWithMessage("Shut down Low Bat!");
    return;
  }

  if (reset_boot) {
    shutdownWithMessage("Shutdown after reset!");
    return;
  }
}
