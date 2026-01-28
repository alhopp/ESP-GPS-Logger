#pragma once

#include <stdint.h>

class GPS_time {
public:
    // Constructor: time window in seconds
    GPS_time(int tijdvenster);

    // Update speed statistics for the current run
    float Update_speed(int actual_run);

    // Reset all stored statistics
    void Reset_stats(void);

    // Current averaged speed
    double avg_s;

    // Accumulator for average speed calculation
    int avg_s_sum;

    // Maximum speed detected in the current run
    double s_max_speed;

    // Live maximum shown on the display
    float display_max_speed;

    // Maximum speed of the most recent completed run
    float display_last_run;

    // Sorted top-10 speeds
    double avg_speed[10];
    double display_speed[10];

    // Average of the best 5 runs
    double avg_5runs;

    // Timestamp of best results
    uint8_t time_hour[10];
    uint8_t time_min[10];
    uint8_t time_sec[10];

    // Run index associated with each result
    int this_run[10];

    // Time window length (seconds)
    int time_window;

    // Bar-graph bookkeeping
    int speed_run_counter;
    uint16_t speed_run[50];

    // Satellite quality metrics at max-speed detection
    uint8_t Mean_cno[10];
    uint8_t Max_cno[10];
    uint8_t Min_cno[10];
    uint8_t Mean_numSat[10];

private:
    int old_run;                  // Previous run index
    int reset_display_last_run;   // Display reset guard
};


// ---- GLOBAL INSTANCES (unchanged API) ----
extern GPS_time S2;
extern GPS_time s2;
extern GPS_time S10;
extern GPS_time s10;
extern GPS_time S1800;
extern GPS_time S3600;
