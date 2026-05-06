#pragma once

// Shared display geometry constants.
// DisplayWindow is used by the display task to request bounded partial refreshes.

struct DisplayWindow {
  int x;
  int y;
  int w;
  int h;
};

constexpr int DISPLAY_WIDTH = 250;
constexpr int DISPLAY_HEIGHT = 122;

constexpr DisplayWindow DISPLAY_FULL_WINDOW = {
  0,
  0,
  DISPLAY_WIDTH,
  DISPLAY_HEIGHT
};

constexpr DisplayWindow DISPLAY_BOTTOM_STATUS_WINDOW = {
  0,
  100,
  DISPLAY_WIDTH,
  DISPLAY_HEIGHT - 100
};
