#pragma once

// ============================================================================
// Configuration manager
//
// Public entry points for loading /config.txt at startup and persisting config
// changes made through the web API.
// ============================================================================

#include "Config/config_types.h"

void initConfig();
void saveConfig();



