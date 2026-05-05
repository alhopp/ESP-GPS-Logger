#include "Config/config_runtime.h"

#include <Arduino.h>

#include "Core/Definitions.h"
#include "Core/Globals.h"
#include "Core/rtc_state.h"
#include "Config/config_types.h"

void config_apply_runtime()
{
  LOG_CONFIG("Apply", "Derived runtime values");

  RTC_minimum_voltage_bat = config.shutdown_voltage;
  TimeZone_env(config.timezone);
}

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
      case 0:    strcpy(TimeZone, "GMT0BST,M3.5.0/1,M10.5.0"); break;
      case 100:  strcpy(TimeZone, "CET-1CEST,M3.5.0,M10.5.0/3"); break;
      case 200:  strcpy(TimeZone, "EET-2EEST,M3.5.0,M10.5.0/3"); break;
      case 300:  strcpy(TimeZone, "<-03>3<-02>,M3.2.0,M11.1.0"); break;
      case 500:  strcpy(TimeZone, "CST5CDT,M3.2.0/0,M11.1.0/1"); break;
      case 600:  strcpy(TimeZone, "CST6CDT,M3.2.0,M11.1.0"); break;
      case 700:  strcpy(TimeZone, "MST7MDT,M3.2.0,M11.1.0"); break;
      case 800:  strcpy(TimeZone, "PST8PDT,M3.2.0,M11.1.0"); break;
      case 950:  strcpy(TimeZone, "ACST-9:30ACDT,M10.1.0,M4.1.0/3"); break;
      case 1000: strcpy(TimeZone, "AEST-10AEDT,M10.1.0,M4.1.0/3"); break;
      case 1050: strcpy(TimeZone, "<+1030>-10:30<+11>-11,M10.1.0,M4.1.0"); break;
      case 1200: strcpy(TimeZone, "NZST-12NZDT,M9.5.0,M4.1.0/3"); break;
      case -100: strcpy(TimeZone, "<-01>1<+00>,M3.5.0/0,M10.5.0/1"); break;
      case -200: strcpy(TimeZone, "IST-2IDT,M3.4.4/26,M10.5.0"); break;
    }
  }
}
