#pragma once

#include <FS.h>

void logging_raw_writers_reset();
void logging_raw_writers_write_ubx(File& ubxfile);
void logging_raw_writers_write_sbp(File& sbpfile);
