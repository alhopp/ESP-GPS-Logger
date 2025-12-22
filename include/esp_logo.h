#pragma once
#include <Arduino.h>

// Explicit sizes to avoid confusion
#define ESP_LOGO_48_W 48
#define ESP_LOGO_48_H 48
#define ESP_LOGO_40_W 40
#define ESP_LOGO_40_H 40

extern const uint8_t ESP_GPS_logo_48[304];
extern const uint8_t ESP_GPS_logo_40[200];

// Drawing helpers
void drawEspLogoCentered();
void drawEspLogo48(int16_t x, int16_t y);
void drawEspLogo40(int16_t x, int16_t y);
