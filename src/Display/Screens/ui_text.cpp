#include "Display/Screens/ui_text.h"

#include "Display/E_paper.h"

void drawCenteredText(const char* text, int y, const GFXfont* font)
{
  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;

  display.setFont(font);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, y);
  display.print(text);
}
