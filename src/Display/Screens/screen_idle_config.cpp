#include "Display/Screens/screen_system.h"

#include "Display/Screens/screen_system_common.h"
#include "Display/Screens/ui_text.h"
#include "Fonts.h"
#include "Layout.h"
#include "Web/wifi_manager.h"

namespace {

void drawConfigStatus(const char* line1, const char* line2)
{
  drawCenteredText(line1, Layout::ROW9(4), Fonts::Body9);
  drawCenteredText(line2, Layout::ROW9(6), Fonts::Body9);
}

} // namespace

void draw_IDLE()
{
  drawMagnet();

  drawCenteredText("ESP-GPS", Layout::ROW9(2), Fonts::Body12);
  drawCenteredText("Tap: Start", Layout::ROW9(4), Fonts::Body9);
  drawCenteredText("Hold: Settings", Layout::ROW9(5), Fonts::Body9);
  drawCenteredText("Use magnet to select mode", Layout::ROW9(7), Fonts::Body9);
}

void draw_WIFI_CONFIG()
{
  drawMagnet();
  drawCenteredText("CONFIG", Layout::ROW9(2), Fonts::Body12);

  switch (wifi_get_ui_state()) {
    case WIFI_UI_TRYING:
      drawConfigStatus("Connecting to hotspot", "Open: gps.local");
      break;

    case WIFI_UI_FAILED:
      drawConfigStatus("Wi-Fi connection failed", "Retrying...");
      break;

    case WIFI_UI_AP:
      drawConfigStatus("Wi-Fi setup required", "Open: gps.local");
      break;

    case WIFI_UI_CONNECTED:
      drawConfigStatus("Wi-Fi connected", "Open: gps.local");
      break;

    case WIFI_UI_OFF:
    default:
      drawConfigStatus("Wi-Fi idle", "Open: gps.local");
      break;
  }
}
