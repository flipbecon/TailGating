cmake_minimum_required(VERSION 3.6)

project(TailGating)

# rtsp://admin:admin@192.168.19.101:554/cam/realmonitor?channel=1&subtype=0

set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g -ggdb -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -std=c++11 -O3")

add_definitions(-Wdeprecated-declarations -Wformat=0)

include_directories(${CMAKE_SOURCE_DIR}/include/ffmpeg)
include_directories(${CMAKE_SOURCE_DIR}/include/f12)

add_subdirectory(src/f12)

link_directories(/usr/lib/x86_64-linux-gnu)

SET(FFMPEG_PATH lib/ffmpeg)

message(${FFMPEG_PATH})

add_library(libavcodec STATIC IMPORTED)
set_target_properties(libavcodec PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${FFMPEG_PATH}/libavcodec.a)

add_library(libavdevice STATIC IMPORTED)
set_target_properties(libavdevice PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${FFMPEG_PATH}/libavdevice.a)

add_library(libavfilter STATIC IMPORTED)
set_target_properties(libavfilter PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${FFMPEG_PATH}/libavfilter.a)

add_library(libavformat STATIC IMPORTED)
set_target_properties(libavformat PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${FFMPEG_PATH}/libavformat.a)

add_library(libavutil STATIC IMPORTED)
set_target_properties(libavutil PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${FFMPEG_PATH}/libavutil.a)

add_library(libswresample STATIC IMPORTED)
set_target_properties(libswresample PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${FFMPEG_PATH}/libswresample.a)

add_library(libswscale STATIC IMPORTED)
set_target_properties(libswscale PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${FFMPEG_PATH}/libswscale.a)

add_executable(TailGating main.cpp)

target_link_libraries(TailGating f12 libavdevice libavformat libavcodec libavfilter libavutil libswresample libswscale
        -lpthread -llzma -lz -lm -ldl -lpthread -lrt)