
#pragma once
#include <FS.h> 

// Declare the file handles as extern so they can be used in other .cpp files
extern File ubxfile;
extern File sbpfile;


void Open_files(void);
void Log_to_SD(void); 
void Flush_files(void);
void Close_files(void);



void logERR(const char *message);






