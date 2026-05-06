#pragma once

#include <time.h>


// ============================================================================
// TIME / CLOCK / SYNC
// ============================================================================
extern char TimeZone[64];
extern tm   tmstruct;
extern int  Time_Set_OK;

// ============================================================================
// SHUTDOWN / SESSION CONTROL
// ============================================================================
extern bool reset_boot;
extern volatile bool woke_from_sleep;

