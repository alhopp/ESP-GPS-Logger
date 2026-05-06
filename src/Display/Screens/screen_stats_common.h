#pragma once

#include "Display/E_paper.h"
#include "Fonts.h"
#include "GPS/gps_config.h"
#include "Layout.h"

float statsKnots(double value);
void statsPrintTime(int hour, int minute);
int statsRowFromBestIndex(int index);
void statsDrawLabelValueRow(int uiOffset, int row, const char* label, float value, int decimals = 2);
void statsDrawInlineValue(int x, int y, const char* label, float value, int decimals);

template<typename T>
void statsDrawDualRows(
  int uiOffset,
  const char* title,
  char leftPrefix,
  const T* leftData,
  char rightPrefix,
  const T* rightData
) {
  display.setFont(Fonts::Body12);
  display.setCursor(uiOffset, Layout::ROW18(1));
  display.print(title);

  for (int i = 9; i > 6; i--) {
    const int y = Layout::ROW18(2) + (9 - i) * Layout::STEP18;

    display.setCursor(uiOffset, y);
    display.print(leftPrefix);
    display.print(10 - i);
    display.print(" ");
    display.setFont(Fonts::Body18);
    display.print(statsKnots(leftData[i]), 1);

    if (i > 7) {
      display.setCursor(uiOffset + 118, y);
      display.setFont(Fonts::Body12);
      display.print(" ");
      display.print(rightPrefix);
      display.print(13 - i);
      display.print(" ");
      display.setFont(Fonts::Body18);
      display.print(statsKnots(rightData[i - 3]), 1);
    }
  }
}

template<typename T, typename V>
void statsDrawTimedList(
  int uiOffset,
  const char* label,
  const T& src,
  const V* values
) {
  display.setFont(Fonts::Body12);

  for (int i = 9; i > 4; i--) {
    display.setCursor(uiOffset, statsRowFromBestIndex(i));
    display.print(label);
    display.print(10 - i);
    display.print(": ");
    display.print(statsKnots(values[i]), 2);
    display.print(" @");
    statsPrintTime(src.time_hour[i], src.time_min[i]);
  }
}
