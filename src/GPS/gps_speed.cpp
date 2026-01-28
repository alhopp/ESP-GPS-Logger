#include "GPS/gps_speed.h"
#include "GPS/GPS_data.h"
#include "core/system_info.h"
#include "Ublox/ublox.h"
#include <time.h>
#include "Core/Globals.h"

#include "GPS/GPS_data.h"


/*Instantie om gemiddelde snelheid over een bepaalde afstand te bepalen, bij een nieuwe run opslaan hoogste snelheid van de vorige run*****************/
GPS_speed::GPS_speed(int afstand){
  m_set_distance=afstand;  
}



double GPS_speed::Update_distance(int actual_run){ 
  m_Set_Distance=m_set_distance*1000*systemInfo.sample_rate;//opgelet, m_set_distance moet nu in mm, dus *1000 + functie van sample_rate !! 
  m_distance=m_distance+_gSpeed[index_GPS%BUFFER_SIZE];//resolutie = 0.1 mm nu, 2,147,483,647 = 214748 m, dus maar 214 km !! 
  if((index_GPS-m_index)>=BUFFER_SIZE){     //controle buffer overflow
      m_distance=0;
      m_index=index_GPS;
      }
  if(m_distance>m_Set_Distance){          //buffer m_index van gewenste afstand bepalen
        while(m_distance>m_Set_Distance&&(index_GPS-m_index)<BUFFER_SIZE){    
              m_distance=m_distance-_gSpeed[m_index%BUFFER_SIZE];
              m_distance_alfa=m_distance;
              m_index++;
              }
        m_index--;    
        m_distance=m_distance+_gSpeed[m_index%BUFFER_SIZE];
        } 
  m_sample=index_GPS-m_index+1; //controle mogelijk van aantal samples
  //Protection divide by zero !!!
  if(index_GPS-m_index+1){
      m_speed=(double)m_distance/m_sample; //10 samples op 1s aan 10mm/s = 100/10 = 10 mm /s
  }
  if(index_GPS-m_index){
    m_speed_alfa=(double)m_distance_alfa/(index_GPS-m_index); 
    }   
  if(m_distance<m_Set_Distance) m_speed=0; //dit om foute snelheid te voorkomen indien afstand nog niet bereikt!!
  if(m_sample>=BUFFER_SIZE) m_speed=0; //dit om foute snelheid te voorkomen bij BUFFER_SIZE overflow !!
  if(m_speed==0) m_speed_alfa=0;
  if(m_max_speed<m_speed){
        m_max_speed=m_speed;
        getLocalTime(&tmstruct, 0);
        time_hour[0]=tmstruct.tm_hour;
        time_min[0]=tmstruct.tm_min;
        time_sec[0]=tmstruct.tm_sec;
        this_run[0]=actual_run;//om berekening te checken
        avg_speed[0]=m_max_speed; 
        m_Distance[0]=m_distance;
        nr_samples[0]=m_sample;
        message_nr[0]=nav_pvt_message;
        //Om de avg te actualiseren tijdens de run, gemiddelde berekenen gekopieerde array  !
        for(int i=0;i<10;i++){
          display_speed[i]=avg_speed[i];
          }
        sort_display(display_speed,10);
        }    
  if((actual_run!=old_run)&&(this_run[0]==old_run)){              //opslaan hoogste snelheid van run + sorteren
      sort_run_alfa(avg_speed,m_Distance,message_nr,time_hour,time_min,time_sec,this_run,nr_samples,10);
      avg_speed[0]=0;
      m_max_speed=0;
      }
  old_run=actual_run;
  return m_max_speed;
}







