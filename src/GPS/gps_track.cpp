#include <stdint.h>
#include <math.h>

#include "Core/Definitions.h"      // TIME_DELAY_NEW_RUN
#include "core/system_info.h"     // systemInfo
#include "Core/Globals.h"         // heading_SD, Mean_heading, run_count, etc
#include "Ublox/ublox.h"

#include "GPS/GPS_data.h"         // alfa_counter
#include "GPS/gps_utils.h"
#include "GPS/gps_track.h"
#include "GPS/gps_geometry.h"

// -----------------------------------------------------------------------------
// Global track instance (500 m speed course)
// -----------------------------------------------------------------------------
GPS_Track M_500;

/* =============================================================================
 * GPS_Track
 *
 * Fixed-distance track timing (e.g. 500 m speed run).
 *
 * - Two virtual lines define the course (start + end)
 * - Crossing start line arms the run
 * - Crossing end line closes the run and computes speed
 * - Time comes from UBX iTOW, distance is theoretical
 *
 * Uses:
 * - Global NAV-PVT state
 * - Geometry helpers (point–line distance)
 * =============================================================================
 */

GPS_Track::GPS_Track(){}

// -----------------------------------------------------------------------------
// Set_course
// Orders start/end lines consistently and stores geometry.
// -----------------------------------------------------------------------------
void GPS_Track::Set_course(double lon_1,double lat_1,double lon_2,double lat_2,
                           double lon_3,double lat_3,double lon_4,double lat_4,
                           int distance)
{
    const double ml=(lat_1+lat_3)/2, mn=(lon_1+lon_3)/2;

    // Start line orientation
    if(Dis_point_line(mn,ml,lon_1,lat_1,lon_2,lat_2)>0){
        lon1=lon_1; lat1=lat_1; lon2=lon_2; lat2=lat_2;
    }else{
        lon1=lon_2; lat1=lat_2; lon2=lon_1; lat2=lat_1;
    }

    // End line orientation
    if(Dis_point_line(mn,ml,lon_3,lat_3,lon_4,lat_4)<0){
        lon3=lon_3; lat3=lat_3; lon4=lon_4; lat4=lat_4;
    }else{
        lon3=lon_4; lat3=lat_4; lon4=lon_3; lat4=lat_3;
    }

    theoretical_track_distance = distance;
    distance_p1p3 = afstandPunten(lon1,lat1,lon3,lat3);
    distance_p2p4 = afstandPunten(lon2,lat2,lon4,lat4);
}

// -----------------------------------------------------------------------------
// Update_Track
// Detects line crossings and finalises a timed run.
// -----------------------------------------------------------------------------
float GPS_Track::Update_Track()
{
    // Start line
    distance_startline = Dis_point_line(
        ubxMessage.navPvt.lon/1e7, ubxMessage.navPvt.lat/1e7,
        lon1,lat1,lon2,lat2);

    if(distance_startline>0 && Old_distance_start<0){
        getLocalTime(&tmstruct,0);
        Start_lon=ubxMessage.navPvt.lon/1e7;
        Start_lat=ubxMessage.navPvt.lat/1e7;
        Start_iTOW_ms=ubxMessage.navPvt.iTOW;
        Run_started=true;
    }
    Old_distance_start = distance_startline;

    // End line
    distance_endline = Dis_point_line(
        ubxMessage.navPvt.lon/1e7, ubxMessage.navPvt.lat/1e7,
        lon3,lat3,lon4,lat4);

    if(distance_endline>0 && Old_distance_end<0 && Run_started){
        getLocalTime(&tmstruct,0);
        End_lon=ubxMessage.navPvt.lon/1e7;
        End_lat=ubxMessage.navPvt.lat/1e7;

        track_distance = afstandPunten(Start_lon,Start_lat,End_lon,End_lat);
        Track_time_ms  = ubxMessage.navPvt.iTOW - Start_iTOW_ms;
        Track_speed    = theoretical_track_distance*1000.0f/Track_time_ms*systemInfo.cal_speed;

        time_hour[0]=tmstruct.tm_hour;
        time_min [0]=tmstruct.tm_min;
        time_sec [0]=tmstruct.tm_sec;
        avg_speed[0]=track_distance*1000.0f/Track_time_ms;

        Run_started=false;
        sort_run(avg_speed,time_hour,time_min,time_sec,
                 dummy,dummy,dummy,dummy,dummy_int,10);
    }

    Old_distance_end = distance_endline;
    return distance_endline;
}

// ============================================================================
// New_run_detection
//
// Detects new runs using:
// - Standstill → speed-up
// - Large heading change (jibe)
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

    // Heading unwrap
    if((actual_heading-old_heading)>300)  delta_heading-=360;
    if((actual_heading-old_heading)<-300) delta_heading+=360;
    old_heading=actual_heading;
    heading=actual_heading+delta_heading;

    heading_SD=heading;

    // Mean heading (low-pass)
    Mean_heading =
        Mean_heading*(MEAN_HEADING_TIME*systemInfo.sample_rate-1) /
        (MEAN_HEADING_TIME*systemInfo.sample_rate)
        + heading/(MEAN_HEADING_TIME*systemInfo.sample_rate);

    // Standstill detection
    if(S2_speed>SPEED_DETECTION_MIN) velocity_5=true;
    if(S2_speed<STANDSTILL_DETECTION_MAX && velocity_5) velocity_0=true;

    if(velocity_0 && S2_speed>SPEED_DETECTION_MIN){
        velocity_0=false; velocity_5=false;
        delay_counter=(TIME_DELAY_NEW_RUN-1)*systemInfo.sample_rate;
    }

    // Jibe detection
    if(abs(Mean_heading-heading)<STRAIGHT_COURSE_MAX_DEV && S2_speed>SPEED_DETECTION_MIN)
        straight_course=true;

    if(abs(Mean_heading-heading)>JIBE_COURSE_DEVIATION_MIN && straight_course){
        straight_course=false;
        delay_counter=0;
        alfa_counter++;      // notify alpha logic
    }

    delay_counter++;
    if(delay_counter==TIME_DELAY_NEW_RUN*systemInfo.sample_rate)
        run_counter++;

    return run_counter;
}
