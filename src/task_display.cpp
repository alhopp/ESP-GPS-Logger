#include "task_display.h"

#include "Ublox.h"
#include "SD_card.h"
#include "ESP_functions.h"
#include "E_paper.h"

// extern globals used inside taskTwo
extern bool sleep_mode;



 
void taskTwo( void * parameter)
{
  while(true){ 
    wdt_task1=millis();
    if(config.Stat_screens_time!=0)stat_count++;//alleen auto switch stat screen als time>0 !!
    if (stat_count>config.screen_count)stat_count=0;//screen_count = 2
    Update_bat();
    if(RTC_voltage_bat<RTC_minimum_voltage_bat) low_bat_count++;
    else low_bat_count=0;
    if(sleep_mode==true){
        Ublox_off();
        Off_screen(RTC_OFF_screen);
        Serial.println("RTC_OFF_screen");
        delay(2000);
        Shut_down();
        delay(100);
        vTaskDelete(NULL);//to avoid that screen get new updates !!!!
    }
    else if(low_bat_count>10){
        sleep_mode=true;
        char tekst[32] = "";
        sprintf(tekst, "Shutdown low bat  @ %.1f V\n",RTC_minimum_voltage_bat);
        logERR(tekst);
        Off_screen(2);//off screen with "shutdown low bat"
        delay(2000);
        Shut_down();
        delay(100);
        vTaskDelete(NULL);//to avoid that screen get new updates !!!!
    }
    else if(millis()<2000)Update_screen(BOOT_SCREEN);
    else if(trouble_screen) Update_screen(TROUBLE);
    else if(GPS_Signal_OK==false) Update_screen(WIFI_ON);
    else if(Time_Set_OK==false) Update_screen(WIFI_ON);
    #if defined (GPIO12_ACTIF)
    else if(Short_push12.long_pulse){Update_screen(config.gpio12_screen[GPIO12_screen]);}//heeft voorrang, na drukken GPIO_pin 12, 10 STAT4 scherm !!!
    #endif
    
    else if((gps_speed/1000.0f<config.stat_speed)&&(Field_choice==false)){
          Update_screen(config.stat_screen[stat_count]);
          }
    else {
          Update_screen(SPEED);
          if(config.Stat_screens_time!=0)stat_count=0;
          }
  }
}