
#include "storage_file_operations.h"
 #include "config_manager.h"

 File ubxfile;
 File errorfile;
 File gpyfile;  //new open source file format, work in progress !!
 File sbpfile;
 File gpxfile;


extern struct Config config;  

void Flush_files(void) {
  if (config.sample_rate <= 10) {  //@18Hz still lost points !!!
    static int load_balance = 0;
    if (load_balance == 0) ubxfile.flush();
    if (load_balance == 1) errorfile.flush();
    if (load_balance == 2) gpyfile.flush();
    if (load_balance == 3) sbpfile.flush();
    if (load_balance == 4) {
      gpxfile.flush();
      load_balance = -1;
    }
    load_balance++;
  }
}


