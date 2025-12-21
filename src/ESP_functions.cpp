#include <Arduino.h>
#include "ESP_functions.h"
#include "E_paper.h"


String IP_adress="0.0.0.0";
const char SW_version[16]="Ver 6.01c";


extern RTC_DATA_ATTR int RTC_Sail_Logo ;
extern RTC_DATA_ATTR char RTC_Sleep_txt[32];

const char *filename = "/config.txt";
const char *filename_backup = "/config_backup.txt";


char Ublox_type[20]="Ublox unknown...";
char TimeZone[64] ="GMT0";
int sdTrouble=0;
bool sdOK = false;
bool button = false;
bool LITTLEFS_OK;
bool reed = false;
bool deep_sleep = false;
bool Wifi_on=true;
bool SoftAP_connection = false;
bool GPS_Signal_OK = false;
bool Field_choice = false;
bool reset_boot = false;
int NTP_time_set = 0;
int Gps_time_set = 0;
bool Shut_down_Save_session = false;
bool trouble_screen = false;
extern bool downloading_file;
bool ap_mode = false;
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
float Afstand_lijn=0;
float Afstand_lijn2=0;
float Afstand_sec=61.61;
float Afstand_sec2=71.71;
float Afstand_gps=0;
*/
String actual_ssid="_ssid_";
 /* variables to hold instances of tasks*/
//TaskHandle_t t1 = NULL;
//TaskHandle_t t2 = NULL;
byte mac[6];  //unique mac adress of esp32
IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

RTC_DATA_ATTR float calibration_speed=3.6;
RTC_DATA_ATTR int offset = 0;
RTC_DATA_ATTR float RTC_distance;
RTC_DATA_ATTR float RTC_avg_10s;
RTC_DATA_ATTR float RTC_max_2s;
RTC_DATA_ATTR float RTC_1h;
RTC_DATA_ATTR float RTC_alp;
RTC_DATA_ATTR float RTC_mile;
RTC_DATA_ATTR float RTC_avg_10s_knots;
RTC_DATA_ATTR float RTC_max_2s_knots;
RTC_DATA_ATTR float RTC_alp_knots;
RTC_DATA_ATTR float RTC_1h_knots;
RTC_DATA_ATTR float RTC_mile_knots;
//Simon
RTC_DATA_ATTR short RTC_year;
RTC_DATA_ATTR short RTC_month;
RTC_DATA_ATTR short RTC_day;
RTC_DATA_ATTR short RTC_hour;
RTC_DATA_ATTR short RTC_min;
RTC_DATA_ATTR float RTC_500m;

RTC_DATA_ATTR float RTC_R1_10s;
RTC_DATA_ATTR float RTC_R2_10s;
RTC_DATA_ATTR float RTC_R3_10s;
RTC_DATA_ATTR float RTC_R4_10s;
RTC_DATA_ATTR float RTC_R5_10s;
RTC_DATA_ATTR int RTC_Board_Logo;
RTC_DATA_ATTR int RTC_SLEEP_screen=0;
RTC_DATA_ATTR int RTC_OFF_screen=0;
RTC_DATA_ATTR int RTC_counter=0;

RTC_DATA_ATTR float RTC_calibration_bat; //t=1.75;
RTC_DATA_ATTR float RTC_voltage_bat=3.6;
RTC_DATA_ATTR float RTC_old_voltage_bat=3.6;
RTC_DATA_ATTR float RTC_minimum_voltage_bat=MINIMUM_VOLTAGE;
RTC_DATA_ATTR int RTC_bat_choice = 0;
RTC_DATA_ATTR int RTC_highest_read = STARTVALUE_HIGHEST_READ;


Button_push::Button_push(int GPIO_pin,
                         int push_time,
                         int long_pulse_time,
                         int max_count,
                         bool default_state)
{
    Input_pin = GPIO_pin;
    Default_state = default_state;
    time_out_millis = push_time;
    max_pulse_time = long_pulse_time;
    max_button_count = max_count;
}


