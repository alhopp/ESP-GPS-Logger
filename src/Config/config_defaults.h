#pragma once

// ============================================================================
// Config defaults
//
// Applies factory defaults to the global config object before JSON overrides are
// loaded or when the config file is missing/invalid.
// ============================================================================

void config_set_defaults();
