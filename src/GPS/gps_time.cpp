#include "GPS/gps_time.h"
#include "GPS/GPS_data.h"

#include "Ublox/ublox.h"
#include "Core/Globals.h"
#include "core/system_info.h"

#include <time.h>

class GPS_time;

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
GPS_time::GPS_time(int tijdvenster)
: time_window(tijdvenster)
{
  Reset_stats();
}

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
void GPS_time::Reset_stats(void){
  for (int i=0;i<10;i++){
    avg_speed[i]=0;
    display_speed[i]=0;
  }
  avg_5runs=0;
}


float GPS_time::Update_speed(int actual_run){
  if(time_window*systemInfo.sample_rate<BUFFER_SIZE){      //indien tijdvenster kleiner is dan de sample_rate*BUFFER, normale buffer gebruiken
        avg_s_sum=avg_s_sum+_gSpeed[index_GPS%BUFFER_SIZE]; //altijd gSpeed optellen bij elke update
        if(index_GPS>=time_window*systemInfo.sample_rate){
            avg_s_sum=avg_s_sum-_gSpeed[(index_GPS-(time_window*systemInfo.sample_rate))%BUFFER_SIZE];//vanaf 10s bereikt, terug -10s aftrekken van som
            }
            avg_s=(double)avg_s_sum/time_window/systemInfo.sample_rate;
            if(s_max_speed<avg_s){
              s_max_speed=avg_s;
              speed_run[actual_run%NR_OF_BAR]=avg_s;
              getLocalTime(&tmstruct, 0);
              time_hour[0]=tmstruct.tm_hour;
              time_min[0]=tmstruct.tm_min;
              time_sec[0]=tmstruct.tm_sec;
              this_run[0]=actual_run;
              avg_speed[0]=s_max_speed; 
              Mean_cno[0]=Ublox_Sat.sat_info.Mean_mean_cno;
              Max_cno[0]=Ublox_Sat.sat_info.Mean_max_cno;
              Min_cno[0]=Ublox_Sat.sat_info.Mean_min_cno;
              Mean_numSat[0]=Ublox_Sat.sat_info.Mean_numSV;
              //Om de avg te actualiseren tijdens de run, gemiddelde berekenen gekopieerde array  !
              for(int i=0;i<10;i++){
                display_speed[i]=avg_speed[i];
              }
              //sort display_speed
              sort_display(display_speed,10);
              display_max_speed=display_speed[9];
              avg_5runs=0;
              for(int i=5;i<10;i++){
                avg_5runs=avg_5runs+display_speed[i];
              }
              avg_5runs=avg_5runs/5;
            }
            if((actual_run!=old_run)&&(this_run[0]==old_run)){          //sorting only if new max during this run !!!
              sort_run(avg_speed,time_hour,time_min,time_sec,Mean_cno,Max_cno,Min_cno,Mean_numSat,this_run,10);
              if(s_max_speed>5000)speed_run_counter ++;//changes SW5.51 min speed bar graph = 5 m/s
              speed_run[actual_run%NR_OF_BAR]=avg_speed[0];    //SW 5.5
              avg_speed[0]=0;
              s_max_speed=0;
              avg_5runs=0;
              for(int i=5;i<10;i++){
                    avg_5runs=avg_5runs+avg_speed[i];
                    }
                avg_5runs=avg_5runs/5;
              }
            if((actual_run!=reset_display_last_run)&&(avg_s>3000)){
              reset_display_last_run=actual_run;
              display_last_run=0;
              }
            else if(display_last_run<s_max_speed){
                display_last_run=s_max_speed;
                }    
            old_run=actual_run;
            return s_max_speed;
  }
  else if(index_GPS%systemInfo.sample_rate==0){        //overschakelen naar seconden buffer, maar één update/seconde !!
            avg_s_sum=avg_s_sum+(int)_secSpeed[index_sec%BUFFER_SIZE]; //_secSpeed[BUFFER_SIZE] en index_sec 
            if(index_sec>=time_window){
                avg_s_sum=avg_s_sum-(int)_secSpeed[(index_sec-time_window)%BUFFER_SIZE];//vanaf 10s bereikt, terug -10s aftrekken van som
                }
            avg_s=(double)avg_s_sum/time_window;//in de seconden array staat de gemiddelde van gspeed !!
            //Serial.print("avg_s ");Serial.println(avg_s);
            if(s_max_speed<avg_s){
                s_max_speed=avg_s;
                getLocalTime(&tmstruct, 0);
                time_hour[0]=tmstruct.tm_hour;
                time_min[0]=tmstruct.tm_min;
                time_sec[0]=tmstruct.tm_sec;
                this_run[0]=actual_run;
                avg_speed[0]=s_max_speed;   //s_max_speed niet resetten bij elke run !!!
                }
            if(s_max_speed>avg_speed[9])display_max_speed=s_max_speed;//update on the fly voor S1800 / S3600 
            else display_max_speed=avg_speed[9];
            if((actual_run!=old_run)&&(this_run[0]==old_run)){   //sorting only if new max during this run !!!
                  //sort_run(avg_speed,time_hour,time_min,time_sec,this_run,10);
                  sort_run(avg_speed,time_hour,time_min,time_sec,Mean_cno,Max_cno,Min_cno,Mean_numSat,this_run,10);
                  avg_speed[0]=0;
                  s_max_speed=0;
                  avg_5runs=0;
                  for(int i=5;i<10;i++){
                    avg_5runs=avg_5runs+avg_speed[i];
                    }
                  avg_5runs=avg_5runs/5;
                  }
            old_run=actual_run;
            return s_max_speed;
            } 
    //}
    return s_max_speed;//anders compiler waarschuwing control reaches end of non-void function [-Werror=return-type]
 }
