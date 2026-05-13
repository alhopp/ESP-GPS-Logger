#pragma once

// ============================================================================
// Config JSON
//
// Reads and writes the persistent /config.txt JSON format. Existing key names
// are compatibility surface and should not be changed casually.
// ============================================================================

#include <FS.h>

bool config_load_json(File& file);
void config_write_json(File& file);
