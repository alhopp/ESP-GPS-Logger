#pragma once

// ======================================================
// Layout.h
// ------------------------------------------------------
// 8-pixel aligned layout system (EPD-safe)
// Backwards compatible with existing ROW9 / ROW12 / ROW18
// ======================================================

namespace Layout {

  // --------------------------------------------------
  // E-paper hardware rule
  // --------------------------------------------------
  constexpr int GRID = 8;

  constexpr int SNAP8(int v) {
    return (v + (GRID - 1)) & ~(GRID - 1);
  }

  // --------------------------------------------------
  // Font metrics (logical, not hardware)
  // --------------------------------------------------
  constexpr int SPACING = 2;

  constexpr int H9  = 14;
  constexpr int H12 = 19;
  constexpr int H18 = 25;

  constexpr int STEP9  = H9  + SPACING;
  constexpr int STEP12 = H12 + SPACING;
  constexpr int STEP18 = H18 + SPACING;

  // --------------------------------------------------
  // Row helpers (NOW SNAP TO 8 PX)
  // --------------------------------------------------
  constexpr int ROW9(int n)  {
    return SNAP8(H9  + (n - 1) * STEP9);
  }

  constexpr int ROW12(int n) {
    return SNAP8(H12 + (n - 1) * STEP12);
  }

  constexpr int ROW18(int n) {
    return SNAP8(H18 + (n - 1) * STEP18);
  }

  // --------------------------------------------------
  // Generic grid rows (new, preferred)
  // --------------------------------------------------
  constexpr int ROW(int n) {
    return GRID * n;
  }

  // --------------------------------------------------
  // System areas (USED FOR PARTIAL REFRESH)
  // --------------------------------------------------
  constexpr int STATUS_Y = ROW(3);
  constexpr int STATUS_H = ROW(8) - ROW(3);

  constexpr int INFO_BAR_HEIGHT = SNAP8(H12 + SPACING);
  constexpr int CONTENT_TOP     = INFO_BAR_HEIGHT + GRID;

  // Graph layout (already safe)
  constexpr int GRAPH_LEFT   = 5;
  constexpr int GRAPH_WIDTH  = 215;
  constexpr int GRAPH_HEIGHT = 64;
}
