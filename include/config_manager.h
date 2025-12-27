#pragma once

void initConfig();
void ensureConfigExistsOnSD();
void saveConfig();
void TimeZone_env (float timezone);

struct Config {
  float cal_bat=1.74;//calibration for read out bat voltage
  float shutdown_voltage=3.2;
  float cal_speed=3.6;//conversion m/s to km/h, for knots use 1.944
  int sample_rate=5;//gps_rate in Hz, 1, 5 or 10Hz !!!
  int gnss=3;//default setting 2 GNSS, GPS & GLONAS
  int field=1;//choice for first field in speed screen !!!
  int field_actual=1;//actual choice in speed screen
  int speed_large_font=1;//fonts on the first line are bigger, actual speed font is smaller
  int dynamic_model=0;//choice for dynamic model "Sea",if 0 model "portable" is used !!
  float timezone=1;//choice for timedifference in hours with UTC, for Belgium 1 or 2 (summertime)
  bool timezone_DST=1;//auto switch to summertime (daylightsaving)
  int Stat_screens=123;//choice for stats field when no speed, here stat_screen 1, 2 and 3 will be active
  int Stat_screens_time=4;//time between switching stat_screens
  int GPIO12_screens=54;//choice for stats field when gpio12 is activated (pull-up high, low = active)
  //int Stat_screens_persist=123;//choice for stats field when no speed, here stat_screen 1, 2 and 3 will be active / for resave the config
  //int GPIO12_screens_persist=54;//choice for stats field when gpio12 is activated (pull-up high, low = active) / for resave the config
  int Board_Logo=1;
  int Sail_Logo=1;
  char stat_screen[22]="167";//which stat_screen you want to see ?
  char gpio12_screen[10];//which stat_screen when gpio 12 toggles ?
  char speed_screen[10];//which speed fields are selected ?
  int screen_count=0;
  int gpio12_count=0;
  int speed_count=0;
  int sleep_off_screen=11;
  int stat_speed=1;//max speed in m/s for showing Stat screens
  int start_logging_speed=1;
  int bar_length=1852;//choice for bar indicator for length of run in m (nautical mile)
  int archive_days=10; //how many days files will be moved to the "Archive" dir
  bool bat_choice=1;//choice for voltage in % or voltage
  bool logTXT=1;// switchinf off .txt files
  bool logUBX=1;//log to .ubx
  bool logUBX_nav_sat=0;// log nav sat msg to .ubx
  bool logSBP=1;//log to .sbp
  bool logGPY=1;//log to .gps
  bool logGPX=0;//log to .gpx
  int file_date_time=2;//type of filenaming, with MAC adress or datetime
  char UBXfile[32]="My_ESP_GPS";//your preferred filename
  char Sleep_info[32]="Your ID";//your preferred sleep text
  char ssid[32]="My_SSID";//your SSID
  char password[32]="password";//your password
  char ssid2[32]="ESP_GPS";//your SSID
  char password2[32]="password2";//your password
  int config_fail=0;
  uint8_t ublox_type=0;
  uint8_t M10_high_nav=0;
  int cpu_freq = 80;
  double p1_lon,p1_lat,p2_lon,p2_lat, p3_lon,p3_lat,p4_lon,p4_lat;
  int track_distance;
  } ;
extern Config config;

void loadConfiguration(const char *filename,
                       const char *filename_backup,
                       Config &config) ;



