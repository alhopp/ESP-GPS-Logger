#pragma once

struct DisplayWindow {
  int x;
  int y;
  int w;
  int h;
};

constexpr DisplayWindow DISPLAY_FULL_WINDOW = {0, 0, 250, 122};

// Request a full screen redraw (async, display-task owned)
void screen_request_redraw();

// Request a partial screen redraw (async, display-task owned)
void screen_request_partial(DisplayWindow window);
