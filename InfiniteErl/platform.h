#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef WINDOWS
    #include "windows.h"
#elif APPLE
#else
#endif

void write_file();
void read_file();
void get_time();
void create_window();
void destroy_window();
void load_assets();

#endif
