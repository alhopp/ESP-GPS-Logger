#include "Display/Screens/screen_stats.h"

#include "Core/Globals.h"
#include "Display/E_paper.h"
#include "Display/Screens/screen_stats_common.h"
#include "Fonts.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_time_speed.h"
#include "Layout.h"

static int ui_offset = 0;

namespace {

void drawStats6Table()
{
  Serial.println("STATS6_Simon_screen");

  constexpr int ROWS = 6;
  const int row0 = 15;
  const int step = 17;

  int row[ROWS];
  for (int i = 0; i < ROWS; i++) {
    row[i] = row0 + i * step;
  }

  const int col1 = ui_offset;
  const int col2 = ui_offset + 46;
  const int col3 = ui_offset + 114;
  const int col4 = ui_offset + 182;

  const float leftVal[ROWS] = {
    statsKnots(S10.avg_5runs),
    statsKnots(S10.display_speed[9]),
    statsKnots(S10.display_speed[8]),
    statsKnots(S10.display_speed[7]),
    statsKnots(S10.display_speed[6]),
    statsKnots(S10.display_speed[5])
  };

  const float rightVal[ROWS] = {
    statsKnots(S2.display_speed[9]),
    statsKnots(S10.s_max_speed),
    static_cast<float>(total_distance) * 1e-6f,
    statsKnots(A500.avg_speed[9]),
    statsKnots(M500.display_speed[9]),
    statsKnots(M1852.display_speed[9])
  };

  const char* leftLbl[ROWS] = { "AV:", "R1:", "R2:", "R3:", "R4:", "R5:" };
  const char* rightLbl[ROWS] = { "2sec:", "Prv :", "Dist:", "Alp :", "500m:", "NM:" };

  display.setFont(Fonts::Mono12);
  for (int i = 0; i < ROWS; i++) {
    display.setCursor(col1, row[i]);
    display.print(leftLbl[i]);
    display.setCursor(col3, row[i]);
    display.print(rightLbl[i]);
  }

  display.setFont(Fonts::Body12);
  for (int i = 0; i < ROWS; i++) {
    display.setCursor(col2, row[i]);
    display.println(leftVal[i], 2);
    display.setCursor(col4, row[i]);
    display.println(rightVal[i], (i == 2) ? 0 : 2);
  }

  const float previous = statsKnots(S10.s_max_speed);
  int line = row[ROWS - 1];
  for (int i = 1; i < ROWS; i++) {
    if (previous > leftVal[i]) {
      line = row[i - 1];
      break;
    }
  }

  display.fillRect(0, line + 2, col3 - 10, 2, GxEPD_BLACK);
}

} // namespace

void Stats_4lines(
  const char* m1,
  const char* m2,
  const char* m3,
  const char* m4,
  float v1,
  float v2,
  float v3,
  float v4
) {
  const char* labels[4] = { m1, m2, m3, m4 };
  const float values[4] = { v1, v2, v3, v4 };

  for (int i = 0; i < 4; ++i) {
    statsDrawLabelValueRow(ui_offset, i + 1, labels[i], values[i]);
  }
}

void Stats_2s_3_lines(
  const char* m1,
  const char* m2,
  const char* m3,
  float v1,
  float v2,
  float v3
) {
  statsDrawInlineValue(ui_offset, Layout::ROW18(1), "2l: ", statsKnots(S2.display_last_run), 1);
  statsDrawInlineValue(ui_offset + 120, Layout::ROW18(1), "2s: ", statsKnots(S2.display_speed[9]), 1);

  display.setFont(Fonts::Body12);

  display.setCursor(ui_offset, Layout::ROW18(2));
  display.print(m1);
  display.println(v1, 2);

  display.setCursor(ui_offset, Layout::ROW18(3));
  display.print(m2);
  display.println(v2, 2);

  display.setCursor(ui_offset, Layout::ROW18(4));
  display.print(m3);
  display.println(v3, 2);
}

void draw_STATS(uint8_t page)
{
  if (page < STATS_PAGE_MIN) {
    page = STATS_PAGE_MIN;
  }
  if (page > STATS_PAGE_MAX) {
    page = STATS_PAGE_MAX;
  }

  switch (page) {
    case 1:
      Stats_2s_3_lines(
        "10sF: ", "10sS: ", "AVG:  ",
        statsKnots(S10.display_speed[9]),
        statsKnots(S10.display_speed[5]),
        statsKnots(S10.avg_5runs)
      );
      break;

    case 2: {
      static bool toggle;

      Stats_4lines(
        "Dist: ", "1852m: ", toggle ? "3600s: " : "1800s: ", "Alfa: ",
        total_distance / 1000,
        statsKnots(M1852.display_speed[9]),
        statsKnots(toggle ? S3600.display_max_speed : S1800.display_max_speed),
        statsKnots(A500.avg_speed[9])
      );

      toggle = !toggle;
      break;
    }

    case 3:
      Stats_4lines(
        "100m:", "250m:", "500m:", "Alfa:",
        statsKnots(M100.display_speed[9]),
        statsKnots(M250.display_speed[9]),
        statsKnots(M500.display_speed[9]),
        statsKnots(A500.avg_speed[9])
      );
      break;

    case 4:
      display.setFont(Fonts::Body12);
      display.setCursor(ui_offset, Layout::ROW18(1));
      display.print("10s Avg: ");
      display.setFont(Fonts::Body18);
      display.println(statsKnots(S10.avg_5runs), 2);

      statsDrawDualRows(ui_offset, "", 'R', S10.display_speed, 'R', S10.display_speed);
      break;

    case 5:
      statsDrawDualRows(ui_offset, "Last Alfa stats ! ", 'A', A500.avg_speed, 'A', A500.avg_speed);
      break;

    case 6:
      drawStats6Table();
      break;

    case 7:
      break;

    case 8:
      statsDrawTimedList(ui_offset, "500 ", M500, M500.avg_speed);
      break;

    case 9:
      statsDrawTimedList(ui_offset, "Run ", S10, S10.avg_speed);
      break;

    case 10:
      statsDrawTimedList(ui_offset, "2s ", S2, S2.avg_speed);
      break;

    case 11:
      Stats_2s_3_lines(
        "10sLast: ", "10sBest: ", "AVG :  ",
        statsKnots(S10.display_last_run),
        statsKnots(S10.display_speed[9]),
        statsKnots(S10.avg_5runs)
      );
      break;

    default:
      display.setFont(Fonts::Body12);
      display.setCursor(ui_offset, Layout::ROW18(1));
      display.print("STATS page?");
      break;
  }
}
