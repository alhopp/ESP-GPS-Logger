#include "screen_context.h"
#include "Layout.h"
#include "config_manager.h"

static int ui_offset = 0;

/* =========================================================
 * Local helpers
 * ========================================================= */
namespace {
  inline float cal(double v) {
    return v * calibration_speed;
  }

  inline void printTime(int h, int m) {
    display.print(h);
    display.print(m < 10 ? ":0" : ":");
    display.print(m);
  }

  inline int rowFromIndex(int i) {
    return 24 * (10 - i);
  }
}



/* =========================================================
 * STATS 1–3
 * ========================================================= */
void draw_STATS1() {
  Stats_2s_3_lines(
    "10sF: ", "10sS: ", "AVG:  ",
    cal(S10.display_speed[9]),
    cal(S10.display_speed[5]),
    cal(S10.avg_5runs)
  );
}

void draw_STATS2() {
  static bool toggle;

  Stats_4lines(
    "Dist: ", "1852m: ", toggle ? "3600s: " : "1800s: ", "Alfa: ",
    Ublox.total_distance / 1000,
    cal(M1852.display_speed[9]),
    cal(toggle ? S3600.display_max_speed : S1800.display_max_speed),
    cal(A500.avg_speed[9])
  );

  toggle = !toggle;
}

void draw_STATS3() {
  Stats_4lines(
    "100m:", "250m:", "500m:", "Alfa:",
    cal(M100.display_speed[9]),
    cal(M250.display_speed[9]),
    cal(M500.display_speed[9]),
    cal(A500.avg_speed[9])
  );
}

/* =========================================================
 * Shared helpers (templated – works with double arrays)
 * ========================================================= */
template<typename T>
static void drawDualRows(
  const char* title,
  char leftPrefix,
  const T* leftData,
  char rightPrefix,
  const T* rightData
) {
  display.setFont(Fonts::Body12);
  display.setCursor(ui_offset, Layout::ROW18(1));
  display.print(title);

  for (int i = 9; i > 6; i--) {
    int y = Layout::ROW18(2) + (9 - i) * Layout::STEP18;

    display.setCursor(ui_offset, y);
    display.print(leftPrefix);
    display.print(10 - i);
    display.print(" ");
    display.setFont(Fonts::Body18);
    display.print(cal(leftData[i]), 1);

    if (i > 7) {
      display.setCursor(ui_offset + 118, y);
      display.setFont(Fonts::Body12);
      display.print(" ");
      display.print(rightPrefix);
      display.print(13 - i);
      display.print(" ");
      display.setFont(Fonts::Body18);
      display.print(cal(rightData[i - 3]), 1);
    }
  }
}

template<typename T, typename V>
static void drawTimedList(
  const char* label,
  const T& src,
  const V* values
) {
  display.setFont(Fonts::Body12);

  for (int i = 9; i > 4; i--) {
    display.setCursor(ui_offset, rowFromIndex(i));
    display.print(label);
    display.print(10 - i);
    display.print(": ");
    display.print(cal(values[i]), 2);
    display.print(" @");
    printTime(src.time_hour[i], src.time_min[i]);
  }
}

/* =========================================================
 * STATS 4–5
 * ========================================================= */
void draw_STATS4() {
  display.setFont(Fonts::Body12);
  display.setCursor(ui_offset, Layout::ROW18(1));
  display.print("10s Avg: ");
  display.setFont(Fonts::Body18);
  display.println(cal(S10.avg_5runs), 2);

  drawDualRows("", 'R', S10.display_speed, 'R', S10.display_speed);
}

void draw_STATS5() {
  drawDualRows("Last Alfa stats ! ", 'A', A500.avg_speed, 'A', A500.avg_speed);
}

/* =========================================================
 * STATS 6 – table view
 * ========================================================= */
void draw_STATS6() {
  Serial.println("STATS6_Simon_screen");

  constexpr int ROWS = 6;
  const int row0 = 15, step = 17;

  int row[ROWS];
  for (int i = 0; i < ROWS; i++) row[i] = row0 + i * step;

  const int col1 =ui_offset;
  const int col2 = ui_offset + 46;
  const int col3 = ui_offset + 114;
  const int col4 = ui_offset + 182;

  const float leftVal[ROWS] = {
    cal(S10.avg_5runs),
    cal(S10.display_speed[9]),
    cal(S10.display_speed[8]),
    cal(S10.display_speed[7]),
    cal(S10.display_speed[6]),
    cal(S10.display_speed[5])
  };

  const float rightVal[ROWS] = {
    cal(S2.display_speed[9]),
    cal(S10.s_max_speed),
    static_cast<float>(Ublox.total_distance) * 1e-6f,
    cal(A500.avg_speed[9]),
    cal(M500.display_speed[9]),
    cal(M1852.display_speed[9])
  };

  const char* leftLbl[ROWS]  = { "AV:", "R1:", "R2:", "R3:", "R4:", "R5:" };
  const char* rightLbl[ROWS] = { "2sec:", "Prv :", "Dist:", "Alp :", "500m:", "NM:" };

  display.setFont(Fonts::Mono12);
  for (int i = 0; i < ROWS; i++) {
    display.setCursor(col1, row[i]); display.print(leftLbl[i]);
    display.setCursor(col3, row[i]); display.print(rightLbl[i]);
  }

  display.setFont(Fonts::Body12);
  for (int i = 0; i < ROWS; i++) {
    display.setCursor(col2, row[i]); display.println(leftVal[i], 2);
    display.setCursor(col4, row[i]); display.println(rightVal[i], i == 2 ? 0 : 2);
  }

  float prv = cal(S10.s_max_speed);
  int line = row[ROWS - 1];
  for (int i = 1; i < ROWS; i++) {
    if (prv > leftVal[i]) { line = row[i - 1]; break; }
  }

  display.fillRect(0, line + 2, col3 - 10, 2, GxEPD_BLACK);
}

