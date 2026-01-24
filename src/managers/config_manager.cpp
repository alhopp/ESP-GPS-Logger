
// -----------------------------------------------------------------------------
// Configuration Manager
//
// Loads / validates config from LittleFS (/config.txt), creates defaults if
// missing or invalid, applies derived runtime values, and heals fragile fields
//
// Must run after storage init and before Wi-Fi, logging, or tasks.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "Definitions.h"
#include "MANAGERS/config_manager.h"
#include "rtc_state.h"
#include "Globals.h"

#include "system_info.h"

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

  config.track_distance      = 1852;

  // -------- UI / behaviour --------
  config.bar_length          = 1852;

  // -------- System --------
  config.timezone            = 1.0f;
  config.timezone_DST        = 1;

  // -------- Logging --------
  config.logUBX              = 0;
  config.logSBP              = 1;

  //-------- Performance screens (on/off) --------------------------  
  config.stat_alpha     = true;
  config.stat_nm        = true;
  config.stat_1h        = true;
  config.stat_2s        = false;
  config.stat_10s       = false;
  config.stat_distance  = false;

  // -------- Other strings --------
  strlcpy(config.Sleep_info, "ESP32 GPS", sizeof(config.Sleep_info));

  strlcpy(config.home_ssid,   "",          sizeof(config.home_ssid));
  strlcpy(config.home_pass,   "",          sizeof(config.home_pass));

  strlcpy(config.phone_ssid,  "",          sizeof(config.phone_ssid));
  strlcpy(config.phone_pass,  "",          sizeof(config.phone_pass));

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

  config.bar_length          =doc["bar_length"]          |config.bar_length;

  config.logUBX              =doc["logUBX"]              |config.logUBX;
  config.logSBP              =doc["logSBP"]              |config.logSBP;
  config.timezone            =doc["timezone"]            |config.timezone;
  config.timezone_DST        =doc["timezone_DST"]        |config.timezone_DST;
  config.track_distance      =doc["track_distance"]      |config.track_distance;

  // Performance screens (on/off)
  config.stat_2s        = doc["stat_2s"]        | config.stat_2s;
  config.stat_10s       = doc["stat_10s"]       | config.stat_10s;
  config.stat_alpha     = doc["stat_alpha"]     | config.stat_alpha;
  config.stat_nm        = doc["stat_nm"]        | config.stat_nm;
  config.stat_1h        = doc["stat_1h"]        | config.stat_1h;
  config.stat_distance  = doc["stat_distance"]  | config.stat_distance;

  // strings (only if non-empty)
  const char* s;
  if((s=doc["Sleep_info"])     && s[0]) strlcpy(config.Sleep_info,s,sizeof(config.Sleep_info));

  // WiFi Details
  if((s=doc["home_ssid"])  && s[0]) strlcpy(config.home_ssid, s, sizeof(config.home_ssid));
  if((s=doc["home_pass"])  && s[0]) strlcpy(config.home_pass, s, sizeof(config.home_pass));
  if((s=doc["phone_ssid"]) && s[0]) strlcpy(config.phone_ssid,s, sizeof(config.phone_ssid));
  if((s=doc["phone_pass"]) && s[0]) strlcpy(config.phone_pass,s, sizeof(config.phone_pass));
  return true;
}

