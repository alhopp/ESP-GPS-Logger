
// -----------------------------------------------------------------------------
// Configuration Manager
//
// Loads / validates config from LittleFS (/config.txt), creates defaults if
// missing or invalid, applies derived runtime values, and heals fragile fields
// (notably speed_screen / gpio12_screen).
//
// Must run after storage init and before Wi-Fi, logging, or tasks.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "Definitions.h"
#include "config_manager.h"
#include "rtc_state.h"
#include "Globals.h"

// -----------------------------------------------------------------------------
// Config location (LittleFS only)
// -----------------------------------------------------------------------------
#define CONFIG_FILE "/config.txt"

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------
static void setDefaultConfig();
static bool loadConfigFromFile(File &file);
static void writeConfigToFile(File &file);
static void validateConfig();
static void sanitizeScreenString(char *dst,size_t dstSize,const char *src);
static void applyDerivedConfig();
static void dumpConfig();

// -----------------------------------------------------------------------------
// Global config instance
// -----------------------------------------------------------------------------
Config config;

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void initConfig()
{
  LOG_CONFIG("Init","Loading configuration");

  if(!LittleFS.begin(true)){ LOG_ERROR("CONFIG","LittleFS not mounted"); return; }

  // ---------------------------------------------------------------------------
  // No config → create defaults
  // ---------------------------------------------------------------------------
  if(!LittleFS.exists(CONFIG_FILE)){
    LOG_CONFIG("Config","No config found → creating default");
    setDefaultConfig();

    File f=LittleFS.open(CONFIG_FILE,FILE_WRITE);
    if(!f){ LOG_ERROR("CONFIG","Cannot create config.txt"); return; }

    writeConfigToFile(f); f.close();
    validateConfig(); applyDerivedConfig(); dumpConfig();
    return;
  }

  // ---------------------------------------------------------------------------
  // Load existing config
  // ---------------------------------------------------------------------------
  File f=LittleFS.open(CONFIG_FILE,FILE_READ);
  if(!f){
    LOG_ERROR("CONFIG","Failed to open config.txt");
    setDefaultConfig(); validateConfig(); applyDerivedConfig(); dumpConfig();
    return;
  }

  if(!loadConfigFromFile(f)){
    LOG_ERROR("CONFIG","Invalid config → reset defaults");
    f.close();

    setDefaultConfig();
    File fw=LittleFS.open(CONFIG_FILE,FILE_WRITE);
    if(fw){ writeConfigToFile(fw); fw.close(); }
  }else f.close();

  // ---------------------------------------------------------------------------
  // Heal + apply derived values
  // ---------------------------------------------------------------------------
  validateConfig();       
  applyDerivedConfig();

  LOG_CONFIG("Init","Configuration loaded");
  dumpConfig();
}

// -----------------------------------------------------------------------------
// Save config (called from Wi-Fi / UI)
// -----------------------------------------------------------------------------
void saveConfig()
{
  validateConfig(); // heal before write

  File f=LittleFS.open(CONFIG_FILE,FILE_WRITE);
  if(!f){ LOG_ERROR("CONFIG","Cannot save config"); return; }

  writeConfigToFile(f); f.close();
  LOG_CONFIG("Save","Configuration saved → dumping final state");
  dumpConfig();
}

// -----------------------------------------------------------------------------
// Defaults
// -----------------------------------------------------------------------------
static void setDefaultConfig()
{
  LOG_CONFIG("Defaults", "Applying defaults");

  // -------- Core numeric defaults --------
  config.cal_bat             = 1.75f;
  config.shutdown_voltage    = 3.2f;
  config.cal_speed           = 3.6f;
  config.sample_rate         = 5;
  config.cpu_freq            = 80;
  config.track_distance      = 1852;

  // -------- UI / behaviour --------
  config.bar_length          = 1852;
  config.Stat_screens        = 12;
  config.stat_speed          = 1;
  config.sleep_off_screen    = 11;

  // -------- System --------
  config.timezone            = 1.0f;
  config.timezone_DST        = 1;

  // -------- Logging --------
  config.logTXT              = 0;
  config.logUBX              = 0;
  config.logSBP              = 1;
  config.file_date_time      = 1;


  // -------- Critical screen strings (must never be empty) --------
  strlcpy(config.speed_screen,  "1",  sizeof(config.speed_screen));
  strlcpy(config.stat_screen,   "12", sizeof(config.stat_screen));
  strlcpy(config.gpio12_screen, "4",  sizeof(config.gpio12_screen));

  // -------- Other strings --------
  strlcpy(config.UBXfile,    "/ubxGPS",   sizeof(config.UBXfile));
  strlcpy(config.Sleep_info, "ESP32 GPS", sizeof(config.Sleep_info));
  strlcpy(config.ssid,       "",          sizeof(config.ssid));
  strlcpy(config.password,   "",          sizeof(config.password));
  strlcpy(config.ssid2,      "ESP32_GPS",  sizeof(config.ssid2));
  strlcpy(config.password2,  "",          sizeof(config.password2));
}


