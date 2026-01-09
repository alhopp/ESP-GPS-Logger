#pragma once

#include <FS.h>   // File abstraction (File)

// -----------------------------------------------------------------------------
// GPX writer parts
//
// Used to control which section of the GPX file is written when calling log_GPX()
// -----------------------------------------------------------------------------
enum GPX_Part : int {
    GPX_HEADER = 0,   // Write XML + GPX header and <trk><trkseg>
    GPX_FRAME  = 1,   // Write one <trkpt> (logged once per second)
    GPX_END    = 2    // Close </trkseg></trk></gpx>
};

// -----------------------------------------------------------------------------
// log_GPX
//
// Writes GPX-formatted data to an already-open file.
//
// Usage pattern:
//   log_GPX(GPX_HEADER, file);
//   log_GPX(GPX_FRAME,  file);   // repeatedly during logging
//   log_GPX(GPX_END,    file);
//
// Notes:
// - The File must already be opened for writing
// - Uses global u-blox state (ubxMessage)
// - Track points are written only on full-second boundaries
// -----------------------------------------------------------------------------
void log_GPX(GPX_Part part, File file);
