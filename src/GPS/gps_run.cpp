#include <stdint.h>

#include "Core/Definitions.h"      // TIME_DELAY_NEW_RUN
#include "core/system_info.h"     // systemInfo
#include "Core/Globals.h"         // heading_SD, Mean_heading, run_count, etc
#include "Ublox/ublox.h"

#include "GPS/GPS_data.h"         // alfa_counter
#include "GPS/gps_utils.h"
#include "GPS/gps_run.h"


// -----------------------------------------------------------------------------
// IMPORTANT: index_GPS is the absolute NAV-PVT sample counter (monotonic)
// -----------------------------------------------------------------------------
extern int index_GPS;

// -----------------------------------------------------------------------------
// NEW: boundary markers for Alpha logic
//
// alpha_gybe_index:
//   - set the instant we detect the jibe (alfa_counter++)
//   - this is the most "SBP-like" boundary for Alpha closure across runs
//
// alpha_run_start_index (optional):
//   - set when TIME_DELAY_NEW_RUN expires (run_counter++)
//   - use this if you want boundary aligned with your run_counter semantics
// -----------------------------------------------------------------------------
volatile int alpha_gybe_index      = -1;
volatile int alpha_run_start_index = -1;

// ============================================================================
// New_run_detection
// ============================================================================

int New_run_detection(float actual_heading, float S2_speed)
{
    #define SPEED_DETECTION_MIN       4000
    #define STANDSTILL_DETECTION_MAX  1000
    #define MEAN_HEADING_TIME         15
    #define STRAIGHT_COURSE_MAX_DEV   10
    #define JIBE_COURSE_DEVIATION_MIN 50

    static float old_heading,delta_heading,heading;
    static uint32_t delay_counter;
    static int run_counter;
    static bool velocity_0=false, velocity_5=false;
    static bool straight_course;

    if((actual_heading-old_heading)>300)  delta_heading-=360;
    if((actual_heading-old_heading)<-300) delta_heading+=360;
    old_heading=actual_heading;
    heading=actual_heading+delta_heading;

    heading_SD=heading;

    Mean_heading =
        Mean_heading*(MEAN_HEADING_TIME*systemInfo.sample_rate-1) /
        (MEAN_HEADING_TIME*systemInfo.sample_rate)
        + heading/(MEAN_HEADING_TIME*systemInfo.sample_rate);

    if(S2_speed>SPEED_DETECTION_MIN) velocity_5=true;
    if(S2_speed<STANDSTILL_DETECTION_MAX && velocity_5) velocity_0=true;

    if(velocity_0 && S2_speed>SPEED_DETECTION_MIN){
        velocity_0=false; velocity_5=false;
        delay_counter=(TIME_DELAY_NEW_RUN-1)*systemInfo.sample_rate;
    }

    if(abs(Mean_heading-heading)<STRAIGHT_COURSE_MAX_DEV && S2_speed>SPEED_DETECTION_MIN)
        straight_course=true;

    // -------------------------------------------------------------------------
    // JIBE DETECTED:
    // This is the "boundary" Alpha must straddle (previous run -> next run).
    // Record the sample index NOW.
    // -------------------------------------------------------------------------
    if(abs(Mean_heading-heading)>JIBE_COURSE_DEVIATION_MIN && straight_course){
        straight_course=false;
        delay_counter=0;

        alfa_counter++;               // notify alpha logic (existing)
        alpha_gybe_index = index_GPS; // NEW: boundary index at moment of jibe
    }

    delay_counter++;

    // -------------------------------------------------------------------------
    // NEW RUN STARTS (delayed boundary):
    // Optional but useful for debugging and for "run_count boundary" semantics.
    // -------------------------------------------------------------------------
    if(delay_counter==TIME_DELAY_NEW_RUN*systemInfo.sample_rate){
        run_counter++;
        alpha_run_start_index = index_GPS; // NEW: boundary index when run flips
    }

    return run_counter;
}