// -----------------------------------------------------------------------------
// Load from file
// -----------------------------------------------------------------------------
static bool loadConfigFromFile(File &file)
{
  StaticJsonDocument<1536> doc;
  if(deserializeJson(doc,file)){ LOG_ERROR("CONFIG","JSON parse failed"); return false; }

  // numeric / boolean
  config.cal_bat             =doc["cal_bat"]             |config.cal_bat;
  config.shutdown_voltage    =doc["shutdown_voltage"]    |config.shutdown_voltage;
  config.cal_speed           =doc["cal_speed"]           |config.cal_speed;
  config.sample_rate         =doc["sample_rate"]         |config.sample_rate;
  config.cpu_freq            =doc["cpu_freq"]            |config.cpu_freq;
  config.bar_length          =doc["bar_length"]          |config.bar_length;
  config.Stat_screens        =doc["Stat_screens"]        |config.Stat_screens;
  config.stat_speed          =doc["stat_speed"]          |config.stat_speed;
  config.sleep_off_screen    =doc["sleep_off_screen"]    |config.sleep_off_screen;
  config.logTXT              =doc["logTXT"]              |config.logTXT;
  config.logUBX              =doc["logUBX"]              |config.logUBX;
  config.logSBP              =doc["logSBP"]              |config.logSBP;
  config.file_date_time      =doc["file_date_time"]      |config.file_date_time;
  config.timezone            =doc["timezone"]            |config.timezone;
  config.timezone_DST        =doc["timezone_DST"]        |config.timezone_DST;
  config.track_distance      =doc["track_distance"]      |config.track_distance;

  // strings (only if non-empty)
  const char* s;
  if((s=doc["speed_screen"])   && s[0]) sanitizeScreenString(config.speed_screen,sizeof(config.speed_screen),s);
  if((s=doc["stat_screen"])    && s[0]) strlcpy(config.stat_screen,s,sizeof(config.stat_screen));
  if((s=doc["gpio12_screen"])  && s[0]) sanitizeScreenString(config.gpio12_screen,sizeof(config.gpio12_screen),s);
  if((s=doc["Sleep_info"])     && s[0]) strlcpy(config.Sleep_info,s,sizeof(config.Sleep_info));
  if((s=doc["UBXfile"])        && s[0]) strlcpy(config.UBXfile,s,sizeof(config.UBXfile));

  return true;
}

// -----------------------------------------------------------------------------
// Write config to file
// -----------------------------------------------------------------------------
static void writeConfigToFile(File &file)
{
  StaticJsonDocument<1536> doc;

  // numeric / boolean
  doc["cal_bat"]=config.cal_bat;           doc["shutdown_voltage"]=config.shutdown_voltage;
  doc["cal_speed"]=config.cal_speed;       doc["sample_rate"]=config.sample_rate;
  doc["cpu_freq"]=config.cpu_freq;           doc["bar_length"]=config.bar_length;   
    doc["Stat_screens"]=config.Stat_screens;  
  doc["stat_speed"]=config.stat_speed;     
  doc["sleep_off_screen"]=config.sleep_off_screen;
  doc["logTXT"]=config.logTXT;             doc["logUBX"]=config.logUBX;
  doc["logSBP"]=config.logSBP;
  doc["file_date_time"]=config.file_date_time;
  doc["timezone"]=config.timezone;         doc["timezone_DST"]=config.timezone_DST;
  doc["track_distance"]=config.track_distance;

  // strings (only if non-empty)
  if(config.speed_screen[0])  doc["speed_screen"]=config.speed_screen;
  if(config.stat_screen[0])   doc["stat_screen"]=config.stat_screen;
  if(config.gpio12_screen[0]) doc["gpio12_screen"]=config.gpio12_screen;
  if(config.Sleep_info[0])    doc["Sleep_info"]=config.Sleep_info;
  if(config.UBXfile[0])       doc["UBXfile"]=config.UBXfile;

  serializeJsonPretty(doc,file);
}

