#pragma once

// ============================================================================
// Runtime task launcher
//
// Starts the long-lived FreeRTOS tasks that run after app startup. Startup code
// owns when tasks are created; this module owns the task stack/core/priority
// settings and rollback if any required task fails to start.
// ============================================================================

bool startRuntimeTasks();
