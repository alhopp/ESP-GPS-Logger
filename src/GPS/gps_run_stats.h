#pragma once

struct RunResult {
  int   run;
  float best_2s;
  float best_10s;
};

constexpr int MAX_SESSION_RUNS = 128;

extern RunResult run_results[MAX_SESSION_RUNS];
extern int run_results_count;

void run_stats_update(int run, float best2s, float best10s);
