#pragma once

// ======================================================
// Layout.h
// ------------------------------------------------------
// Screen geometry & vertical rhythm helpers
// NO fonts, NO display objects, NO globals
// Header-only, constexpr-only
// ======================================================

namespace Layout {

  constexpr int SPACING = 2;

  constexpr int H9  = 14;
  constexpr int H12 = 19;
  constexpr int H18 = 25;

  constexpr int STEP9  = H9  + SPACING;
  constexpr int STEP12 = H12 + SPACING;
  constexpr int STEP18 = H18 + SPACING;

  constexpr int ROW9(int n)  { return H9  + (n - 1) * STEP9; }
  constexpr int ROW12(int n) { return H12 + (n - 1) * STEP12; }
  constexpr int ROW18(int n) { return H18 + (n - 1) * STEP18; }

  constexpr int INFO_BAR_HEIGHT = H12 + SPACING;

  constexpr int CONTENT_TOP = INFO_BAR_HEIGHT + SPACING;

  constexpr int GRAPH_LEFT   = 5;
  constexpr int GRAPH_WIDTH  = 215;
  constexpr int GRAPH_HEIGHT = 60;
}


