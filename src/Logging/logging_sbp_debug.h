#pragma once

struct SessionStatsSnapshot;

void logging_sbp_debug_print_samples(const char* sbpPath, const SessionStatsSnapshot& snapshot);
