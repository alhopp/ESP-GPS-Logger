#include "Display/Screens/screen_system.h"

#include "Display/Screens/screen_system_common.h"
#include "Fonts.h"
#include "Layout.h"
#include "Web/wifi_manager.h"

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
      drawCenteredText("Connecting to hotspot", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Open: gps.local", Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_FAILED:
      drawCenteredText("Wi-Fi connection failed", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Retrying...", Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_AP:
      drawCenteredText("Wi-Fi setup required", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Open: gps.local", Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_CONNECTED:
      drawCenteredText("Wi-Fi connected", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Open: gps.local", Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_OFF:
    default:
      drawCenteredText("Wi-Fi idle", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Open: gps.local", Layout::ROW9(6), Fonts::Body9);
      break;
  }
}
