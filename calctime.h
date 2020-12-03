#ifndef CALCTIME_HPP_
#define CALCTIME_HPP_
#include <stdio.h>

void add_debug_log(const char* plog);
void init_log();
void close_log();

FILE* gLog_File;
#endif // CALCTIME_HPP_
