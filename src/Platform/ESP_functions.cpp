#include "ESP_functions.h"

#include <sys/time.h>   // getLocalTime()
#include <esp_system.h>

#include "Definitions.h"


// -----------------------------------------------------------------------------
// Utility functions
// -----------------------------------------------------------------------------
void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        LOG_ERROR("Time", "Failed to obtain time");

        return;
    }
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf),
         "%A, %B %d %Y %H:%M:%S", &timeinfo);

    LOG_SYS("Time", "NTP Time = %s", timebuf);

}
