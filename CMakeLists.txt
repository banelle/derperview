cmake_minimum_required(VERSION 3.10)

project(derperview VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)

# try to find libswscale
find_library(LIBSWSCALE swscale)
if (NOT LIBSWSCALE)
    message(FATAL_ERROR "libswscale not found, on debian/ubuntu/mint systems try to install libswscale-dev")
else()
    message(STATUS "using libswscale: ${LIBSWSCALE}")
endif()

# try to find libavformat
find_library(LIBAVFORMAT avformat)
if (NOT LIBAVFORMAT)
    message(FATAL_ERROR "libavformat not found, on debian/ubuntu/mint systems try to install libavformat-dev")
else()
    message(STATUS "using libavformat: ${LIBAVFORMAT}")
endif()

# try to find libavcodec
find_library(LIBAVCODEC avcodec)
if (NOT LIBAVCODEC)
    message(FATAL_ERROR "libavcodec not found, on debian/ubuntu/mint systems try to install libavcodec-dev")
else()
    message(STATUS "using libavcodec: ${LIBAVCODEC}")
endif()

# try to find libavutil
find_library(LIBAVUTIL avutil)
if (NOT LIBAVFORMAT)
    message(FATAL_ERROR "libavutil not found, on debian/ubuntu/mint systems try to install libavutil-dev")
else()
    message(STATUS "using libavutil: ${LIBAVFORMAT}")
endif()

add_subdirectory(src)

