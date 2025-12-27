#include "eeprom_manager.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "ESP_functions.h"
#include "config_manager.h"
#include "rtc_state.h"
#include "Definitions.h"


// ----------------------------------------------------
// EEPROM layout (fixed offsets)
// ----------------------------------------------------
static constexpr uint8_t EEPROM_ADDR_UBLOX_TYPE   = 0;
static constexpr uint8_t EEPROM_ADDR_M10_NAV      = 1;
static constexpr uint8_t EEPROM_ADDR_HIGHEST_READ = 2;

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initEEPROM()
{
  EEPROM.begin(EEPROM_SIZE);

  // --------------------------------------------------
  // u-blox type
  // --------------------------------------------------
  config.ublox_type = EEPROM.readByte(EEPROM_ADDR_UBLOX_TYPE);
  Serial.print(F("EEPROM ublox_type = "));
  Serial.println(config.ublox_type);

  // --------------------------------------------------
  // M10 navigation mode
  // --------------------------------------------------
  config.M10_high_nav = EEPROM.readByte(EEPROM_ADDR_M10_NAV);
  if (config.M10_high_nav > 3) {
    config.M10_high_nav = NO_M10_GPS;
    EEPROM.writeByte(EEPROM_ADDR_M10_NAV, NO_M10_GPS);
    EEPROM.commit();
  }

  // --------------------------------------------------
  // Battery calibration reference
  // --------------------------------------------------
  RTC_highest_read = EEPROM.readInt(EEPROM_ADDR_HIGHEST_READ);
  if (RTC_highest_read < STARTVALUE_HIGHEST_READ ||
      RTC_highest_read > MAXVALUE_HIGHEST_READ) {

    RTC_highest_read = STARTVALUE_HIGHEST_READ;
    EEPROM.writeInt(EEPROM_ADDR_HIGHEST_READ, RTC_highest_read);
    EEPROM.commit();
  }

  RTC_calibration_bat =
    FULLY_CHARGED_LIPO_VOLTAGE / RTC_highest_read;

  Serial.print(F("RTC_calibration_bat = "));
  Serial.println(RTC_calibration_bat);
}
