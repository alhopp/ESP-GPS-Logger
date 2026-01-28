

#pragma once
#include <stdint.h>

class GPS_speed {
public:
    // Constructor
    // afstand = distance window in meters over which average speed is calculated
    GPS_speed(int afstand);

    // Update distance-based speed calculation for the current run
    double Update_distance(int actual_run);

    double m_speed;        // Average speed over the configured distance window
    double m_speed_alfa;   // Average speed for Alfa calculation (shorter distance)
    double m_max_speed;    // Maximum speed recorded in the current run

    double avg_speed[10];      // Top speeds (sorted, per run)
    double display_speed[10];  // Copy used for display sorting

    int m_Distance[10];    // Distance covered for each stored speed entry
    uint8_t time_hour[10];
    uint8_t time_min[10];
    uint8_t time_sec[10];

    int this_run[10];      // Run index associated with each speed entry
    int nr_samples[10];   // Number of samples used per calculation
    int message_nr[10];   // UBX message index for traceability

    int m_index;           // Start index of the distance window in the GPS buffer
    int m_distance;        // Accumulated distance (first value exceeding target)
    int m_distance_alfa;   // Accumulated distance below target (for Alfa)
    int m_set_distance;    // Configured distance window in meters
    int m_Set_Distance;    // Distance window in internal units (mm * sample_rate)
    int m_sample;          // Number of samples in the current window

private:
    int old_run;           // Previous run index (used to detect run transitions)
};


extern GPS_speed M100;
extern GPS_speed M250;
extern GPS_speed M500;
extern GPS_speed M1852;

