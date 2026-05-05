#pragma once

#include <stdint.h>

// Initialise the Hall sensor input and internal timing state
void initMagnet();

// Poll the Hall sensor and interpret user intent
// Must be called frequently from loop()
void magnet_poll();

// Esternal state
#pragma once

extern bool magnet_active;

constexpr uint8_t MAGNET_PIN = 39;









