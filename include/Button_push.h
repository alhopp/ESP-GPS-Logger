#pragma once
#include <Arduino.h>

class Button_push {
public:
  Button_push(int GPIO_pin,
              int push_time,
              int long_pulse_time,
              int max_count,
              bool default_state);

  void begin(int GPIO_pin, bool default_state);
  bool Button_pushed();

  bool isLongPulse() const { return long_pulse; }

private:
  int Input_pin;
  bool Default_state;
  unsigned long push_millis = 0;
  unsigned long millis_10s = 0;

  int time_out_millis;
  int max_pulse_time;
  int max_button_count;

  bool return_value = false;
  bool long_pulse = false;
  int button_count = 0;
  int old_button_status = 0;
};
