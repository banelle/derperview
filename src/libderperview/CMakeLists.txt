add_library(lib${CMAKE_PROJECT_NAME} STATIC Entry.cpp Process.cpp GpuProcess.cpp Video.cpp Process.hpp GpuProcess.hpp Video.hpp ${VERSION_FILE})

if (UNIX)
    target_link_libraries(lib${CMAKE_PROJECT_NAME} Threads::Threads ${LIBAVUTIL} ${LIBAVCODEC} ${LIBAVFORMAT} ${LIBSWRESAMPLE} ${LIBOPENCL})
else()
    target_link_libraries(lib${CMAKE_PROJECT_NAME} ${LIBAVUTIL} ${LIBAVCODEC} ${LIBAVFORMAT} ${LIBSWRESAMPLE} ${LIBOPENCL})
endif()
