#include "Display/Screens/ui_fixed_numbers.h"

#include <Arduino.h>
#include <cstring>

#include "Display/E_paper.h"
#include "Fonts.h"

namespace {

int digitCellWidth = 0;
int dotCellWidth = 0;
bool digitCellsReady = false;

void initDigitCells()
{
  const GFXfont* body = Fonts::Body12;
  const GFXfont* mono = Fonts::Mono12;

  for (char c = '0'; c <= '9'; ++c) {
    if (c < body->first || c > body->last) {
      continue;
    }

    digitCellWidth = max(digitCellWidth, static_cast<int>(body->glyph[c - body->first].xAdvance));
  }

  if ('.' >= mono->first && '.' <= mono->last) {
    dotCellWidth = mono->glyph['.' - mono->first].xAdvance - 8;
    if (dotCellWidth < 4) {
      dotCellWidth = 4;
    }
  }

  digitCellsReady = true;
}

} // namespace

void drawFixedNumber(int xRight, int yBaseline, float value)
{
  if (!digitCellsReady) {
    initDigitCells();
  }

  char buf[8];
  dtostrf(value, 6, 2, buf);

  int x = xRight;

  for (int i = strlen(buf) - 1; i >= 0; --i) {
    const char c = buf[i];
    const bool dot = (c == '.');

    const int cellWidth = dot ? dotCellWidth : digitCellWidth;
    const GFXfont* font = dot ? Fonts::Mono12 : Fonts::Body12;

    x -= cellWidth;
    display.setFont(font);

    if (c < font->first || c > font->last) {
      continue;
    }

    const GFXglyph& glyph = font->glyph[c - font->first];
    display.setCursor(x + (cellWidth - glyph.xAdvance) / 2, yBaseline);
    display.write(c);
  }
}
