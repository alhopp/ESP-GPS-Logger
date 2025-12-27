#include "Button_push.h"

Button_push::Button_push(int GPIO_pin,
                         int push_time,
                         int long_pulse_time,
                         int max_count,
                         bool default_state)
{
  Input_pin = GPIO_pin;
  Default_state = default_state;
  time_out_millis = push_time;
  max_pulse_time = long_pulse_time;
  max_button_count = max_count;
}

void Button_push::begin(int GPIO_pin, bool default_state)
{
  if (default_state)
    pinMode(GPIO_pin, INPUT_PULLUP);
  else
    pinMode(GPIO_pin, INPUT_PULLDOWN);
}

bool Button_push::Button_pushed()
{
  return_value = false;
  int button_status = digitalRead(Input_pin);

  if (button_status == Default_state)
    push_millis = millis();

  if ((millis() - push_millis > time_out_millis) &&
      (old_button_status == 0)) {

    button_count++;
    if (button_count > max_button_count)
      button_count = 0;

    old_button_status = 1;
    millis_10s = millis();
    return_value = true;
  }

  long_pulse = (millis() - millis_10s) < (1000 * max_pulse_time);

  if (button_status == Default_state)
    old_button_status = 0;

  return return_value;
}
