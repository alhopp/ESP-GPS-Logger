#include "GPS/gps_run_stats.h"

RunResult run_results[MAX_SESSION_RUNS] = {};
int run_results_count = 0;

void run_stats_update(int run, float best2s, float best10s)
{
  if(run <= 0) return;

  // avoid duplicates
  if(run_results_count > 0 &&
     run_results[run_results_count - 1].run == run)
    return;

  if(run_results_count >= MAX_SESSION_RUNS) return;

  run_results[run_results_count++] = {
    run,
    best2s,
    best10s
  };
}
