#pragma once
#include <Adafruit_GFX.h>

// ---- FreeSansBold ----
extern const GFXfont FreeSansBold6pt7b;
extern const GFXfont FreeSansBold9pt7b;
extern const GFXfont FreeSansBold12pt7b;
extern const GFXfont FreeSansBold18pt7b;
extern const GFXfont FreeSansBold24pt7b;
extern const GFXfont FreeSansBold30pt7b;
extern const GFXfont FreeSansBold75pt7b;
extern const GFXfont FreeSansBold80pt7b;
extern const GFXfont FreeSansBold100pt7b;

// ---- FreeMonoBold ----
extern const GFXfont FreeMonoBold9pt7b;
extern const GFXfont FreeMonoBold12pt7b;

// ---- SansSerif (custom) ----
extern const GFXfont SansSerif_bold_46_nr;
extern const GFXfont SansSerif_bold_84_nr;
extern const GFXfont SansSerif_bold_96_nr;

// ---- SF Distant Galaxy ----
extern const GFXfont SF_Distant_Galaxy9pt7b;

namespace Fonts {

  constexpr const GFXfont* Small6  = &FreeSansBold6pt7b;

  constexpr const GFXfont* Body9   = &FreeSansBold9pt7b;
  constexpr const GFXfont* Body12  = &FreeSansBold12pt7b;
  constexpr const GFXfont* Body18  = &FreeSansBold18pt7b;

  constexpr const GFXfont* Mono9   = &FreeMonoBold9pt7b;
  constexpr const GFXfont* Mono12  = &FreeMonoBold12pt7b;

  constexpr const GFXfont* SpeedM  = &SansSerif_bold_46_nr;
  constexpr const GFXfont* SpeedL  = &SansSerif_bold_84_nr;
  constexpr const GFXfont* SpeedXL = &SansSerif_bold_96_nr;

  constexpr const GFXfont* Big30   = &FreeSansBold30pt7b;
  constexpr const GFXfont* Huge75  = &FreeSansBold75pt7b;

}


