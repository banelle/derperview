cmake_minimum_required(VERSION 3.10)

project(derperview LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")

file(MAKE_DIRECTORY bin)
include_directories("include")

if(UNIX)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads)
endif()

if(WIN32)
    set(CMAKE_LIBRARY_PATH "${PROJECT_SOURCE_DIR}/lib/ffmpeg-windows/lib;${PROJECT_SOURCE_DIR}/lib/wxwidgets-windows/lib")
    set(DERPERVIEW_FFMPEG_DLL_PATH "${PROJECT_SOURCE_DIR}/lib/ffmpeg-windows/bin")
    set(DERPERVIEW_WXWIDGETS_DLL_PATH "${PROJECT_SOURCE_DIR}/lib/wxwidgets-windows/bin")
    include_directories("${PROJECT_SOURCE_DIR}/lib/ffmpeg-windows/include")
endif()

# try to find libswresample
find_library(LIBSWRESAMPLE swresample)
if (NOT LIBSWRESAMPLE)
    message(FATAL_ERROR "libswresample not found, on debian/ubuntu/mint systems try to install libswresample-dev")
else()
    message(STATUS "using libswresample: ${LIBSWRESAMPLE}")
endif()

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
if (NOT LIBAVUTIL)
    message(FATAL_ERROR "libavutil not found, on debian/ubuntu/mint systems try to install libavutil-dev")
else()
    message(STATUS "using libavutil: ${LIBAVUTIL}")
endif()

find_library(WXWIDGET_BASE_DEBUG wxbase32ud)
find_library(WXWIDGET_CORE_DEBUG wxmsw32ud_core)
find_library(WXWIDGET_BASE_RELEASE wxbase32u)
find_library(WXWIDGET_CORE_RELEASE wxmsw32u_core)

find_package(Git QUIET)
if(NOT GIT_FOUND)
    message(FATAL_ERROR "git not found")
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" describe --tags --always --dirty=-dirty
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_VARIABLE GIT_OUTPUT
    ERROR_VARIABLE GIT_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT GIT_RESULT EQUAL 0)
    message(FATAL_ERROR "cannot git describe: ${GIT_RESULT} - ${GIT_ERROR}")
endif()
set(VERSION "${GIT_OUTPUT}")
message(STATUS "version: ${VERSION}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp)
set(VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp")

add_definitions(-D_UNICODE -DUNICODE)

add_subdirectory(src/libderperview)
add_subdirectory(src/derperview)
add_subdirectory(src/derperview-wx)
