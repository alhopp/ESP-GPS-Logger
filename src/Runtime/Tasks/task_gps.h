#pragma once

// ============================================================================
// GPS runtime task
//
// Task entry point for polling the GPS source, updating metric state, starting
// sessions when policy allows, writing fixes, and requesting GPS-driven display
// refreshes.
// ============================================================================

void gpsTask(void* parameter);
