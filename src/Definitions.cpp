#include "Definitions.h"

BootMode bootMode = MODE_RUN;   // ✅ single definition

// Short press buttons
Button_push Short_push12 (12, 50, 15, 1, 1);
Button_push Short_push19 (GO_TO_SLEEP_PULLDOWN, 10, 10, 9, 0);
Button_push Short_push39 (GO_TO_SLEEP_GPIO, 10, 10, 9, 1);

// Long press buttons
Button_push Long_push12  (12, 2000, 10, 4, 1);
Button_push Long_push19  (GO_TO_SLEEP_PULLDOWN, 1700, 10, 9, 0);
Button_push Long_push39  (GO_TO_SLEEP_GPIO, 1700, 10, 9, 1);