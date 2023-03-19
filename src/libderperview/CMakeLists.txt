add_library(lib${CMAKE_PROJECT_NAME} STATIC Entry.cpp Process.cpp Video.cpp ${VERSION_FILE})

if (UNIX)
    target_link_libraries(lib${CMAKE_PROJECT_NAME} Threads::Threads ${LIBAVUTIL} ${LIBAVCODEC} ${LIBAVFORMAT} ${LIBSWRESAMPLE})
else()
    target_link_libraries(lib${CMAKE_PROJECT_NAME} ${LIBAVUTIL} ${LIBAVCODEC} ${LIBAVFORMAT} ${LIBSWRESAMPLE})
endif()