FtpServer ftpSrv;  
GPS_data Ublox; // create an object storing GPS_data !
GPS_SAT_info Ublox_Sat;//create an object storing GPS_SAT info !
GPS_speed M100(100);
GPS_speed M250(250);
GPS_speed M500(500);
GPS_speed M1852(1852);
GPS_time S2(2);
GPS_time s2(2);
GPS_time S10(10);
GPS_time s10(10);//for  stats GPIO_12 screens, reset possible !!
GPS_time S1800(1800);
GPS_time S3600(3600);
Alfa_speed A250(50);
Alfa_speed A500(50);
Alfa_speed a500(50);//for  Alfa stats GPIO_12 screens, reset possible !!
GPS_Track M_500;

SPIClass sdSPI(VSPI);//was VSPI

void go_to_sleep(uint64_t sleep_time,bool refresh_screen);
void Update_bat(void);
void taskOne( void * parameter );
void taskTwo( void * parameter);  
void GPSTC_info(char* GPSTC_post );

  /*
Method to print the reason by which ESP32 has been awaken from sleep
*/
void print_wakeup_reason(){
  int sleeping_time= TIME_TO_SLEEP;                                                
  analog_mean = analogRead(PIN_BAT);
  for(int i=0;i<50;i++){Update_bat();}  
  analog_bat = analog_mean;
  RTC_voltage_bat=analog_mean*RTC_calibration_bat/1000;
  Serial.print("Battery voltage = ");
  Serial.println(RTC_voltage_bat);
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {  
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO");
                                 pinMode(HOLD_PIN, OUTPUT);
                                 digitalWrite(HOLD_PIN,HIGH);
                                 reset_boot=false; 
                                 pinMode(GO_TO_SLEEP_GPIO,INPUT_PULLUP);
                                 while(millis()<200){ //minimal puls length = 200 ms !!!
                                    if (digitalRead(GO_TO_SLEEP_GPIO)==1){
                                      go_to_sleep(TIME_TO_SLEEP,0);
                                      break;
                                      }
                                    }
                                 rtc_gpio_deinit(WAKE_UP_GPIO_NUM);//was 39   
                                 reed=1;   
                                // esp_sleep_disableRTC_minimum_voltage_batESP_SLEEP_WAKEUP_ALL);
                                 if(RTC_voltage_bat<RTC_minimum_voltage_bat){
                                      Boot_screen();
                                      sleeping_time=40000;//sleep much longer
                                      go_to_sleep(sleeping_time,1); //was 4000
                                    }
                                 Boot_screen();
                                 break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); 
                                 break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer");                                 
                                  if((int)analog_mean>(RTC_highest_read+TOLERANCE)){ 
                                    RTC_highest_read=(int)analog_mean; 
                                    EEPROM.writeInt(2,RTC_highest_read) ;
                                    EEPROM.commit();
                                    RTC_calibration_bat= FULLY_CHARGED_LIPO_VOLTAGE/RTC_highest_read;
                                    Serial.print("New RTC_highest_read = ");
                                    Serial.println(RTC_highest_read);
                                    }   
                                  if(abs(RTC_voltage_bat-RTC_old_voltage_bat)>MINIMUM_VOLTAGE_CHANGE){
                                    Sleep_screen(RTC_SLEEP_screen);
                                    display.powerOff();
                                   // delay(500);
                                    //digitalWrite(HOLD_PIN,LOW);
                                    RTC_old_voltage_bat=RTC_voltage_bat;
                                    }
                                  if(RTC_voltage_bat<RTC_minimum_voltage_bat){
                                      Boot_screen();
                                      display.powerOff();
                                      delay(100);
                                      pinMode(GO_TO_SLEEP_GPIO,INPUT_PULLUP);
                                      esp_sleep_enable_ext0_wakeup(WAKE_UP_GPIO_NUM,0);
                                      esp_deep_sleep_start();//sleep forever.....
                                    }
                                  go_to_sleep(sleeping_time,0); //was 4000
                                  break;                               
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); 
                                     break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); 
                                break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason);
              pinMode(HOLD_PIN, OUTPUT);
              digitalWrite(HOLD_PIN,HIGH);
              break;
    }
}
void print_reset_reason(int reason)
{
  switch ( reason)
  {

     case 5 : Serial.println ("DEEPSLEEP_RESET");break;        //<5,  Deep Sleep reset digital core
    case 12 : Serial.println ("SW_CPU_RESET");break;          /**<12, Software reset CPU*/        
    default : Serial.println ("NO_MEAN, always back to sleep after bootscreen()!!!");reset_boot=true; 
  }
}
void go_to_sleep(uint64_t sleep_time,bool refresh_screen){
  deep_sleep=true;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Ublox_off();
  Serial.println("Setup ESP32 to sleep for every " + String((int)sleep_time) + " Seconds");
  Serial.println("Going to sleep now");
  Serial.flush();
  if(reset_boot==false) delay(3000);//time needed for showing go to sleep screen
  pinMode(11, OUTPUT);
  digitalWrite(11, HIGH);//flash in deepsleep, CS stays HIGH!!
  //test Andres
 // SD_MMC.end();
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  if(refresh_screen==1){
    Sleep_screen(RTC_SLEEP_screen);
    delay(1000);
    display.powerOff();
    }
  digitalWrite(HOLD_PIN,LOW);  
  pinMode(GO_TO_SLEEP_GPIO,INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(WAKE_UP_GPIO_NUM,0);
  esp_sleep_enable_timer_wakeup( uS_TO_S_FACTOR*sleep_time);
  esp_deep_sleep(uS_TO_S_FACTOR*sleep_time);
}

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
void Update_bat(void){
    analog_bat = analogRead(PIN_BAT);
    analog_mean=analog_bat*0.02+analog_mean*0.98;
    RTC_voltage_bat=analog_mean*RTC_calibration_bat/1000;
}
void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.print("NTP Time = ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}  

void OnWiFiEvent(WiFiEvent_t event){
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("ESP32 Connected to SSID Station mode");
      WiFi.mode(WIFI_MODE_STA);//switch off softAP
      Serial.println("ESP32 Soft AP switched off");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:         //test
      Serial.println("ESP32 disconnected to WIFI");
      //SoftAP_connection=false;
      break;
    case SYSTEM_EVENT_STA_GOT_IP://  @this event no IP !!!         ARDUINO__EVENT_STA_CONNECTED:
      Serial.println("ESP32 Connected to WiFi Network");
      IP_adress =  WiFi.localIP().toString();
      break;
    case SYSTEM_EVENT_AP_START:
      WiFi.softAPConfig(local_IP, gateway, subnet);  
      Serial.println("ESP32 soft AP started");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("Station connected to ESP32 soft AP");
      IP_adress =  WiFi.softAPIP().toString();
      SoftAP_connection=true;
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("Station disconnected from ESP32 soft AP");
      SoftAP_connection=false;
      break;
    default: break;
  }
}

void Button_push::begin(int GPIO_pin,bool default_state){
  if(default_state==1){pinMode(GPIO_pin, INPUT_PULLUP);}
  if(default_state==0){pinMode(GPIO_pin, INPUT_PULLDOWN);}
  }
boolean Button_push::Button_pushed(void) {
  return_value = false;
  button_status = digitalRead(Input_pin);
  if (digitalRead(Input_pin) == Default_state) push_millis = millis();
  if (((millis() - push_millis) > time_out_millis) & (old_button_status == 0)) {
    button_count++;
    if (button_count > max_button_count) button_count = 0;
    old_button_status = 1;
    millis_10s = millis();
    //Serial.print ("Class button_count ");Serial.print(button_count);
    return_value = true;
  } else return_value = false;
  if ((millis() - millis_10s) < (1000 * max_pulse_time)) long_pulse = true;
  else long_pulse = false;
  if (digitalRead(Input_pin) == Default_state) old_button_status = 0;
  return return_value;
}

void Search_for_wifi(void) {
{

  return;

}
 // Legacy function — intentionally disabled in refactor

 
 // while ((WiFi.status() != WL_CONNECTED)&&(SoftAP_connection==false)){  
 //   if(Short_push39.Button_pushed()|Short_push19.Button_pushed()){ap_mode=true;yield();break;}
 //   Update_bat();        
 //   if(ap_mode==false)Update_screen(WIFI_STATION);
 //   else Update_screen(WIFI_SOFT_AP);
 //   Serial.print(".");
 //   wifi_search--;
 //   if(wifi_search<=0){
 //     IP_adress = "0.0.0.0";
 //     break;
 //     }
 //   }
} 
