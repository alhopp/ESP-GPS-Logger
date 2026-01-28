
#include <stdint.h>
#include <math.h>
#include "Core/Definitions.h"     // TIME_DELAY_NEW_RUN
#include "core/system_info.h"    // systemInfo
#include "Core/Globals.h"        // heading_SD, Mean_heading, run_count, etc
#include "GPS/GPS_data.h"        // alfa_counter



int New_run_detection(float actual_heading, float S2_speed){
   /*Berekening van de gemiddelde heading over de laatste 10s************************************************************************/
   #define SPEED_DETECTION_MIN 4000       //min average speed over 2s for new run detection (mm/s)
   #define STANDSTILL_DETECTION_MAX 1000  //max average speed over 2s voor stand still detection (mm/s)
   #define MEAN_HEADING_TIME 15           //tijd in s voor berekening gemiddelde heading
   #define STRAIGHT_COURSE_MAX_DEV 10     //max hoek afwijking voor rechtdoor herkenning (graden)
   #define JIBE_COURSE_DEVIATION_MIN 50   //min hoek afwijking voor dettectie jibe (graden)

   static float old_heading,delta_heading,heading;
   static uint32_t delay_counter;
   static int run_counter;
   static bool velocity_0 = false;
   static bool velocity_5 = false;
   int speed_detection_min=SPEED_DETECTION_MIN;//minimum snelheid 4m/s (14 km/h)voor snelheid display
   int standstill_detection_max=STANDSTILL_DETECTION_MAX;//maximum snelheid 1 m/s (3.6 km/h) voor stilstand herkenning, was 1.5 m/s change SW5.51
   //float headAcc=ubxMessage.navPvt.headingAcc/100000.0f;  //heading Accuracy wordt niet gebruikt ???  
   //actual_heading=ubxMessage.navPvt.heading/100000.0f;
   if((actual_heading-old_heading)>300.0f) delta_heading=delta_heading-360.0f;
   if((actual_heading-old_heading)<-300.0f) delta_heading=delta_heading+360.0f;
   old_heading=actual_heading;
   heading=actual_heading+delta_heading;
   /*detectie heading change over 15s is more then 40°, nieuwe run wordt gestart !!***************************************************************************/
   int mean_heading_time=MEAN_HEADING_TIME;//tijd in s voor berekening gemiddelde heading
   int straight_course_max=STRAIGHT_COURSE_MAX_DEV;//max hoek afwijking voor rechtdoor herkenning
   int course_deviation_min=JIBE_COURSE_DEVIATION_MIN;//min hoek afwijking om gijp te detecteren, was 40
   int time_delay_new_run=TIME_DELAY_NEW_RUN;//vertraging om nieuwe run te starten, sw 4.59
   heading_SD=heading;
   Mean_heading=Mean_heading*(mean_heading_time*systemInfo.sample_rate-1)/(mean_heading_time*systemInfo.sample_rate)+heading/(mean_heading_time*systemInfo.sample_rate);
   /*detection stand still, more then 2s with velocity<1m/s**************************************************************************************************/
   if(S2_speed>speed_detection_min)velocity_5=1;    //snelheid was hoger dan 4m/s        
   if((S2_speed<standstill_detection_max)&&(velocity_5==1))velocity_0=1;//snelheid is kleiner dan 1m/s
   //else velocity_0=0;
   /*Nieuwe run gedetecteerd omwille stilstand **********************************************************************************************************************/
   if((velocity_0==1)&&(S2_speed>speed_detection_min)){
     velocity_5=0;
     velocity_0=0;
     delay_counter=(time_delay_new_run-1)*systemInfo.sample_rate;//delay only 1 s after standstill + speed> min speed !!
    }
   /*Nieuwe run gedetecteerd omwille heading change*****************************************************************************************************************/
   static bool straight_course;
   //if(abs(Mean_heading-heading)<straight_course_max){straight_course=true;}//stabiele koers terug bereikt
   if((abs(Mean_heading-heading)<straight_course_max)&&(S2_speed>speed_detection_min)){straight_course=true;}//stabiele koers terug bereikt, added min_speed SW5.51
   if(((abs(Mean_heading-heading)>course_deviation_min)&&(straight_course==true))){      
      straight_course=false;
      delay_counter=0;
      alfa_counter++;//jibe detection for alfa_indicator ....
      }
   delay_counter++;   
   if(delay_counter==(time_delay_new_run*systemInfo.sample_rate)) run_counter++;   
   return run_counter;   
}