/* =========================================================
 * STATS 7 – bar graph
 * ========================================================= */
void draw_STATS7() {
  Serial.println("STATS7_Simon_bar graph");

  const int posX = 5;
  const int posY = 0;
  const int GraphWidth = 215;

  const int MaxBars = NR_OF_BAR;
  int barSpace = 2, barWidth = 3, barPitch;
  static int r;

  int top = cal(S10.display_speed[9]);
  int max_bar = max(int(top / 5 + 1) * 5, 24);
  int step = (max_bar > 45) ? 5 : 3;
  int min_bar = max_bar - step * 8;
  float scale = 80.0f / (max_bar - min_bar);

  display.setFont(Fonts::Body9);
  display.setCursor(0, 15);
  display.println("Graph : Speed runs (10sec)");

  r = run_count % MaxBars + 1;

  display.setFont(Fonts::Small6);
  for (int i = 0; i < 9; i++) {
    int y = posY - i * 10;
    display.fillRect(ui_offset + posX, y, GraphWidth, 1, GxEPD_BLACK);
    display.setCursor(ui_offset + 225, y);
    display.print(min_bar + i * step);
  }

  display.setCursor(0, 26);
  display.print("R1-R5:");
  for (int i = 9; i > 4; i--) {
    display.print(cal(S10.display_speed[i]));
    if (i > 5) display.print(" ");
  }
  display.println();

  int bars = (run_count < MaxBars) ? r : MaxBars;
  barWidth = max((GraphWidth - bars * barSpace) / bars, 3);
  barPitch = barWidth + barSpace;

  for (int i = 0; i < bars; i++) {
    int idx = (run_count < MaxBars) ? i : (i + r) % MaxBars;
    int h = (cal(S10.speed_run[idx]) - min_bar) * scale;
    display.fillRect(ui_offset + posX + i * barPitch, posY - h, barWidth, h, GxEPD_BLACK);
  }
}

/* =========================================================
 * STATS 8–A
 * ========================================================= */
void draw_STATS8() { drawTimedList("500 ", M500, M500.avg_speed); }
void draw_STATS9() { drawTimedList("Run ",  S10,  S10.avg_speed); }
void draw_STATSA() { drawTimedList("2s ",   S2,   S2.avg_speed); }

/* =========================================================
 * STATS B
 * ========================================================= */
void draw_STATSB() {
  Stats_2s_3_lines(
    "10sLast: ", "10sBest: ", "AVG :  ",
    cal(S10.display_last_run),
    cal(S10.display_speed[9]),
    cal(S10.avg_5runs)
  );
}



void Stats_4lines(
  const char* m1, const char* m2,
  const char* m3, const char* m4,
  float v1, float v2, float v3, float v4
) {
  constexpr int VALUE_COL = 150;

  display.setFont(Fonts::Body18);

  const char* labels[4] = { m1, m2, m3, m4 };


  for (int i = 0; i < 4; ++i) {
    int y = Layout::ROW18(i + 1);

    display.setCursor(ui_offset, y);
    display.print(labels[i]);

    display.setCursor(VALUE_COL, y);

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
  // Top row: 2s info
  display.setFont(Fonts::Body12);
  display.setCursor(ui_offset, Layout::ROW18(1));
  display.print("2l: ");

  display.setFont(Fonts::Body18);
  display.print(S2.display_last_run * calibration_speed, 1);

  display.setFont(Fonts::Body12);
  display.setCursor(ui_offset + 120, Layout::ROW18(1));
  display.print("2s: ");

  display.setFont(Fonts::Body18);
  display.print(S2.display_speed[9] * calibration_speed, 1);

  // Remaining rows
  display.setFont(Fonts::Body12);

  display.setCursor(ui_offset, Layout::ROW18(2));
  display.print(m1);
  display.println(v1);

  display.setCursor(ui_offset, Layout::ROW18(3));
  display.print(m2);
  display.println(v2);

  display.setCursor(ui_offset, Layout::ROW18(4));
  display.print(m3);
  display.println(v3);
}





