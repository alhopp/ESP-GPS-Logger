#pragma once

// ============================================================================
// Legacy GxEPD compatibility wrapper
// Backend: GxEPD2 (paged)
// ============================================================================

#include <Arduino.h>
#include <Adafruit_GFX.h>

#include <GxEPD2_BW.h>

// -----------------------------------------------------------------------------
// PANEL SELECTION (exactly ONE must be defined)
// -----------------------------------------------------------------------------

#if defined(EPD_213_B74)
  #include <epd/GxEPD2_213_B74.h>
  using PanelDriver = GxEPD2_213_B74;

#elif defined(EPD_213_B73)
  #include <epd/GxEPD2_213_B73.h>
  using PanelDriver = GxEPD2_213_B73;

#elif defined(EPD_266_BN)
  #include <epd/GxEPD2_266_BN.h>
  using PanelDriver = GxEPD2_266_BN;

#elif defined(EPD_154_OC1)
  #include <epd/GxEPD2_154_GDEY0154D67.h>
  using PanelDriver = GxEPD2_154_GDEY0154D67;

#else
  #error "Define exactly one EPD_xxx panel type"
#endif

// -----------------------------------------------------------------------------
// LEGACY COLORS
// -----------------------------------------------------------------------------

#ifndef GxEPD_BLACK
  #define GxEPD_BLACK  0
  #define GxEPD_WHITE  1
#endif

// ============================================================================
// GxEPD_Class — legacy API, GxEPD2 backend
// ============================================================================

class GxEPD_Class
  : public GxEPD2_BW<PanelDriver, PanelDriver::HEIGHT>
{
public:
  using Base = GxEPD2_BW<PanelDriver, PanelDriver::HEIGHT>;

  // ---------------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------------
  GxEPD_Class(int8_t cs, int8_t dc, int8_t rst, int8_t busy)
  : Base(PanelDriver(cs, dc, rst, busy)) {}

  // ---------------------------------------------------------------------------
  // Legacy init()
  // ---------------------------------------------------------------------------
  void init(uint32_t serial_diag_bitrate = 0)
  {
    Base::init(serial_diag_bitrate);
  }

  // ---------------------------------------------------------------------------
  // Legacy update()  (FULL refresh, correct paging)
  // ---------------------------------------------------------------------------
  void update()
  {
    Base::firstPage();
    do {
      // drawing already performed by application
    } while (Base::nextPage());
  }

  // ---------------------------------------------------------------------------
  // Legacy updateWindow() (partial refresh)
  // ---------------------------------------------------------------------------
  void updateWindow(int16_t x, int16_t y,
                    int16_t w, int16_t h,
                    bool = true)
  {
    Base::displayWindow(x, y, w, h);
  }

  // ---------------------------------------------------------------------------
  // Legacy fillScreen()
  // ---------------------------------------------------------------------------
  void fillScreen(uint16_t color)
  {
    Base::fillScreen(color);
  }

  // ---------------------------------------------------------------------------
  // Legacy bitmap helper
  // ---------------------------------------------------------------------------
  void drawExampleBitmap(const uint8_t* bitmap,
                         int16_t x, int16_t y,
                         int16_t w, int16_t h,
                         uint16_t color)
  {
    Base::drawBitmap(x, y, bitmap, w, h, color);
  }

  // ---------------------------------------------------------------------------
  // Power control
  // ---------------------------------------------------------------------------
  void powerDown()
  {
    Base::hibernate();
  }

  void wakeUp() {}

  // ---------------------------------------------------------------------------
  // Expose Adafruit_GFX / Print API
  // ---------------------------------------------------------------------------
  using Base::setFont;
  using Base::setCursor;
  using Base::setTextColor;
  using Base::setTextWrap;
  using Base::setRotation;

  using Base::drawPixel;
  using Base::drawBitmap;
  using Base::fillRect;
  using Base::drawRect;

  using Base::print;
  using Base::println;
  using Base::printf;
  using Base::write;

  using Base::width;
  using Base::height;
};

// Legacy alias
typedef GxEPD_Class GxEPD;
