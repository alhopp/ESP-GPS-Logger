#include "Display/Screens/screen_stats_common.h"

float statsKnots(double value)
{
  return value * MMPS_TO_KNOTS;
}

void statsPrintTime(int hour, int minute)
{
  display.print(hour);
  display.print(minute < 10 ? ":0" : ":");
  display.print(minute);
}

int statsRowFromBestIndex(int index)
{
  return 24 * (10 - index);
}

void statsDrawLabelValueRow(int uiOffset, int row, const char* label, float value, int decimals)
{
  constexpr int VALUE_COL = 150;

  const int y = Layout::ROW18(row);
  display.setFont(Fonts::Body12);
  display.setCursor(uiOffset, y);
  display.print(label);

  display.setCursor(VALUE_COL, y);
  display.setFont(Fonts::Body18);
  display.print(value, decimals);
}

void statsDrawInlineValue(int x, int y, const char* label, float value, int decimals)
{
  display.setFont(Fonts::Body12);
  display.setCursor(x, y);
  display.print(label);

  display.setFont(Fonts::Body18);
  display.print(value, decimals);
}
