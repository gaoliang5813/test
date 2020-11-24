cmake_minimum_required(VERSION 2.8)
project( main )

#add_executable( main main.cpp)
add_executable( main main.cpp filtering_video.c)

target_link_libraries(main avcodec avformat avdevice avutil avfilter swscale postproc swresample)
