

#pragma once
#include <stdint.h>
#include <math.h>

// -----------------------------------------------------------------------------
// GPS result sorting helpers
//
// Responsibilities:
// - Sort run result arrays by speed while keeping all associated metadata aligned
// - Sorting is performed in-place on fixed-size arrays
// - All parallel arrays (time, distance, CNO, run index, etc.) are kept in sync
// -----------------------------------------------------------------------------

// Sort run results by speed (descending), keeping timing and satellite data aligned
void sort_run(double a[],
              uint8_t hour[],
              uint8_t minute[],
              uint8_t seconde[],
              uint8_t mean_cno[],
              uint8_t max_cno[],
              uint8_t min_cno[],
              uint8_t nrSats[],
              int runs[],
              int size);

// Sort alfa run results by speed, keeping distance and sample metadata aligned
void sort_run_results(double a[],
                   int dis[],
                   int message[],
                   uint8_t hour[],
                   uint8_t minute[],
                   uint8_t seconde[],
                   int runs[],
                   int samples[],
                   int size);

void sort_display(double a[], int size);


