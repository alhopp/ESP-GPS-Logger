#pragma once

#include <FS.h>

void session_raw_writers_reset();
void session_write_ubx(File& ubxfile);
void session_write_sbp(File& sbpfile);
