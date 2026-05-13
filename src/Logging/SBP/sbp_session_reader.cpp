// ============================================================================
// sbp_session_reader.cpp
//
// Reads completed project SBP session logs using the canonical SBP frame layout.
// ============================================================================

#include "Logging/SBP/sbp_session_reader.h"

#include "Storage/storage_manager.h"

bool sbp_session_open(File& file, const char* sbpPath)
{
  fs::FS& storage = storage_sd_fs();
  file = storage.open(sbpPath, FILE_READ);
  if (!file) return false;
  if (file.size() <= SBP_HEADER_SIZE) {
    file.close();
    return false;
  }
  file.seek(SBP_HEADER_SIZE);
  return true;
}

bool sbp_session_read_frame(File& file, SbpFrame& frame)
{
  return file.read(reinterpret_cast<uint8_t*>(&frame), sizeof(frame)) == sizeof(frame);
}

bool sbp_session_seek_frame(File& file, int sbpIndex)
{
  if (!file || sbpIndex < 1) return false;

  const size_t offset =
      SBP_HEADER_SIZE + static_cast<size_t>(sbpIndex - 1) * sizeof(SbpFrame);
  if (offset + sizeof(SbpFrame) > file.size()) return false;

  return file.seek(offset);
}

bool sbp_session_read_frame_at(File& file, int sbpIndex, SbpFrame& frame)
{
  if (!sbp_session_seek_frame(file, sbpIndex)) return false;
  return sbp_session_read_frame(file, frame);
}

int sbp_session_count_frames(const char* sbpPath)
{
  fs::FS& storage = storage_sd_fs();
  File file = storage.open(sbpPath, FILE_READ);
  if (!file) return 0;

  const size_t size = file.size();
  file.close();

  if (size <= SBP_HEADER_SIZE) return 0;
  return (size - SBP_HEADER_SIZE) / sizeof(SbpFrame);
}
