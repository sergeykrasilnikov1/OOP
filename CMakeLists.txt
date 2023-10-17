cmake_minimum_required(VERSION 3.20)
project(SoundProcessor)

set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
#find_package(SndFile)
add_executable(SoundProcessor main.cpp
        sndfile.h)


# Link against libsndfile
target_link_libraries(SoundProcessor "/usr/lib/x86_64-linux-gnu/libsndfile.so.1")
#target_link_libraries(SoundProcessor PRIVATE SndFile::sndfile)
set(CMAKE_CXX_STANDARD 17)