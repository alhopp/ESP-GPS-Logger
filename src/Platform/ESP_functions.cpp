#include "ESP_functions.h"

#include <sys/time.h>   // getLocalTime()
#include <esp_system.h>




// -----------------------------------------------------------------------------
// Utility functions
// -----------------------------------------------------------------------------
void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.print("NTP Time = ");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
