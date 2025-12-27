#include <Arduino.h>
#include "ESP_functions.h"
#include "E_paper.h"
#include "config_manager.h"

#include "screen_system.h"
#include "rtc_state.h"
#include "Definitions.h"



char Ublox_type[20]="Ublox unknown...";
char TimeZone[64] ="GMT0";
int sdTrouble=0;


bool Wifi_on=true;


bool reset_boot = false;
int NTP_time_set = 0;
int Gps_time_set = 0;
bool Shut_down_Save_session = false;

extern bool downloading_file;

int GPS_OK = 0;
int analog_bat;
int first_fix_GPS,run_count,old_run_count,stat_count,GPS_delay;
int start_logging_millis;
int wifi_search=10;
int ftpStatus=0;
int last_gps_msg=0;
int nav_pvt_message=0;
int old_message=0;
int nav_sat_message=0;
int next_gpy_full_frame=0;
int msgType;
int GPIO12_screen=0;//keuze welk scherm
int low_bat_count;
int gps_speed;
int S10_previous_run;
float alfa_window;
float analog_mean=2000;
float Mean_heading,heading_SD;
int wdt_task0,wdt_task1;
int max_count_wdt_task0;
int freeSpace;







/*
void Shut_down(void){
        Ublox_off();
        GPS_Signal_OK=false;
        GPS_delay=0;
        if(Time_Set_OK){    //Only safe to RTC memory if new GPS data is available !!
            Time_Set_OK=false;
            RTC_distance=Ublox.total_distance/1000000;
            RTC_max_2s= S2.avg_speed[9]*calibration_speed;
            RTC_avg_10s=S10.avg_5runs*calibration_speed;
            RTC_mile=M1852.display_speed[9]*calibration_speed;
            RTC_alp=A500.display_max_speed*calibration_speed;
            RTC_1h=S3600.display_max_speed*calibration_speed; 
            RTC_500m=M500.avg_speed[9]*calibration_speed;

            RTC_max_2s_knots= S2.avg_speed[9]*1.9438/1000;
            RTC_avg_10s_knots=S10.avg_5runs*1.9438/1000;
            RTC_1h_knots=S3600.display_speed[9]*1.9438/1000;               
            RTC_mile_knots=M1852.display_speed[9]*1.9438/1000;
            RTC_alp_knots=A500.display_max_speed*1.9438/1000;
            
            RTC_R1_10s=S10.avg_speed[9]*calibration_speed;
            RTC_R2_10s=S10.avg_speed[8]*calibration_speed;
            RTC_R3_10s=S10.avg_speed[7]*calibration_speed;
            RTC_R4_10s=S10.avg_speed[6]*calibration_speed;
            RTC_R5_10s=S10.avg_speed[5]*calibration_speed;

            RTC_year=(tmstruct.tm_year+1900);//local time is corrected with timezone in close_files() !!
            RTC_month=(tmstruct.tm_mon+1);
            RTC_day=(tmstruct.tm_mday);
            RTC_hour=(tmstruct.tm_hour);
            RTC_min=(tmstruct.tm_min);

            char gpstc_post[2000]="";
            GPSTC_info(gpstc_post);
            if(config.logTXT){
              Session_info(Ublox);
              Session_results_S(S2);
              Session_results_S(S10);
              Session_results_S(S1800);
              Session_results_S(S3600);
              Session_results_M(M100);
              Session_results_M(M500);
              Session_results_M(M1852);
              Session_results_Alfa(A250,M250);
              Session_results_Alfa(A500,M500);
              Session_gpstc(gpstc_post);
              }
            //delay(3000);// go to sleep screen need some time...
            Close_files();  
            delay(500);//jh test lost files
            }
   
        go_to_sleep(TIME_TO_SLEEP,1);//got to sleep after 5 s, this to prevent booting when GPIO39 is still low !     
}

*/

/*
void GPSTC_info(char *GPSTC_post) {
  char tekst[160] ="<html><hr><h2>GPS Team Challenge Category Results:</h2>\r";
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<h3>Date: %d-%d-%d</h3>\r",RTC_year,RTC_month,RTC_day);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<table cellpadding=\"10\" style=\"text-align: right;\"><tr style=\"text-align: center;\"><th>Category</th><th>Speed (kn)</th><th>Speed (km/h)</th></tr>\r");
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<tr><td>2 sec</td><td><b>%.3f  </b></td><td>%.3f</td></tr>\r",RTC_max_2s_knots,RTC_max_2s);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<tr><td>5*10 sec</td><td><b>%.3f  </b></td><td>%.3f</td></tr>\r",RTC_avg_10s_knots,RTC_avg_10s);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<tr><td>1 hour</td><td><b>%.3f  </b></td><td>%.3f</td></tr>\r",RTC_1h_knots,RTC_1h);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<tr><td>alfa500</td><td><b>%.3f  </b></td><td>%.3f</td></tr>\r",RTC_alp_knots,RTC_alp);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<tr><td>1852 m</td><td><b>%.3f  </b></td><td>%.3f</td></tr>\r",RTC_mile_knots,RTC_mile);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<tr><td>Distance</td><td><b>%.3f  </b></td><td>km</td></tr>\r",RTC_distance);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<form method=\"POST\" action=\"https://gpsteamchallenge.com.au/sailor_session/post\"><input type=\"hidden\" name=\"load_from_post\" value=\"true\">");
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"date\" value=\"%d-%02d-%02d\">",RTC_year,RTC_month,RTC_day);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"2_sec_peak\" value=\"%.3f\">",RTC_max_2s_knots);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"2_sec_peak_calc_method\" value=\"D\">");
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"5x10\" value=\"%.3f\">",RTC_avg_10s_knots);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"5x10_calc_method\" value=\"D\">");
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"1_hour\" value=\"%.3f\">",RTC_1h_knots);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"1_hour_calc_method\" value=\"D\">");
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"alpha_500\" value=\"%.3f\">",RTC_alp_knots);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"alpha_500_calc_method\" value=\"D\">");
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"nautical_mile\" value=\"%.3f\">",RTC_mile_knots);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"nautical_mile_calc_method\" value=\"D\">");
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"distance\" value=\"%.3f\">",RTC_distance);
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"hidden\" name=\"distane_calc_method\" value=\"D\">");
  strcat(GPSTC_post, tekst);
  sprintf(tekst,"<input type=\"submit\" name=\"Submit\" value=\"Submit this session to the GPS Team Challenge website\"></form>\r</html>");
  strcat(GPSTC_post, tekst);
}
*/


void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.print("NTP Time = ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}  


