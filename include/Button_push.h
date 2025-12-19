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
    boolean Button_pushed(void);

    boolean long_pulse;
    int button_count;

  private:
    boolean button_status;
    boolean old_button_status;
    boolean return_value;
    boolean Default_state;

    int Input_pin;
    int push_millis;
    int time_out_millis;
    int millis_10s;
    int max_pulse_time;
    int max_button_count;
};


