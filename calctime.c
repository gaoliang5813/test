#include "calctime.h"
#include <stdio.h>
#include <time.h>
void add_debug_log(const char* plog)
{
    float time = clock() - time;   //时间间隔
    float cost_time = 1000 * (double) time / CLOCKS_PER_SEC;
    printf("%s %f ms\n", plog,cost_time);
    time = clock();
}




