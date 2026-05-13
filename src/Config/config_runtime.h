#pragma once

// ============================================================================
// Config runtime application
//
// Applies config values that must be mirrored into runtime globals/RTC state
// after loading defaults or reading /config.txt.
// ============================================================================

void config_apply_runtime();
