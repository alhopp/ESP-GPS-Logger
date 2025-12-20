#include <SD_MMC.h>
#include <sd_defines.h>
#include <sd_diskio.h>
#include "wifi_manager.h"
#include "FS.h"
#include "SPI.h"
#include "sys/time.h"
#include "Arduino.h"




#include "Ublox.h"
#include "SD_card.h"
#include <esp_task_wdt.h>
#include "freertos/task.h"//added V3
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <esp32-hal.h>
#include <time.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include "rom/rtc.h"
#include "ESP_functions.h"
#include "E_paper.h"
#include "task_gps.h"


// ----------------------------------------------------
// Forward declarations (required for C++)
// ----------------------------------------------------
static void initSerial();
static void initEEPROM();
static void initCPUAndResetState();
static void initWatchdog();
static void initSPIAndTime();
static void initStorage();
static void initConfig();
static void initBootChecks();
static void startTasks();



extern RTC_DATA_ATTR int RTC_Sail_Logo;
extern RTC_DATA_ATTR char RTC_Sleep_txt[32];


// Pin mapping – adjust per board later
#define EPD_CS   5
#define EPD_DC   17
#define EPD_RST  16
#define EPD_BUSY 4


GxEPD_Class display(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);

const char* ssid = config.ssid; //WiFi SSID
const char* password = config.password; //WiFi Password
const char* ssid2 = config.ssid2; //WiFi SSID
const char* password2 = config.password2; //WiFi Password
const char* soft_ap_ssid = "ESP32AP"; //accespoint ssid
const char* soft_ap_password = "password"; //accespoint password
bool ap_mode=false;
bool sleep_mode=false;
extern bool reset_boot; 


void setup() {
  initSerial();
  initEEPROM();
  initCPUAndResetState();
  initWatchdog();
  initSPIAndTime();
  initStorage();
  initConfig();
  initBootChecks();
  wifi_init();
  startTasks();
}

static void initSerial() {
  Serial.begin(115200);
  Serial.print("Actual CPU freq @ boot ");
  Serial.println(getCpuFrequencyMhz());
}

static void initEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  config.ublox_type = EEPROM.readByte(0);
  Serial.print("EEPROM ublox_type=");
  Serial.println(config.ublox_type);

  config.M10_high_nav = EEPROM.readByte(1);
  if (config.M10_high_nav > 3) {
    config.M10_high_nav = 0;
    EEPROM.writeByte(1, NO_M10_GPS);
    EEPROM.commit();
  }
  Serial.print("EEPROM M10_high_nav=");
  Serial.println(config.M10_high_nav);

  RTC_highest_read = EEPROM.readInt(2);
  if ((RTC_highest_read < STARTVALUE_HIGHEST_READ) ||
      (RTC_highest_read > MAXVALUE_HIGHEST_READ)) {

    EEPROM.writeInt(2, STARTVALUE_HIGHEST_READ);
    EEPROM.commit();
    RTC_highest_read = STARTVALUE_HIGHEST_READ;
    Serial.println("Eeprom highest read set to starting value !!");
  }

  RTC_calibration_bat = FULLY_CHARGED_LIPO_VOLTAGE / RTC_highest_read;
  Serial.print("RTC_calibration_bat EEPROM = ");
  Serial.println(RTC_calibration_bat);
}

static void initCPUAndResetState() {
  pinMode(2, INPUT_PULLUP); // required for SD_MMC mode

  if (reset_boot == true) {
    setCpuFrequencyMhz(80);
  }

  print_wakeup_reason();

  Serial.println("setup Serial");
  Serial.println("Serial Txd is on pin: " + String(TX));
  Serial.println("Serial Rxd is on pin: " + String(RX));
}

static void initWatchdog() {
  Serial.println("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL); // current task
}

static void initSPIAndTime() {
  analog_mean = analogRead(PIN_BAT); // pre-fill FIR filter

  SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);

  struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
  settimeofday(&tv, NULL);
}

static void initStorage() {
  if (!SD_MMC.begin("/sdcard", true)) {
    sdOK = false;
    Serial.println("No SDCard found!");

    if (!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
      Serial.println("LITTLEFS Mount Failed");
      return;
    }

    LITTLEFS_OK = true;
    Serial.print("LITTLEFS Mounted. Total space = ");
    Serial.print(LITTLEFS.totalBytes());
    Serial.println(" bytes");

  } else {
    sdOK = true;
    Serial.println("SDCard found!");

    uint64_t cardSize  = SD_MMC.cardSize() / (1024 * 1024);
    uint64_t totalMB   = SD_MMC.totalBytes() / (1024 * 1024);
    uint64_t usedMB    = SD_MMC.usedBytes() / (1024 * 1024);
    freeSpace = totalMB - usedMB;

    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    Serial.printf("SD Total bytes: %lluMB\n", totalMB);
    Serial.printf("SD Used bytes: %lluMB\n", usedMB);
    Serial.printf("SD free space: %lluMB\n", freeSpace);

    testFileIO(SD_MMC, "/test.txt");
  }
}


  //print_reset_reason(rtc_get_reset_reason(0));//Find out the reset reason, if no SW-reset-> back to deep sleep !
 // RTC_highest_read=EEPROM.readInt(2);
 

static void initConfig() {
  if (sdOK || LITTLEFS_OK) {
    Serial.println(F("Loading configuration..."));
    ensureConfigExistsOnSD();
    loadConfiguration(filename, filename_backup, config);

    Serial.print(F("Print config file..."));
    printFile(filename);
  }
}

static void initBootChecks() {
  Boot_screen();

  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    RTC_OFF_screen = 1;
    logERR("Shutdown low bat");
    strcpy(RTC_Sleep_txt, "Shut down Low Bat !");
    Shut_down();
  }

  if (reset_boot == true) {
    RTC_OFF_screen = 1;
    strcpy(RTC_Sleep_txt, "Shutdown after reset!");
    Shut_down();
  }

  setCpuFrequencyMhz(240);
  Update_screen(BOOT_SCREEN);

  Serial.print("Actual CPU freq before Wifi.begin(): ");
  Serial.println(getCpuFrequencyMhz());
}


static void startTasks() {
  xTaskCreatePinnedToCore(
    taskOne,
    "TaskOne",
    10000,
    NULL,
    1,
    &t1,
    1
  );

  xTaskCreatePinnedToCore(
    taskTwo,
    "TaskTwo",
    20000,
    NULL,
    1,
    &t2,
    0
  );
}

  

 
void loop() { 
  int wdt_task0_duration=millis()-wdt_task0;
  int wdt_task1_duration=millis()-wdt_task1;
  int task_timeout=(WDT_TIMEOUT -1)*1000;//1 second less then reboot timeout 
  if((wdt_task0_duration<task_timeout)&&(wdt_task1_duration<task_timeout)){
    feedTheDog_Task0();
    feedTheDog_Task1();
    }
  if((wdt_task0_duration>task_timeout)&&(downloading_file)&&(max_count_wdt_task0<MAX_COUNT_WDT_TASK0)) {
    max_count_wdt_task0++;
    feedTheDog_Task0();
    wdt_task0=millis();
    Serial.println("Extend watchdog_timeout due long download"); 
    }     
  if((wdt_task0_duration>task_timeout)&&(!downloading_file)) Serial.println("Watchdog task0 triggered");
  if(wdt_task1_duration>task_timeout) Serial.println("Watchdog task1 triggered");
  Update_bat();
  delay(100); 
}


