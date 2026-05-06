#pragma once

#include <Adafruit_GFX.h>

extern const GFXfont FreeSansBold9pt7b;
extern const GFXfont FreeSansBold12pt7b;
extern const GFXfont FreeSansBold18pt7b;
extern const GFXfont FreeSansBold30pt7b;
extern const GFXfont FreeSansBold75pt7b;

extern const GFXfont FreeMonoBold9pt7b;
extern const GFXfont FreeMonoBold12pt7b;

extern const GFXfont SansSerif_bold_84_nr;

namespace Fonts {

constexpr const GFXfont* Body9 = &FreeSansBold9pt7b;
constexpr const GFXfont* Body12 = &FreeSansBold12pt7b;
constexpr const GFXfont* Body18 = &FreeSansBold18pt7b;

constexpr const GFXfont* Mono9 = &FreeMonoBold9pt7b;
constexpr const GFXfont* Mono12 = &FreeMonoBold12pt7b;

constexpr const GFXfont* SpeedL = &SansSerif_bold_84_nr;

constexpr const GFXfont* Big30 = &FreeSansBold30pt7b;
constexpr const GFXfont* Huge75 = &FreeSansBold75pt7b;

} // namespace Fonts