// -----------------------------------------------------------------------------
// Sanitize screen strings (digits only)
// -----------------------------------------------------------------------------
static void sanitizeScreenString(char *dst,size_t dstSize,const char *src)
{
  if(!dst||dstSize<2) return;
  size_t w=0;
  for(size_t r=0;src&&src[r];++r)
    if(src[r]>='0'&&src[r]<='9'){
      if(w<dstSize-1) dst[w++]=src[r];
      else break;
    }
  dst[w]='\0';
}

// -----------------------------------------------------------------------------
// Validate / heal critical fields
// -----------------------------------------------------------------------------
static void validateConfig()
{
  // screen sequences (never empty)
  if(!config.speed_screen[0]){
    LOG_CONFIG("CONFIG","speed_screen empty → defaulting to '1'");
    strlcpy(config.speed_screen,"1",sizeof(config.speed_screen));
  }
  if(!config.gpio12_screen[0]){
    LOG_CONFIG("CONFIG","gpio12_screen empty → defaulting to '4'");
    strlcpy(config.gpio12_screen,"4",sizeof(config.gpio12_screen));
  }
  if(!config.stat_screen[0]){
    LOG_CONFIG("CONFIG","stat_screen empty → defaulting to '12'");
    strlcpy(config.stat_screen,"12",sizeof(config.stat_screen));
  }

  // first speed digit sanity
  if(config.speed_screen[0]<'0'||config.speed_screen[0]>'9'){
    LOG_CONFIG("CONFIG","speed_screen[0] invalid → forcing '1'");
    strlcpy(config.speed_screen,"1",sizeof(config.speed_screen));
  }

  // numeric sanity
  if(config.track_distance<=0){
    LOG_CONFIG("CONFIG","track_distance invalid → defaulting to 1852");
    config.track_distance=1852;
  }
  if(config.sample_rate!=1&&config.sample_rate!=5&&config.sample_rate!=10){
    LOG_CONFIG("CONFIG","sample_rate invalid → defaulting to 5");
    config.sample_rate=5;
  }
  if(config.cpu_freq!=80&&config.cpu_freq!=160&&config.cpu_freq!=240){
    LOG_CONFIG("CONFIG","cpu_freq invalid → defaulting to 80");
    config.cpu_freq=80;
  }
  if(config.bar_length<=0){
    LOG_CONFIG("CONFIG","bar_length invalid → defaulting to 1852");
    config.bar_length=1852;
  }
}








// -----------------------------------------------------------------------------
// Apply derived runtime values
// -----------------------------------------------------------------------------
static void applyDerivedConfig()
{
  LOG_CONFIG("Apply", "Derived runtime values");

  RTC_minimum_voltage_bat = config.shutdown_voltage;
  strcpy(RTC_Sleep_txt, config.Sleep_info);

  RTC_SLEEP_screen = config.sleep_off_screen % 10;
  RTC_OFF_screen   = (config.sleep_off_screen / 10) % 10;

  // ---------------------------------------------------------------------------
  // FIX: never allow negative counts (when strings are short)
  // ---------------------------------------------------------------------------
  const int statLen  = (int)strlen(config.stat_screen);
  const int speedLen = (int)strlen(config.speed_screen);
  const int gpioLen  = (int)strlen(config.gpio12_screen);

  config.screen_count = (statLen  > 0) ? (statLen  - 1) : 0;
  config.speed_count  = (speedLen > 0) ? (speedLen - 1) : 0;
  config.gpio12_count = (gpioLen  > 0) ? (gpioLen  - 1) : 0;

  TimeZone_env(config.timezone);
}

