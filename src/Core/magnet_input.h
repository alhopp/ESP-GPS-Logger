#pragma once

// ============================================================================
// Magnet input
//
// Polls the Hall sensor and maps short/long holds to system-mode requests.
// Rendering code may read magnet_active to draw the on-screen affordance.
// ============================================================================

void initMagnet();

// Poll the Hall sensor and interpret user intent. Must be called frequently
// from loop().
void magnet_poll();

extern bool magnet_active;









