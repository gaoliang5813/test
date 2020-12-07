cmake_minimum_required(VERSION 3.11)
project(pullstream)

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(/usr/lib64)

set(CMAKE_CXX_STANDARD 14)

add_executable(pullstream main.cpp mult_video.cpp mult_video.h filter_video.cpp)

target_link_libraries(pullstream avcodec avformat avdevice avutil avfilter swscale postproc swresample)
