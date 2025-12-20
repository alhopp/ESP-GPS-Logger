#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t t2;
void taskTwo(void* parameter);

