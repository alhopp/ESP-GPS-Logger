#pragma once
#include <stdint.h>

// Generic bitmap descriptor
struct BitmapDef {
  const uint8_t* data;
  uint16_t width;
  uint16_t height;
};
