// Globals.h
#pragma once
#include <Arduino.h>
#include "Button_push.h"

// Button instances (runtime globals)
extern Button_push Short_push12;
extern Button_push Long_push12;

extern Button_push Short_push19;
extern Button_push Long_push19;

extern Button_push Short_push39;
extern Button_push Long_push39;


extern bool GPS_Signal_OK;
extern bool Field_choice;



extern byte mac[6];

extern const char SW_version[16];

