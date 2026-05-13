#pragma once

// ============================================================================
// Shared legacy runtime globals
//
// This header is intentionally small. Prefer module-owned state for new code;
// keep only cross-cutting globals that still have multiple owners.
// ============================================================================

#include <time.h>


// ============================================================================
// TIME / CLOCK / SYNC
// ============================================================================
extern char TimeZone[64];
extern tm   tmstruct;
extern int  Time_Set_OK;

// ============================================================================
// WAKE INPUT STATE
// ============================================================================
extern volatile bool woke_from_sleep;

