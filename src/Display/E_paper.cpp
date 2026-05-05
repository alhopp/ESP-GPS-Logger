// ============================================================================
// E_paper.cpp
// - Owns physical e-paper object
// - Owns low-level e-paper initialization
// ============================================================================

#include <Arduino.h>
#include "Display/E_paper.h"

// ============================================================================
// Display instance (OWNED HERE)
// ============================================================================
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
  GxEPD2_213_B74(ELINK_SS, ELINK_DC, ELINK_RESET, ELINK_BUSY)
);

void display_init()
{
  display.init(115200, true, 2, false);
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
}