// -----------------------------------------------------------------------------
// Write config to file
// -----------------------------------------------------------------------------
static void writeConfigToFile(File &file)
{
  StaticJsonDocument<1536> doc;

 // numeric / boolean
  doc["cal_bat"]          = config.cal_bat;
  doc["shutdown_voltage"] = config.shutdown_voltage;

  doc["bar_length"]       = config.bar_length;

  doc["logUBX"]           = config.logUBX;
  doc["logSBP"]           = config.logSBP;

  doc["timezone"]         = config.timezone;
  doc["timezone_DST"]     = config.timezone_DST;
  doc["track_distance"]   = config.track_distance;

  // Performance screens (on/off)
  doc["stat_2s"]        = config.stat_2s;
  doc["stat_10s"]       = config.stat_10s;
  doc["stat_alpha"]     = config.stat_alpha;
  doc["stat_nm"]        = config.stat_nm;
  doc["stat_1h"]        = config.stat_1h;
  doc["stat_distance"]  = config.stat_distance;

  // strings (only if non-empty)
  if(config.Sleep_info[0])    doc["Sleep_info"]=config.Sleep_info;

   // Wi-Fi roles
  if(config.home_ssid[0])   doc["home_ssid"]  = config.home_ssid;
  if(config.home_pass[0])   doc["home_pass"]  = config.home_pass;
  if(config.phone_ssid[0])  doc["phone_ssid"] = config.phone_ssid;
  if(config.phone_pass[0])  doc["phone_pass"] = config.phone_pass;

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
 
  // numeric sanity
  if(config.track_distance<=0){
    LOG_CONFIG("CONFIG","track_distance invalid → defaulting to 1852");
    config.track_distance=1852;
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
// Config dump (JSON-loaded fields + SystemInfo)
// -----------------------------------------------------------------------------
static void dumpConfig()
{
  Serial.println();
  Serial.println("[CONFIG ] ===== Loaded from JSON =====");

  // -------- numeric / boolean --------
  Serial.print("[CONFIG ] cal_bat          = "); Serial.println(config.cal_bat);
  Serial.print("[CONFIG ] shutdown_voltage = "); Serial.println(config.shutdown_voltage);

  Serial.print("[CONFIG ] bar_length       = "); Serial.println(config.bar_length);

  Serial.print("[CONFIG ] logUBX           = "); Serial.println(config.logUBX);
  Serial.print("[CONFIG ] logSBP           = "); Serial.println(config.logSBP);

  Serial.print("[CONFIG ] timezone         = "); Serial.println(config.timezone);
  Serial.print("[CONFIG ] timezone_DST     = "); Serial.println(config.timezone_DST);
  Serial.print("[CONFIG ] track_distance   = "); Serial.println(config.track_distance);

  // -------- performance screen toggles --------
  Serial.println("[CONFIG ] Performance screens");
  Serial.print("[CONFIG ]   2s             = "); Serial.println(config.stat_2s);
  Serial.print("[CONFIG ]   10s            = "); Serial.println(config.stat_10s);
  Serial.print("[CONFIG ]   alpha          = "); Serial.println(config.stat_alpha);
  Serial.print("[CONFIG ]   nm             = "); Serial.println(config.stat_nm);
  Serial.print("[CONFIG ]   1h             = "); Serial.println(config.stat_1h);
  Serial.print("[CONFIG ]   distance       = "); Serial.println(config.stat_distance);

  // -------- strings --------
  Serial.print("[CONFIG ] Sleep_info       = "); Serial.println(config.Sleep_info);

  // -------- Wi-Fi --------
  Serial.println("[CONFIG ] Wi-Fi");
  Serial.print  ("[CONFIG ]   home_ssid      = ");
  Serial.println(config.home_ssid[0] ? config.home_ssid : "(not set)");

  Serial.print  ("[CONFIG ]   home_pass      = ");
  Serial.println(config.home_pass[0] ? "***" : "(not set)");

  Serial.print  ("[CONFIG ]   phone_ssid     = ");
  Serial.println(config.phone_ssid[0] ? config.phone_ssid : "(not set)");

  Serial.print  ("[CONFIG ]   phone_pass     = ");
  Serial.println(config.phone_pass[0] ? "***" : "(not set)");

  
  // ---------------------------------------------------------------------------
  // SystemInfo (static / runtime)
  // ---------------------------------------------------------------------------
  Serial.println();
  Serial.println("[SYSTEM ] ===== Static system info =====");

  Serial.print("[SYSTEM ] gnss_module      = "); Serial.println(systemInfo.gnss_module);
  Serial.print("[SYSTEM ] gnss_mode        = "); Serial.println(systemInfo.gnss_mode);
  Serial.print("[SYSTEM ] dynamic_model    = "); Serial.println(systemInfo.dynamic_model);
  Serial.print("[SYSTEM ] sample_rate      = "); Serial.println(systemInfo.sample_rate);
  Serial.print("[SYSTEM ] speed_units      = "); Serial.println(systemInfo.speed_units);
  Serial.print("[SYSTEM ] cal_speed        = "); Serial.println(systemInfo.cal_speed);

  Serial.print("[SYSTEM ] storage_mb       = "); Serial.println(systemInfo.storage_mb);
  Serial.print("[SYSTEM ] display          = "); Serial.println(systemInfo.display);
  Serial.print("[SYSTEM ] cpu_freq         = "); Serial.println(systemInfo.cpu_freq);
  Serial.print("[SYSTEM ] software_version = "); Serial.println(systemInfo.software_version);

  Serial.println("=======================================");
  Serial.println();
}
