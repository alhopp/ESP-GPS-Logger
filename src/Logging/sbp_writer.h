#pragma once
#include <FS.h>

void sbp_write_header(File& file);
void sbp_writer_reset();
void sbp_write_frame(File& file);
int sbp_writer_frame_count();
