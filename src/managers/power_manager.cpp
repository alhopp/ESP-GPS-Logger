#include "power_manager.h"
#include <Arduino.h>
#include "Definitions.h"
#include "rtc_state.h"
#include "Globals.h"

// INTERNAL state (owned here)


void Update_bat(void)
{
    analog_bat  = analogRead(PIN_BAT);
    analog_mean = analog_bat * 0.02f + analog_mean * 0.98f;

    RTC_voltage_bat = analog_mean * RTC_calibration_bat / 1000.0f;
}
