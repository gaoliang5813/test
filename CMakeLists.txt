cmake_minimum_required(VERSION 2.8)
project( main )

#add_executable( main ffmpeg_libavdevice.cpp)
add_executable( main main.cpp filter_video.cpp)

target_link_libraries(main avcodec avformat avdevice avutil avfilter swscale postproc swresample)