// -----------------------------------------------------------------------------
// TimeZone_env (existing behaviour retained)
// -----------------------------------------------------------------------------
void TimeZone_env(float timezone)
{
  int hours = (int)(timezone);
  int minutes = abs((int)(timezone * 60) % 60);

  char time_noDST[64] = "GMT0";

  if (hours > 0) {
    sprintf(time_noDST, "CET-%d:%02d", hours, minutes);
  } else {
    sprintf(time_noDST, "CET+%d:%02d", -hours, minutes);
  }

  strcpy(TimeZone, time_noDST);

  if (config.timezone_DST) {
    switch ((int)(timezone * 100)) {
      case 0:    strcpy(TimeZone,"GMT0BST,M3.5.0/1,M10.5.0"); break;
      case 100:  strcpy(TimeZone,"CET-1CEST,M3.5.0,M10.5.0/3"); break;
      case 200:  strcpy(TimeZone,"EET-2EEST,M3.5.0,M10.5.0/3"); break;
      case 300:  strcpy(TimeZone,"<-03>3<-02>,M3.2.0,M11.1.0"); break;
      case 500:  strcpy(TimeZone,"CST5CDT,M3.2.0/0,M11.1.0/1"); break;
      case 600:  strcpy(TimeZone,"CST6CDT,M3.2.0,M11.1.0"); break;
      case 700:  strcpy(TimeZone,"MST7MDT,M3.2.0,M11.1.0"); break;
      case 800:  strcpy(TimeZone,"PST8PDT,M3.2.0,M11.1.0"); break;
      case 950:  strcpy(TimeZone,"ACST-9:30ACDT,M10.1.0,M4.1.0/3"); break;
      case 1000: strcpy(TimeZone,"AEST-10AEDT,M10.1.0,M4.1.0/3"); break;
      case 1050: strcpy(TimeZone,"<+1030>-10:30<+11>-11,M10.1.0,M4.1.0"); break;
      case 1200: strcpy(TimeZone,"NZST-12NZDT,M9.5.0,M4.1.0/3"); break;
      case -100: strcpy(TimeZone,"<-01>1<+00>,M3.5.0/0,M10.5.0/1"); break;
      case -200: strcpy(TimeZone,"IST-2IDT,M3.4.4/26,M10.5.0"); break;
    }
  }
}

// -----------------------------------------------------------------------------
// Config dump (SERIAL DIAGNOSTICS)
// -----------------------------------------------------------------------------
static void dumpConfig()
{
  Serial.println();
  Serial.println("[CONFIG ] Dump ----------------------------");

  Serial.print("[CONFIG ] cal_bat              = "); Serial.println(config.cal_bat);
  Serial.print("[CONFIG ] shutdown_voltage     = "); Serial.println(config.shutdown_voltage);
  Serial.print("[CONFIG ] cal_speed            = "); Serial.println(config.cal_speed);
  Serial.print("[CONFIG ] sample_rate          = "); Serial.println(config.sample_rate);
  Serial.print("[CONFIG ] cpu_freq             = "); Serial.println(config.cpu_freq);
  Serial.print("[CONFIG ] bar_length           = "); Serial.println(config.bar_length);
  Serial.print("[CONFIG ] Stat_screens         = "); Serial.println(config.Stat_screens);
  Serial.print("[CONFIG ] stat_speed           = "); Serial.println(config.stat_speed);
  Serial.print("[CONFIG ] archive_days         = "); Serial.println(config.archive_days);

  Serial.print("[CONFIG ] sleep_off_screen     = "); Serial.println(config.sleep_off_screen);
  Serial.print("[CONFIG ] logTXT               = "); Serial.println(config.logTXT);
  Serial.print("[CONFIG ] logUBX               = "); Serial.println(config.logUBX);
  Serial.print("[CONFIG ] logSBP               = "); Serial.println(config.logSBP);

  Serial.print("[CONFIG ] file_date_time       = "); Serial.println(config.file_date_time);
  Serial.print("[CONFIG ] timezone             = "); Serial.println(config.timezone);
  Serial.print("[CONFIG ] timezone_DST         = "); Serial.println(config.timezone_DST);
  Serial.print("[CONFIG ] track_distance       = "); Serial.println(config.track_distance);

  Serial.print("[CONFIG ] speed_screen         = "); Serial.println(config.speed_screen);
  Serial.print("[CONFIG ] stat_screen          = "); Serial.println(config.stat_screen);
  Serial.print("[CONFIG ] gpio12_screen        = "); Serial.println(config.gpio12_screen);
  Serial.print("[CONFIG ] Sleep_info           = "); Serial.println(config.Sleep_info);

  Serial.println("[CONFIG ] ---------------------------------");
  Serial.println();
}
