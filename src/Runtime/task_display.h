#pragma once

// ============================================================================
// Display runtime task
//
// Task entry point for the only code path that renders to the e-paper display
// and enters deep sleep after the final sleep screen refresh.
// ============================================================================

void displayTask(void* parameter);
