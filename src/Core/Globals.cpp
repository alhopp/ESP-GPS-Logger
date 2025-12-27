#include <Arduino.h>
#include "Globals.h"
#include "Definitions.h"

// Put your actual declarations exactly as in original

bool wifi_configured = false;
bool downloading_file = false;



Button_push Short_push12 (12, 50,   15, 1, 1);
Button_push Long_push12  (12, 2000, 10, 4, 1);

Button_push Short_push19 (GO_TO_SLEEP_PULLDOWN, 10,   10, 9, 0);
Button_push Long_push19  (GO_TO_SLEEP_PULLDOWN, 1700, 10, 9, 0);

Button_push Short_push39 (GO_TO_SLEEP_GPIO, 10,   10, 9, 1);
Button_push Long_push39  (GO_TO_SLEEP_GPIO, 1700, 10, 9, 1);

bool GPS_Signal_OK = false;
bool Field_choice  = false;

byte mac[6] = {0};   // ESP32 MAC address

const char SW_version[16]="Ver 6.01c";


