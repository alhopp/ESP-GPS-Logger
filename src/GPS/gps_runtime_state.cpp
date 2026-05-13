#include "GPS/gps_runtime_state.h"

bool GPS_Signal_OK = false;
int  last_gps_msg    = 0;
int  nav_pvt_message = 0;

int   run_count = 0;
int   old_run_count = 0;
int   gps_speed_value = 0;
