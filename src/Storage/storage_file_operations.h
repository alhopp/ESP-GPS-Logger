

#include <FS.h>
#include <LittleFS.h>

// Declare the file handles as extern so they can be used in other .cpp files
extern File ubxfile;
extern File errorfile;
extern File gpyfile;
extern File sbpfile;
extern File gpxfile;


void Open_files(void);
void Log_to_SD(void); 
void Flush_files(void);
void Close_files(void);

void printFile(const char *filename);

void logERR(const char *message);






