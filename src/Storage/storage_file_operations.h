

#include <FS.h>
#include <LittleFS.h>

// Declare the file handles as extern so they can be used in other .cpp files
extern File ubxfile;
extern File errorfile;
extern File gpyfile;
extern File sbpfile;
extern File gpxfile;

void Flush_files(void);

