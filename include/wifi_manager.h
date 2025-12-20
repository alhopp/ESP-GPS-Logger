#pragma once
#include <Arduino.h>

void wifi_init();
void wifi_handle();      // called from taskOne

extern bool ap_mode;
