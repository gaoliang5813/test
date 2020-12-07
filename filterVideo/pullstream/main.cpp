
#include <stdio.h>
#include "mult_video.h"

//mult_flag 2为左右拼接，3为上下拼接，4为四路拼接，后期代码优化需设置为有意义的宏定义
const int muti_flag =4;
const int filter_flag = 0;

int main(int argc, char* argv[])
{
    if(muti_flag == 4){
        mult_video_4();

    }

    if(filter_flag == 1){
        filter_video();
    }

    if(muti_flag == 2) {
        mult_video_left_right();
    }

    if(muti_flag == 3) {
        mult_video_up_down();
    }
    return 0;
}