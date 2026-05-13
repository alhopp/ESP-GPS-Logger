// ============================================================================
// Shared legacy runtime globals
//
// Contains only data declarations for the remaining cross-cutting globals.
// New state should live in the module that owns the behaviour.
// ============================================================================

#include "Core/Globals.h"

// ============================================================================
// TIME / CLOCK / SYNC
// ============================================================================
char TimeZone[64] = "GMT0";
tm tmstruct{};
int Time_Set_OK = 0;

// ============================================================================
// WAKE INPUT STATE
// ============================================================================
volatile bool woke_from_sleep = false;
