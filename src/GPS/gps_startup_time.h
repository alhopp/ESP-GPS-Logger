#pragma once

// Optional GPS warm-start time injection.
// Called during GPS bring-up after the receiver is powered and configured.
void gps_send_time_from_rtc();
