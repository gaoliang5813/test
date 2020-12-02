cmake_minimum_required(VERSION 2.8)
project( main )

add_executable( main main.cpp filter_video.cpp)
add_executable( test filtering_video_print_time.c calctime.c)

target_link_libraries(main avcodec avformat avdevice avutil avfilter swscale postproc swresample)
target_link_libraries(test avcodec avformat avdevice avutil avfilter swscale postproc swresample)