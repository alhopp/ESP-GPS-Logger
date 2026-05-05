#pragma once

// Initialise the Hall sensor input and internal timing state
void initMagnet();

// Poll the Hall sensor and interpret user intent
// Must be called frequently from loop()
void magnet_poll();

// External state
extern bool magnet_active;









