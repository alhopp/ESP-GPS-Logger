#pragma once
#include <Arduino.h>

void wifi_init();
void wifi_handle();      // called from taskOne
bool wifi_is_connected();
