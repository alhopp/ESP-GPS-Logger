#pragma once

#include <stdint.h>

// Initialise the Hall sensor input and internal timing state
void magnet_init();

// Poll the Hall sensor and interpret user intent
// Must be called frequently from loop()
void magnet_poll();
